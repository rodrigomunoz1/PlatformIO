#ifndef KEYBOARD_MATRIX_H
#define KEYBOARD_MATRIX_H

#include <stdint.h>

// Keyboard matrix configuration
#define KB_ROWS 4
#define KB_COLS 4

// Row output pins (KB out1-4)
#define KB_OUT1 25
#define KB_OUT2 26
#define KB_OUT3 27
#define KB_OUT4 14

// Column input pins (KB in1-4)
#define KB_IN1  36  // SENSOR_VP
#define KB_IN2  39  // SENSOR_VN
#define KB_IN3  34
#define KB_IN4  35

// Function declarations
void keyboard_init();
uint16_t keyboard_scan();  // Returns bitmask of pressed keys (16 keys)
void keyboard_activate_row(uint8_t row);
void keyboard_deactivate_all();
uint8_t keyboard_read_cols();

#endif // KEYBOARD_MATRIX_H
