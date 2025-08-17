#include "input_manager.h"
#include "../system/os_manager.h"
#include <esp_log.h>

static const char* TAG = "InputManager";

// Static members for LVGL callback
bool InputManager::s_touched = false;
uint16_t InputManager::s_touchX = 0;
uint16_t InputManager::s_touchY = 0;

InputManager::~InputManager() {
    shutdown();
}

os_error_t InputManager::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Input Manager");

    // Initialize LVGL input device
    os_error_t result = initializeLVGLInput();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize LVGL input device");
        return result;
    }

    // Subscribe to touch events from HAL
    SUBSCRIBE_EVENT(EVENT_UI_TOUCH_PRESS, 
                   [this](const EventData& event) {
                       if (event.data && event.dataSize >= sizeof(TouchEventData)) {
                           handleTouchEvent(*(TouchEventData*)event.data);
                       }
                   });

    m_initialized = true;
    ESP_LOGI(TAG, "Input Manager initialized");

    return OS_OK;
}

os_error_t InputManager::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Input Manager");

    // Reset touch state
    s_touched = false;
    s_touchX = 0;
    s_touchY = 0;

    m_initialized = false;
    ESP_LOGI(TAG, "Input Manager shutdown complete");

    return OS_OK;
}

os_error_t InputManager::update(uint32_t deltaTime) {
    if (!m_initialized || !m_enabled) {
        return OS_OK;
    }

    // Input processing is handled by LVGL and event callbacks
    return OS_OK;
}

void InputManager::handleTouchEvent(const TouchEventData& eventData) {
    if (!m_initialized || !m_enabled) {
        return;
    }

    switch (eventData.event) {
        case TouchEvent::PRESS:
            s_touched = true;
            s_touchX = eventData.point.x;
            s_touchY = eventData.point.y;
            ESP_LOGD(TAG, "Touch press at (%d, %d)", s_touchX, s_touchY);
            break;

        case TouchEvent::MOVE:
            if (s_touched) {
                s_touchX = eventData.point.x;
                s_touchY = eventData.point.y;
                ESP_LOGD(TAG, "Touch move to (%d, %d)", s_touchX, s_touchY);
            }
            break;

        case TouchEvent::RELEASE:
            s_touched = false;
            ESP_LOGD(TAG, "Touch release");
            break;

        case TouchEvent::GESTURE:
            ESP_LOGD(TAG, "Gesture detected: %d", (int)eventData.gesture);
            // Handle gesture events
            break;
    }
}

os_error_t InputManager::initializeLVGLInput() {
    // Initialize LVGL input device driver
    lv_indev_drv_init(&m_inputDriver);
    m_inputDriver.type = LV_INDEV_TYPE_POINTER;
    m_inputDriver.read_cb = lvglInputRead;

    // Register input device with LVGL
    m_inputDevice = lv_indev_drv_register(&m_inputDriver);
    if (!m_inputDevice) {
        ESP_LOGE(TAG, "Failed to register LVGL input device");
        return OS_ERROR_GENERIC;
    }

    ESP_LOGD(TAG, "LVGL input device initialized");
    return OS_OK;
}

void InputManager::lvglInputRead(lv_indev_drv_t* indev_drv, lv_indev_data_t* data) {
    // Fill LVGL input data structure
    data->point.x = s_touchX;
    data->point.y = s_touchY;
    data->state = s_touched ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}