#ifndef DISPLAY_HAL_H
#define DISPLAY_HAL_H

#include "../system/os_config.h"
#include <lvgl.h>

/**
 * @file display_hal.h
 * @brief Display Hardware Abstraction Layer for M5Stack Tab5
 * 
 * Manages the 5-inch 1280x720 MIPI-DSI display with LVGL integration.
 */

class DisplayHAL {
public:
    DisplayHAL() = default;
    ~DisplayHAL();

    /**
     * @brief Initialize the display hardware
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown the display
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update display (handle LVGL refresh, etc.)
     * @param deltaTime Time elapsed since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Perform display self-test
     * @return true if test passes, false otherwise
     */
    bool selfTest();

    /**
     * @brief Set display brightness
     * @param brightness Brightness level (0-255)
     * @return OS_OK on success, error code on failure
     */
    os_error_t setBrightness(uint8_t brightness);

    /**
     * @brief Get current display brightness
     * @return Current brightness level (0-255)
     */
    uint8_t getBrightness() const { return m_brightness; }

    /**
     * @brief Enable/disable display
     * @param enabled True to enable, false to disable
     * @return OS_OK on success, error code on failure
     */
    os_error_t setEnabled(bool enabled);

    /**
     * @brief Check if display is enabled
     * @return true if enabled, false if disabled
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * @brief Set low power mode
     * @param enabled True to enable low power mode
     * @return OS_OK on success, error code on failure
     */
    os_error_t setLowPowerMode(bool enabled);

    /**
     * @brief Get display width
     * @return Display width in pixels
     */
    uint16_t getWidth() const { return OS_SCREEN_WIDTH; }

    /**
     * @brief Get display height
     * @return Display height in pixels
     */
    uint16_t getHeight() const { return OS_SCREEN_HEIGHT; }

    /**
     * @brief Get LVGL display driver
     * @return Pointer to LVGL display driver
     */
    lv_disp_t* getLVGLDisplay() { return m_lvglDisplay; }

    /**
     * @brief Force display refresh
     * @return OS_OK on success, error code on failure
     */
    os_error_t forceRefresh();

    /**
     * @brief Get frame rate statistics
     * @return Current FPS
     */
    float getFPS() const { return m_fps; }

    /**
     * @brief Get display statistics
     */
    void printStats() const;

private:
    /**
     * @brief LVGL display flush callback
     * @param disp_drv Display driver
     * @param area Area to flush
     * @param color_p Color data
     */
    static void lvglFlushCallback(lv_disp_drv_t* disp_drv, 
                                 const lv_area_t* area, 
                                 lv_color_t* color_p);

    /**
     * @brief Initialize hardware (GPIO, PWM, MIPI-DSI)
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeHardware();

    /**
     * @brief Initialize LVGL display driver
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeLVGL();

    /**
     * @brief Update FPS statistics
     */
    void updateFPS();

    // Hardware configuration
    bool m_initialized = false;
    bool m_enabled = false;
    uint8_t m_brightness = 128;
    bool m_lowPowerMode = false;

    // LVGL components
    lv_disp_t* m_lvglDisplay = nullptr;
    lv_disp_drv_t m_displayDriver;
    lv_disp_draw_buf_t m_drawBuffer;
    lv_color_t* m_buffer1 = nullptr;
    lv_color_t* m_buffer2 = nullptr;

    // Statistics
    uint32_t m_frameCount = 0;
    uint32_t m_lastFPSUpdate = 0;
    float m_fps = 0.0f;
    uint32_t m_totalFlushes = 0;
    uint32_t m_lastRefresh = 0;
};

#endif // DISPLAY_HAL_H