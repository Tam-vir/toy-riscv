#pragma once
#include <cstdint>
#include "../device.hpp"

class Button : public Device
{
public:
    Button();
    virtual ~Button();

    virtual bool get() const override;
    virtual void set(bool state) override;

    // GPIO read: return button state
    virtual uint8_t on_gpio_read() override;

    // GPIO write: button doesn't accept writes (input device)
    virtual void on_gpio_write(uint8_t value) override;

    // Handle terminal input - pressing 't' toggles button
    void handle_terminal_input(char key);

private:
    bool last_state = false;
};
