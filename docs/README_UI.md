# Meshtastic ESP32-S3 UI Prototype

This README explains how to build and test the UI prototype on the hardware.

Requirements
- PlatformIO (VSCode recommended)
- Branch: feature/esp32s3-ui

Build & upload
1. Open project in VSCode with PlatformIO.
2. Select environment: DX_LR30_ST7789 (or the env you added).
3. Build: PlatformIO: Build
4. Upload: PlatformIO: Upload

Example
- Flash examples/hw_ui_demo to test screen, buttons and buzzer.

Notes
- Do NOT power the radio without an antenna.
- Confirm TFT VCC (3.3V vs 5V) before connecting.
