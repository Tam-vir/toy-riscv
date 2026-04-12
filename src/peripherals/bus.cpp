#include "bus.hpp"
#include <iostream>

Bus::Bus()
{
    gpio = new GPIO();
    uart = new UART();
    current_route_mode = RouteMode::GPIO_ONLY;

    // Initialize signal array and bind to pins
    for (int i = 0; i < 32; i++)
    {
        signals[i].set(0);         // default LOW
        pins[i].bind(&signals[i]); // pin <-> signal
        pins[i].set_mode(Pin::INPUT);
    }

    // Initialize interrupt lines
    for (int i = 0; i < BUS_INT_MAX; i++)
    {
        interrupt_lines[i].set(0);
    }

    // Apply initial routing
    apply_routing();
}

Bus::~Bus()
{
    delete gpio;
    delete uart;
}

// MMIO READ
uint32_t Bus::load(uint32_t addr)
{
    // Handle interrupt status and enable registers
    if (is_interrupt(addr))
    {
        switch (addr)
        {
        case 0x1100: // BUS_INTERRUPT_STATUS
            return interrupt_status;
        case 0x1104: // BUS_INTERRUPT_ENABLE
            return interrupt_enable;
        case 0x1108: // BUS_INTERRUPT_PENDING
            return interrupt_status & interrupt_enable;
        default:
            return 0;
        }
    }

    if (is_gpio(addr))
    {
        auto [port, reg] = decode_gpio(addr);
        if (gpio)
            return gpio->read(port, reg);
        return 0;
    }
    else if (is_uart(addr))
    {
        auto [port, reg] = decode_uart(addr);
        if (!uart)
            return 0;
        switch (reg)
        {
        case 0x0: // UART_DATA
            return uart_read();
        case 0x4: // UART_STATUS
            return uart->get_baud();
        default:
            return 0;
        }
    }

    return 0;
}

// MMIO WRITE
void Bus::store(uint32_t addr, uint32_t val)
{
    //std::cerr << "[BUS] store addr=0x" << std::hex << addr << std::dec << " val=" << val << std::endl;

    // Handle interrupt enable register
    if (is_interrupt(addr))
    {
        switch (addr)
        {
        case 0x1104: // BUS_INTERRUPT_ENABLE
            interrupt_enable = val;
            break;
        case 0x110C: // BUS_INTERRUPT_CLEAR
            interrupt_status &= ~val;
            break;
        }
        return;
    }

    if (is_gpio(addr))
    {
        auto [port, reg] = decode_gpio(addr);
        if (gpio)
            gpio->write(port, reg, val);
        return;
    }
    else if (is_uart(addr))
    {
        auto [port, reg] = decode_uart(addr);
        if (!uart)
            return;
        switch (reg)
        {
        case 0x0: // UART_DATA
            uart_write(val & 0xFF);
            break;
        case 0x4: // UART_CONTROL
            uart->set_baud(val);
            break;
        }
        return;
    }
}

// ROUTING CONTROL
void Bus::set_route_mode(RouteMode mode)
{
    if (current_route_mode != mode)
    {
        current_route_mode = mode;
        apply_routing();
    }
}

RouteMode Bus::get_route_mode() const
{
    return current_route_mode;
}

// UART ACCESS
void Bus::uart_write(uint8_t data)
{
    uart->write(data);
}

uint8_t Bus::uart_read()
{
    return uart->read();
}

void Bus::uart_tick(uint32_t cpu_cycles)
{
    uart->tick(cpu_cycles);
}

// TICK (Periodic Update)
void Bus::tick(uint32_t cpu_cycles)
{
    // Check GPIO pin state changes and aggregate interrupts
    if (gpio)
    {
        gpio->check_pin_interrupts();

        // Aggregate GPIO interrupts into Bus interrupt_status
        uint32_t gpio_interrupts = gpio->get_interrupt_status();
        if (gpio_interrupts != 0)
        {
            // Add GPIO interrupts to bus interrupt status (pins 0-15)
            for (int i = 0; i < 16; i++)
            {
                if (gpio_interrupts & (1 << i))
                {
                    interrupt_status |= (1 << i);
                }
            }
            // These are aggregated by GPIO internally
            gpio->clear_interrupt(gpio_interrupts);
        }
    }

    // Update UART state and check for tx/rx events
    if (uart)
    {
        uart->tick(cpu_cycles);

    }
}

// PIN ACCESS (for external / debug)
Pin *Bus::get_pin(int index)
{
    if (index < 0 || index >= 32)
        return nullptr;
    return &pins[index];
}

// SIGNAL ACCESS (for UART, SPI, etc.)
Signal *Bus::get_signal(int index)
{
    if (index < 0 || index >= 32)
        return nullptr;
    return &signals[index];
}

// ROUTING IMPLEMENTATION
void Bus::apply_routing()
{
    switch (current_route_mode)
    {
    case RouteMode::GPIO_ONLY:
        route_gpio_only();
        break;
    case RouteMode::UART_GPIO:
        route_uart_gpio();
        break;
    case RouteMode::CUSTOM:
        route_gpio_only();
        break;
    }
}

void Bus::route_gpio_only()
{
    if (!uart || !gpio)
        return;

    // Disconnect UART from signals
    uart->connect_tx(nullptr);
    uart->connect_rx(nullptr);

    // Connect all pins to GPIO
    for (int port = 0; port < 4; port++)
    {
        for (int pin = 0; pin < 8; pin++)
        {
            int pin_index = port * 8 + pin;
            gpio->connect_pin(port, pin, &signals[pin_index]);
        }
    }

    // Enable GPIO interrupt checking
    gpio->check_pin_interrupts();
}

void Bus::route_uart_gpio()
{
    if (!uart || !gpio)
        return;

    // UART on pins 0 (TX) and 1 (RX)
    uart->connect_tx(&signals[0]); // TX on pin 0
    uart->connect_rx(&signals[1]); // RX on pin 1

    // Disconnect these pins from GPIO
    gpio->disconnect_pin(0, 0);
    gpio->disconnect_pin(0, 1);

    // Connect remaining pins to GPIO
    for (int port = 0; port < 4; port++)
    {
        for (int pin = 0; pin < 8; pin++)
        {
            int pin_index = port * 8 + pin;
            if (pin_index > 1)
                gpio->connect_pin(port, pin, &signals[pin_index]);
        }
    }

    // Enable GPIO interrupt checking
    gpio->check_pin_interrupts();
}

// Address decoding
bool Bus::is_gpio(uint32_t addr)
{
    return addr >= 0x1000 && addr <= 0x101F;
}

bool Bus::is_uart(uint32_t addr)
{
    return addr >= 0x1020 && addr <= 0x102F;
}

bool Bus::is_interrupt(uint32_t addr)
{
    return addr >= 0x1100 && addr <= 0x110F;
}

std::pair<int, int> Bus::decode_gpio(uint32_t addr)
{
    uint32_t offset = addr - 0x1000;
    int port = offset / 0x8; // Each port is 0x8 bytes apart
    int reg = offset % 0x8;  // Register offset within port
    return {port, reg};
}

std::pair<int, int> Bus::decode_uart(uint32_t addr)
{
    uint32_t offset = addr - 0x1020;
    int reg = offset % 8;
    return {0, reg}; // UART only has one port
}

// INTERRUPT SUPPORT
uint32_t Bus::get_interrupt_status() const
{
    return interrupt_status;
}

uint32_t Bus::get_interrupt_enable() const
{
    return interrupt_enable;
}

void Bus::set_interrupt_enable(uint32_t enable)
{
    interrupt_enable = enable;
}

void Bus::clear_interrupt(uint32_t interrupt_mask)
{
    interrupt_status &= ~interrupt_mask;
}

void Bus::trigger_interrupt(uint32_t interrupt_vector)
{
    if (interrupt_vector < BUS_INT_MAX)
    {
        interrupt_status |= (1 << interrupt_vector);
        interrupt_lines[interrupt_vector].write(1);
        interrupt_lines[interrupt_vector].write(0); // Pulse interrupt
    }
}

bool Bus::is_interrupt_pending(uint32_t interrupt_vector) const
{
    if (interrupt_vector >= BUS_INT_MAX)
        return false;
    return (interrupt_status & interrupt_enable & (1 << interrupt_vector)) != 0;
}

void Bus::connect_interrupt_line(uint32_t vector, Signal *signal)
{
    if (vector < BUS_INT_MAX && signal)
    {
        interrupt_lines[vector].bind(signal);
    }
}

Signal *Bus::get_interrupt_line(uint32_t vector)
{
    if (vector < BUS_INT_MAX)
        return &interrupt_lines[vector];
    return nullptr;
}