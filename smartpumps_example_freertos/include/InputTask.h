#ifndef INPUT_TASK_H
#define INPUT_TASK_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern TaskHandle_t inputTaskHandle;

void InputTask(void *pvParameters);

#endif // INPUT_TASK_H
