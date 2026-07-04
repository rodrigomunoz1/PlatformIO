#ifndef KEYBOARD_TASK_H
#define KEYBOARD_TASK_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern TaskHandle_t keyboardTaskHandle;

void KeyboardTask(void *pvParameters);

#endif // KEYBOARD_TASK_H
