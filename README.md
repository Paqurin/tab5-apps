# M5Stack Tab5 Application Suite

A comprehensive collection of 14 professional applications for the M5Stack Tab5 ESP32-P4 personal assistant system.

## 🚀 Overview

This repository contains the complete application suite extracted from the M5Stack Tab5 LVGL operating system. These applications provide a full personal assistant and productivity ecosystem designed specifically for the ESP32-P4 RISC-V architecture.

## 📱 Included Applications

### Personal Assistant Apps
- **📞 Contact Management** - Full address book with VCard support
- **✅ Task Management** - Smart to-do lists with priorities and due dates
- **🎤 Voice Recognition** - ChatGPT integration with multi-language support
- **⏰ Alarm & Timer** - Smart alarms, timers, stopwatch, and world clock

### Productivity Suite
- **🧮 Basic Apps Suite** - Calculator, expense tracker, spreadsheet, games
- **📅 Calendar** - Event management with reminders and iCal support
- **📁 File Manager** - Multi-storage support with file operations
- **📷 Camera** - Photo/video capture with QR scanner and gallery

### System & Utility Apps
- **💻 Enhanced Terminal** - RS-485, Telnet, SSH with multiple sessions
- **🔧 RS-485 Terminal** - Industrial Modbus RTU/ASCII communication
- **📱 Modular App System** - APK-style dynamic loading
- **🔗 App Integration** - Inter-app communication framework

### Core Framework
- **🏗️ App Manager** - Application lifecycle management
- **📋 Base App** - Standardized application development framework

## 🛠️ Architecture

### Directory Structure
```
├── apps/                   # Application implementations
│   ├── contact_management_app.*
│   ├── task_management_app.*
│   ├── voice_recognition_app.*
│   ├── basic_apps_suite.*
│   ├── alarm_timer_app.*
│   ├── file_manager_app.*
│   ├── camera_app.*
│   ├── calendar_app.*
│   ├── enhanced_terminal_app.*
│   ├── rs485_terminal_app.*
│   ├── modular_app.*
│   ├── app_integration.*
│   ├── app_manager.*
│   └── base_app.*
├── framework/              # Core framework components
│   ├── hal/               # Hardware abstraction layer
│   ├── ui/                # User interface components
│   ├── system/            # Core system services
│   └── lv_conf.h          # LVGL configuration
├── docs/                  # Documentation
│   ├── APPLICATIONS.md    # Detailed application features
│   ├── README_M5TAB5.md   # Hardware specifications
│   └── MEMORY_OPTIMIZATION.md # Performance optimization
├── platformio.ini         # Build configuration
└── README.md              # This file
```

## 🔧 Hardware Requirements

### M5Stack Tab5 Specifications
- **MCU**: ESP32-P4 (RISC-V dual-core @ 360MHz)
- **Display**: 5-inch IPS TFT, 1280×720 resolution (HD)
- **Touch**: GT911 multi-touch capacitive controller
- **Memory**: 500KB internal SRAM + 32MB PSRAM
- **Flash**: 16MB
- **Connectivity**: WiFi 6 (802.11ax), Bluetooth 5.0

## 🚀 Getting Started

### Prerequisites
```bash
# ESP-IDF RISC-V toolchain (required for ESP32-P4)
export PATH="/path/to/riscv32-esp-elf/bin:$PATH"

# Install PlatformIO
pip install platformio
```

### Building Applications
```bash
# Clone this repository
git clone https://github.com/YOUR_USERNAME/tab5-apps.git
cd tab5-apps

# Build with PlatformIO
pio run -e esp32-p4-evboard

# Upload to device
pio run -e esp32-p4-evboard -t upload
```

## 📊 Features

### 🤖 AI Integration
- **ChatGPT Voice Assistant** - Natural language processing
- **Multi-Language Support** - 6 languages (EN, ES, FR, DE, ZH, JA)
- **Voice Commands** - System control via voice

### 🏗️ Enterprise Architecture
- **Modular Design** - HAL, System, UI, Apps, Services layers
- **Dynamic Loading** - APK-style installation with dependency management
- **Inter-App Communication** - Shared data and service APIs
- **Memory Optimization** - Efficient use of 16MB Flash + 32MB PSRAM

### 🔌 Industrial Connectivity
- **RS-485 Communication** - Modbus RTU/ASCII protocols
- **Terminal Access** - Telnet, SSH with authentication
- **File Transfer** - SCP and SFTP capabilities

## 🎨 User Interface

- **LVGL 8.4 Graphics** - Hardware-accelerated HD display
- **Theme System** - Dark/light mode with customization
- **Touch Interface** - Multi-touch gesture support
- **Responsive Design** - Adaptive layouts for different orientations

## 📈 Development

### Application Framework
All applications inherit from the `BaseApp` framework providing:
- Standardized lifecycle management (create, start, stop, destroy)
- Event handling and inter-app communication
- Resource management and memory optimization
- UI creation and management helpers

### Adding New Applications
1. Inherit from `BaseApp` class
2. Implement required lifecycle methods
3. Register with `AppManager`
4. Handle UI creation and events

## 📝 Documentation

- **[APPLICATIONS.md](docs/APPLICATIONS.md)** - Complete feature descriptions
- **[README_M5TAB5.md](docs/README_M5TAB5.md)** - Hardware specifications
- **[MEMORY_OPTIMIZATION.md](docs/MEMORY_OPTIMIZATION.md)** - Performance optimization

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Implement your application following the `BaseApp` framework
4. Test on actual M5Stack Tab5 hardware
5. Submit a pull request

## 📄 License

This project is provided as-is for educational and development purposes.

## 🔗 Related Projects

- **[M5Stack Tab5 LVGL OS](https://github.com/Paqurin/m5tab5-lvgl)** - Complete operating system
- **[M5Stack Tab5 App Store](https://github.com/Paqurin/m5tab5-lvgl/tree/v5/app-store-server)** - Application distribution server

---

**Total Applications**: 14 comprehensive apps  
**Lines of Code**: 15,000+ LOC  
**Memory Footprint**: Optimized for ESP32-P4 constraints  
**Development Status**: Production-ready with ongoing enhancements