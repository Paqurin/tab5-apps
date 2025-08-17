#ifndef POWER_HAL_H
#define POWER_HAL_H

#include "../system/os_config.h"

/**
 * @file power_hal.h
 * @brief Power Management Hardware Abstraction Layer for M5Stack Tab5
 * 
 * Manages power consumption, battery monitoring, and power states.
 */

// Forward declaration - PowerState is defined in system/power_manager.h
enum class PowerState;

enum class ChargeState {
    UNKNOWN,
    NOT_CHARGING,
    CHARGING,
    CHARGED,
    ERROR
};

struct PowerInfo {
    float batteryVoltage;
    uint8_t batteryLevel;
    ChargeState chargeState;
    float temperature;
    uint32_t uptime;
    PowerState currentState;
};

class PowerHAL {
public:
    PowerHAL() = default;
    ~PowerHAL();

    /**
     * @brief Initialize power management
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown power management
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update power management
     * @param deltaTime Time elapsed since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Perform power system self-test
     * @return true if test passes, false otherwise
     */
    bool selfTest();

    /**
     * @brief Set power state
     * @param state Desired power state
     * @return OS_OK on success, error code on failure
     */
    os_error_t setPowerState(PowerState state);

    /**
     * @brief Get current power state
     * @return Current power state
     */
    PowerState getPowerState() const { return m_currentState; }

    /**
     * @brief Set low power mode
     * @param enabled True to enable low power mode
     * @return OS_OK on success, error code on failure
     */
    os_error_t setLowPowerMode(bool enabled);

    /**
     * @brief Get battery level percentage
     * @return Battery level (0-100)
     */
    uint8_t getBatteryLevel() const { return m_batteryLevel; }

    /**
     * @brief Get battery voltage
     * @return Battery voltage in volts
     */
    float getBatteryVoltage() const { return m_batteryVoltage; }

    /**
     * @brief Get charging state
     * @return Current charging state
     */
    ChargeState getChargeState() const { return m_chargeState; }

    /**
     * @brief Get system temperature
     * @return Temperature in Celsius
     */
    float getTemperature() const { return m_temperature; }

    /**
     * @brief Get power information
     * @return Power info structure
     */
    PowerInfo getPowerInfo() const;

    /**
     * @brief Check if battery is low
     * @return true if battery is low, false otherwise
     */
    bool isBatteryLow() const { return m_batteryLevel < 20; }

    /**
     * @brief Check if battery is critical
     * @return true if battery is critical, false otherwise
     */
    bool isBatteryCritical() const { return m_batteryLevel < 5; }

    /**
     * @brief Enable/disable charging
     * @param enabled True to enable charging, false to disable
     * @return OS_OK on success, error code on failure
     */
    os_error_t setChargingEnabled(bool enabled);

    /**
     * @brief Get power consumption estimate
     * @return Power consumption in milliwatts
     */
    float getPowerConsumption() const { return m_powerConsumption; }

    /**
     * @brief Print power statistics
     */
    void printStats() const;

private:
    /**
     * @brief Read battery status from hardware
     */
    void updateBatteryStatus();

    /**
     * @brief Read temperature from sensors
     */
    void updateTemperature();

    /**
     * @brief Calculate power consumption
     */
    void updatePowerConsumption();

    // Power state
    PowerState m_currentState; // Will be initialized in constructor
    bool m_lowPowerMode = false;
    bool m_chargingEnabled = true;

    // Battery monitoring
    float m_batteryVoltage = 3.7f;
    uint8_t m_batteryLevel = 100;
    ChargeState m_chargeState = ChargeState::UNKNOWN;
    float m_temperature = 25.0f;
    float m_powerConsumption = 0.0f;

    // Statistics
    uint32_t m_lastBatteryUpdate = 0;
    uint32_t m_lastTemperatureUpdate = 0;
    uint32_t m_lowBatteryWarnings = 0;
    uint32_t m_initTime = 0;

    bool m_initialized = false;
};

#endif // POWER_HAL_H