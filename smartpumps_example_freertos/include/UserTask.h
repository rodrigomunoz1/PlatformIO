#ifndef USER_TASK_H
#define USER_TASK_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

extern TaskHandle_t userTaskHandle;

void UserTask(void *pvParameters);

#endif // USER_TASK_H
