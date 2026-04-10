#pragma once
#include <cstdint>
#include <functional>

// Forward declaration
class Signal;

class Device
{
public:
    Device();
    virtual ~Device();

    // Core device state
    virtual bool get() const;
    virtual void set(bool state);

    // GPIO integration: which GPIO port/pin this device is attached to
    void set_gpio_location(int port, int pin);
    int get_gpio_port() const { return gpio_port; }
    int get_gpio_pin() const { return gpio_pin; }

    // Called by GPIO when pin is read (return device value)
    virtual uint8_t on_gpio_read();

    // Called by GPIO when pin is written (update device from GPIO)
    virtual void on_gpio_write(uint8_t value);

    // Interrupt callback registration
    // Called when device state changes and can trigger interrupt
    void set_interrupt_callback(std::function<void(uint32_t)> callback);

    // Virtual update for derived classes to implement custom behavior
    virtual void on_state_changed(bool new_state);

protected:
    bool state = false; // true = active, false = inactive
    int gpio_port = -1;
    int gpio_pin = -1;
    std::function<void(uint32_t)> interrupt_callback;
};