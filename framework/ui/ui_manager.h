#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "../system/os_config.h"
#include "screen_manager.h"
#include "theme_manager.h"
#include "input_manager.h"
#include <lvgl.h>
#include <map>
#include <string>

/**
 * @file ui_manager.h
 * @brief User Interface Manager for M5Stack Tab5
 * 
 * Manages the overall UI system including screens, themes, inputs,
 * and provides a high-level interface for applications.
 */

class UIManager {
public:
    UIManager() = default;
    ~UIManager();

    /**
     * @brief Initialize the UI manager
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown the UI manager
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update UI system
     * @param deltaTime Time elapsed since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Get screen manager
     * @return Reference to screen manager
     */
    ScreenManager& getScreenManager() { return *m_screenManager; }

    /**
     * @brief Get theme manager
     * @return Reference to theme manager
     */
    ThemeManager& getThemeManager() { return *m_themeManager; }

    /**
     * @brief Get input manager
     * @return Reference to input manager
     */
    InputManager& getInputManager() { return *m_inputManager; }

    /**
     * @brief Create a message box
     * @param title Message box title
     * @param message Message content
     * @param buttons Button configuration (LV_MSGBOX_PART_BTN_*)
     * @return Pointer to created message box
     */
    lv_obj_t* createMessageBox(const char* title, const char* message, 
                              const char* buttons[] = nullptr);

    /**
     * @brief Show notification
     * @param message Notification message
     * @param duration Duration in milliseconds (0 = permanent)
     * @param type Notification type (info, warning, error)
     * @return OS_OK on success, error code on failure
     */
    os_error_t showNotification(const char* message, uint32_t duration = 3000,
                               const char* type = "info");

    /**
     * @brief Hide all notifications
     */
    void hideNotifications();

    /**
     * @brief Set global UI scale factor
     * @param scale Scale factor (0.5 - 2.0)
     * @return OS_OK on success, error code on failure
     */
    os_error_t setUIScale(float scale);

    /**
     * @brief Get current UI scale factor
     * @return Current scale factor
     */
    float getUIScale() const { return m_uiScale; }

    /**
     * @brief Enable/disable animations
     * @param enabled True to enable animations, false to disable
     */
    void setAnimationsEnabled(bool enabled);

    /**
     * @brief Check if animations are enabled
     * @return true if enabled, false if disabled
     */
    bool areAnimationsEnabled() const { return m_animationsEnabled; }

    /**
     * @brief Set UI refresh rate
     * @param fps Frames per second (10-60)
     * @return OS_OK on success, error code on failure
     */
    os_error_t setRefreshRate(uint8_t fps);

    /**
     * @brief Get current refresh rate
     * @return Current FPS
     */
    uint8_t getRefreshRate() const { return m_refreshRate; }

    /**
     * @brief Get UI statistics
     */
    void printStats() const;

    /**
     * @brief Force UI refresh
     * @return OS_OK on success, error code on failure
     */
    os_error_t forceRefresh();

    /**
     * @brief Create loading spinner
     * @param parent Parent object (nullptr for screen)
     * @param message Loading message
     * @return Pointer to loading container
     */
    lv_obj_t* createLoadingSpinner(lv_obj_t* parent = nullptr, 
                                  const char* message = "Loading...");

    /**
     * @brief Hide loading spinner
     * @param spinner Spinner object to hide
     */
    void hideLoadingSpinner(lv_obj_t* spinner);

private:
    /**
     * @brief Initialize LVGL styles and themes
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeStyles();

    /**
     * @brief Create status bar
     * @return OS_OK on success, error code on failure
     */
    os_error_t createStatusBar();

    /**
     * @brief Update status bar information
     */
    void updateStatusBar();

    /**
     * @brief Create dock/navigation bar
     * @return OS_OK on success, error code on failure
     */
    os_error_t createDock();

    // Component managers
    ScreenManager* m_screenManager = nullptr;
    ThemeManager* m_themeManager = nullptr;
    InputManager* m_inputManager = nullptr;

    // UI state
    bool m_initialized = false;
    float m_uiScale = 1.0f;
    bool m_animationsEnabled = true;
    uint8_t m_refreshRate = OS_UI_REFRESH_RATE;

    // UI components
    lv_obj_t* m_statusBar = nullptr;
    lv_obj_t* m_dock = nullptr;
    lv_obj_t* m_notificationContainer = nullptr;

    // Status bar elements
    lv_obj_t* m_timeLabel = nullptr;
    lv_obj_t* m_batteryIcon = nullptr;
    lv_obj_t* m_wifiIcon = nullptr;

    // Statistics
    uint32_t m_frameCount = 0;
    uint32_t m_lastFPSUpdate = 0;
    float m_actualFPS = 0.0f;
    uint32_t m_notificationCount = 0;
};

#endif // UI_MANAGER_H