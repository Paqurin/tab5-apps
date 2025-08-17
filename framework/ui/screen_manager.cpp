#include "screen_manager.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <algorithm>

static const char* TAG = "ScreenManager";

ScreenManager::~ScreenManager() {
    shutdown();
}

os_error_t ScreenManager::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Screen Manager");

    // Clear any existing state
    m_screens.clear();
    m_screenHistory.clear();
    m_currentScreenName.clear();

    m_initialized = true;
    ESP_LOGI(TAG, "Screen Manager initialized");

    return OS_OK;
}

os_error_t ScreenManager::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Screen Manager");

    // Destroy all screens
    for (auto& [name, info] : m_screens) {
        destroyScreen(name);
    }

    m_screens.clear();
    m_screenHistory.clear();
    m_currentScreenName.clear();

    m_initialized = false;
    ESP_LOGI(TAG, "Screen Manager shutdown complete");

    return OS_OK;
}

os_error_t ScreenManager::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    // Periodic cleanup of unused screens
    static uint32_t lastCleanup = 0;
    uint32_t now = millis();
    
    if (now - lastCleanup >= 60000) { // Cleanup every minute
        cleanupScreens();
        lastCleanup = now;
    }

    return OS_OK;
}

os_error_t ScreenManager::registerScreen(const std::string& name,
                                        ScreenCallback createCallback,
                                        ScreenCallback destroyCallback,
                                        bool persistent) {
    if (!m_initialized || name.empty() || !createCallback) {
        return OS_ERROR_INVALID_PARAM;
    }

    if (hasScreen(name)) {
        ESP_LOGW(TAG, "Screen '%s' already registered", name.c_str());
        return OS_ERROR_GENERIC;
    }

    ScreenInfo info;
    info.name = name;
    info.screen = nullptr;
    info.createCallback = createCallback;
    info.destroyCallback = destroyCallback;
    info.persistent = persistent;
    info.createTime = 0;
    info.lastAccess = 0;

    m_screens[name] = info;

    ESP_LOGD(TAG, "Registered screen '%s' (persistent: %s)", 
             name.c_str(), persistent ? "yes" : "no");

    return OS_OK;
}

os_error_t ScreenManager::unregisterScreen(const std::string& name) {
    if (!m_initialized || name.empty()) {
        return OS_ERROR_INVALID_PARAM;
    }

    auto it = m_screens.find(name);
    if (it == m_screens.end()) {
        return OS_ERROR_NOT_FOUND;
    }

    // If this is the current screen, switch away first
    if (m_currentScreenName == name && !m_screenHistory.empty()) {
        goBack();
    }

    // Destroy the screen
    destroyScreen(name);
    m_screens.erase(it);

    ESP_LOGD(TAG, "Unregistered screen '%s'", name.c_str());

    return OS_OK;
}

os_error_t ScreenManager::switchToScreen(const std::string& name,
                                        ScreenTransition transition,
                                        uint32_t animationTime) {
    if (!m_initialized || name.empty()) {
        return OS_ERROR_INVALID_PARAM;
    }

    if (!hasScreen(name)) {
        ESP_LOGE(TAG, "Screen '%s' not registered", name.c_str());
        return OS_ERROR_NOT_FOUND;
    }

    if (m_currentScreenName == name) {
        ESP_LOGD(TAG, "Already on screen '%s'", name.c_str());
        return OS_OK;
    }

    // Get current screen
    lv_obj_t* currentScreen = getCurrentScreen();

    // Create new screen if needed
    lv_obj_t* newScreen = createScreen(name);
    if (!newScreen) {
        ESP_LOGE(TAG, "Failed to create screen '%s'", name.c_str());
        return OS_ERROR_GENERIC;
    }

    // Apply transition animation
    applyTransition(currentScreen, newScreen, transition, animationTime);

    // Update navigation state
    if (!m_currentScreenName.empty()) {
        m_screenHistory.push_back(m_currentScreenName);
        
        // Limit history size
        if (m_screenHistory.size() > m_maxHistory) {
            m_screenHistory.erase(m_screenHistory.begin());
        }
    }

    m_currentScreenName = name;
    updateAccessTime(name);

    // Set as active screen
    lv_scr_load(newScreen);

    m_totalTransitions++;

    ESP_LOGD(TAG, "Switched to screen '%s'", name.c_str());
    PUBLISH_EVENT(EVENT_UI_SCREEN_CHANGE, (void*)name.c_str(), name.length());

    return OS_OK;
}

os_error_t ScreenManager::goBack(ScreenTransition transition) {
    if (!m_initialized || m_screenHistory.empty()) {
        return OS_ERROR_GENERIC;
    }

    std::string previousScreen = m_screenHistory.back();
    m_screenHistory.pop_back();

    // Don't add current screen to history when going back
    std::string tempCurrent = m_currentScreenName;
    m_currentScreenName.clear();

    os_error_t result = switchToScreen(previousScreen, transition);
    
    if (result != OS_OK) {
        // Restore state if switch failed
        m_currentScreenName = tempCurrent;
        m_screenHistory.push_back(previousScreen);
    }

    return result;
}

lv_obj_t* ScreenManager::getCurrentScreen() const {
    if (m_currentScreenName.empty()) {
        return nullptr;
    }

    auto it = m_screens.find(m_currentScreenName);
    if (it != m_screens.end()) {
        return it->second.screen;
    }

    return nullptr;
}

bool ScreenManager::hasScreen(const std::string& name) const {
    return m_screens.find(name) != m_screens.end();
}

std::vector<std::string> ScreenManager::getScreenNames() const {
    std::vector<std::string> names;
    for (const auto& [name, info] : m_screens) {
        names.push_back(name);
    }
    return names;
}

void ScreenManager::clearHistory() {
    m_screenHistory.clear();
    ESP_LOGD(TAG, "Cleared screen history");
}

void ScreenManager::printStats() const {
    ESP_LOGI(TAG, "=== Screen Manager Statistics ===");
    ESP_LOGI(TAG, "Registered screens: %d", m_screens.size());
    ESP_LOGI(TAG, "Current screen: %s", m_currentScreenName.c_str());
    ESP_LOGI(TAG, "History depth: %d/%d", m_screenHistory.size(), m_maxHistory);
    ESP_LOGI(TAG, "Total transitions: %d", m_totalTransitions);
    ESP_LOGI(TAG, "Screen creations: %d", m_screenCreations);
    ESP_LOGI(TAG, "Screen destructions: %d", m_screenDestructions);

    ESP_LOGI(TAG, "=== Screen Details ===");
    for (const auto& [name, info] : m_screens) {
        ESP_LOGI(TAG, "Screen '%s': %s, persistent: %s, created: %s",
                name.c_str(),
                info.screen ? "in memory" : "not loaded",
                info.persistent ? "yes" : "no",
                info.createTime > 0 ? "yes" : "no");
    }
}

void ScreenManager::cleanupScreens() {
    if (m_screens.size() <= m_maxScreens) {
        return;
    }

    std::vector<std::pair<std::string, uint32_t>> candidates;

    // Find non-persistent, non-current screens for cleanup
    for (const auto& [name, info] : m_screens) {
        if (!info.persistent && name != m_currentScreenName && info.screen) {
            candidates.push_back({name, info.lastAccess});
        }
    }

    // Sort by last access time (oldest first)
    std::sort(candidates.begin(), candidates.end(),
              [](const auto& a, const auto& b) {
                  return a.second < b.second;
              });

    // Clean up oldest screens
    size_t toRemove = m_screens.size() - m_maxScreens;
    for (size_t i = 0; i < std::min(toRemove, candidates.size()); i++) {
        destroyScreen(candidates[i].first);
    }

    if (!candidates.empty()) {
        ESP_LOGD(TAG, "Cleaned up %d unused screens", 
                std::min(toRemove, candidates.size()));
    }
}

lv_obj_t* ScreenManager::createScreen(const std::string& name) {
    auto it = m_screens.find(name);
    if (it == m_screens.end()) {
        return nullptr;
    }

    ScreenInfo& info = it->second;

    // Return existing screen if already created
    if (info.screen) {
        updateAccessTime(name);
        return info.screen;
    }

    // Create new screen
    info.screen = lv_obj_create(nullptr);
    if (!info.screen) {
        ESP_LOGE(TAG, "Failed to create LVGL screen object");
        return nullptr;
    }

    // Set up screen properties
    lv_obj_set_size(info.screen, LV_HOR_RES, LV_VER_RES);
    lv_obj_clear_flag(info.screen, LV_OBJ_FLAG_SCROLLABLE);

    // Call create callback to populate screen content
    try {
        info.createCallback(info.screen);
        info.createTime = millis();
        updateAccessTime(name);
        m_screenCreations++;

        ESP_LOGD(TAG, "Created screen '%s'", name.c_str());
    } catch (...) {
        ESP_LOGE(TAG, "Exception in screen create callback for '%s'", name.c_str());
        lv_obj_del(info.screen);
        info.screen = nullptr;
        return nullptr;
    }

    return info.screen;
}

void ScreenManager::destroyScreen(const std::string& name) {
    auto it = m_screens.find(name);
    if (it == m_screens.end() || !it->second.screen) {
        return;
    }

    ScreenInfo& info = it->second;

    // Call destroy callback if provided
    if (info.destroyCallback) {
        try {
            info.destroyCallback(info.screen);
        } catch (...) {
            ESP_LOGE(TAG, "Exception in screen destroy callback for '%s'", name.c_str());
        }
    }

    // Delete LVGL screen object
    lv_obj_del(info.screen);
    info.screen = nullptr;
    info.createTime = 0;
    
    m_screenDestructions++;

    ESP_LOGD(TAG, "Destroyed screen '%s'", name.c_str());
}

void ScreenManager::applyTransition(lv_obj_t* fromScreen, lv_obj_t* toScreen,
                                   ScreenTransition transition, uint32_t animationTime) {
    if (!toScreen) return;

    // For now, implement simple transitions
    // In a full implementation, this would handle complex animations
    
    switch (transition) {
        case ScreenTransition::FADE:
            // Simple fade transition
            lv_obj_set_style_opa(toScreen, LV_OPA_TRANSP, 0);
            lv_anim_t anim;
            lv_anim_init(&anim);
            lv_anim_set_var(&anim, toScreen);
            lv_anim_set_values(&anim, LV_OPA_TRANSP, LV_OPA_COVER);
            lv_anim_set_time(&anim, animationTime);
            lv_anim_set_exec_cb(&anim, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
            lv_anim_start(&anim);
            break;

        case ScreenTransition::SLIDE_LEFT:
        case ScreenTransition::SLIDE_RIGHT:
        case ScreenTransition::SLIDE_UP:
        case ScreenTransition::SLIDE_DOWN:
            // TODO: Implement slide transitions
            break;

        case ScreenTransition::NONE:
        default:
            // No animation, just show the screen
            break;
    }
}

void ScreenManager::updateAccessTime(const std::string& name) {
    auto it = m_screens.find(name);
    if (it != m_screens.end()) {
        it->second.lastAccess = millis();
    }
}