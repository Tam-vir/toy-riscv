#pragma once
#include <iostream>
#include <cstdint>
#include <vector>
#include "../environment/environment.hpp"
#include "../peripherals/bus.hpp"

class RISCV
{
public:
    static constexpr uint32_t RAM_SIZE = 16 * 1024 * 1024; // 16 MB

    RISCV();
    explicit RISCV(uint32_t ram_size);
    ~RISCV() = default;

    void reset();
    void run();
    void step();

    // Fetch instruction
    uint32_t fetch32(uint32_t addr);

    // Memory access
    uint8_t load8(uint32_t addr);
    uint16_t load16(uint32_t addr);
    uint32_t load32(uint32_t addr);

    void store8(uint32_t addr, uint8_t value);
    void store16(uint32_t addr, uint16_t value);
    void store32(uint32_t addr, uint32_t value);

    void load_program(const uint8_t *data, uint32_t size, uint32_t start_addr = 0);

    // Environment for ECALLs
    void set_environment(Environment *env_ptr) { env = env_ptr; }
    void set_bus(Bus *bus_ptr) { bus = bus_ptr; }

    // Register access (for environment or testing)
    uint32_t get_reg(uint32_t index) const { return reg[index]; }
    void set_reg(uint32_t index, uint32_t value)
    {
        if (index != 0)
            reg[index] = value;
    }

    // CSRS access
    uint32_t read_csr(uint32_t addr)
    {
        switch (addr)
        {
        case 0x300:
            return mstatus; // Machine Status
        case 0x301:
            return misa; // Machine ISA
        case 0x304:
            return mie; // Machine Interrupt Enable
        case 0x305:
            return mtvec; // Machine Trap Vector
        case 0x340:
            return mscratch; // Machine Scratch
        case 0x341:
            return mepc; // Machine Exception PC
        case 0x342:
            return mcause; // Machine Exception Cause
        case 0x343:
            return mtval; // Machine Trap Value
        case 0x344:
            return mip; // Machine Interrupt Pending
        default:
            return 0; // or trap later
        }
    }

    void write_csr(uint32_t addr, uint32_t val)
    {
        switch (addr)
        {
        case 0x300:
            mstatus = val;
            break; // Machine Status
        case 0x301:
            misa = val;
            break; // Machine ISA
        case 0x304:
            mie = val;
            break; // Machine Interrupt Enable
        case 0x305:
            mtvec = val;
            break; // Machine Trap Vector
        case 0x340:
            mscratch = val;
            break; // Machine Scratch
        case 0x341:
            mepc = val;
            break; // Machine Exception PC
        case 0x342:
            mcause = val;
            break; // Machine Exception Cause
        case 0x343:
            mtval = val;
            break; // Machine Trap Value
        case 0x344:
            mip = val;
            break; // Machine Interrupt Pending
        }
    }
    void trap(uint32_t cause, bool is_interrupt = false);
    uint64_t get_cycles() const;
    bool is_running() const { return running; }
    void stop() { running = false; }

    // Interrupt handling
    void check_interrupts();
    void handle_interrupt(uint32_t interrupt_vector);
    bool is_interrupt_enabled(uint32_t vector);
    void trigger_external_interrupt(uint32_t vector);
    void clear_external_interrupt(uint32_t vector);

private:
    void exec(uint32_t instr);

    uint32_t reg[32]{}; // x0-x31
    uint32_t pc = 0;
    // CSRS
    uint32_t mstatus = 0;
    uint32_t misa = 0x40141101; // RV32I + M extension
    uint32_t mie = 0;           // Machine Interrupt Enable
    uint32_t mtvec = 0;
    uint32_t mscratch = 0; // Machine Scratch
    uint32_t mip = 0;      // Machine Interrupt Pending
    uint32_t mepc = 0;
    uint32_t mcause = 0;
    uint32_t mtval = 0;
    uint64_t cycles = 0; // Cycle count for performance measurement
    bool running = false;
    std::vector<uint8_t> mem; // RAM

    Environment *env = nullptr;
    Bus *bus = nullptr;
};