#pragma once
#include <cstdint>
#include "../Signal/signal.hpp"

class PinBlock
{
public:
    uint8_t mode = 0; // 1=input, 0=output
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
                pins[i].set((val >> i) & 1);
        }
    }

    uint8_t read()
    {
        uint8_t result = 0;

        for (int i = 0; i < 8; i++)
        {
            uint8_t bit = pins[i].get();
            result |= (bit << i);
        }

        return result;
    }

    uint8_t read_mode()
    {
        return mode;
    }
};