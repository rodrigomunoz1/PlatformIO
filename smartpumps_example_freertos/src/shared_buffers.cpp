#include "shared_buffers.h"
#include <string.h>

// Global buffers
BufferData input_buffer = {0};
BufferData output_buffer = {0};
BufferData keyboard_buffer = {0};
LCDMessageBuffer lcd_message_buffer = {0};

// Synchronization primitives
SemaphoreHandle_t input_mutex = NULL;
SemaphoreHandle_t output_mutex = NULL;
SemaphoreHandle_t keyboard_mutex = NULL;

void buffers_init() {
    input_mutex = xSemaphoreCreateMutex();
    output_mutex = xSemaphoreCreateMutex();
    keyboard_mutex = xSemaphoreCreateMutex();
    lcd_message_buffer.mutex = xSemaphoreCreateMutex();
}

void input_buffer_update(uint16_t inputs) {
    if (xSemaphoreTake(input_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        input_buffer.active_inputs = inputs;
        input_buffer.last_update = xTaskGetTickCount();
        xSemaphoreGive(input_mutex);
    }
}

void output_buffer_update(uint16_t outputs) {
    if (xSemaphoreTake(output_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        output_buffer.active_outputs = outputs;
        output_buffer.last_update = xTaskGetTickCount();
        xSemaphoreGive(output_mutex);
    }
}

void keyboard_buffer_update(uint16_t keys) {
    if (xSemaphoreTake(keyboard_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        keyboard_buffer.active_keys = keys;
        keyboard_buffer.last_update = xTaskGetTickCount();
        xSemaphoreGive(keyboard_mutex);
    }
}

void lcd_message_set(const char* message) {
    if (xSemaphoreTake(lcd_message_buffer.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        strncpy(lcd_message_buffer.message, message, sizeof(lcd_message_buffer.message) - 1);
        lcd_message_buffer.message[sizeof(lcd_message_buffer.message) - 1] = '\0';
        xSemaphoreGive(lcd_message_buffer.mutex);
    }
}

char* lcd_message_get() {
    static char temp_buffer[80];
    if (xSemaphoreTake(lcd_message_buffer.mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        strncpy(temp_buffer, lcd_message_buffer.message, sizeof(temp_buffer) - 1);
        xSemaphoreGive(lcd_message_buffer.mutex);
    }
    return temp_buffer;
}
