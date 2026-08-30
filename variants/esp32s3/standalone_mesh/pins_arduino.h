#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

#define USB_VID 0x303a
#define USB_PID 0x1001

// I2C is unused (HAS_WIRE 0 / MESHTASTIC_EXCLUDE_I2C). These must not be
// GPIO 8 (LCD WR) or GPIO 47 (LEFT). 3 and 45 are strapping pins — never
// call Wire.begin() with them on this board.
static const uint8_t SDA = 3;
static const uint8_t SCL = 45;

// Default SPI bus matches the SX1262 wiring (not the TFT 8080 bus).
static const uint8_t MISO = 13;
static const uint8_t SCK = 12;
static const uint8_t MOSI = 14;
static const uint8_t SS = 21;

#endif /* Pins_Arduino_h */
