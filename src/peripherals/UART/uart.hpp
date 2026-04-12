#pragma once
#include <cstdint>
#include "../Signal/signal.hpp"

class UART
{
public:
    UART(uint32_t baud_rate = 115200)
        : tx(nullptr), rx(nullptr), baud_rate(baud_rate)
    {
        // tx and rx will be set later via connect_tx/connect_rx
    }

    ~UART() = default;

    void connect_tx(Signal *s) { tx = s; }
    void connect_rx(Signal *s) { rx = s; }

    // CPU writes byte into TX buffer
    void write(uint8_t data)
    {
        if (!tx || tx_busy)
            return; // ignore or later make FIFO

        tx_shift = data;
        tx_bit_index = 0;
        tx_phase = START_BIT;
        tx_busy = true;

        // Clear TX interrupt flag when starting transmission
        interrupt_status &= ~UART_INT_TX_EMPTY;
    }

    uint8_t read()
    {
        if (!rx)
            return 0;
        return rx->read();
    }

    // CPU cycles-based simulation
    void tick(uint32_t cpu_cycles)
    {
        if (!tx)
            return;

        cycles_accum += cpu_cycles;

        uint32_t cycles_per_bit = cpu_clock_hz / baud_rate;

        while (tx_busy && cycles_accum >= cycles_per_bit)
        {
            cycles_accum -= cycles_per_bit;

            switch (tx_phase)
            {
            case START_BIT:
                tx->write(0); // start bit
                tx_phase = DATA_BITS;
                break;

            case DATA_BITS:
            {
                uint8_t bit = (tx_shift >> tx_bit_index) & 0x01;
                tx->write(bit);

                tx_bit_index++;

                if (tx_bit_index >= 8)
                    tx_phase = STOP_BIT;

                break;
            }

            case STOP_BIT:
                tx->write(1); // stop bit
                tx_busy = false;
                tx_phase = IDLE;

                // Set TX interrupt flag when transmission completes
                interrupt_status |= UART_INT_TX_EMPTY;

                // Trigger interrupt line if enabled
                if (interrupt_enable & UART_INT_TX_EMPTY)
                {
                    interrupt_lines[0].write(1);
                    interrupt_lines[0].write(0); // Pulse interrupt
                }
                break;

            case IDLE:
            default:
                tx->write(1);
                tx_busy = false;
                break;
            }
        }
    }

    uint32_t get_baud() const { return baud_rate; }
    void set_baud(uint32_t b) { baud_rate = b; }

    void set_cpu_clock(uint32_t hz) { cpu_clock_hz = hz; }

    // =========================
    // INTERRUPT SUPPORT
    // =========================
    uint32_t get_interrupt_status() const { return interrupt_status; }
    uint32_t get_interrupt_enable() const { return interrupt_enable; }
    void set_interrupt_enable(uint32_t enable) { interrupt_enable = enable; }
    void clear_interrupt(uint32_t mask) { interrupt_status &= ~mask; }

private:
    Signal *tx;
    Signal *rx;

    uint32_t baud_rate;
    uint32_t cpu_clock_hz = 50000000; // default 50 MHz

    uint32_t cycles_accum = 0;

    // TX state machine
    enum Phase
    {
        IDLE,
        START_BIT,
        DATA_BITS,
        STOP_BIT
    };
    Phase tx_phase = IDLE;

    bool tx_busy = false;

    uint8_t tx_shift = 0;
    uint8_t tx_bit_index = 0;

    // Interrupt support
    uint32_t interrupt_status = 0; // Active UART interrupts
    uint32_t interrupt_enable = 0; // Enabled UART interrupts
    Signal interrupt_lines[2];     // TX and RX interrupt lines

    // Interrupt flag definitions
    static constexpr uint32_t UART_INT_TX_EMPTY = 0x01;
    static constexpr uint32_t UART_INT_RX_READY = 0x02;

public:
    // Interrupt method implementations
    bool is_tx_interrupt_pending() const
    {
        return (interrupt_status & interrupt_enable & UART_INT_TX_EMPTY) != 0;
    }

    bool is_rx_interrupt_pending() const
    {
        return (interrupt_status & interrupt_enable & UART_INT_RX_READY) != 0;
    }

    void connect_interrupt_line(uint32_t vector, Signal *signal)
    {
        if (vector < 2 && signal)
        {
            interrupt_lines[vector].bind(signal);
        }
    }

    Signal *get_interrupt_line(uint32_t vector)
    {
        if (vector < 2)
            return &interrupt_lines[vector];
        return nullptr;
    }
};