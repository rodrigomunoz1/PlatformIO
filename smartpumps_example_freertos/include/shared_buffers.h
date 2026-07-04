#ifndef SHARED_BUFFERS_H
#define SHARED_BUFFERS_H

#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Input/Output buffer structure
typedef struct {
    uint16_t active_inputs;   // Bitmask of active inputs (1-16)
    uint16_t active_outputs;  // Bitmask of active outputs (1-16)
    uint16_t active_keys;     // Bitmask of pressed keys (0-15)
    TickType_t last_update;
} BufferData;

// LCD message buffer
typedef struct {
    char message[80];         // User message for LCD line 3
    SemaphoreHandle_t mutex;  // Mutex for thread-safe access
} LCDMessageBuffer;

// Global shared buffers
extern BufferData input_buffer;
extern BufferData output_buffer;
extern BufferData keyboard_buffer;
extern LCDMessageBuffer lcd_message_buffer;

// Synchronization primitives
extern SemaphoreHandle_t input_mutex;
extern SemaphoreHandle_t output_mutex;
extern SemaphoreHandle_t keyboard_mutex;

// Function declarations
void buffers_init();
void input_buffer_update(uint16_t inputs);
void output_buffer_update(uint16_t outputs);
void keyboard_buffer_update(uint16_t keys);
void lcd_message_set(const char* message);
char* lcd_message_get();

#endif // SHARED_BUFFERS_H
