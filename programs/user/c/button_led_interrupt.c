#include <stdint.h>
#include "../../../include/stndio.h"
// MMIO addresses
#define GPIO_PORT0_DATA      0x1000
#define GPIO_PORT0_MODE      0x1004
#define GPIO_PORT1_DATA      0x1008
#define GPIO_PORT1_MODE      0x100C
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


// LED on/off (bit 0 of GPIO_PORT1_DATA)
void set_led(int on) {
    uint32_t val = read_u32(GPIO_PORT1_DATA);
    if (on) {
        val |= (1 << 0);  // Set bit 0
    } else {
        val &= ~(1 << 0); // Clear bit 0
    }
    write_u32(GPIO_PORT1_DATA, val);
}
// Read LED state (bit 0 of GPIO_PORT1_DATA)
int read_led(void) {
    uint32_t val = read_u32(GPIO_PORT1_DATA);
    return (val & (1 << 0)) ? 1 : 0;
}

// Read button state (bit 0 of GPIO_PORT0_DATA)
int read_button(void) {
    uint32_t val = read_u32(GPIO_PORT0_DATA);
    return (val & (1 << 0)) ? 1 : 0;
}

// Global counter for button presses
volatile int button_count = 0;
volatile int last_button_state = 0;

int main(void) {
    prts("=== Button/LED Interrupt Demo ===\n");
    
    // Configure GPIO modes (bit=0: output, bit=1: input)
    // Port 0: input (button at pin 0) - set bit 0 to 1
    write_u32(GPIO_PORT0_MODE, 0x01);
    
    // Port 1: output (LED at pin 0) - set bit 0 to 0
    write_u32(GPIO_PORT1_MODE, 0x00);
    
    // Initialize LED to off
    set_led(0);
    
    prts("Waiting for button press...\n");
    prts("Use 'p' key to simulate button press\n\n");
    
    // Poll the button state
    while (1) {
        int button_state = read_button();
        int led_state = read_led();
        
        // Detect rising edge (0 -> 1 transition)
        if (button_state && !last_button_state) {
            prts("[BEFORE] Button: ");
            prtnum(last_button_state);
            prts(", LED: ");
            prtnum(led_state);
            prtc('\n');
            
            button_count++;
            
            // Toggle LED
            int new_led_value = !led_state;
            set_led(new_led_value);
            int new_led_state = read_led();
            
            prts("Button pressed! Count: ");
            prtnum(button_count);
            
            prts("\n[AFTER] Button: ");
            prtnum(button_state);
            prts(", LED: ");
            prtnum(new_led_state);
            prtc('\n');
            prtc('\n');
        }
        
        last_button_state = button_state;
    }
    
    return 0;
}
