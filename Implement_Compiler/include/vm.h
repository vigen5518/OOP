#pragma once

#include "Common.h"
#include "CodeGenerator.h"
#include <vector>
#include <cstdint>

enum class Segment : uint32_t {
    Code = 0, Data = 1, SymbolTable = 2, RelocTable = 3, JumpTable = 4
};

enum class XLen : uint32_t { W16 = 16, W32 = 32, W64 = 64 };

enum CmpMode : uint32_t {
    CMP_EQ = 0, CMP_NEQ, CMP_LT, CMP_GT, CMP_LTE, CMP_GTE
};

class VM {
public:
    static constexpr uint32_t MEMORY_SIZE = 1024 * 1024;
    static constexpr uint32_t REGISTER_COUNT = 32;
    static constexpr uint32_t CODE_BASE = 0;
    static constexpr uint32_t DATA_BASE = 64 * 1024;
    static constexpr uint32_t HEAP_BASE = 512 * 1024;
    static constexpr uint32_t STACK_TOP = MEMORY_SIZE - 1;

    uint16_t reg16[REGISTER_COUNT]{};  // 32 x 16-bit view (Image 5)
    uint64_t regs[REGISTER_COUNT]{};   // wide view
    uint64_t pc = 0;
    uint64_t ip = 0;                 // byte IP in code segment (Image 5)
    uint64_t sp = STACK_TOP;
    uint64_t bp = STACK_TOP;
    uint64_t heapPtr = HEAP_BASE;

    uint64_t ff = 0;                 // flags register (Image 6)
    int64_t cmpFlags = 0;

    bool running = true;
    bool stepMode = false;
    uint64_t breakPc = UINT64_MAX;
    XLen xlen = XLen::W64;

    Instruction decoded{};
    bool decodedValid = false;

    std::vector<uint8_t> memory;
    std::vector<Instruction> code;
    std::vector<double> constPool;
    std::vector<CodeGen::Program::JumpTable> jumpTables;
    std::vector<uint64_t> callStack;

    VM();

    void setXLen(XLen w);
    uint64_t maskVal(uint64_t v) const;
    void syncRegs();

    Instruction fetch();
    void decode(const Instruction& instr);
    void execute();

    void run();
    void runOne();

    void loadProgram(const std::vector<Instruction>& c,
                     const std::vector<double>& pool,
                     const std::vector<CodeGen::Program::JumpTable>& jt = {});

    void pushStack(uint64_t v);
    uint64_t popStack();
    uint64_t readMem(uint64_t addr) const;
    void writeMem(uint64_t addr, uint64_t v);
    uint64_t heapAlloc(uint32_t bytes);

    bool evalCmp(CmpMode mode, uint64_t a, uint64_t b) const;
    void branchIf(bool take, uint64_t target);
    void mapCodeToMemory();
};
