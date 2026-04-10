#include "button.hpp"

Button::Button() : Device(), last_state(false)
{
}

Button::~Button()
{
}

bool Button::get() const
{
    return Device::get();
}

void Button::set(bool state)
{
    Device::set(state);
}

uint8_t Button::on_gpio_read()
{
    // Button is an input device: return current state
    return state ? 1 : 0;
}

void Button::on_gpio_write(uint8_t value)
{
    // Button is INPUT-only, but we can simulate button press via write
    // This allows software-triggered button simulation
    bool new_state = (value != 0);
    if (new_state != last_state)
    {
        last_state = new_state;
        Device::set(new_state); // This will trigger interrupt if registered
    }
}

void Button::handle_terminal_input(char key)
{
    // Toggle button state when 't' is pressed
    if (key == 't' || key == 'T')
    {
        Device::set(!state); // Toggle state and trigger interrupt callback
    }
}
