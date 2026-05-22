#pragma once

#include "vm.h"
#include <string>

class Loader {
public:
    void load(VM& vm);
    bool loadExec(const std::string& path, VM& vm);
};
