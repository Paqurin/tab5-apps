#include "event_system.h"
#include <esp_log.h>
#include <algorithm>

static const char* TAG = "EventSystem";

EventSystem::~EventSystem() {
    shutdown();
}

os_error_t EventSystem::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Event System");
    
    // Clear any existing state
    m_listeners.clear();
    clearQueue();
    
    m_initialized = true;
    ESP_LOGI(TAG, "Event System initialized");
    
    return OS_OK;
}

os_error_t EventSystem::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Event System");
    
    // Clear all listeners and queued events
    m_listeners.clear();
    clearQueue();
    
    m_initialized = false;
    ESP_LOGI(TAG, "Event System shutdown complete");
    
    return OS_OK;
}

size_t EventSystem::processEvents() {
    if (!m_initialized) {
        return 0;
    }

    size_t eventsProcessed = 0;
    
    // Process events in batches to avoid blocking too long
    const size_t maxEventsPerFrame = 10;
    
    while (!m_eventQueue.empty() && eventsProcessed < maxEventsPerFrame) {
        EventData event = m_eventQueue.front();
        m_eventQueue.pop();
        
        size_t listenersNotified = publishSync(event);
        
        if (m_loggingEnabled && listenersNotified > 0) {
            ESP_LOGD(TAG, "Processed async event %d, notified %d listeners", 
                    event.type, listenersNotified);
        }
        
        eventsProcessed++;
        m_eventsProcessed++;
    }
    
    // Clean up one-shot listeners that have been called
    if (eventsProcessed > 0) {
        cleanupListeners();
    }
    
    return eventsProcessed;
}

ListenerId EventSystem::subscribe(EventType eventType, EventCallback callback, 
                                 uint8_t priority, bool oneShot) {
    if (!m_initialized || !callback) {
        return 0;
    }

    EventListener listener;
    listener.id = generateListenerId();
    listener.eventType = eventType;
    listener.callback = callback;
    listener.priority = priority;
    listener.oneShot = oneShot;
    listener.callCount = 0;

    m_listeners[eventType].push_back(listener);
    
    // Sort listeners by priority (higher priority first)
    std::sort(m_listeners[eventType].begin(), m_listeners[eventType].end(),
              [](const EventListener& a, const EventListener& b) {
                  return a.priority > b.priority;
              });

    if (m_loggingEnabled) {
        ESP_LOGD(TAG, "Subscribed listener %d to event %d (priority %d, one-shot: %s)", 
                listener.id, eventType, priority, oneShot ? "yes" : "no");
    }

    return listener.id;
}

bool EventSystem::unsubscribe(ListenerId listenerId) {
    if (!m_initialized) {
        return false;
    }

    for (auto& [eventType, listeners] : m_listeners) {
        auto it = std::find_if(listeners.begin(), listeners.end(),
                              [listenerId](const EventListener& listener) {
                                  return listener.id == listenerId;
                              });
        
        if (it != listeners.end()) {
            if (m_loggingEnabled) {
                ESP_LOGD(TAG, "Unsubscribed listener %d from event %d", 
                        listenerId, eventType);
            }
            listeners.erase(it);
            return true;
        }
    }
    
    return false;
}

size_t EventSystem::unsubscribeAll(EventType eventType) {
    if (!m_initialized) {
        return 0;
    }

    auto it = m_listeners.find(eventType);
    if (it != m_listeners.end()) {
        size_t count = it->second.size();
        m_listeners.erase(it);
        
        if (m_loggingEnabled) {
            ESP_LOGD(TAG, "Unsubscribed all %d listeners from event %d", count, eventType);
        }
        
        return count;
    }
    
    return 0;
}

size_t EventSystem::publishSync(const EventData& event) {
    if (!m_initialized) {
        return 0;
    }

    auto listeners = findListeners(event.type);
    size_t notified = 0;

    for (auto* listener : listeners) {
        try {
            listener->callback(event);
            listener->callCount++;
            notified++;
            m_listenersNotified++;
        } catch (...) {
            ESP_LOGE(TAG, "Exception in event listener %d for event %d", 
                    listener->id, event.type);
        }
    }

    if (m_loggingEnabled && notified > 0) {
        ESP_LOGD(TAG, "Published event %d synchronously, notified %d listeners", 
                event.type, notified);
    }

    return notified;
}

bool EventSystem::publishAsync(const EventData& event) {
    if (!m_initialized) {
        return false;
    }

    if (m_eventQueue.size() >= OS_EVENT_QUEUE_SIZE) {
        ESP_LOGW(TAG, "Event queue full, dropping event %d", event.type);
        return false;
    }

    m_eventQueue.push(event);
    m_eventsPublished++;

    if (m_loggingEnabled) {
        ESP_LOGD(TAG, "Queued async event %d", event.type);
    }

    return true;
}

size_t EventSystem::publish(EventType eventType, void* data, size_t dataSize,
                           uint32_t senderId, bool async) {
    EventData event(eventType, data, dataSize, senderId);
    
    if (async) {
        return publishAsync(event) ? 1 : 0;
    } else {
        return publishSync(event);
    }
}

bool EventSystem::hasListeners(EventType eventType) const {
    auto it = m_listeners.find(eventType);
    return it != m_listeners.end() && !it->second.empty();
}

size_t EventSystem::getListenerCount(EventType eventType) const {
    auto it = m_listeners.find(eventType);
    return (it != m_listeners.end()) ? it->second.size() : 0;
}

void EventSystem::clearQueue() {
    while (!m_eventQueue.empty()) {
        m_eventQueue.pop();
    }
}

void EventSystem::printStats() const {
    ESP_LOGI(TAG, "=== Event System Statistics ===");
    ESP_LOGI(TAG, "Events published: %d", m_eventsPublished);
    ESP_LOGI(TAG, "Events processed: %d", m_eventsProcessed);
    ESP_LOGI(TAG, "Listeners notified: %d", m_listenersNotified);
    ESP_LOGI(TAG, "Queued events: %d/%d", m_eventQueue.size(), OS_EVENT_QUEUE_SIZE);
    ESP_LOGI(TAG, "Event types with listeners: %d", m_listeners.size());

    ESP_LOGI(TAG, "=== Event Type Statistics ===");
    for (const auto& [eventType, listeners] : m_listeners) {
        ESP_LOGI(TAG, "Event %d: %d listeners", eventType, listeners.size());
        
        for (const auto& listener : listeners) {
            ESP_LOGI(TAG, "  Listener %d: priority %d, calls %d, one-shot: %s",
                    listener.id, listener.priority, listener.callCount,
                    listener.oneShot ? "yes" : "no");
        }
    }
}

std::vector<EventListener*> EventSystem::findListeners(EventType eventType) {
    std::vector<EventListener*> result;
    
    auto it = m_listeners.find(eventType);
    if (it != m_listeners.end()) {
        for (auto& listener : it->second) {
            result.push_back(&listener);
        }
    }
    
    return result;
}

void EventSystem::cleanupListeners() {
    for (auto& [eventType, listeners] : m_listeners) {
        listeners.erase(
            std::remove_if(listeners.begin(), listeners.end(),
                          [](const EventListener& listener) {
                              return listener.oneShot && listener.callCount > 0;
                          }),
            listeners.end());
    }
}