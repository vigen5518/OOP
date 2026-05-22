#include "ExecIO.h"
#include <fstream>
#include <cstring>
#include <map>
#include <vector>

#pragma pack(push, 1)
struct ExecFileOnDisk {
    char signature[4];
    uint32_t headerSize;
    uint32_t sectionCount;
    uint32_t xlenBits;
    SectionHeader sections[5];
};
#pragma pack(pop)

static constexpr uint32_t SEC_CODE = 0;
static constexpr uint32_t SEC_DATA = 1;
static constexpr uint32_t SEC_SYMTAB = 2;
static constexpr uint32_t SEC_RELOC = 3;
static constexpr uint32_t SEC_JUMP = 4;

bool ExecIO::write(const std::string& path, const CodeGen::Program& prog) {
    std::vector<uint8_t> blob;
    auto append = [&](const void* p, size_t n) {
        const uint8_t* b = (const uint8_t*)p;
        blob.insert(blob.end(), b, b + n);
    };

    size_t jumpDataSize = 0;
    for (auto& jt : prog.jumpTables)
        jumpDataSize += sizeof(int32_t) + sizeof(uint32_t) + jt.targets.size() * sizeof(uint32_t);

    size_t hdrSize = sizeof(ExecFileOnDisk);
    size_t codeOff = hdrSize, codeSize = prog.code.size() * sizeof(Instruction);
    size_t dataOff = codeOff + codeSize, dataSize = prog.constPool.size() * sizeof(double);
    size_t symOff = dataOff + dataSize, symSize = prog.symbols.size() * sizeof(SymbolEntry);
    size_t relOff = symOff + symSize, relSize = prog.relocs.size() * sizeof(RelocEntry);
    size_t jmpOff = relOff + relSize;

    ExecFileOnDisk hdr{};
    hdr.signature[0] = 'E'; hdr.signature[1] = 'X';
    hdr.signature[2] = 'E'; hdr.signature[3] = 'C';
    hdr.headerSize = (uint32_t)hdrSize;
    hdr.sectionCount = 5;
    hdr.xlenBits = 64;
    hdr.sections[0] = { SEC_CODE, (uint32_t)codeSize, (uint32_t)codeOff };
    hdr.sections[1] = { SEC_DATA, (uint32_t)dataSize, (uint32_t)dataOff };
    hdr.sections[2] = { SEC_SYMTAB, (uint32_t)symSize, (uint32_t)symOff };
    hdr.sections[3] = { SEC_RELOC, (uint32_t)relSize, (uint32_t)relOff };
    hdr.sections[4] = { SEC_JUMP, (uint32_t)jumpDataSize, (uint32_t)jmpOff };

    append(&hdr, sizeof(hdr));
    if (!prog.code.empty()) append(prog.code.data(), codeSize);
    if (!prog.constPool.empty()) append(prog.constPool.data(), dataSize);

    for (const auto& s : prog.symbols) {
        SymbolEntry e{};
        size_t n = s.name.size() < 31 ? s.name.size() : 31;
        memcpy(e.name, s.name.c_str(), n);
        e.address = s.address;
        e.type = s.type;
        append(&e, sizeof(e));
    }

    std::map<std::string, uint32_t> symIndex;
    for (uint32_t i = 0; i < (uint32_t)prog.symbols.size(); i++)
        symIndex[prog.symbols[i].name] = i;

    for (const auto& r : prog.relocs) {
        RelocEntry e{};
        e.instrOffset = r.instrIndex * (uint32_t)sizeof(Instruction);
        auto it = symIndex.find(r.targetName);
        e.symbolIndex = it != symIndex.end() ? it->second : 0;
        append(&e, sizeof(e));
    }

    for (auto& jt : prog.jumpTables) {
        int32_t minV = jt.minVal;
        uint32_t cnt = (uint32_t)jt.targets.size();
        append(&minV, sizeof(minV));
        append(&cnt, sizeof(cnt));
        if (!jt.targets.empty())
            append(jt.targets.data(), jt.targets.size() * sizeof(uint32_t));
    }

    std::ofstream out(path, std::ios::binary);
    if (!out) return false;
    out.write((const char*)blob.data(), (std::streamsize)blob.size());
    return out.good();
}

bool ExecIO::load(const std::string& path, VM& vm) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;
    in.seekg(0, std::ios::end);
    size_t sz = (size_t)in.tellg();
    in.seekg(0);
    std::vector<uint8_t> blob(sz);
    in.read((char*)blob.data(), (std::streamsize)sz);
    if (sz < sizeof(ExecFileOnDisk)) return false;

    ExecFileOnDisk hdr;
    memcpy(&hdr, blob.data(), sizeof(hdr));

    std::vector<Instruction> code;
    std::vector<double> pool;
    std::vector<CodeGen::Program::JumpTable> jumpTables;

    auto readSec = [&](uint32_t type, auto fn) {
        for (uint32_t i = 0; i < hdr.sectionCount && i < 5; i++) {
            if (hdr.sections[i].type == type) {
                size_t off = hdr.sections[i].offset;
                size_t size = hdr.sections[i].size;
                if (off + size <= blob.size()) fn(off, size);
            }
        }
    };

    readSec(SEC_CODE, [&](size_t off, size_t size) {
        code.resize(size / sizeof(Instruction));
        memcpy(code.data(), blob.data() + off, size);
    });
    readSec(SEC_DATA, [&](size_t off, size_t size) {
        pool.resize(size / sizeof(double));
        memcpy(pool.data(), blob.data() + off, size);
    });
    readSec(SEC_JUMP, [&](size_t off, size_t size) {
        size_t pos = off;
        while (pos + 8 <= off + size) {
            CodeGen::Program::JumpTable jt;
            memcpy(&jt.minVal, blob.data() + pos, 4); pos += 4;
            uint32_t cnt = 0;
            memcpy(&cnt, blob.data() + pos, 4); pos += 4;
            jt.targets.resize(cnt);
            if (cnt > 0) {
                memcpy(jt.targets.data(), blob.data() + pos, cnt * 4);
                pos += cnt * 4;
            }
            jumpTables.push_back(jt);
        }
    });

    std::vector<SymbolEntry> syms;
    std::vector<RelocEntry> relocs;
    readSec(SEC_SYMTAB, [&](size_t off, size_t size) {
        syms.resize(size / sizeof(SymbolEntry));
        memcpy(syms.data(), blob.data() + off, size);
    });
    readSec(SEC_RELOC, [&](size_t off, size_t size) {
        relocs.resize(size / sizeof(RelocEntry));
        memcpy(relocs.data(), blob.data() + off, size);
    });

    for (const auto& r : relocs) {
        if (r.symbolIndex < syms.size()) {
            uint32_t idx = r.instrOffset / (uint32_t)sizeof(Instruction);
            if (idx < code.size())
                code[idx].rs1 = syms[r.symbolIndex].address;
        }
    }

    vm.loadProgram(code, pool, jumpTables);
    vm.pc = 0;
    if (hdr.xlenBits == 16) vm.setXLen(XLen::W16);
    else if (hdr.xlenBits == 32) vm.setXLen(XLen::W32);
    else vm.setXLen(XLen::W64);
    return true;
}
