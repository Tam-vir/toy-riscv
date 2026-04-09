# Minimal RISC-V Emulator (RV32I/M)

A lightweight RISC-V (RV32IM) CPU emulator in C++ with basic environment syscalls, `crt0` and stack setup, capable of running hand-written assembly and simple C programs.

## Features

* Implements **RV32I** base integer instructions.
* Supports **RV32M** extension (multiplication/division/remainder).
* Basic system calls (`ecall`) via `SimpleEnvironment`:

  * Print character, integer (decimal, hexadecimal, binary)
  * Print string (null-terminated or fixed-length)
  * Print newline or space
  * Print memory dump
  * Print register dump
  * Exit
* Handles `crt0` and stack initialization.
* Simple memory model with bounds checking.
* Step-by-step execution (`step()`) or full run (`run()`).
* Modular design: CPU (`RISCV`) and environment (`Environment` / `SimpleEnvironment`).

## Getting Started

### Prerequisites

* C++17 compatible compiler (tested with `g++`)
* CMake or simple Makefile for building (optional)
* RISC-V toolchain (e.g., `riscv32-elf-gcc`) for compiling assembly programs.
* Bash (To make life easier)

### Example Syscalls

* **Print integer**: `a7 = 1`, `a0 = value`
* **Print string**: `a7 = 4`, `a0 = address`
* **Exit program**: `a7 = 10`

### Memory Access

```cpp
cpu.load8(addr);
cpu.load16(addr);
cpu.load32(addr);

cpu.store8(addr, val);
cpu.store16(addr, val);
cpu.store32(addr, val);
```

### Step Execution

```cpp
while(cpu.is_running()) {
    cpu.step(); // executes single instruction
}
```

## Instruction Support

### Base Instructions (RV32I)

* LUI, AUIPC
* JAL, JALR
* Branch: BEQ, BNE, BLT, BGE, BLTU, BGEU
* Immediate arithmetic/logical: ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI
* Register arithmetic/logical: ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
* Loads: LB, LH, LW, LBU, LHU
* Stores: SB, SH, SW
* ECALL, EBREAK

### M-extension (RV32M)

* MUL, MULH, MULHSU, MULHU
* DIV, DIVU
* REM, REMU

## Project Structure

```
.
├── include
│   ├── linker.ld
│   ├── math.c
│   ├── math.h
│   ├── stdo.c
│   └── stdo.h
├── Makefile
├── programs
│   ├── assembly
│   │   ├── arithmatic.s
│   │   ├── fibonacci.s
│   │   ├── hello.s
│   │   └── loop.s
│   └── c
│       ├── asm_test.c
│       ├── main.c
│       └── watermelon.c
├── README.md
├── scripts
│   └── run_menu.sh
├── src
│   ├── cpu
│   │   ├── riscv.cpp
│   │   └── riscv.hpp
│   ├── environment
│   │   ├── environment.hpp
│   │   └── simple_env.hpp
│   └── main.cpp
└── startup
    └── crt0.s
```
## Example Programs
`hello.s`
```asm
# Simple RISC-V program that prints "Hello, World!"
.text
.global _start
_start:
    # Load string address
    lui a0, %hi(msg)
    addi a0, a0, %lo(msg)
    
    # Set syscall: print string
    li a7, 4
    ecall
    
    # Exit
    li a7, 10
    ecall

.data
msg:
    .string "Hello, World!\n"
```
`Output`
```
========================================
Running: hello
========================================
Loading program: bin/hello.bin (4135 bytes)
Starting execution...
--- Program output ---

Hello, World!

--- End of program ---
Program exited successfully.
========================================
```


## Run
```
bash scripts/run_menu.sh
```
## Notes

* Register `x0` is always zero.
* Memory bounds are checked; accessing out-of-bounds addresses throws exceptions.
* Currently, CSR instructions are not implemented.
* All RISC-V integer arithmetic matches the spec, including overflow/division by zero rules.

## Possible Future Updates

* Implement additional RISC-V extensions (F, D, A, C, etc.)

* Full CSR instruction support

* Exception and interrupt handling

* Support for system-level features like virtual memory

* Debugger interface for stepping, breakpoints, and watchpoints
* File I/O and more complex syscalls

* Optimization of memory access for larger programs

* Integration with assembly/C compiler toolchains for automated program loading

* GUI or terminal-based visualization of registers and memory

* Multi-core CPU emulation 

## License

MIT License © 2026 Tamvir Shahabuddin
