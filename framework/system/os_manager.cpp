#include "os_manager.h"
#include <esp_log.h>
#include <esp_task_wdt.h>

static const char* TAG = "OSManager";

OSManager& OSManager::getInstance() {
    static OSManager instance;
    return instance;
}

os_error_t OSManager::initialize() {
    if (m_initialized) {
        ESP_LOGW(TAG, "OS already initialized");
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing M5Tab5 OS v%s", OS_VERSION_STRING);
    
    // Record start time
    m_startTime = millis();

    // Initialize subsystems in order of dependency
    os_error_t result = initializeSubsystems();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize subsystems: %d", result);
        return result;
    }

    m_initialized = true;
    ESP_LOGI(TAG, "OS initialization complete");
    
    return OS_OK;
}

os_error_t OSManager::start() {
    if (!m_initialized) {
        ESP_LOGE(TAG, "OS not initialized");
        return OS_ERROR_GENERIC;
    }

    if (m_running) {
        ESP_LOGW(TAG, "OS already running");
        return OS_OK;
    }

    ESP_LOGI(TAG, "Starting OS main loop");
    m_running = true;
    m_lastUpdate = millis();

    return OS_OK;
}

os_error_t OSManager::shutdown() {
    if (!m_running) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down OS");
    m_running = false;

    // Shutdown subsystems in reverse order
    if (m_serviceManager) {
        m_serviceManager->shutdown();
    }
    if (m_appManager) {
        m_appManager->shutdown();
    }
    if (m_uiManager) {
        m_uiManager->shutdown();
    }
    if (m_halManager) {
        m_halManager->shutdown();
    }
    if (m_taskScheduler) {
        m_taskScheduler->shutdown();
    }

    ESP_LOGI(TAG, "OS shutdown complete");
    return OS_OK;
}

os_error_t OSManager::update() {
    if (!m_running) {
        return OS_ERROR_GENERIC;
    }

    uint32_t now = millis();
    uint32_t deltaTime = now - m_lastUpdate;
    m_lastUpdate = now;

    // Update CPU usage statistics
    if (now - m_lastCPUCheck >= 1000) {
        updateCPUUsage();
        m_lastCPUCheck = now;
    }

    // Feed watchdog
    feedWatchdog();

    // Update subsystems
    if (m_taskScheduler) {
        m_taskScheduler->update(deltaTime);
    }

    if (m_eventSystem) {
        m_eventSystem->processEvents();
    }

    if (m_halManager) {
        m_halManager->update(deltaTime);
    }

    if (m_uiManager) {
        m_uiManager->update(deltaTime);
    }

    if (m_appManager) {
        m_appManager->update(deltaTime);
    }

    if (m_serviceManager) {
        m_serviceManager->update(deltaTime);
    }

    return OS_OK;
}

os_error_t OSManager::initializeSubsystems() {
    ESP_LOGI(TAG, "Initializing subsystems...");

    // Memory Manager (first - others depend on it)
    m_memoryManager = new MemoryManager();
    if (!m_memoryManager || m_memoryManager->initialize() != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize Memory Manager");
        return OS_ERROR_GENERIC;
    }

    // Task Scheduler
    m_taskScheduler = new TaskScheduler();
    if (!m_taskScheduler || m_taskScheduler->initialize() != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize Task Scheduler");
        return OS_ERROR_GENERIC;
    }

    // Event System
    m_eventSystem = new EventSystem();
    if (!m_eventSystem || m_eventSystem->initialize() != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize Event System");
        return OS_ERROR_GENERIC;
    }

    // Hardware Abstraction Layer
    m_halManager = new HALManager();
    if (!m_halManager || m_halManager->initialize() != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize HAL Manager");
        return OS_ERROR_GENERIC;
    }

    // UI Manager
    m_uiManager = new UIManager();
    if (!m_uiManager || m_uiManager->initialize() != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize UI Manager");
        return OS_ERROR_GENERIC;
    }

    // Application Manager
    m_appManager = new AppManager();
    if (!m_appManager || m_appManager->initialize() != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize App Manager");
        return OS_ERROR_GENERIC;
    }

    // Service Manager
    m_serviceManager = new ServiceManager();
    if (!m_serviceManager || m_serviceManager->initialize() != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize Service Manager");
        return OS_ERROR_GENERIC;
    }

    ESP_LOGI(TAG, "All subsystems initialized successfully");
    return OS_OK;
}

void OSManager::updateCPUUsage() {
    // Simple CPU usage calculation based on idle time
    static uint32_t lastIdleTime = 0;
    uint32_t currentIdleTime = xTaskGetIdleTaskHandle() ? 
        uxTaskGetSystemState(NULL, 0, NULL) : 0;
    
    if (lastIdleTime > 0) {
        uint32_t idleDelta = currentIdleTime - lastIdleTime;
        uint32_t totalDelta = 1000; // 1 second
        m_cpuUsage = 100 - ((idleDelta * 100) / totalDelta);
        if (m_cpuUsage > 100) m_cpuUsage = 100;
    }
    
    lastIdleTime = currentIdleTime;
}

void OSManager::feedWatchdog() {
    // Feed the hardware watchdog
    #ifdef CONFIG_ESP_TASK_WDT_EN
    esp_task_wdt_reset();
    #endif
}