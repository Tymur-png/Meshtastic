/*
  variants/esp32s3/DX_LR30_ST7789/variant.h

  Board variant for ESP32-S3 (custom DX-LR30 + ST7789V 320x240 8-bit parallel)
  Pins and defines are based on the provided hardware mapping in the user's spec.

  NOTE: If OK/CHAT (GPIO19/20) interfere with USB/boot, move them to other GPIOs and update this file.
*/

#ifndef _VARIANT_DX_LR30_ST7789_H
#define _VARIANT_DX_LR30_ST7789_H

// --- Display (ST7789V 320x240, 8-bit parallel) --------------------------------
// 8-bit parallel data lines D0..D7
#define TFT_D0 4
#define TFT_D1 5
#define TFT_D2 6
#define TFT_D3 7
#define TFT_D4 15
#define TFT_D5 16
#define TFT_D6 17
#define TFT_D7 18

// Control pins
#define TFT_WR 8    // Write strobe (WR)
#define TFT_DC 9    // Data/Command (RS / D/C)
#define TFT_CS 10   // Chip select
#define TFT_RST 11  // Reset
// RD is tied to 3V3 on the panel (read not used)
#define TFT_RD -1

#define USE_ST7789
#define TFT_WIDTH 320
#define TFT_HEIGHT 240

// --- LoRa (DX-LR30 / SX1262) -----------------------------------------------
#define USE_SX1262

// SPI -> SX1262 (use GPIO matrix on S3)
#define LORA_SCK 12
#define LORA_MISO 13
#define LORA_MOSI 14
#define LORA_CS 21

// SX1262 control pins
#define LORA_RESET 38
#define LORA_BUSY 39
#define LORA_DIO1 40
// DIO2 is used as RF switch on the module (no separate GPIO required)
#define SX126X_DIO2_AS_RF_SWITCH

// Backwards-compatible names used in other variants
#define SX126X_CS     LORA_CS
#define SX126X_DIO1   LORA_DIO1
#define SX126X_BUSY   LORA_BUSY
#define SX126X_RESET  LORA_RESET

// Optional TCXO voltage (if applicable)
//#define SX126X_DIO3_TCXO_VOLTAGE 1.8

// --- Buttons (6-button layout) --------------------------------------------
// Connect buttons between GPIO and GND; use INPUT_PULLUP in software
#define BUTTON_UP     41
#define BUTTON_DOWN   42
#define BUTTON_LEFT   47
#define BUTTON_RIGHT  48
#define BUTTON_OK     19  // NOTE: GPIO19 is USB-related on some modules
#define BUTTON_CHAT   20  // NOTE: GPIO20 is USB-related on some modules
#define BUTTON_NEED_PULLUP

// If you see USB / upload problems after adding buttons on 19/20, move OK/CHAT to other pins.

// --- Buzzer ---------------------------------------------------------------
#define BUZZER_PIN 2

// --- Power / Battery ADC -------------------------------------------------
// Use an ADC pin to measure battery via divider (example: TP4056 powered single-cell Li-ion)
#define BATTERY_PIN 7
#define ADC_CHANNEL ADC_CHANNEL_6
// Voltage divider multiplier (adjust to your hardware)
#define ADC_MULTIPLIER 3.1

// --- Misc -----------------------------------------------------------------
#define HAS_TFT
#define HAS_LORA

#endif // _VARIANT_DX_LR30_ST7789_H
