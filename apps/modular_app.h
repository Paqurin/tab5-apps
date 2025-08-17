#ifndef MODULAR_APP_H
#define MODULAR_APP_H

#include "base_app.h"
#include <functional>
#include <vector>
#include <memory>

/**
 * @file modular_app.h
 * @brief Modular Application System for M5Stack Tab5
 * 
 * Provides APK-like modular app functionality with installation, 
 * uninstallation, and dynamic loading capabilities.
 */

struct AppPackage {
    std::string id;
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::string category;
    size_t packageSize;
    std::string iconPath;
    std::vector<std::string> permissions;
    std::vector<std::string> dependencies;
    std::string installPath;
    uint32_t installTime;
    bool isInstalled;
    bool isEnabled;
};

enum class InstallResult {
    SUCCESS,
    ALREADY_INSTALLED,
    INSUFFICIENT_SPACE,
    DEPENDENCY_MISSING,
    PERMISSION_DENIED,
    INVALID_PACKAGE,
    INSTALL_FAILED
};

class ModularAppManager {
public:
    ModularAppManager() = default;
    ~ModularAppManager() = default;

    /**
     * @brief Initialize modular app manager
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown modular app manager
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Install an app package
     * @param packageData Package data buffer
     * @param packageSize Size of package data
     * @return Installation result
     */
    InstallResult installApp(const uint8_t* packageData, size_t packageSize);

    /**
     * @brief Install app from file path
     * @param packagePath Path to package file
     * @return Installation result
     */
    InstallResult installAppFromFile(const std::string& packagePath);

    /**
     * @brief Uninstall an application
     * @param appId Application ID to uninstall
     * @return OS_OK on success, error code on failure
     */
    os_error_t uninstallApp(const std::string& appId);

    /**
     * @brief Enable/disable an installed app
     * @param appId Application ID
     * @param enabled Enable state
     * @return OS_OK on success, error code on failure
     */
    os_error_t setAppEnabled(const std::string& appId, bool enabled);

    /**
     * @brief Get list of installed apps
     * @return Vector of installed app packages
     */
    std::vector<AppPackage> getInstalledApps() const;

    /**
     * @brief Get list of available apps for installation
     * @return Vector of available app packages
     */
    std::vector<AppPackage> getAvailableApps() const;

    /**
     * @brief Get app package information
     * @param appId Application ID
     * @return App package info or nullptr if not found
     */
    const AppPackage* getAppPackage(const std::string& appId) const;

    /**
     * @brief Check if app is installed
     * @param appId Application ID
     * @return true if installed, false otherwise
     */
    bool isAppInstalled(const std::string& appId) const;

    /**
     * @brief Get storage usage statistics
     * @param totalSpace Total available space in bytes
     * @param usedSpace Used space in bytes
     * @param freeSpace Free space in bytes
     */
    void getStorageInfo(size_t& totalSpace, size_t& usedSpace, size_t& freeSpace) const;

    /**
     * @brief Clean up temporary files and cache
     */
    void cleanupCache();

    /**
     * @brief Update app package registry
     * @return OS_OK on success, error code on failure
     */
    os_error_t updatePackageRegistry();

private:
    /**
     * @brief Validate app package
     * @param packageData Package data
     * @param packageSize Package size
     * @param packageInfo Output package info
     * @return true if valid, false otherwise
     */
    bool validatePackage(const uint8_t* packageData, size_t packageSize, 
                        AppPackage& packageInfo);

    /**
     * @brief Check dependencies for app
     * @param appId Application ID
     * @return true if dependencies satisfied, false otherwise
     */
    bool checkDependencies(const std::string& appId) const;

    /**
     * @brief Extract package to installation directory
     * @param packageData Package data
     * @param packageSize Package size
     * @param appId Application ID
     * @return OS_OK on success, error code on failure
     */
    os_error_t extractPackage(const uint8_t* packageData, size_t packageSize,
                             const std::string& appId);

    /**
     * @brief Register app with main app manager
     * @param appId Application ID
     * @return OS_OK on success, error code on failure
     */
    os_error_t registerAppWithManager(const std::string& appId);

    /**
     * @brief Save package registry to storage
     * @return OS_OK on success, error code on failure
     */
    os_error_t savePackageRegistry();

    /**
     * @brief Load package registry from storage
     * @return OS_OK on success, error code on failure
     */
    os_error_t loadPackageRegistry();

    // Package registry
    std::vector<AppPackage> m_installedApps;
    std::vector<AppPackage> m_availableApps;
    
    // Configuration
    std::string m_installPath = "/apps";
    std::string m_cachePath = "/tmp/apps";
    std::string m_registryPath = "/config/app_registry.json";
    
    // Statistics
    size_t m_totalInstalls = 0;
    size_t m_totalUninstalls = 0;
    
    bool m_initialized = false;
};

/**
 * @brief App Store UI for browsing and managing apps
 */
class AppStoreUI {
public:
    AppStoreUI(ModularAppManager& manager);
    ~AppStoreUI() = default;

    /**
     * @brief Create app store UI
     * @param parent Parent LVGL object
     * @return OS_OK on success, error code on failure
     */
    os_error_t createUI(lv_obj_t* parent);

    /**
     * @brief Destroy app store UI
     * @return OS_OK on success, error code on failure
     */
    os_error_t destroyUI();

    /**
     * @brief Update UI display
     */
    void updateDisplay();

private:
    /**
     * @brief Create installed apps tab
     */
    void createInstalledAppsTab();

    /**
     * @brief Create available apps tab
     */
    void createAvailableAppsTab();

    /**
     * @brief Create app list item
     * @param parent Parent object
     * @param package App package info
     * @param isInstalled Whether app is installed
     * @return Created list item object
     */
    lv_obj_t* createAppListItem(lv_obj_t* parent, const AppPackage& package, bool isInstalled);

    // UI event callbacks
    static void installButtonCallback(lv_event_t* e);
    static void uninstallButtonCallback(lv_event_t* e);
    static void enableButtonCallback(lv_event_t* e);
    static void refreshButtonCallback(lv_event_t* e);

    ModularAppManager& m_manager;
    
    // UI elements
    lv_obj_t* m_container = nullptr;
    lv_obj_t* m_tabview = nullptr;
    lv_obj_t* m_installedTab = nullptr;
    lv_obj_t* m_availableTab = nullptr;
    lv_obj_t* m_installedList = nullptr;
    lv_obj_t* m_availableList = nullptr;
    lv_obj_t* m_statusLabel = nullptr;
    lv_obj_t* m_storageLabel = nullptr;
};

#endif // MODULAR_APP_H