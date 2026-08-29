// ============================================================================
// standalone_mesh — autonomous phone-free Meshtastic messenger
// ESP32-S3-DevKitC-1 + DX-LR30-900M22SP (SX1262, 868 MHz) + 2.4" TFT LCD
// Shield (ST7789V 240x320, 8-bit parallel 8080 bus) + 6 buttons + buzzer
// + TP4056 charger + 3.7 V 2000 mAh LiPo on a 100k/100k ADC divider.
//
// The shield (LCD_RST / LCD_CS / LCD_RS / LCD_WR / LCD_RD markings) is an
// 8-bit parallel (8080) bus device — NOT a 4-wire SPI panel. LCD_RD is tied
// to 3V3 (write-only). The SD slot pins (SD_*) are left unconnected.
//
// GPIO budget (checked for no conflicts, all valid on ESP32-S3 DevKitC-1):
//   LCD parallel : data 4,5,6,7,15,16,17,18 / WR 8 / DC(RS) 9 / CS 10 / RST 11
//   LoRa SPI     : SCK 12, MISO 13, MOSI 14, NSS 21, RST 38, BUSY 39, DIO1 40
//   Buttons      : UP 41, DOWN 42, LEFT 47, RIGHT 48, OK 19, CHAT 20
//   Buzzer       : GPIO2
//   Battery ADC  : GPIO1 (ADC1_CH0, 100k/100k divider from the LiPo)
//   CHRG detect  : GPIO46 (optional — only if your TP4056 module breaks CHRG out)
// Flash pins 26-32, octal-PSRAM pins 33-37, UART0 43/44 and straps 0/3/45
// are deliberately unused. Pins 19/20 are buttons, so use the USB-UART port
// (GPIO43/44) for flashing/logging — the native USB stack is not needed.
// ============================================================================

#define STANDALONE_UI 1
// OLED_RU (Russian fonts) comes from the build flags in platformio.ini

// ------------------------ TFT display (ST7789V) -------------------------------
// 2.4" parallel-8080 shield, 240x320. Driven through the parallel branch of
// TFTDisplay (ST7789_PARALLEL). Trigger define + wiring pins:
#define ST7789_PARALLEL 1
#define ST7789_D0 4
#define ST7789_D1 5
#define ST7789_D2 6
#define ST7789_D3 7
#define ST7789_D4 15
#define ST7789_D5 16
#define ST7789_D6 17
#define ST7789_D7 18
#define ST7789_WR 8
#define ST7789_DC 9 // LCD_RS: low = command, high = data
#define ST7789_CS_PIN 10
#define ST7789_RST 11
#define ST7789_RD -1 // tie LCD_RD to 3V3
#define HAS_SPI_TFT 1 // enables the TFT-capable font/UI paths
#define USE_TFTDISPLAY 1
#define TFT_WIDTH 240  // panel is 240x320; SCREEN_ROTATE below flips to 320x240
#define TFT_HEIGHT 320
#define TFT_OFFSET_X 0
#define TFT_OFFSET_Y 0
#define TFT_OFFSET_ROTATION 0
#define SCREEN_ROTATE
#define BRIGHTNESS_DEFAULT 255

// ------------------------------ LoRa radio ----------------------------------
// DX-LR30-900M22SP core module (SX1262 + PA, 22 dBm) on SPI2 (FSPI).
// Spare module pins (DIO2/TXEN/RXEN) may float; the RF switch is wired to
// DIO2 internally on most M22 modules, hence DIO2_AS_RF_SWITCH below.
#define USE_SX1262
#define LORA_SCK 12
#define LORA_MISO 13
#define LORA_MOSI 14
#define LORA_CS 21
#define LORA_RESET 38
#define LORA_DIO1 40
#define LORA_BUSY 39

#define SX126X_CS LORA_CS
#define SX126X_DIO1 LORA_DIO1
#define SX126X_BUSY LORA_BUSY
#define SX126X_RESET LORA_RESET
#define SX126X_DIO2_AS_RF_SWITCH // PA module: DIO2 drives the RF switch
#define SX126X_MAX_POWER 22      // module/EU limit

// ------------------------------ 6 buttons -----------------------------------
// Layout:      [UP]
//        [LEFT] [OK] [RIGHT]
//             [DOWN]
//             [CHAT]
// All buttons are active-low to GND with internal pullups (INPUT_PULLUP).
#define HAS_BUTTON 1
#define STANDALONE_BTN_UP 41
#define STANDALONE_BTN_DOWN 42
#define STANDALONE_BTN_LEFT 47
#define STANDALONE_BTN_RIGHT 48
#define STANDALONE_BTN_OK 19
#define STANDALONE_BTN_CHAT 20

// ------------------------------- Buzzer -------------------------------------
#define PIN_BUZZER 2

// --------------------------- Battery / charger ------------------------------
// 100k/100k divider from the LiPo (4.2V max) to ADC1_CH0 (GPIO1). 2.1 V at
// full charge is comfortably inside the default attenuation window.
#define BATTERY_PIN 1
#define ADC_CHANNEL ADC_CHANNEL_0
#define ADC_MULTIPLIER 2.0f
#define BATTERY_SENSE_RESOLUTION_BITS 12
// Optional TP4056 CHRG sense (open-drain, LOW while charging). Wire it only
// if your TP4056 module actually exposes the CHRG pin; otherwise leave -1.
// #define EXT_CHRG_DETECT 46
// #define EXT_CHRG_DETECT_MODE INPUT_PULLUP

// --------------------------- Backlight (brightness) -------------------------
// The 8080 LCD shield powers its backlight straight from VCC, so the "Яркость
// экрана" setting cannot dim the screen until you wire the TFT LED to a spare
// GPIO through a small NPN/2N2222 (base via 1k to the GPIO, collector to the
// LED-, emitter to GND) or a logic-level MOSFET. When you do, set that GPIO as
// PIN_PWM_BACKLIGHT below and the setting drives it with PWM (0..255).
//
// The only free, boot-safe GPIO on this build is GPIO46 (the optional TP4056
// CHRG line, currently unassigned). If your backlight happens to be wired to a
// different pin, change the value here.
#define PIN_PWM_BACKLIGHT 46

// --------------------------------- Misc -------------------------------------
#define HAS_GPS 0

// Default region for Poland/EU; can be changed on-device in Settings
#define USERPREFS_CONFIG_LORA_REGION meshtastic_Config_LoRaConfig_RegionCode_EU_868
