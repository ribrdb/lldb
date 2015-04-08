//===-- GoParser.h -----------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_GoParser_h
#define liblldb_GoParser_h

#include "lldb/lldb-private.h"
#include "lldb/Expression/GoAST.h"
#include "lldb/Expression/GoLexer.h"

namespace lldb_private
{
    class GoParser
    {
    public:
        GoASTStmt* Statement();

        GoASTStmt* GoStmt();
        GoASTStmt* ReturnStmt();
        GoASTStmt* BranchStmt();
        GoASTStmt* EmptyStmt();
        GoASTStmt* ExpressionStmt(GoASTExpr* e);
        GoASTStmt* IncDecStmt(GoASTExpr* e);
        GoASTStmt* Assignment(GoASTExpr* e);
        GoASTBlockStmt* Block();
        
        GoASTExpr*  MoreExpressionList(); // ["," Expression]
        GoASTIdent* MoreIdentifierList(); // ["," Identifier]

        GoASTExpr* Expression();
        GoASTExpr* UnaryExpr();
        GoASTExpr* OrExpr();
        GoASTExpr* AndExpr();
        GoASTExpr* RelExpr();
        GoASTExpr* AddExpr();
        GoASTExpr* MulExpr();
        GoASTExpr* PrimaryExpr();
        GoASTExpr* Operand();
        GoASTExpr* Conversion();

        GoASTExpr* Selector(GoASTExpr* e);
        GoASTExpr* IndexOrSlice(GoASTExpr* e);
        GoASTExpr* TypeAssertion(GoASTExpr* e);
        GoASTExpr* Arguments(GoASTExpr* e);

        GoASTExpr* Type();
        GoASTExpr* Type2();
        GoASTExpr* ArrayOrSliceType();
        GoASTExpr* StructType();
        GoASTExpr* FunctionType();
        GoASTExpr* InterfaceType();
        GoASTExpr* MapType();
        GoASTExpr* ChanType();
        GoASTExpr* ChanType2();
        
        GoASTExpr* Name();
        GoASTExpr* QualifiedIdent(GoASTIdent* p);
        GoASTIdent* Identifier();
        
        GoASTField* FieldDecl();
        GoASTExpr* AnonymousFieldType();
        GoASTExpr* FieldNamesAndType(GoASTField* f);
        
        GoASTFieldList* Params();
        GoASTField* ParamDecl();
        GoASTExpr* ParamType();
        GoASTFuncType* Signature();
        GoASTExpr* CompositeLit();
        GoASTExpr* FunctionLit();
        GoASTExpr* Element();
        GoASTCompositeLit* LiteralValue();
    
    private:
        class Rule;
        friend class Rule;
        
        std::nullptr_t syntaxerror() {
            m_failed = true;
            return nullptr;
        }
        GoLexer::Token& next() {
            if (m_pos >= m_tokens.size())
            {
                m_pos = m_tokens.size();
                m_tokens.push_back(m_lexer.Lex());
            }
            return m_tokens[m_pos++];
        }
        GoLexer::TokenType peek() {
            GoLexer::Token& tok = next();
            --m_pos;
            return tok.m_type;
        }
        GoLexer::Token* match(GoLexer::TokenType t) {
            GoLexer::Token& tok = next();
            if (tok.m_type == t)
                return &tok;
            return nullptr;
        }
        GoLexer::Token* mustMatch(GoLexer::TokenType t) {
            GoLexer::Token* tok = match(t);
            if (tok)
                return tok;
            return syntaxerror();
        }
        bool Semicolon();
        
        GoASTStmt* FinishStmt(GoASTStmt* s) {
            if (!Semicolon())
                m_failed = true;
            return s;
        }
        
        llvm::StringRef CopyString(llvm::StringRef s);
        
        GoLexer m_lexer;
        std::vector<GoLexer::Token> m_tokens;
        size_t m_pos;
        llvm::StringRef m_last;
        GoLexer::TokenType m_last_tok;
        llvm::StringMap<uint8_t> m_strings;
        bool m_failed;
    };
}

#endif
