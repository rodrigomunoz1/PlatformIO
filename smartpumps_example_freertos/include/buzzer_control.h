#ifndef BUZZER_CONTROL_H
#define BUZZER_CONTROL_H

#include <stdint.h>

// Buzzer configuration
#define BUZZER_PIN     13
#define BUZZER_FREQ    2700  // 2.7kHz
#define BUZZER_DUTY    50    // 50% duty cycle
#define BUZZER_CHANNEL 0     // PWM channel

// Function declarations
void buzzer_init();
void buzzer_on(uint32_t duration_ms);
void buzzer_off();
void buzzer_update();  // Call periodically to handle timeout

#endif // BUZZER_CONTROL_H
