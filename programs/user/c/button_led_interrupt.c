#include <stdint.h>
#include "../../../include/stndio.h"
// MMIO addresses
#define GPIO_PORT0_DATA      0x1000
#define GPIO_PORT0_MODE      0x1004
#define GPIO_PORT1_DATA      0x1020
#define GPIO_PORT1_MODE      0x1024
#define BUS_INT_ENABLE       0x1104
#define BUS_INT_STATUS       0x1100
#define BUS_INT_CLEAR        0x110C

// GPIO modes
#define GPIO_INPUT  0
#define GPIO_OUTPUT 1

// Read/write MMIO
static inline uint32_t read_u32(uint32_t addr) {
    return *(volatile uint32_t*)addr;
}

static inline void write_u32(uint32_t addr, uint32_t val) {
    *(volatile uint32_t*)addr = val;
}

// Interrupt control functions are defined in crt0.s
extern void enable_interrupt(int vector);
extern void disable_interrupt(int vector);
extern void enable_interrupts(void);


// LED on/off
void set_led(int on) {
    uint32_t val = on ? 1 : 0;
    write_u32(GPIO_PORT1_DATA, val);
}

// Read button state
int read_button(void) {
    uint32_t val = read_u32(GPIO_PORT0_DATA);
    return (val & 1) ? 1 : 0;
}

// Global counter for button presses
volatile int button_count = 0;
volatile int last_button_state = 0;

int main(void) {
    prts("=== Button/LED Interrupt Demo ===\n");
    
    // Configure GPIO
    // Port 0: input (button at pin 0)
    write_u32(GPIO_PORT0_MODE, 0x00);
    
    // Port 1: output (LED at pin 0) 
    write_u32(GPIO_PORT1_MODE, 0xFF);
    
    // Initialize LED to off
    set_led(0);
    
    prts("Waiting for button press...\n");
    prts("Use 'p' key to simulate button press\n\n");
    
    // Poll the button state
    while (1) {
        int button_state = read_button();
        
        // Detect rising edge (0 -> 1 transition)
        if (button_state && !last_button_state) {
            button_count++;
            
            // Toggle LED
            set_led(button_count & 1);
            
            prts("Button pressed! Count: ");
            prtnum(button_count);
            prtc('\n');
        }
        
        last_button_state = button_state;
    }
    
    return 0;
}
