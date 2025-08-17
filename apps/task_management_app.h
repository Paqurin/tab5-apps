#ifndef TASK_MANAGEMENT_APP_H
#define TASK_MANAGEMENT_APP_H

#include "base_app.h"
#include <vector>
#include <string>
#include <ctime>

enum class TaskPriority {
    TASK_LOW = 0,
    TASK_MEDIUM = 1,
    TASK_HIGH = 2,
    TASK_URGENT = 3
};

enum class TaskStatus {
    TODO = 0,
    IN_PROGRESS = 1,
    COMPLETED = 2
};

struct AppTask {
    uint32_t id;
    std::string title;
    std::string description;
    TaskPriority priority;
    TaskStatus status;
    time_t dueDate;
    time_t createdDate;
    time_t completedDate;
    std::string category;
    bool hasReminder;
    time_t reminderTime;
};

class TaskManagementApp : public BaseApp {
public:
    TaskManagementApp();
    ~TaskManagementApp() override;

    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;

private:
    void createMainUI();
    void createTaskList();
    void createTaskDetails();
    void createToolbar();
    void createFilterBar();
    void createAddEditDialog();
    void createDatePicker();
    
    void refreshTaskList();
    void showTaskDetails(const AppTask& task);
    void showAddEditDialog(AppTask* task = nullptr);
    void hideAddEditDialog();
    void filterTasks();
    void sortTasks();
    
    void addTask(const AppTask& task);
    void editTask(uint32_t taskId, const AppTask& task);
    void deleteTask(uint32_t taskId);
    void toggleTaskStatus(uint32_t taskId);
    void markTaskCompleted(uint32_t taskId);
    
    os_error_t loadTasks();
    os_error_t saveTasks();
    
    std::string priorityToString(TaskPriority priority);
    std::string statusToString(TaskStatus status);
    lv_color_t getPriorityColor(TaskPriority priority);
    
    // Callbacks
    static void taskListCallback(lv_event_t* e);
    static void addButtonCallback(lv_event_t* e);
    static void editButtonCallback(lv_event_t* e);
    static void deleteButtonCallback(lv_event_t* e);
    static void completeButtonCallback(lv_event_t* e);
    static void saveButtonCallback(lv_event_t* e);
    static void cancelButtonCallback(lv_event_t* e);
    static void filterCallback(lv_event_t* e);
    static void sortCallback(lv_event_t* e);
    static void dueDateCallback(lv_event_t* e);
    
    std::vector<AppTask> m_tasks;
    std::vector<AppTask> m_filteredTasks;
    uint32_t m_nextTaskId;
    uint32_t m_selectedTaskId;
    
    // Filtering and sorting
    TaskStatus m_statusFilter;
    TaskPriority m_priorityFilter;
    std::string m_categoryFilter;
    bool m_showCompleted;
    
    // UI elements
    lv_obj_t* m_taskList = nullptr;
    lv_obj_t* m_detailsPanel = nullptr;
    lv_obj_t* m_toolbar = nullptr;
    lv_obj_t* m_filterBar = nullptr;
    lv_obj_t* m_addEditDialog = nullptr;
    lv_obj_t* m_datePicker = nullptr;
    
    // Toolbar buttons
    lv_obj_t* m_editBtn = nullptr;
    lv_obj_t* m_deleteBtn = nullptr;
    lv_obj_t* m_completeBtn = nullptr;
    
    // Filter controls
    lv_obj_t* m_statusFilterDropdown = nullptr;
    lv_obj_t* m_priorityFilterDropdown = nullptr;
    lv_obj_t* m_sortDropdown = nullptr;
    lv_obj_t* m_showCompletedSwitch = nullptr;
    
    // Add/Edit dialog elements
    lv_obj_t* m_titleInput = nullptr;
    lv_obj_t* m_descriptionInput = nullptr;
    lv_obj_t* m_priorityInput = nullptr;
    lv_obj_t* m_categoryInput = nullptr;
    lv_obj_t* m_dueDateBtn = nullptr;
    lv_obj_t* m_reminderSwitch = nullptr;
    lv_obj_t* m_reminderTimeBtn = nullptr;
    
    AppTask* m_editingTask = nullptr;
    time_t m_selectedDueDate;
    time_t m_selectedReminderTime;
};

#endif // TASK_MANAGEMENT_APP_H