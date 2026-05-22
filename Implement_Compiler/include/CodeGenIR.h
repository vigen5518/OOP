#pragma once

#include "CodeGenerator.h"
#include "ir.h"
#include <map>

// IR -> Logical machine code -> binary instructions (Image 3)
class CodeGenIR {
public:
    CodeGen::Program generate(const IRProgram& ir);

private:
    CodeGen::Program prog_;
    std::map<std::string, int> vars_;
    std::map<std::string, uint32_t> labels_;
    std::map<std::string, uint32_t> funcAddrs_;
    int nextReg_ = 0;
    int localSlot_ = 0;

    int regOf(const std::string& name);
    void emit(Instruction i);
    uint32_t here();
    int newConst(double v);
};
