# RISC-V Emulator with Peripheral Support (RV32IM)

A lightweight RISC-V (RV32IM) CPU emulator in C++ with interrupt-capable hardware simulation, GPIO and UART peripheral support, and interactive hardware signal injection. Demonstrates memory-mapped I/O, CPU execution, and embedded systems concepts.

**The CPU currently handles I/O with Polling-based I/O, though Interrupt-driven I/O is supported.**

## Project Motivation

This project addresses the challenge of learning computer architecture and embedded systems in an accessible way:

- **Cost Barrier**: Real RISC-V development boards are expensive; this emulator runs on any computer
- **Hardware Risk**: Buggy embedded code can damage real devices; this sandbox is safe to experiment in
- **Transparency**: See exactly how CPU instructions execute, interrupts fire, and peripherals respond
- **Education**: Perfect teaching tool for understanding:
  - CPU instruction cycles and register state
  - Interrupt handling and control flow
  - Memory-mapped I/O and bus transactions
  - Real-time peripheral interaction

## Core Features

### CPU & Execution

- **RV32I Base Instructions**: LUI, AUIPC, JAL, JALR, branching, arithmetic/logical ops, loads/stores
- **RV32M Extension**: Multiplication, division, remainder operations
- **CSR Support**: Machine-level control/status registers (mstatus, mie, mtvec, mepc, mcause, mip, mtval)
- **Interrupt Handling**: Full trap/interrupt mechanism with vectored and direct modes
- **Step-by-step Debugging**: Execute and inspect state cycle-by-cycle or run full programs

### Peripherals & Hardware Simulation

- **GPIO (General Purpose I/O)**
  - 4 ports × 8 pins = 32 GPIO pins
  - Configurable input/output modes
  - Pin state monitoring and change detection
  - Individual pin interrupts (per-pin and per-port)
- **UART/Serial Communication**
  - Configurable baud rate
  - TX/RX buffering with cycle-accurate timing
  - Full duplex operation
- **Memory-Mapped I/O Bus**
  - Flexible peripheral addressing (0x1000-0x1FFF range)
  - MMIO register access for all peripherals
  - Interrupt aggregation and routing
- **Interactive Signal Injection**
  - Simulate real-world hardware events from the terminal
  - Press `p` to trigger button presses, `q` to quit

## Supported RISC-V Instructions (RV32IM)

### Base Instructions (RV32I)

- LUI, AUIPC
- JAL, JALR
- Branch: BEQ, BNE, BLT, BGE, BLTU, BGEU
- Immediate arithmetic/logical: ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI
- Register arithmetic/logical: ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
- Loads: LB, LH, LW, LBU, LHU
- Stores: SB, SH, SW
- ECALL, EBREAK

### M-extension (RV32M)

- MUL, MULH, MULHSU, MULHU
- DIV, DIVU
- REM, REMU

### CSR Instructions (Zicsr Extension)

- CSRRW, CSRRS, CSRRC
- CSRRWI, CSRRSI, CSRRCI

This instruction set covers the complete RV32IM base integer and multiplication extension, plus the Zicsr extension for control and status register manipulation. The emulator implements all required instructions for running standard RISC-V software, including interrupt handlers that rely on CSR operations.



### System Features

- 64MB configurable RAM with bounds checking
- Firmware + user program dual memory layout
- Boot sequence with `crt0.s` startup/firmware code
- Stack initialization and frame pointer setup
- Cycle-accurate timing simulation

## Quick Start

### Build

```bash
cd toy-riscv
make all          # Build emulator and all test programs
```

### Run Button/LED Demo (Interactive!)

```bash
./bin/rvemu bin/button_led_interrupt.bin

# During execution, press:
#   p - Simulate button press
#   q - Quit
```

Expected output:

```
=== Button/LED Interrupt Demo ===
Waiting for button press...

p
[BEFORE] Button: 0, LED: 0
Button pressed! Count: 1
[AFTER] Button: 1, LED: 1

p
[BEFORE] Button: 0, LED: 1
Button pressed! Count: 2
[AFTER] Button: 1, LED: 0
```

### Run Other Programs

```bash
./bin/rvemu bin/hello.bin          # Hello world
./bin/rvemu bin/fibonacci.bin      # Fibonacci sequence
./bin/rvemu bin/arithmatic.bin     # Arithmetic operations
```

## Syscalls (Environment Calls)

| **Number** | **Name**       | **Input**        | **Output** | **Description**                 |
| ---------- | -------------- | ---------------- | ---------- | ------------------------------- |
| 1          | `prtnum`     | `a0` = integer   | stdout     | Print 32-bit integer in decimal |
| 11         | `prtc`       | `a0` = char code | stdout     | Print single character          |
| 4          | `prts`       | `a0` = address   | stdout     | Print null-terminated string    |
| 5          | `scanint`    | `a0` = integer   |  stdin     | Read integer from stdin         |
| 8          | `scanstr`    | `a0` = address   |  stdin     | Read string from stdin          |
| 12         | `scanchar`   | `a0` = char code |  stdin     | Read character from stdin       |
| 10         | `exit`       | —                | —          | Stop execution                  |

## Memory-Mapped I/O (MMIO)

### GPIO Registers

**Port 0-3 Data & Mode** (per port):

```
0x1000 + port*0x08 + 0x00  = GPIO_DATA      (read/write pin state)
0x1000 + port*0x08 + 0x04  = GPIO_MODE      (pin direction: 0=input, 1=output)
```

Example: GPIO Port 0, Pin 0 (button)

```c
uint32_t button_state = *(uint32_t*)0x1000;          // Read pin state
*(uint32_t*)0x1004 = 0x00;                           // Set as input
```

### Interrupt Control

```
0x1100 = BUS_INTERRUPT_STATUS   (read-only: active interrupts)
0x1104 = BUS_INTERRUPT_ENABLE   (read/write: enable individual interrupts)
0x1108 = BUS_INTERRUPT_PENDING  (read-only: status & enable)
0x110C = BUS_INTERRUPT_CLEAR    (write: clear active interrupts)
```

### Interrupt Vectors

| **Vector** | **Source**     | **Description**                |
| ---------- | -------------- | ------------------------------ |
| 0-15       | GPIO Pins 0-15 | Individual GPIO pin interrupts |
| 16-17      | UART           | TX empty, RX data available    |
| 24-27      | GPIO Ports 0-3 | Port-wide interrupts           |

## Interactive Mode Features

The emulator runs in **step-by-step mode** with interactive input:

- **Button Press (`p`)**: Simulates GPIO pin 0 state change
  1. Sets GPIO_PORT0_DATA bit 0 to HIGH
  2. Runs 100 CPU cycles
  3. Sets GPIO_PORT0_DATA bit 0 to LOW
  4. Runs 20 more cycles for cleanup

- **Quit (`q`)**: Stops CPU execution cleanly

## Project Structure

```
toy-riscv/
├── README.md                      # This file
├── Makefile                       # Build system
├── src/
│   ├── main.cpp                  # Emulator core + interactive mode
│   ├── cpu/
│   │   ├── riscv.cpp             # CPU execution engine
│   │   └── riscv.hpp             # CPU interface
│   ├── environment/
│   │   ├── environment.hpp       # Abstract environment interface
│   │   └── simple_env.hpp        # Syscall handlers
│   ├── peripherals/
│   │   ├── bus.cpp/hpp           # Memory-mapped I/O bus
│   │   ├── GPIO/
│   │   │   ├── gpio.cpp/hpp      # GPIO controller
│   │   │   └── pinblock.hpp      # Pin group management
│   │   ├── UART/
│   │   │   └── uart.hpp          # Serial interface
│   │   ├── Signal/
│   │   │   └── signal.hpp        # Wire/signal abstraction
│   │   └── pin.hpp               # Physical pin representation
│   └── Device/
│       ├── device.cpp/hpp        # Base device class
│       ├── LED/
│       │   ├── led.cpp/hpp       # LED controller (prototype)
│       └── Button/
│           ├── button.cpp/hpp    # Button controller (prototype)
├── startup/
│   └── crt0.s                   # Firmware + interrupt vector table
├── include/
│   ├── linker.ld               # Linker script
│   ├── math.c/h                # Math utility functions
│   └── stndio.c/h              # Standard I/O functions
├── programs/
│   ├── bootloader/
│   │   └── boot.s              # Bootloader
│   └── user/
│       ├── assembly/
│       │   ├── hello.s
│       │   ├── fibonacci.s
│       │   ├── arithmatic.s
│       │   ├── loop.s
│       │   └── button_led_interrupt.s
│       └── c/
│           ├── main.c
│           ├── fibo.c
│           └── button_led_interrupt.c    # **Interactive GPIO demo!**
├── scripts/
│   └── run_menu.sh            # Interactive test menu
├── bin/                        # Compiled binaries (generated)
├── build/                      # Build artifacts (generated)
└── image/                      # (Reserved for disk image export)
```

## Building & Compiling

### Prerequisites

- **C++ Compiler**: g++ with C++11 support
- **RISC-V Toolchain**: `riscv64-unknown-elf-*` (for compiling user programs)
- **Make**: GNU Make
- **Bash**: For utility scripts

### Build Commands

```bash
make all              # Build emulator + all programs
make clean            # Remove build artifacts
make tests            # Build only user programs
./bin/rvemu <prog>   # Run compiled program
```

### Makefile Targets

```
all              - Build emulator and all programs
clean            - Remove build artifacts
tests            - Build user programs only
rvemu            - Build emulator only
```

## Learning Resources

This project demonstrates:

1. **Computer Architecture**
   - CPU instruction execution pipeline
   - Register state management
   - Memory hierarchy and MMIO

2. **Embedded Systems**
   - GPIO control and polling
   - Interrupt-driven programming
   - Hardware abstraction layers

3. **RISC-V ISA**
   - Base integer instruction set
   - Privilege modes
   - Control and status registers

## University Use

**Perfect for teaching**:

- Computer organization and architecture
- Operating systems and exception handling
- Embedded systems design
- Hardware-software co-design
- Compiler/assembler backends

**Advantages for coursework**:

- Safe sandbox for buggy code
- Step-by-step debugging capability
- Complete system control and visibility
- Runs on laptops without special hardware
- Open source and customizable

## Notes

- Programs load at 0x2000 (user space)
- Firmware (if present) loads at 0x0000
- Stack grows downward from 0x4000000
- Cycle count available via `get_cycles()`
- All memory accesses go through the bus (MMIO-compatible)

## Future Enhancements

- [ ] Interrupt mode configuration for GPIO pins
- [ ] SPI/I2C peripheral support
- [ ] Timer/PWM functionality
- [ ] Real filesystem integration
- [ ] GDB remote debugging support
- [ ] Performance profiling tools
- [ ] Graphical peripheral visualization

## License

MIT License © 2026 Tamvir Shahabuddin
