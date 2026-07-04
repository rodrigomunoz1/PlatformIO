#include "KeyboardTask.h"
#include "keyboard_matrix.h"
#include "buzzer_control.h"
#include "shared_buffers.h"

TaskHandle_t keyboardTaskHandle = NULL;

// Debouncing buffer
static uint16_t keyboard_reads[5] = {0};
static uint8_t keyboard_read_idx = 0;

// ==============================================================
// KeyboardTask - High Priority (Priority 2)
// Scans 4x4 keyboard matrix every 20ms with 5-cycle debouncing
// Triggers 200ms buzzer on key press
// ==============================================================
void KeyboardTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(20);
    
    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        
        // Scan keyboard
        uint16_t current_keys = keyboard_scan();
        keyboard_reads[keyboard_read_idx] = current_keys;
        keyboard_read_idx = (keyboard_read_idx + 1) % 5;
        
        // Check if 5 reads are the same (5-cycle debouncing)
        uint8_t match_count = 0;
        for (int i = 0; i < 5; i++) {
            if (keyboard_reads[i] == current_keys) {
                match_count++;
            }
        }
        
        // If all 5 reads match, update the stable keyboard buffer
        if (match_count == 5) {
            static uint16_t last_keys = 0;
            
            // Detect new key presses (transition from 0 to 1)
            uint16_t new_presses = current_keys & ~last_keys;
            
            if (new_presses != 0) {
                // A key was pressed - activate buzzer for 200ms
                buzzer_on(200);
            }
            
            keyboard_buffer_update(current_keys);
            last_keys = current_keys;
        }
        
        // Update buzzer timeout
        buzzer_update();
    }
}
