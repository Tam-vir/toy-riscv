#include "led.hpp"

LED::LED() : Device()
{
}

LED::~LED()
{
}

void LED::set(bool state)
{
    Device::set(state);
}

bool LED::get() const
{
    return Device::get();
}

uint8_t LED::on_gpio_read()
{
    // LED is an output device: return current state
    return state ? 1 : 0;
}

void LED::on_gpio_write(uint8_t value)
{
    // LED controlled by GPIO write
    Device::set(value != 0);
}
