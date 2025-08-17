#include "task_scheduler.h"
#include <esp_log.h>
#include <algorithm>

static const char* TAG = "TaskScheduler";

TaskScheduler::~TaskScheduler() {
    shutdown();
}

os_error_t TaskScheduler::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Task Scheduler");
    
    m_tasks.reserve(OS_MAX_TASKS);
    m_lastUpdateTime = millis();
    m_initialized = true;

    ESP_LOGI(TAG, "Task Scheduler initialized");
    return OS_OK;
}

os_error_t TaskScheduler::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Task Scheduler");
    
    // Cancel all tasks
    m_tasks.clear();
    m_initialized = false;

    ESP_LOGI(TAG, "Task Scheduler shutdown complete");
    return OS_OK;
}

os_error_t TaskScheduler::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    uint32_t currentTime = millis();
    uint32_t frameStartTime = currentTime;

    // Sort tasks by priority (higher priority first)
    std::sort(m_tasks.begin(), m_tasks.end(), 
              [](const Task& a, const Task& b) {
                  if (a.priority != b.priority) {
                      return a.priority > b.priority;
                  }
                  return a.nextExecution < b.nextExecution;
              });

    // Execute ready tasks
    for (auto& task : m_tasks) {
        if (task.state == TaskState::READY && currentTime >= task.nextExecution) {
            executeTask(task);
            
            // Check if we've exceeded our frame budget
            uint32_t frameTime = millis() - frameStartTime;
            if (frameTime > 16) { // ~60fps budget
                break;
            }
        }
    }

    // Clean up completed tasks
    cleanupTasks();

    // Update statistics
    updateCPULoad();

    return OS_OK;
}

uint32_t TaskScheduler::scheduleOnce(TaskFunction function, uint8_t priority, 
                                    uint32_t delay, const char* name) {
    if (!m_initialized || !function || m_tasks.size() >= OS_MAX_TASKS) {
        return 0;
    }

    Task task;
    task.id = generateTaskId();
    task.function = function;
    task.priority = priority;
    task.state = TaskState::READY;
    task.nextExecution = millis() + delay;
    task.period = 0; // One-shot
    task.maxRunTime = m_defaultMaxRunTime;
    task.actualRunTime = 0;
    task.executionCount = 0;
    task.name = name;
    task.autoDelete = true;

    m_tasks.push_back(task);

    #if OS_DEBUG_ENABLED >= 2
    ESP_LOGD(TAG, "Scheduled one-shot task %d '%s' (priority %d, delay %d ms)", 
            task.id, name ? name : "unnamed", priority, delay);
    #endif

    return task.id;
}

uint32_t TaskScheduler::schedulePeriodic(TaskFunction function, uint32_t period, 
                                        uint8_t priority, uint32_t delay, const char* name) {
    if (!m_initialized || !function || period == 0 || m_tasks.size() >= OS_MAX_TASKS) {
        return 0;
    }

    Task task;
    task.id = generateTaskId();
    task.function = function;
    task.priority = priority;
    task.state = TaskState::READY;
    task.nextExecution = millis() + delay;
    task.period = period;
    task.maxRunTime = m_defaultMaxRunTime;
    task.actualRunTime = 0;
    task.executionCount = 0;
    task.name = name;
    task.autoDelete = false;

    m_tasks.push_back(task);

    #if OS_DEBUG_ENABLED >= 2
    ESP_LOGD(TAG, "Scheduled periodic task %d '%s' (priority %d, period %d ms, delay %d ms)", 
            task.id, name ? name : "unnamed", priority, period, delay);
    #endif

    return task.id;
}

bool TaskScheduler::cancelTask(uint32_t taskId) {
    auto it = findTask(taskId);
    if (it != m_tasks.end()) {
        #if OS_DEBUG_ENABLED >= 2
        ESP_LOGD(TAG, "Cancelled task %d '%s'", taskId, it->name ? it->name : "unnamed");
        #endif
        m_tasks.erase(it);
        return true;
    }
    return false;
}

bool TaskScheduler::suspendTask(uint32_t taskId) {
    auto it = findTask(taskId);
    if (it != m_tasks.end() && it->state != TaskState::SUSPENDED) {
        it->state = TaskState::SUSPENDED;
        #if OS_DEBUG_ENABLED >= 2
        ESP_LOGD(TAG, "Suspended task %d '%s'", taskId, it->name ? it->name : "unnamed");
        #endif
        return true;
    }
    return false;
}

bool TaskScheduler::resumeTask(uint32_t taskId) {
    auto it = findTask(taskId);
    if (it != m_tasks.end() && it->state == TaskState::SUSPENDED) {
        it->state = TaskState::READY;
        #if OS_DEBUG_ENABLED >= 2
        ESP_LOGD(TAG, "Resumed task %d '%s'", taskId, it->name ? it->name : "unnamed");
        #endif
        return true;
    }
    return false;
}

const Task* TaskScheduler::getTaskInfo(uint32_t taskId) const {
    auto it = std::find_if(m_tasks.begin(), m_tasks.end(),
                          [taskId](const Task& task) { return task.id == taskId; });
    return (it != m_tasks.end()) ? &(*it) : nullptr;
}

void TaskScheduler::printStats() const {
    ESP_LOGI(TAG, "=== Task Scheduler Statistics ===");
    ESP_LOGI(TAG, "Active tasks: %d/%d", m_tasks.size(), OS_MAX_TASKS);
    ESP_LOGI(TAG, "CPU load: %d%%", m_cpuLoad);
    ESP_LOGI(TAG, "Tasks executed: %d", m_tasksExecuted);
    ESP_LOGI(TAG, "Tasks with overrun: %d", m_tasksOverrun);

    ESP_LOGI(TAG, "=== Active Tasks ===");
    for (const auto& task : m_tasks) {
        const char* stateStr = "UNKNOWN";
        switch (task.state) {
            case TaskState::READY: stateStr = "READY"; break;
            case TaskState::RUNNING: stateStr = "RUNNING"; break;
            case TaskState::WAITING: stateStr = "WAITING"; break;
            case TaskState::SUSPENDED: stateStr = "SUSPENDED"; break;
            case TaskState::COMPLETED: stateStr = "COMPLETED"; break;
        }

        ESP_LOGI(TAG, "Task %d: '%s' [%s] P=%d Period=%d ms Exec=%d Avg=%d ms",
                task.id, task.name ? task.name : "unnamed", stateStr,
                task.priority, task.period, task.executionCount,
                task.executionCount > 0 ? task.actualRunTime / task.executionCount : 0);
    }
}

std::vector<Task>::iterator TaskScheduler::findTask(uint32_t taskId) {
    return std::find_if(m_tasks.begin(), m_tasks.end(),
                       [taskId](const Task& task) { return task.id == taskId; });
}

void TaskScheduler::executeTask(Task& task) {
    if (task.state != TaskState::READY) {
        return;
    }

    task.state = TaskState::RUNNING;
    uint32_t startTime = millis();

    try {
        task.function();
        m_tasksExecuted++;
    } catch (...) {
        ESP_LOGE(TAG, "Task %d '%s' threw an exception", 
                task.id, task.name ? task.name : "unnamed");
    }

    uint32_t endTime = millis();
    uint32_t executionTime = endTime - startTime;

    // Update task statistics
    task.actualRunTime += executionTime;
    task.executionCount++;

    // Check for overrun
    if (executionTime > task.maxRunTime) {
        m_tasksOverrun++;
        ESP_LOGW(TAG, "Task %d '%s' overran: %d ms > %d ms limit", 
                task.id, task.name ? task.name : "unnamed", 
                executionTime, task.maxRunTime);
    }

    // Schedule next execution for periodic tasks
    if (task.period > 0) {
        task.nextExecution = endTime + task.period;
        task.state = TaskState::READY;
    } else {
        // One-shot task completed
        task.state = TaskState::COMPLETED;
    }

    // Update total execution time for CPU load calculation
    m_totalExecutionTime += executionTime;

    #if OS_DEBUG_ENABLED >= 3
    ESP_LOGD(TAG, "Executed task %d '%s' in %d ms", 
            task.id, task.name ? task.name : "unnamed", executionTime);
    #endif
}

void TaskScheduler::updateCPULoad() {
    uint32_t currentTime = millis();
    uint32_t elapsed = currentTime - m_lastUpdateTime;
    
    if (elapsed >= 1000) { // Update every second
        m_cpuLoad = (m_totalExecutionTime * 100) / elapsed;
        if (m_cpuLoad > 100) m_cpuLoad = 100;
        
        m_totalExecutionTime = 0;
        m_lastUpdateTime = currentTime;
    }
}

void TaskScheduler::cleanupTasks() {
    m_tasks.erase(
        std::remove_if(m_tasks.begin(), m_tasks.end(),
                      [](const Task& task) {
                          return task.state == TaskState::COMPLETED && task.autoDelete;
                      }),
        m_tasks.end());
}