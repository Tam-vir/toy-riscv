#include "cpu/riscv.hpp"
#include "environment/simple_env.hpp"
#include "peripherals/bus.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#endif

// Memory addresses
const uint32_t FIRMWARE_BASE = 0x0000;
const uint32_t USER_BASE = 0x2000;
const char *FIRMWARE_PATH = "bin/bootloader_boot.bin";

// Enable non-blocking input
void enable_raw_input()
{
#ifdef _WIN32
    // Windows console is already suitable for reading single chars
#else
    // Unix/Linux: configure terminal for raw input
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    // Set non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
#endif
}

// Restore terminal
void restore_terminal()
{
#ifndef _WIN32
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);
    raw.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
#endif
}

// Simulate a button press by toggling GPIO pin 0
void simulate_button_press(Bus &bus, RISCV &cpu)
{
    // std::cout << "\n[BUTTON] Simulating button press..." << std::endl;

    // Read current GPIO port 0 state
    uint32_t current_state = bus.load(0x1000);
    // std::cout << "[BUTTON] GPIO Port 0 current state: 0x" << std::hex << current_state << std::dec << std::endl;

    // Press button (set bit 0 to 1)
    // std::cout << "[BUTTON] Setting pin 0 HIGH" << std::endl;
    bus.store(0x1000, current_state | 1);

    // Run cycles to let the interrupt trigger and handler execute
    // std::cout << "[BUTTON] Running CPU to process interrupt (100 cycles)..." << std::endl;
    for (int i = 0; i < 100 && cpu.is_running(); i++)
    {
        cpu.step();
    }

    // Release button (set bit 0 to 0)
    //  std::cout << "[BUTTON] Setting pin 0 LOW" << std::endl;
    bus.store(0x1000, current_state & ~1);

    // Run a few more cycles
    for (int i = 0; i < 20 && cpu.is_running(); i++)
    {
        cpu.step();
    }

    // std::cout << "[BUTTON] Button press complete\n"
    //           << std::endl;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "RISC-V Emulator" << std::endl;
        std::cout << "Usage: " << argv[0] << " <user_program.bin>" << std::endl;
        std::cout << std::endl;
        std::cout << "Interactive input:" << std::endl;
        std::cout << "  p - Press button (GPIO pin 0)" << std::endl;
        std::cout << "  q - Quit emulator" << std::endl;
        std::cout << std::endl;
        std::cout << "Example:" << std::endl;
        std::cout << "  " << argv[0] << " bin/button_led_interrupt.bin" << std::endl;
        return 1;
    }

    const char *user_filename = argv[1];

    // Allocate 64MB of RAM
    RISCV cpu(64 * 1024 * 1024);
    SimpleEnvironment env;
    Bus bus;

    cpu.set_environment(&env);
    cpu.set_bus(&bus);

    // Load firmware binary (silently skip if not found)
    std::ifstream firmware_file(FIRMWARE_PATH, std::ios::binary | std::ios::ate);
    std::streamsize firmware_size = 0;
    std::vector<uint8_t> firmware;

    if (firmware_file)
    {
        firmware_size = firmware_file.tellg();
        firmware_file.seekg(0, std::ios::beg);
        firmware.resize(firmware_size);
        if (!firmware_file.read((char *)firmware.data(), firmware_size))
        {
            std::cerr << "Error: Failed to read firmware file" << std::endl;
            return 1;
        }
        cpu.load_program(firmware.data(), firmware.size(), FIRMWARE_BASE);
    }

    // Load user program binary
    std::ifstream user_file(user_filename, std::ios::binary | std::ios::ate);
    if (!user_file)
    {
        std::cerr << "Error: Cannot open user program file '" << user_filename << "'" << std::endl;
        return 1;
    }

    std::streamsize user_size = user_file.tellg();
    user_file.seekg(0, std::ios::beg);
    std::vector<uint8_t> user_program(user_size);
    if (!user_file.read((char *)user_program.data(), user_size))
    {
        std::cerr << "Error: Failed to read user program file" << std::endl;
        return 1;
    }

    std::cout << "Loading user program: " << user_filename << " (" << user_size << " bytes)" << std::endl;
    cpu.load_program(user_program.data(), user_program.size(), USER_BASE);

    std::cout << "Starting execution..." << std::endl;
    std::cout << "\n--- Program output ---\n"
              << std::endl;

    enable_raw_input();

    try
    {
        // Run CPU with interactive input support
        const int CYCLES_PER_CHECK = 1000;

        while (cpu.is_running())
        {
            // Run CPU for a bit
            for (int i = 0; i < CYCLES_PER_CHECK && cpu.is_running(); i++)
            {
                cpu.step();
            }

            // Check for user input
            char input;
#ifdef _WIN32
            if (_kbhit())
            {
                input = _getch();
#else
            if (read(STDIN_FILENO, &input, 1) > 0)
            {
#endif
                if (input == 'p' || input == 'P')
                {
                    // Inject button press signal
                    simulate_button_press(bus, cpu);
                }
                else if (input == 'q' || input == 'Q')
                {
                    // Stop execution
                    cpu.stop();
                    break;
                }
            }
        }

        std::cout << "\n--- End of program ---" << std::endl;
        std::cout << "Program exited successfully." << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << std::endl
                  << "Error during execution: " << e.what() << std::endl;
        restore_terminal();
        return 1;
    }

    restore_terminal();
    return 0;
}