#include "task_management_app.h"
#include "../system/os_manager.h"
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>

TaskManagementApp::TaskManagementApp() 
    : BaseApp("com.m5stack.tasks", "Tasks", "1.0.0"), 
      m_nextTaskId(1), m_selectedTaskId(0),
      m_statusFilter(TaskStatus::TODO), m_priorityFilter(TaskPriority::TASK_LOW),
      m_showCompleted(false), m_selectedDueDate(0), m_selectedReminderTime(0) {
    setDescription("Task management application with to-do lists and prioritization");
    setAuthor("M5Stack");
    setPriority(AppPriority::APP_NORMAL);
}

TaskManagementApp::~TaskManagementApp() {
    shutdown();
}

os_error_t TaskManagementApp::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Initializing Task Management App");
    
    // Load existing tasks
    loadTasks();
    
    // Set memory usage estimate
    setMemoryUsage(40960); // 40KB estimated usage
    
    m_initialized = true;
    return OS_OK;
}

os_error_t TaskManagementApp::update(uint32_t deltaTime) {
    // Check for reminder notifications
    time_t currentTime = time(nullptr);
    for (const auto& task : m_tasks) {
        if (task.hasReminder && task.status != TaskStatus::COMPLETED && 
            task.reminderTime > 0 && currentTime >= task.reminderTime) {
            // Trigger reminder notification
            log(ESP_LOG_INFO, "Reminder: %s", task.title.c_str());
        }
    }
    
    return OS_OK;
}

os_error_t TaskManagementApp::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Shutting down Task Management App");
    
    // Save tasks before shutdown
    saveTasks();
    
    m_initialized = false;
    return OS_OK;
}

os_error_t TaskManagementApp::createUI(lv_obj_t* parent) {
    if (!parent) {
        return OS_ERROR_INVALID_PARAM;
    }

    // Create main container
    m_uiContainer = lv_obj_create(parent);
    lv_obj_set_size(m_uiContainer, LV_HOR_RES, 
                    LV_VER_RES - 60 - 40); // Account for status bar and dock
    lv_obj_align(m_uiContainer, LV_ALIGN_CENTER, 0, 0);
    
    // Apply theme
    lv_obj_set_style_bg_color(m_uiContainer, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_opa(m_uiContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(m_uiContainer, 10, 0);

    createMainUI();
    return OS_OK;
}

os_error_t TaskManagementApp::destroyUI() {
    if (m_uiContainer) {
        lv_obj_del(m_uiContainer);
        m_uiContainer = nullptr;
    }
    return OS_OK;
}

void TaskManagementApp::createMainUI() {
    createFilterBar();
    createToolbar();
    createTaskList();
    createTaskDetails();
    
    // Initially hide details panel
    lv_obj_add_flag(m_detailsPanel, LV_OBJ_FLAG_HIDDEN);
    
    refreshTaskList();
}

void TaskManagementApp::createFilterBar() {
    m_filterBar = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_filterBar, LV_PCT(100), 50);
    lv_obj_align(m_filterBar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(m_filterBar, lv_color_hex(0x2C2C2C), 0);
    lv_obj_set_style_pad_all(m_filterBar, 5, 0);
    
    // Status filter
    lv_obj_t* statusLabel = lv_label_create(m_filterBar);
    lv_label_set_text(statusLabel, "Status:");
    lv_obj_align(statusLabel, LV_ALIGN_LEFT_MID, 0, 0);
    
    m_statusFilterDropdown = lv_dropdown_create(m_filterBar);
    lv_obj_set_size(m_statusFilterDropdown, 100, 35);
    lv_obj_align_to(m_statusFilterDropdown, statusLabel, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_dropdown_set_options(m_statusFilterDropdown, "All\nTo Do\nIn Progress\nCompleted");
    lv_obj_add_event_cb(m_statusFilterDropdown, filterCallback, LV_EVENT_VALUE_CHANGED, this);
    
    // Priority filter
    lv_obj_t* priorityLabel = lv_label_create(m_filterBar);
    lv_label_set_text(priorityLabel, "Priority:");
    lv_obj_align_to(priorityLabel, m_statusFilterDropdown, LV_ALIGN_OUT_RIGHT_MID, 15, 0);
    
    m_priorityFilterDropdown = lv_dropdown_create(m_filterBar);
    lv_obj_set_size(m_priorityFilterDropdown, 100, 35);
    lv_obj_align_to(m_priorityFilterDropdown, priorityLabel, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_dropdown_set_options(m_priorityFilterDropdown, "All\nLow\nMedium\nHigh\nUrgent");
    lv_obj_add_event_cb(m_priorityFilterDropdown, filterCallback, LV_EVENT_VALUE_CHANGED, this);
    
    // Sort options
    lv_obj_t* sortLabel = lv_label_create(m_filterBar);
    lv_label_set_text(sortLabel, "Sort:");
    lv_obj_align_to(sortLabel, m_priorityFilterDropdown, LV_ALIGN_OUT_RIGHT_MID, 15, 0);
    
    m_sortDropdown = lv_dropdown_create(m_filterBar);
    lv_obj_set_size(m_sortDropdown, 120, 35);
    lv_obj_align_to(m_sortDropdown, sortLabel, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_dropdown_set_options(m_sortDropdown, "Due Date\nPriority\nCreated\nTitle");
    lv_obj_add_event_cb(m_sortDropdown, sortCallback, LV_EVENT_VALUE_CHANGED, this);
}

void TaskManagementApp::createToolbar() {
    m_toolbar = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_toolbar, LV_PCT(100), 50);
    lv_obj_align_to(m_toolbar, m_filterBar, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_obj_set_style_bg_color(m_toolbar, lv_color_hex(0x2C2C2C), 0);
    lv_obj_set_style_pad_all(m_toolbar, 5, 0);
    
    // Add button
    lv_obj_t* addBtn = lv_btn_create(m_toolbar);
    lv_obj_set_size(addBtn, 80, LV_PCT(100));
    lv_obj_align(addBtn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(addBtn, addButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(addBtn, lv_color_hex(0x3498DB), 0);
    
    lv_obj_t* addLabel = lv_label_create(addBtn);
    lv_label_set_text(addLabel, "Add");
    lv_obj_center(addLabel);
    
    // Edit button
    m_editBtn = lv_btn_create(m_toolbar);
    lv_obj_set_size(m_editBtn, 80, LV_PCT(100));
    lv_obj_align_to(m_editBtn, addBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(m_editBtn, editButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_add_state(m_editBtn, LV_STATE_DISABLED);
    
    lv_obj_t* editLabel = lv_label_create(m_editBtn);
    lv_label_set_text(editLabel, "Edit");
    lv_obj_center(editLabel);
    
    // Complete button
    m_completeBtn = lv_btn_create(m_toolbar);
    lv_obj_set_size(m_completeBtn, 100, LV_PCT(100));
    lv_obj_align_to(m_completeBtn, m_editBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(m_completeBtn, completeButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(m_completeBtn, lv_color_hex(0x27AE60), 0);
    lv_obj_add_state(m_completeBtn, LV_STATE_DISABLED);
    
    lv_obj_t* completeLabel = lv_label_create(m_completeBtn);
    lv_label_set_text(completeLabel, "Complete");
    lv_obj_center(completeLabel);
    
    // Delete button
    m_deleteBtn = lv_btn_create(m_toolbar);
    lv_obj_set_size(m_deleteBtn, 80, LV_PCT(100));
    lv_obj_align_to(m_deleteBtn, m_completeBtn, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_add_event_cb(m_deleteBtn, deleteButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(m_deleteBtn, lv_color_hex(0xE74C3C), 0);
    lv_obj_add_state(m_deleteBtn, LV_STATE_DISABLED);
    
    lv_obj_t* deleteLabel = lv_label_create(m_deleteBtn);
    lv_label_set_text(deleteLabel, "Delete");
    lv_obj_center(deleteLabel);
}

void TaskManagementApp::createTaskList() {
    m_taskList = lv_list_create(m_uiContainer);
    lv_obj_set_size(m_taskList, LV_PCT(50), LV_PCT(100) - 110);
    lv_obj_align_to(m_taskList, m_toolbar, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_set_style_bg_color(m_taskList, lv_color_hex(0x2C2C2C), 0);
}

void TaskManagementApp::createTaskDetails() {
    m_detailsPanel = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_detailsPanel, LV_PCT(45), LV_PCT(100) - 110);
    lv_obj_align_to(m_detailsPanel, m_taskList, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    lv_obj_set_style_bg_color(m_detailsPanel, lv_color_hex(0x2C2C2C), 0);
    lv_obj_set_style_pad_all(m_detailsPanel, 15, 0);
}

void TaskManagementApp::refreshTaskList() {
    lv_obj_clean(m_taskList);
    
    // Apply filters and sorting
    filterTasks();
    sortTasks();
    
    for (const auto& task : m_filteredTasks) {
        // Create task item
        std::string itemText = task.title;
        if (task.status == TaskStatus::COMPLETED) {
            itemText = LV_SYMBOL_OK " " + itemText;
        } else if (task.status == TaskStatus::IN_PROGRESS) {
            itemText = LV_SYMBOL_PLAY " " + itemText;
        } else {
            itemText = LV_SYMBOL_BULLET " " + itemText;
        }
        
        lv_obj_t* btn = lv_list_add_btn(m_taskList, nullptr, itemText.c_str());
        lv_obj_set_user_data(btn, (void*)(uintptr_t)task.id);
        lv_obj_add_event_cb(btn, taskListCallback, LV_EVENT_CLICKED, this);
        
        // Set priority color
        lv_color_t priorityColor = getPriorityColor(task.priority);
        lv_obj_set_style_border_color(btn, priorityColor, 0);
        lv_obj_set_style_border_width(btn, 3, 0);
        lv_obj_set_style_border_side(btn, LV_BORDER_SIDE_LEFT, 0);
        
        // Add due date if set
        if (task.dueDate > 0) {
            struct tm* timeinfo = localtime(&task.dueDate);
            char dateStr[32];
            strftime(dateStr, sizeof(dateStr), "%m/%d", timeinfo);
            
            lv_obj_t* dateLabel = lv_label_create(btn);
            lv_label_set_text(dateLabel, dateStr);
            lv_obj_align(dateLabel, LV_ALIGN_BOTTOM_RIGHT, -5, -2);
            lv_obj_set_style_text_color(dateLabel, lv_color_hex(0x95A5A6), 0);
            lv_obj_set_style_text_font(dateLabel, &lv_font_montserrat_12, 0);
        }
        
        // Strike through completed tasks
        if (task.status == TaskStatus::COMPLETED) {
            lv_obj_set_style_text_decor(btn, LV_TEXT_DECOR_STRIKETHROUGH, 0);
            lv_obj_set_style_text_color(btn, lv_color_hex(0x7F8C8D), 0);
        }
    }
}

void TaskManagementApp::showTaskDetails(const AppTask& task) {
    lv_obj_clean(m_detailsPanel);
    lv_obj_clear_flag(m_detailsPanel, LV_OBJ_FLAG_HIDDEN);
    
    // Enable toolbar buttons
    lv_obj_clear_state(m_editBtn, LV_STATE_DISABLED);
    lv_obj_clear_state(m_deleteBtn, LV_STATE_DISABLED);
    if (task.status != TaskStatus::COMPLETED) {
        lv_obj_clear_state(m_completeBtn, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(m_completeBtn, LV_STATE_DISABLED);
    }
    
    // Title
    lv_obj_t* titleLabel = lv_label_create(m_detailsPanel);
    lv_label_set_text(titleLabel, task.title.c_str());
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_16, 0);
    lv_label_set_long_mode(titleLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(titleLabel, LV_PCT(90));
    
    // Status and Priority
    lv_obj_t* statusLabel = lv_label_create(m_detailsPanel);
    std::string statusText = "Status: " + statusToString(task.status);
    lv_label_set_text(statusLabel, statusText.c_str());
    lv_obj_align_to(statusLabel, titleLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 20);
    
    lv_obj_t* priorityLabel = lv_label_create(m_detailsPanel);
    std::string priorityText = "Priority: " + priorityToString(task.priority);
    lv_label_set_text(priorityLabel, priorityText.c_str());
    lv_obj_align_to(priorityLabel, statusLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    lv_obj_set_style_text_color(priorityLabel, getPriorityColor(task.priority), 0);
    
    // Description
    if (!task.description.empty()) {
        lv_obj_t* descLabel = lv_label_create(m_detailsPanel);
        lv_label_set_text(descLabel, task.description.c_str());
        lv_obj_align_to(descLabel, priorityLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 15);
        lv_label_set_long_mode(descLabel, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(descLabel, LV_PCT(90));
    }
    
    // Due date
    if (task.dueDate > 0) {
        struct tm* timeinfo = localtime(&task.dueDate);
        char dateStr[64];
        strftime(dateStr, sizeof(dateStr), "Due: %B %d, %Y", timeinfo);
        
        lv_obj_t* dueDateLabel = lv_label_create(m_detailsPanel);
        lv_label_set_text(dueDateLabel, dateStr);
        lv_obj_align(dueDateLabel, LV_ALIGN_BOTTOM_LEFT, 0, -40);
        
        // Check if overdue
        time_t currentTime = time(nullptr);
        if (currentTime > task.dueDate && task.status != TaskStatus::COMPLETED) {
            lv_obj_set_style_text_color(dueDateLabel, lv_color_hex(0xE74C3C), 0);
        }
    }
    
    // Category
    if (!task.category.empty()) {
        lv_obj_t* categoryLabel = lv_label_create(m_detailsPanel);
        std::string categoryText = "Category: " + task.category;
        lv_label_set_text(categoryLabel, categoryText.c_str());
        lv_obj_align(categoryLabel, LV_ALIGN_BOTTOM_LEFT, 0, -20);
    }
}

void TaskManagementApp::createAddEditDialog() {
    m_addEditDialog = lv_obj_create(lv_scr_act());
    lv_obj_set_size(m_addEditDialog, 450, 400);
    lv_obj_center(m_addEditDialog);
    lv_obj_set_style_bg_color(m_addEditDialog, lv_color_hex(0x2C2C2C), 0);
    lv_obj_set_style_border_color(m_addEditDialog, lv_color_hex(0x3498DB), 0);
    lv_obj_set_style_border_width(m_addEditDialog, 2, 0);
    
    // Title
    lv_obj_t* title = lv_label_create(m_addEditDialog);
    lv_label_set_text(title, m_editingTask ? "Edit Task" : "Add Task");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    
    // Title input
    lv_obj_t* titleLabel = lv_label_create(m_addEditDialog);
    lv_label_set_text(titleLabel, "Title:");
    lv_obj_align_to(titleLabel, title, LV_ALIGN_OUT_BOTTOM_LEFT, -170, 20);
    
    m_titleInput = lv_textarea_create(m_addEditDialog);
    lv_obj_set_size(m_titleInput, 350, 30);
    lv_obj_align_to(m_titleInput, titleLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_textarea_set_one_line(m_titleInput, true);
    
    // Description input
    lv_obj_t* descLabel = lv_label_create(m_addEditDialog);
    lv_label_set_text(descLabel, "Description:");
    lv_obj_align_to(descLabel, m_titleInput, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    
    m_descriptionInput = lv_textarea_create(m_addEditDialog);
    lv_obj_set_size(m_descriptionInput, 350, 60);
    lv_obj_align_to(m_descriptionInput, descLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    
    // Priority and Category in same row
    lv_obj_t* priorityLabel = lv_label_create(m_addEditDialog);
    lv_label_set_text(priorityLabel, "Priority:");
    lv_obj_align_to(priorityLabel, m_descriptionInput, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    
    m_priorityInput = lv_dropdown_create(m_addEditDialog);
    lv_obj_set_size(m_priorityInput, 120, 30);
    lv_obj_align_to(m_priorityInput, priorityLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_dropdown_set_options(m_priorityInput, "Low\nMedium\nHigh\nUrgent");
    
    lv_obj_t* categoryLabel = lv_label_create(m_addEditDialog);
    lv_label_set_text(categoryLabel, "Category:");
    lv_obj_align_to(categoryLabel, priorityLabel, LV_ALIGN_OUT_RIGHT_MID, 50, 0);
    
    m_categoryInput = lv_textarea_create(m_addEditDialog);
    lv_obj_set_size(m_categoryInput, 150, 30);
    lv_obj_align_to(m_categoryInput, categoryLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_textarea_set_one_line(m_categoryInput, true);
    lv_textarea_set_placeholder_text(m_categoryInput, "Work, Personal, etc.");
    
    // Due date
    lv_obj_t* dueDateLabel = lv_label_create(m_addEditDialog);
    lv_label_set_text(dueDateLabel, "Due Date:");
    lv_obj_align_to(dueDateLabel, m_priorityInput, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 15);
    
    m_dueDateBtn = lv_btn_create(m_addEditDialog);
    lv_obj_set_size(m_dueDateBtn, 150, 30);
    lv_obj_align_to(m_dueDateBtn, dueDateLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_obj_add_event_cb(m_dueDateBtn, dueDateCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* dueDateBtnLabel = lv_label_create(m_dueDateBtn);
    lv_label_set_text(dueDateBtnLabel, "Select Date");
    lv_obj_center(dueDateBtnLabel);
    
    // Buttons
    lv_obj_t* saveBtn = lv_btn_create(m_addEditDialog);
    lv_obj_set_size(saveBtn, 80, 35);
    lv_obj_align(saveBtn, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_add_event_cb(saveBtn, saveButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(saveBtn, lv_color_hex(0x27AE60), 0);
    
    lv_obj_t* saveLabel = lv_label_create(saveBtn);
    lv_label_set_text(saveLabel, "Save");
    lv_obj_center(saveLabel);
    
    lv_obj_t* cancelBtn = lv_btn_create(m_addEditDialog);
    lv_obj_set_size(cancelBtn, 80, 35);
    lv_obj_align(cancelBtn, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_add_event_cb(cancelBtn, cancelButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(cancelBtn, lv_color_hex(0xE74C3C), 0);
    
    lv_obj_t* cancelLabel = lv_label_create(cancelBtn);
    lv_label_set_text(cancelLabel, "Cancel");
    lv_obj_center(cancelLabel);
    
    // Pre-fill if editing
    if (m_editingTask) {
        lv_textarea_set_text(m_titleInput, m_editingTask->title.c_str());
        lv_textarea_set_text(m_descriptionInput, m_editingTask->description.c_str());
        lv_textarea_set_text(m_categoryInput, m_editingTask->category.c_str());
        lv_dropdown_set_selected(m_priorityInput, (uint16_t)m_editingTask->priority);
        m_selectedDueDate = m_editingTask->dueDate;
    }
}

void TaskManagementApp::showAddEditDialog(AppTask* task) {
    m_editingTask = task;
    createAddEditDialog();
}

void TaskManagementApp::hideAddEditDialog() {
    if (m_addEditDialog) {
        lv_obj_del(m_addEditDialog);
        m_addEditDialog = nullptr;
        m_editingTask = nullptr;
    }
}

void TaskManagementApp::filterTasks() {
    m_filteredTasks.clear();
    
    for (const auto& task : m_tasks) {
        bool includeTask = true;
        
        // Status filter
        if (lv_dropdown_get_selected(m_statusFilterDropdown) > 0) {
            TaskStatus filterStatus = (TaskStatus)(lv_dropdown_get_selected(m_statusFilterDropdown) - 1);
            if (task.status != filterStatus) {
                includeTask = false;
            }
        }
        
        // Priority filter
        if (lv_dropdown_get_selected(m_priorityFilterDropdown) > 0) {
            TaskPriority filterPriority = (TaskPriority)(lv_dropdown_get_selected(m_priorityFilterDropdown) - 1);
            if (task.priority != filterPriority) {
                includeTask = false;
            }
        }
        
        // Hide completed tasks if needed
        if (!m_showCompleted && task.status == TaskStatus::COMPLETED) {
            includeTask = false;
        }
        
        if (includeTask) {
            m_filteredTasks.push_back(task);
        }
    }
}

void TaskManagementApp::sortTasks() {
    uint16_t sortOption = lv_dropdown_get_selected(m_sortDropdown);
    
    std::sort(m_filteredTasks.begin(), m_filteredTasks.end(), 
              [sortOption](const AppTask& a, const AppTask& b) {
        switch (sortOption) {
            case 0: // Due Date
                if (a.dueDate == 0 && b.dueDate == 0) return a.createdDate > b.createdDate;
                if (a.dueDate == 0) return false;
                if (b.dueDate == 0) return true;
                return a.dueDate < b.dueDate;
            case 1: // Priority
                return a.priority > b.priority;
            case 2: // Created
                return a.createdDate > b.createdDate;
            case 3: // Title
                return a.title < b.title;
            default:
                return false;
        }
    });
}

void TaskManagementApp::addTask(const AppTask& task) {
    AppTask newTask = task;
    newTask.id = m_nextTaskId++;
    newTask.createdDate = time(nullptr);
    newTask.status = TaskStatus::TODO;
    m_tasks.push_back(newTask);
    saveTasks();
    refreshTaskList();
}

void TaskManagementApp::editTask(uint32_t taskId, const AppTask& task) {
    for (auto& t : m_tasks) {
        if (t.id == taskId) {
            t.title = task.title;
            t.description = task.description;
            t.priority = task.priority;
            t.category = task.category;
            t.dueDate = task.dueDate;
            t.hasReminder = task.hasReminder;
            t.reminderTime = task.reminderTime;
            break;
        }
    }
    saveTasks();
    refreshTaskList();
}

void TaskManagementApp::deleteTask(uint32_t taskId) {
    m_tasks.erase(
        std::remove_if(m_tasks.begin(), m_tasks.end(),
                      [taskId](const AppTask& t) { return t.id == taskId; }),
        m_tasks.end());
    saveTasks();
    refreshTaskList();
    lv_obj_add_flag(m_detailsPanel, LV_OBJ_FLAG_HIDDEN);
    
    // Disable toolbar buttons
    lv_obj_add_state(m_editBtn, LV_STATE_DISABLED);
    lv_obj_add_state(m_deleteBtn, LV_STATE_DISABLED);
    lv_obj_add_state(m_completeBtn, LV_STATE_DISABLED);
}

void TaskManagementApp::markTaskCompleted(uint32_t taskId) {
    for (auto& task : m_tasks) {
        if (task.id == taskId) {
            task.status = TaskStatus::COMPLETED;
            task.completedDate = time(nullptr);
            break;
        }
    }
    saveTasks();
    refreshTaskList();
}

os_error_t TaskManagementApp::loadTasks() {
    // Add some sample tasks
    AppTask task1 = {m_nextTaskId++, "Review project proposal", "Check the new project proposal and provide feedback", 
                  TaskPriority::TASK_HIGH, TaskStatus::TODO, 0, time(nullptr), 0, "Work", false, 0};
    
    AppTask task2 = {m_nextTaskId++, "Grocery shopping", "Buy milk, bread, eggs, and fruits", 
                  TaskPriority::TASK_MEDIUM, TaskStatus::TODO, 0, time(nullptr), 0, "Personal", false, 0};
    
    AppTask task3 = {m_nextTaskId++, "Call dentist", "Schedule appointment for dental checkup", 
                  TaskPriority::TASK_LOW, TaskStatus::COMPLETED, 0, time(nullptr) - 86400, time(nullptr), "Health", false, 0};
    
    m_tasks.push_back(task1);
    m_tasks.push_back(task2);
    m_tasks.push_back(task3);
    
    return OS_OK;
}

os_error_t TaskManagementApp::saveTasks() {
    // In a real implementation, save to file system
    return OS_OK;
}

std::string TaskManagementApp::priorityToString(TaskPriority priority) {
    switch (priority) {
        case TaskPriority::TASK_LOW: return "Low";
        case TaskPriority::TASK_MEDIUM: return "Medium";
        case TaskPriority::TASK_HIGH: return "High";
        case TaskPriority::TASK_URGENT: return "Urgent";
        default: return "Unknown";
    }
}

std::string TaskManagementApp::statusToString(TaskStatus status) {
    switch (status) {
        case TaskStatus::TODO: return "To Do";
        case TaskStatus::IN_PROGRESS: return "In Progress";
        case TaskStatus::COMPLETED: return "Completed";
        default: return "Unknown";
    }
}

lv_color_t TaskManagementApp::getPriorityColor(TaskPriority priority) {
    switch (priority) {
        case TaskPriority::TASK_LOW: return lv_color_hex(0x95A5A6);
        case TaskPriority::TASK_MEDIUM: return lv_color_hex(0xF39C12);
        case TaskPriority::TASK_HIGH: return lv_color_hex(0xE67E22);
        case TaskPriority::TASK_URGENT: return lv_color_hex(0xE74C3C);
        default: return lv_color_hex(0x95A5A6);
    }
}

// Static callbacks
void TaskManagementApp::taskListCallback(lv_event_t* e) {
    TaskManagementApp* app = static_cast<TaskManagementApp*>(lv_event_get_user_data(e));
    lv_obj_t* btn = lv_event_get_target(e);
    uint32_t taskId = (uint32_t)(uintptr_t)lv_obj_get_user_data(btn);
    app->m_selectedTaskId = taskId;
    
    for (const auto& task : app->m_tasks) {
        if (task.id == taskId) {
            app->showTaskDetails(task);
            break;
        }
    }
}

void TaskManagementApp::addButtonCallback(lv_event_t* e) {
    TaskManagementApp* app = static_cast<TaskManagementApp*>(lv_event_get_user_data(e));
    app->showAddEditDialog();
}

void TaskManagementApp::editButtonCallback(lv_event_t* e) {
    TaskManagementApp* app = static_cast<TaskManagementApp*>(lv_event_get_user_data(e));
    if (app->m_selectedTaskId > 0) {
        for (auto& task : app->m_tasks) {
            if (task.id == app->m_selectedTaskId) {
                app->showAddEditDialog(&task);
                break;
            }
        }
    }
}

void TaskManagementApp::deleteButtonCallback(lv_event_t* e) {
    TaskManagementApp* app = static_cast<TaskManagementApp*>(lv_event_get_user_data(e));
    if (app->m_selectedTaskId > 0) {
        app->deleteTask(app->m_selectedTaskId);
        app->m_selectedTaskId = 0;
    }
}

void TaskManagementApp::completeButtonCallback(lv_event_t* e) {
    TaskManagementApp* app = static_cast<TaskManagementApp*>(lv_event_get_user_data(e));
    if (app->m_selectedTaskId > 0) {
        app->markTaskCompleted(app->m_selectedTaskId);
    }
}

void TaskManagementApp::saveButtonCallback(lv_event_t* e) {
    TaskManagementApp* app = static_cast<TaskManagementApp*>(lv_event_get_user_data(e));
    
    AppTask task;
    task.title = lv_textarea_get_text(app->m_titleInput);
    task.description = lv_textarea_get_text(app->m_descriptionInput);
    task.category = lv_textarea_get_text(app->m_categoryInput);
    task.priority = (TaskPriority)lv_dropdown_get_selected(app->m_priorityInput);
    task.dueDate = app->m_selectedDueDate;
    task.hasReminder = false;
    task.reminderTime = 0;
    
    if (app->m_editingTask) {
        app->editTask(app->m_editingTask->id, task);
    } else {
        app->addTask(task);
    }
    
    app->hideAddEditDialog();
}

void TaskManagementApp::cancelButtonCallback(lv_event_t* e) {
    TaskManagementApp* app = static_cast<TaskManagementApp*>(lv_event_get_user_data(e));
    app->hideAddEditDialog();
}

void TaskManagementApp::filterCallback(lv_event_t* e) {
    TaskManagementApp* app = static_cast<TaskManagementApp*>(lv_event_get_user_data(e));
    app->refreshTaskList();
}

void TaskManagementApp::sortCallback(lv_event_t* e) {
    TaskManagementApp* app = static_cast<TaskManagementApp*>(lv_event_get_user_data(e));
    app->refreshTaskList();
}

void TaskManagementApp::dueDateCallback(lv_event_t* e) {
    // In a real implementation, this would open a date picker
    TaskManagementApp* app = static_cast<TaskManagementApp*>(lv_event_get_user_data(e));
    app->m_selectedDueDate = time(nullptr) + 86400 * 7; // Default to 1 week from now
}

extern "C" std::unique_ptr<BaseApp> createTaskManagementApp() {
    return std::make_unique<TaskManagementApp>();
}