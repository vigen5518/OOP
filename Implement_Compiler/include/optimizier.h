#pragma once

#include "CodeGenerator.h"

class Optimizer {
public:
    void optimize();
    void optimize(CodeGen::Program& prog);
};
