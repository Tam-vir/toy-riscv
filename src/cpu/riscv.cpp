#include "riscv.hpp"
#include <cstring>
#include <stdexcept>
#include <bitset>
#include <climits>
#include <cstdint>

static inline int32_t sign_extend(uint32_t value, int bits)
{
    int32_t x = (int32_t)(value & ((1u << bits) - 1));
    if (x & (1 << (bits - 1)))
    {
        x |= ~((1 << bits) - 1);
    }
    return x;
}

static inline uint32_t zext(uint32_t value, int bits)
{
    return value & ((1u << bits) - 1);
}

RISCV::RISCV() : mem(RAM_SIZE), cycles(0) { reset(); }
RISCV::RISCV(uint32_t ram_size) : mem(ram_size), cycles(0) { reset(); }

void RISCV::reset()
{
    pc = 0x00000000;
    running = true;
    cycles = 0;
    std::memset(reg, 0, sizeof(reg));

    // CSR defaults (important!)
    write_csr(0x300, 0x00000000); // mstatus
}

void RISCV::run()
{
    while (running)
        step();
}

void RISCV::step()
{
    uint32_t instr = fetch32(pc);
    pc += 4;
    cycles++; // Base cycle for instruction fetch and decode
    exec(instr);
    
    reg[0] = 0; // x0 always zero
}

uint32_t RISCV::fetch32(uint32_t addr)
{
    if (addr + 3 >= mem.size())
        throw std::runtime_error("Fetch out of bounds");

    // Memory access cycles
    cycles++; // For memory read

    return mem[addr] | (mem[addr + 1] << 8) | (mem[addr + 2] << 16) | (mem[addr + 3] << 24);
}

uint8_t RISCV::load8(uint32_t addr)
{
    cycles++; // Memory access cycle
    return mem.at(addr);
}

uint16_t RISCV::load16(uint32_t addr)
{
    cycles++; // Memory access cycle
    return mem.at(addr) | (mem.at(addr + 1) << 8);
}

uint32_t RISCV::load32(uint32_t addr)
{
    cycles++; // Memory access cycle
    return mem.at(addr) | (mem.at(addr + 1) << 8) | (mem.at(addr + 2) << 16) | (mem.at(addr + 3) << 24);
}

void RISCV::store8(uint32_t addr, uint8_t value)
{
    cycles++; // Memory access cycle
    mem.at(addr) = value;
}

void RISCV::store16(uint32_t addr, uint16_t value)
{
    cycles++; // Memory access cycle
    mem.at(addr) = value & 0xFF;
    mem.at(addr + 1) = (value >> 8) & 0xFF;
}

void RISCV::store32(uint32_t addr, uint32_t value)
{
    cycles++; // Memory access cycle
    mem.at(addr) = value & 0xFF;
    mem.at(addr + 1) = (value >> 8) & 0xFF;
    mem.at(addr + 2) = (value >> 16) & 0xFF;
    mem.at(addr + 3) = (value >> 24) & 0xFF;
}

void RISCV::load_program(const uint8_t *data, uint32_t size, uint32_t start_addr)
{
    if (start_addr + size > mem.size())
        throw std::runtime_error("Program too large");
    std::memcpy(&mem[start_addr], data, size);
    pc = start_addr;
}

void RISCV::trap(uint32_t cause, bool is_interrupt)
{
    cycles += 4; // Trap handling takes extra cycles

    uint32_t mstatus = read_csr(0x300);

    // mepc = return address
    write_csr(0x341, pc);

    // mcause = interrupt bit + cause
    if (is_interrupt)
        write_csr(0x342, cause | 0x80000000);
    else
        write_csr(0x342, cause);

    // save MIE into MPIE
    uint32_t mie = (mstatus >> 3) & 1;
    mstatus = (mstatus & ~(1 << 7)) | (mie << 7);

    // disable interrupts
    mstatus &= ~(1 << 3);

    write_csr(0x300, mstatus);

    uint32_t mtvec = read_csr(0x305);
    uint32_t base = mtvec & ~0x3;
    uint32_t mode = mtvec & 0x3;

    if (mode == 0)
    {
        // DIRECT MODE
        pc = base;
    }
    else
    {
        // VECTORED MODE
        if (is_interrupt)
            pc = base + 4 * cause;
        else
            pc = base;
    }
}

void RISCV::exec(uint32_t instr)
{
    uint32_t opcode = instr & 0x7F;
    uint32_t rd = (instr >> 7) & 0x1F;
    uint32_t funct3 = (instr >> 12) & 0x7;
    uint32_t rs1 = (instr >> 15) & 0x1F;
    uint32_t rs2 = (instr >> 20) & 0x1F;
    uint32_t funct7 = (instr >> 25);

    // Base cycle for instruction execution (already counted in step())
    // Additional cycles for complex instructions
    uint32_t extra_cycles = 0;

    switch (opcode)
    {
    // LUI
    case 0x37:
        reg[rd] = instr & 0xFFFFF000;
        break;

    // AUIPC
    case 0x17:
        reg[rd] = pc - 4 + (instr & 0xFFFFF000);
        break;

    // JAL
    case 0x6F:
    { // JAL
        uint32_t imm = ((instr >> 21) & 0x3FF) << 1 | ((instr >> 20) & 0x1) << 11 |
                       ((instr >> 12) & 0xFF) << 12 | ((instr >> 31) & 0x1) << 20;
        int32_t simm = sign_extend(imm, 21);

        reg[rd] = pc;
        pc += simm - 4;
        extra_cycles = 1; // Branch penalty
        break;
    }

    case 0x67:
    { // JALR
        int32_t imm = sign_extend(instr >> 20, 12);
        uint32_t target = (reg[rs1] + imm) & ~1u;

        reg[rd] = pc;
        pc = target;
        extra_cycles = 1; // Jump penalty
        break;
    }

    // BRANCH
    case 0x63:
    {
        uint32_t imm = ((instr >> 8) & 0xF) << 1 | ((instr >> 25) & 0x3F) << 5 |
                       ((instr >> 7) & 0x1) << 11 | ((instr >> 31) & 0x1) << 12;
        int32_t simm = sign_extend(imm, 13);
        bool branch = false;

        switch (funct3)
        {
        case 0x0: // BEQ
            branch = (reg[rs1] == reg[rs2]);
            break;
        case 0x1: // BNE
            branch = (reg[rs1] != reg[rs2]);
            break;
        case 0x4: // BLT
            branch = ((int32_t)reg[rs1] < (int32_t)reg[rs2]);
            break;
        case 0x5: // BGE
            branch = ((int32_t)reg[rs1] >= (int32_t)reg[rs2]);
            break;
        case 0x6: // BLTU
            branch = (reg[rs1] < reg[rs2]);
            break;
        case 0x7: // BGEU
            branch = (reg[rs1] >= reg[rs2]);
            break;
        default:
            throw std::runtime_error("Unknown BRANCH funct3");
        }

        if (branch)
        {
            pc += simm - 4;
            extra_cycles = 1; // Branch taken penalty
        }
        // else: branch not taken - no penalty
        break;
    }

    // OP-IMM
    case 0x13:
    {
        int32_t imm = sign_extend(instr >> 20, 12);
        uint32_t uimm = imm & 0xFFF;

        switch (funct3)
        {
        case 0x0: // ADDI
            reg[rd] = reg[rs1] + imm;
            break;
        case 0x1: // SLLI
            reg[rd] = reg[rs1] << (uimm & 0x1F);
            break;
        case 0x2: // SLTI
            reg[rd] = ((int32_t)reg[rs1] < imm) ? 1 : 0;
            break;
        case 0x3: // SLTIU
            reg[rd] = (reg[rs1] < (uint32_t)imm) ? 1 : 0;
            break;
        case 0x4: // XORI
            reg[rd] = reg[rs1] ^ imm;
            break;
        case 0x5: // SRLI/SRAI
            if ((instr >> 25) == 0x00)
            { // SRLI
                reg[rd] = reg[rs1] >> (uimm & 0x1F);
            }
            else
            { // SRAI
                reg[rd] = (int32_t)reg[rs1] >> (uimm & 0x1F);
            }
            break;
        case 0x6: // ORI
            reg[rd] = reg[rs1] | imm;
            break;
        case 0x7: // ANDI
            reg[rd] = reg[rs1] & imm;
            break;
        default:
            throw std::runtime_error("Unknown OP-IMM funct3");
        }
        break;
    }

    // OP (including M extension - multiplication and division)
    case 0x33:
    {
        // Check if this is an M-extension instruction (funct7[0] = 1)
        if ((funct7 & 0x1) == 0x1)
        {
            // M-extension instructions take extra cycles
            extra_cycles = 2; // Base penalty for multiply/divide

            // M-extension instructions (RV32M)
            switch (funct3)
            {
            case 0x0: // MUL
            {
                int32_t a = (int32_t)reg[rs1];
                int32_t b = (int32_t)reg[rs2];
                int64_t result = (int64_t)a * (int64_t)b;
                reg[rd] = (uint32_t)result;
                break;
            }

            case 0x1: // MULH (signed × signed)
            {
                int32_t a = (int32_t)reg[rs1];
                int32_t b = (int32_t)reg[rs2];
                int64_t result = (int64_t)a * (int64_t)b;
                reg[rd] = (uint32_t)(result >> 32);
                break;
            }

            case 0x2: // MULHSU (signed × unsigned)
            {
                int32_t a = (int32_t)reg[rs1];
                uint32_t b = reg[rs2];
                int64_t result = (int64_t)a * (int64_t)b;
                reg[rd] = (uint32_t)(result >> 32);
                break;
            }

            case 0x3: // MULHU (unsigned × unsigned)
            {
                uint64_t a = (uint64_t)reg[rs1];
                uint64_t b = (uint64_t)reg[rs2];
                uint64_t result = a * b;
                reg[rd] = (uint32_t)(result >> 32);
                break;
            }

            case 0x4: // DIV (signed division)
            {
                int32_t dividend = (int32_t)reg[rs1];
                int32_t divisor = (int32_t)reg[rs2];
                extra_cycles = 4; // Division takes more cycles

                // Handle division by zero according to RISC-V spec
                if (divisor == 0)
                {
                    // Division by zero yields all ones
                    reg[rd] = (uint32_t)-1;
                }
                // Handle overflow of division of INT32_MIN by -1
                else if (dividend == INT32_MIN && divisor == -1)
                {
                    // Overflow yields the dividend
                    reg[rd] = (uint32_t)dividend;
                }
                else
                {
                    reg[rd] = (uint32_t)(dividend / divisor);
                }
                break;
            }

            case 0x5: // DIVU (unsigned division)
            {
                uint32_t dividend = reg[rs1];
                uint32_t divisor = reg[rs2];
                extra_cycles = 4; // Division takes more cycles

                if (divisor == 0)
                {
                    // Division by zero yields all ones
                    reg[rd] = UINT32_MAX;
                }
                else
                {
                    reg[rd] = dividend / divisor;
                }
                break;
            }

            case 0x6: // REM (signed remainder)
            {
                int32_t dividend = (int32_t)reg[rs1];
                int32_t divisor = (int32_t)reg[rs2];
                extra_cycles = 4; // Division takes more cycles

                // Handle division by zero according to RISC-V spec
                if (divisor == 0)
                {
                    // Remainder by zero yields the dividend
                    reg[rd] = (uint32_t)dividend;
                }
                // Handle overflow of division of INT32_MIN by -1
                else if (dividend == INT32_MIN && divisor == -1)
                {
                    // Overflow yields 0
                    reg[rd] = 0;
                }
                else
                {
                    // Standard remainder operation
                    reg[rd] = (uint32_t)(dividend % divisor);
                }
                break;
            }

            case 0x7: // REMU (unsigned remainder)
            {
                uint32_t dividend = reg[rs1];
                uint32_t divisor = reg[rs2];
                extra_cycles = 4; // Division takes more cycles

                if (divisor == 0)
                {
                    // Remainder by zero yields the dividend
                    reg[rd] = dividend;
                }
                else
                {
                    reg[rd] = dividend % divisor;
                }
                break;
            }

            default:
                throw std::runtime_error("Unknown RV32M funct3");
            }
        }
        else
        {
            // Original integer instructions (non-M-extension)
            switch (funct3)
            {
            case 0x0: // ADD/SUB
                if (funct7 == 0x00)
                {
                    reg[rd] = reg[rs1] + reg[rs2];
                }
                else if (funct7 == 0x20)
                {
                    reg[rd] = reg[rs1] - reg[rs2];
                }
                else
                {
                    throw std::runtime_error("Unknown OP funct7 for ADD/SUB");
                }
                break;
            case 0x1: // SLL
                reg[rd] = reg[rs1] << (reg[rs2] & 0x1F);
                break;
            case 0x2: // SLT
                reg[rd] = ((int32_t)reg[rs1] < (int32_t)reg[rs2]) ? 1 : 0;
                break;
            case 0x3: // SLTU
                reg[rd] = (reg[rs1] < reg[rs2]) ? 1 : 0;
                break;
            case 0x4: // XOR
                reg[rd] = reg[rs1] ^ reg[rs2];
                break;
            case 0x5: // SRL/SRA
                if (funct7 == 0x00)
                { // SRL
                    reg[rd] = reg[rs1] >> (reg[rs2] & 0x1F);
                }
                else if (funct7 == 0x20)
                { // SRA
                    reg[rd] = (int32_t)reg[rs1] >> (reg[rs2] & 0x1F);
                }
                else
                {
                    throw std::runtime_error("Unknown OP funct7 for SRL/SRA");
                }
                break;
            case 0x6: // OR
                reg[rd] = reg[rs1] | reg[rs2];
                break;
            case 0x7: // AND
                reg[rd] = reg[rs1] & reg[rs2];
                break;
            default:
                throw std::runtime_error("Unknown OP funct3");
            }
        }
        break;
    }

    // LOAD
    case 0x03:
    {
        int32_t imm = sign_extend(instr >> 20, 12);
        uint32_t addr = reg[rs1] + imm;
        extra_cycles = 2; // Load instructions take extra cycles (memory access)

        switch (funct3)
        {
        case 0x0: // LB
            reg[rd] = sign_extend(load8(addr), 8);
            break;
        case 0x1: // LH
            reg[rd] = sign_extend(load16(addr), 16);
            break;
        case 0x2: // LW
            reg[rd] = load32(addr);
            break;
        case 0x4: // LBU
            reg[rd] = load8(addr);
            break;
        case 0x5: // LHU
            reg[rd] = load16(addr);
            break;
        default:
            throw std::runtime_error("Unknown LOAD funct3");
        }
        break;
    }

    // STORE
    case 0x23:
    {
        uint32_t imm = ((instr >> 7) & 0x1F) | ((instr >> 25) << 5);
        int32_t simm = sign_extend(imm, 12);
        uint32_t addr = reg[rs1] + simm;
        extra_cycles = 2; // Store instructions take extra cycles (memory access)

        switch (funct3)
        {
        case 0x0: // SB
            store8(addr, reg[rs2] & 0xFF);
            break;
        case 0x1: // SH
            store16(addr, reg[rs2] & 0xFFFF);
            break;
        case 0x2: // SW
            store32(addr, reg[rs2]);
            break;
        default:
            throw std::runtime_error("Unknown STORE funct3");
        }
        break;
    }

    // MISC-MEM (FENCE)
    case 0x0F:
        // FENCE instruction - do nothing in emulator
        extra_cycles = 1; // FENCE has small penalty
        break;

    // SYSTEM
    case 0x73:
    {
        uint32_t imm12 = instr >> 20;
        extra_cycles = 3; // System instructions take more cycles

        if (funct3 == 0x0)
        {
            if (imm12 == 0x0)
            {
                if (env)
                    env->on_trap(*this, 11);
            }
            else if (imm12 == 0x1)
            {
                if (env)
                    env->on_trap(*this, 3);
            }
            else if (imm12 == 0x302) // MRET
            {
                uint32_t m = read_csr(0x300);
                uint32_t mpie = (m >> 7) & 1;

                m = (m & ~(1 << 3)) | (mpie << 3);
                m |= (1 << 7);

                write_csr(0x300, m);

                pc = read_csr(0x341) & ~1u;
                extra_cycles = 5; // MRET takes longer
            }
        }
        else
        {
            uint32_t csr_addr = (instr >> 20) & 0xFFF;
            uint32_t csr_val = read_csr(csr_addr);

            switch (funct3)
            {
            case 1:
                reg[rd] = csr_val;
                write_csr(csr_addr, reg[rs1]);
                break;

            case 2:
                reg[rd] = csr_val;
                if (rs1)
                    write_csr(csr_addr, csr_val | reg[rs1]);
                break;

            case 3:
                reg[rd] = csr_val;
                if (rs1)
                    write_csr(csr_addr, csr_val & ~reg[rs1]);
                break;

            default:
                trap(2);
                break;
            }
        }
        break;
    }

    default:
        throw std::runtime_error(std::string("Unknown opcode: 0x") +
                                 std::to_string(opcode) + " at PC=0x" +
                                 std::to_string(pc - 4));
    }

    // Add extra cycles for this instruction
    cycles += extra_cycles;
}

uint64_t RISCV::get_cycles() const
{
    return cycles;
}