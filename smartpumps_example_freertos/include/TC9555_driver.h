#ifndef TC9555_DRIVER_H
#define TC9555_DRIVER_H

#include <stdint.h>
#include <Wire.h>

// I2C addresses
#define TC9555_IN_ADDR 0x20
#define TC9555_OUT_ADDR 0x21

// IO expander pins
typedef struct {
    uint8_t port;  // Port number (0-4)
    uint8_t pin;   // Pin in port (0-7)
} IOPin;

// Input mappings (TC9555_IN)
extern const IOPin input_pins[16];  // inputs 1-16, indexed 0-15

// Output mappings (TC9555_OUT)
extern const IOPin output_pins[16]; // outputs 1-16, indexed 0-15

// Function declarations
void TC9555_init();
uint16_t TC9555_read_inputs();
void TC9555_write_outputs(uint16_t outputs);
uint8_t TC9555_read_port(uint8_t addr, uint8_t port);
void TC9555_write_port(uint8_t addr, uint8_t port, uint8_t value);

#endif // TC9555_DRIVER_H
