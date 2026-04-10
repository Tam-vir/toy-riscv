#include "../../../include/stndio.h"

// Global variables to track interrupt counts
volatile int gpio_pin0_count = 0;
volatile int gpio_pin1_count = 0;
volatile int gpio_port0_count = 0;
volatile int uart_rx_count = 0;

// External functions from boot.s
extern void enable_interrupts(void);
extern void disable_interrupts(void);
extern void enable_interrupt(int interrupt_num);
extern void disable_interrupt(int interrupt_num);
extern void configure_gpio_interrupt(int port, int pin, int mode);

// GPIO interrupt modes
#define GPIO_INT_DISABLED 0
#define GPIO_INT_LOW      1
#define GPIO_INT_HIGH     2
#define GPIO_INT_RISING   3
#define GPIO_INT_FALLING  4
#define GPIO_INT_CHANGE   5

// Interrupt vector numbers
#define INT_GPIO_PIN0   16
#define INT_GPIO_PIN1   17
#define INT_GPIO_PIN8   24
#define INT_GPIO_PIN9   25
#define INT_GPIO_PORT0  48
#define INT_UART_RX     53

// GPIO register addresses
#define GPIO0_DATA      0x1000
#define GPIO0_MODE      0x1004
#define GPIO1_DATA      0x1010
#define GPIO1_MODE      0x1014

// Bus interrupt registers
#define BUS_INT_STATUS  0x1100
#define BUS_INT_ENABLE  0x1104
#define BUS_INT_CLEAR   0x110C

// User interrupt handlers (weak symbols from boot.s)
extern void gpio_pin0_user_handler(void);
extern void gpio_pin1_user_handler(void);
extern void gpio_pin8_user_handler(void);
extern void gpio_pin9_user_handler(void);
extern void gpio_port0_user_handler(void);
extern void uart_rx_user_handler(void);

// GPIO Pin 0 interrupt handler
void gpio_pin0_user_handler(void) {
    gpio_pin0_count++;
    prts("GPIO Pin 0 interrupt! Count: ");
    prtnum(gpio_pin0_count);
    prtc('\n');
}

// GPIO Pin 1 interrupt handler
void gpio_pin1_user_handler(void) {
    gpio_pin1_count++;
    prts("GPIO Pin 1 interrupt! Count: ");
    prtnum(gpio_pin1_count);
    prtc('\n');
}

// GPIO Pin 8 interrupt handler
void gpio_pin8_user_handler(void) {
    prts("GPIO Pin 8 (Port 1, Pin 0) interrupt!\n");
}

// GPIO Pin 9 interrupt handler
void gpio_pin9_user_handler(void) {
    prts("GPIO Pin 9 (Port 1, Pin 1) interrupt!\n");
}

// GPIO Port 0 interrupt handler
void gpio_port0_user_handler(void) {
    gpio_port0_count++;
    prts("GPIO Port 0 interrupt! Count: ");
    prtnum(gpio_port0_count);
    prtc('\n');
}

// UART RX interrupt handler
void uart_rx_user_handler(void) {
    uart_rx_count++;
    prts("UART RX interrupt! Count: ");
    prtnum(uart_rx_count);
    prtc('\n');
}

// Configure GPIO pins for interrupt example
void setup_gpio_interrupts(void) {
    prts("Setting up GPIO interrupts...\n");
    
    prts("Debug: Configuring GPIO pins as inputs...\n");
    // Configure GPIO pins as inputs
    *(volatile int*)GPIO0_MODE = 0xFF;  // All pins as inputs
    *(volatile int*)GPIO1_MODE = 0xFF;  // All pins as inputs
    prts("Debug: GPIO pins configured as inputs\n");
    
    prts("Debug: Configuring individual pin interrupts...\n");
    // Configure individual pin interrupts (this just sets up the hardware)
    configure_gpio_interrupt(0, 0, GPIO_INT_RISING);   // Pin 0: rising edge
    prts("Debug: Pin 0 configured\n");
    configure_gpio_interrupt(0, 1, GPIO_INT_FALLING); // Pin 1: falling edge
    prts("Debug: Pin 1 configured\n");
    configure_gpio_interrupt(1, 0, GPIO_INT_CHANGE);   // Pin 8: any change
    prts("Debug: Pin 8 configured\n");
    configure_gpio_interrupt(1, 1, GPIO_INT_HIGH);      // Pin 9: high level
    prts("Debug: Pin 9 configured\n");
    
    prts("Debug: Enabling individual interrupts in bus...\n");
    // Enable individual interrupts in bus
    enable_interrupt(INT_GPIO_PIN0);
    prts("Debug: Pin 0 interrupt enabled\n");
    enable_interrupt(INT_GPIO_PIN1);
    prts("Debug: Pin 1 interrupt enabled\n");
    enable_interrupt(INT_GPIO_PIN8);
    prts("Debug: Pin 8 interrupt enabled\n");
    enable_interrupt(INT_GPIO_PIN9);
    prts("Debug: Pin 9 interrupt enabled\n");
    enable_interrupt(INT_GPIO_PORT0);
    prts("Debug: Port 0 interrupt enabled\n");
    enable_interrupt(INT_UART_RX);
    prts("Debug: UART RX interrupt enabled\n");
    
    prts("Debug: Enabling global interrupts...\n");
    // Enable global interrupts LAST, after everything is configured
    enable_interrupts();
    prts("Debug: Global interrupts enabled\n");
    
    prts("GPIO interrupts configured!\n");
}

// Simulate GPIO pin changes for testing
void simulate_gpio_changes(void) {
    prts("Simulating GPIO pin changes...\n");
    
    // Simulate pin 0 going high (should trigger rising edge)
    *(volatile int*)GPIO0_DATA = 0x01;
    
    // Simulate pin 1 going low (should trigger falling edge)
    *(volatile int*)GPIO0_DATA = 0x00;
    
    // Simulate pin 8 changing (should trigger change)
    *(volatile int*)GPIO1_DATA = 0x01;
    *(volatile int*)GPIO1_DATA = 0x00;
    
    // Simulate pin 9 going high (should trigger high level)
    *(volatile int*)GPIO1_DATA = 0x02;
    
    prts("GPIO simulation complete!\n");
}

// Display interrupt status
void show_interrupt_status(void) {
    int status = *(volatile int*)BUS_INT_STATUS;
    int enabled = *(volatile int*)BUS_INT_ENABLE;
    
    prts("Interrupt Status: 0x");
    prtnum(status);
    prts(" Enabled: 0x");
    prtnum(enabled);
    prtc('\n');
    
    prts("Pin 0 count: ");
    prtnum(gpio_pin0_count);
    prts(" Pin 1 count: ");
    prtnum(gpio_pin1_count);
    prts(" Port 0 count: ");
    prtnum(gpio_port0_count);
    prts(" UART RX count: ");
    prtnum(uart_rx_count);
    prtc('\n');
}

void main(void) {
    prts("=== GPIO Interrupt Example ===\n");
    
    // Setup GPIO interrupts
    setup_gpio_interrupts();
    
    // Show initial status
    show_interrupt_status();
    
    // Simulate some GPIO activity
    simulate_gpio_changes();
    
    // Show final status
    prts("\nFinal interrupt status:\n");
    show_interrupt_status();
    
    prts("\nGPIO interrupt example complete!\n");
    
    // Keep running to allow more interrupts
    while (1) {
        // Main loop - interrupts will occur asynchronously
    }
}