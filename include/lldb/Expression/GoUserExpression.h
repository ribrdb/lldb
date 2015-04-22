//===-- GoUserExpression.h -----------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_GoUserExpression_h_
#define liblldb_GoUserExpression_h_

// C Includes
// C++ Includes
#include <string>
#include <map>
#include <vector>

// Other libraries and framework includes
// Project includes

#include "lldb/lldb-forward.h"
#include "lldb/lldb-private.h"
#include "lldb/Target/ExecutionContext.h"

namespace lldb_private
{
    
    //----------------------------------------------------------------------
    /// @class GoUserExpression GoUserExpression.h "lldb/Expression/GoUserExpression.h"
    /// @brief Encapsulates a single expression for use with Go
    ///
    /// LLDB uses expressions for various purposes, notably to call functions
    /// and as a backend for the expr command.  GoUserExpression encapsulates
    /// the objects needed to parse and interpret an expression.  It
    /// uses the Clang parser to produce LLVM IR from the expression.
    //----------------------------------------------------------------------
    class GoUserExpression
    {
    public:
        
        
        //------------------------------------------------------------------
        /// Evaluate one expression and return its result.
        ///
        /// @param[in] exe_ctx
        ///     The execution context to use when evaluating the expression.
        ///
        /// @param[in] options
        ///     Expression evaluation options.
        ///
        /// @param[in] expr_cstr
        ///     A C string containing the expression to be evaluated.
        ///
        /// @param[in] expr_prefix
        ///     If non-NULL, a C string containing translation-unit level
        ///     definitions to be included when the expression is parsed.
        ///
        /// @param[in/out] result_valobj_sp
        ///      If execution is successful, the result valobj is placed here.
        ///
        /// @param[out]
        ///     Filled in with an error in case the expression evaluation
        ///     fails to parse, run, or evaluated.
        ///
        /// @result
        ///      A Process::ExpressionResults value.  eExpressionCompleted for success.
        //------------------------------------------------------------------
        static lldb::ExpressionResults
        Evaluate (ExecutionContext &exe_ctx,
                  const EvaluateExpressionOptions& options,
                  const char *expr_cstr,
                  const char *expr_prefix,
                  lldb::ValueObjectSP &result_valobj_sp,
                  Error &error);
    private:
    };
    
} // namespace lldb_private

#endif  // liblldb_GoUserExpression_h_
