#include "debugger.h"
#include <iostream>
#include <iomanip>
#include <sstream>

void Debugger::dumpRegisters(VM& vm) const {
    std::cout << "--- Registers (32 x 16-bit view) ---\n";
    for (int i = 0; i < 32; i++)
        std::cout << "R" << std::setw(2) << i << " w64=" << vm.regs[i]
                  << " w16=" << vm.reg16[i] << "\n";
}

void Debugger::dumpState(VM& vm) const {
    std::cout << "PC=" << vm.pc << " IP=" << vm.ip << " SP=" << vm.sp
              << " BP=" << vm.bp << " FF=" << vm.ff
              << " xlen=" << (int)vm.xlen << "\n";
}

void Debugger::setBreakpoint(VM& vm, uint64_t pc) { vm.breakPc = pc; }
void Debugger::clearBreakpoint(VM& vm) { vm.breakPc = UINT64_MAX; }

bool Debugger::step(VM& vm) {
    vm.stepMode = false;
    if (!vm.running || vm.pc >= vm.code.size()) return false;
    vm.runOne();
    return vm.running;
}

void Debugger::runToBreak(VM& vm) {
    vm.stepMode = false;
    while (vm.running && vm.pc < vm.code.size()) {
        if (vm.pc == vm.breakPc) { vm.stepMode = true; break; }
        vm.runOne();
    }
}

void Debugger::monitor(VM& vm) {
    std::cout << "Debugger: (s)tep (c)ontinue (r)egs (m)emory (b)reak N (q)uit\n";
    std::string cmd;
    while (vm.running && std::getline(std::cin, cmd)) {
        if (cmd.empty()) continue;
        if (cmd[0] == 'q') break;
        if (cmd[0] == 's') { step(vm); dumpState(vm); }
        else if (cmd[0] == 'c') { vm.stepMode = false; runToBreak(vm); }
        else if (cmd[0] == 'r') dumpRegisters(vm);
        else if (cmd[0] == 'm') {
            std::cout << "Code@" << VM::CODE_BASE << " Data@" << VM::DATA_BASE
                      << " Heap@" << VM::HEAP_BASE << " Stack@" << VM::STACK_TOP << "\n";
        }
        else if (cmd[0] == 'b' && cmd.size() > 2)
            setBreakpoint(vm, (uint64_t)std::stoul(cmd.substr(2)));
    }
}
