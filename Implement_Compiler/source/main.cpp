#include "../include/Common.h"
#include "../include/Lexer.h"
#include "../include/Parser.h"
#include "../include/CodeGenerator.h"
#include "../include/CodeGenIR.h"
#include "../include/IRGen.h"
#include "../include/LogicalCode.h"
#include "../include/vm.h"
#include "../include/ExecIO.h"
#include "../include/loader.h"
#include "../include/debugger.h"
#include "../include/optimizier.h"
#include "../include/assembler.h"
#include <iostream>
#include <fstream>

static const char* DEMO_SOURCE = R"(
int main() {
    int a = 5;
    int b = 10;
    int c = 0;
    if (a < b) {
        c = a + b;
    }
    return c;
}
)";

int main(int argc, char* argv[]) {
    bool useExec = false;
    bool debugMode = false;
    std::string execPath = "program.exec";

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a == "--exec") useExec = true;
        else if (a == "--debug") debugMode = true;
        else execPath = a;
    }

    std::string source = DEMO_SOURCE;
    if (argc > 1 && !useExec && std::string(argv[1]) != "--exec" &&
        std::string(argv[1]) != "--debug") {
        std::ifstream f(argv[1]);
        if (f) source.assign((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
    }

    std::cout << "=== Full curriculum pipeline ===\n\n";

    std::cout << "[1] Lexer + Parser -> AST\n";
    Lexer lexer(source);
    auto tokens = lexer.tokenize();
    Parser parser(tokens);
    auto ast = parser.parseProgram();

    std::cout << "[2] AST -> Intermediate Code (IR)\n";
    IRGen irGen;
    IRProgram ir = irGen.generate(ast.get());

    std::cout << "[3] IR -> Logical Code (lower && || !)\n";
    LogicalCode::process(ir);

    std::cout << "[4] Logical Code -> Code Generator -> machine code\n";
    CodeGenIR backend;
    auto prog = backend.generate(ir);
    Optimizer opt;
    opt.optimize(prog);

    Assembler asm_;
    std::cout << "\n--- Assembler listing ---\n";
    std::cout << asm_.disassemble(prog);

    std::cout << "\n[5] Binary EXEC file (header, sections, sym, reloc, jump table)\n";
    if (ExecIO::write(execPath, prog))
        std::cout << "Wrote: " << execPath << "\n";

    VM vm;
    Loader loader;
    Debugger dbg;

    std::cout << "\n[6] Loader + VM monitor\n";
    if (useExec) {
        if (!loader.loadExec(execPath, vm)) return 1;
    } else {
        loader.load(vm);
        vm.loadProgram(prog.code, prog.constPool, prog.jumpTables);
    }

    vm.setXLen(XLen::W64);
    std::cout << "Segments: Code[" << VM::CODE_BASE << "] Data[" << VM::DATA_BASE
              << "] Heap[" << VM::HEAP_BASE << "] Stack[" << VM::STACK_TOP << "]\n";

    dbg.dumpState(vm);
    if (debugMode) {
        dbg.monitor(vm);
    } else {
        vm.run();
        std::cout << "\nResult R0 = " << vm.regs[0] << " (expected 15)\n";
        dbg.dumpRegisters(vm);
    }
    return 0;
}
