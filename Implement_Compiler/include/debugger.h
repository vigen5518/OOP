#pragma once

#include "vm.h"
#include <string>

class Debugger {
public:
    void dumpRegisters(VM& vm) const;
    void dumpState(VM& vm) const;
    void setBreakpoint(VM& vm, uint64_t pc);
    void clearBreakpoint(VM& vm);
    bool step(VM& vm);
    void runToBreak(VM& vm);
    void monitor(VM& vm);
};
