#pragma once
#include <iostream>
#include <cstdint>
#include <vector>
#include "../environment/environment.hpp"

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
            return mstatus;
        case 0x305:
            return mtvec;
        case 0x341:
            return mepc;
        case 0x342:
            return mcause;
        case 0x343:
            return mtval;
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
            break;
        case 0x305:
            mtvec = val;
            break;
        case 0x341:
            mepc = val;
            break;
        case 0x342:
            mcause = val;
            break;
        case 0x343:
            mtval = val;
            break;
         }
    }
    void trap(uint32_t cause, bool is_interrupt = false);
    void stop() { running = false; }

private:
    void exec(uint32_t instr);

    uint32_t reg[32]{}; // x0-x31
    uint32_t pc = 0;
    // CSRS
    uint32_t mstatus = 0;
    uint32_t mtvec = 0;
    uint32_t mepc = 0;
    uint32_t mcause = 0;
    uint32_t mtval = 0;

    bool running = false;
    std::vector<uint8_t> mem; // RAM

    Environment *env = nullptr;
};
