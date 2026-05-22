#include "LogicalCode.h"

void LogicalCode::process(IRProgram& ir) {
    std::vector<IRInstruction> out;
    for (auto& ins : ir.code) {
        if (ins.op == IROp::LOG_AND) {
            std::string el = ins.dst + "_f", end = ins.dst;
            out.push_back({ IROp::BR_EQ, ins.src1, "0", el }); // src2 = label
            out.push_back({ IROp::MOV_REG, ins.dst, "0", "" });
            out.push_back({ IROp::JMP, "", end, "" });
            out.push_back({ IROp::LABEL, el, "", "" });
            out.push_back({ IROp::CMP, ins.dst, ins.src2, "0" });
            out.back().cmpMode = 0;
            out.push_back({ IROp::BR_EQ, ins.dst, "0", end });
            out.push_back({ IROp::MOV_CONST, ins.dst, "", "" });
            out.back().imm = 1;
            out.push_back({ IROp::LABEL, end, "", "" });
        } else if (ins.op == IROp::LOG_OR) {
            std::string el = ins.dst + "_t", end = ins.dst;
            out.push_back({ IROp::BR_NEQ, ins.src1, "0", el });
            out.push_back({ IROp::MOV_CONST, ins.dst, "", "" });
            out.back().imm = 1;
            out.push_back({ IROp::JMP, "", end, "" });
            out.push_back({ IROp::LABEL, el, "", "" });
            out.push_back({ IROp::BR_NEQ, ins.src2, "0", end });
            out.push_back({ IROp::MOV_CONST, ins.dst, "", "" });
            out.back().imm = 1;
            out.push_back({ IROp::LABEL, end, "", "" });
        } else if (ins.op == IROp::LOG_NOT) {
            out.push_back({ IROp::CMP, ins.dst, ins.src1, "0" });
            out.back().cmpMode = 0;
            out.push_back({ IROp::BR_EQ, ins.dst, "0", ins.dst + "_n" });
            out.push_back({ IROp::MOV_CONST, ins.dst, "", "" });
            out.back().imm = 1;
            out.push_back({ IROp::JMP, "", ins.dst + "_e", "" });
            out.push_back({ IROp::LABEL, ins.dst + "_n", "", "" });
            out.push_back({ IROp::MOV_CONST, ins.dst, "", "" });
            out.back().imm = 0;
            out.push_back({ IROp::LABEL, ins.dst + "_e", "", "" });
        } else {
            out.push_back(ins);
        }
    }
    ir.code = std::move(out);
}
