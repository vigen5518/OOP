#pragma once
#include "Common.h"
#include "IRGen.h"
#include "LogicalCode.h"
#include "CodeGenIR.h"
#include <stdexcept>
#include <functional>

class CodeGen {
public:
    struct Program {
        std::vector<Instruction> code;
        std::vector<double>      constPool;
        std::vector<std::string> strings;

        struct Symbol {
            std::string name;
            uint32_t    address;
            uint8_t     type;
        };
        std::vector<Symbol> symbols;

        struct Reloc {
            uint32_t instrIndex;
            std::string targetName;
        };
        std::vector<Reloc> relocs;

        struct JumpTable {
            int32_t  minVal = 0;
            uint32_t tableId = 0;
            uint32_t switchReg = 0;
            std::vector<uint32_t> targets;
        };
        std::vector<JumpTable> jumpTables;
    };

    Program generate(const ASTNode* prog) {
        IRGen irGen;
        IRProgram ir = irGen.generate(prog);
        LogicalCode::process(ir);
        CodeGenIR backend;
        return backend.generate(ir);
    }

    Program generateDirect(const ASTNode* prog) {
        prog_ = &prog_;
        nextReg_ = 0;
        // Two-pass: first collect function addresses, then emit
        for (auto& child : prog->children) {
            if (child && child->kind == NodeKind::FUNC_DECL)
                funcPlaceholders_[child->children.empty() ? child->text : funcName(child->text)] = 0;
        }
        for (auto& child : prog->children)
            if (child) emitDecl(child.get());
        emit({ (uint32_t)OpCode::HALT, 0, 0, 0 });

        // Resolve relocations
        for (auto& r : prog_.relocs) {
            auto it = funcAddrs_.find(r.targetName);
            if (it != funcAddrs_.end()) {
                // patch rs1 with actual address
                prog_.code[r.instrIndex].rs1 = it->second;
            }
        }
        return std::move(prog_);
    }

private:
    Program prog_;
    int nextReg_;
    int nextLabel_ = 0;
    const ASTNode** prog_ptr_ = nullptr;

    // var name -> register index (local scope stack)
    std::vector<std::map<std::string, int>> scopes_;
    std::map<std::string, int> globalVars_;
    std::map<std::string, uint32_t> funcAddrs_;
    std::map<std::string, uint32_t> funcPlaceholders_;

    // ---- helpers ----
    void emit(Instruction i) { prog_.code.push_back(i); }
    uint32_t here() { return (uint32_t)prog_.code.size(); }

    int newReg() { return nextReg_++; }
    int newConst(double v) {
        prog_.constPool.push_back(v);
        return (int)prog_.constPool.size() - 1;
    }

    std::string funcName(const std::string& text) {
        // "retType name" -> "name"
        auto sp = text.find(' ');
        return sp == std::string::npos ? text : text.substr(sp + 1);
    }

    void pushScope() { scopes_.push_back({}); }
    void popScope() { if (!scopes_.empty()) scopes_.pop_back(); }

    int lookupVar(const std::string& name) {
        for (int i = (int)scopes_.size() - 1; i >= 0; i--) {
            auto it = scopes_[i].find(name);
            if (it != scopes_[i].end()) return it->second;
        }
        auto it = globalVars_.find(name);
        if (it != globalVars_.end()) return it->second;
        // allocate new register
        int r = newReg();
        if (!scopes_.empty()) scopes_.back()[name] = r;
        else globalVars_[name] = r;
        return r;
    }

    // ---- declaration emitters ----
    void emitDecl(const ASTNode* node) {
        if (!node) return;
        if (node->kind == NodeKind::FUNC_DECL) {
            std::string name = funcName(node->text);
            funcAddrs_[name] = here();
            // Add to symbol table
            Program::Symbol sym;
            sym.name = name;
            sym.address = here();
            sym.type = 0;
            prog_.symbols.push_back(sym);
            // Emit function body (last child is the block)
            pushScope();
            // parameters: first N-1 children are params, last is block
            int paramCount = (int)node->children.size() - 1;
            for (int i = 0; i < paramCount; i++) {
                if (node->children[i]) {
                    int r = newReg();
                    scopes_.back()[node->children[i]->text] = r;
                }
            }
            if (!node->children.empty())
                emitStmt(node->children.back().get());
            popScope();
        }
        else if (node->kind == NodeKind::VAR_DECL) {
            std::string name = node->text;
            auto sp = name.find(' ');
            if (sp != std::string::npos) name = name.substr(sp + 1);
            int r = newReg();
            globalVars_[name] = r;
            if (!node->children.empty()) {
                int valReg = emitExpr(node->children[0].get());
                // MOV global reg from valReg
                emit({ (uint32_t)OpCode::ADD, (uint32_t)r, (uint32_t)valReg, 0 });
            }
            Program::Symbol sym;
            sym.name = name;
            sym.address = r;
            sym.type = 1;
            prog_.symbols.push_back(sym);
        }
        else {
            emitStmt(node);
        }
    }

    // ---- statement emitters ----
    void emitStmt(const ASTNode* node) {
        if (!node) return;
        switch (node->kind) {
        case NodeKind::BLOCK:
            pushScope();
            for (auto& c : node->children) emitDecl(c.get());
            popScope();
            break;

        case NodeKind::EXPR_STMT:
            if (!node->children.empty()) emitExpr(node->children[0].get());
            break;

        case NodeKind::VAR_DECL: {
            std::string name = node->text;
            auto sp = name.find(' ');
            if (sp != std::string::npos) name = name.substr(sp + 1);
            int r = newReg();
            if (!scopes_.empty()) scopes_.back()[name] = r;
            else globalVars_[name] = r;
            if (!node->children.empty()) {
                int valReg = emitExpr(node->children[0].get());
                emit({ (uint32_t)OpCode::ADD, (uint32_t)r, (uint32_t)valReg, 0 });
            }
            break;
        }

        case NodeKind::IF_STMT: {
            // children: [cond, then, (else)?]
            int condReg = emitExpr(node->children[0].get());
            // BR_EQ condReg, 0 -> jump to else/end
            uint32_t patchIdx = here();
            emit({ (uint32_t)OpCode::BR_EQ, (uint32_t)condReg, 0, 0 }); // placeholder

            emitStmt(node->children[1].get()); // then

            if (node->children.size() > 2) {
                uint32_t jmpIdx = here();
                emit({ (uint32_t)OpCode::JMP, 0, 0, 0 }); // jump over else
                prog_.code[patchIdx].rs2 = here();       // patch BR_EQ
                emitStmt(node->children[2].get());       // else
                prog_.code[jmpIdx].rs1 = here();         // patch JMP
            }
            else {
                prog_.code[patchIdx].rs2 = here();
            }
            break;
        }

        case NodeKind::WHILE_STMT: {
            uint32_t loopStart = here();
            int condReg = emitExpr(node->children[0].get());
            uint32_t patchIdx = here();
            emit({ (uint32_t)OpCode::BR_EQ, (uint32_t)condReg, 0, 0 });
            emitStmt(node->children[1].get());
            emit({ (uint32_t)OpCode::JMP, 0, (uint32_t)loopStart, 0 });
            prog_.code[patchIdx].rs2 = here();
            break;
        }

        case NodeKind::DO_WHILE_STMT: {
            uint32_t loopStart = here();
            emitStmt(node->children[0].get()); // body
            int condReg = emitExpr(node->children[1].get());
            // if cond != 0 -> jump back
            emit({ (uint32_t)OpCode::BR_NEQ, (uint32_t)condReg, (uint32_t)loopStart, 0 });
            break;
        }

        case NodeKind::FOR_STMT: {
            // children: [init, cond, step, body]
            pushScope();
            if (node->children[0]) emitDecl(node->children[0].get());
            uint32_t loopStart = here();
            uint32_t patchCond = UINT32_MAX;
            if (node->children[1]) {
                int condReg = emitExpr(node->children[1].get());
                patchCond = here();
                emit({ (uint32_t)OpCode::BR_EQ, (uint32_t)condReg, 0, 0 });
            }
            emitStmt(node->children[3].get()); // body
            if (node->children[2]) emitExpr(node->children[2].get()); // step
            emit({ (uint32_t)OpCode::JMP, 0, (uint32_t)loopStart, 0 });
            if (patchCond != UINT32_MAX)
                prog_.code[patchCond].rs2 = here();
            popScope();
            break;
        }

        case NodeKind::RETURN_STMT: {
            int retReg = 0;
            if (!node->children.empty())
                retReg = emitExpr(node->children[0].get());
            emit({ (uint32_t)OpCode::RET, (uint32_t)retReg, 0, 0 });
            break;
        }

        case NodeKind::SWITCH_STMT: {
            int switchReg = emitExpr(node->children[0].get());
            std::vector<uint32_t> jmpEnd;
            uint32_t defaultJmp = UINT32_MAX;
            for (size_t i = 1; i < node->children.size(); i++) {
                auto* clause = node->children[i].get();
                if (!clause) continue;
                if (clause->kind == NodeKind::CASE_CLAUSE) {
                    int caseReg = emitExpr(clause->children[0].get());
                    // CMP switchReg, caseReg
                    int cmpReg = newReg();
                    emit({ (uint32_t)OpCode::CMP, (uint32_t)cmpReg, (uint32_t)switchReg,
                           ((uint32_t)caseReg << 3) | 0u });
                    uint32_t skipIdx = here();
                    emit({ (uint32_t)OpCode::BR_EQ, (uint32_t)cmpReg, 0, 0 });
                    for (size_t j = 1; j < clause->children.size(); j++)
                        emitStmt(clause->children[j].get());
                    uint32_t endIdx = here();
                    emit({ (uint32_t)OpCode::JMP, 0, 0, 0 });
                    jmpEnd.push_back(endIdx);
                    prog_.code[skipIdx].rs2 = here();
                }
                else if (clause->kind == NodeKind::DEFAULT_CLAUSE) {
                    defaultJmp = here();
                    for (size_t j = 0; j < clause->children.size(); j++)
                        emitStmt(clause->children[j].get());
                }
            }
            uint32_t endAddr = here();
            for (auto idx : jmpEnd) prog_.code[idx].rs1 = endAddr;
            break;
        }

        default:
            break;
        }
    }

    // ---- expression emitters -> returns register holding result ----
    int emitExpr(const ASTNode* node) {
        if (!node) return 0;
        switch (node->kind) {
        case NodeKind::NUM_LIT: {
            int r = newReg();
            int ci = newConst(node->numVal);
            emit({ (uint32_t)OpCode::MOV_CONST, (uint32_t)r, (uint32_t)ci, 0 });
            return r;
        }
        case NodeKind::VAR_REF: {
            int r = lookupVar(node->text);
            return r;
        }
        case NodeKind::ASSIGN_EXPR: {
            int valReg = emitExpr(node->children[1].get());
            std::string varName = node->children[0]->text;
            int dst = lookupVar(varName);
            emit({ (uint32_t)OpCode::ADD, (uint32_t)dst, (uint32_t)valReg, 0 });
            return dst;
        }
        case NodeKind::BINOP: {
            int l = emitExpr(node->children[0].get());
            int r = emitExpr(node->children[1].get());
            int dst = newReg();
            OpCode op;
            if (node->text == "+")  op = OpCode::ADD;
            else if (node->text == "-")  op = OpCode::SUB;
            else if (node->text == "*")  op = OpCode::MUL;
            else if (node->text == "/")  op = OpCode::DIV;
            else if (node->text == "%")  op = OpCode::MOD;
            else if (node->text == "==") op = OpCode::BR_EQ;
            else if (node->text == "!=") op = OpCode::BR_NEQ;
            else if (node->text == "<")  op = OpCode::BR_LT;
            else if (node->text == ">")  op = OpCode::BR_GT;
            else if (node->text == "<=") op = OpCode::BR_LTE;
            else if (node->text == ">=") op = OpCode::BR_GTE;
            else op = OpCode::CMP;
            // for relational ops emit a CMP and store result
            if (node->text == "==" || node->text == "!=" || node->text == "<" ||
                node->text == ">" || node->text == "<=" || node->text == ">=") {
                uint32_t mode = 0;
                if (node->text == "!=") mode = 1;
                else if (node->text == "<")  mode = 2;
                else if (node->text == ">")  mode = 3;
                else if (node->text == "<=") mode = 4;
                else if (node->text == ">=") mode = 5;
                uint32_t packed = ((uint32_t)r << 3) | mode;
                emit({ (uint32_t)OpCode::CMP, (uint32_t)dst, (uint32_t)l, packed });
            }
            else {
                emit({ (uint32_t)op, (uint32_t)dst, (uint32_t)l, (uint32_t)r });
            }
            return dst;
        }
        case NodeKind::UNOP: {
            int operand = emitExpr(node->children[0].get());
            int dst = newReg();
            if (node->text == "-") {
                int zero = newReg();
                int ci = newConst(0);
                emit({ (uint32_t)OpCode::MOV_CONST, (uint32_t)zero, (uint32_t)ci, 0 });
                emit({ (uint32_t)OpCode::SUB, (uint32_t)dst, (uint32_t)zero, (uint32_t)operand });
            }
            else {
                // NOT: dst = (operand == 0) ? 1 : 0  -> simplified as SUB
                emit({ (uint32_t)OpCode::CMP, (uint32_t)dst, (uint32_t)operand, (0u << 3) | 0u });
            }
            return dst;
        }
        case NodeKind::CALL: {
            // Push args, emit CALL, result in returned register
            std::vector<int> argRegs;
            for (auto& arg : node->children)
                argRegs.push_back(emitExpr(arg.get()));
            for (int ar : argRegs)
                emit({ (uint32_t)OpCode::PUSH, (uint32_t)ar, 0, 0 });
            int dst = newReg();
            uint32_t callIdx = here();
            emit({ (uint32_t)OpCode::CALL, (uint32_t)dst, 0, 0 }); // rs1 patched later
            // Record relocation
            Program::Reloc rel;
            rel.instrIndex = callIdx;
            rel.targetName = node->text;
            prog_.relocs.push_back(rel);
            return dst;
        }
        default:
            return 0;
        }
    }
};