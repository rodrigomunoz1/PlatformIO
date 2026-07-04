#ifndef LCD_TASK_H
#define LCD_TASK_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern TaskHandle_t lcdTaskHandle;

void LCDTask(void *pvParameters);

#endif // LCD_TASK_H
