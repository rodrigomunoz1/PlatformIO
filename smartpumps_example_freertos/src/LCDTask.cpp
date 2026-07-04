#include "LCDTask.h"
#include "LCD_driver.h"
#include "shared_buffers.h"
#include <string.h>

TaskHandle_t lcdTaskHandle = NULL;

// ==============================================================
// LCDTask - Lowest Priority (Priority 0)
// Updates display every 200ms with 4 lines:
// Line 0: Active inputs
// Line 1: Active keys
// Line 2: Active outputs
// Line 3: User message
// ==============================================================
void LCDTask(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(100); // faster refresh for snappier UI
    
    // Keep last displayed lines to avoid unnecessary redraws
    static char last_lines[4][21] = {{0}};

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        // Read shared buffers with mutex protection to avoid torn reads
        uint16_t inputs = 0;
        uint16_t keys = 0;
        uint16_t outputs = 0;

        if (xSemaphoreTake(input_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            inputs = input_buffer.active_inputs;
            xSemaphoreGive(input_mutex);
        }
        if (xSemaphoreTake(keyboard_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            keys = keyboard_buffer.active_keys;
            xSemaphoreGive(keyboard_mutex);
        }
        if (xSemaphoreTake(output_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
            outputs = output_buffer.active_outputs;
            xSemaphoreGive(output_mutex);
        }

        char* user_msg = lcd_message_get();
        
        // Format line 0: Active inputs
        char line0[21];
        char input_str[50] = "";
        for (int i = 0; i < 16; i++) {
            if (inputs & (1 << i)) {
                char num_str[4];
                snprintf(num_str, sizeof(num_str), "%d,", i + 1);
                strcat(input_str, num_str);
            }
        }
        if (strlen(input_str) > 0) {
            input_str[strlen(input_str) - 1] = '\0';  // Remove trailing comma
        }
        snprintf(line0, sizeof(line0), "IN: %s", input_str);
        
        // Format line 1: Active keys
        char line1[21];
        char key_str[50] = "";
        for (int i = 0; i < 16; i++) {
            if (keys & (1 << i)) {
                char num_str[4];
                snprintf(num_str, sizeof(num_str), "%d,", i);
                strcat(key_str, num_str);
            }
        }
        if (strlen(key_str) > 0) {
            key_str[strlen(key_str) - 1] = '\0';  // Remove trailing comma
        }
        snprintf(line1, sizeof(line1), "KEY: %s", key_str);
        
        // Format line 2: Active outputs
        char line2[21];
        char output_str[50] = "";
        for (int i = 0; i < 16; i++) {
            if (outputs & (1 << i)) {
                char num_str[4];
                snprintf(num_str, sizeof(num_str), "%d,", i + 1);
                strcat(output_str, num_str);
            }
        }
        if (strlen(output_str) > 0) {
            output_str[strlen(output_str) - 1] = '\0';  // Remove trailing comma
        }
        snprintf(line2, sizeof(line2), "OUT: %s", output_str);
        
        // Only update lines that changed to reduce flicker
        bool changed = false;
        if (strncmp(last_lines[0], line0, sizeof(last_lines[0])) != 0) {
            strncpy(last_lines[0], line0, sizeof(last_lines[0]) - 1);
            last_lines[0][sizeof(last_lines[0]) - 1] = '\0';
            LCD_draw_line(0, line0);
            changed = true;
        }
        if (strncmp(last_lines[1], line1, sizeof(last_lines[1])) != 0) {
            strncpy(last_lines[1], line1, sizeof(last_lines[1]) - 1);
            last_lines[1][sizeof(last_lines[1]) - 1] = '\0';
            LCD_draw_line(1, line1);
            changed = true;
        }
        if (strncmp(last_lines[2], user_msg, sizeof(last_lines[2])) != 0) {
            strncpy(last_lines[2], user_msg, sizeof(last_lines[2]) - 1);
            last_lines[2][sizeof(last_lines[2]) - 1] = '\0';
            LCD_draw_line(2, user_msg);
            changed = true;
        }
        if (strncmp(last_lines[3], line2, sizeof(last_lines[3])) != 0) {
            strncpy(last_lines[3], line2, sizeof(last_lines[3]) - 1);
            last_lines[3][sizeof(last_lines[3]) - 1] = '\0';
            LCD_draw_line(3, line2);
            changed = true;
        }

        if (changed) {
            LCD_display();
        }
    }
}
