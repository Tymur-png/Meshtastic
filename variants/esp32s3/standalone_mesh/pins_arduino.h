#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

#define USB_VID 0x303a
#define USB_PID 0x1001

// I2C (reserved for future sensors)
static const uint8_t SDA = 8;
static const uint8_t SCL = 47;

// Default SPI bus is used by the SX1262 LoRa radio
static const uint8_t MISO = 13;
static const uint8_t SCK = 12;
static const uint8_t MOSI = 11;
static const uint8_t SS = 10;

#endif /* Pins_Arduino_h */
