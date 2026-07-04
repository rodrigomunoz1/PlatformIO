#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "TC9555_driver.h"
#include "LCD_driver.h"
#include "buzzer_control.h"
#include "keyboard_matrix.h"
#include "shared_buffers.h"
#include "InputTask.h"
#include "KeyboardTask.h"
#include "UserTask.h"
#include "LCDTask.h"

// ==============================================================
// Setup and Loop
// ==============================================================
void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("Initializing SmartPump FreeRTOS Template...");
    
    // Initialize hardware
    TC9555_init();
    LCD_init();
    buzzer_init();
    keyboard_init();
    buffers_init();
    
    Serial.println("Hardware initialized.");
    
    // Create FreeRTOS tasks
    // InputTask: Highest priority (3)
    xTaskCreatePinnedToCore(
        InputTask,           // Task function
        "InputTask",         // Task name
        2048,                // Stack size
        NULL,                // Parameters
        3,                   // Priority (highest)
        &inputTaskHandle,    // Task handle
        0                    // Core 0
    );
    
    // KeyboardTask: High priority (2)
    xTaskCreatePinnedToCore(
        KeyboardTask,
        "KeyboardTask",
        2048,
        NULL,
        2,                   // Priority
        &keyboardTaskHandle,
        0
    );
    
    // UserTask: Medium priority (1)
    xTaskCreatePinnedToCore(
        UserTask,
        "UserTask",
        2048,
        NULL,
        1,                   // Priority
        &userTaskHandle,
        1                    // Core 1
    );
    
    // LCDTask: Make LCD more responsive (priority 1)
    xTaskCreatePinnedToCore(
        LCDTask,
        "LCDTask",
        2048,
        NULL,
        1,                   // Priority (match UserTask)
        &lcdTaskHandle,
        1                    // Core 1
    );
    
    Serial.println("FreeRTOS tasks created. Scheduler starting...");
}

void loop() {
    // FreeRTOS scheduler handles everything
    vTaskDelay(pdMS_TO_TICKS(1000));
}