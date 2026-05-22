#include "assembler.h"
#include "Common.h"
#include <sstream>
#include <map>

void Assembler::assemble() {}

std::string Assembler::disassemble(const CodeGen::Program& prog) {
    std::ostringstream out;
    for (size_t i = 0; i < prog.code.size(); ++i) {
        const Instruction& in = prog.code[i];
        uint32_t opIdx = in.op;
        const char* name = (opIdx < 32) ? opCodeNames[opIdx] : "?";
        out << i << ": " << name << " rd=" << in.rd
            << " rs1=" << in.rs1 << " rs2=" << in.rs2 << "\n";
    }
    return out.str();
}

static std::map<std::string, OpCode> mnemonicMap() {
    std::map<std::string, OpCode> m;
    for (int i = 0; i < 32; i++)
        m[opCodeNames[i]] = (OpCode)i;
    return m;
}

CodeGen::Program Assembler::assembleSource(const std::string& asmText) {
    CodeGen::Program prog;
    auto mnem = mnemonicMap();
    std::istringstream in(asmText);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == ';') continue;
        std::istringstream ls(line);
        std::string op, a, b, c;
        ls >> op >> a >> b >> c;
        auto it = mnem.find(op);
        if (it == mnem.end()) continue;
        Instruction ins{};
        ins.op = (uint32_t)it->second;
        try {
            ins.rd = a.empty() ? 0 : (uint32_t)std::stoul(a);
            ins.rs1 = b.empty() ? 0 : (uint32_t)std::stoul(b);
            ins.rs2 = c.empty() ? 0 : (uint32_t)std::stoul(c);
        } catch (...) { continue; }
        prog.code.push_back(ins);
    }
    return prog;
}
