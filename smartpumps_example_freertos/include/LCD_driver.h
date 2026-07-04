#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

#include <U8g2lib.h>

// LCD SPI pins for ST7920 (fixed pinout)
#define LCD_MOSI   23  // MOSI for LCD SPI
#define LCD_CLK    18  // CLK for LCD SPI
#define LCD_CS     5   // CS for LCD SPI
#define LCD_BL     17  // Backlight control (active high)

// LCD dimensions
#define LCD_WIDTH  128
#define LCD_HEIGHT 64
#define LCD_LINES  4

// Function declarations
void LCD_init();
void LCD_clear();
void LCD_draw_line(uint8_t line_num, const char* text);
void LCD_display();
void LCD_backlight_on();
void LCD_backlight_off();

// Get the U8G2 object for direct drawing if needed (software SPI)
U8G2_ST7920_128X64_F_SW_SPI* LCD_get_u8g2();

#endif // LCD_DRIVER_H
