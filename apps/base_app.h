#ifndef BASE_APP_H
#define BASE_APP_H

#include "../system/os_config.h"
#include <lvgl.h>
#include <string>
#include <functional>

/**
 * @file base_app.h
 * @brief Base application class for M5Stack Tab5
 * 
 * Provides the foundation for all applications running on the system.
 * Applications inherit from this class and implement the virtual methods.
 */

enum class AppState {
    STOPPED,
    STARTING,
    RUNNING,
    PAUSED,
    STOPPING,
    ERROR
};

enum class AppPriority {
    APP_LOW = 0,
    APP_NORMAL = 1,
    APP_HIGH = 2,
    APP_SYSTEM = 3
};

struct AppInfo {
    std::string id;
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    AppPriority priority;
    size_t memoryUsage;
    uint32_t startTime;
    uint32_t runTime;
    AppState state;
};

class BaseApp {
public:
    /**
     * @brief Constructor
     * @param id Unique application identifier
     * @param name Human-readable application name
     * @param version Application version string
     */
    BaseApp(const std::string& id, const std::string& name, const std::string& version);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~BaseApp() = default;

    /**
     * @brief Initialize the application
     * @return OS_OK on success, error code on failure
     */
    virtual os_error_t initialize() = 0;

    /**
     * @brief Start the application
     * @return OS_OK on success, error code on failure
     */
    virtual os_error_t start();

    /**
     * @brief Update the application (called every frame)
     * @param deltaTime Time elapsed since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    virtual os_error_t update(uint32_t deltaTime) = 0;

    /**
     * @brief Pause the application
     * @return OS_OK on success, error code on failure
     */
    virtual os_error_t pause();

    /**
     * @brief Resume the application from paused state
     * @return OS_OK on success, error code on failure
     */
    virtual os_error_t resume();

    /**
     * @brief Stop the application
     * @return OS_OK on success, error code on failure
     */
    virtual os_error_t stop();

    /**
     * @brief Shutdown the application (cleanup resources)
     * @return OS_OK on success, error code on failure
     */
    virtual os_error_t shutdown() = 0;

    /**
     * @brief Create the application's UI screen
     * @param parent Parent object to create UI in
     * @return OS_OK on success, error code on failure
     */
    virtual os_error_t createUI(lv_obj_t* parent) = 0;

    /**
     * @brief Destroy the application's UI screen
     * @return OS_OK on success, error code on failure
     */
    virtual os_error_t destroyUI() = 0;

    /**
     * @brief Handle application-specific events
     * @param eventType Event type
     * @param eventData Event data pointer
     * @param dataSize Size of event data
     * @return OS_OK on success, error code on failure
     */
    virtual os_error_t handleEvent(uint32_t eventType, void* eventData, size_t dataSize);

    /**
     * @brief Get application information
     * @return Application info structure
     */
    AppInfo getAppInfo() const;

    /**
     * @brief Get application ID
     * @return Application ID string
     */
    const std::string& getId() const { return m_id; }

    /**
     * @brief Get application name
     * @return Application name string
     */
    const std::string& getName() const { return m_name; }

    /**
     * @brief Get application version
     * @return Version string
     */
    const std::string& getVersion() const { return m_version; }

    /**
     * @brief Get current application state
     * @return Current state
     */
    AppState getState() const { return m_state; }

    /**
     * @brief Check if application is running
     * @return true if running, false otherwise
     */
    bool isRunning() const { return m_state == AppState::RUNNING; }

    /**
     * @brief Check if application is paused
     * @return true if paused, false otherwise
     */
    bool isPaused() const { return m_state == AppState::PAUSED; }

    /**
     * @brief Set application description
     * @param description Description string
     */
    void setDescription(const std::string& description) { m_description = description; }

    /**
     * @brief Set application author
     * @param author Author string
     */
    void setAuthor(const std::string& author) { m_author = author; }

    /**
     * @brief Set application priority
     * @param priority Priority level
     */
    void setPriority(AppPriority priority) { m_priority = priority; }

    /**
     * @brief Get application priority
     * @return Priority level
     */
    AppPriority getPriority() const { return m_priority; }

    /**
     * @brief Get runtime in milliseconds
     * @return Runtime since start
     */
    uint32_t getRuntime() const;

    /**
     * @brief Get memory usage estimate
     * @return Memory usage in bytes
     */
    size_t getMemoryUsage() const { return m_memoryUsage; }

    /**
     * @brief Request application exit
     */
    void requestExit() { m_exitRequested = true; }

    /**
     * @brief Check if exit was requested
     * @return true if exit requested, false otherwise
     */
    bool isExitRequested() const { return m_exitRequested; }

protected:
    /**
     * @brief Set application state
     * @param state New state
     */
    void setState(AppState state);

    /**
     * @brief Update memory usage estimate
     * @param usage Memory usage in bytes
     */
    void setMemoryUsage(size_t usage) { m_memoryUsage = usage; }

    /**
     * @brief Log application message
     * @param level Log level
     * @param format Message format string
     * @param ... Variable arguments for formatting
     */
    void log(int level, const char* format, ...) const;

    // Application metadata
    std::string m_id;
    std::string m_name;
    std::string m_version;
    std::string m_description;
    std::string m_author;

    // Application state
    AppState m_state = AppState::STOPPED;
    AppPriority m_priority = AppPriority::APP_NORMAL;
    uint32_t m_startTime = 0;
    size_t m_memoryUsage = 0;
    bool m_exitRequested = false;
    bool m_initialized = false;

    // UI reference
    lv_obj_t* m_uiContainer = nullptr;
};

#endif // BASE_APP_H