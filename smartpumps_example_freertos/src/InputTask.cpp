#include "InputTask.h"
#include "TC9555_driver.h"
#include "shared_buffers.h"

TaskHandle_t inputTaskHandle = NULL;

// Debouncing buffer
static uint16_t input_reads[4] = {0};
static uint8_t input_read_idx = 0;

// ==============================================================
// InputTask - Highest Priority (Priority 3)
// Reads TC9555_IN inputs every 10ms with 4-cycle debouncing
// ==============================================================
void InputTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(10);
    
    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        
        // Read inputs
        uint16_t current_inputs = TC9555_read_inputs();
        input_reads[input_read_idx] = current_inputs;
        input_read_idx = (input_read_idx + 1) % 4;
        
        // Check if 3 out of 4 reads are the same (debouncing)
        uint8_t match_count = 0;
        for (int i = 0; i < 4; i++) {
            if (input_reads[i] == current_inputs) {
                match_count++;
            }
        }
        
        // If at least 3 reads match, update the stable input buffer
        if (match_count >= 3) {
            input_buffer_update(current_inputs);
        }
    }
}
