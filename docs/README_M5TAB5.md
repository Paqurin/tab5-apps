# M5Stack Tab5 Hardware Specifications

## Display
- **Size**: 5-inch IPS TFT
- **Resolution**: 1280 × 720 pixels (HD, 16:9 aspect ratio)
- **Interface**: MIPI-DSI
- **Touch Controller**: GT911 (multi-touch capacitive)

## Processor
- **MCU**: ESP32-P4 (RISC-V dual-core)
- **Clock**: 360MHz
- **Architecture**: RISC-V RV32IMAFC
- **RAM**: 500KB internal SRAM + external PSRAM support
- **Flash**: 16MB

## Connectivity
- WiFi 6 (802.11ax)
- Bluetooth 5.0
- USB-C for programming and power

## M5Stack Tab5 LVGL test os
This operating system framework is specifically designed for the M5Stack Tab5 hardware specifications above, providing:

- Full hardware abstraction layer (HAL) for all components
- Optimized memory management for ESP32-P4 RISC-V architecture
- Native MIPI-DSI display support with 1280×720 resolution
- GT911 multi-touch input handling
- Modular application and service framework