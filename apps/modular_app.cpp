#include "modular_app.h"
#include "app_manager.h"
#include "../system/os_manager.h"
#include "../services/storage_service.h"
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <cstring>
#include <algorithm>

static const char* TAG = "ModularAppManager";

os_error_t ModularAppManager::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Modular App Manager");

    // Create directories if they don't exist
    // Note: This would require filesystem implementation
    // For now, we'll simulate with memory storage

    // Load existing package registry
    loadPackageRegistry();

    // Initialize with some default available apps
    AppPackage calendarApp = {
        .id = "calendar",
        .name = "Calendar",
        .version = "1.0.0",
        .description = "Calendar and scheduling application",
        .author = "M5Stack Tab5 OS",
        .category = "Productivity",
        .packageSize = 50 * 1024, // 50KB
        .iconPath = "/icons/calendar.png",
        .permissions = {"STORAGE_READ", "STORAGE_WRITE", "NOTIFICATIONS"},
        .dependencies = {},
        .installPath = "",
        .installTime = 0,
        .isInstalled = false,
        .isEnabled = false
    };

    AppPackage terminalApp = {
        .id = "enhanced_terminal",
        .name = "Enhanced Terminal",
        .version = "2.0.0",
        .description = "Terminal with Telnet and SSH support",
        .author = "M5Stack Tab5 OS",
        .category = "System",
        .packageSize = 75 * 1024, // 75KB
        .iconPath = "/icons/terminal.png",
        .permissions = {"NETWORK_ACCESS", "STORAGE_READ", "STORAGE_WRITE"},
        .dependencies = {},
        .installPath = "",
        .installTime = 0,
        .isInstalled = false,
        .isEnabled = false
    };

    m_availableApps.push_back(calendarApp);
    m_availableApps.push_back(terminalApp);

    m_initialized = true;
    ESP_LOGI(TAG, "Modular App Manager initialized");

    return OS_OK;
}

os_error_t ModularAppManager::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Modular App Manager");

    // Save package registry
    savePackageRegistry();

    // Clean up
    m_installedApps.clear();
    m_availableApps.clear();

    m_initialized = false;
    ESP_LOGI(TAG, "Modular App Manager shutdown complete");

    return OS_OK;
}

InstallResult ModularAppManager::installApp(const uint8_t* packageData, size_t packageSize) {
    if (!m_initialized || !packageData || packageSize == 0) {
        return InstallResult::INVALID_PACKAGE;
    }

    // Validate package
    AppPackage packageInfo;
    if (!validatePackage(packageData, packageSize, packageInfo)) {
        ESP_LOGE(TAG, "Invalid package format");
        return InstallResult::INVALID_PACKAGE;
    }

    // Check if already installed
    if (isAppInstalled(packageInfo.id)) {
        ESP_LOGW(TAG, "App '%s' already installed", packageInfo.id.c_str());
        return InstallResult::ALREADY_INSTALLED;
    }

    // Check storage space
    size_t totalSpace, usedSpace, freeSpace;
    getStorageInfo(totalSpace, usedSpace, freeSpace);
    if (freeSpace < packageInfo.packageSize) {
        ESP_LOGE(TAG, "Insufficient storage space for '%s'", packageInfo.id.c_str());
        return InstallResult::INSUFFICIENT_SPACE;
    }

    // Check dependencies
    if (!checkDependencies(packageInfo.id)) {
        ESP_LOGE(TAG, "Missing dependencies for '%s'", packageInfo.id.c_str());
        return InstallResult::DEPENDENCY_MISSING;
    }

    // Extract package
    os_error_t result = extractPackage(packageData, packageSize, packageInfo.id);
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to extract package '%s'", packageInfo.id.c_str());
        return InstallResult::INSTALL_FAILED;
    }

    // Update package info
    packageInfo.installPath = m_installPath + "/" + packageInfo.id;
    packageInfo.installTime = millis();
    packageInfo.isInstalled = true;
    packageInfo.isEnabled = true;

    // Add to installed apps
    m_installedApps.push_back(packageInfo);

    // Register with app manager
    result = registerAppWithManager(packageInfo.id);
    if (result != OS_OK) {
        ESP_LOGW(TAG, "Failed to register app '%s' with manager", packageInfo.id.c_str());
    }

    // Save registry
    savePackageRegistry();

    m_totalInstalls++;
    ESP_LOGI(TAG, "Successfully installed app '%s'", packageInfo.id.c_str());

    return InstallResult::SUCCESS;
}

InstallResult ModularAppManager::installAppFromFile(const std::string& packagePath) {
    ESP_LOGI(TAG, "Installing app from file: %s", packagePath.c_str());
    
    // For simulation, we'll use predefined app data
    if (packagePath.find("calendar") != std::string::npos) {
        // Simulate calendar app installation
        std::string dummyData = "CALENDAR_APP_PACKAGE_DATA";
        return installApp(reinterpret_cast<const uint8_t*>(dummyData.c_str()), dummyData.size());
    } else if (packagePath.find("terminal") != std::string::npos) {
        // Simulate enhanced terminal installation
        std::string dummyData = "TERMINAL_APP_PACKAGE_DATA";
        return installApp(reinterpret_cast<const uint8_t*>(dummyData.c_str()), dummyData.size());
    }
    
    return InstallResult::INVALID_PACKAGE;
}

os_error_t ModularAppManager::uninstallApp(const std::string& appId) {
    if (!m_initialized || appId.empty()) {
        return OS_ERROR_INVALID_PARAM;
    }

    // Find installed app
    auto it = std::find_if(m_installedApps.begin(), m_installedApps.end(),
                          [&appId](const AppPackage& pkg) { return pkg.id == appId; });

    if (it == m_installedApps.end()) {
        ESP_LOGW(TAG, "App '%s' not found for uninstall", appId.c_str());
        return OS_ERROR_NOT_FOUND;
    }

    // Kill app if running
    OS().getAppManager().killApp(appId);

    // Unregister from app manager
    OS().getAppManager().unregisterApp(appId);

    // Remove installation files
    // Note: This would require filesystem implementation
    ESP_LOGI(TAG, "Removing installation files for '%s'", appId.c_str());

    // Remove from installed apps
    m_installedApps.erase(it);

    // Save registry
    savePackageRegistry();

    m_totalUninstalls++;
    ESP_LOGI(TAG, "Successfully uninstalled app '%s'", appId.c_str());

    return OS_OK;
}

os_error_t ModularAppManager::setAppEnabled(const std::string& appId, bool enabled) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    // Find installed app
    auto it = std::find_if(m_installedApps.begin(), m_installedApps.end(),
                          [&appId](AppPackage& pkg) { return pkg.id == appId; });

    if (it == m_installedApps.end()) {
        return OS_ERROR_NOT_FOUND;
    }

    it->isEnabled = enabled;

    if (enabled) {
        // Register with app manager
        registerAppWithManager(appId);
    } else {
        // Kill and unregister
        OS().getAppManager().killApp(appId);
        OS().getAppManager().unregisterApp(appId);
    }

    // Save registry
    savePackageRegistry();

    ESP_LOGI(TAG, "App '%s' %s", appId.c_str(), enabled ? "enabled" : "disabled");
    return OS_OK;
}

std::vector<AppPackage> ModularAppManager::getInstalledApps() const {
    return m_installedApps;
}

std::vector<AppPackage> ModularAppManager::getAvailableApps() const {
    return m_availableApps;
}

const AppPackage* ModularAppManager::getAppPackage(const std::string& appId) const {
    // Check installed apps first
    auto it = std::find_if(m_installedApps.begin(), m_installedApps.end(),
                          [&appId](const AppPackage& pkg) { return pkg.id == appId; });

    if (it != m_installedApps.end()) {
        return &(*it);
    }

    // Check available apps
    it = std::find_if(m_availableApps.begin(), m_availableApps.end(),
                     [&appId](const AppPackage& pkg) { return pkg.id == appId; });

    if (it != m_availableApps.end()) {
        return &(*it);
    }

    return nullptr;
}

bool ModularAppManager::isAppInstalled(const std::string& appId) const {
    return std::any_of(m_installedApps.begin(), m_installedApps.end(),
                      [&appId](const AppPackage& pkg) { return pkg.id == appId; });
}

void ModularAppManager::getStorageInfo(size_t& totalSpace, size_t& usedSpace, size_t& freeSpace) const {
    // Simulate storage info
    totalSpace = 16 * 1024 * 1024; // 16MB total
    usedSpace = 0;
    
    // Calculate used space from installed apps
    for (const auto& app : m_installedApps) {
        usedSpace += app.packageSize;
    }
    
    freeSpace = totalSpace - usedSpace;
}

void ModularAppManager::cleanupCache() {
    ESP_LOGI(TAG, "Cleaning up app cache");
    // Implementation would clean temporary files
}

os_error_t ModularAppManager::updatePackageRegistry() {
    ESP_LOGI(TAG, "Updating package registry");
    // Implementation would fetch latest app list from server
    return OS_OK;
}

bool ModularAppManager::validatePackage(const uint8_t* packageData, size_t packageSize, 
                                        AppPackage& packageInfo) {
    if (!packageData || packageSize < 32) {
        return false;
    }

    // Simple validation - in real implementation would parse actual package format
    std::string data(reinterpret_cast<const char*>(packageData), packageSize);
    
    if (data.find("CALENDAR_APP") != std::string::npos) {
        auto it = std::find_if(m_availableApps.begin(), m_availableApps.end(),
                              [](const AppPackage& pkg) { return pkg.id == "calendar"; });
        if (it != m_availableApps.end()) {
            packageInfo = *it;
            return true;
        }
    } else if (data.find("TERMINAL_APP") != std::string::npos) {
        auto it = std::find_if(m_availableApps.begin(), m_availableApps.end(),
                              [](const AppPackage& pkg) { return pkg.id == "enhanced_terminal"; });
        if (it != m_availableApps.end()) {
            packageInfo = *it;
            return true;
        }
    }

    return false;
}

bool ModularAppManager::checkDependencies(const std::string& appId) const {
    const AppPackage* package = getAppPackage(appId);
    if (!package) {
        return false;
    }

    // Check if all dependencies are installed
    for (const auto& dep : package->dependencies) {
        if (!isAppInstalled(dep)) {
            ESP_LOGW(TAG, "Missing dependency '%s' for app '%s'", dep.c_str(), appId.c_str());
            return false;
        }
    }

    return true;
}

os_error_t ModularAppManager::extractPackage(const uint8_t* packageData, size_t packageSize,
                                            const std::string& appId) {
    ESP_LOGI(TAG, "Extracting package for app '%s'", appId.c_str());
    
    // In real implementation, this would extract files to the installation directory
    // For simulation, we just log the operation
    
    return OS_OK;
}

os_error_t ModularAppManager::registerAppWithManager(const std::string& appId) {
    if (appId == "calendar") {
        // Register calendar app factory
        return OS().getAppManager().registerApp(appId, []() -> std::unique_ptr<BaseApp> {
            // Would return calendar app instance
            return nullptr; // Placeholder for now
        });
    } else if (appId == "enhanced_terminal") {
        // Register enhanced terminal app factory
        return OS().getAppManager().registerApp(appId, []() -> std::unique_ptr<BaseApp> {
            // Would return enhanced terminal app instance
            return nullptr; // Placeholder for now
        });
    }

    return OS_ERROR_NOT_FOUND;
}

os_error_t ModularAppManager::savePackageRegistry() {
    ESP_LOGI(TAG, "Saving package registry");
    // Implementation would save to persistent storage
    return OS_OK;
}

os_error_t ModularAppManager::loadPackageRegistry() {
    ESP_LOGI(TAG, "Loading package registry");
    // Implementation would load from persistent storage
    return OS_OK;
}

// AppStoreUI Implementation

AppStoreUI::AppStoreUI(ModularAppManager& manager) : m_manager(manager) {
}

os_error_t AppStoreUI::createUI(lv_obj_t* parent) {
    if (!parent) {
        return OS_ERROR_INVALID_PARAM;
    }

    // Create main container
    m_container = lv_obj_create(parent);
    lv_obj_set_size(m_container, LV_HOR_RES, LV_VER_RES - 100);
    lv_obj_center(m_container);
    lv_obj_set_style_bg_color(m_container, lv_color_hex(0x1E1E1E), 0);

    // Create tab view
    m_tabview = lv_tabview_create(m_container, LV_DIR_TOP, 50);
    lv_obj_set_size(m_tabview, LV_HOR_RES - 40, LV_VER_RES - 140);
    lv_obj_center(m_tabview);

    // Create tabs
    m_installedTab = lv_tabview_add_tab(m_tabview, "Installed");
    m_availableTab = lv_tabview_add_tab(m_tabview, "Available");

    // Create tab content
    createInstalledAppsTab();
    createAvailableAppsTab();

    // Create status and storage labels
    m_statusLabel = lv_label_create(m_container);
    lv_obj_align(m_statusLabel, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_style_text_color(m_statusLabel, lv_color_white(), 0);

    m_storageLabel = lv_label_create(m_container);
    lv_obj_align(m_storageLabel, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_style_text_color(m_storageLabel, lv_color_white(), 0);

    updateDisplay();

    return OS_OK;
}

os_error_t AppStoreUI::destroyUI() {
    if (m_container) {
        lv_obj_del(m_container);
        m_container = nullptr;
        m_tabview = nullptr;
        m_installedTab = nullptr;
        m_availableTab = nullptr;
        m_installedList = nullptr;
        m_availableList = nullptr;
        m_statusLabel = nullptr;
        m_storageLabel = nullptr;
    }
    return OS_OK;
}

void AppStoreUI::updateDisplay() {
    if (!m_statusLabel || !m_storageLabel) {
        return;
    }

    // Update status
    auto installedApps = m_manager.getInstalledApps();
    lv_label_set_text_fmt(m_statusLabel, "Apps: %d installed", installedApps.size());

    // Update storage info
    size_t totalSpace, usedSpace, freeSpace;
    m_manager.getStorageInfo(totalSpace, usedSpace, freeSpace);
    lv_label_set_text_fmt(m_storageLabel, "Storage: %d KB / %d KB", 
                         usedSpace / 1024, totalSpace / 1024);
}

void AppStoreUI::createInstalledAppsTab() {
    if (!m_installedTab) {
        return;
    }

    // Create list for installed apps
    m_installedList = lv_list_create(m_installedTab);
    lv_obj_set_size(m_installedList, LV_HOR_RES - 80, LV_VER_RES - 200);
    lv_obj_center(m_installedList);

    // Populate with installed apps
    auto installedApps = m_manager.getInstalledApps();
    for (const auto& app : installedApps) {
        createAppListItem(m_installedList, app, true);
    }
}

void AppStoreUI::createAvailableAppsTab() {
    if (!m_availableTab) {
        return;
    }

    // Create list for available apps
    m_availableList = lv_list_create(m_availableTab);
    lv_obj_set_size(m_availableList, LV_HOR_RES - 80, LV_VER_RES - 200);
    lv_obj_center(m_availableList);

    // Populate with available apps
    auto availableApps = m_manager.getAvailableApps();
    for (const auto& app : availableApps) {
        if (!m_manager.isAppInstalled(app.id)) {
            createAppListItem(m_availableList, app, false);
        }
    }
}

lv_obj_t* AppStoreUI::createAppListItem(lv_obj_t* parent, const AppPackage& package, bool isInstalled) {
    lv_obj_t* item = lv_list_add_btn(parent, LV_SYMBOL_SETTINGS, package.name.c_str());
    
    // Store package ID as user data
    lv_obj_set_user_data(item, (void*)package.id.c_str());
    
    if (isInstalled) {
        lv_obj_add_event_cb(item, uninstallButtonCallback, LV_EVENT_CLICKED, this);
    } else {
        lv_obj_add_event_cb(item, installButtonCallback, LV_EVENT_CLICKED, this);
    }
    
    return item;
}

void AppStoreUI::installButtonCallback(lv_event_t* e) {
    AppStoreUI* ui = static_cast<AppStoreUI*>(lv_event_get_user_data(e));
    lv_obj_t* obj = lv_event_get_target(e);
    const char* appId = static_cast<const char*>(lv_obj_get_user_data(obj));
    
    if (ui && appId) {
        ESP_LOGI("AppStoreUI", "Installing app: %s", appId);
        // Simulate installation
        ui->m_manager.installAppFromFile(std::string("/packages/") + appId + ".app");
    }
}

void AppStoreUI::uninstallButtonCallback(lv_event_t* e) {
    AppStoreUI* ui = static_cast<AppStoreUI*>(lv_event_get_user_data(e));
    lv_obj_t* obj = lv_event_get_target(e);
    const char* appId = static_cast<const char*>(lv_obj_get_user_data(obj));
    
    if (ui && appId) {
        ESP_LOGI("AppStoreUI", "Uninstalling app: %s", appId);
        ui->m_manager.uninstallApp(appId);
    }
}

void AppStoreUI::enableButtonCallback(lv_event_t* e) {
    // Implementation for enable/disable toggle
}

void AppStoreUI::refreshButtonCallback(lv_event_t* e) {
    AppStoreUI* ui = static_cast<AppStoreUI*>(lv_event_get_user_data(e));
    if (ui) {
        ui->m_manager.updatePackageRegistry();
        ui->updateDisplay();
    }
}