#include "cpu/riscv.hpp"
#include "environment/simple_env.hpp"
#include "peripherals/bus.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>

// Memory addresses
const uint32_t FIRMWARE_BASE = 0x0000;
const uint32_t USER_BASE = 0x2000;

// Firmware binary path (you can make this configurable or hardcoded)
const char *FIRMWARE_PATH = "bin/bootloader_boot.bin";

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "RISC-V Emulator" << std::endl;
        std::cout << "Usage: " << argv[0] << " <user_program.bin>" << std::endl;
        std::cout << std::endl;
        std::cout << "Memory layout:" << std::endl;
        std::cout << "  Firmware:   0x0000 - 0x1FFF (loaded from " << FIRMWARE_PATH << ")" << std::endl;
        std::cout << "  User space: 0x2000 - ...    (loaded from user program)" << std::endl;
        std::cout << std::endl;
        std::cout << "Example:" << std::endl;
        std::cout << "  " << argv[0] << " bin/hello.bin" << std::endl;
        std::cout << "  " << argv[0] << " bin/main.bin" << std::endl;
        return 1;
    }

    const char *user_filename = argv[1];

    // Allocate 64MB of RAM
    RISCV cpu(64 * 1024 * 1024);
    SimpleEnvironment env;
    Bus bus; // Create the bus with GPIO and UART

    cpu.set_environment(&env);
    cpu.set_bus(&bus); // Connect bus to CPU

    // Load firmware binary
    std::ifstream firmware_file(FIRMWARE_PATH, std::ios::binary | std::ios::ate);
    std::streamsize firmware_size = 0;
    std::vector<uint8_t> firmware;

    if (!firmware_file)
    {
        std::cerr << "Error: Cannot open firmware file '" << FIRMWARE_PATH << "'" << std::endl;
        std::cerr << "Make sure you've built the firmware first (make all)" << std::endl;
        std::cerr << "Note: Running without firmware - CPU will start at user program directly" << std::endl;
        // Continue without firmware - CPU will start at user program
    }
    else
    {
        // Get firmware file size
        firmware_size = firmware_file.tellg();
        firmware_file.seekg(0, std::ios::beg);

        // Read firmware into memory
        firmware.resize(firmware_size);
        if (!firmware_file.read((char *)firmware.data(), firmware_size))
        {
            std::cerr << "Error: Failed to read firmware file" << std::endl;
            return 1;
        }
    }

    if (firmware_file.is_open())
    {
        std::cout << "Loading firmware: " << FIRMWARE_PATH << " (" << firmware_size << " bytes)" << std::endl;
        // std::cout << "  Load address: 0x" << std::hex << FIRMWARE_BASE << std::dec << std::endl;

        // Load firmware at address 0x0000
        cpu.load_program(firmware.data(), firmware.size(), FIRMWARE_BASE);
    }

    // Load user program binary
    std::ifstream user_file(user_filename, std::ios::binary | std::ios::ate);
    if (!user_file)
    {
        std::cerr << "Error: Cannot open user program file '" << user_filename << "'" << std::endl;
        return 1;
    }

    // Get user program file size
    std::streamsize user_size = user_file.tellg();
    user_file.seekg(0, std::ios::beg);

    // Read user program into memory
    std::vector<uint8_t> user_program(user_size);
    if (!user_file.read((char *)user_program.data(), user_size))
    {
        std::cerr << "Error: Failed to read user program file" << std::endl;
        return 1;
    }

    std::cout << "Loading user program: " << user_filename << " (" << user_size << " bytes)" << std::endl;
    // std::cout << "  Load address: 0x" << std::hex << USER_BASE << std::dec << std::endl;

    // Load user program at address 0x2000
    cpu.load_program(user_program.data(), user_program.size(), USER_BASE);

    // Set initial PC to firmware base (0x0000) if firmware exists, otherwise start at user program
    if (firmware_file.is_open())
    {
        // Run the program (starts executing firmware at 0x0000)
        std::cout << "Starting execution from firmware at 0x" << std::hex << FIRMWARE_BASE << std::dec << "..." << std::endl;
    }
    else
    {
        // No firmware - start directly at user program
        std::cout << "No firmware found - starting execution from user program at 0x" << std::hex << USER_BASE << std::dec << "..." << std::endl;
    }
    std::cout << "--- Program output ---\n"
              << std::endl;

    try
    {
        cpu.run();
        std::cout << "\n--- End of program ---" << std::endl;
        std::cout << "Program exited successfully." << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << std::endl
                  << "Error during execution: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}