#include "keyboard_matrix.h"
#include <Arduino.h>

const uint8_t row_pins[KB_ROWS] = {KB_OUT1, KB_OUT2, KB_OUT3, KB_OUT4};
const uint8_t col_pins[KB_COLS] = {KB_IN1, KB_IN2, KB_IN3, KB_IN4};

void keyboard_init() {
    for (int i = 0; i < KB_ROWS; i++) {
        pinMode(row_pins[i], OUTPUT);
        digitalWrite(row_pins[i], LOW);
    }
    
    for (int i = 0; i < KB_COLS; i++) {
        pinMode(col_pins[i], INPUT_PULLDOWN);
    }
}

void keyboard_activate_row(uint8_t row) {
    if (row < KB_ROWS) {
        digitalWrite(row_pins[row], HIGH);
    }
}

void keyboard_deactivate_all() {
    for (int i = 0; i < KB_ROWS; i++) {
        digitalWrite(row_pins[i], LOW);
    }
}

uint8_t keyboard_read_cols() {
    uint8_t col_state = 0;
    for (int i = 0; i < KB_COLS; i++) {
        if (digitalRead(col_pins[i]) == HIGH) {
            col_state |= (1 << i);
        }
    }
    return col_state;
}

uint16_t keyboard_scan() {
    uint16_t keys = 0;
    
    for (uint8_t row = 0; row < KB_ROWS; row++) {
        keyboard_deactivate_all();
        keyboard_activate_row(row);
        delayMicroseconds(100);  // Allow signals to settle
        
        uint8_t cols = keyboard_read_cols();
        for (uint8_t col = 0; col < KB_COLS; col++) {
            if (cols & (1 << col)) {
                uint8_t key_num = row * KB_COLS + col;
                keys |= (1 << key_num);
            }
        }
    }
    
    keyboard_deactivate_all();
    return keys;
}
