#ifndef EVENT_SYSTEM_H
#define EVENT_SYSTEM_H

#include "os_config.h"
#include <functional>
#include <vector>
#include <queue>
#include <map>

/**
 * @file event_system.h
 * @brief Event system for M5Stack Tab5
 * 
 * Provides a publish-subscribe event system for loose coupling
 * between system components and applications.
 */

typedef uint32_t EventType;
typedef uint32_t ListenerId;

// System event types
enum SystemEvents : EventType {
    EVENT_SYSTEM_STARTUP = 1000,
    EVENT_SYSTEM_SHUTDOWN,
    EVENT_SYSTEM_LOW_MEMORY,
    EVENT_SYSTEM_ERROR,
    
    EVENT_UI_TOUCH_PRESS = 2000,
    EVENT_UI_TOUCH_RELEASE,
    EVENT_UI_TOUCH_MOVE,
    EVENT_UI_GESTURE,
    EVENT_UI_SCREEN_CHANGE,
    
    EVENT_APP_LAUNCH = 3000,
    EVENT_APP_EXIT,
    EVENT_APP_SUSPEND,
    EVENT_APP_RESUME,
    EVENT_APP_ERROR,
    
    EVENT_HAL_BUTTON_PRESS = 4000,
    EVENT_HAL_BUTTON_RELEASE,
    EVENT_HAL_SENSOR_UPDATE,
    EVENT_HAL_BATTERY_CHANGE,
    
    EVENT_SERVICE_START = 5000,
    EVENT_SERVICE_STOP,
    EVENT_SERVICE_ERROR,
    
    EVENT_USER_DEFINED = 10000
};

struct EventData {
    EventType type;
    void* data;
    size_t dataSize;
    uint32_t timestamp;
    uint32_t senderId;
    
    EventData() : type(0), data(nullptr), dataSize(0), timestamp(0), senderId(0) {}
    
    EventData(EventType t, void* d = nullptr, size_t size = 0, uint32_t sender = 0)
        : type(t), data(d), dataSize(size), timestamp(millis()), senderId(sender) {}
};

typedef std::function<void(const EventData&)> EventCallback;

struct EventListener {
    ListenerId id;
    EventType eventType;
    EventCallback callback;
    uint8_t priority;
    bool oneShot;
    uint32_t callCount;
};

class EventSystem {
public:
    EventSystem() = default;
    ~EventSystem();

    /**
     * @brief Initialize the event system
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown the event system
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Process queued events
     * @return Number of events processed
     */
    size_t processEvents();

    /**
     * @brief Subscribe to an event type
     * @param eventType Event type to listen for
     * @param callback Callback function to invoke
     * @param priority Listener priority (higher = called first)
     * @param oneShot True to automatically unsubscribe after first event
     * @return Listener ID or 0 on failure
     */
    ListenerId subscribe(EventType eventType, EventCallback callback, 
                        uint8_t priority = 100, bool oneShot = false);

    /**
     * @brief Unsubscribe from events
     * @param listenerId Listener ID returned by subscribe()
     * @return true if listener was found and removed, false otherwise
     */
    bool unsubscribe(ListenerId listenerId);

    /**
     * @brief Unsubscribe all listeners for an event type
     * @param eventType Event type to clear
     * @return Number of listeners removed
     */
    size_t unsubscribeAll(EventType eventType);

    /**
     * @brief Publish an event immediately (synchronous)
     * @param event Event data to publish
     * @return Number of listeners notified
     */
    size_t publishSync(const EventData& event);

    /**
     * @brief Queue an event for async processing
     * @param event Event data to queue
     * @return true if event was queued, false if queue is full
     */
    bool publishAsync(const EventData& event);

    /**
     * @brief Publish an event with simple data
     * @param eventType Event type
     * @param data Optional data pointer
     * @param dataSize Size of data in bytes
     * @param senderId Optional sender ID
     * @param async True to queue async, false to publish immediately
     * @return Number of listeners notified (sync) or true/false (async)
     */
    size_t publish(EventType eventType, void* data = nullptr, size_t dataSize = 0,
                  uint32_t senderId = 0, bool async = true);

    /**
     * @brief Check if there are listeners for an event type
     * @param eventType Event type to check
     * @return true if there are listeners, false otherwise
     */
    bool hasListeners(EventType eventType) const;

    /**
     * @brief Get number of listeners for an event type
     * @param eventType Event type to check
     * @return Number of listeners
     */
    size_t getListenerCount(EventType eventType) const;

    /**
     * @brief Get number of queued events
     * @return Number of events in queue
     */
    size_t getQueuedEventCount() const { return m_eventQueue.size(); }

    /**
     * @brief Clear all queued events
     */
    void clearQueue();

    /**
     * @brief Print event system statistics
     */
    void printStats() const;

    /**
     * @brief Enable/disable event logging
     * @param enabled True to enable logging, false to disable
     */
    void setLoggingEnabled(bool enabled) { m_loggingEnabled = enabled; }

private:
    /**
     * @brief Generate unique listener ID
     * @return Unique listener ID
     */
    ListenerId generateListenerId() { return ++m_nextListenerId; }

    /**
     * @brief Find listeners for an event type
     * @param eventType Event type to find
     * @return Vector of listeners (sorted by priority)
     */
    std::vector<EventListener*> findListeners(EventType eventType);

    /**
     * @brief Remove one-shot listeners that have been called
     */
    void cleanupListeners();

    // Event listeners organized by type
    std::map<EventType, std::vector<EventListener>> m_listeners;
    
    // Event queue for async processing
    std::queue<EventData> m_eventQueue;
    
    // Statistics
    uint32_t m_eventsPublished = 0;
    uint32_t m_eventsProcessed = 0;
    uint32_t m_listenersNotified = 0;
    
    // Configuration
    ListenerId m_nextListenerId = 0;
    bool m_loggingEnabled = false;
    bool m_initialized = false;
};

// Convenience macros for event publishing
#define PUBLISH_EVENT(type, ...) OS().getEventSystem().publish(type, ##__VA_ARGS__)
#define PUBLISH_EVENT_SYNC(event) OS().getEventSystem().publishSync(event)
#define SUBSCRIBE_EVENT(type, callback, ...) OS().getEventSystem().subscribe(type, callback, ##__VA_ARGS__)

#endif // EVENT_SYSTEM_H