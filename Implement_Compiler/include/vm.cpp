#include "vm.h"
#include <stdexcept>
#include <cstring>

VM::VM() {
    memory.resize(MEMORY_SIZE, 0);
    sp = STACK_TOP;
    bp = STACK_TOP;
}

void VM::setXLen(XLen w) { xlen = w; syncRegs(); }

void VM::syncRegs() {
    for (int i = 0; i < (int)REGISTER_COUNT; i++) {
        if (xlen == XLen::W16)
            regs[i] = reg16[i];
        else
            reg16[i] = (uint16_t)(regs[i] & 0xFFFF);
    }
}

uint64_t VM::maskVal(uint64_t v) const {
    switch (xlen) {
    case XLen::W16: return v & 0xFFFFu;
    case XLen::W32: return v & 0xFFFFFFFFu;
    default: return v;
    }
}

void VM::mapCodeToMemory() {
    uint64_t addr = CODE_BASE;
    for (auto& in : code) {
        if (addr + 4 > memory.size()) break;
        memcpy(&memory[addr], &in, sizeof(Instruction));
        addr += 4;
    }
    ip = CODE_BASE;
}

void VM::loadProgram(const std::vector<Instruction>& c,
                     const std::vector<double>& pool,
                     const std::vector<CodeGen::Program::JumpTable>& jt) {
    code = c;
    constPool = pool;
    jumpTables = jt;
    pc = 0;
    ip = CODE_BASE;
    running = true;
    mapCodeToMemory();
}

Instruction VM::fetch() {
    if (pc >= code.size())
        return { (uint32_t)OpCode::HALT, 0, 0, 0 };
    ip = CODE_BASE + pc * 4;
    return code[pc];
}

void VM::decode(const Instruction& instr) {
    decoded = instr;
    decodedValid = true;
}

void VM::pushStack(uint64_t v) {
    if (sp < HEAP_BASE + 8) throw std::runtime_error("Stack overflow");
    sp -= 8;
    writeMem(sp, maskVal(v));
}

uint64_t VM::popStack() {
    if (sp >= STACK_TOP - 7) throw std::runtime_error("Stack underflow");
    uint64_t v = readMem(sp);
    sp += 8;
    return v;
}

uint64_t VM::readMem(uint64_t addr) const {
    if (addr + 8 > memory.size()) return 0;
    uint64_t v = 0;
    for (int i = 7; i >= 0; --i)
        v = (v << 8) | memory[addr + (size_t)i];
    return maskVal(v);
}

void VM::writeMem(uint64_t addr, uint64_t v) {
    if (addr + 8 > memory.size()) return;
    v = maskVal(v);
    for (int i = 0; i < 8; ++i) {
        memory[addr + (size_t)i] = (uint8_t)(v & 0xFF);
        v >>= 8;
    }
}

uint64_t VM::heapAlloc(uint32_t bytes) {
    uint64_t p = heapPtr;
    heapPtr += (bytes + 7) & ~7u;
    return p;
}

bool VM::evalCmp(CmpMode mode, uint64_t a, uint64_t b) const {
    int64_t sa = (int64_t)maskVal(a), sb = (int64_t)maskVal(b);
    switch (mode) {
    case CMP_EQ:  return sa == sb;
    case CMP_NEQ: return sa != sb;
    case CMP_LT:  return sa < sb;
    case CMP_GT:  return sa > sb;
    case CMP_LTE: return sa <= sb;
    case CMP_GTE: return sa >= sb;
    default: return sa == sb;
    }
}

void VM::branchIf(bool take, uint64_t target) {
    if (take) pc = target;
}

void VM::runOne() {
    if (!running || pc >= code.size()) return;
    if (pc == breakPc) stepMode = true;
    Instruction instr = fetch();
    pc++;
    decode(instr);
    execute();
}

void VM::run() {
    while (running && pc < code.size()) {
        if (stepMode) break;
        runOne();
    }
}

void VM::execute() {
    if (!decodedValid) return;
    const Instruction& instr = decoded;
    OpCode op = (OpCode)instr.op;
    uint32_t rd = instr.rd, rs1 = instr.rs1, rs2 = instr.rs2;

    switch (op) {
    case OpCode::MOV_CONST:
        if (rs1 < constPool.size()) {
            regs[rd] = maskVal((uint64_t)constPool[rs1]);
            reg16[rd] = (uint16_t)regs[rd];
        }
        break;
    case OpCode::LOAD_VAR:
        regs[rd] = readMem(DATA_BASE + rs1 * 8);
        reg16[rd] = (uint16_t)regs[rd];
        break;
    case OpCode::STORE_VAR:
        writeMem(DATA_BASE + rs1 * 8, regs[rd]);
        break;
    case OpCode::ADD:
        regs[rd] = maskVal(regs[rs1] + regs[rs2]);
        reg16[rd] = (uint16_t)regs[rd];
        break;
    case OpCode::SUB:
        regs[rd] = maskVal(regs[rs1] - regs[rs2]);
        reg16[rd] = (uint16_t)regs[rd];
        break;
    case OpCode::MUL:
        regs[rd] = maskVal(regs[rs1] * regs[rs2]);
        reg16[rd] = (uint16_t)regs[rd];
        break;
    case OpCode::DIV:
        if (regs[rs2]) { regs[rd] = maskVal(regs[rs1] / regs[rs2]); reg16[rd] = (uint16_t)regs[rd]; }
        break;
    case OpCode::MOD:
        if (regs[rs2]) { regs[rd] = maskVal(regs[rs1] % regs[rs2]); reg16[rd] = (uint16_t)regs[rd]; }
        break;
    case OpCode::CMP: {
        CmpMode mode = (CmpMode)(rs2 & 7);
        uint32_t r2 = (rs2 >> 3) & 0x1FF;
        bool ok = evalCmp(mode, regs[rs1], regs[r2]);
        regs[rd] = ok ? 1 : 0;
        reg16[rd] = (uint16_t)regs[rd];
        cmpFlags = ok ? 0 : ((int64_t)maskVal(regs[rs1]) < (int64_t)maskVal(regs[r2]) ? -1 : 1);
        ff = (uint64_t)(cmpFlags + 1);
        break;
    }
    case OpCode::BR_EQ: branchIf(regs[rd] == 0, rs2); break;
    case OpCode::BR_NEQ: branchIf(regs[rd] != 0, rs1); break;
    case OpCode::BR_LT: branchIf((int64_t)regs[rd] < 0, rs2 ? rs2 : rs1); break;
    case OpCode::BR_GT: branchIf((int64_t)regs[rd] > 0, rs2 ? rs2 : rs1); break;
    case OpCode::BR_LTE: branchIf((int64_t)regs[rd] <= 0, rs2); break;
    case OpCode::BR_GTE: branchIf((int64_t)regs[rd] >= 0, rs2); break;
    case OpCode::JMP: pc = rs1; break;
    case OpCode::PUSH: pushStack(regs[rd]); break;
    case OpCode::POP: regs[rd] = popStack(); reg16[rd] = (uint16_t)regs[rd]; break;
    case OpCode::PUSH_BP: pushStack(bp); break;
    case OpCode::SET_BP_SP: bp = sp; break;
    case OpCode::ALLOC_STACK: sp -= rs1; break;
    case OpCode::RESTORE_BP: sp = bp; bp = popStack(); break;
    case OpCode::LOAD_LOCAL: {
        int64_t off = (int64_t)bp - (int64_t)(rs1 * 8);
        regs[rd] = readMem((uint64_t)off);
        reg16[rd] = (uint16_t)regs[rd];
        break;
    }
    case OpCode::STORE_LOCAL: {
        int64_t off = (int64_t)bp - (int64_t)(rs1 * 8);
        writeMem((uint64_t)off, regs[rd]);
        break;
    }
    case OpCode::CALL: {
        callStack.push_back(pc);
        callStack.push_back(bp);
        callStack.push_back(rd);
        bp = sp;
        pc = rs1;
        break;
    }
    case OpCode::RET: {
        uint64_t retVal = maskVal(regs[rd]);
        regs[0] = retVal;
        reg16[0] = (uint16_t)retVal;
        if (!callStack.empty()) {
            uint64_t resReg = callStack.back(); callStack.pop_back();
            bp = callStack.back(); callStack.pop_back();
            pc = callStack.back(); callStack.pop_back();
            regs[resReg] = retVal;
            sp = bp;
        }
        break;
    }
    case OpCode::JMP_TABLE: {
        if (rs1 < jumpTables.size()) {
            auto& jt = jumpTables[rs1];
            uint64_t idx = regs[rd];
            if (idx < jt.targets.size())
                pc = jt.targets[(size_t)idx];
        }
        break;
    }
    case OpCode::HEAP_ALLOC:
        regs[rd] = heapAlloc(rs1);
        reg16[rd] = (uint16_t)regs[rd];
        break;
    case OpCode::LOAD_MEM:
        regs[rd] = readMem(regs[rs1] + regs[rs2]);
        reg16[rd] = (uint16_t)regs[rd];
        break;
    case OpCode::STORE_MEM:
        writeMem(regs[rs1] + regs[rs2], regs[rd]);
        break;
    case OpCode::HALT:
        running = false;
        break;
    default:
        break;
    }
    syncRegs();
}
