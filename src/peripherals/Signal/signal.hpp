#pragma once

#include <cstdint>

class Signal
{
public:
    Signal() = default;
    ~Signal() = default;

    void write(uint8_t val)
    {
        value = val;
        if (bound_signal)
        {
            bound_signal->value = val; // propagate to bound signal
        }
    }

    uint8_t read() const
    {
        if (bound_signal)
        {
            return bound_signal->value; // read from bound signal
        }
        return value;
    }

    void set(uint8_t val) { write(val); }  // Alias for write
    uint8_t get() const { return read(); } // Alias for read

    void bind(Signal *s)
    {
        bound_signal = s;
    } // simple wire connection

private:
    uint8_t value = 0;
    Signal *bound_signal = nullptr;
};