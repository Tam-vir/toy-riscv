#pragma once
#include <cstdint>
#include "pinblock.hpp"
#include "../Signal/signal.hpp"

class GPIO
{
public:
    GPIO();
    
    void write(int port, int reg, uint32_t val)
    {
        if (port < 0 || port >= 4)
            return;

        switch (reg)
        {
        case 0x0: // DATA
            pin_blocks[port].write_out(val);
            break;

        case 0x4: // MODE
            pin_blocks[port].write_mode(val);
            break;

        case 0x20: // CONTROL (optional)
            ctrl = val;
            break;

        default:
            break;
        }
    }

    uint32_t read(int port, int reg)
    {
        if (port < 0 || port >= 4)
            return 0;

        switch (reg)
        {
        case 0x0:
            return pin_blocks[port].read();

        case 0x4:
            return pin_blocks[port].read_mode();

        case 0x20:
            return ctrl;

        default:
            return 0;
        }
    }

    Signal *get_pin(int port, int pin)
    {
        if (port < 0 || port >= 4 || pin < 0 || pin >= 8)
            return nullptr;

        return &pin_blocks[port].pins[pin];
    }

    void connect_pin(int port, int pin, Signal *signal)
    {
        if (port < 0 || port >= 4 || pin < 0 || pin >= 8)
            return;

        // Bind the signal to the pin block
        pin_blocks[port].pins[pin].bind(signal);
    }

    void disconnect_pin(int port, int pin)
    {
        if (port < 0 || port >= 4 || pin < 0 || pin >= 8)
            return;

        // Unbind the signal (set to nullptr)
        pin_blocks[port].pins[pin].bind(nullptr);
    }

    // =========================
    // INTERRUPT SUPPORT
    // =========================
    uint32_t get_interrupt_status() const { return interrupt_status; }
    uint32_t get_interrupt_enable() const { return interrupt_enable; }
    void set_interrupt_enable(uint32_t enable) { interrupt_enable = enable; }
    void clear_interrupt(uint32_t mask) { interrupt_status &= ~mask; }
    void check_pin_interrupts();
    void enable_pin_interrupt(int port, int pin, int mode);
    void disable_pin_interrupt(int port, int pin);
    void connect_interrupt_line(uint32_t vector, Signal *signal);
    Signal *get_interrupt_line(uint32_t vector);

private:
    uint32_t ctrl = 0x1;
    PinBlock pin_blocks[4];

    // =========================
    // INTERRUPT STATE
    // =========================
    uint32_t interrupt_status = 0;     // Active GPIO interrupts
    uint32_t interrupt_enable = 0;     // Enabled GPIO interrupts
    uint8_t pin_interrupt_modes[32];   // Interrupt mode per pin
    uint8_t last_pin_state[4];         // Last known pin state for edge detection
    Signal interrupt_lines[32];        // Individual pin interrupt lines
    Signal port_interrupt_lines[4];    // Port-wide interrupt lines
};