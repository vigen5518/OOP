#pragma once

#include "Common.h"
#include "ir.h"

// AST -> Intermediate Code (Image 3)
class IRGen {
public:
    IRProgram generate(const ASTNode* prog);

private:
    IRProgram ir_;
    int tmp_ = 0;
    int label_ = 0;

    std::string fresh();
    std::string label();
    void emit(IRInstruction i);

    void emitDecl(const ASTNode* n);
    void emitStmt(const ASTNode* n);
    std::string emitExpr(const ASTNode* n);

    std::vector<std::map<std::string, std::string>> scopes_;
    std::map<std::string, std::string> globals_;
};
