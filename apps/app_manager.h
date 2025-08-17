#ifndef APP_MANAGER_H
#define APP_MANAGER_H

#include "../system/os_config.h"
#include "../system/event_system.h"
#include "base_app.h"
#include <memory>
#include <map>
#include <vector>
#include <functional>

/**
 * @file app_manager.h
 * @brief Application Manager for M5Stack Tab5
 * 
 * Manages application lifecycle, installation, and execution.
 * Provides the framework for running multiple applications.
 */

typedef std::function<std::unique_ptr<BaseApp>()> AppFactory;

class AppManager {
public:
    AppManager() = default;
    ~AppManager();

    /**
     * @brief Initialize application manager
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown application manager
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update application manager and running apps
     * @param deltaTime Time elapsed since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Register an application factory
     * @param appId Unique application identifier
     * @param factory Function that creates application instance
     * @return OS_OK on success, error code on failure
     */
    os_error_t registerApp(const std::string& appId, AppFactory factory);

    /**
     * @brief Unregister an application
     * @param appId Application identifier to unregister
     * @return OS_OK on success, error code on failure
     */
    os_error_t unregisterApp(const std::string& appId);

    /**
     * @brief Launch an application
     * @param appId Application identifier to launch
     * @return OS_OK on success, error code on failure
     */
    os_error_t launchApp(const std::string& appId);

    /**
     * @brief Kill a running application
     * @param appId Application identifier to kill
     * @return OS_OK on success, error code on failure
     */
    os_error_t killApp(const std::string& appId);

    /**
     * @brief Pause an application
     * @param appId Application identifier to pause
     * @return OS_OK on success, error code on failure
     */
    os_error_t pauseApp(const std::string& appId);

    /**
     * @brief Resume a paused application
     * @param appId Application identifier to resume
     * @return OS_OK on success, error code on failure
     */
    os_error_t resumeApp(const std::string& appId);

    /**
     * @brief Switch to an application (bring to foreground)
     * @param appId Application identifier to switch to
     * @return OS_OK on success, error code on failure
     */
    os_error_t switchToApp(const std::string& appId);

    /**
     * @brief Get current foreground application
     * @return Pointer to foreground app or nullptr if none
     */
    BaseApp* getCurrentApp() const;

    /**
     * @brief Get application by ID
     * @param appId Application identifier
     * @return Pointer to app or nullptr if not found
     */
    BaseApp* getApp(const std::string& appId) const;

    /**
     * @brief Check if application is running
     * @param appId Application identifier
     * @return true if running, false otherwise
     */
    bool isAppRunning(const std::string& appId) const;

    /**
     * @brief Get list of registered applications
     * @return Vector of application IDs
     */
    std::vector<std::string> getRegisteredApps() const;

    /**
     * @brief Get list of running applications
     * @return Vector of running application IDs
     */
    std::vector<std::string> getRunningApps() const;

    /**
     * @brief Get application information
     * @param appId Application identifier
     * @return Application info or empty info if not found
     */
    AppInfo getAppInfo(const std::string& appId) const;

    /**
     * @brief Kill all running applications
     * @return OS_OK on success, error code on failure
     */
    os_error_t killAllApps();

    /**
     * @brief Set maximum number of concurrent applications
     * @param maxApps Maximum concurrent apps
     */
    void setMaxConcurrentApps(size_t maxApps) { m_maxConcurrentApps = maxApps; }

    /**
     * @brief Get maximum concurrent applications
     * @return Maximum concurrent apps
     */
    size_t getMaxConcurrentApps() const { return m_maxConcurrentApps; }

    /**
     * @brief Get total memory usage of all apps
     * @return Total memory usage in bytes
     */
    size_t getTotalMemoryUsage() const;

    /**
     * @brief Print application statistics
     */
    void printStats() const;

private:
    /**
     * @brief Create application instance
     * @param appId Application identifier
     * @return Pointer to created app or nullptr on failure
     */
    std::unique_ptr<BaseApp> createApp(const std::string& appId);

    /**
     * @brief Cleanup stopped applications
     */
    void cleanupStoppedApps();

    /**
     * @brief Check resource limits and cleanup if needed
     */
    void enforceResourceLimits();

    /**
     * @brief Handle application events
     * @param eventData Event data
     */
    void handleAppEvent(const EventData& eventData);

    // Application registry and instances
    std::map<std::string, AppFactory> m_appFactories;
    std::map<std::string, std::unique_ptr<BaseApp>> m_runningApps;
    
    // State
    std::string m_currentAppId;
    size_t m_maxConcurrentApps = OS_MAX_APPS;
    
    // Statistics
    uint32_t m_totalLaunches = 0;
    uint32_t m_totalKills = 0;
    uint32_t m_lastCleanup = 0;
    
    bool m_initialized = false;
};

#endif // APP_MANAGER_H