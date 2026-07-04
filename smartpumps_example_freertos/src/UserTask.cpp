#include "UserTask.h"
#include "shared_buffers.h"
#include "TC9555_driver.h"

TaskHandle_t userTaskHandle = NULL;

// ==============================================================
// UserTask - Medium Priority (Priority 1)
// Runs every 100ms
// Manages output control and formats message for LCD
// ==============================================================
void UserTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100);
    
    uint16_t current_outputs = 0;
    
    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        
        // Read current input and keyboard states
        uint16_t current_inputs = input_buffer.active_inputs;
        uint16_t current_keys = keyboard_buffer.active_keys;
        
        // Format message with hex values
        char message[80];
        snprintf(message, sizeof(message), "Hello: I=%04X K=%04X", 
                 current_inputs, current_keys);
        lcd_message_set(message);
        
        // Handle output toggle on key press
        static uint16_t last_keys = 0;
        uint16_t new_presses = current_keys & ~last_keys;
        
        if (new_presses != 0) {
            // Toggle all outputs
            current_outputs = (current_outputs == 0) ? 0xFFFF : 0x0000;
            output_buffer_update(current_outputs);
            TC9555_write_outputs(current_outputs);
        }
        
        last_keys = current_keys;
    }
}
