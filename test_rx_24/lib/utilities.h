/**
 * @file      utilities.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-05-12
 * @last-update 2025-07-07
 * 
 * @note      Cleaned and simplified for T3_S3_V1_2_SX1280 only
 */
#pragma once

// T3 S3 V1.2 with SX1280 Radio
// Product: https://lilygo.cc/products/t3-s3-v1-3

#define T3_S3_V1_2_SX1280
#define USING_SX1280

#define UNUSED_PIN                   (0)

// I2C Configuration
#define I2C_SDA                     18
#define I2C_SCL                     17
#define OLED_RST                    UNUSED_PIN

// Radio SPI Configuration
#define RADIO_SCLK_PIN              5
#define RADIO_MISO_PIN              3
#define RADIO_MOSI_PIN              6
#define RADIO_CS_PIN                7
#define RADIO_RST_PIN               8

// SX1280 Specific Pins
#define RADIO_DIO1_PIN              9       // SX1280 DIO1 = IO9
#define RADIO_BUSY_PIN              36      // SX1280 BUSY = IO36

// SD Card Configuration
#define SDCARD_MOSI                 11
#define SDCARD_MISO                 2
#define SDCARD_SCLK                 14
#define SDCARD_CS                   13

// LED Configuration
#define BOARD_LED                   37
#define LED_ON                      HIGH

// Button and ADC
#define BUTTON_PIN                  0
#define ADC_PIN                     1

// Battery Configuration
#define BAT_ADC_PULLUP_RES          (100000.0)
#define BAT_ADC_PULLDOWN_RES        (100000.0)
#define BAT_MAX_VOLTAGE             (4.2)
#define BAT_VOL_COMPENSATION        (0.0)

// Display Configuration
#define HAS_SDCARD
#define HAS_DISPLAY
#define BOARD_VARIANT_NAME          "T3-S3-V1.2-SX1280"
#define DISPLAY_MODEL_SSD_LIB       SSD1306Wire
#define DISPLAY_MODEL               U8G2_SSD1306_128X64_NONAME_F_HW_I2C
#define DISPLAY_ADDR                0x3C

// Radio Type String
#define RADIO_TYPE_STR              "SX1280"
