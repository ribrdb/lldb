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
#include "lldb/Core/Log.h"
#include "lldb/Core/Module.h"
#include "lldb/Core/StreamFile.h"
#include "lldb/Core/StreamString.h"
#include "lldb/Core/ValueObjectConstResult.h"
#include "lldb/Expression/ClangExpression.h"
#include "lldb/Expression/ClangPersistentVariables.h"
#include "lldb/Expression/GoUserExpression.h"
#include "lldb/Target/ExecutionContext.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/StackFrame.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/ThreadPlan.h"
#include "lldb/Target/ThreadPlanCallUserExpression.h"

using namespace lldb_private;

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
    lldb::VariableSP var_sp;
    result_valobj_sp = exe_ctx.GetFrameRef().GetValueForVariableExpressionPath(expr_cstr, options.GetUseDynamic(), 0, var_sp, error);
    
    if (result_valobj_sp.get() == NULL)
    {
        result_valobj_sp = ValueObjectConstResult::Create (exe_ctx.GetBestExecutionContextScope(), error);
    }
    execution_results = lldb::eExpressionCompleted;
    
    return execution_results;
}
