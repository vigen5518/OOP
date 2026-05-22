#include "loader.h"
#include "ExecIO.h"

void Loader::load(VM& vm) {
    vm.pc = 0;
    vm.running = true;
    vm.sp = VM::STACK_TOP;
    vm.bp = VM::STACK_TOP;
}

bool Loader::loadExec(const std::string& path, VM& vm) {
    if (!ExecIO::load(path, vm))
        return false;
    load(vm);
    return true;
}
