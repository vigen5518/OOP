#pragma once

#include "Common.h"
#include "CodeGenerator.h"
#include "vm.h"
#include <string>

class ExecIO {
public:
    static bool write(const std::string& path, const CodeGen::Program& prog);
    static bool load(const std::string& path, VM& vm);
};
