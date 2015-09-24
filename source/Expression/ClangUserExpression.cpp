//===-- ClangUserExpression.cpp ---------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include <stdio.h>
#if HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif

#include <cstdlib>
#include <string>
#include <map>

#include "lldb/Core/ConstString.h"
#include "lldb/Core/Log.h"
#include "lldb/Core/Module.h"
#include "lldb/Core/StreamFile.h"
#include "lldb/Core/StreamString.h"
#include "lldb/Core/ValueObjectConstResult.h"
#include "lldb/Expression/ASTResultSynthesizer.h"
#include "lldb/Expression/ClangExpressionDeclMap.h"
#include "lldb/Expression/ClangExpressionParser.h"
#include "lldb/Expression/ClangModulesDeclVendor.h"
#include "lldb/Expression/ClangPersistentVariables.h"
#include "lldb/Expression/ClangUserExpression.h"
#include "lldb/Expression/ExpressionSourceCode.h"
#include "lldb/Expression/IRExecutionUnit.h"
#include "lldb/Expression/IRInterpreter.h"
#include "lldb/Expression/Materializer.h"
#include "lldb/Host/HostInfo.h"
#include "lldb/Symbol/Block.h"
#include "lldb/Symbol/ClangASTContext.h"
#include "lldb/Symbol/Function.h"
#include "lldb/Symbol/ObjectFile.h"
#include "lldb/Symbol/SymbolVendor.h"
#include "lldb/Symbol/Type.h"
#include "lldb/Symbol/ClangExternalASTSourceCommon.h"
#include "lldb/Symbol/VariableList.h"
#include "lldb/Target/ExecutionContext.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/StackFrame.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/ThreadPlan.h"
#include "lldb/Target/ThreadPlanCallUserExpression.h"

#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclObjC.h"

using namespace lldb_private;

ClangUserExpression::ClangUserExpression (ExecutionContextScope &exe_scope,
                                          const char *expr,
                                          const char *expr_prefix,
                                          lldb::LanguageType language,
                                          ResultType desired_type) :
    UserExpression (exe_scope, expr, expr_prefix, language, desired_type),
    m_stack_frame_bottom (LLDB_INVALID_ADDRESS),
    m_stack_frame_top (LLDB_INVALID_ADDRESS),
    m_transformed_text (),
    m_execution_unit_sp(),
    m_materializer_ap(),
    m_jit_module_wp(),
    m_enforce_valid_object (true),
    m_in_cplusplus_method (false),
    m_in_objectivec_method (false),
    m_in_static_method(false),
    m_needs_object_ptr (false),
    m_const_object (false),
    m_target (NULL),
    m_can_interpret (false),
    m_materialized_address (LLDB_INVALID_ADDRESS),
    m_type_system_helper(*m_target_wp.lock().get())
{
    switch (m_language)
    {
    case lldb::eLanguageTypeC_plus_plus:
        m_allow_cxx = true;
        break;
    case lldb::eLanguageTypeObjC:
        m_allow_objc = true;
        break;
    case lldb::eLanguageTypeObjC_plus_plus:
    default:
        m_allow_cxx = true;
        m_allow_objc = true;
        break;
    }
}

ClangUserExpression::~ClangUserExpression ()
{
    if (m_target)
    {
        lldb::ModuleSP jit_module_sp (m_jit_module_wp.lock());
        if (jit_module_sp)
            m_target->GetImages().Remove(jit_module_sp);
    }
}

void
ClangUserExpression::ScanContext(ExecutionContext &exe_ctx, Error &err)
{
    Log *log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_EXPRESSIONS));

    if (log)
        log->Printf("ClangUserExpression::ScanContext()");

    m_target = exe_ctx.GetTargetPtr();

    if (!(m_allow_cxx || m_allow_objc))
    {
        if (log)
            log->Printf("  [CUE::SC] Settings inhibit C++ and Objective-C");
        return;
    }

    StackFrame *frame = exe_ctx.GetFramePtr();
    if (frame == NULL)
    {
        if (log)
            log->Printf("  [CUE::SC] Null stack frame");
        return;
    }

    SymbolContext sym_ctx = frame->GetSymbolContext(lldb::eSymbolContextFunction | lldb::eSymbolContextBlock);

    if (!sym_ctx.function)
    {
        if (log)
            log->Printf("  [CUE::SC] Null function");
        return;
    }

    // Find the block that defines the function represented by "sym_ctx"
    Block *function_block = sym_ctx.GetFunctionBlock();

    if (!function_block)
    {
        if (log)
            log->Printf("  [CUE::SC] Null function block");
        return;
    }

    CompilerDeclContext decl_context = function_block->GetDeclContext();

    if (!decl_context)
    {
        if (log)
            log->Printf("  [CUE::SC] Null decl context");
        return;
    }

    if (clang::CXXMethodDecl *method_decl = ClangASTContext::DeclContextGetAsCXXMethodDecl(decl_context))
    {
        if (m_allow_cxx && method_decl->isInstance())
        {
            if (m_enforce_valid_object)
            {
                lldb::VariableListSP variable_list_sp (function_block->GetBlockVariableList (true));

                const char *thisErrorString = "Stopped in a C++ method, but 'this' isn't available; pretending we are in a generic context";

                if (!variable_list_sp)
                {
                    err.SetErrorString(thisErrorString);
                    return;
                }

                lldb::VariableSP this_var_sp (variable_list_sp->FindVariable(ConstString("this")));

                if (!this_var_sp ||
                    !this_var_sp->IsInScope(frame) ||
                    !this_var_sp->LocationIsValidForFrame (frame))
                {
                    err.SetErrorString(thisErrorString);
                    return;
                }
            }

            m_in_cplusplus_method = true;
            m_needs_object_ptr = true;
        }
    }
    else if (clang::ObjCMethodDecl *method_decl = ClangASTContext::DeclContextGetAsObjCMethodDecl(decl_context))
    {
        if (m_allow_objc)
        {
            if (m_enforce_valid_object)
            {
                lldb::VariableListSP variable_list_sp (function_block->GetBlockVariableList (true));

                const char *selfErrorString = "Stopped in an Objective-C method, but 'self' isn't available; pretending we are in a generic context";

                if (!variable_list_sp)
                {
                    err.SetErrorString(selfErrorString);
                    return;
                }

                lldb::VariableSP self_variable_sp = variable_list_sp->FindVariable(ConstString("self"));

                if (!self_variable_sp ||
                    !self_variable_sp->IsInScope(frame) ||
                    !self_variable_sp->LocationIsValidForFrame (frame))
                {
                    err.SetErrorString(selfErrorString);
                    return;
                }
            }

            m_in_objectivec_method = true;
            m_needs_object_ptr = true;

            if (!method_decl->isInstanceMethod())
                m_in_static_method = true;
        }
    }
    else if (clang::FunctionDecl *function_decl = ClangASTContext::DeclContextGetAsFunctionDecl(decl_context))
    {
        // We might also have a function that said in the debug information that it captured an
        // object pointer.  The best way to deal with getting to the ivars at present is by pretending
        // that this is a method of a class in whatever runtime the debug info says the object pointer
        // belongs to.  Do that here.

        ClangASTMetadata *metadata = ClangASTContext::DeclContextGetMetaData (decl_context, function_decl);
        if (metadata && metadata->HasObjectPtr())
        {
            lldb::LanguageType language = metadata->GetObjectPtrLanguage();
            if (language == lldb::eLanguageTypeC_plus_plus)
            {
                if (m_enforce_valid_object)
                {
                    lldb::VariableListSP variable_list_sp (function_block->GetBlockVariableList (true));

                    const char *thisErrorString = "Stopped in a context claiming to capture a C++ object pointer, but 'this' isn't available; pretending we are in a generic context";

                    if (!variable_list_sp)
                    {
                        err.SetErrorString(thisErrorString);
                        return;
                    }

                    lldb::VariableSP this_var_sp (variable_list_sp->FindVariable(ConstString("this")));

                    if (!this_var_sp ||
                        !this_var_sp->IsInScope(frame) ||
                        !this_var_sp->LocationIsValidForFrame (frame))
                    {
                        err.SetErrorString(thisErrorString);
                        return;
                    }
                }

                m_in_cplusplus_method = true;
                m_needs_object_ptr = true;
            }
            else if (language == lldb::eLanguageTypeObjC)
            {
                if (m_enforce_valid_object)
                {
                    lldb::VariableListSP variable_list_sp (function_block->GetBlockVariableList (true));

                    const char *selfErrorString = "Stopped in a context claiming to capture an Objective-C object pointer, but 'self' isn't available; pretending we are in a generic context";

                    if (!variable_list_sp)
                    {
                        err.SetErrorString(selfErrorString);
                        return;
                    }

                    lldb::VariableSP self_variable_sp = variable_list_sp->FindVariable(ConstString("self"));

                    if (!self_variable_sp ||
                        !self_variable_sp->IsInScope(frame) ||
                        !self_variable_sp->LocationIsValidForFrame (frame))
                    {
                        err.SetErrorString(selfErrorString);
                        return;
                    }

                    Type *self_type = self_variable_sp->GetType();

                    if (!self_type)
                    {
                        err.SetErrorString(selfErrorString);
                        return;
                    }

                    CompilerType self_clang_type = self_type->GetForwardCompilerType ();

                    if (!self_clang_type)
                    {
                        err.SetErrorString(selfErrorString);
                        return;
                    }

                    if (ClangASTContext::IsObjCClassType(self_clang_type))
                    {
                        return;
                    }
                    else if (ClangASTContext::IsObjCObjectPointerType(self_clang_type))
                    {
                        m_in_objectivec_method = true;
                        m_needs_object_ptr = true;
                    }
                    else
                    {
                        err.SetErrorString(selfErrorString);
                        return;
                    }
                }
                else
                {
                    m_in_objectivec_method = true;
                    m_needs_object_ptr = true;
                }
            }
        }
    }
}

// This is a really nasty hack, meant to fix Objective-C expressions of the form
// (int)[myArray count].  Right now, because the type information for count is
// not available, [myArray count] returns id, which can't be directly cast to
// int without causing a clang error.
static void
ApplyObjcCastHack(std::string &expr)
{
#define OBJC_CAST_HACK_FROM "(int)["
#define OBJC_CAST_HACK_TO   "(int)(long long)["

    size_t from_offset;

    while ((from_offset = expr.find(OBJC_CAST_HACK_FROM)) != expr.npos)
        expr.replace(from_offset, sizeof(OBJC_CAST_HACK_FROM) - 1, OBJC_CAST_HACK_TO);

#undef OBJC_CAST_HACK_TO
#undef OBJC_CAST_HACK_FROM
}

bool
ClangUserExpression::Parse (Stream &error_stream,
                            ExecutionContext &exe_ctx,
                            lldb_private::ExecutionPolicy execution_policy,
                            bool keep_result_in_memory,
                            bool generate_debug_info)
{
    Log *log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_EXPRESSIONS));

    Error err;

    InstallContext(exe_ctx);

    ScanContext(exe_ctx, err);

    if (!err.Success())
    {
        error_stream.Printf("warning: %s\n", err.AsCString());
    }

    StreamString m_transformed_stream;

    ////////////////////////////////////
    // Generate the expression
    //

    ApplyObjcCastHack(m_expr_text);
    //ApplyUnicharHack(m_expr_text);

    std::string prefix = m_expr_prefix;
    
    if (ClangModulesDeclVendor *decl_vendor = m_target->GetClangModulesDeclVendor())
    {
        const ClangModulesDeclVendor::ModuleVector &hand_imported_modules = m_target->GetPersistentVariables().GetHandLoadedClangModules();
        ClangModulesDeclVendor::ModuleVector modules_for_macros;
        
        for (ClangModulesDeclVendor::ModuleID module : hand_imported_modules)
        {
            modules_for_macros.push_back(module);
        }

        if (m_target->GetEnableAutoImportClangModules())
        {
            if (StackFrame *frame = exe_ctx.GetFramePtr())
            {
                if (Block *block = frame->GetFrameBlock())
                {
                    SymbolContext sc;
                    
                    block->CalculateSymbolContext(&sc);
                    
                    if (sc.comp_unit)
                    {
                        StreamString error_stream;
                        
                        decl_vendor->AddModulesForCompileUnit(*sc.comp_unit, modules_for_macros, error_stream);
                    }
                }
            }
        }
    }
    
    std::unique_ptr<ExpressionSourceCode> source_code (ExpressionSourceCode::CreateWrapped(prefix.c_str(), m_expr_text.c_str()));
    
    lldb::LanguageType lang_type;

    if (m_in_cplusplus_method)
        lang_type = lldb::eLanguageTypeC_plus_plus;
    else if (m_in_objectivec_method)
        lang_type = lldb::eLanguageTypeObjC;
    else
        lang_type = lldb::eLanguageTypeC;

    if (!source_code->GetText(m_transformed_text, lang_type, m_const_object, m_in_static_method, exe_ctx))
    {
        error_stream.PutCString ("error: couldn't construct expression body");
        return false;
    }

    if (log)
        log->Printf("Parsing the following code:\n%s", m_transformed_text.c_str());

    ////////////////////////////////////
    // Set up the target and compiler
    //

    Target *target = exe_ctx.GetTargetPtr();

    if (!target)
    {
        error_stream.PutCString ("error: invalid target\n");
        return false;
    }

    //////////////////////////
    // Parse the expression
    //

    m_materializer_ap.reset(new Materializer());

    ResetDeclMap(exe_ctx, keep_result_in_memory);

    class OnExit
    {
    public:
        typedef std::function <void (void)> Callback;

        OnExit (Callback const &callback) :
            m_callback(callback)
        {
        }

        ~OnExit ()
        {
            m_callback();
        }
    private:
        Callback m_callback;
    };

    OnExit on_exit([this]() { ResetDeclMap(); });

    if (!DeclMap()->WillParse(exe_ctx, m_materializer_ap.get()))
    {
        error_stream.PutCString ("error: current process state is unsuitable for expression parsing\n");

        ResetDeclMap(); // We are being careful here in the case of breakpoint conditions.

        return false;
    }

    Process *process = exe_ctx.GetProcessPtr();
    ExecutionContextScope *exe_scope = process;

    if (!exe_scope)
        exe_scope = exe_ctx.GetTargetPtr();

    ClangExpressionParser parser(exe_scope, *this, generate_debug_info);

    unsigned num_errors = parser.Parse (error_stream);

    if (num_errors)
    {
        error_stream.Printf ("error: %d errors parsing expression\n", num_errors);

        ResetDeclMap(); // We are being careful here in the case of breakpoint conditions.

        return false;
    }

    //////////////////////////////////////////////////////////////////////////////////////////
    // Prepare the output of the parser for execution, evaluating it statically if possible
    //

    Error jit_error = parser.PrepareForExecution (m_jit_start_addr,
                                                  m_jit_end_addr,
                                                  m_execution_unit_sp,
                                                  exe_ctx,
                                                  m_can_interpret,
                                                  execution_policy);

    if (generate_debug_info)
    {
        lldb::ModuleSP jit_module_sp ( m_execution_unit_sp->GetJITModule());

        if (jit_module_sp)
        {
            ConstString const_func_name(FunctionName());
            FileSpec jit_file;
            jit_file.GetFilename() = const_func_name;
            jit_module_sp->SetFileSpecAndObjectName (jit_file, ConstString());
            m_jit_module_wp = jit_module_sp;
            target->GetImages().Append(jit_module_sp);
        }
//        lldb_private::ObjectFile *jit_obj_file = jit_module_sp->GetObjectFile();
//        StreamFile strm (stdout, false);
//        if (jit_obj_file)
//        {
//            jit_obj_file->GetSectionList();
//            jit_obj_file->GetSymtab();
//            jit_obj_file->Dump(&strm);
//        }
//        lldb_private::SymbolVendor *jit_sym_vendor = jit_module_sp->GetSymbolVendor();
//        if (jit_sym_vendor)
//        {
//            lldb_private::SymbolContextList sc_list;
//            jit_sym_vendor->FindFunctions(const_func_name, NULL, lldb::eFunctionNameTypeFull, true, false, sc_list);
//            sc_list.Dump(&strm, target);
//            jit_sym_vendor->Dump(&strm);
//        }
    }

    ResetDeclMap(); // Make this go away since we don't need any of its state after parsing.  This also gets rid of any ClangASTImporter::Minions.

    if (jit_error.Success())
    {
        if (process && m_jit_start_addr != LLDB_INVALID_ADDRESS)
            m_jit_process_wp = lldb::ProcessWP(process->shared_from_this());
        return true;
    }
    else
    {
        const char *error_cstr = jit_error.AsCString();
        if (error_cstr && error_cstr[0])
            error_stream.Printf ("error: %s\n", error_cstr);
        else
            error_stream.Printf ("error: expression can't be interpreted or run\n");
        return false;
    }
}

bool
ClangUserExpression::AddInitialArguments (ExecutionContext &exe_ctx,
                                          std::vector<lldb::addr_t> &args,
                                          Stream &error_stream)
{
    lldb::addr_t object_ptr = LLDB_INVALID_ADDRESS;
    lldb::addr_t cmd_ptr    = LLDB_INVALID_ADDRESS;
    
    if (m_needs_object_ptr)
    {
        lldb::StackFrameSP frame_sp = exe_ctx.GetFrameSP();
        if (!frame_sp)
            return true;
        
        ConstString object_name;

        if (m_in_cplusplus_method)
        {
            object_name.SetCString("this");
        }
        else if (m_in_objectivec_method)
        {
            object_name.SetCString("self");
        }
        else
        {
            error_stream.Printf("Need object pointer but don't know the language\n");
            return false;
        }

        Error object_ptr_error;

        object_ptr = GetObjectPointer(frame_sp, object_name, object_ptr_error);

        if (!object_ptr_error.Success())
        {
            error_stream.Printf("warning: couldn't get required object pointer (substituting NULL): %s\n", object_ptr_error.AsCString());
            object_ptr = 0;
        }

        if (m_in_objectivec_method)
        {
            ConstString cmd_name("_cmd");

            cmd_ptr = GetObjectPointer(frame_sp, cmd_name, object_ptr_error);

            if (!object_ptr_error.Success())
            {
                error_stream.Printf("warning: couldn't get cmd pointer (substituting NULL): %s\n", object_ptr_error.AsCString());
                cmd_ptr = 0;
            }
        }
        if (object_ptr)
            args.push_back(object_ptr);

        if (m_in_objectivec_method)
            args.push_back(cmd_ptr);


    }
    return true;
}

void
ClangUserExpression::ClangUserExpressionHelper::ResetDeclMap(ExecutionContext &exe_ctx, bool keep_result_in_memory)
{
    m_expr_decl_map_up.reset(new ClangExpressionDeclMap(keep_result_in_memory, exe_ctx));
}

clang::ASTConsumer *
ClangUserExpression::ClangUserExpressionHelper::ASTTransformer (clang::ASTConsumer *passthrough)
{
    m_result_synthesizer_up.reset(new ASTResultSynthesizer(passthrough,
                                                           m_target));

    return m_result_synthesizer_up.get();
}

lldb::ExpressionResults
ClangUserExpression::Execute (Stream &error_stream,
                         ExecutionContext &exe_ctx,
                         const EvaluateExpressionOptions& options,
                         lldb::UserExpressionSP &shared_ptr_to_me,
                         lldb::ExpressionVariableSP &result)
{
    // The expression log is quite verbose, and if you're just tracking the execution of the
    // expression, it's quite convenient to have these logs come out with the STEP log as well.
    Log *log(lldb_private::GetLogIfAnyCategoriesSet (LIBLLDB_LOG_EXPRESSIONS | LIBLLDB_LOG_STEP));
    
    if (m_jit_start_addr != LLDB_INVALID_ADDRESS || m_can_interpret)
    {
        lldb::addr_t struct_address = LLDB_INVALID_ADDRESS;
        
        if (!PrepareToExecuteJITExpression (error_stream, exe_ctx, struct_address))
        {
            error_stream.Printf("Errored out in %s, couldn't PrepareToExecuteJITExpression", __FUNCTION__);
            return lldb::eExpressionSetupError;
        }
        
        lldb::addr_t function_stack_bottom = LLDB_INVALID_ADDRESS;
        lldb::addr_t function_stack_top = LLDB_INVALID_ADDRESS;
        
        if (m_can_interpret)
        {
            llvm::Module *module = m_execution_unit_sp->GetModule();
            llvm::Function *function = m_execution_unit_sp->GetFunction();
            
            if (!module || !function)
            {
                error_stream.Printf("Supposed to interpret, but nothing is there");
                return lldb::eExpressionSetupError;
            }
            
            Error interpreter_error;
            
            std::vector<lldb::addr_t> args;
            
            if (!AddInitialArguments(exe_ctx, args, error_stream))
            {
                error_stream.Printf ("Errored out in %s, couldn't AddInitialArguments", __FUNCTION__);
                return lldb::eExpressionSetupError;
            }
            
            args.push_back(struct_address);
            
            function_stack_bottom = m_stack_frame_bottom;
            function_stack_top = m_stack_frame_top;
            
            IRInterpreter::Interpret (*module,
                                      *function,
                                      args,
                                      *m_execution_unit_sp.get(),
                                      interpreter_error,
                                      function_stack_bottom,
                                      function_stack_top,
                                      exe_ctx);
            
            if (!interpreter_error.Success())
            {
                error_stream.Printf("Supposed to interpret, but failed: %s", interpreter_error.AsCString());
                return lldb::eExpressionDiscarded;
            }
        }
        else
        {
            if (!exe_ctx.HasThreadScope())
            {
                error_stream.Printf("UserExpression::Execute called with no thread selected.");
                return lldb::eExpressionSetupError;
            }
            
            Address wrapper_address (m_jit_start_addr);
            
            std::vector<lldb::addr_t> args;
            
            if (!AddInitialArguments(exe_ctx, args, error_stream))
            {
                error_stream.Printf ("Errored out in %s, couldn't AddInitialArguments", __FUNCTION__);
                return lldb::eExpressionSetupError;
            }
            
            args.push_back(struct_address);
            
            lldb::ThreadPlanSP call_plan_sp(new ThreadPlanCallUserExpression (exe_ctx.GetThreadRef(),
                                                                              wrapper_address,
                                                                              args,
                                                                              options,
                                                                              shared_ptr_to_me));
            
            if (!call_plan_sp || !call_plan_sp->ValidatePlan (&error_stream))
                return lldb::eExpressionSetupError;
            
            ThreadPlanCallUserExpression *user_expression_plan = static_cast<ThreadPlanCallUserExpression *>(call_plan_sp.get());
            
            lldb::addr_t function_stack_pointer = user_expression_plan->GetFunctionStackPointer();
            
            function_stack_bottom = function_stack_pointer - HostInfo::GetPageSize();
            function_stack_top = function_stack_pointer;
            
            if (log)
                log->Printf("-- [UserExpression::Execute] Execution of expression begins --");
            
            if (exe_ctx.GetProcessPtr())
                exe_ctx.GetProcessPtr()->SetRunningUserExpression(true);
            
            lldb::ExpressionResults execution_result = exe_ctx.GetProcessRef().RunThreadPlan (exe_ctx,
                                                                                              call_plan_sp,
                                                                                              options,
                                                                                              error_stream);
            
            if (exe_ctx.GetProcessPtr())
                exe_ctx.GetProcessPtr()->SetRunningUserExpression(false);
            
            if (log)
                log->Printf("-- [UserExpression::Execute] Execution of expression completed --");
            
            if (execution_result == lldb::eExpressionInterrupted || execution_result == lldb::eExpressionHitBreakpoint)
            {
                const char *error_desc = NULL;
                
                if (call_plan_sp)
                {
                    lldb::StopInfoSP real_stop_info_sp = call_plan_sp->GetRealStopInfo();
                    if (real_stop_info_sp)
                        error_desc = real_stop_info_sp->GetDescription();
                }
                if (error_desc)
                    error_stream.Printf ("Execution was interrupted, reason: %s.", error_desc);
                else
                    error_stream.PutCString ("Execution was interrupted.");
                
                if ((execution_result == lldb::eExpressionInterrupted && options.DoesUnwindOnError())
                    || (execution_result == lldb::eExpressionHitBreakpoint && options.DoesIgnoreBreakpoints()))
                    error_stream.PutCString ("\nThe process has been returned to the state before expression evaluation.");
                else
                {
                    if (execution_result == lldb::eExpressionHitBreakpoint)
                        user_expression_plan->TransferExpressionOwnership();
                    error_stream.PutCString ("\nThe process has been left at the point where it was interrupted, "
                                             "use \"thread return -x\" to return to the state before expression evaluation.");
                }
                
                return execution_result;
            }
            else if (execution_result == lldb::eExpressionStoppedForDebug)
            {
                error_stream.PutCString ("Execution was halted at the first instruction of the expression "
                                         "function because \"debug\" was requested.\n"
                                         "Use \"thread return -x\" to return to the state before expression evaluation.");
                return execution_result;
            }
            else if (execution_result != lldb::eExpressionCompleted)
            {
                error_stream.Printf ("Couldn't execute function; result was %s\n", Process::ExecutionResultAsCString (execution_result));
                return execution_result;
            }
        }
        
        if  (FinalizeJITExecution (error_stream, exe_ctx, result, function_stack_bottom, function_stack_top))
        {
            return lldb::eExpressionCompleted;
        }
        else
        {
            return lldb::eExpressionResultUnavailable;
        }
    }
    else
    {
        error_stream.Printf("Expression can't be run, because there is no JIT compiled function");
        return lldb::eExpressionSetupError;
    }
}

bool
ClangUserExpression::FinalizeJITExecution (Stream &error_stream,
                                      ExecutionContext &exe_ctx,
                                      lldb::ExpressionVariableSP &result,
                                      lldb::addr_t function_stack_bottom,
                                      lldb::addr_t function_stack_top)
{
    Log *log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_EXPRESSIONS));
    
    if (log)
        log->Printf("-- [UserExpression::FinalizeJITExecution] Dematerializing after execution --");
    
    if (!m_dematerializer_sp)
    {
        error_stream.Printf ("Couldn't apply expression side effects : no dematerializer is present");
        return false;
    }
    
    Error dematerialize_error;
    
    m_dematerializer_sp->Dematerialize(dematerialize_error, result, function_stack_bottom, function_stack_top);
    
    if (!dematerialize_error.Success())
    {
        error_stream.Printf ("Couldn't apply expression side effects : %s\n", dematerialize_error.AsCString("unknown error"));
        return false;
    }
    
    if (result)
        result->TransferAddress();
    
    m_dematerializer_sp.reset();
    
    return true;
}

bool
ClangUserExpression::PrepareToExecuteJITExpression (Stream &error_stream,
                                                    ExecutionContext &exe_ctx,
                                                    lldb::addr_t &struct_address)
{
    lldb::TargetSP target;
    lldb::ProcessSP process;
    lldb::StackFrameSP frame;
    
    if (!LockAndCheckContext(exe_ctx,
                             target,
                             process,
                             frame))
    {
        error_stream.Printf("The context has changed before we could JIT the expression!\n");
        return false;
    }
    
    if (m_jit_start_addr != LLDB_INVALID_ADDRESS || m_can_interpret)
    {
        if (m_materialized_address == LLDB_INVALID_ADDRESS)
        {
            Error alloc_error;
            
            IRMemoryMap::AllocationPolicy policy = m_can_interpret ? IRMemoryMap::eAllocationPolicyHostOnly : IRMemoryMap::eAllocationPolicyMirror;
            
            m_materialized_address = m_execution_unit_sp->Malloc(m_materializer_ap->GetStructByteSize(),
                                                                 m_materializer_ap->GetStructAlignment(),
                                                                 lldb::ePermissionsReadable | lldb::ePermissionsWritable,
                                                                 policy,
                                                                 alloc_error);
            
            if (!alloc_error.Success())
            {
                error_stream.Printf("Couldn't allocate space for materialized struct: %s\n", alloc_error.AsCString());
                return false;
            }
        }
        
        struct_address = m_materialized_address;
        
        if (m_can_interpret && m_stack_frame_bottom == LLDB_INVALID_ADDRESS)
        {
            Error alloc_error;
            
            const size_t stack_frame_size = 512 * 1024;
            
            m_stack_frame_bottom = m_execution_unit_sp->Malloc(stack_frame_size,
                                                               8,
                                                               lldb::ePermissionsReadable | lldb::ePermissionsWritable,
                                                               IRMemoryMap::eAllocationPolicyHostOnly,
                                                               alloc_error);
            
            m_stack_frame_top = m_stack_frame_bottom + stack_frame_size;
            
            if (!alloc_error.Success())
            {
                error_stream.Printf("Couldn't allocate space for the stack frame: %s\n", alloc_error.AsCString());
                return false;
            }
        }
        
        Error materialize_error;
        
        m_dematerializer_sp = m_materializer_ap->Materialize(frame, *m_execution_unit_sp, struct_address, materialize_error);
        
        if (!materialize_error.Success())
        {
            error_stream.Printf("Couldn't materialize: %s\n", materialize_error.AsCString());
            return false;
        }
    }
    return true;
}


