# RISC-V Emulator Makefile

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
PROGRAMS_DIR = programs/c
ASM_DIR = programs/assembly
INCLUDE_DIR = include
# Runtime C files inside include (not recommended structure, but works)
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

# Find all assembly test files
ASM_TESTS = $(wildcard $(ASM_DIR)/*.s)
ASM_BINS = $(patsubst $(ASM_DIR)/%.s,$(BIN_DIR)/%.bin,$(ASM_TESTS))

# Find all C test files



# Find all program files
PROGRAMS = $(wildcard $(PROGRAMS_DIR)/*.c)
PROGRAM_BINS = $(patsubst $(PROGRAMS_DIR)/%.c,$(BIN_DIR)/%.bin,$(PROGRAMS))

# All binaries to build
ALL_BINS = $(ASM_BINS) $(C_BINS) $(PROGRAM_BINS)

# Get list of program names
ASM_NAMES = $(notdir $(basename $(ASM_TESTS)))
C_NAMES = $(notdir $(basename $(C_TESTS)))
PROGRAM_NAMES = $(notdir $(basename $(PROGRAMS)))
ALL_NAMES = $(ASM_NAMES) $(C_NAMES) $(PROGRAM_NAMES)

# Default target
all: $(TARGET) $(STARTUP_SRC)

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

# Build assembly test
$(BIN_DIR)/%.bin: $(ASM_DIR)/%.s
	@echo "Building assembly: $*"
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)
	$(RISCV_AS) -march=rv32im -mabi=ilp32 -o $(BUILD_DIR)/$*.o $<
	$(RISCV_LD) -m elf32lriscv -Ttext=0x0 -o $(BUILD_DIR)/$*.elf $(BUILD_DIR)/$*.o
	$(RISCV_OBJCOPY) -O binary $(BUILD_DIR)/$*.elf $@
# Compile include/*.c runtime files
$(BUILD_DIR)/include_%.o: $(INCLUDE_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(RISCV_CC) -march=rv32im -mabi=ilp32 -nostdlib -ffreestanding \
		-c -o $@ $<


$(BIN_DIR)/%.bin: $(PROGRAMS_DIR)/%.c $(STARTUP_SRC) $(INCLUDE_C_OBJS)
	@echo "Building C program with startup: $*"
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

	$(RISCV_CC) -march=rv32im -mabi=ilp32 -nostdlib -ffreestanding \
		-c -o $(BUILD_DIR)/$*.c.o $<

	$(RISCV_AS) -march=rv32im -mabi=ilp32 \
		-o $(BUILD_DIR)/crt0.o $(STARTUP_SRC)

	$(RISCV_LD) -m elf32lriscv -Ttext=0x0 \
		-o $(BUILD_DIR)/$*.elf \
		$(BUILD_DIR)/crt0.o \
		$(INCLUDE_C_OBJS) \
		$(BUILD_DIR)/$*.c.o

	$(RISCV_OBJCOPY) -O binary $(BUILD_DIR)/$*.elf $@


# Alternative: Direct compilation without intermediate objects
$(BIN_DIR)/%_direct.bin: $(PROGRAMS_DIR)/%.c $(STARTUP_SRC)
	@echo "Building C program (direct): $*"
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)
	$(RISCV_CC) -march=rv32im -mabi=ilp32 -nostdlib -ffreestanding \
		-Wl,-Ttext=0x0 -o $(BUILD_DIR)/$*.elf $(STARTUP_SRC) $<
	$(RISCV_OBJCOPY) -O binary $(BUILD_DIR)/$*.elf $@

# Run all tests
.PHONY: run
run: $(TARGET) tests
	@echo "========================================"
	@echo "Running tests..."
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
	@# Add assembly tests
	for test in $(ASM_TESTS); do \
		name=$$(basename $$test .s); \
		names[$$idx]="[ASM] $$name"; \
		bins[$$idx]="$(BIN_DIR)/$$name.bin"; \
		echo "  $$idx. [ASM] $$name"; \
		idx=$$((idx + 1)); \
	done; \
	\
	@# Add C tests
	for test in $(C_TESTS); do \
		name=$$(basename $$test .c); \
		names[$$idx]="[C]   $$name"; \
		bins[$$idx]="$(BIN_DIR)/$$name.bin"; \
		echo "  $$idx. [C]   $$name"; \
		idx=$$((idx + 1)); \
	done; \
	\
	@# Add programs
	for prog in $(PROGRAMS); do \
		name=$$(basename $$prog .c); \
		names[$$idx]="[PROG] $$name"; \
		bins[$$idx]="$(BIN_DIR)/$$name.bin"; \
		echo "  $$idx. [PROG] $$name"; \
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
		echo "Running: $*"; \
		echo "========================================"; \
		$(TARGET) $(BIN_DIR)/$*.bin; \
		echo "========================================"; \
	elif [ -f "$(BIN_DIR)/$*_direct.bin" ]; then \
		echo "========================================"; \
		echo "Running: $*_direct"; \
		echo "========================================"; \
		$(TARGET) $(BIN_DIR)/$*_direct.bin; \
		echo "========================================"; \
	else \
		echo "Program $*.bin not found!"; \
		echo "Available programs:"; \
		$(MAKE) list; \
	fi

# Clean
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# List available tests
.PHONY: list
list:
	@echo "Available assembly tests:"
	@ls $(ASM_DIR)/*.s 2>/dev/null | xargs -n1 basename || echo "None"
	@echo ""
	@echo "Available C tests:"
	
	@echo ""
	@echo "Available programs:"
	@ls $(PROGRAMS_DIR)/*.c 2>/dev/null | xargs -n1 basename || echo "None"

# Help
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all          - Build the emulator (default)"
	@echo "  tests        - Build all test programs"
	@echo "  run          - Build and run all tests"
	@echo "  run-menu     - Interactive menu to select program"
	@echo "  run-<name>   - Run specific program (e.g., make run-hello)"
	@echo "  list         - List available tests"
	@echo "  clean        - Remove build files"
	@echo "  help         - Show this help"

# Quick test
.PHONY: test
test: $(TARGET)
	@if [ -f "$(PROGRAMS_DIR)/minimal_c_safe.c" ]; then \
		$(MAKE) $(BIN_DIR)/minimal_c_safe.bin; \
		echo "========================================"; \
		echo "Testing minimal_c_safe"; \
		echo "========================================"; \
		$(TARGET) $(BIN_DIR)/minimal_c_safe.bin; \
	elif [ -f "$(PROGRAMS_DIR)/hello.c" ]; then \
		$(MAKE) $(BIN_DIR)/hello.bin; \
		echo "========================================"; \
		echo "Testing hello"; \
		echo "========================================"; \
		$(TARGET) $(BIN_DIR)/hello.bin; \
	else \
		echo "No test program found"; \
	fi