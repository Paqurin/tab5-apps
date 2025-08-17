#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include "../system/os_config.h"
#include <lvgl.h>
#include <map>
#include <string>
#include <vector>

/**
 * @file screen_manager.h
 * @brief Screen management system for M5Stack Tab5
 * 
 * Manages screen transitions, history, and provides
 * a framework for creating and managing UI screens.
 */

typedef std::function<void(lv_obj_t*)> ScreenCallback;

enum class ScreenTransition {
    NONE,
    SLIDE_LEFT,
    SLIDE_RIGHT,
    SLIDE_UP,
    SLIDE_DOWN,
    FADE,
    ZOOM_IN,
    ZOOM_OUT
};

struct ScreenInfo {
    std::string name;
    lv_obj_t* screen;
    ScreenCallback createCallback;
    ScreenCallback destroyCallback;
    bool persistent;
    uint32_t createTime;
    uint32_t lastAccess;
};

class ScreenManager {
public:
    ScreenManager() = default;
    ~ScreenManager();

    /**
     * @brief Initialize screen manager
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown screen manager
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update screen manager
     * @param deltaTime Time elapsed since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Register a screen
     * @param name Screen name/identifier
     * @param createCallback Function to create screen content
     * @param destroyCallback Function to cleanup screen (optional)
     * @param persistent True to keep screen in memory
     * @return OS_OK on success, error code on failure
     */
    os_error_t registerScreen(const std::string& name,
                             ScreenCallback createCallback,
                             ScreenCallback destroyCallback = nullptr,
                             bool persistent = false);

    /**
     * @brief Unregister a screen
     * @param name Screen name to unregister
     * @return OS_OK on success, error code on failure
     */
    os_error_t unregisterScreen(const std::string& name);

    /**
     * @brief Switch to a screen
     * @param name Screen name to switch to
     * @param transition Transition animation type
     * @param animationTime Animation duration in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t switchToScreen(const std::string& name,
                             ScreenTransition transition = ScreenTransition::FADE,
                             uint32_t animationTime = OS_UI_ANIMATION_TIME);

    /**
     * @brief Go back to previous screen
     * @param transition Transition animation type
     * @return OS_OK on success, error code on failure
     */
    os_error_t goBack(ScreenTransition transition = ScreenTransition::SLIDE_RIGHT);

    /**
     * @brief Get current screen name
     * @return Current screen name or empty string if none
     */
    std::string getCurrentScreenName() const { return m_currentScreenName; }

    /**
     * @brief Get current screen object
     * @return Pointer to current screen object or nullptr
     */
    lv_obj_t* getCurrentScreen() const;

    /**
     * @brief Check if screen exists
     * @param name Screen name to check
     * @return true if screen exists, false otherwise
     */
    bool hasScreen(const std::string& name) const;

    /**
     * @brief Get list of registered screen names
     * @return Vector of screen names
     */
    std::vector<std::string> getScreenNames() const;

    /**
     * @brief Clear screen history
     */
    void clearHistory();

    /**
     * @brief Get screen history
     * @return Vector of screen names in history order
     */
    std::vector<std::string> getHistory() const { return m_screenHistory; }

    /**
     * @brief Set maximum screens to keep in memory
     * @param maxScreens Maximum number of screens
     */
    void setMaxScreens(size_t maxScreens) { m_maxScreens = maxScreens; }

    /**
     * @brief Get screen statistics
     */
    void printStats() const;

    /**
     * @brief Force cleanup of unused screens
     */
    void cleanupScreens();

private:
    /**
     * @brief Create screen if it doesn't exist
     * @param name Screen name
     * @return Pointer to screen object or nullptr on failure
     */
    lv_obj_t* createScreen(const std::string& name);

    /**
     * @brief Destroy screen and cleanup resources
     * @param name Screen name
     */
    void destroyScreen(const std::string& name);

    /**
     * @brief Apply transition animation
     * @param fromScreen Source screen
     * @param toScreen Target screen
     * @param transition Transition type
     * @param animationTime Animation duration
     */
    void applyTransition(lv_obj_t* fromScreen, lv_obj_t* toScreen,
                        ScreenTransition transition, uint32_t animationTime);

    /**
     * @brief Update screen access time
     * @param name Screen name
     */
    void updateAccessTime(const std::string& name);

    // Screen registry
    std::map<std::string, ScreenInfo> m_screens;
    
    // Navigation state
    std::string m_currentScreenName;
    std::vector<std::string> m_screenHistory;
    
    // Configuration
    size_t m_maxScreens = 5;
    size_t m_maxHistory = 10;
    
    // Statistics
    uint32_t m_totalTransitions = 0;
    uint32_t m_screenCreations = 0;
    uint32_t m_screenDestructions = 0;
    
    bool m_initialized = false;
};

#endif // SCREEN_MANAGER_H