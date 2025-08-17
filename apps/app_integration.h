#ifndef APP_INTEGRATION_H
#define APP_INTEGRATION_H

#include "app_manager.h"
#include "modular_app.h"
#include "calendar_app.h"
#include "enhanced_terminal_app.h"
#include "../system/os_config.h"

/**
 * @file app_integration.h
 * @brief Integration layer for modular app system
 * 
 * Provides factory functions and registration for all modular apps.
 */

class AppIntegration {
public:
    /**
     * @brief Initialize app integration system
     * @param appManager Reference to app manager
     * @return OS_OK on success, error code on failure
     */
    static os_error_t initialize(AppManager& appManager);

    /**
     * @brief Shutdown app integration system
     * @return OS_OK on success, error code on failure
     */
    static os_error_t shutdown();

    /**
     * @brief Register all available apps
     * @param appManager Reference to app manager
     * @return OS_OK on success, error code on failure
     */
    static os_error_t registerAllApps(AppManager& appManager);

    /**
     * @brief Initialize modular app manager
     * @return OS_OK on success, error code on failure
     */
    static os_error_t initializeModularApps();

    /**
     * @brief Get modular app manager instance
     * @return Reference to modular app manager
     */
    static ModularAppManager& getModularAppManager();

    /**
     * @brief Create app store UI
     * @param parent Parent LVGL object
     * @return OS_OK on success, error code on failure
     */
    static os_error_t createAppStoreUI(lv_obj_t* parent);

    /**
     * @brief Destroy app store UI
     * @return OS_OK on success, error code on failure
     */
    static os_error_t destroyAppStoreUI();

private:
    /**
     * @brief Calendar app factory function
     * @return Unique pointer to calendar app instance
     */
    static std::unique_ptr<BaseApp> createCalendarApp();

    /**
     * @brief Enhanced terminal app factory function
     * @return Unique pointer to enhanced terminal app instance
     */
    static std::unique_ptr<BaseApp> createEnhancedTerminalApp();

    /**
     * @brief RS485 terminal app factory function
     * @return Unique pointer to RS485 terminal app instance
     */
    static std::unique_ptr<BaseApp> createRS485TerminalApp();

    /**
     * @brief Camera app factory function
     * @return Unique pointer to camera app instance
     */
    static std::unique_ptr<BaseApp> createCameraApp();

    /**
     * @brief File manager app factory function
     * @return Unique pointer to file manager app instance
     */
    static std::unique_ptr<BaseApp> createFileManagerApp();

    static ModularAppManager s_modularAppManager;
    static AppStoreUI* s_appStoreUI;
    static bool s_initialized;
};

#endif // APP_INTEGRATION_H