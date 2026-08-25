// ============================================================================
// standalone_mesh — autonomous phone-free Meshtastic messenger
// ESP32-S3 + DX-LR30 (SX1262) LoRa + ST7796 SPI TFT + 6 buttons + buzzer
// + TP4056 charger + 3.7 V 2000 mAh LiPo on an ADC divider
//
// All pin numbers below are for a generic ESP32-S3 devkit; adjust to your
// wiring. Avoid GPIO 19/20 (USB), 26-32 (flash), 33-37 (octal PSRAM),
// 43/44 (UART0) and the strapping pins 0/3/45/46.
// ============================================================================

#define STANDALONE_UI 1
#define OLED_RU // Russian (Cyrillic) display fonts

// ----------------------------- TFT display ---------------------------------
// ST7796 320x480 SPI panel on its own SPI host (shares nothing with LoRa)
#define HAS_SPI_TFT 1
#define USE_TFTDISPLAY 1
#define ST7796_CS 39
#define ST7796_RS 42    // DC
#define ST7796_SDA 41   // MOSI
#define ST7796_SCK 40   // SCK
#define ST7796_RESET 38 // panel reset
#define ST7796_MISO -1
#define ST7796_BUSY -1
#define ST7796_BL 2 // backlight, PWM driven
#define ST7796_SPI_HOST SPI3_HOST
#define TFT_BL 2
#define PIN_PWM_BACKLIGHT 2
#define PWM_BACKLIGHT_MAX 255
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 16000000
#define TFT_HEIGHT 480
#define TFT_WIDTH 320
#define TFT_OFFSET_X 0
#define TFT_OFFSET_Y 0
#define TFT_OFFSET_ROTATION 0
#define SCREEN_ROTATE
#define SCREEN_TRANSITION_FRAMERATE 30
#define BRIGHTNESS_DEFAULT 200

// ------------------------------ LoRa radio ----------------------------------
// DX-LR30 module with SX1262, 868 MHz (EU_868)
#define USE_SX1262
#define LORA_SCK 12
#define LORA_MISO 13
#define LORA_MOSI 11
#define LORA_CS 10
#define LORA_RESET 9
#define LORA_DIO1 21
#define LORA_BUSY 14

#define SX126X_CS LORA_CS
#define SX126X_DIO1 LORA_DIO1
#define SX126X_BUSY LORA_BUSY
#define SX126X_RESET LORA_RESET
#define SX126X_DIO2_AS_RF_SWITCH // module switches the RF path via DIO2
#define SX126X_MAX_POWER 22      // keep within the module/EU limits

// ------------------------------ 6 buttons -----------------------------------
// Layout:      [UP]
//        [LEFT] [OK] [RIGHT]
//             [DOWN]
//             [CHAT]
// All buttons are active-low to GND with internal pullups.
#define HAS_BUTTON 1
#define STANDALONE_BTN_UP 4
#define STANDALONE_BTN_DOWN 5
#define STANDALONE_BTN_LEFT 6
#define STANDALONE_BTN_RIGHT 7
#define STANDALONE_BTN_OK 15
#define STANDALONE_BTN_CHAT 16

// ------------------------------- Buzzer -------------------------------------
#define PIN_BUZZER 17

// --------------------------- Battery / charger ------------------------------
// 100k/100k divider from the LiPo to ADC1_CH0 (GPIO1)
#define BATTERY_PIN 1
#define ADC_CHANNEL ADC_CHANNEL_0
#define ADC_MULTIPLIER 2.0f
#define BATTERY_SENSE_RESOLUTION_BITS 12
// TP4056 CHRG pin (open-drain, LOW while charging)
#define EXT_CHRG_DETECT 18
#define EXT_CHRG_DETECT_MODE INPUT_PULLUP

// --------------------------------- Misc -------------------------------------
#define LED_PIN 48
#define HAS_GPS 0
#define I2C_SDA SDA
#define I2C_SCL SCL

// Default region for Poland/EU; can be changed on-device in Settings
#define USERPREFS_CONFIG_LORA_REGION meshtastic_Config_LoRaConfig_RegionCode_EU_868
