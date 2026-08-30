// pins_arduino.h for DX_LR30_ST7789 variant
#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>
#include <variant.h>

#define USB_VID 0x303a
#define USB_PID 0x1001

// Serial
static const uint8_t TX = UART_TX;
static const uint8_t RX = UART_RX;

// Default SPI mapped to Radio
static const uint8_t SS = LORA_CS;
static const uint8_t SCK = LORA_SCK;
static const uint8_t MOSI = LORA_MOSI;
static const uint8_t MISO = LORA_MISO;

// Display control pins (aliases)
static const uint8_t TFT_WR_PIN  = TFT_WR;
static const uint8_t TFT_DC_PIN  = TFT_DC;
static const uint8_t TFT_CS_PIN  = TFT_CS;
static const uint8_t TFT_RST_PIN = TFT_RST;

// I2C (if used)
static const uint8_t SCL = I2C_SCL;
static const uint8_t SDA = I2C_SDA;

#endif /* Pins_Arduino_h */
