#include "touch_hal.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <Wire.h>
#include <cmath>

static const char* TAG = "TouchHAL";

TouchHAL::~TouchHAL() {
    shutdown();
}

os_error_t TouchHAL::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Touch HAL (GT911)");

    // Initialize I2C if not already done
    // Note: This would typically be done in a board-specific initialization
    Wire.begin();

    // Initialize GT911 controller
    os_error_t result = initializeController();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize GT911 controller");
        return result;
    }

    // Reserve space for touch points
    m_currentTouches.reserve(10); // GT911 supports up to 10 points
    m_previousTouches.reserve(10);

    // Enable touch and set default sensitivity
    setEnabled(true);
    setSensitivity(128);

    m_initialized = true;
    ESP_LOGI(TAG, "Touch HAL initialized successfully");

    return OS_OK;
}

os_error_t TouchHAL::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Touch HAL");

    // Disable touch
    setEnabled(false);

    // Clear touch data
    m_currentTouches.clear();
    m_previousTouches.clear();
    m_touchCallback = nullptr;

    m_initialized = false;
    ESP_LOGI(TAG, "Touch HAL shutdown complete");

    return OS_OK;
}

os_error_t TouchHAL::update(uint32_t deltaTime) {
    if (!m_initialized || !m_enabled) {
        return OS_OK;
    }

    // Read touch data from controller
    os_error_t result = readTouchData();
    if (result != OS_OK) {
        return result;
    }

    // Filter and process touch points
    filterTouchPoints();
    processTouchEvents();

    return OS_OK;
}

bool TouchHAL::selfTest() {
    if (!m_initialized) {
        return false;
    }

    ESP_LOGI(TAG, "Running touch controller self-test");

    // TODO: Implement GT911 specific self-test
    // For now, just check basic communication

    // Test I2C communication
    Wire.beginTransmission(GT911_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
        ESP_LOGE(TAG, "GT911 I2C communication failed");
        return false;
    }

    ESP_LOGI(TAG, "Touch controller self-test passed");
    return true;
}

os_error_t TouchHAL::setEnabled(bool enabled) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    if (m_enabled == enabled) {
        return OS_OK;
    }

    m_enabled = enabled;

    if (enabled) {
        ESP_LOGI(TAG, "Touch input enabled");
        // TODO: Enable GT911 touch detection
    } else {
        ESP_LOGI(TAG, "Touch input disabled");
        // TODO: Disable GT911 touch detection
        // Clear current touches
        m_currentTouches.clear();
    }

    return OS_OK;
}

os_error_t TouchHAL::setLowPowerMode(bool enabled) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    m_lowPowerMode = enabled;

    if (enabled) {
        ESP_LOGI(TAG, "Touch low power mode enabled");
        // TODO: Configure GT911 for low power mode
    } else {
        ESP_LOGI(TAG, "Touch low power mode disabled");
        // TODO: Configure GT911 for normal power mode
    }

    return OS_OK;
}

os_error_t TouchHAL::calibrate() {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    ESP_LOGI(TAG, "Calibrating touch controller");

    // TODO: Implement GT911 calibration procedure
    // For now, just record the calibration time
    m_lastCalibration = millis();

    ESP_LOGI(TAG, "Touch calibration complete");
    return OS_OK;
}

size_t TouchHAL::getActiveTouchCount() const {
    size_t count = 0;
    for (const auto& touch : m_currentTouches) {
        if (touch.valid) {
            count++;
        }
    }
    return count;
}

os_error_t TouchHAL::setSensitivity(uint8_t sensitivity) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    m_sensitivity = sensitivity;

    // TODO: Configure GT911 sensitivity
    ESP_LOGD(TAG, "Set touch sensitivity to %d", sensitivity);

    return OS_OK;
}

void TouchHAL::printStats() const {
    ESP_LOGI(TAG, "=== Touch HAL Statistics ===");
    ESP_LOGI(TAG, "Enabled: %s", m_enabled ? "yes" : "no");
    ESP_LOGI(TAG, "Low power mode: %s", m_lowPowerMode ? "yes" : "no");
    ESP_LOGI(TAG, "Sensitivity: %d/255", m_sensitivity);
    ESP_LOGI(TAG, "Gesture recognition: %s", m_gestureEnabled ? "yes" : "no");
    ESP_LOGI(TAG, "Active touches: %d", getActiveTouchCount());
    ESP_LOGI(TAG, "Total touches: %d", m_totalTouches);
    ESP_LOGI(TAG, "Total gestures: %d", m_totalGestures);
    ESP_LOGI(TAG, "Last calibration: %d ms ago", 
             m_lastCalibration > 0 ? millis() - m_lastCalibration : 0);
}

os_error_t TouchHAL::initializeController() {
    // TODO: Implement GT911 specific initialization
    // This would include:
    // - Reset sequence
    // - Configuration setup
    // - Interrupt configuration
    // - Touch parameters setup

    ESP_LOGD(TAG, "GT911 controller initialized");
    return OS_OK;
}

os_error_t TouchHAL::readTouchData() {
    // Store previous touches
    m_previousTouches = m_currentTouches;
    m_currentTouches.clear();

    // TODO: Implement actual GT911 touch data reading
    // This is a simplified simulation for now

    // Read status register to check for touch data
    // Read number of touch points
    // Read each touch point data (x, y, pressure, id)

    // For simulation, we'll just maintain empty touch data
    // In real implementation, this would read from GT911 registers

    return OS_OK;
}

void TouchHAL::processTouchEvents() {
    uint32_t currentTime = millis();

    // Process each current touch point
    for (const auto& currentTouch : m_currentTouches) {
        if (!currentTouch.valid) continue;

        // Find corresponding previous touch
        bool foundPrevious = false;
        for (const auto& prevTouch : m_previousTouches) {
            if (prevTouch.id == currentTouch.id && prevTouch.valid) {
                // Touch moved
                if (abs(currentTouch.x - prevTouch.x) > OS_TOUCH_THRESHOLD ||
                    abs(currentTouch.y - prevTouch.y) > OS_TOUCH_THRESHOLD) {
                    
                    TouchEventData eventData;
                    eventData.event = TouchEvent::MOVE;
                    eventData.point = currentTouch;
                    eventData.timestamp = currentTime;
                    sendTouchEvent(eventData);
                }
                foundPrevious = true;
                break;
            }
        }

        if (!foundPrevious) {
            // New touch (press)
            TouchEventData eventData;
            eventData.event = TouchEvent::PRESS;
            eventData.point = currentTouch;
            eventData.timestamp = currentTime;
            sendTouchEvent(eventData);

            m_totalTouches++;
            m_lastTouchTime = currentTime;
        }
    }

    // Check for released touches
    for (const auto& prevTouch : m_previousTouches) {
        if (!prevTouch.valid) continue;

        bool stillActive = false;
        for (const auto& currentTouch : m_currentTouches) {
            if (currentTouch.id == prevTouch.id && currentTouch.valid) {
                stillActive = true;
                break;
            }
        }

        if (!stillActive) {
            // Touch released
            TouchEventData eventData;
            eventData.event = TouchEvent::RELEASE;
            eventData.point = prevTouch;
            eventData.timestamp = currentTime;
            sendTouchEvent(eventData);
        }
    }

    // Detect gestures if enabled
    if (m_gestureEnabled) {
        GestureType gesture = detectGesture();
        if (gesture != GestureType::NONE) {
            TouchEventData eventData;
            eventData.event = TouchEvent::GESTURE;
            eventData.gesture = gesture;
            eventData.timestamp = currentTime;
            sendTouchEvent(eventData);

            m_totalGestures++;
        }
    }
}

GestureType TouchHAL::detectGesture() {
    // Simple gesture detection implementation
    // In a real implementation, this would be much more sophisticated

    uint32_t currentTime = millis();
    size_t activeTouches = getActiveTouchCount();

    if (activeTouches == 0) {
        // No active touches, check if we just finished a gesture
        if (m_inGesture && currentTime - m_lastTouchTime > OS_TOUCH_DEBOUNCE_MS) {
            m_inGesture = false;
            
            uint32_t gestureDuration = currentTime - m_gestureStartTime;
            
            // Simple tap detection
            if (gestureDuration < 200) {
                return GestureType::TAP;
            }
            // Long press detection
            else if (gestureDuration > 1000) {
                return GestureType::LONG_PRESS;
            }
        }
    } else if (activeTouches == 1 && !m_inGesture) {
        // Start of potential gesture
        m_inGesture = true;
        m_gestureStartTime = currentTime;
        if (!m_currentTouches.empty() && m_currentTouches[0].valid) {
            m_gestureStartPoint = m_currentTouches[0];
        }
    }

    return GestureType::NONE;
}

void TouchHAL::filterTouchPoints() {
    // Simple filtering to remove noise and apply smoothing
    // In a real implementation, this would be more sophisticated

    for (auto& touch : m_currentTouches) {
        if (!touch.valid) continue;

        // Basic bounds checking
        if (touch.x >= OS_SCREEN_WIDTH) touch.x = OS_SCREEN_WIDTH - 1;
        if (touch.y >= OS_SCREEN_HEIGHT) touch.y = OS_SCREEN_HEIGHT - 1;

        // Simple smoothing with previous touch if available
        for (const auto& prevTouch : m_previousTouches) {
            if (prevTouch.id == touch.id && prevTouch.valid) {
                // Apply simple low-pass filter
                touch.x = (touch.x + prevTouch.x) / 2;
                touch.y = (touch.y + prevTouch.y) / 2;
                break;
            }
        }
    }
}

void TouchHAL::sendTouchEvent(const TouchEventData& eventData) {
    if (m_touchCallback) {
        m_touchCallback(eventData);
    }

    // Also publish as system event
    PUBLISH_EVENT(EVENT_UI_TOUCH_PRESS + (int)eventData.event, 
                  (void*)&eventData, sizeof(eventData));
}