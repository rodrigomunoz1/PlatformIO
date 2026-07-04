#include "buzzer_control.h"
#include <Arduino.h>

static uint32_t buzzer_off_time = 0;
static bool buzzer_active = false;

void buzzer_init() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    ledcSetup(BUZZER_CHANNEL, BUZZER_FREQ, 8);  // 8-bit resolution
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
    ledcWrite(BUZZER_CHANNEL, 0);
}

void buzzer_on(uint32_t duration_ms) {
    buzzer_active = true;
    buzzer_off_time = millis() + duration_ms;
    ledcWrite(BUZZER_CHANNEL, (255 * BUZZER_DUTY) / 100);  // Set duty cycle
}

void buzzer_off() {
    buzzer_active = false;
    ledcWrite(BUZZER_CHANNEL, 0);
}

void buzzer_update() {
    if (buzzer_active && millis() >= buzzer_off_time) {
        buzzer_off();
    }
}
