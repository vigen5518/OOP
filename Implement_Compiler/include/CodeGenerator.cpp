#include "CodeGenerator.h"

const char* opCodeNames[] = {
    "MOV_CONST", "LOAD_VAR", "STORE_VAR",
    "ADD", "SUB", "MUL", "DIV", "MOD",
    "CMP",
    "BR_EQ", "BR_NEQ", "BR_LT", "BR_GT", "BR_LTE", "BR_GTE",
    "JMP", "CALL", "RET", "PUSH", "POP",
    "LOAD_MEM", "STORE_MEM",
    "PUSH_BP", "SET_BP_SP", "ALLOC_STACK", "RESTORE_BP",
    "LOAD_LOCAL", "STORE_LOCAL",
    "JMP_TABLE", "HEAP_ALLOC",
    "HALT"
};
