#ifndef HAL_MANAGER_H
#define HAL_MANAGER_H

#include "../system/os_config.h"
#include "display_hal.h"
#include "touch_hal.h"
#include "power_hal.h"
#include "storage_hal.h"

/**
 * @file hal_manager.h
 * @brief Hardware Abstraction Layer Manager for M5Stack Tab5
 * 
 * Manages all hardware abstraction components and provides
 * a unified interface to the underlying hardware.
 */

class HALManager {
public:
    HALManager() = default;
    ~HALManager();

    /**
     * @brief Initialize the HAL manager and all hardware components
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown the HAL manager
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update all hardware components
     * @param deltaTime Time elapsed since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Check if HAL is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const { return m_initialized; }

    // Hardware component accessors
    DisplayHAL& getDisplay() { return *m_displayHAL; }
    TouchHAL& getTouch() { return *m_touchHAL; }
    PowerHAL& getPower() { return *m_powerHAL; }
    StorageHAL& getStorage() { return *m_storageHAL; }

    /**
     * @brief Get hardware information string
     * @return Hardware info string
     */
    const char* getHardwareInfo() const;

    /**
     * @brief Perform hardware self-test
     * @return OS_OK if all tests pass, error code otherwise
     */
    os_error_t selfTest();

    /**
     * @brief Get system temperature
     * @return Temperature in Celsius or -999 if unavailable
     */
    float getSystemTemperature() const;

    /**
     * @brief Enable/disable low power mode
     * @param enabled True to enable low power mode
     * @return OS_OK on success, error code on failure
     */
    os_error_t setLowPowerMode(bool enabled);

private:
    /**
     * @brief Initialize hardware components in order
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeComponents();

    // Hardware component instances
    DisplayHAL* m_displayHAL = nullptr;
    TouchHAL* m_touchHAL = nullptr;
    PowerHAL* m_powerHAL = nullptr;
    StorageHAL* m_storageHAL = nullptr;

    bool m_initialized = false;
    bool m_lowPowerMode = false;
    char m_hardwareInfo[128];
};

#endif // HAL_MANAGER_H