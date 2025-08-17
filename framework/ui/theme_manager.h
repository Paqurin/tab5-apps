#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include "../system/os_config.h"
#include <lvgl.h>
#include <string>

/**
 * @file theme_manager.h
 * @brief Theme management system for M5Stack Tab5
 * 
 * Manages UI themes, colors, fonts, and visual styling.
 */

enum class ThemeType {
    LIGHT,
    DARK,
    AUTO  // Automatic based on time or ambient light
};

class ThemeManager {
public:
    ThemeManager() = default;
    ~ThemeManager();

    /**
     * @brief Initialize theme manager
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown theme manager
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update theme manager
     * @param deltaTime Time elapsed since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Set active theme
     * @param theme Theme type to activate
     * @return OS_OK on success, error code on failure
     */
    os_error_t setTheme(ThemeType theme);

    /**
     * @brief Get current theme type
     * @return Current theme type
     */
    ThemeType getCurrentTheme() const { return m_currentTheme; }

    /**
     * @brief Apply theme to an object
     * @param obj LVGL object to apply theme to
     * @return OS_OK on success, error code on failure
     */
    os_error_t applyTheme(lv_obj_t* obj);

    /**
     * @brief Get primary color for current theme
     * @return Primary color
     */
    lv_color_t getPrimaryColor() const;

    /**
     * @brief Get secondary color for current theme
     * @return Secondary color
     */
    lv_color_t getSecondaryColor() const;

    /**
     * @brief Get background color for current theme
     * @return Background color
     */
    lv_color_t getBackgroundColor() const;

    /**
     * @brief Get text color for current theme
     * @return Text color
     */
    lv_color_t getTextColor() const;

private:
    /**
     * @brief Initialize light theme
     */
    void initializeLightTheme();

    /**
     * @brief Initialize dark theme
     */
    void initializeDarkTheme();

    ThemeType m_currentTheme = ThemeType::LIGHT;
    lv_theme_t* m_lightTheme = nullptr;
    lv_theme_t* m_darkTheme = nullptr;
    bool m_initialized = false;
};

#endif // THEME_MANAGER_H