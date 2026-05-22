#include "IRGen.h"

std::string IRGen::fresh() { return "t" + std::to_string(tmp_++); }
std::string IRGen::label() { return "L" + std::to_string(label_++); }
void IRGen::emit(IRInstruction i) { ir_.code.push_back(std::move(i)); }

IRProgram IRGen::generate(const ASTNode* prog) {
    ir_ = {};
    tmp_ = 0;
    label_ = 0;
    for (auto& c : prog->children)
        if (c) emitDecl(c.get());
    emit({ IROp::HALT, "", "", "" });
    return std::move(ir_);
}

void IRGen::emitDecl(const ASTNode* n) {
    if (!n) return;
    if (n->kind == NodeKind::FUNC_DECL) {
        auto sp = n->text.find(' ');
        std::string name = sp == std::string::npos ? n->text : n->text.substr(sp + 1);
        emit({ IROp::LABEL, name, "", "" });
        emit({ IROp::PUSH_BP, "", "", "" });
        emit({ IROp::SET_BP_SP, "", "", "" });
        scopes_.push_back({});
        int paramCount = (int)n->children.size() - 1;
        for (int i = 0; i < paramCount; i++)
            if (n->children[i])
                scopes_.back()[n->children[i]->text] = "p" + std::to_string(i);
        if (!n->children.empty())
            emitStmt(n->children.back().get());
        emit({ IROp::RESTORE_BP, "", "", "" });
        scopes_.pop_back();
    } else if (n->kind == NodeKind::VAR_DECL) {
        std::string name = n->text;
        auto sp = name.find(' ');
        if (sp != std::string::npos) name = name.substr(sp + 1);
        globals_[name] = name;
        if (!n->children.empty()) {
            std::string v = emitExpr(n->children[0].get());
            emit({ IROp::MOV_REG, name, v, "" });
        }
    } else {
        emitStmt(n);
    }
}

void IRGen::emitStmt(const ASTNode* n) {
    if (!n) return;
    switch (n->kind) {
    case NodeKind::BLOCK:
        scopes_.push_back({});
        for (auto& c : n->children) emitDecl(c.get());
        scopes_.pop_back();
        break;
    case NodeKind::EXPR_STMT:
        if (!n->children.empty()) emitExpr(n->children[0].get());
        break;
    case NodeKind::VAR_DECL: {
        std::string name = n->text;
        auto sp = name.find(' ');
        if (sp != std::string::npos) name = name.substr(sp + 1);
        if (!scopes_.empty()) scopes_.back()[name] = name;
        else globals_[name] = name;
        if (!n->children.empty()) {
            std::string v = emitExpr(n->children[0].get());
            emit({ IROp::MOV_REG, name, v, "" });
        }
        break;
    }
    case NodeKind::IF_STMT: {
        std::string c = emitExpr(n->children[0].get());
        std::string el = label(), end = label();
        emit({ IROp::BR_EQ, c, "0", el });
        emitStmt(n->children[1].get());
        if (n->children.size() > 2) {
            emit({ IROp::JMP, "", end, "" });
            emit({ IROp::LABEL, el, "", "" });
            emitStmt(n->children[2].get());
        } else {
            emit({ IROp::LABEL, el, "", "" });
        }
        emit({ IROp::LABEL, end, "", "" });
        break;
    }
    case NodeKind::WHILE_STMT: {
        std::string start = label(), end = label();
        emit({ IROp::LABEL, start, "", "" });
        std::string c = emitExpr(n->children[0].get());
        emit({ IROp::BR_EQ, c, "0", end });
        emitStmt(n->children[1].get());
        emit({ IROp::JMP, "", start, "" });
        emit({ IROp::LABEL, end, "", "" });
        break;
    }
    case NodeKind::RETURN_STMT: {
        std::string r = n->children.empty() ? "0" : emitExpr(n->children[0].get());
        emit({ IROp::MOV_REG, "0", r, "" });
        emit({ IROp::RET, r, "", "" });
        break;
    }
    case NodeKind::SWITCH_STMT: {
        std::string sw = emitExpr(n->children[0].get());
        IRInstruction jt{ IROp::JMP_TABLE, sw, "", "" };
        jt.jumpTableId = (int)ir_.code.size();
        emit(jt);
        for (size_t i = 1; i < n->children.size(); i++) {
            auto* cl = n->children[i].get();
            if (!cl) continue;
            emit({ IROp::LABEL, "case" + std::to_string(i), "", "" });
            if (cl->kind == NodeKind::CASE_CLAUSE || cl->kind == NodeKind::DEFAULT_CLAUSE) {
                for (size_t j = (cl->kind == NodeKind::CASE_CLAUSE ? 1 : 0); j < cl->children.size(); j++)
                    emitStmt(cl->children[j].get());
            }
        }
        break;
    }
    default:
        break;
    }
}

std::string IRGen::emitExpr(const ASTNode* n) {
    if (!n) return "0";
    switch (n->kind) {
    case NodeKind::NUM_LIT: {
        std::string t = fresh();
        emit({ IROp::MOV_CONST, t, "", "" });
        ir_.code.back().imm = n->numVal;
        return t;
    }
    case NodeKind::VAR_REF: {
        for (int i = (int)scopes_.size() - 1; i >= 0; i--) {
            auto it = scopes_[i].find(n->text);
            if (it != scopes_[i].end()) return it->second;
        }
        auto it = globals_.find(n->text);
        return it != globals_.end() ? it->second : n->text;
    }
    case NodeKind::BINOP: {
        if (n->text == "&&") {
            std::string t = fresh();
            emit({ IROp::LOG_AND, t, emitExpr(n->children[0].get()), emitExpr(n->children[1].get()) });
            return t;
        }
        if (n->text == "||") {
            std::string t = fresh();
            emit({ IROp::LOG_OR, t, emitExpr(n->children[0].get()), emitExpr(n->children[1].get()) });
            return t;
        }
        std::string l = emitExpr(n->children[0].get());
        std::string r = emitExpr(n->children[1].get());
        std::string t = fresh();
        IROp op = IROp::ADD;
        if (n->text == "-") op = IROp::SUB;
        else if (n->text == "*") op = IROp::MUL;
        else if (n->text == "/") op = IROp::DIV;
        else if (n->text == "%") op = IROp::MOD;
        else if (n->text == "==" || n->text == "!=" || n->text == "<" ||
                 n->text == ">" || n->text == "<=" || n->text == ">=") {
            emit({ IROp::CMP, t, l, r });
            ir_.code.back().cmpMode = (n->text == "!=") ? 1 : (n->text == "<") ? 2 :
                (n->text == ">") ? 3 : (n->text == "<=") ? 4 : (n->text == ">=") ? 5 : 0;
            return t;
        }
        emit({ op, t, l, r });
        return t;
    }
    case NodeKind::UNOP: {
        if (n->text == "!") {
            std::string t = fresh();
            emit({ IROp::LOG_NOT, t, emitExpr(n->children[0].get()), "" });
            return t;
        }
        std::string o = emitExpr(n->children[0].get());
        std::string t = fresh(), z = fresh();
        emit({ IROp::MOV_CONST, z, "", "" });
        emit({ IROp::SUB, t, z, o });
        return t;
    }
    case NodeKind::CALL: {
        for (auto& a : n->children) {
            std::string ar = emitExpr(a.get());
            emit({ IROp::PUSH, ar, "", "" });
        }
        std::string t = fresh();
        emit({ IROp::CALL, t, n->text, "" });
        return t;
    }
    default:
        return "0";
    }
}
