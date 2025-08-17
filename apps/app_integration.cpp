#include "app_integration.h"
#include "rs485_terminal_app.h"
#include "camera_app.h"
#include "file_manager_app.h"
#include "../system/os_manager.h"
#include <esp_log.h>

static const char* TAG = "AppIntegration";

// Static member definitions
ModularAppManager AppIntegration::s_modularAppManager;
AppStoreUI* AppIntegration::s_appStoreUI = nullptr;
bool AppIntegration::s_initialized = false;

os_error_t AppIntegration::initialize(AppManager& appManager) {
    if (s_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing App Integration System");

    // Initialize modular app manager
    os_error_t result = s_modularAppManager.initialize();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize modular app manager");
        return result;
    }

    // Register all available apps
    result = registerAllApps(appManager);
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to register apps");
        return result;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "App Integration System initialized successfully");

    return OS_OK;
}

os_error_t AppIntegration::shutdown() {
    if (!s_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down App Integration System");

    // Destroy app store UI if active
    destroyAppStoreUI();

    // Shutdown modular app manager
    s_modularAppManager.shutdown();

    s_initialized = false;
    ESP_LOGI(TAG, "App Integration System shutdown complete");

    return OS_OK;
}

os_error_t AppIntegration::registerAllApps(AppManager& appManager) {
    ESP_LOGI(TAG, "Registering all available applications");

    os_error_t result;

    // Register Calendar App
    result = appManager.registerApp("calendar", createCalendarApp);
    if (result != OS_OK) {
        ESP_LOGW(TAG, "Failed to register calendar app: %d", result);
    } else {
        ESP_LOGI(TAG, "Registered Calendar App");
    }

    // Register Enhanced Terminal App
    result = appManager.registerApp("enhanced_terminal", createEnhancedTerminalApp);
    if (result != OS_OK) {
        ESP_LOGW(TAG, "Failed to register enhanced terminal app: %d", result);
    } else {
        ESP_LOGI(TAG, "Registered Enhanced Terminal App");
    }

    // Register RS485 Terminal App
    result = appManager.registerApp("rs485_terminal", createRS485TerminalApp);
    if (result != OS_OK) {
        ESP_LOGW(TAG, "Failed to register RS485 terminal app: %d", result);
    } else {
        ESP_LOGI(TAG, "Registered RS485 Terminal App");
    }

    // Register Camera App
    result = appManager.registerApp("camera", createCameraApp);
    if (result != OS_OK) {
        ESP_LOGW(TAG, "Failed to register camera app: %d", result);
    } else {
        ESP_LOGI(TAG, "Registered Camera App");
    }

    // Register File Manager App
    result = appManager.registerApp("file_manager", createFileManagerApp);
    if (result != OS_OK) {
        ESP_LOGW(TAG, "Failed to register file manager app: %d", result);
    } else {
        ESP_LOGI(TAG, "Registered File Manager App");
    }

    ESP_LOGI(TAG, "App registration complete");
    return OS_OK;
}

os_error_t AppIntegration::initializeModularApps() {
    if (!s_initialized) {
        ESP_LOGE(TAG, "App integration not initialized");
        return OS_ERROR_GENERIC;
    }

    // Install available apps automatically for demo
    ESP_LOGI(TAG, "Installing available modular apps");

    // Install Calendar App
    InstallResult result = s_modularAppManager.installAppFromFile("/packages/calendar.app");
    if (result == InstallResult::SUCCESS) {
        ESP_LOGI(TAG, "Calendar app installed successfully");
    } else {
        ESP_LOGW(TAG, "Failed to install calendar app: %d", static_cast<int>(result));
    }

    // Install Enhanced Terminal App
    result = s_modularAppManager.installAppFromFile("/packages/enhanced_terminal.app");
    if (result == InstallResult::SUCCESS) {
        ESP_LOGI(TAG, "Enhanced terminal app installed successfully");
    } else {
        ESP_LOGW(TAG, "Failed to install enhanced terminal app: %d", static_cast<int>(result));
    }

    return OS_OK;
}

ModularAppManager& AppIntegration::getModularAppManager() {
    return s_modularAppManager;
}

os_error_t AppIntegration::createAppStoreUI(lv_obj_t* parent) {
    if (!parent || !s_initialized) {
        return OS_ERROR_INVALID_PARAM;
    }

    if (s_appStoreUI) {
        return OS_OK; // Already created
    }

    s_appStoreUI = new AppStoreUI(s_modularAppManager);
    os_error_t result = s_appStoreUI->createUI(parent);
    
    if (result != OS_OK) {
        delete s_appStoreUI;
        s_appStoreUI = nullptr;
        ESP_LOGE(TAG, "Failed to create app store UI");
        return result;
    }

    ESP_LOGI(TAG, "App Store UI created successfully");
    return OS_OK;
}

os_error_t AppIntegration::destroyAppStoreUI() {
    if (!s_appStoreUI) {
        return OS_OK; // Already destroyed or never created
    }

    s_appStoreUI->destroyUI();
    delete s_appStoreUI;
    s_appStoreUI = nullptr;

    ESP_LOGI(TAG, "App Store UI destroyed");
    return OS_OK;
}

// App Factory Functions

std::unique_ptr<BaseApp> AppIntegration::createCalendarApp() {
    ESP_LOGI(TAG, "Creating Calendar App instance");
    return std::make_unique<CalendarApp>();
}

std::unique_ptr<BaseApp> AppIntegration::createEnhancedTerminalApp() {
    ESP_LOGI(TAG, "Creating Enhanced Terminal App instance");
    return std::make_unique<EnhancedTerminalApp>();
}

std::unique_ptr<BaseApp> AppIntegration::createRS485TerminalApp() {
    ESP_LOGI(TAG, "Creating RS485 Terminal App instance");
    return std::make_unique<RS485TerminalApp>();
}

std::unique_ptr<BaseApp> AppIntegration::createCameraApp() {
    ESP_LOGI(TAG, "Creating Camera App instance");
    // For now, return nullptr as we don't have the camera app implementation
    // In the actual implementation, this would be:
    // return std::make_unique<CameraApp>();
    return nullptr;
}

std::unique_ptr<BaseApp> AppIntegration::createFileManagerApp() {
    ESP_LOGI(TAG, "Creating File Manager App instance");
    // For now, return nullptr as we don't have the file manager app implementation
    // In the actual implementation, this would be:
    // return std::make_unique<FileManagerApp>();
    return nullptr;
}