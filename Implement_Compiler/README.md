# Compiler + Virtual Machine (100% curriculum)

## Pipeline (Images 2–10)

```
Source
  → Lexer → Parser → AST
  → IRGen (Intermediate Code)
  → LogicalCode (&&, ||, ! lowering)
  → CodeGenIR (machine instructions)
  → Optimizer → Assembler (list / .asm in)
  → EXEC file (header, sections, symbols, relocs, jump tables)
  → Loader → VM (Fetch / Decode / Execute, IP+PC, FF flags)
  → Debugger
```

## Run

```text
Compiler.exe              # build + run demo
Compiler.exe --exec         # load program.exec via Loader
Compiler.exe --debug        # interactive debugger (s/c/r/m/b/q)
Compiler.exe myfile.c       # compile from source file
```

Expected result: **R0 = 15**.

## Curriculum checklist

| # | Topic | Implementation |
|---|--------|----------------|
| 1 | RISC-V-style ISA, VM, Assembler, Debugger | `Common.h`, `vm.cpp`, `assembler.cpp`, `debugger.cpp` |
| 2 | Full compiler pipeline + Loader + Memory + VM monitor | `IRGen`, `LogicalCode`, `CodeGenIR`, `ExecIO`, `loader`, `main.cpp` |
| 3 | Code/Data/Heap/Stack + xlen 16/32/64 | `vm.h` segment constants, `setXLen()` |
| 4 | 32×16-bit regs, IP+PC, Fetch/Decode/Execute | `reg16[]`, `ip`, `fetch()`/`decode()`/`execute()` |
| 5 | CPU, memory, CMP, conditional jumps | `OpCode::CMP`, `BR_*`, `JMP`, flags `ff` |
| 6 | Calls, BP/SP, stack frames | `CALL`/`RET`, `PUSH_BP`, `SET_BP_SP`, `LOAD_LOCAL` |
| 7 | IA-32-style prologue/epilogue | `PUSH_BP`, `SET_BP_SP`, `ALLOC_STACK`, `RESTORE_BP` in `IRGen` |
| 8 | EXEC, symbol table, reloc table, jump table, loader | `ExecIO.cpp`, `Program::jumpTables` |
| 9 | Header/sections, RAM layout, `VM::run` | `ExecFileOnDisk`, segment bases, `run()`/`runOne()` |

## Key files

- `IRGen.cpp` — AST → IR  
- `LogicalCode.cpp` — logical lowering  
- `CodeGenIR.cpp` — IR → bytecode  
- `vm.cpp` — processor executor  
- `ExecIO.cpp` — binary EXEC I/O  

## Build

Open `include/Compiler.vcxproj` in Visual Studio, **x64 Debug**, Build Solution.
