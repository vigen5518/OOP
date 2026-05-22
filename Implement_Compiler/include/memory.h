#pragma once

#include <vector>
#include <cstdint>

class Memory {

public:

    std::vector<uint8_t> ram;

    Memory(size_t size);

    uint8_t read8(uint64_t addr);

    void write8(uint64_t addr, uint8_t value);
};