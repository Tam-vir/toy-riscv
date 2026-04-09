// simple_env.hpp
#pragma once
#include "../environment/environment.hpp"
#include "../cpu/riscv.hpp"
#include <iostream>
#include <cstdint>
#include <iomanip>

class SimpleEnvironment : public Environment {
public:
    virtual void on_trap(RISCV &cpu, uint32_t cause)
    {
        if(cause == 11) // ECALL
            ecall(cpu);
        else if(cause == 3) // EBREAK
            ebreak(cpu);
    }
    void ecall(RISCV &cpu) override
    {
        uint32_t syscall = cpu.get_reg(17); // a7
        uint32_t a0 = cpu.get_reg(10);      // a0
        uint32_t a1 = cpu.get_reg(11);      // a1

        switch (syscall)
        {


        case 1: // Print Integer
            std::cout << (int32_t)a0;
            break;

        case 4:
        { // Print String (null-terminated)
            uint32_t addr = a0;
            while (true)
            {
                char c = cpu.load8(addr++);
                if (c == 0)
                    break;
                std::cout << c;
            }
            break;
        }

        case 11: // Print Character
            std::cout << (char)(a0 & 0xFF);
            break;


        case 5:
        { // Read Integer
            int32_t value;
            std::cin >> value;
            cpu.set_reg(10, (uint32_t)value); // return in a0
            break;
        }

        case 8:
        { // Read String
            uint32_t addr = a0;
            uint32_t max_len = a1;

            std::string input;
            std::getline(std::cin >> std::ws, input);

            uint32_t i = 0;
            for (; i < input.size() && i < max_len - 1; i++)
            {
                cpu.store8(addr + i, input[i]);
            }
            cpu.store8(addr + i, 0); // null terminator
            break;
        }

        case 12:
        { // Read Character
            char c;
            std::cin.get(c);
            cpu.set_reg(10, (uint32_t)c); // return in a0
            break;
        }


        case 10: // Exit
            cpu.stop();
            break;

        default:
            std::cerr << "Unknown syscall: " << syscall << "\n";
            cpu.stop();
        }
    }
    void ebreak(RISCV& cpu) override {
        std::cerr << "EBREAK encountered at PC = 0x\n"; 
    
        cpu.stop();
    }
};