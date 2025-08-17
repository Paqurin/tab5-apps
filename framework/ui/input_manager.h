#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include "../system/os_config.h"
#include "../hal/touch_hal.h"
#include <lvgl.h>

/**
 * @file input_manager.h
 * @brief Input management system for M5Stack Tab5
 * 
 * Manages touch input integration with LVGL and provides
 * high-level input event handling.
 */

class InputManager {
public:
    InputManager() = default;
    ~InputManager();

    /**
     * @brief Initialize input manager
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown input manager
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update input manager
     * @param deltaTime Time elapsed since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Handle touch events from TouchHAL
     * @param eventData Touch event data
     */
    void handleTouchEvent(const TouchEventData& eventData);

    /**
     * @brief Enable/disable input processing
     * @param enabled True to enable, false to disable
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }

    /**
     * @brief Check if input is enabled
     * @return true if enabled, false if disabled
     */
    bool isEnabled() const { return m_enabled; }

private:
    /**
     * @brief Initialize LVGL input device
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeLVGLInput();

    /**
     * @brief LVGL input read callback
     * @param indev_drv Input device driver
     * @param data Input data to fill
     */
    static void lvglInputRead(lv_indev_drv_t* indev_drv, lv_indev_data_t* data);

    // Input state
    bool m_initialized = false;
    bool m_enabled = true;
    
    // LVGL input device
    lv_indev_drv_t m_inputDriver;
    lv_indev_t* m_inputDevice = nullptr;
    
    // Touch state for LVGL
    static bool s_touched;
    static uint16_t s_touchX;
    static uint16_t s_touchY;
};

#endif // INPUT_MANAGER_H