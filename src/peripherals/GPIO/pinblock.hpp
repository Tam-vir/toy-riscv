#pragma once
#include <cstdint>
#include "../Signal/signal.hpp"

class PinBlock
{
public:
    uint8_t mode = 0;         // 1=input, 0=output
    uint8_t external_in = 0;  // External input state for input pins
    uint8_t output_state = 0; // Output drive state for output pins
    Signal pins[8];

    void write_mode(uint8_t val)
    {
        mode = val;
    }

    void write_out(uint8_t val)
    {
        for (int i = 0; i < 8; i++)
        {
            if (!(mode & (1 << i))) // output
            {
                uint8_t bit = (val >> i) & 1;
                if (bit)
                    output_state |= (1 << i);
                else
                    output_state &= ~(1 << i);
                pins[i].set(bit);
            }
        }
    }

    // Set external input state (for input pins from outside the CPU)
    void set_external_in(uint8_t val)
    {
        external_in = val;
    }

    uint8_t read()
    {
        uint8_t result = 0;

        for (int i = 0; i < 8; i++)
        {
            uint8_t bit;

            if (mode & (1 << i)) // input pin
                bit = (external_in >> i) & 1;
            else // output pin
                bit = (output_state >> i) & 1;

            result |= (bit << i);
        }

        return result;
    }

    uint8_t read_mode()
    {
        return mode;
    }
};