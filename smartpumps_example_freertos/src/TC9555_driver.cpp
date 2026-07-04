#include "TC9555_driver.h"

// Input pin mappings (TC9555_IN at 0x20)
const IOPin input_pins[16] = {
    {1, 3},  // Input 1 -> P13
    {1, 4},  // Input 2 -> P14
    {1, 5},  // Input 3 -> P15
    {1, 2},  // Input 5 -> P12 (gap for input 4)
    {1, 1},  // Input 6 -> P11
    {1, 0},  // Input 7 -> P10
    {0, 0},  // Input 8 -> P00
    {0, 1},  // Input 9 -> P01
    {0, 3},  // Input 11 -> P03 (gap for input 10)
    {0, 4},  // Input 12 -> P04
    {0, 5},  // Input 13 -> P05
    {0, 6},  // Input 14 -> P06
    {0, 7},  // Input 15 -> P07
};

// Output pin mappings (TC9555_OUT at 0x21)
const IOPin output_pins[16] = {
    {1, 6},  // Output 2 -> P16
    {1, 5},  // Output 3 -> P15
    {1, 4},  // Output 4 -> P14
    {0, 0},  // Output 7 -> P00
    {0, 1},  // Output 8 -> P01
    {0, 2},  // Output 9 -> P02
    {0, 3},  // Output 10 -> P03
    {0, 4},  // Output 11 -> P04
    {0, 5},  // Output 12 -> P05
    {0, 6},  // Output 13 -> P06
    {0, 7},  // Output 14 -> P07
    {1, 0},  // Output 15 -> P10
    {1, 1},  // Output 16 -> P11
};

void TC9555_init() {
    Wire.begin(21, 22);  // SDA=GPIO21, SCL=GPIO22
    Wire.setClock(100000);
    
    // Configure all ports as inputs on TC9555_IN, outputs on TC9555_OUT
    for (int i = 0; i < 5; i++) {
        TC9555_write_port(TC9555_IN_ADDR, i, 0xFF);   // All inputs
        TC9555_write_port(TC9555_OUT_ADDR, i, 0x00);  // All outputs
    }
}

uint8_t TC9555_read_port(uint8_t addr, uint8_t port) {
    Wire.beginTransmission(addr);
    Wire.write(port);
    Wire.endTransmission();
    
    Wire.requestFrom((int)addr, 1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0;
}

void TC9555_write_port(uint8_t addr, uint8_t port, uint8_t value) {
    Wire.beginTransmission(addr);
    Wire.write(port);
    Wire.write(value);
    Wire.endTransmission();
}

uint16_t TC9555_read_inputs() {
    uint16_t result = 0;
    
    // Read all 5 ports from TC9555_IN
    for (int port = 0; port < 5; port++) {
        uint8_t value = TC9555_read_port(TC9555_IN_ADDR, port);
        // Map port pins to input numbers (simplified for 13 active inputs)
        if (port == 0) {
            result |= ((value & 0xFF) << 8);  // Port 0 pins 0-7
        } else if (port == 1) {
            result |= (value & 0x3F);  // Port 1 pins 0-5
        }
    }
    
    return result;
}

void TC9555_write_outputs(uint16_t outputs) {
    // Write to TC9555_OUT ports
    TC9555_write_port(TC9555_OUT_ADDR, 0, (outputs >> 8) & 0xFF);  // Port 0
    TC9555_write_port(TC9555_OUT_ADDR, 1, outputs & 0xFF);         // Port 1
}
