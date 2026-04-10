#pragma once
#include <cstdint>
#include "../device.hpp"

class LED : public Device
{
public:
    LED();
    virtual ~LED();

    virtual void set(bool state) override;
    virtual bool get() const override;

    // GPIO read: return LED state
    virtual uint8_t on_gpio_read() override;

    // GPIO write: set LED state from GPIO
    virtual void on_gpio_write(uint8_t value) override;

private:
};