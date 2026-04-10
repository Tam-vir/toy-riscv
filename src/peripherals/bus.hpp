#pragma once

#include <cstdint>
#include <utility>
#include "GPIO/gpio.hpp"
#include "UART/uart.hpp"
#include "Signal/signal.hpp"
#include "pin.hpp"

enum class RouteMode
{
    GPIO_ONLY, // All pins routed to GPIO
    UART_GPIO, // Pins 0-1 for UART, rest for GPIO
    CUSTOM     // Custom routing configuration
};

// Bus interrupt vector definitions
enum BusInterruptVector
{
    BUS_INT_GPIO_PIN0 = 0,
    BUS_INT_GPIO_PIN1 = 1,
    BUS_INT_GPIO_PIN2 = 2,
    BUS_INT_GPIO_PIN3 = 3,
    BUS_INT_GPIO_PIN4 = 4,
    BUS_INT_GPIO_PIN5 = 5,
    BUS_INT_GPIO_PIN6 = 6,
    BUS_INT_GPIO_PIN7 = 7,
    BUS_INT_GPIO_PIN8 = 8,
    BUS_INT_GPIO_PIN9 = 9,
    BUS_INT_GPIO_PIN10 = 10,
    BUS_INT_GPIO_PIN11 = 11,
    BUS_INT_GPIO_PIN12 = 12,
    BUS_INT_GPIO_PIN13 = 13,
    BUS_INT_GPIO_PIN14 = 14,
    BUS_INT_GPIO_PIN15 = 15,
    BUS_INT_UART_TX = 16,
    BUS_INT_UART_RX = 17,
    BUS_INT_GPIO_PORT0 = 24,
    BUS_INT_GPIO_PORT1 = 25,
    BUS_INT_GPIO_PORT2 = 26,
    BUS_INT_GPIO_PORT3 = 27,
    BUS_INT_MAX = 32
};

class Bus
{
public:
    Bus();
    ~Bus();

    // =========================
    // MMIO READ/WRITE
    // =========================
    uint32_t load(uint32_t addr);
    void store(uint32_t addr, uint32_t val);

    // =========================
    // ROUTING CONTROL
    // =========================
    void set_route_mode(RouteMode mode);
    RouteMode get_route_mode() const;

    // =========================
    // UART ACCESS
    // =========================
    void uart_write(uint8_t data);
    uint8_t uart_read();
    void uart_tick(uint32_t cpu_cycles);

    // =========================
    // PIN ACCESS (for external / debug)
    // =========================
    Pin *get_pin(int index);

    // =========================
    // SIGNAL ACCESS (for UART, SPI, etc.)
    // =========================
    Signal *get_signal(int index);

    // =========================
    // INTERRUPT SUPPORT
    // =========================
    uint32_t get_interrupt_status() const;
    uint32_t get_interrupt_enable() const;
    void set_interrupt_enable(uint32_t enable);
    void clear_interrupt(uint32_t interrupt_mask);
    void trigger_interrupt(uint32_t interrupt_vector);
    bool is_interrupt_pending(uint32_t interrupt_vector) const;
    void connect_interrupt_line(uint32_t vector, Signal *signal);
    Signal *get_interrupt_line(uint32_t vector);

private:
    GPIO *gpio;
    UART *uart;
    RouteMode current_route_mode;

    Pin pins[32];       // physical pins
    Signal signals[32]; // wires

    // =========================
    // INTERRUPT STATE
    // =========================
    uint32_t interrupt_status = 0;       // Active interrupts
    uint32_t interrupt_enable = 0;       // Enabled interrupts
    Signal interrupt_lines[BUS_INT_MAX]; // Physical interrupt lines

    // =========================
    // ROUTING IMPLEMENTATION
    // =========================
    void apply_routing();
    void route_gpio_only();
    void route_uart_gpio();

    // =========================
    // Address decoding
    // =========================
    bool is_gpio(uint32_t addr);
    bool is_uart(uint32_t addr);
    bool is_interrupt(uint32_t addr);
    std::pair<int, int> decode_gpio(uint32_t addr);
    std::pair<int, int> decode_uart(uint32_t addr);
};