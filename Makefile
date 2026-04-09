# RISC-V Emulator Makefile

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
PROGRAMS_DIR = programs
USER_C_DIR = $(PROGRAMS_DIR)/user/c
USER_ASM_DIR = $(PROGRAMS_DIR)/user/assembly
FIRMWARE_DIR = $(PROGRAMS_DIR)/firmware
INCLUDE_DIR = include

# Memory addresses
FIRMWARE_BASE = 0x0000
USER_PROGRAM_BASE = 0x2000

# RISC-V architecture flags (include zicsr extension for CSR instructions)
RISCV_ARCH = rv32im_zicsr
RISCV_ABI = ilp32

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

# Source files
SRCS = $(SRC_DIR)/main.cpp \
       $(SRC_DIR)/cpu/riscv.cpp

# Object files
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Target
TARGET = $(BIN_DIR)/rvemu

# Startup file
STARTUP_SRC = startup/crt0.s
STARTUP_OBJ = $(BUILD_DIR)/crt0.o

# Find all assembly test files (user assembly)
ASM_TESTS = $(wildcard $(USER_ASM_DIR)/*.s)
ASM_BINS = $(patsubst $(USER_ASM_DIR)/%.s,$(BIN_DIR)/%.bin,$(ASM_TESTS))

# Find all C program files (user C)
C_PROGRAMS = $(wildcard $(USER_C_DIR)/*.c)
C_BINS = $(patsubst $(USER_C_DIR)/%.c,$(BIN_DIR)/%.bin,$(C_PROGRAMS))

# Find firmware files
FIRMWARE_FILES = $(wildcard $(FIRMWARE_DIR)/*.s)
FIRMWARE_BINS = $(patsubst $(FIRMWARE_DIR)/%.s,$(BIN_DIR)/firmware_%.bin,$(FIRMWARE_FILES))

# All binaries to build
ALL_BINS = $(FIRMWARE_BINS) $(ASM_BINS) $(C_BINS)

# Default target
all: $(TARGET) $(ALL_BINS)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile C++ source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build all test programs
.PHONY: tests
tests: $(ALL_BINS)

# Build firmware programs (start at 0x0000) with zicsr extension
$(BIN_DIR)/firmware_%.bin: $(FIRMWARE_DIR)/%.s
	@echo "Building firmware: $* (base address: $(FIRMWARE_BASE))"
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)
	$(RISCV_AS) -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -o $(BUILD_DIR)/firmware_$*.o $<
	$(RISCV_LD) -m elf32lriscv -Ttext=$(FIRMWARE_BASE) -o $(BUILD_DIR)/firmware_$*.elf $(BUILD_DIR)/firmware_$*.o
	$(RISCV_OBJCOPY) -O binary $(BUILD_DIR)/firmware_$*.elf $@

# Build user assembly programs (start at 0x2000) with zicsr extension
$(BIN_DIR)/%.bin: $(USER_ASM_DIR)/%.s
	@echo "Building user assembly: $* (base address: $(USER_PROGRAM_BASE))"
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)
	$(RISCV_AS) -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -o $(BUILD_DIR)/$*.o $<
	$(RISCV_LD) -m elf32lriscv -Ttext=$(USER_PROGRAM_BASE) -o $(BUILD_DIR)/$*.elf $(BUILD_DIR)/$*.o
	$(RISCV_OBJCOPY) -O binary $(BUILD_DIR)/$*.elf $@

# Compile include/*.c runtime files
$(BUILD_DIR)/include_%.o: $(INCLUDE_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(RISCV_CC) -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -nostdlib -ffreestanding \
		-c -o $@ $<

# Build user C programs with startup (start at 0x2000)
$(BIN_DIR)/%.bin: $(USER_C_DIR)/%.c $(STARTUP_SRC) $(INCLUDE_C_OBJS)
	@echo "Building user C program: $* (base address: $(USER_PROGRAM_BASE))"
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

	$(RISCV_CC) -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -nostdlib -ffreestanding \
		-c -o $(BUILD_DIR)/$*.c.o $<

	$(RISCV_AS) -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) \
		-o $(BUILD_DIR)/crt0.o $(STARTUP_SRC)

	$(RISCV_LD) -m elf32lriscv -Ttext=$(USER_PROGRAM_BASE) \
		-o $(BUILD_DIR)/$*.elf \
		$(BUILD_DIR)/crt0.o \
		$(INCLUDE_C_OBJS) \
		$(BUILD_DIR)/$*.c.o

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
	
	@# Build arrays of names and corresponding bins
	@idx=1; \
	declare -a names; \
	declare -a bins; \
	\
	@# Add firmware (runs at 0x0000)
	for fw in $(FIRMWARE_FILES); do \
		name=$$(basename $$fw .s); \
		names[$$idx]="[FIRMWARE] $$name (0x0000)"; \
		bins[$$idx]="$(BIN_DIR)/firmware_$$name.bin"; \
		echo "  $$idx. [FIRMWARE] $$name - runs at 0x0000"; \
		idx=$$((idx + 1)); \
	done; \
	\
	@# Add user assembly tests (runs at 0x2000)
	for test in $(ASM_TESTS); do \
		name=$$(basename $$test .s); \
		names[$$idx]="[USER-ASM] $$name (0x2000)"; \
		bins[$$idx]="$(BIN_DIR)/$$name.bin"; \
		echo "  $$idx. [USER-ASM] $$name - runs at 0x2000"; \
		idx=$$((idx + 1)); \
	done; \
	\
	@# Add user C programs (runs at 0x2000)
	for prog in $(C_PROGRAMS); do \
		name=$$(basename $$prog .c); \
		names[$$idx]="[USER-C] $$name (0x2000)"; \
		bins[$$idx]="$(BIN_DIR)/$$name.bin"; \
		echo "  $$idx. [USER-C] $$name - runs at 0x2000"; \
		idx=$$((idx + 1)); \
	done; \
	\
	echo ""; \
	echo "  a. Run all programs"; \
	echo "  q. Quit"; \
	echo ""; \
	\
	while true; do \
		read -p "Select program (1-$$((idx-1)), a, or q): " choice; \
		case $$choice in \
			[1-9]*) \
				if [ $$choice -ge 1 ] && [ $$choice -lt $$idx ]; then \
					echo ""; \
					echo "========================================"; \
					echo "Running: $${names[$$choice]}"; \
					echo "========================================"; \
					$(TARGET) $${bins[$$choice]}; \
					echo "========================================"; \
					echo ""; \
				else \
					echo "Invalid selection!"; \
				fi \
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
	elif [ -f "$(BIN_DIR)/firmware_$*.bin" ]; then \
		echo "========================================"; \
		echo "Running firmware: $* (at 0x0000)"; \
		echo "========================================"; \
		$(TARGET) $(BIN_DIR)/firmware_$*.bin; \
		echo "========================================"; \
	else \
		echo "Program $*.bin not found!"; \
		echo "Available programs:"; \
		$(MAKE) list; \
	fi

# List available programs
.PHONY: list
list:
	@echo "Firmware (runs at 0x0000):"
	@ls $(FIRMWARE_DIR)/*.s 2>/dev/null | xargs -n1 basename | sed 's/\.s$$//' || echo "  None"
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
	@echo "  Firmware:   0x0000 - 0x1FFF (8KB)"
	@echo "  User space: 0x2000 - ..."
	@echo ""
	@echo "RISC-V extensions enabled: $(RISCV_ARCH)"

# Quick test
.PHONY: test
test: $(TARGET)
	@if [ -f "$(FIRMWARE_DIR)/boot.s" ]; then \
		$(MAKE) $(BIN_DIR)/firmware_boot.bin; \
		echo "========================================"; \
		echo "Testing firmware boot"; \
		echo "========================================"; \
		$(TARGET) $(BIN_DIR)/firmware_boot.bin; \
	elif [ -f "$(USER_C_DIR)/main.c" ]; then \
		$(MAKE) $(BIN_DIR)/main.bin; \
		echo "========================================"; \
		echo "Testing main"; \
		echo "========================================"; \
		$(TARGET) $(BIN_DIR)/main.bin; \
	else \
		echo "No test program found"; \
	fi