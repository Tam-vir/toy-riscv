#include "cpu/riscv.hpp"
#include "environment/simple_env.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "RISC-V Emulator" << std::endl;
        std::cout << "Usage: " << argv[0] << " <program.bin>" << std::endl;
        std::cout << std::endl;
        std::cout << "Example:" << std::endl;
        std::cout << "  " << argv[0] << " hello.bin" << std::endl;
        return 1;
    }
    
    const char* filename = argv[1];
    
    // Allocate 64MB of RAM
    RISCV cpu(64 * 1024 * 1024);
    SimpleEnvironment env;
    cpu.set_environment(&env);
    
    // Load binary file
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Error: Cannot open file '" << filename << "'" << std::endl;
        return 1;
    }
    
    // Get file size
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read file into memory
    std::vector<uint8_t> program(size);
    if (!file.read((char*)program.data(), size)) {
        std::cerr << "Error: Failed to read file '" << filename << "'" << std::endl;
        return 1;
    }
    
    std::cout << "Loading program: " << filename << " (" << size << " bytes)" << std::endl;
    
    // Load program at address 0
    cpu.load_program(program.data(), program.size(), 0);
    
    // Run the program
    std::cout << "Starting execution..." << std::endl;
    std::cout << "--- Program output ---\n" << std::endl;
    
    try {
        cpu.run();
        std::cout << "\n--- End of program ---" << std::endl;
        std::cout << "Program exited successfully." << std::endl;
    } catch (const std::exception &e) {
        std::cerr << std::endl << "Error during execution: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}