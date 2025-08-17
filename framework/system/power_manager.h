#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include "os_config.h"
#include "../hal/hardware_config.h"
#include <esp_sleep.h>
#include <esp_wifi.h>
// Conditional Bluetooth include
#ifdef CONFIG_BT_ENABLED
#include <esp_bt.h>
#endif

/**
 * @file power_manager.h
 * @brief Power Management System for M5Stack Tab5
 * 
 * Handles sleep/wake functionality, power button controls,
 * wireless radio management, and 5V output controls.
 */

enum class PowerState {
    ACTIVE,
    IDLE,
    LIGHT_SLEEP,
    DEEP_SLEEP,
    SHUTDOWN
};

enum class ButtonEvent {
    NONE,
    SHORT_PRESS,
    DOUBLE_PRESS,
    LONG_PRESS
};

enum class WakeupReason {
    UNKNOWN,
    POWER_BUTTON,
    TOUCH_INPUT,
    TIMER,
    EXTERNAL_INTERRUPT
};

class PowerManager {
public:
    PowerManager() = default;
    ~PowerManager();

    /**
     * @brief Initialize power management system
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown power management
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update power management (called from main loop)
     * @param deltaTime Time elapsed since last update
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Enter sleep mode
     * @param sleepType Type of sleep to enter
     * @return OS_OK on success, error code on failure
     */
    os_error_t enterSleep(PowerState sleepType);

    /**
     * @brief Wake up from sleep
     * @return Wake up reason
     */
    WakeupReason wakeUp();

    /**
     * @brief Set sleep timeout
     * @param timeoutMs Timeout in milliseconds
     */
    void setSleepTimeout(uint32_t timeoutMs) { m_sleepTimeout = timeoutMs; }

    /**
     * @brief Get current power state
     * @return Current power state
     */
    PowerState getPowerState() const { return m_currentState; }

    /**
     * @brief Reset activity timer (prevents auto-sleep)
     */
    void resetActivityTimer() { m_lastActivity = millis(); }

    // WiFi and Bluetooth control
    /**
     * @brief Enable/disable WiFi
     * @param enabled True to enable, false to disable
     * @return OS_OK on success, error code on failure
     */
    os_error_t setWiFiEnabled(bool enabled);

    /**
     * @brief Enable/disable Bluetooth
     * @param enabled True to enable, false to disable
     * @return OS_OK on success, error code on failure
     */
    os_error_t setBluetoothEnabled(bool enabled);

    /**
     * @brief Get WiFi enabled status
     * @return true if WiFi is enabled
     */
    bool isWiFiEnabled() const { return m_wifiEnabled; }

    /**
     * @brief Get Bluetooth enabled status
     * @return true if Bluetooth is enabled
     */
    bool isBluetoothEnabled() const { return m_bluetoothEnabled; }

    // 5V output control (LPW5209)
    /**
     * @brief Enable/disable 5V output 1
     * @param enabled True to enable, false to disable
     * @return OS_OK on success, error code on failure
     */
    os_error_t set5VOutput1(bool enabled);

    /**
     * @brief Enable/disable 5V output 2
     * @param enabled True to enable, false to disable
     * @return OS_OK on success, error code on failure
     */
    os_error_t set5VOutput2(bool enabled);

    /**
     * @brief Get 5V output 1 status
     * @return true if output 1 is enabled
     */
    bool is5VOutput1Enabled() const { return m_5vOutput1Enabled; }

    /**
     * @brief Get 5V output 2 status
     * @return true if output 2 is enabled
     */
    bool is5VOutput2Enabled() const { return m_5vOutput2Enabled; }

    /**
     * @brief Check for 5V output faults
     * @return true if any output has a fault
     */
    bool has5VOutputFault() const;

    // Power button handling
    /**
     * @brief Get last button event
     * @return Last detected button event
     */
    ButtonEvent getLastButtonEvent();

    /**
     * @brief Get battery level (if available)
     * @return Battery level percentage (0-100)
     */
    uint8_t getBatteryLevel() const { return m_batteryLevel; }

    /**
     * @brief Get power statistics
     */
    void printPowerStats() const;

private:
    /**
     * @brief Initialize GPIO pins for power management
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeGPIO();

    /**
     * @brief Configure sleep wake up sources
     * @return OS_OK on success, error code on failure
     */
    os_error_t configureSleepWakeup();

    /**
     * @brief Handle power button interrupt
     */
    static void IRAM_ATTR powerButtonISR(void* arg);

    /**
     * @brief Process button events
     */
    void processButtonEvents();

    /**
     * @brief Update battery level
     */
    void updateBatteryLevel();

    /**
     * @brief Check if system should enter sleep
     * @return true if sleep should be entered
     */
    bool shouldEnterSleep() const;

    // State management
    PowerState m_currentState = PowerState::ACTIVE;
    bool m_initialized = false;
    uint32_t m_sleepTimeout = SLEEP_TIMEOUT_MS;
    uint32_t m_lastActivity = 0;

    // Wireless control
    bool m_wifiEnabled = true;
    bool m_bluetoothEnabled = true;

    // 5V output control
    bool m_5vOutput1Enabled = false;
    bool m_5vOutput2Enabled = false;

    // Button handling
    volatile ButtonEvent m_buttonEvent = ButtonEvent::NONE;
    uint32_t m_buttonPressTime = 0;
    uint32_t m_lastButtonPress = 0;
    bool m_buttonPressed = false;

    // Power monitoring
    uint8_t m_batteryLevel = 100;
    uint32_t m_totalSleepTime = 0;
    uint32_t m_wakeupCount = 0;

    // Constants
    static constexpr uint32_t DOUBLE_PRESS_WINDOW = 500;  // ms
    static constexpr uint32_t LONG_PRESS_THRESHOLD = 2000; // ms
    static constexpr uint32_t DEBOUNCE_TIME = 50; // ms
};

#endif // POWER_MANAGER_H