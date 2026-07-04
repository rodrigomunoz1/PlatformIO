#include "LCD_driver.h"
#include <Arduino.h>
#include <string.h>
#include <SPI.h>

// ST7920 128x64 display connected via software SPI
// U8G2_ST7920_128X64_F_SW_SPI(rotation, clock, data, cs, reset)
// Constructor parameters: rotation, CLK pin, MOSI pin, CS pin, RST pin
static U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, LCD_CLK, LCD_MOSI, LCD_CS, U8X8_PIN_NONE);
static char lcd_lines[4][21] = {0};  // 4 lines, 20 chars max each

void LCD_init() {
    // Initialize SW-SPI pins and backlight pin
    pinMode(LCD_CS, OUTPUT);
    pinMode(LCD_CLK, OUTPUT);
    pinMode(LCD_MOSI, OUTPUT);
    pinMode(LCD_BL, OUTPUT);

    // Set initial states (CS high, clock/data low, backlight off)
    digitalWrite(LCD_CS, HIGH);
    digitalWrite(LCD_CLK, LOW);
    digitalWrite(LCD_MOSI, LOW);
    digitalWrite(LCD_BL, LOW);
    
    // Initialize U8G2 with software SPI
    u8g2.begin();
    
    // Configure display settings
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setFontDirection(0);
    u8g2.setDrawColor(1);
    
    // Clear display
    u8g2.clearBuffer();
    u8g2.sendBuffer();
    
    // Turn on backlight
    digitalWrite(LCD_BL, HIGH);
    delay(100);
    
    // Display initialization message
    u8g2.clearBuffer();
    u8g2.drawStr(0, 10, "ST7920 Ready");
    u8g2.drawStr(0, 26, "LCD Active");
    u8g2.sendBuffer();
    delay(3000);
}

void LCD_clear() {
    u8g2.clearBuffer();
    for (int i = 0; i < 4; i++) {
        memset(lcd_lines[i], 0, sizeof(lcd_lines[i]));
    }
}

void LCD_draw_line(uint8_t line_num, const char* text) {
    if (line_num >= 4) return;
    if (text == NULL) return;
    
    strncpy(lcd_lines[line_num], text, 20);
    lcd_lines[line_num][20] = '\0';
}

void LCD_display() {
    u8g2.clearBuffer();
    
    // Draw 4 lines with 16-pixel vertical spacing
    // Y position: 10 is baseline for first line, then increment by 16 for each line
    for (int i = 0; i < 4; i++) {
        u8g2.drawStr(0, 10 + (i * 16), lcd_lines[i]);
    }
    
    u8g2.sendBuffer();
}

void LCD_backlight_on() {
    digitalWrite(LCD_BL, HIGH);
}

void LCD_backlight_off() {
    digitalWrite(LCD_BL, LOW);
}

// Return pointer to the u8g2 instance for advanced drawing if needed
U8G2_ST7920_128X64_F_SW_SPI* LCD_get_u8g2() {
    return &u8g2;
}

