#include "device.hpp"

Device::Device() : state(false), gpio_port(-1), gpio_pin(-1)
{
}

Device::~Device()
{
}

bool Device::get() const
{
    return state;
}

void Device::set(bool new_state)
{
    if (state != new_state)
    {
        state = new_state;
        on_state_changed(new_state);

        // Trigger interrupt if callback is registered
        if (interrupt_callback)
        {
            interrupt_callback(state ? 1 : 0);
        }
    }
}

void Device::set_gpio_location(int port, int pin)
{
    gpio_port = port;
    gpio_pin = pin;
}

uint8_t Device::on_gpio_read()
{
    // Default: return device state
    return state ? 1 : 0;
}

void Device::on_gpio_write(uint8_t value)
{
    set(value != 0);
}

void Device::set_interrupt_callback(std::function<void(uint32_t)> callback)
{
    interrupt_callback = callback;
}

void Device::on_state_changed(bool new_state)
{
}
