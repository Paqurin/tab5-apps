#ifndef OS_MANAGER_H
#define OS_MANAGER_H

#include "os_config.h"
#include "memory_manager.h"
#include "task_scheduler.h"
#include "event_system.h"
#include "../hal/hal_manager.h"
#include "../ui/ui_manager.h"
#include "../apps/app_manager.h"
#include "../services/service_manager.h"

/**
 * @file os_manager.h
 * @brief Core operating system manager for M5Stack Tab5
 * 
 * The OSManager is the central coordinator for all system components.
 * It handles initialization, shutdown, and coordination between subsystems.
 */

class OSManager {
public:
    /**
     * @brief Get the singleton instance of OSManager
     * @return Reference to the OSManager instance
     */
    static OSManager& getInstance();

    /**
     * @brief Initialize the operating system
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Start the operating system main loop
     * @return OS_OK on success, error code on failure
     */
    os_error_t start();

    /**
     * @brief Shutdown the operating system
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Run one iteration of the main loop
     * @return OS_OK on success, error code on failure
     */
    os_error_t update();

    /**
     * @brief Check if the system is initialized
     * @return true if initialized, false otherwise
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Check if the system is running
     * @return true if running, false otherwise
     */
    bool isRunning() const { return m_running; }

    /**
     * @brief Get system uptime in milliseconds
     * @return System uptime in milliseconds
     */
    uint32_t getUptime() const { return millis() - m_startTime; }

    /**
     * @brief Get free heap memory
     * @return Free heap memory in bytes
     */
    size_t getFreeHeap() const { return ESP.getFreeHeap(); }

    /**
     * @brief Get total heap memory
     * @return Total heap memory in bytes
     */
    size_t getTotalHeap() const { return ESP.getHeapSize(); }

    /**
     * @brief Get CPU usage percentage
     * @return CPU usage as percentage (0-100)
     */
    uint8_t getCPUUsage() const { return m_cpuUsage; }

    // Subsystem accessors
    MemoryManager& getMemoryManager() { return *m_memoryManager; }
    TaskScheduler& getTaskScheduler() { return *m_taskScheduler; }
    EventSystem& getEventSystem() { return *m_eventSystem; }
    HALManager& getHALManager() { return *m_halManager; }
    UIManager& getUIManager() { return *m_uiManager; }
    AppManager& getAppManager() { return *m_appManager; }
    ServiceManager& getServiceManager() { return *m_serviceManager; }

private:
    OSManager() = default;
    ~OSManager() = default;
    OSManager(const OSManager&) = delete;
    OSManager& operator=(const OSManager&) = delete;

    /**
     * @brief Initialize all subsystems
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeSubsystems();

    /**
     * @brief Update CPU usage statistics
     */
    void updateCPUUsage();

    /**
     * @brief Handle system watchdog
     */
    void feedWatchdog();

    // System state
    bool m_initialized = false;
    bool m_running = false;
    uint32_t m_startTime = 0;
    uint32_t m_lastUpdate = 0;
    uint32_t m_lastCPUCheck = 0;
    uint8_t m_cpuUsage = 0;

    // Subsystem managers
    MemoryManager* m_memoryManager = nullptr;
    TaskScheduler* m_taskScheduler = nullptr;
    EventSystem* m_eventSystem = nullptr;
    HALManager* m_halManager = nullptr;
    UIManager* m_uiManager = nullptr;
    AppManager* m_appManager = nullptr;
    ServiceManager* m_serviceManager = nullptr;
};

// Global accessor macro
#define OS() OSManager::getInstance()

#endif // OS_MANAGER_H