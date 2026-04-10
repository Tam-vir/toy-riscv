#include "gpio.hpp"
#include <cstring>

GPIO::GPIO()
{
    // Initialize interrupt state
    interrupt_status = 0;
    interrupt_enable = 0;
    std::memset(pin_interrupt_modes, 0, sizeof(pin_interrupt_modes));
    std::memset(last_pin_state, 0, sizeof(last_pin_state));
}

void GPIO::check_pin_interrupts()
{
    uint32_t new_interrupts = 0;

    // Check each port for pin changes
    for (int port = 0; port < 4; port++)
    {
        uint8_t current_state = pin_blocks[port].read() & 0xFF;
        uint8_t state_changes = current_state ^ last_pin_state[port];

        if (state_changes == 0)
            continue; // No changes in this port

        // Check each pin in the port
        for (int pin = 0; pin < 8; pin++)
        {
            int pin_index = port * 8 + pin;
            uint8_t pin_mask = 1 << pin;

            if (!(state_changes & pin_mask))
                continue; // No change on this pin

            bool current_pin_state = (current_state & pin_mask) != 0;
            bool last_pin_state_bit = (last_pin_state[port] & pin_mask) != 0;

            // Check interrupt mode and trigger if appropriate
            switch (pin_interrupt_modes[pin_index])
            {
            case 1: // LOW level
                if (!current_pin_state)
                    new_interrupts |= (1 << pin_index);
                break;
            case 2: // HIGH level
                if (current_pin_state)
                    new_interrupts |= (1 << pin_index);
                break;
            case 3: // RISING edge
                if (current_pin_state && !last_pin_state_bit)
                    new_interrupts |= (1 << pin_index);
                break;
            case 4: // FALLING edge
                if (!current_pin_state && last_pin_state_bit)
                    new_interrupts |= (1 << pin_index);
                break;
            case 5: // CHANGE (any edge)
                new_interrupts |= (1 << pin_index);
                break;
            }
        }

        last_pin_state[port] = current_state;
    }

    // Update interrupt status and trigger interrupt lines
    if (new_interrupts != 0)
    {
        interrupt_status |= new_interrupts;

        // Trigger individual pin interrupt lines
        for (int i = 0; i < 32; i++)
        {
            if (new_interrupts & (1 << i))
            {
                interrupt_lines[i].write(1);
                interrupt_lines[i].write(0); // Pulse
            }
        }

        // Check for port-wide interrupts
        for (int port = 0; port < 4; port++)
        {
            uint8_t port_changes = (new_interrupts >> (port * 8)) & 0xFF;
            if (port_changes != 0)
            {
                port_interrupt_lines[port].write(1);
                port_interrupt_lines[port].write(0); // Pulse
            }
        }
    }
}

void GPIO::enable_pin_interrupt(int port, int pin, int mode)
{
    if (port < 0 || port >= 4 || pin < 0 || pin >= 8)
        return;

    int pin_index = port * 8 + pin;
    pin_interrupt_modes[pin_index] = mode;

    // Initialize last state for this pin
    uint8_t current_state = pin_blocks[port].read() & 0xFF;
    if (pin == 0)
        last_pin_state[port] = current_state;
}

void GPIO::disable_pin_interrupt(int port, int pin)
{
    if (port < 0 || port >= 4 || pin < 0 || pin >= 8)
        return;

    int pin_index = port * 8 + pin;
    pin_interrupt_modes[pin_index] = 0; // Disabled
}

void GPIO::connect_interrupt_line(uint32_t vector, Signal *signal)
{
    if (vector < 32 && signal)
    {
        interrupt_lines[vector].bind(signal);
    }
}

Signal *GPIO::get_interrupt_line(uint32_t vector)
{
    if (vector < 32)
        return &interrupt_lines[vector];
    return nullptr;
}
