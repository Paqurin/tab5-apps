#ifndef CALENDAR_APP_H
#define CALENDAR_APP_H

#include "base_app.h"
#include <vector>
#include <string>
#include <ctime>

/**
 * @file calendar_app.h
 * @brief Calendar Application for M5Stack Tab5
 * 
 * Provides calendar display, event management, and scheduling functionality.
 */

struct CalendarEvent {
    std::string id;
    std::string title;
    std::string description;
    std::time_t startTime;
    std::time_t endTime;
    std::string location;
    bool isAllDay;
    bool hasReminder;
    uint32_t reminderMinutes;
    std::string category;
    uint32_t color;
};

enum class CalendarView {
    MONTH,
    WEEK,
    DAY,
    AGENDA
};

class CalendarApp : public BaseApp {
public:
    CalendarApp();
    ~CalendarApp() override;

    // BaseApp interface
    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;
    os_error_t handleEvent(uint32_t eventType, void* eventData, size_t dataSize) override;

    /**
     * @brief Add calendar event
     * @param event Event to add
     * @return OS_OK on success, error code on failure
     */
    os_error_t addEvent(const CalendarEvent& event);

    /**
     * @brief Remove calendar event
     * @param eventId Event ID to remove
     * @return OS_OK on success, error code on failure
     */
    os_error_t removeEvent(const std::string& eventId);

    /**
     * @brief Update calendar event
     * @param event Updated event
     * @return OS_OK on success, error code on failure
     */
    os_error_t updateEvent(const CalendarEvent& event);

    /**
     * @brief Get events for specific date
     * @param date Date to get events for
     * @return Vector of events
     */
    std::vector<CalendarEvent> getEventsForDate(std::time_t date) const;

    /**
     * @brief Get events in date range
     * @param startDate Start date
     * @param endDate End date
     * @return Vector of events
     */
    std::vector<CalendarEvent> getEventsInRange(std::time_t startDate, std::time_t endDate) const;

    /**
     * @brief Set calendar view mode
     * @param view View mode to set
     */
    void setCalendarView(CalendarView view);

    /**
     * @brief Navigate to specific date
     * @param date Date to navigate to
     */
    void navigateToDate(std::time_t date);

    /**
     * @brief Navigate to today
     */
    void navigateToToday();

    /**
     * @brief Go to previous period (month/week/day)
     */
    void navigatePrevious();

    /**
     * @brief Go to next period (month/week/day)
     */
    void navigateNext();

private:
    /**
     * @brief Create calendar UI components
     */
    void createCalendarUI();

    /**
     * @brief Create toolbar UI
     */
    void createToolbar();

    /**
     * @brief Create month view
     */
    void createMonthView();

    /**
     * @brief Create week view
     */
    void createWeekView();

    /**
     * @brief Create day view
     */
    void createDayView();

    /**
     * @brief Create agenda view
     */
    void createAgendaView();

    /**
     * @brief Create event editor dialog
     */
    void createEventEditor();

    /**
     * @brief Update calendar display
     */
    void updateCalendarDisplay();

    /**
     * @brief Update toolbar display
     */
    void updateToolbar();

    /**
     * @brief Check for event reminders
     */
    void checkReminders();

    /**
     * @brief Format date string
     * @param date Date to format
     * @param format Format string
     * @return Formatted date string
     */
    std::string formatDate(std::time_t date, const char* format) const;

    /**
     * @brief Get current time
     * @return Current time
     */
    std::time_t getCurrentTime() const;

    /**
     * @brief Load events from storage
     * @return OS_OK on success, error code on failure
     */
    os_error_t loadEvents();

    /**
     * @brief Save events to storage
     * @return OS_OK on success, error code on failure
     */
    os_error_t saveEvents();

    // UI event callbacks
    static void todayButtonCallback(lv_event_t* e);
    static void prevButtonCallback(lv_event_t* e);
    static void nextButtonCallback(lv_event_t* e);
    static void viewButtonCallback(lv_event_t* e);
    static void addEventButtonCallback(lv_event_t* e);
    static void dateClickCallback(lv_event_t* e);
    static void eventClickCallback(lv_event_t* e);
    static void saveEventCallback(lv_event_t* e);
    static void cancelEventCallback(lv_event_t* e);

    // Calendar data
    std::vector<CalendarEvent> m_events;
    CalendarView m_currentView = CalendarView::MONTH;
    std::time_t m_currentDate;
    std::time_t m_selectedDate;

    // UI elements
    lv_obj_t* m_calendarContainer = nullptr;
    lv_obj_t* m_toolbar = nullptr;
    lv_obj_t* m_titleLabel = nullptr;
    lv_obj_t* m_todayButton = nullptr;
    lv_obj_t* m_prevButton = nullptr;
    lv_obj_t* m_nextButton = nullptr;
    lv_obj_t* m_viewButton = nullptr;
    lv_obj_t* m_addEventButton = nullptr;

    // View containers
    lv_obj_t* m_monthView = nullptr;
    lv_obj_t* m_weekView = nullptr;
    lv_obj_t* m_dayView = nullptr;
    lv_obj_t* m_agendaView = nullptr;

    // Calendar widget
    lv_obj_t* m_calendar = nullptr;
    lv_obj_t* m_eventList = nullptr;

    // Event editor
    lv_obj_t* m_eventEditor = nullptr;
    lv_obj_t* m_eventTitleInput = nullptr;
    lv_obj_t* m_eventDescInput = nullptr;
    lv_obj_t* m_eventLocationInput = nullptr;
    lv_obj_t* m_eventStartTime = nullptr;
    lv_obj_t* m_eventEndTime = nullptr;
    lv_obj_t* m_eventAllDay = nullptr;
    lv_obj_t* m_eventReminder = nullptr;
    lv_obj_t* m_saveEventButton = nullptr;
    lv_obj_t* m_cancelEventButton = nullptr;

    // State
    bool m_isEditing = false;
    std::string m_editingEventId;
    uint32_t m_lastReminderCheck = 0;

    // Configuration
    static constexpr uint32_t REMINDER_CHECK_INTERVAL = 60000; // 1 minute
    static constexpr size_t MAX_EVENTS = 1000;
    
    // Event categories and colors
    std::vector<std::pair<std::string, uint32_t>> m_categories = {
        {"Work", 0x3498DB},
        {"Personal", 0xE74C3C},
        {"Health", 0x2ECC71},
        {"Travel", 0xF39C12},
        {"Holiday", 0x9B59B6}
    };
};

#endif // CALENDAR_APP_H