#ifndef TOUCH_HAL_H
#define TOUCH_HAL_H

#include "../system/os_config.h"
#include <vector>

/**
 * @file touch_hal.h
 * @brief Touch Hardware Abstraction Layer for M5Stack Tab5
 * 
 * Manages the GT911 touch controller with multi-touch support
 * and gesture recognition capabilities.
 */

struct TouchPoint {
    uint16_t x;
    uint16_t y;
    uint8_t pressure;
    uint8_t id;
    bool valid;
    uint32_t timestamp;
    
    TouchPoint() : x(0), y(0), pressure(0), id(0), valid(false), timestamp(0) {}
};

enum class TouchEvent {
    PRESS,
    RELEASE,
    MOVE,
    GESTURE
};

enum class GestureType {
    NONE,
    TAP,
    DOUBLE_TAP,
    LONG_PRESS,
    SWIPE_UP,
    SWIPE_DOWN,
    SWIPE_LEFT,
    SWIPE_RIGHT,
    PINCH_IN,
    PINCH_OUT,
    ROTATE
};

struct TouchEventData {
    TouchEvent event;
    TouchPoint point;
    GestureType gesture;
    uint32_t timestamp;
    
    TouchEventData() : event(TouchEvent::PRESS), gesture(GestureType::NONE), timestamp(0) {}
};

typedef std::function<void(const TouchEventData&)> TouchCallback;

class TouchHAL {
public:
    TouchHAL() = default;
    ~TouchHAL();

    /**
     * @brief Initialize the touch controller
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown the touch controller
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update touch input (read from hardware and process)
     * @param deltaTime Time elapsed since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Perform touch controller self-test
     * @return true if test passes, false otherwise
     */
    bool selfTest();

    /**
     * @brief Set touch callback function
     * @param callback Function to call on touch events
     */
    void setTouchCallback(TouchCallback callback) { m_touchCallback = callback; }

    /**
     * @brief Enable/disable touch input
     * @param enabled True to enable, false to disable
     * @return OS_OK on success, error code on failure
     */
    os_error_t setEnabled(bool enabled);

    /**
     * @brief Check if touch is enabled
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
     * @brief Calibrate touch controller
     * @return OS_OK on success, error code on failure
     */
    os_error_t calibrate();

    /**
     * @brief Get current touch points
     * @return Vector of active touch points
     */
    const std::vector<TouchPoint>& getCurrentTouches() const { return m_currentTouches; }

    /**
     * @brief Get number of active touches
     * @return Number of active touch points
     */
    size_t getActiveTouchCount() const;

    /**
     * @brief Set touch sensitivity
     * @param sensitivity Sensitivity level (0-255)
     * @return OS_OK on success, error code on failure
     */
    os_error_t setSensitivity(uint8_t sensitivity);

    /**
     * @brief Get touch sensitivity
     * @return Current sensitivity level
     */
    uint8_t getSensitivity() const { return m_sensitivity; }

    /**
     * @brief Enable/disable gesture recognition
     * @param enabled True to enable gestures, false to disable
     */
    void setGestureEnabled(bool enabled) { m_gestureEnabled = enabled; }

    /**
     * @brief Check if gesture recognition is enabled
     * @return true if enabled, false if disabled
     */
    bool isGestureEnabled() const { return m_gestureEnabled; }

    /**
     * @brief Get touch statistics
     */
    void printStats() const;

private:
    /**
     * @brief Initialize GT911 touch controller
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeController();

    /**
     * @brief Read touch data from controller
     * @return OS_OK on success, error code on failure
     */
    os_error_t readTouchData();

    /**
     * @brief Process touch events and detect gestures
     */
    void processTouchEvents();

    /**
     * @brief Detect gestures from touch data
     * @return Detected gesture type
     */
    GestureType detectGesture();

    /**
     * @brief Filter touch points (debouncing, smoothing)
     */
    void filterTouchPoints();

    /**
     * @brief Send touch event via callback
     * @param eventData Touch event data to send
     */
    void sendTouchEvent(const TouchEventData& eventData);

    // Hardware configuration
    bool m_initialized = false;
    bool m_enabled = false;
    bool m_lowPowerMode = false;
    uint8_t m_sensitivity = 128;
    bool m_gestureEnabled = true;

    // Touch data
    std::vector<TouchPoint> m_currentTouches;
    std::vector<TouchPoint> m_previousTouches;
    TouchCallback m_touchCallback;

    // Gesture detection
    uint32_t m_lastTouchTime = 0;
    uint32_t m_gestureStartTime = 0;
    TouchPoint m_gestureStartPoint;
    bool m_inGesture = false;

    // Statistics
    uint32_t m_totalTouches = 0;
    uint32_t m_totalGestures = 0;
    uint32_t m_lastCalibration = 0;

    // GT911 specific
    static const uint8_t GT911_I2C_ADDR = 0x5D;
    static const uint16_t GT911_REG_STATUS = 0x814E;
    static const uint16_t GT911_REG_TOUCH = 0x814F;
};

#endif // TOUCH_HAL_H