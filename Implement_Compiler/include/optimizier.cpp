#include "optimizier.h"
#include <cstddef>

void Optimizer::optimize() {
}

void Optimizer::optimize(CodeGen::Program& prog) {
    for (size_t i = 0; i + 1 < prog.code.size(); ++i) {
        const Instruction& a = prog.code[i];
        const Instruction& b = prog.code[i + 1];
        if ((OpCode)a.op == OpCode::MOV_CONST &&
            (OpCode)b.op == OpCode::MOV_CONST &&
            a.rd == b.rd) {
            prog.code.erase(prog.code.begin() + (ptrdiff_t)i);
            if (i > 0) --i;
        }
    }
}
