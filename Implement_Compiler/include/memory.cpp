#include "../include/memory.h"

Memory::Memory(size_t size) {

    ram.resize(size);
}

uint8_t Memory::read8(uint64_t addr) {

    return ram[addr];
}

void Memory::write8(
    uint64_t addr,
    uint8_t value
) {
    ram[addr] = value;
}