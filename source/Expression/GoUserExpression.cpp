//===-- GoUserExpression.cpp ---------------------------------*- C++ -*-===//
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

#include "lldb/lldb-private.h"
#include "lldb/Core/ConstString.h"
#include "lldb/Core/DataBufferHeap.h"
#include "lldb/Core/DataEncoder.h"
#include "lldb/Core/DataExtractor.h"
#include "lldb/Core/Log.h"
#include "lldb/Core/Module.h"
#include "lldb/Core/StreamFile.h"
#include "lldb/Core/StreamString.h"
#include "lldb/Core/ValueObjectConstResult.h"
#include "lldb/Expression/ClangExpression.h"
#include "lldb/Expression/ClangPersistentVariables.h"
#include "lldb/Expression/GoAST.h"
#include "lldb/Expression/GoParser.h"
#include "lldb/Expression/GoUserExpression.h"
#include "lldb/Symbol/TypeList.h"
#include "lldb/Symbol/VariableList.h"
#include "lldb/Target/ExecutionContext.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/StackFrame.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/ThreadPlan.h"
#include "lldb/Target/ThreadPlanCallUserExpression.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringMap.h"

using namespace lldb_private;
using namespace lldb;

namespace {
class GoInterpreter {
public:
    GoInterpreter(lldb::StackFrameSP frame, const char* expr, DynamicValueType use_dynamic, Error& error) : m_frame(frame), m_parser(expr), m_use_dynamic(use_dynamic), m_error(error) {
        const SymbolContext& ctx = frame->GetSymbolContext(eSymbolContextFunction);
        ConstString fname = ctx.GetFunctionName();
        if (fname.GetLength() > 0)
        {
            size_t dot = fname.GetStringRef().find('.');
            if (dot != llvm::StringRef::npos)
                m_package = llvm::StringRef(fname.AsCString(), dot);
        }
    }
    
    lldb::ValueObjectSP Evaluate();
    lldb::ValueObjectSP EvaluateStatement(const GoASTStmt* s);
    lldb::ValueObjectSP EvaluateExpr(const GoASTExpr* e);
    
    ValueObjectSP VisitBadExpr(const GoASTBadExpr* e)
    {
        m_parser.GetError(m_error);
        return nullptr;
    }
    ValueObjectSP VisitParenExpr(const GoASTParenExpr* e);
    ValueObjectSP VisitIdent(const GoASTIdent* e);
    ValueObjectSP VisitStarExpr(const GoASTStarExpr* e);
    ValueObjectSP VisitSelectorExpr(const GoASTSelectorExpr* e);
    ValueObjectSP VisitBasicLit(const GoASTBasicLit* e);
    ValueObjectSP VisitIndexExpr(const GoASTIndexExpr* e);
    ValueObjectSP VisitUnaryExpr(const GoASTUnaryExpr* e);
    ValueObjectSP VisitCallExpr(const GoASTCallExpr* e);
    ValueObjectSP VisitTypeAssertExpr(const GoASTTypeAssertExpr* e) { return NotImplemented(e); }
    ValueObjectSP VisitBinaryExpr(const GoASTBinaryExpr* e) { return NotImplemented(e); }
    ValueObjectSP VisitArrayType(const GoASTArrayType* e) { return NotImplemented(e); }
    ValueObjectSP VisitChanType(const GoASTChanType* e) { return NotImplemented(e); }
    ValueObjectSP VisitCompositeLit(const GoASTCompositeLit* e) { return NotImplemented(e); }
    ValueObjectSP VisitEllipsis(const GoASTEllipsis* e) { return NotImplemented(e); }
    ValueObjectSP VisitFuncType(const GoASTFuncType* e) { return NotImplemented(e); }
    ValueObjectSP VisitFuncLit(const GoASTFuncLit* e) { return NotImplemented(e); }
    ValueObjectSP VisitInterfaceType(const GoASTInterfaceType* e) { return NotImplemented(e); }
    ValueObjectSP VisitKeyValueExpr(const GoASTKeyValueExpr* e) { return NotImplemented(e); }
    ValueObjectSP VisitMapType(const GoASTMapType* e) { return NotImplemented(e); }
    ValueObjectSP VisitSliceExpr(const GoASTSliceExpr* e) { return NotImplemented(e); }
    ValueObjectSP VisitStructType(const GoASTStructType* e) { return NotImplemented(e); }

    ClangASTType EvaluateType(const GoASTExpr* e);

private:
    std::nullptr_t NotImplemented(const GoASTExpr* e)
    {
        m_error.SetErrorStringWithFormat("%s node not implemented", e->GetKindName());
        return nullptr;
    }
    
    lldb::StackFrameSP m_frame;
    GoParser m_parser;
    DynamicValueType m_use_dynamic;
    Error& m_error;
    llvm::StringRef m_package;
};
    
VariableSP
FindGlobalVariable(TargetSP target, llvm::Twine name)
{
    ConstString fullname(name.str());
    VariableList variable_list;
    const bool append = true;
    if (!target) {
        return nullptr;
    }
    const uint32_t match_count = target->GetImages().FindGlobalVariables (fullname,
                                                                          append,
                                                                          1,
                                                                          variable_list);
    if (match_count == 1)
    {
        return variable_list.GetVariableAtIndex(0);
    }
    return nullptr;
}
    
ClangASTType LookupType(TargetSP target, ConstString name)
{
    if (!target)
        return ClangASTType();
    SymbolContext sc;
    TypeList type_list;
    uint32_t num_matches = target->GetImages().FindTypes (sc,
                                                          name,
                                                          false,
                                                          2,
                                                          type_list);
    if (num_matches > 0) {
        return type_list.GetTypeAtIndex(0)->GetClangFullType();
    }
    return ClangASTType();

}
}  // namespace

lldb::ExpressionResults
GoUserExpression::Evaluate (ExecutionContext &exe_ctx,
                            const EvaluateExpressionOptions& options,
                            const char *expr_cstr,
                            const char *expr_prefix,
                            lldb::ValueObjectSP &result_valobj_sp,
                            Error &error)
{
    Log *log(lldb_private::GetLogIfAnyCategoriesSet (LIBLLDB_LOG_EXPRESSIONS | LIBLLDB_LOG_STEP));
    
    lldb_private::ExecutionPolicy execution_policy = options.GetExecutionPolicy();
    lldb::ExpressionResults execution_results = lldb::eExpressionSetupError;
    
    Process *process = exe_ctx.GetProcessPtr();
    
    if (process == NULL || process->GetState() != lldb::eStateStopped)
    {
        if (execution_policy == eExecutionPolicyAlways)
        {
            if (log)
                log->Printf("== [GoUserExpression::Evaluate] Expression may not run, but is not constant ==");
            
            error.SetErrorString ("expression needed to run but couldn't");
            
            return execution_results;
        }
    }
    
    if (process == NULL || !process->CanJIT())
        execution_policy = eExecutionPolicyNever;
    GoInterpreter expr(exe_ctx.GetFrameSP(), expr_cstr, options.GetUseDynamic(), error);
    result_valobj_sp = expr.Evaluate();
    
    if (result_valobj_sp.get() == NULL)
    {
        result_valobj_sp = ValueObjectConstResult::Create (exe_ctx.GetBestExecutionContextScope(), error);
        return lldb::eExpressionParseError;
    }
    execution_results = lldb::eExpressionCompleted;
    
    return execution_results;
}

ValueObjectSP
GoInterpreter::Evaluate()
{
    ValueObjectSP result;
    for (std::unique_ptr<GoASTStmt> stmt(m_parser.Statement()); stmt; stmt.reset(m_parser.Statement()))
    {
        if (m_parser.Failed())
            break;
        result = EvaluateStatement(stmt.get());
        if (m_error.Fail())
            return nullptr;
    }
    if (m_parser.Failed() || !m_parser.AtEOF())
        m_parser.GetError(m_error);
    return result;
}

ValueObjectSP
GoInterpreter::EvaluateStatement(const lldb_private::GoASTStmt *stmt)
{
    ValueObjectSP result;
    switch (stmt->GetKind())
    {
        case GoASTNode::eBlockStmt:
        {
            const GoASTBlockStmt* block = llvm::cast<GoASTBlockStmt>(stmt);
            for (size_t i = 0; i < block->NumList(); ++i)
                result = EvaluateStatement(block->GetList(i));
            break;
        }
        case GoASTNode::eBadStmt:
            m_parser.GetError(m_error);
            break;
        case GoASTNode::eExprStmt:
        {
            const GoASTExprStmt* expr = llvm::cast<GoASTExprStmt>(stmt);
            return EvaluateExpr(expr->GetX());
        }
        default:
            m_error.SetErrorStringWithFormat("%s node not supported", stmt->GetKindName());
    }
    return result;
}

ValueObjectSP
GoInterpreter::EvaluateExpr(const lldb_private::GoASTExpr *e)
{
    if (e)
        return e->Visit<ValueObjectSP>(this);
    return ValueObjectSP();
}

ValueObjectSP
GoInterpreter::VisitParenExpr(const lldb_private::GoASTParenExpr *e)
{
    return EvaluateExpr(e->GetX());
}

ValueObjectSP
GoInterpreter::VisitIdent(const GoASTIdent* e)
{
    VariableSP var_sp;
    ValueObjectSP val = m_frame->GetValueForVariableExpressionPath(e->GetName().m_value.str().c_str(), m_use_dynamic, 0, var_sp, m_error);
    if (m_error.Fail())
    {
        m_error.Clear();
        TargetSP target = m_frame->CalculateTarget();
        if (!target) {
            m_error.SetErrorString("No target");
            return nullptr;
        }
        var_sp = FindGlobalVariable(target, m_package + "." + e->GetName().m_value);
        if (var_sp)
            return m_frame->GetValueObjectForFrameVariable(var_sp, m_use_dynamic);
    }
    if (!val)
        m_error.SetErrorStringWithFormat("Unknown variable %s", e->GetName().m_value.str().c_str());
    return val;
}


ValueObjectSP
GoInterpreter::VisitStarExpr(const GoASTStarExpr* e)
{
    ValueObjectSP target = EvaluateExpr(e->GetX());
    if (!target)
        return nullptr;
    return target->Dereference(m_error);
}

ValueObjectSP
GoInterpreter::VisitSelectorExpr(const lldb_private::GoASTSelectorExpr *e)
{
    ValueObjectSP target = EvaluateExpr(e->GetX());
    if (target)
    {
        if (target->GetClangType().IsPointerType())
        {
            target = target->Dereference(m_error);
            if (m_error.Fail())
                return nullptr;
        }
        ConstString field(e->GetSel()->GetName().m_value);
        ValueObjectSP result = target->GetChildMemberWithName(field, true);
        if (!result)
            m_error.SetErrorStringWithFormat("Unknown child %s", field.AsCString());
        return result;
    }
    if (const GoASTIdent* package = llvm::dyn_cast<GoASTIdent>(e->GetX())) {
        if (VariableSP global = FindGlobalVariable(m_frame->CalculateTarget(),
                                                   package->GetName().m_value + "." + e->GetSel()->GetName().m_value))
        {
            m_error.Clear();
            return m_frame->GetValueObjectForFrameVariable(global, m_use_dynamic);
        }
    }
    // EvaluateExpr should have already set m_error.
    return target;
}

ValueObjectSP
GoInterpreter::VisitBasicLit(const lldb_private::GoASTBasicLit *e)
{
    std::string value =  e->GetValue().m_value.str();
    if (e->GetValue().m_type != GoLexer::LIT_INTEGER) {
        m_error.SetErrorStringWithFormat("Unsupported literal %s",value.c_str());
        return nullptr;
    }
    errno = 0;
    int64_t intvalue = strtol(value.c_str(), NULL, 0);
    if (errno != 0) {
        m_error.SetErrorToErrno();
        return nullptr;
    }
    DataBufferSP buf(new DataBufferHeap(sizeof(intvalue), 0));
    TargetSP target = m_frame->CalculateTarget();
    if (!target)
    {
        m_error.SetErrorString("No target");
        return nullptr;
    }
    ByteOrder order = target->GetArchitecture().GetByteOrder();
    uint8_t addr_size = target->GetArchitecture().GetAddressByteSize();
    DataEncoder enc(buf, order, addr_size);
    enc.PutU64(0, static_cast<uint64_t>(intvalue));
    DataExtractor data(buf, order, addr_size);

    ClangASTType type = LookupType(target, ConstString("int64"));
    ExecutionContext exe_ctx;
    m_frame->CalculateExecutionContext(exe_ctx);
    return ValueObject::CreateValueObjectFromData(nullptr, data, exe_ctx, type);
}

ValueObjectSP
GoInterpreter::VisitIndexExpr(const lldb_private::GoASTIndexExpr *e)
{
    ValueObjectSP target = EvaluateExpr(e->GetX());
    if (!target)
        return nullptr;
    ValueObjectSP index = EvaluateExpr(e->GetIndex());
    if (!index)
        return nullptr;
    bool is_signed;
    if (!index->GetClangType().IsIntegerType(is_signed))
    {
        m_error.SetErrorString("Unsupported index");
        return nullptr;
    }
    size_t idx;
    if (is_signed)
        idx = index->GetValueAsSigned(0);
    else
        idx = index->GetValueAsUnsigned(0);
    return target->GetChildAtIndex(idx, true);
}

ValueObjectSP
GoInterpreter::VisitUnaryExpr(const GoASTUnaryExpr* e)
{
    ValueObjectSP x = EvaluateExpr(e->GetX());
    if (!x)
        return nullptr;
    switch (e->GetOp())
    {
        case GoLexer::OP_AMP:
        {
            ClangASTType type = x->GetClangType().GetPointerType();
            uint64_t address = x->GetAddressOf();
            ExecutionContext exe_ctx;
            m_frame->CalculateExecutionContext(exe_ctx);
            return ValueObject::CreateValueObjectFromAddress(nullptr, address, exe_ctx, type);
        }
        case GoLexer::OP_PLUS:
            return x;
        default:
            m_error.SetErrorStringWithFormat("Operator %s not supported", GoLexer::LookupToken(e->GetOp()).str().c_str());
            return nullptr;
    }
}

ClangASTType
GoInterpreter::EvaluateType(const GoASTExpr* e)
{
    TargetSP target = m_frame->CalculateTarget();
    if (auto* id = llvm::dyn_cast<GoASTIdent>(e))
    {
        ClangASTType result = LookupType(target, ConstString(id->GetName().m_value));
        if (result.IsValid())
            return result;
        std::string fullname = (m_package + "." + id->GetName().m_value).str();
        result = LookupType(target, ConstString(fullname));
        if (!result)
            m_error.SetErrorStringWithFormat("Unknown type %s", fullname.c_str());
        return result;
    }
    if (auto* sel = llvm::dyn_cast<GoASTSelectorExpr>(e))
    {
        std::string package;
        if (auto* pkg_node = llvm::dyn_cast<GoASTIdent>(sel->GetX()))
        {
            package = pkg_node->GetName().m_value.str();
        }
        else if (auto* str_node = llvm::dyn_cast<GoASTBasicLit>(sel->GetX()))
        {
            if (str_node->GetValue().m_type == GoLexer::LIT_STRING)
            {
                package = str_node->GetValue().m_value.substr(1).str();
                package.resize(package.length() - 1);
            }
        }
        if (package.empty())
        {
            m_error.SetErrorStringWithFormat("Invalid %s in type expression", sel->GetX()->GetKindName());
            return ClangASTType();
        }
        std::string fullname = (package + "." + sel->GetSel()->GetName().m_value).str();
        ClangASTType result = LookupType(target, ConstString(fullname));
        if (!result)
            m_error.SetErrorStringWithFormat("Unknown type %s", fullname.c_str());
        return result;
    }
    if (auto* star = llvm::dyn_cast<GoASTStarExpr>(e))
    {
        ClangASTType elem = EvaluateType(star->GetX());
        return elem.GetPointerType();
    }
    if (auto* paren = llvm::dyn_cast<GoASTParenExpr>(e))
        return EvaluateType(paren->GetX());
    
    m_error.SetErrorStringWithFormat("Invalid %s in type expression", e->GetKindName());
    return ClangASTType();
}

ValueObjectSP
GoInterpreter::VisitCallExpr(const lldb_private::GoASTCallExpr *e)
{
    ValueObjectSP x = EvaluateExpr(e->GetFun());
    if (x || e->NumArgs() != 1)
    {
        m_error.SetErrorStringWithFormat("Code execution not supported");
        return nullptr;
    }
    m_error.Clear();
    ClangASTType type = EvaluateType(e->GetFun());
    if (!type)
    {
        return nullptr;
    }
    ValueObjectSP value = EvaluateExpr(e->GetArgs(0));
    if (!value)
        return nullptr;
    // TODO: Handle special conversions
    return value->Cast(type);
}
