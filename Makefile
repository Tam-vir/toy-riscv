# RISC-V Emulator Makefile

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
PROGRAMS_DIR = programs
USER_C_DIR = $(PROGRAMS_DIR)/user/c
USER_ASM_DIR = $(PROGRAMS_DIR)/user/assembly
BOOTLOADER_DIR = $(PROGRAMS_DIR)/bootloader
INCLUDE_DIR = include

# Memory addresses
BOOTLOADER_BASE = 0x0000
USER_PROGRAM_BASE = 0x2000

# RISC-V architecture flags (include zicsr extension for CSR instructions)
RISCV_ARCH = rv32im_zicsr
RISCV_ABI = ilp32

# Linker script
LINKER_SCRIPT = $(INCLUDE_DIR)/linker.ld

# Runtime C files inside include
INCLUDE_C_SRCS = $(wildcard $(INCLUDE_DIR)/*.c)
INCLUDE_C_OBJS = $(patsubst $(INCLUDE_DIR)/%.c,$(BUILD_DIR)/include_%.o,$(INCLUDE_C_SRCS))

# Compiler and tools
CXX = g++
CXXFLAGS = -std=c++11 -I$(SRC_DIR) -I$(INCLUDE_DIR) -Wall -Wextra
RISCV_CC = riscv64-elf-gcc
RISCV_AS = riscv64-elf-as
RISCV_LD = riscv64-elf-ld
RISCV_OBJCOPY = riscv64-elf-objcopy

# Source files (only .cpp files that actually exist)
SRCS = $(SRC_DIR)/main.cpp \
       $(SRC_DIR)/cpu/riscv.cpp \
       $(SRC_DIR)/peripherals/bus.cpp \
       $(SRC_DIR)/peripherals/GPIO/gpio.cpp

# Object files (preserve directory structure in build)
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SRCS))

# Target
TARGET = $(BIN_DIR)/rvemu

# Startup files for different program types
STARTUP_C_SRC = startup/c/crt0.s
STARTUP_ASM_SRC = startup/asm/crt0.s
STARTUP_C_OBJ = $(BUILD_DIR)/crt0_c.o
STARTUP_ASM_OBJ = $(BUILD_DIR)/crt0_asm.o

# Find all assembly test files (user assembly)
ASM_TESTS = $(wildcard $(USER_ASM_DIR)/*.s)
ASM_BINS = $(patsubst $(USER_ASM_DIR)/%.s,$(BIN_DIR)/%.bin,$(ASM_TESTS))

# Find all C program files (user C)
C_PROGRAMS = $(wildcard $(USER_C_DIR)/*.c)
C_BINS = $(patsubst $(USER_C_DIR)/%.c,$(BIN_DIR)/%.bin,$(C_PROGRAMS))

# Find bootloader files
BOOTLOADER_FILES = $(wildcard $(BOOTLOADER_DIR)/*.s)
BOOTLOADER_BINS = $(patsubst $(BOOTLOADER_DIR)/%.s,$(BIN_DIR)/bootloader_%.bin,$(BOOTLOADER_FILES))

# All binaries to build
ALL_BINS = $(BOOTLOADER_BINS) $(ASM_BINS) $(C_BINS)

# Default target
all: $(TARGET) $(ALL_BINS)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile C++ source files - handle nested directories
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build all test programs
.PHONY: tests
tests: $(ALL_BINS)

# Build bootloader programs (start at 0x0000) with zicsr extension
# Bootloader uses simple -Ttext approach
$(BIN_DIR)/bootloader_%.bin: $(BOOTLOADER_DIR)/%.s
	@echo "Building bootloader: $* (base address: $(BOOTLOADER_BASE))"
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)
	$(RISCV_AS) -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -o $(BUILD_DIR)/bootloader_$*.o $<
	$(RISCV_LD) -m elf32lriscv -Ttext=$(BOOTLOADER_BASE) -o $(BUILD_DIR)/bootloader_$*.elf $(BUILD_DIR)/bootloader_$*.o
	$(RISCV_OBJCOPY) -O binary $(BUILD_DIR)/bootloader_$*.elf $@

# Build user assembly programs with assembly-specific startup file and linker script
$(BIN_DIR)/%.bin: $(USER_ASM_DIR)/%.s $(STARTUP_ASM_SRC) $(LINKER_SCRIPT)
	@echo "Building user assembly: $* (using startup/asm/crt0.s and linker script)"
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)
	
	# Assemble the user assembly program
	$(RISCV_AS) -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -o $(BUILD_DIR)/$*.o $<
	
	# Assemble the assembly-specific crt0 startup code
	$(RISCV_AS) -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -o $(BUILD_DIR)/crt0_asm_$*.o $(STARTUP_ASM_SRC)
	
	# Link with startup code and linker script
	$(RISCV_LD) -m elf32lriscv -T $(LINKER_SCRIPT) \
		-o $(BUILD_DIR)/$*.elf \
		$(BUILD_DIR)/crt0_asm_$*.o \
		$(BUILD_DIR)/$*.o
	
	# Convert to binary
	$(RISCV_OBJCOPY) -O binary $(BUILD_DIR)/$*.elf $@

# Compile include/*.c runtime files
$(BUILD_DIR)/include_%.o: $(INCLUDE_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(RISCV_CC) -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -nostdlib -ffreestanding \
		-I$(INCLUDE_DIR) -c -o $@ $<

# Build user C programs with C-specific startup file and linker script
$(BIN_DIR)/%.bin: $(USER_C_DIR)/%.c $(STARTUP_C_SRC) $(INCLUDE_C_OBJS) $(LINKER_SCRIPT)
	@echo "Building user C program: $* (using startup/c/crt0.s and linker script)"
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

	# Compile the C file
	$(RISCV_CC) -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -nostdlib -ffreestanding \
		-mcmodel=medany -fno-common \
		-I$(INCLUDE_DIR) -c -o $(BUILD_DIR)/$*.c.o $<

	# Assemble the C-specific crt0 startup code
	$(RISCV_AS) -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) \
		-o $(BUILD_DIR)/crt0_c.o $(STARTUP_C_SRC)

	# Link everything together using linker script
	$(RISCV_LD) -m elf32lriscv -T $(LINKER_SCRIPT) \
		-o $(BUILD_DIR)/$*.elf \
		$(BUILD_DIR)/crt0_c.o \
		$(INCLUDE_C_OBJS) \
		$(BUILD_DIR)/$*.c.o

	# Convert to binary
	$(RISCV_OBJCOPY) -O binary $(BUILD_DIR)/$*.elf $@

# Clean
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Run all tests
.PHONY: run
run: $(TARGET) tests
	@echo "========================================"
	@echo "Running all programs..."
	@echo "========================================"
	@for bin in $(ALL_BINS); do \
		if [ -f "$$bin" ]; then \
			name=$$(basename $$bin .bin); \
			echo "Running $$name..."; \
			echo "----------------------------------------"; \
			$(TARGET) $$bin; \
			echo "----------------------------------------"; \
			echo ""; \
		fi; \
	done

# Interactive menu to select which program to run
.PHONY: run-menu
run-menu: $(TARGET) tests
	@echo "========================================"
	@echo "RISC-V Emulator - Program Selector"
	@echo "========================================"
	@echo ""
	@echo " 1. [BOOTLOADER] boot (0x0000)"
	@echo ""
	@echo " 2. [USER-ASM] arithmatic (0x2000)"
	@echo " 3. [USER-ASM] fibonacci (0x2000)"
	@echo " 4. [USER-ASM] hello (0x2000)"
	@echo " 5. [USER-ASM] loop (0x2000)"
	@echo " 6. [USER-ASM] watermelon_asm (0x2000)"
	@echo ""
	@echo " 7. [USER-C] asm_test (0x2000)"
	@echo " 8. [USER-C] bustest (0x2000)"
	@echo " 9. [USER-C] fibo (0x2000)"
	@echo "10. [USER-C] main (0x2000)"
	@echo "11. [USER-C] watermelon (0x2000)"
	@echo ""
	@echo " a. Run all programs"
	@echo " q. Quit"
	@echo ""
	@while true; do \
		read -p "Select program (1-11, a, or q): " choice; \
		case $$choice in \
			1) \
				echo ""; \
				echo "========================================"; \
				echo "Running boot (bootloader at 0x0000)"; \
				echo "========================================"; \
				$(TARGET) $(BIN_DIR)/bootloader_boot.bin; \
				echo "========================================"; \
				echo ""; \
				;; \
			2) \
				echo ""; \
				echo "========================================"; \
				echo "Running arithmatic (user assembly at 0x2000)"; \
				echo "========================================"; \
				$(TARGET) $(BIN_DIR)/arithmatic.bin; \
				echo "========================================"; \
				echo ""; \
				;; \
			3) \
				echo ""; \
				echo "========================================"; \
				echo "Running fibonacci (user assembly at 0x2000)"; \
				echo "========================================"; \
				$(TARGET) $(BIN_DIR)/fibonacci.bin; \
				echo "========================================"; \
				echo ""; \
				;; \
			4) \
				echo ""; \
				echo "========================================"; \
				echo "Running hello (user assembly at 0x2000)"; \
				echo "========================================"; \
				$(TARGET) $(BIN_DIR)/hello.bin; \
				echo "========================================"; \
				echo ""; \
				;; \
			5) \
				echo ""; \
				echo "========================================"; \
				echo "Running loop (user assembly at 0x2000)"; \
				echo "========================================"; \
				$(TARGET) $(BIN_DIR)/loop.bin; \
				echo "========================================"; \
				echo ""; \
				;; \
			6) \
				echo ""; \
				echo "========================================"; \
				echo "Running watermelon_asm (user assembly at 0x2000)"; \
				echo "========================================"; \
				$(TARGET) $(BIN_DIR)/watermelon_asm.bin; \
				echo "========================================"; \
				echo ""; \
				;; \
			7) \
				echo ""; \
				echo "========================================"; \
				echo "Running asm_test (user C at 0x2000)"; \
				echo "========================================"; \
				$(TARGET) $(BIN_DIR)/asm_test.bin; \
				echo "========================================"; \
				echo ""; \
				;; \
			8) \
				echo ""; \
				echo "========================================"; \
				echo "Running bustest (user C at 0x2000)"; \
				echo "========================================"; \
				$(TARGET) $(BIN_DIR)/bustest.bin; \
				echo "========================================"; \
				echo ""; \
				;; \
			9) \
				echo ""; \
				echo "========================================"; \
				echo "Running fibo (user C at 0x2000)"; \
				echo "========================================"; \
				$(TARGET) $(BIN_DIR)/fibo.bin; \
				echo "========================================"; \
				echo ""; \
				;; \
			10) \
				echo ""; \
				echo "========================================"; \
				echo "Running main (user C at 0x2000)"; \
				echo "========================================"; \
				$(TARGET) $(BIN_DIR)/main.bin; \
				echo "========================================"; \
				echo ""; \
				;; \
			11) \
				echo ""; \
				echo "========================================"; \
				echo "Running watermelon (user C at 0x2000)"; \
				echo "========================================"; \
				$(TARGET) $(BIN_DIR)/watermelon.bin; \
				echo "========================================"; \
				echo ""; \
				;; \
			a|A) \
				echo ""; \
				$(MAKE) run; \
				break \
				;; \
			q|Q) \
				echo "Goodbye!"; \
				break \
				;; \
			*) \
				echo "Invalid selection!"; \
				;; \
		esac; \
	done

# Run specific program by name
.PHONY: run-%
run-%: $(TARGET)
	@if [ -f "$(BIN_DIR)/$*.bin" ]; then \
		echo "========================================"; \
		echo "Running user program: $* (at 0x2000)"; \
		echo "========================================"; \
		$(TARGET) $(BIN_DIR)/$*.bin; \
		echo "========================================"; \
	elif [ -f "$(BIN_DIR)/bootloader_$*.bin" ]; then \
		echo "========================================"; \
		echo "Running bootloader: $* (at 0x0000)"; \
		echo "========================================"; \
		$(TARGET) $(BIN_DIR)/bootloader_$*.bin; \
		echo "========================================"; \
	else \
		echo "Program $*.bin not found!"; \
		echo "Available programs:"; \
		$(MAKE) list; \
	fi

# List available programs
.PHONY: list
list:
	@echo "Bootloader (runs at 0x0000):"
	@ls $(BOOTLOADER_DIR)/*.s 2>/dev/null | xargs -n1 basename | sed 's/\.s$$//' || echo "  None"
	@echo ""
	@echo "User assembly programs (runs at 0x2000):"
	@ls $(USER_ASM_DIR)/*.s 2>/dev/null | xargs -n1 basename | sed 's/\.s$$//' || echo "  None"
	@echo ""
	@echo "User C programs (runs at 0x2000):"
	@ls $(USER_C_DIR)/*.c 2>/dev/null | xargs -n1 basename | sed 's/\.c$$//' || echo "  None"

# Help
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all          - Build the emulator and all programs (default)"
	@echo "  tests        - Build all test programs"
	@echo "  run          - Build and run all programs"
	@echo "  run-menu     - Interactive menu to select program"
	@echo "  run-<name>   - Run specific program (e.g., make run-hello)"
	@echo "  list         - List available programs"
	@echo "  clean        - Remove build files"
	@echo "  help         - Show this help"
	@echo ""
	@echo "Memory map:"
	@echo "  Bootloader:  0x0000 - 0x1FFF (8KB)"
	@echo "  User space:  0x2000 - ..."
	@echo ""
	@echo "RISC-V extensions enabled: $(RISCV_ARCH)"
	@echo "Linker script: $(LINKER_SCRIPT)"
	@echo ""
	@echo "Startup files:"
	@echo "  C programs:       $(STARTUP_C_SRC)"
	@echo "  Assembly programs: $(STARTUP_ASM_SRC)"

# Quick test
.PHONY: test
test: $(TARGET)
	@if [ -f "$(BOOTLOADER_DIR)/boot.s" ]; then \
		$(MAKE) $(BIN_DIR)/bootloader_boot.bin; \
		echo "========================================"; \
		echo "Testing bootloader boot"; \
		echo "========================================"; \
		$(TARGET) $(BIN_DIR)/bootloader_boot.bin; \
	elif [ -f "$(USER_C_DIR)/main.c" ]; then \
		$(MAKE) $(BIN_DIR)/main.bin; \
		echo "========================================"; \
		echo "Testing main"; \
		echo "========================================"; \
		$(TARGET) $(BIN_DIR)/main.bin; \
	else \
		echo "No test program found"; \
	fi