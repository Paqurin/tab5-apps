#include "calendar_app.h"
#include "../system/os_manager.h"
#include "../services/storage_service.h"
#include <esp_log.h>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

static const char* TAG = "CalendarApp";

CalendarApp::CalendarApp() : BaseApp("calendar", "Calendar", "1.0.0") {
    setDescription("Calendar and scheduling application");
    setAuthor("M5Stack Tab5 OS");
    setPriority(AppPriority::APP_NORMAL);
    
    // Initialize current date to today
    m_currentDate = getCurrentTime();
    m_selectedDate = m_currentDate;
}

CalendarApp::~CalendarApp() {
    shutdown();
}

os_error_t CalendarApp::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Initializing Calendar application");

    // Set memory usage estimate
    setMemoryUsage(64 * 1024); // 64KB

    // Load events from storage
    loadEvents();

    // Add some sample events for demonstration
    if (m_events.empty()) {
        CalendarEvent sampleEvent1 = {
            .id = "sample1",
            .title = "Team Meeting",
            .description = "Weekly team standup meeting",
            .startTime = getCurrentTime() + 3600, // 1 hour from now
            .endTime = getCurrentTime() + 7200,   // 2 hours from now
            .location = "Conference Room A",
            .isAllDay = false,
            .hasReminder = true,
            .reminderMinutes = 15,
            .category = "Work",
            .color = 0x3498DB
        };

        CalendarEvent sampleEvent2 = {
            .id = "sample2",
            .title = "Doctor Appointment",
            .description = "Annual checkup",
            .startTime = getCurrentTime() + 86400, // Tomorrow
            .endTime = getCurrentTime() + 90000,   // Tomorrow + 1 hour
            .location = "Medical Center",
            .isAllDay = false,
            .hasReminder = true,
            .reminderMinutes = 30,
            .category = "Health",
            .color = 0x2ECC71
        };

        m_events.push_back(sampleEvent1);
        m_events.push_back(sampleEvent2);
        saveEvents();
    }

    m_initialized = true;
    log(ESP_LOG_INFO, "Calendar application initialized with %d events", m_events.size());

    return OS_OK;
}

os_error_t CalendarApp::update(uint32_t deltaTime) {
    // Check for reminders periodically
    if (millis() - m_lastReminderCheck >= REMINDER_CHECK_INTERVAL) {
        checkReminders();
        m_lastReminderCheck = millis();
    }

    return OS_OK;
}

os_error_t CalendarApp::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Shutting down Calendar application");

    // Save events to storage
    saveEvents();

    // Clear events
    m_events.clear();

    m_initialized = false;
    log(ESP_LOG_INFO, "Calendar application shutdown complete");

    return OS_OK;
}

os_error_t CalendarApp::createUI(lv_obj_t* parent) {
    if (!parent) {
        return OS_ERROR_INVALID_PARAM;
    }

    log(ESP_LOG_INFO, "Creating Calendar UI");

    // Create main container
    m_uiContainer = lv_obj_create(parent);
    lv_obj_set_size(m_uiContainer, LV_HOR_RES, LV_VER_RES - OS_STATUS_BAR_HEIGHT - OS_DOCK_HEIGHT);
    lv_obj_align(m_uiContainer, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(m_uiContainer, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_opa(m_uiContainer, LV_OPA_TRANSP, 0);

    // Create calendar UI components
    createCalendarUI();

    // Set initial view
    setCalendarView(m_currentView);

    return OS_OK;
}

os_error_t CalendarApp::destroyUI() {
    if (!m_uiContainer) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Destroying Calendar UI");

    // Clean up UI elements
    if (m_uiContainer) {
        lv_obj_del(m_uiContainer);
        m_uiContainer = nullptr;
        m_calendarContainer = nullptr;
        m_toolbar = nullptr;
        m_titleLabel = nullptr;
        m_todayButton = nullptr;
        m_prevButton = nullptr;
        m_nextButton = nullptr;
        m_viewButton = nullptr;
        m_addEventButton = nullptr;
        m_monthView = nullptr;
        m_weekView = nullptr;
        m_dayView = nullptr;
        m_agendaView = nullptr;
        m_calendar = nullptr;
        m_eventList = nullptr;
        m_eventEditor = nullptr;
    }

    return OS_OK;
}

os_error_t CalendarApp::handleEvent(uint32_t eventType, void* eventData, size_t dataSize) {
    // Handle application-specific events
    return OS_OK;
}

os_error_t CalendarApp::addEvent(const CalendarEvent& event) {
    if (m_events.size() >= MAX_EVENTS) {
        log(ESP_LOG_WARN, "Maximum number of events reached");
        return OS_ERROR_BUSY;
    }

    // Check for duplicate ID
    auto it = std::find_if(m_events.begin(), m_events.end(),
                          [&event](const CalendarEvent& e) { return e.id == event.id; });

    if (it != m_events.end()) {
        log(ESP_LOG_WARN, "Event with ID '%s' already exists", event.id.c_str());
        return OS_ERROR_GENERIC;
    }

    m_events.push_back(event);
    saveEvents();
    updateCalendarDisplay();

    log(ESP_LOG_INFO, "Added event '%s'", event.title.c_str());
    return OS_OK;
}

os_error_t CalendarApp::removeEvent(const std::string& eventId) {
    auto it = std::find_if(m_events.begin(), m_events.end(),
                          [&eventId](const CalendarEvent& e) { return e.id == eventId; });

    if (it == m_events.end()) {
        return OS_ERROR_NOT_FOUND;
    }

    log(ESP_LOG_INFO, "Removing event '%s'", it->title.c_str());
    m_events.erase(it);
    saveEvents();
    updateCalendarDisplay();

    return OS_OK;
}

os_error_t CalendarApp::updateEvent(const CalendarEvent& event) {
    auto it = std::find_if(m_events.begin(), m_events.end(),
                          [&event](const CalendarEvent& e) { return e.id == event.id; });

    if (it == m_events.end()) {
        return OS_ERROR_NOT_FOUND;
    }

    *it = event;
    saveEvents();
    updateCalendarDisplay();

    log(ESP_LOG_INFO, "Updated event '%s'", event.title.c_str());
    return OS_OK;
}

std::vector<CalendarEvent> CalendarApp::getEventsForDate(std::time_t date) const {
    std::vector<CalendarEvent> dayEvents;
    
    // Get start and end of day
    struct tm* tm_date = localtime(&date);
    tm_date->tm_hour = 0;
    tm_date->tm_min = 0;
    tm_date->tm_sec = 0;
    std::time_t dayStart = mktime(tm_date);
    
    tm_date->tm_hour = 23;
    tm_date->tm_min = 59;
    tm_date->tm_sec = 59;
    std::time_t dayEnd = mktime(tm_date);

    for (const auto& event : m_events) {
        if ((event.startTime >= dayStart && event.startTime <= dayEnd) ||
            (event.endTime >= dayStart && event.endTime <= dayEnd) ||
            (event.startTime <= dayStart && event.endTime >= dayEnd)) {
            dayEvents.push_back(event);
        }
    }

    // Sort by start time
    std::sort(dayEvents.begin(), dayEvents.end(),
              [](const CalendarEvent& a, const CalendarEvent& b) {
                  return a.startTime < b.startTime;
              });

    return dayEvents;
}

std::vector<CalendarEvent> CalendarApp::getEventsInRange(std::time_t startDate, std::time_t endDate) const {
    std::vector<CalendarEvent> rangeEvents;

    for (const auto& event : m_events) {
        if ((event.startTime >= startDate && event.startTime <= endDate) ||
            (event.endTime >= startDate && event.endTime <= endDate) ||
            (event.startTime <= startDate && event.endTime >= endDate)) {
            rangeEvents.push_back(event);
        }
    }

    // Sort by start time
    std::sort(rangeEvents.begin(), rangeEvents.end(),
              [](const CalendarEvent& a, const CalendarEvent& b) {
                  return a.startTime < b.startTime;
              });

    return rangeEvents;
}

void CalendarApp::setCalendarView(CalendarView view) {
    if (m_currentView == view) {
        return;
    }

    m_currentView = view;

    // Hide all views
    if (m_monthView) lv_obj_add_flag(m_monthView, LV_OBJ_FLAG_HIDDEN);
    if (m_weekView) lv_obj_add_flag(m_weekView, LV_OBJ_FLAG_HIDDEN);
    if (m_dayView) lv_obj_add_flag(m_dayView, LV_OBJ_FLAG_HIDDEN);
    if (m_agendaView) lv_obj_add_flag(m_agendaView, LV_OBJ_FLAG_HIDDEN);

    // Show current view
    switch (view) {
        case CalendarView::MONTH:
            if (!m_monthView) createMonthView();
            lv_obj_clear_flag(m_monthView, LV_OBJ_FLAG_HIDDEN);
            break;
        case CalendarView::WEEK:
            if (!m_weekView) createWeekView();
            lv_obj_clear_flag(m_weekView, LV_OBJ_FLAG_HIDDEN);
            break;
        case CalendarView::DAY:
            if (!m_dayView) createDayView();
            lv_obj_clear_flag(m_dayView, LV_OBJ_FLAG_HIDDEN);
            break;
        case CalendarView::AGENDA:
            if (!m_agendaView) createAgendaView();
            lv_obj_clear_flag(m_agendaView, LV_OBJ_FLAG_HIDDEN);
            break;
    }

    updateToolbar();
    updateCalendarDisplay();
}

void CalendarApp::navigateToDate(std::time_t date) {
    m_currentDate = date;
    m_selectedDate = date;
    updateCalendarDisplay();
    updateToolbar();
}

void CalendarApp::navigateToToday() {
    navigateToDate(getCurrentTime());
}

void CalendarApp::navigatePrevious() {
    struct tm* tm_date = localtime(&m_currentDate);
    
    switch (m_currentView) {
        case CalendarView::MONTH:
            tm_date->tm_mon--;
            break;
        case CalendarView::WEEK:
            tm_date->tm_mday -= 7;
            break;
        case CalendarView::DAY:
            tm_date->tm_mday--;
            break;
        case CalendarView::AGENDA:
            tm_date->tm_mday -= 30; // Go back 30 days
            break;
    }
    
    m_currentDate = mktime(tm_date);
    updateCalendarDisplay();
    updateToolbar();
}

void CalendarApp::navigateNext() {
    struct tm* tm_date = localtime(&m_currentDate);
    
    switch (m_currentView) {
        case CalendarView::MONTH:
            tm_date->tm_mon++;
            break;
        case CalendarView::WEEK:
            tm_date->tm_mday += 7;
            break;
        case CalendarView::DAY:
            tm_date->tm_mday++;
            break;
        case CalendarView::AGENDA:
            tm_date->tm_mday += 30; // Go forward 30 days
            break;
    }
    
    m_currentDate = mktime(tm_date);
    updateCalendarDisplay();
    updateToolbar();
}

void CalendarApp::createCalendarUI() {
    if (!m_uiContainer) {
        return;
    }

    // Create toolbar
    createToolbar();

    // Create calendar container
    m_calendarContainer = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_calendarContainer, LV_HOR_RES - 20, LV_VER_RES - 140);
    lv_obj_align(m_calendarContainer, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(m_calendarContainer, lv_color_hex(0x2C2C2C), 0);
    lv_obj_set_style_border_opa(m_calendarContainer, LV_OPA_TRANSP, 0);
}

void CalendarApp::createToolbar() {
    if (!m_uiContainer) {
        return;
    }

    // Create toolbar container
    m_toolbar = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_toolbar, LV_HOR_RES - 20, 50);
    lv_obj_align(m_toolbar, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_bg_color(m_toolbar, lv_color_hex(0x3C3C3C), 0);
    lv_obj_set_style_border_opa(m_toolbar, LV_OPA_TRANSP, 0);

    // Create title label
    m_titleLabel = lv_label_create(m_toolbar);
    lv_obj_set_style_text_color(m_titleLabel, lv_color_white(), 0);
    lv_obj_set_style_text_font(m_titleLabel, &lv_font_montserrat_16, 0);
    lv_obj_align(m_titleLabel, LV_ALIGN_LEFT_MID, 10, 0);

    // Create navigation buttons
    m_prevButton = lv_btn_create(m_toolbar);
    lv_obj_set_size(m_prevButton, 40, 30);
    lv_obj_align(m_prevButton, LV_ALIGN_CENTER, -120, 0);
    lv_obj_t* prevLabel = lv_label_create(m_prevButton);
    lv_label_set_text(prevLabel, LV_SYMBOL_LEFT);
    lv_obj_center(prevLabel);
    lv_obj_add_event_cb(m_prevButton, prevButtonCallback, LV_EVENT_CLICKED, this);

    m_nextButton = lv_btn_create(m_toolbar);
    lv_obj_set_size(m_nextButton, 40, 30);
    lv_obj_align(m_nextButton, LV_ALIGN_CENTER, -70, 0);
    lv_obj_t* nextLabel = lv_label_create(m_nextButton);
    lv_label_set_text(nextLabel, LV_SYMBOL_RIGHT);
    lv_obj_center(nextLabel);
    lv_obj_add_event_cb(m_nextButton, nextButtonCallback, LV_EVENT_CLICKED, this);

    // Create today button
    m_todayButton = lv_btn_create(m_toolbar);
    lv_obj_set_size(m_todayButton, 60, 30);
    lv_obj_align(m_todayButton, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t* todayLabel = lv_label_create(m_todayButton);
    lv_label_set_text(todayLabel, "Today");
    lv_obj_center(todayLabel);
    lv_obj_add_event_cb(m_todayButton, todayButtonCallback, LV_EVENT_CLICKED, this);

    // Create view button
    m_viewButton = lv_btn_create(m_toolbar);
    lv_obj_set_size(m_viewButton, 60, 30);
    lv_obj_align(m_viewButton, LV_ALIGN_CENTER, 70, 0);
    lv_obj_t* viewLabel = lv_label_create(m_viewButton);
    lv_label_set_text(viewLabel, "Month");
    lv_obj_center(viewLabel);
    lv_obj_add_event_cb(m_viewButton, viewButtonCallback, LV_EVENT_CLICKED, this);

    // Create add event button
    m_addEventButton = lv_btn_create(m_toolbar);
    lv_obj_set_size(m_addEventButton, 40, 30);
    lv_obj_align(m_addEventButton, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_t* addLabel = lv_label_create(m_addEventButton);
    lv_label_set_text(addLabel, LV_SYMBOL_PLUS);
    lv_obj_center(addLabel);
    lv_obj_add_event_cb(m_addEventButton, addEventButtonCallback, LV_EVENT_CLICKED, this);
}

void CalendarApp::createMonthView() {
    if (!m_calendarContainer) {
        return;
    }

    m_monthView = lv_obj_create(m_calendarContainer);
    lv_obj_set_size(m_monthView, lv_obj_get_width(m_calendarContainer), lv_obj_get_height(m_calendarContainer));
    lv_obj_center(m_monthView);
    lv_obj_set_style_bg_opa(m_monthView, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(m_monthView, LV_OPA_TRANSP, 0);

    // Create LVGL calendar widget
    m_calendar = lv_calendar_create(m_monthView);
    lv_obj_set_size(m_calendar, lv_obj_get_width(m_monthView) - 20, lv_obj_get_height(m_monthView) - 20);
    lv_obj_center(m_calendar);
    lv_obj_add_event_cb(m_calendar, dateClickCallback, LV_EVENT_VALUE_CHANGED, this);

    // Style the calendar
    lv_obj_set_style_bg_color(m_calendar, lv_color_hex(0x2C2C2C), 0);
    lv_obj_set_style_text_color(m_calendar, lv_color_white(), 0);
}

void CalendarApp::createWeekView() {
    if (!m_calendarContainer) {
        return;
    }

    m_weekView = lv_obj_create(m_calendarContainer);
    lv_obj_set_size(m_weekView, lv_obj_get_width(m_calendarContainer), lv_obj_get_height(m_calendarContainer));
    lv_obj_center(m_weekView);
    lv_obj_set_style_bg_opa(m_weekView, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(m_weekView, LV_OPA_TRANSP, 0);

    // Create week view grid (simplified implementation)
    lv_obj_t* weekLabel = lv_label_create(m_weekView);
    lv_label_set_text(weekLabel, "Week View\n(Not fully implemented)");
    lv_obj_set_style_text_color(weekLabel, lv_color_white(), 0);
    lv_obj_center(weekLabel);
}

void CalendarApp::createDayView() {
    if (!m_calendarContainer) {
        return;
    }

    m_dayView = lv_obj_create(m_calendarContainer);
    lv_obj_set_size(m_dayView, lv_obj_get_width(m_calendarContainer), lv_obj_get_height(m_calendarContainer));
    lv_obj_center(m_dayView);
    lv_obj_set_style_bg_opa(m_dayView, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(m_dayView, LV_OPA_TRANSP, 0);

    // Create day view with hourly slots
    m_eventList = lv_list_create(m_dayView);
    lv_obj_set_size(m_eventList, lv_obj_get_width(m_dayView) - 20, lv_obj_get_height(m_dayView) - 20);
    lv_obj_center(m_eventList);
    lv_obj_set_style_bg_color(m_eventList, lv_color_hex(0x2C2C2C), 0);
}

void CalendarApp::createAgendaView() {
    if (!m_calendarContainer) {
        return;
    }

    m_agendaView = lv_obj_create(m_calendarContainer);
    lv_obj_set_size(m_agendaView, lv_obj_get_width(m_calendarContainer), lv_obj_get_height(m_calendarContainer));
    lv_obj_center(m_agendaView);
    lv_obj_set_style_bg_opa(m_agendaView, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(m_agendaView, LV_OPA_TRANSP, 0);

    // Create agenda list
    m_eventList = lv_list_create(m_agendaView);
    lv_obj_set_size(m_eventList, lv_obj_get_width(m_agendaView) - 20, lv_obj_get_height(m_agendaView) - 20);
    lv_obj_center(m_eventList);
    lv_obj_set_style_bg_color(m_eventList, lv_color_hex(0x2C2C2C), 0);
}

void CalendarApp::updateCalendarDisplay() {
    if (!m_uiContainer) {
        return;
    }

    switch (m_currentView) {
        case CalendarView::MONTH:
            if (m_calendar) {
                // Update calendar widget (LVGL calendar automatically shows current month)
                // Add event indicators if needed
            }
            break;

        case CalendarView::DAY:
        case CalendarView::AGENDA:
            if (m_eventList) {
                // Clear existing items
                lv_obj_clean(m_eventList);

                // Get events for current period
                std::vector<CalendarEvent> events;
                if (m_currentView == CalendarView::DAY) {
                    events = getEventsForDate(m_currentDate);
                } else {
                    // Agenda view shows next 30 days
                    events = getEventsInRange(m_currentDate, m_currentDate + 30 * 86400);
                }

                // Add events to list
                for (const auto& event : events) {
                    lv_obj_t* item = lv_list_add_btn(m_eventList, LV_SYMBOL_CALL, event.title.c_str());
                    lv_obj_set_user_data(item, (void*)event.id.c_str());
                    lv_obj_add_event_cb(item, eventClickCallback, LV_EVENT_CLICKED, this);
                    
                    // Set event color
                    lv_obj_set_style_bg_color(item, lv_color_hex(event.color), LV_STATE_DEFAULT);
                }

                if (events.empty()) {
                    lv_obj_t* item = lv_list_add_text(m_eventList, "No events");
                    lv_obj_set_style_text_color(item, lv_color_hex(0x888888), 0);
                }
            }
            break;

        case CalendarView::WEEK:
            // Week view implementation would go here
            break;
    }
}

void CalendarApp::updateToolbar() {
    if (!m_titleLabel) {
        return;
    }

    // Update title based on current view and date
    std::string title;
    switch (m_currentView) {
        case CalendarView::MONTH:
            title = formatDate(m_currentDate, "%B %Y");
            break;
        case CalendarView::WEEK:
            title = "Week of " + formatDate(m_currentDate, "%b %d");
            break;
        case CalendarView::DAY:
            title = formatDate(m_currentDate, "%A, %B %d");
            break;
        case CalendarView::AGENDA:
            title = "Agenda";
            break;
    }

    lv_label_set_text(m_titleLabel, title.c_str());
}

void CalendarApp::checkReminders() {
    std::time_t now = getCurrentTime();
    
    for (const auto& event : m_events) {
        if (event.hasReminder) {
            std::time_t reminderTime = event.startTime - (event.reminderMinutes * 60);
            
            // Check if it's time for reminder (within 1 minute window)
            if (now >= reminderTime && now <= reminderTime + 60) {
                // Trigger reminder notification
                log(ESP_LOG_INFO, "Reminder: %s in %d minutes", 
                    event.title.c_str(), event.reminderMinutes);
                
                // In a real implementation, this would trigger a system notification
                // For now, we just log it
            }
        }
    }
}

std::string CalendarApp::formatDate(std::time_t date, const char* format) const {
    struct tm* tm_date = localtime(&date);
    char buffer[256];
    strftime(buffer, sizeof(buffer), format, tm_date);
    return std::string(buffer);
}

std::time_t CalendarApp::getCurrentTime() const {
    return time(nullptr);
}

os_error_t CalendarApp::loadEvents() {
    // In a real implementation, this would load from persistent storage
    log(ESP_LOG_INFO, "Loading events from storage");
    return OS_OK;
}

os_error_t CalendarApp::saveEvents() {
    // In a real implementation, this would save to persistent storage
    log(ESP_LOG_INFO, "Saving %d events to storage", m_events.size());
    return OS_OK;
}

// UI Event Callbacks

void CalendarApp::todayButtonCallback(lv_event_t* e) {
    CalendarApp* app = static_cast<CalendarApp*>(lv_event_get_user_data(e));
    if (app) {
        app->navigateToToday();
    }
}

void CalendarApp::prevButtonCallback(lv_event_t* e) {
    CalendarApp* app = static_cast<CalendarApp*>(lv_event_get_user_data(e));
    if (app) {
        app->navigatePrevious();
    }
}

void CalendarApp::nextButtonCallback(lv_event_t* e) {
    CalendarApp* app = static_cast<CalendarApp*>(lv_event_get_user_data(e));
    if (app) {
        app->navigateNext();
    }
}

void CalendarApp::viewButtonCallback(lv_event_t* e) {
    CalendarApp* app = static_cast<CalendarApp*>(lv_event_get_user_data(e));
    if (app) {
        // Cycle through views
        CalendarView newView = static_cast<CalendarView>((static_cast<int>(app->m_currentView) + 1) % 4);
        app->setCalendarView(newView);
    }
}

void CalendarApp::addEventButtonCallback(lv_event_t* e) {
    CalendarApp* app = static_cast<CalendarApp*>(lv_event_get_user_data(e));
    if (app) {
        // Create new event dialog
        app->createEventEditor();
    }
}

void CalendarApp::dateClickCallback(lv_event_t* e) {
    CalendarApp* app = static_cast<CalendarApp*>(lv_event_get_user_data(e));
    lv_obj_t* calendar = lv_event_get_target(e);
    
    if (app && calendar) {
        // Get selected date from calendar
        lv_calendar_date_t date;
        if (lv_calendar_get_pressed_date(calendar, &date) == LV_RES_OK) {
            struct tm tm_date = {0};
            tm_date.tm_year = date.year - 1900;
            tm_date.tm_mon = date.month - 1;
            tm_date.tm_mday = date.day;
            
            std::time_t selectedDate = mktime(&tm_date);
            app->m_selectedDate = selectedDate;
            
            // Switch to day view for selected date
            app->navigateToDate(selectedDate);
            app->setCalendarView(CalendarView::DAY);
        }
    }
}

void CalendarApp::eventClickCallback(lv_event_t* e) {
    CalendarApp* app = static_cast<CalendarApp*>(lv_event_get_user_data(e));
    lv_obj_t* item = lv_event_get_target(e);
    
    if (app && item) {
        const char* eventId = static_cast<const char*>(lv_obj_get_user_data(item));
        if (eventId) {
            // Find and edit the event
            auto it = std::find_if(app->m_events.begin(), app->m_events.end(),
                                  [eventId](const CalendarEvent& e) { return e.id == eventId; });
            
            if (it != app->m_events.end()) {
                app->m_editingEventId = eventId;
                app->createEventEditor();
            }
        }
    }
}

void CalendarApp::saveEventCallback(lv_event_t* e) {
    // Implementation for saving edited event
}

void CalendarApp::cancelEventCallback(lv_event_t* e) {
    // Implementation for canceling event editing
}

void CalendarApp::createEventEditor() {
    // Implementation for event editor dialog would go here
    log(ESP_LOG_INFO, "Opening event editor");
}