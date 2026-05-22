#pragma once

#include <string>
#include <vector>

enum class IROp {
    LABEL,
    MOV_CONST, MOV_REG,
    ADD, SUB, MUL, DIV, MOD,
    CMP,
    BR_EQ, BR_NEQ, JMP,
    LOG_AND, LOG_OR, LOG_NOT,
    PUSH_BP, SET_BP_SP, ALLOC_STACK, RESTORE_BP,
    PUSH, POP, CALL, RET,
    LOAD_LOCAL, STORE_LOCAL,
    JMP_TABLE,
    HALT
};

struct IRInstruction {
    IROp op = IROp::LABEL;
    std::string dst;
    std::string src1;
    std::string src2;
    double imm = 0;
    int cmpMode = 0;
    int labelId = -1;
    int jumpTableId = -1;
};

struct IRProgram {
    std::vector<IRInstruction> code;
};
