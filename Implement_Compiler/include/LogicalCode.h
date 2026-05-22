#pragma once

#include "ir.h"

// Logical code layer: lowers &&, ||, ! to branch/compare IR (Image 3)
class LogicalCode {
public:
    static void process(IRProgram& ir);
};
