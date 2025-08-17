#include "app_manager.h"
#include "../system/os_manager.h"
#include "../system/event_system.h"
#include <esp_log.h>
#include <algorithm>

static const char* TAG = "AppManager";

AppManager::~AppManager() {
    shutdown();
}

os_error_t AppManager::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Application Manager");

    // Subscribe to application events
    SUBSCRIBE_EVENT(EVENT_APP_LAUNCH, 
                   [this](const EventData& event) { handleAppEvent(event); });
    SUBSCRIBE_EVENT(EVENT_APP_EXIT, 
                   [this](const EventData& event) { handleAppEvent(event); });
    SUBSCRIBE_EVENT(EVENT_APP_ERROR, 
                   [this](const EventData& event) { handleAppEvent(event); });

    m_lastCleanup = millis();
    m_initialized = true;

    ESP_LOGI(TAG, "Application Manager initialized");
    return OS_OK;
}

os_error_t AppManager::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Application Manager");

    // Kill all running applications
    killAllApps();

    // Clear registry
    m_appFactories.clear();
    m_runningApps.clear();
    m_currentAppId.clear();

    m_initialized = false;
    ESP_LOGI(TAG, "Application Manager shutdown complete");

    return OS_OK;
}

os_error_t AppManager::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    // Update all running applications
    for (auto& [appId, app] : m_runningApps) {
        if (app && app->isRunning()) {
            try {
                app->update(deltaTime);
                
                // Check if app requested exit
                if (app->isExitRequested()) {
                    ESP_LOGI(TAG, "Application '%s' requested exit", appId.c_str());
                    killApp(appId);
                }
            } catch (...) {
                ESP_LOGE(TAG, "Exception in app '%s' update", appId.c_str());
                PUBLISH_EVENT(EVENT_APP_ERROR, (void*)appId.c_str(), appId.length());
                killApp(appId);
            }
        }
    }

    // Periodic cleanup
    uint32_t now = millis();
    if (now - m_lastCleanup >= 10000) { // Every 10 seconds
        cleanupStoppedApps();
        enforceResourceLimits();
        m_lastCleanup = now;
    }

    return OS_OK;
}

os_error_t AppManager::registerApp(const std::string& appId, AppFactory factory) {
    if (!m_initialized || appId.empty() || !factory) {
        return OS_ERROR_INVALID_PARAM;
    }

    if (m_appFactories.find(appId) != m_appFactories.end()) {
        ESP_LOGW(TAG, "Application '%s' already registered", appId.c_str());
        return OS_ERROR_GENERIC;
    }

    m_appFactories[appId] = factory;
    ESP_LOGI(TAG, "Registered application '%s'", appId.c_str());

    return OS_OK;
}

os_error_t AppManager::unregisterApp(const std::string& appId) {
    if (!m_initialized || appId.empty()) {
        return OS_ERROR_INVALID_PARAM;
    }

    // Kill app if running
    killApp(appId);

    // Remove from registry
    auto it = m_appFactories.find(appId);
    if (it != m_appFactories.end()) {
        m_appFactories.erase(it);
        ESP_LOGI(TAG, "Unregistered application '%s'", appId.c_str());
        return OS_OK;
    }

    return OS_ERROR_NOT_FOUND;
}

os_error_t AppManager::launchApp(const std::string& appId) {
    if (!m_initialized || appId.empty()) {
        return OS_ERROR_INVALID_PARAM;
    }

    // Check if app is already running
    if (isAppRunning(appId)) {
        ESP_LOGW(TAG, "Application '%s' is already running", appId.c_str());
        return switchToApp(appId);
    }

    // Check resource limits
    if (m_runningApps.size() >= m_maxConcurrentApps) {
        ESP_LOGW(TAG, "Maximum concurrent apps reached (%d)", m_maxConcurrentApps);
        return OS_ERROR_BUSY;
    }

    // Create application instance
    auto app = createApp(appId);
    if (!app) {
        ESP_LOGE(TAG, "Failed to create application '%s'", appId.c_str());
        return OS_ERROR_GENERIC;
    }

    // Initialize the application
    os_error_t result = app->initialize();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize application '%s': %d", appId.c_str(), result);
        return result;
    }

    // Start the application
    result = app->start();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to start application '%s': %d", appId.c_str(), result);
        app->shutdown();
        return result;
    }

    // Add to running apps
    m_runningApps[appId] = std::move(app);
    m_totalLaunches++;

    ESP_LOGI(TAG, "Launched application '%s'", appId.c_str());

    // Switch to the new app
    return switchToApp(appId);
}

os_error_t AppManager::killApp(const std::string& appId) {
    if (!m_initialized || appId.empty()) {
        return OS_ERROR_INVALID_PARAM;
    }

    auto it = m_runningApps.find(appId);
    if (it == m_runningApps.end()) {
        return OS_ERROR_NOT_FOUND;
    }

    auto& app = it->second;
    if (app) {
        // Stop the application
        app->stop();
        app->shutdown();
    }

    // Remove from running apps
    m_runningApps.erase(it);
    m_totalKills++;

    // Switch to another app if this was the current one
    if (m_currentAppId == appId) {
        m_currentAppId.clear();
        
        // Find another running app to switch to
        if (!m_runningApps.empty()) {
            switchToApp(m_runningApps.begin()->first);
        }
    }

    ESP_LOGI(TAG, "Killed application '%s'", appId.c_str());
    return OS_OK;
}

os_error_t AppManager::pauseApp(const std::string& appId) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    BaseApp* app = getApp(appId);
    if (!app) {
        return OS_ERROR_NOT_FOUND;
    }

    return app->pause();
}

os_error_t AppManager::resumeApp(const std::string& appId) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    BaseApp* app = getApp(appId);
    if (!app) {
        return OS_ERROR_NOT_FOUND;
    }

    return app->resume();
}

os_error_t AppManager::switchToApp(const std::string& appId) {
    if (!m_initialized || appId.empty()) {
        return OS_ERROR_INVALID_PARAM;
    }

    BaseApp* app = getApp(appId);
    if (!app || !app->isRunning()) {
        return OS_ERROR_NOT_FOUND;
    }

    if (m_currentAppId == appId) {
        return OS_OK; // Already current
    }

    // Pause current app if any
    if (!m_currentAppId.empty()) {
        BaseApp* currentApp = getApp(m_currentAppId);
        if (currentApp && currentApp->isRunning()) {
            currentApp->pause();
        }
    }

    // Switch to new app
    m_currentAppId = appId;
    
    // Resume the app if it was paused
    if (app->isPaused()) {
        app->resume();
    }

    // TODO: Switch UI to this app's screen
    // This would typically involve switching LVGL screens

    ESP_LOGI(TAG, "Switched to application '%s'", appId.c_str());
    return OS_OK;
}

BaseApp* AppManager::getCurrentApp() const {
    return getApp(m_currentAppId);
}

BaseApp* AppManager::getApp(const std::string& appId) const {
    auto it = m_runningApps.find(appId);
    return (it != m_runningApps.end()) ? it->second.get() : nullptr;
}

bool AppManager::isAppRunning(const std::string& appId) const {
    BaseApp* app = getApp(appId);
    return app && app->isRunning();
}

std::vector<std::string> AppManager::getRegisteredApps() const {
    std::vector<std::string> apps;
    for (const auto& [appId, factory] : m_appFactories) {
        apps.push_back(appId);
    }
    return apps;
}

std::vector<std::string> AppManager::getRunningApps() const {
    std::vector<std::string> apps;
    for (const auto& [appId, app] : m_runningApps) {
        if (app && app->isRunning()) {
            apps.push_back(appId);
        }
    }
    return apps;
}

AppInfo AppManager::getAppInfo(const std::string& appId) const {
    BaseApp* app = getApp(appId);
    if (app) {
        return app->getAppInfo();
    }
    
    // Return empty info if not found
    AppInfo info;
    info.id = appId;
    info.state = AppState::STOPPED;
    return info;
}

os_error_t AppManager::killAllApps() {
    ESP_LOGI(TAG, "Killing all applications");

    // Create a copy of app IDs to avoid iterator invalidation
    std::vector<std::string> appIds;
    for (const auto& [appId, app] : m_runningApps) {
        appIds.push_back(appId);
    }

    // Kill each app
    for (const auto& appId : appIds) {
        killApp(appId);
    }

    m_currentAppId.clear();
    return OS_OK;
}

size_t AppManager::getTotalMemoryUsage() const {
    size_t total = 0;
    for (const auto& [appId, app] : m_runningApps) {
        if (app) {
            total += app->getMemoryUsage();
        }
    }
    return total;
}

void AppManager::printStats() const {
    ESP_LOGI(TAG, "=== Application Manager Statistics ===");
    ESP_LOGI(TAG, "Registered apps: %d", m_appFactories.size());
    ESP_LOGI(TAG, "Running apps: %d/%d", m_runningApps.size(), m_maxConcurrentApps);
    ESP_LOGI(TAG, "Current app: %s", m_currentAppId.empty() ? "none" : m_currentAppId.c_str());
    ESP_LOGI(TAG, "Total launches: %d", m_totalLaunches);
    ESP_LOGI(TAG, "Total kills: %d", m_totalKills);
    ESP_LOGI(TAG, "Total memory usage: %d KB", getTotalMemoryUsage() / 1024);

    ESP_LOGI(TAG, "=== Running Applications ===");
    for (const auto& [appId, app] : m_runningApps) {
        if (app) {
            AppInfo info = app->getAppInfo();
            ESP_LOGI(TAG, "App '%s': %s, runtime: %d s, memory: %d KB",
                    appId.c_str(),
                    info.state == AppState::RUNNING ? "running" :
                    info.state == AppState::PAUSED ? "paused" : "other",
                    info.runTime / 1000,
                    info.memoryUsage / 1024);
        }
    }
}

std::unique_ptr<BaseApp> AppManager::createApp(const std::string& appId) {
    auto it = m_appFactories.find(appId);
    if (it == m_appFactories.end()) {
        ESP_LOGE(TAG, "Application '%s' not registered", appId.c_str());
        return nullptr;
    }

    try {
        return it->second();
    } catch (...) {
        ESP_LOGE(TAG, "Exception creating application '%s'", appId.c_str());
        return nullptr;
    }
}

void AppManager::cleanupStoppedApps() {
    auto it = m_runningApps.begin();
    while (it != m_runningApps.end()) {
        if (!it->second || it->second->getState() == AppState::STOPPED) {
            ESP_LOGD(TAG, "Cleaning up stopped app '%s'", it->first.c_str());
            it = m_runningApps.erase(it);
        } else {
            ++it;
        }
    }
}

void AppManager::enforceResourceLimits() {
    // Check memory usage and kill apps if needed
    size_t totalMemory = getTotalMemoryUsage();
    size_t systemMemory = OS().getMemoryManager().getTotalAllocated();
    
    if (totalMemory > OS_APP_HEAP_SIZE || systemMemory > OS_SYSTEM_HEAP_SIZE) {
        ESP_LOGW(TAG, "Memory usage high, considering app cleanup");
        
        // Find lowest priority apps to kill
        std::vector<std::pair<std::string, AppPriority>> candidates;
        for (const auto& [appId, app] : m_runningApps) {
            if (app && appId != m_currentAppId) {
                candidates.push_back({appId, app->getPriority()});
            }
        }
        
        // Sort by priority (lowest first)
        std::sort(candidates.begin(), candidates.end(),
                  [](const auto& a, const auto& b) {
                      return a.second < b.second;
                  });
        
        // Kill lowest priority apps until memory is acceptable
        for (const auto& [appId, priority] : candidates) {
            if (getTotalMemoryUsage() < OS_APP_HEAP_SIZE * 0.8) {
                break;
            }
            ESP_LOGW(TAG, "Killing app '%s' due to memory pressure", appId.c_str());
            killApp(appId);
        }
    }
}

void AppManager::handleAppEvent(const EventData& eventData) {
    // Handle application-related events
    // This could include logging, statistics, or triggering other actions
}