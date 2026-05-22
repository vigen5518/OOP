#include "CodeGenIR.h"
#include <cstdlib>

void CodeGenIR::emit(Instruction i) { prog_.code.push_back(i); }
uint32_t CodeGenIR::here() { return (uint32_t)prog_.code.size(); }

int CodeGenIR::newConst(double v) {
    prog_.constPool.push_back(v);
    return (int)prog_.constPool.size() - 1;
}

int CodeGenIR::regOf(const std::string& name) {
    if (name.empty() || name == "0") return 0;
    auto it = vars_.find(name);
    if (it != vars_.end()) return it->second;
    int r = nextReg_++;
    if (name.size() > 1 && name[0] == 'p') {
        int slot = atoi(name.c_str() + 1);
        emit({ (uint32_t)OpCode::LOAD_LOCAL, (uint32_t)r, (uint32_t)slot, 0 });
    }
    vars_[name] = r;
    return r;
}

CodeGen::Program CodeGenIR::generate(const IRProgram& ir) {
    prog_ = {};
    vars_.clear();
    labels_.clear();
    funcAddrs_.clear();
    nextReg_ = 1;

    for (auto& ins : ir.code)
        if (ins.op == IROp::LABEL)
            labels_[ins.dst] = 0;

    struct Patch { uint32_t idx; std::string label; bool useRs1; };
    std::vector<Patch> patches;

    for (auto& ins : ir.code) {
        switch (ins.op) {
        case IROp::LABEL:
            labels_[ins.dst] = here();
            if (ins.dst.size() > 4 && ins.dst.compare(0, 4, "case") != 0 &&
                ins.dst[0] == 'L') { /* skip */ }
            else if (ins.dst[0] != 'L' || (ins.dst.size() > 1 && ins.dst[1] != '0')) {
                if (funcAddrs_.count(ins.dst) || ins.dst.find(' ') == std::string::npos) {
                    funcAddrs_[ins.dst] = here();
                    CodeGen::Program::Symbol s{ ins.dst, here(), 0 };
                    prog_.symbols.push_back(s);
                }
            }
            break;
        case IROp::MOV_CONST: {
            int r = regOf(ins.dst);
            int c = newConst(ins.imm);
            emit({ (uint32_t)OpCode::MOV_CONST, (uint32_t)r, (uint32_t)c, 0 });
            break;
        }
        case IROp::MOV_REG: {
            int d = regOf(ins.dst), s = regOf(ins.src1);
            emit({ (uint32_t)OpCode::ADD, (uint32_t)d, (uint32_t)s, 0 });
            break;
        }
        case IROp::ADD: case IROp::SUB: case IROp::MUL: case IROp::DIV: case IROp::MOD: {
            int d = regOf(ins.dst), a = regOf(ins.src1), b = regOf(ins.src2);
            OpCode op = OpCode::ADD;
            if (ins.op == IROp::SUB) op = OpCode::SUB;
            else if (ins.op == IROp::MUL) op = OpCode::MUL;
            else if (ins.op == IROp::DIV) op = OpCode::DIV;
            else if (ins.op == IROp::MOD) op = OpCode::MOD;
            emit({ (uint32_t)op, (uint32_t)d, (uint32_t)a, (uint32_t)b });
            break;
        }
        case IROp::CMP: {
            int d = regOf(ins.dst), a = regOf(ins.src1), b = regOf(ins.src2);
            uint32_t packed = ((uint32_t)b << 3) | (uint32_t)ins.cmpMode;
            emit({ (uint32_t)OpCode::CMP, (uint32_t)d, (uint32_t)a, packed });
            break;
        }
        case IROp::BR_EQ: {
            int c = regOf(ins.dst);
            uint32_t p = here();
            emit({ (uint32_t)OpCode::BR_EQ, (uint32_t)c, 0, 0 });
            patches.push_back({ p, ins.src2, false });
            break;
        }
        case IROp::BR_NEQ: {
            int c = regOf(ins.dst);
            uint32_t p = here();
            emit({ (uint32_t)OpCode::BR_NEQ, (uint32_t)c, 0, 0 });
            patches.push_back({ p, ins.src2, true });
            break;
        }
        case IROp::JMP: {
            uint32_t p = here();
            emit({ (uint32_t)OpCode::JMP, 0, 0, 0 });
            patches.push_back({ p, ins.dst, true });
            break;
        }
        case IROp::PUSH_BP: emit({ (uint32_t)OpCode::PUSH_BP, 0, 0, 0 }); break;
        case IROp::SET_BP_SP: emit({ (uint32_t)OpCode::SET_BP_SP, 0, 0, 0 }); break;
        case IROp::ALLOC_STACK: emit({ (uint32_t)OpCode::ALLOC_STACK, 0, 64, 0 }); break;
        case IROp::RESTORE_BP: emit({ (uint32_t)OpCode::RESTORE_BP, 0, 0, 0 }); break;
        case IROp::PUSH: emit({ (uint32_t)OpCode::PUSH, (uint32_t)regOf(ins.dst), 0, 0 }); break;
        case IROp::CALL: {
            int d = regOf(ins.dst);
            uint32_t ci = here();
            emit({ (uint32_t)OpCode::CALL, (uint32_t)d, 0, 0 });
            prog_.relocs.push_back({ ci, ins.src1 });
            break;
        }
        case IROp::RET: emit({ (uint32_t)OpCode::RET, (uint32_t)regOf(ins.dst), 0, 0 }); break;
        case IROp::JMP_TABLE: {
            CodeGen::Program::JumpTable jt;
            jt.tableId = (uint32_t)prog_.jumpTables.size();
            jt.switchReg = regOf(ins.dst);
            for (auto& kv : labels_)
                if (kv.first.compare(0, 4, "case") == 0)
                    jt.targets.push_back(kv.second);
            if (jt.targets.empty()) jt.targets.push_back(here());
            prog_.jumpTables.push_back(jt);
            emit({ (uint32_t)OpCode::JMP_TABLE, (uint32_t)jt.switchReg, jt.tableId, 0 });
            break;
        }
        case IROp::HALT: emit({ (uint32_t)OpCode::HALT, 0, 0, 0 }); break;
        default: break;
        }
    }

    for (auto& p : patches) {
        if (!labels_.count(p.label)) continue;
        if (p.useRs1) prog_.code[p.idx].rs1 = labels_[p.label];
        else prog_.code[p.idx].rs2 = labels_[p.label];
    }
    for (auto& r : prog_.relocs) {
        auto it = funcAddrs_.find(r.targetName);
        if (it != funcAddrs_.end())
            prog_.code[r.instrIndex].rs1 = it->second;
    }
    return std::move(prog_);
}
