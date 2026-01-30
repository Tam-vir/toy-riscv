// simple_env.hpp
#pragma once
#include "../environment/environment.hpp"
#include "../cpu/riscv.hpp"
#include <iostream>
#include <cstdint>
#include <iomanip>

class SimpleEnvironment : public Environment {
public:
    void ecall(RISCV& cpu) override {
        uint32_t syscall = cpu.get_reg(17); // a7
        uint32_t a0 = cpu.get_reg(10);     // a0
        uint32_t a1 = cpu.get_reg(11);     // a1 (for some syscalls)

        switch(syscall) {
            case 0: // print character (new)
                std::cout << (char)(a0 & 0xFF);
                break;

            case 1: // print integer (decimal)
                std::cout << (int32_t)a0;
                break;

            case 2: // print integer (hexadecimal) - new
                std::cout << "0x" << std::hex << a0 << std::dec;
                break;

            case 3: // print integer (binary) - new
                std::cout << "0b";
                for (int i = 31; i >= 0; i--) {
                    std::cout << ((a0 >> i) & 1);
                    if (i > 0 && i % 4 == 0) std::cout << "'";
                }
                break;

            case 4: { // print string (null-terminated)
                uint32_t addr = a0;
                while (true) {
                    char c = cpu.load8(addr++);
                    if (c == 0) break;
                    std::cout << c;
                }
                break;
            }

            case 5: { // print string with length - new
                uint32_t addr = a0;
                uint32_t len = a1;
                for (uint32_t i = 0; i < len; i++) {
                    char c = cpu.load8(addr + i);
                    std::cout << c;
                }
                break;
            }

            case 6: // print newline - new
                std::cout << std::endl;
                break;

            case 7: // print space - new
                std::cout << ' ';
                break;

            case 8: { // print formatted number - new
                // a0 = value, a1 = format (0=dec, 1=hex, 2=bin, 3=char)
                switch (a1) {
                    case 0: std::cout << (int32_t)a0; break;
                    case 1: std::cout << "0x" << std::hex << a0 << std::dec; break;
                    case 2: 
                        std::cout << "0b";
                        for (int i = 31; i >= 0; i--) {
                            std::cout << ((a0 >> i) & 1);
                            if (i > 0 && i % 4 == 0) std::cout << "'";
                        }
                        break;
                    case 3: std::cout << (char)(a0 & 0xFF); break;
                    default: std::cout << (int32_t)a0; break;
                }
                break;
            }

            case 9: { // print memory dump - new
                uint32_t addr = a0;
                uint32_t len = a1;
                std::cout << std::hex;
                for (uint32_t i = 0; i < len; i++) {
                    if (i % 16 == 0) {
                        if (i > 0) std::cout << std::endl;
                        std::cout << "0x" << std::setw(8) << std::setfill('0') 
                                  << (addr + i) << ": ";
                    }
                    uint8_t val = cpu.load8(addr + i);
                    std::cout << std::setw(2) << std::setfill('0') 
                              << (int)val << " ";
                }
                std::cout << std::dec << std::endl;
                break;
            }

            case 10: // exit
                cpu.stop();
                break;

            case 11: // print register dump - new
                std::cout << "\nRegister dump:\n";
                for (int i = 0; i < 32; i++) {
                    if (i % 4 == 0) std::cout << std::endl;
                    std::cout << "x" << std::setw(2) << i << " = 0x" 
                              << std::hex << std::setw(8) << std::setfill('0') 
                              << cpu.get_reg(i) << "  ";
                }
                std::cout << std::dec << std::endl;
                break;

            case 12: // print PC - new
                //std::cout << "PC = 0x" << std::hex << cpu.get_pc() << std::dec;
                break;

            default:
                std::cerr << "Unknown syscall: " << syscall << "\n";
                cpu.stop();
        }
    }
    
    void ebreak(RISCV& cpu) override {
        std::cerr << "EBREAK encountered at PC = 0x\n"; 
                  //<< std::hex << cpu.get_pc() << std::dec << "\n";
        cpu.stop();
    }
};