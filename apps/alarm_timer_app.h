#ifndef ALARM_TIMER_APP_H
#define ALARM_TIMER_APP_H

#include "base_app.h"
#include <vector>
#include <string>
#include <ctime>

struct Alarm {
    uint32_t id;
    std::string name;
    uint8_t hour;
    uint8_t minute;
    bool enabled;
    bool repeat;
    uint8_t repeatDays; // Bitmask: Sunday=1, Monday=2, etc.
    std::string ringtone;
    uint8_t volume;
    bool snoozeEnabled;
    uint8_t snoozeMinutes;
    time_t nextTrigger;
};

struct Timer {
    uint32_t id;
    std::string name;
    uint32_t totalSeconds;
    uint32_t remainingSeconds;
    bool isRunning;
    bool isPaused;
    time_t startTime;
    time_t pauseTime;
    std::string ringtone;
    uint8_t volume;
};

enum class AppMode {
    CLOCK,
    ALARMS,
    TIMERS,
    STOPWATCH
};

class AlarmTimerApp : public BaseApp {
public:
    AlarmTimerApp();
    ~AlarmTimerApp() override;

    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;

private:
    void createMainUI();
    void createTabView();
    void createClockTab();
    void createAlarmsTab();
    void createTimersTab();
    void createStopwatchTab();
    
    // Clock functions
    void updateClock();
    void createDigitalClock();
    void createAnalogClock();
    
    // Alarm functions
    void createAlarmList();
    void createAlarmDialog();
    void showAlarmDialog(Alarm* alarm = nullptr);
    void hideAlarmDialog();
    void addAlarm(const Alarm& alarm);
    void editAlarm(uint32_t alarmId, const Alarm& alarm);
    void deleteAlarm(uint32_t alarmId);
    void toggleAlarm(uint32_t alarmId);
    void checkAlarms();
    void triggerAlarm(const Alarm& alarm);
    void snoozeAlarm(uint32_t alarmId);
    void dismissAlarm(uint32_t alarmId);
    
    // Timer functions
    void createTimerList();
    void createTimerDialog();
    void showTimerDialog(Timer* timer = nullptr);
    void hideTimerDialog();
    void addTimer(const Timer& timer);
    void editTimer(uint32_t timerId, const Timer& timer);
    void deleteTimer(uint32_t timerId);
    void startTimer(uint32_t timerId);
    void pauseTimer(uint32_t timerId);
    void stopTimer(uint32_t timerId);
    void resetTimer(uint32_t timerId);
    void updateTimers();
    void checkTimerAlerts();
    
    // Stopwatch functions
    void createStopwatch();
    void startStopwatch();
    void pauseStopwatch();
    void resetStopwatch();
    void lapStopwatch();
    void updateStopwatch();
    
    // Utility functions
    void playSound(const std::string& ringtone, uint8_t volume);
    void stopSound();
    std::string formatTime(uint8_t hour, uint8_t minute);
    std::string formatDuration(uint32_t seconds);
    std::string getDayName(uint8_t dayIndex);
    void calculateNextAlarmTrigger(Alarm& alarm);
    
    os_error_t loadAlarms();
    os_error_t saveAlarms();
    os_error_t loadTimers();
    os_error_t saveTimers();
    
    // Event callbacks
    static void tabChangedCallback(lv_event_t* e);
    static void alarmToggleCallback(lv_event_t* e);
    static void alarmEditCallback(lv_event_t* e);
    static void alarmDeleteCallback(lv_event_t* e);
    static void addAlarmCallback(lv_event_t* e);
    static void saveAlarmCallback(lv_event_t* e);
    static void cancelAlarmCallback(lv_event_t* e);
    static void timerStartCallback(lv_event_t* e);
    static void timerPauseCallback(lv_event_t* e);
    static void timerStopCallback(lv_event_t* e);
    static void timerResetCallback(lv_event_t* e);
    static void addTimerCallback(lv_event_t* e);
    static void saveTimerCallback(lv_event_t* e);
    static void cancelTimerCallback(lv_event_t* e);
    static void stopwatchStartCallback(lv_event_t* e);
    static void stopwatchPauseCallback(lv_event_t* e);
    static void stopwatchResetCallback(lv_event_t* e);
    static void stopwatchLapCallback(lv_event_t* e);
    static void snoozeCallback(lv_event_t* e);
    static void dismissCallback(lv_event_t* e);
    
    // Data
    std::vector<Alarm> m_alarms;
    std::vector<Timer> m_timers;
    std::vector<std::string> m_lapTimes;
    uint32_t m_nextAlarmId;
    uint32_t m_nextTimerId;
    
    // State
    AppMode m_currentMode;
    bool m_24HourFormat;
    bool m_soundEnabled;
    uint32_t m_activeAlarmId;
    uint32_t m_stopwatchTime;
    bool m_stopwatchRunning;
    time_t m_stopwatchStartTime;
    time_t m_stopwatchPauseTime;
    
    // UI elements
    lv_obj_t* m_tabView = nullptr;
    lv_obj_t* m_clockTab = nullptr;
    lv_obj_t* m_alarmsTab = nullptr;
    lv_obj_t* m_timersTab = nullptr;
    lv_obj_t* m_stopwatchTab = nullptr;
    
    // Clock UI
    lv_obj_t* m_digitalClock = nullptr;
    lv_obj_t* m_analogClock = nullptr;
    lv_obj_t* m_dateLabel = nullptr;
    
    // Alarm UI
    lv_obj_t* m_alarmList = nullptr;
    lv_obj_t* m_alarmDialog = nullptr;
    lv_obj_t* m_alarmAlert = nullptr;
    lv_obj_t* m_alarmNameInput = nullptr;
    lv_obj_t* m_alarmHourRoller = nullptr;
    lv_obj_t* m_alarmMinuteRoller = nullptr;
    lv_obj_t* m_alarmRepeatSwitch = nullptr;
    lv_obj_t* m_alarmDaysCheckboxes[7] = {nullptr};
    lv_obj_t* m_alarmVolumeSlider = nullptr;
    
    // Timer UI
    lv_obj_t* m_timerList = nullptr;
    lv_obj_t* m_timerDialog = nullptr;
    lv_obj_t* m_timerNameInput = nullptr;
    lv_obj_t* m_timerHourRoller = nullptr;
    lv_obj_t* m_timerMinuteRoller = nullptr;
    lv_obj_t* m_timerSecondRoller = nullptr;
    
    // Stopwatch UI
    lv_obj_t* m_stopwatchDisplay = nullptr;
    lv_obj_t* m_stopwatchStartBtn = nullptr;
    lv_obj_t* m_stopwatchPauseBtn = nullptr;
    lv_obj_t* m_stopwatchResetBtn = nullptr;
    lv_obj_t* m_stopwatchLapBtn = nullptr;
    lv_obj_t* m_lapList = nullptr;
    
    Alarm* m_editingAlarm = nullptr;
    Timer* m_editingTimer = nullptr;
};

#endif // ALARM_TIMER_APP_H