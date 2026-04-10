#pragma once
#include <cstdint>
#include "./Signal/signal.hpp"

class Pin
{
public:
    enum Mode
    {
        INPUT,
        OUTPUT
    };

    Pin() = default;

    // Bind this pin to a physical wire
    void bind(Signal *s)
    {
        signal = s;
    }

    // Set direction
    void set_mode(Mode m)
    {
        mode = m;
    }

    Mode get_mode() const
    {
        return mode;
    }

    // Drive signal (only if OUTPUT)
    void write(uint8_t value)
    {
        if (!signal || mode != OUTPUT)
            return;

        signal->set(value ? 1 : 0);
    }

    // Read signal (always allowed, MCU-style)
    uint8_t read() const
    {
        if (!signal)
            return 0;

        return signal->get();
    }

private:
    Signal *signal = nullptr;
    Mode mode = INPUT;
};