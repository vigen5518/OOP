#pragma once

#include "CodeGenerator.h"
#include <string>

class Assembler {
public:
    void assemble();
    std::string disassemble(const CodeGen::Program& prog);
    CodeGen::Program assembleSource(const std::string& asmText);
};
