#include "ui_manager.h"
#include "../system/os_manager.h"
#include <esp_log.h>

static const char* TAG = "UIManager";

UIManager::~UIManager() {
    shutdown();
}

os_error_t UIManager::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing UI Manager");

    // Initialize component managers
    m_screenManager = new ScreenManager();
    if (!m_screenManager || m_screenManager->initialize() != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize Screen Manager");
        return OS_ERROR_GENERIC;
    }

    m_themeManager = new ThemeManager();
    if (!m_themeManager || m_themeManager->initialize() != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize Theme Manager");
        return OS_ERROR_GENERIC;
    }

    m_inputManager = new InputManager();
    if (!m_inputManager || m_inputManager->initialize() != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize Input Manager");
        return OS_ERROR_GENERIC;
    }

    // Initialize LVGL styles and themes
    os_error_t result = initializeStyles();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize styles");
        return result;
    }

    // Create system UI components
    result = createStatusBar();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to create status bar");
        return result;
    }

    result = createDock();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to create dock");
        return result;
    }

    // Create notification container
    m_notificationContainer = lv_obj_create(lv_scr_act());
    lv_obj_set_size(m_notificationContainer, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_opa(m_notificationContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(m_notificationContainer, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(m_notificationContainer, LV_OBJ_FLAG_CLICKABLE);

    m_lastFPSUpdate = millis();
    m_initialized = true;

    ESP_LOGI(TAG, "UI Manager initialized successfully");
    return OS_OK;
}

os_error_t UIManager::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down UI Manager");

    // Clean up notifications
    hideNotifications();

    // Shutdown component managers
    if (m_inputManager) {
        m_inputManager->shutdown();
        delete m_inputManager;
        m_inputManager = nullptr;
    }

    if (m_themeManager) {
        m_themeManager->shutdown();
        delete m_themeManager;
        m_themeManager = nullptr;
    }

    if (m_screenManager) {
        m_screenManager->shutdown();
        delete m_screenManager;
        m_screenManager = nullptr;
    }

    m_initialized = false;
    ESP_LOGI(TAG, "UI Manager shutdown complete");

    return OS_OK;
}

os_error_t UIManager::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    // Update component managers
    if (m_screenManager) {
        m_screenManager->update(deltaTime);
    }

    if (m_themeManager) {
        m_themeManager->update(deltaTime);
    }

    if (m_inputManager) {
        m_inputManager->update(deltaTime);
    }

    // Update status bar
    updateStatusBar();

    // Update FPS statistics
    m_frameCount++;
    uint32_t now = millis();
    if (now - m_lastFPSUpdate >= 1000) {
        m_actualFPS = (float)m_frameCount * 1000.0f / (now - m_lastFPSUpdate);
        m_frameCount = 0;
        m_lastFPSUpdate = now;
    }

    return OS_OK;
}

lv_obj_t* UIManager::createMessageBox(const char* title, const char* message, 
                                     const char* buttons[]) {
    if (!m_initialized) {
        return nullptr;
    }

    // Create message box
    lv_obj_t* msgbox = lv_msgbox_create(lv_scr_act(), title, message, 
                                       buttons ? buttons : (const char*[]){"OK", nullptr}, 
                                       true);
    
    // Center the message box
    lv_obj_center(msgbox);

    ESP_LOGD(TAG, "Created message box: %s", title ? title : "Untitled");
    return msgbox;
}

os_error_t UIManager::showNotification(const char* message, uint32_t duration, 
                                      const char* type) {
    if (!m_initialized || !message) {
        return OS_ERROR_INVALID_PARAM;
    }

    // Create notification container
    lv_obj_t* notification = lv_obj_create(m_notificationContainer);
    lv_obj_set_size(notification, LV_HOR_RES - 40, 60);
    lv_obj_align(notification, LV_ALIGN_TOP_MID, 0, 20 + (m_notificationCount * 70));

    // Set notification style based on type
    if (strcmp(type, "error") == 0) {
        lv_obj_set_style_bg_color(notification, lv_color_hex(0xFF5555), 0);
    } else if (strcmp(type, "warning") == 0) {
        lv_obj_set_style_bg_color(notification, lv_color_hex(0xFFAA00), 0);
    } else {
        lv_obj_set_style_bg_color(notification, lv_color_hex(0x0088FF), 0);
    }

    lv_obj_set_style_bg_opa(notification, LV_OPA_90, 0);
    lv_obj_set_style_radius(notification, 10, 0);

    // Create message label
    lv_obj_t* label = lv_label_create(notification);
    lv_label_set_text(label, message);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_center(label);

    m_notificationCount++;

    // Auto-hide notification if duration is specified
    if (duration > 0) {
        // TODO: Implement auto-hide with timer
        // For now, notifications persist until manually hidden
    }

    ESP_LOGD(TAG, "Showed notification: %s (%s)", message, type);
    return OS_OK;
}

void UIManager::hideNotifications() {
    if (!m_initialized || !m_notificationContainer) {
        return;
    }

    lv_obj_clean(m_notificationContainer);
    m_notificationCount = 0;

    ESP_LOGD(TAG, "Hidden all notifications");
}

os_error_t UIManager::setUIScale(float scale) {
    if (scale < 0.5f || scale > 2.0f) {
        return OS_ERROR_INVALID_PARAM;
    }

    m_uiScale = scale;
    
    // TODO: Apply scale to all UI elements
    ESP_LOGI(TAG, "Set UI scale to %.1f", scale);

    return OS_OK;
}

void UIManager::setAnimationsEnabled(bool enabled) {
    m_animationsEnabled = enabled;
    
    // Configure LVGL animation settings
    if (enabled) {
        // Enable animations with normal speed
    } else {
        // Disable or speed up animations
    }

    ESP_LOGI(TAG, "Animations %s", enabled ? "enabled" : "disabled");
}

os_error_t UIManager::setRefreshRate(uint8_t fps) {
    if (fps < 10 || fps > 60) {
        return OS_ERROR_INVALID_PARAM;
    }

    m_refreshRate = fps;
    
    // TODO: Update LVGL timer period based on FPS
    ESP_LOGI(TAG, "Set refresh rate to %d FPS", fps);

    return OS_OK;
}

void UIManager::printStats() const {
    ESP_LOGI(TAG, "=== UI Manager Statistics ===");
    ESP_LOGI(TAG, "UI Scale: %.1f", m_uiScale);
    ESP_LOGI(TAG, "Animations: %s", m_animationsEnabled ? "enabled" : "disabled");
    ESP_LOGI(TAG, "Target FPS: %d", m_refreshRate);
    ESP_LOGI(TAG, "Actual FPS: %.1f", m_actualFPS);
    ESP_LOGI(TAG, "Active notifications: %d", m_notificationCount);

    if (m_screenManager) {
        m_screenManager->printStats();
    }
}

os_error_t UIManager::forceRefresh() {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(nullptr);

    return OS_OK;
}

lv_obj_t* UIManager::createLoadingSpinner(lv_obj_t* parent, const char* message) {
    if (!m_initialized) {
        return nullptr;
    }

    lv_obj_t* container = lv_obj_create(parent ? parent : lv_scr_act());
    lv_obj_set_size(container, 200, 120);
    lv_obj_center(container);
    lv_obj_set_style_bg_color(container, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(container, LV_OPA_80, 0);
    lv_obj_set_style_radius(container, 10, 0);

    // Create spinner
    lv_obj_t* spinner = lv_spinner_create(container, 1000, 60);
    lv_obj_set_size(spinner, 40, 40);
    lv_obj_align(spinner, LV_ALIGN_CENTER, 0, -20);

    // Create message label
    if (message) {
        lv_obj_t* label = lv_label_create(container);
        lv_label_set_text(label, message);
        lv_obj_set_style_text_color(label, lv_color_white(), 0);
        lv_obj_align(label, LV_ALIGN_CENTER, 0, 20);
    }

    return container;
}

void UIManager::hideLoadingSpinner(lv_obj_t* spinner) {
    if (spinner) {
        lv_obj_del(spinner);
    }
}

os_error_t UIManager::initializeStyles() {
    // TODO: Initialize custom LVGL styles
    // This would include defining colors, fonts, borders, etc.
    
    ESP_LOGD(TAG, "Initialized UI styles");
    return OS_OK;
}

os_error_t UIManager::createStatusBar() {
    // Create status bar container
    m_statusBar = lv_obj_create(lv_scr_act());
    lv_obj_set_size(m_statusBar, LV_HOR_RES, OS_STATUS_BAR_HEIGHT);
    lv_obj_align(m_statusBar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(m_statusBar, lv_color_hex(0x2C3E50), 0);
    lv_obj_set_style_radius(m_statusBar, 0, 0);
    lv_obj_clear_flag(m_statusBar, LV_OBJ_FLAG_SCROLLABLE);

    // Create time label
    m_timeLabel = lv_label_create(m_statusBar);
    lv_label_set_text(m_timeLabel, "12:34");
    lv_obj_set_style_text_color(m_timeLabel, lv_color_white(), 0);
    lv_obj_align(m_timeLabel, LV_ALIGN_LEFT_MID, 10, 0);

    // Create battery icon
    m_batteryIcon = lv_label_create(m_statusBar);
    lv_label_set_text(m_batteryIcon, "100%");
    lv_obj_set_style_text_color(m_batteryIcon, lv_color_white(), 0);
    lv_obj_align(m_batteryIcon, LV_ALIGN_RIGHT_MID, -10, 0);

    // Create WiFi icon
    m_wifiIcon = lv_label_create(m_statusBar);
    lv_label_set_text(m_wifiIcon, "WiFi");
    lv_obj_set_style_text_color(m_wifiIcon, lv_color_white(), 0);
    lv_obj_align(m_wifiIcon, LV_ALIGN_RIGHT_MID, -60, 0);

    ESP_LOGD(TAG, "Created status bar");
    return OS_OK;
}

void UIManager::updateStatusBar() {
    if (!m_timeLabel || !m_batteryIcon || !m_wifiIcon) {
        return;
    }

    // Update time (simplified)
    static uint32_t lastTimeUpdate = 0;
    uint32_t now = millis();
    if (now - lastTimeUpdate >= 1000) {
        // TODO: Get actual time from RTC
        uint32_t seconds = now / 1000;
        uint32_t minutes = (seconds / 60) % 60;
        uint32_t hours = (seconds / 3600) % 24;
        
        char timeStr[16];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", hours, minutes);
        lv_label_set_text(m_timeLabel, timeStr);
        
        lastTimeUpdate = now;
    }

    // Update battery level
    if (OS().getHALManager().isInitialized()) {
        uint8_t batteryLevel = OS().getHALManager().getPower().getBatteryLevel();
        char batteryStr[16];
        snprintf(batteryStr, sizeof(batteryStr), "%d%%", batteryLevel);
        lv_label_set_text(m_batteryIcon, batteryStr);
    }
}

os_error_t UIManager::createDock() {
    // Create dock container
    m_dock = lv_obj_create(lv_scr_act());
    lv_obj_set_size(m_dock, LV_HOR_RES, OS_DOCK_HEIGHT);
    lv_obj_align(m_dock, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(m_dock, lv_color_hex(0x34495E), 0);
    lv_obj_set_style_radius(m_dock, 0, 0);
    lv_obj_clear_flag(m_dock, LV_OBJ_FLAG_SCROLLABLE);

    // TODO: Add dock icons/buttons for common functions

    ESP_LOGD(TAG, "Created dock");
    return OS_OK;
}