#include "base_app.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <cstdarg>

BaseApp::BaseApp(const std::string& id, const std::string& name, const std::string& version)
    : m_id(id), m_name(name), m_version(version) {
    
    // Set default metadata
    m_description = "Application for M5Stack Tab5";
    m_author = "Unknown";
}

os_error_t BaseApp::start() {
    if (m_state == AppState::RUNNING) {
        return OS_OK; // Already running
    }

    if (m_state == AppState::PAUSED) {
        return resume(); // Resume from pause
    }

    setState(AppState::STARTING);
    
    log(ESP_LOG_INFO, "Starting application");
    
    m_startTime = millis();
    m_exitRequested = false;
    
    setState(AppState::RUNNING);
    
    // Publish app launch event
    PUBLISH_EVENT(EVENT_APP_LAUNCH, (void*)m_id.c_str(), m_id.length());
    
    return OS_OK;
}

os_error_t BaseApp::pause() {
    if (m_state != AppState::RUNNING) {
        return OS_ERROR_GENERIC;
    }

    setState(AppState::PAUSED);
    log(ESP_LOG_INFO, "Application paused");
    
    // Publish app suspend event
    PUBLISH_EVENT(EVENT_APP_SUSPEND, (void*)m_id.c_str(), m_id.length());
    
    return OS_OK;
}

os_error_t BaseApp::resume() {
    if (m_state != AppState::PAUSED) {
        return OS_ERROR_GENERIC;
    }

    setState(AppState::RUNNING);
    log(ESP_LOG_INFO, "Application resumed");
    
    // Publish app resume event
    PUBLISH_EVENT(EVENT_APP_RESUME, (void*)m_id.c_str(), m_id.length());
    
    return OS_OK;
}

os_error_t BaseApp::stop() {
    if (m_state == AppState::STOPPED) {
        return OS_OK; // Already stopped
    }

    setState(AppState::STOPPING);
    log(ESP_LOG_INFO, "Stopping application");

    // Destroy UI if it exists
    if (m_uiContainer) {
        destroyUI();
    }

    setState(AppState::STOPPED);
    
    // Publish app exit event
    PUBLISH_EVENT(EVENT_APP_EXIT, (void*)m_id.c_str(), m_id.length());
    
    return OS_OK;
}

os_error_t BaseApp::handleEvent(uint32_t eventType, void* eventData, size_t dataSize) {
    // Default implementation - applications can override this
    return OS_OK;
}

AppInfo BaseApp::getAppInfo() const {
    AppInfo info;
    info.id = m_id;
    info.name = m_name;
    info.version = m_version;
    info.description = m_description;
    info.author = m_author;
    info.priority = m_priority;
    info.memoryUsage = m_memoryUsage;
    info.startTime = m_startTime;
    info.runTime = getRuntime();
    info.state = m_state;
    return info;
}

uint32_t BaseApp::getRuntime() const {
    if (m_startTime == 0) {
        return 0;
    }
    return millis() - m_startTime;
}

void BaseApp::setState(AppState state) {
    if (m_state != state) {
        AppState previousState = m_state;
        m_state = state;
        
        // Log state changes in debug mode
        #if OS_DEBUG_ENABLED >= 2
        const char* stateNames[] = {
            "STOPPED", "STARTING", "RUNNING", "PAUSED", "STOPPING", "ERROR"
        };
        log(ESP_LOG_DEBUG, ("State changed: " + std::string(stateNames[(int)previousState]) + 
                           " -> " + std::string(stateNames[(int)state])).c_str());
        #endif
    }
}

void BaseApp::log(int level, const char* format, ...) const {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    std::string logMessage = "[" + m_name + "] " + buffer;
    esp_log_write((esp_log_level_t)level, m_id.c_str(), "%s", logMessage.c_str());
}