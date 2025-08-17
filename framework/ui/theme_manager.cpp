#include "theme_manager.h"
#include <esp_log.h>

static const char* TAG = "ThemeManager";

ThemeManager::~ThemeManager() {
    shutdown();
}

os_error_t ThemeManager::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Theme Manager");

    // Initialize themes
    initializeLightTheme();
    initializeDarkTheme();

    // Set default theme
    setTheme(ThemeType::LIGHT);

    m_initialized = true;
    ESP_LOGI(TAG, "Theme Manager initialized");

    return OS_OK;
}

os_error_t ThemeManager::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Theme Manager");

    // Cleanup themes would go here if needed
    m_initialized = false;

    ESP_LOGI(TAG, "Theme Manager shutdown complete");
    return OS_OK;
}

os_error_t ThemeManager::update(uint32_t deltaTime) {
    // Auto theme switching logic would go here
    return OS_OK;
}

os_error_t ThemeManager::setTheme(ThemeType theme) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    m_currentTheme = theme;

    // Apply theme to current display
    lv_theme_t* activeTheme = nullptr;
    switch (theme) {
        case ThemeType::LIGHT:
            activeTheme = m_lightTheme;
            break;
        case ThemeType::DARK:
            activeTheme = m_darkTheme;
            break;
        case ThemeType::AUTO:
            // TODO: Determine based on time/sensor
            activeTheme = m_lightTheme;
            break;
    }

    if (activeTheme) {
        lv_disp_set_theme(nullptr, activeTheme);
    }

    ESP_LOGI(TAG, "Set theme to %d", (int)theme);
    return OS_OK;
}

os_error_t ThemeManager::applyTheme(lv_obj_t* obj) {
    // Apply current theme styles to object
    // This would be more complex in a full implementation
    return OS_OK;
}

lv_color_t ThemeManager::getPrimaryColor() const {
    switch (m_currentTheme) {
        case ThemeType::DARK:
            return lv_color_hex(0x3498DB);
        default:
            return lv_color_hex(0x2980B9);
    }
}

lv_color_t ThemeManager::getSecondaryColor() const {
    switch (m_currentTheme) {
        case ThemeType::DARK:
            return lv_color_hex(0x95A5A6);
        default:
            return lv_color_hex(0x7F8C8D);
    }
}

lv_color_t ThemeManager::getBackgroundColor() const {
    switch (m_currentTheme) {
        case ThemeType::DARK:
            return lv_color_hex(0x2C3E50);
        default:
            return lv_color_hex(0xECF0F1);
    }
}

lv_color_t ThemeManager::getTextColor() const {
    switch (m_currentTheme) {
        case ThemeType::DARK:
            return lv_color_hex(0xECF0F1);
        default:
            return lv_color_hex(0x2C3E50);
    }
}

void ThemeManager::initializeLightTheme() {
    m_lightTheme = lv_theme_default_init(nullptr, 
                                        lv_color_hex(0x2980B9),  // Primary
                                        lv_color_hex(0x7F8C8D),  // Secondary
                                        false,                    // Dark mode
                                        lv_font_default());
}

void ThemeManager::initializeDarkTheme() {
    m_darkTheme = lv_theme_default_init(nullptr,
                                       lv_color_hex(0x3498DB),   // Primary
                                       lv_color_hex(0x95A5A6),   // Secondary
                                       true,                     // Dark mode
                                       lv_font_default());
}