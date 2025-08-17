#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H

#include "os_config.h"
#include <functional>
#include <vector>
#include <queue>

/**
 * @file task_scheduler.h
 * @brief Task scheduling system for M5Stack Tab5
 * 
 * Provides cooperative multitasking with priority-based scheduling,
 * periodic tasks, and deferred execution capabilities.
 */

typedef std::function<void()> TaskFunction;

enum class TaskState {
    READY,
    RUNNING,
    WAITING,
    SUSPENDED,
    COMPLETED
};

struct Task {
    uint32_t id;
    TaskFunction function;
    uint8_t priority;
    TaskState state;
    uint32_t nextExecution;
    uint32_t period;  // 0 for one-shot tasks
    uint32_t maxRunTime;
    uint32_t actualRunTime;
    uint32_t executionCount;
    const char* name;
    bool autoDelete;
};

class TaskScheduler {
public:
    TaskScheduler() = default;
    ~TaskScheduler();

    /**
     * @brief Initialize the task scheduler
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown the task scheduler
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update and execute ready tasks
     * @param deltaTime Time elapsed since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Schedule a one-shot task
     * @param function Task function to execute
     * @param priority Task priority (0-3, higher = more important)
     * @param delay Delay before execution in milliseconds
     * @param name Optional task name for debugging
     * @return Task ID or 0 on failure
     */
    uint32_t scheduleOnce(TaskFunction function, uint8_t priority = OS_TASK_PRIORITY_NORMAL, 
                         uint32_t delay = 0, const char* name = nullptr);

    /**
     * @brief Schedule a periodic task
     * @param function Task function to execute
     * @param period Period between executions in milliseconds
     * @param priority Task priority (0-3, higher = more important)
     * @param delay Initial delay before first execution in milliseconds
     * @param name Optional task name for debugging
     * @return Task ID or 0 on failure
     */
    uint32_t schedulePeriodic(TaskFunction function, uint32_t period, 
                             uint8_t priority = OS_TASK_PRIORITY_NORMAL,
                             uint32_t delay = 0, const char* name = nullptr);

    /**
     * @brief Cancel a scheduled task
     * @param taskId Task ID to cancel
     * @return true if task was found and cancelled, false otherwise
     */
    bool cancelTask(uint32_t taskId);

    /**
     * @brief Suspend a task (can be resumed later)
     * @param taskId Task ID to suspend
     * @return true if task was found and suspended, false otherwise
     */
    bool suspendTask(uint32_t taskId);

    /**
     * @brief Resume a suspended task
     * @param taskId Task ID to resume
     * @return true if task was found and resumed, false otherwise
     */
    bool resumeTask(uint32_t taskId);

    /**
     * @brief Get task information
     * @param taskId Task ID to query
     * @return Pointer to task info or nullptr if not found
     */
    const Task* getTaskInfo(uint32_t taskId) const;

    /**
     * @brief Get number of active tasks
     * @return Number of active tasks
     */
    size_t getActiveTaskCount() const { return m_tasks.size(); }

    /**
     * @brief Get CPU load percentage
     * @return CPU load as percentage (0-100)
     */
    uint8_t getCPULoad() const { return m_cpuLoad; }

    /**
     * @brief Print task statistics
     */
    void printStats() const;

    /**
     * @brief Set maximum execution time for new tasks
     * @param maxTime Maximum execution time in milliseconds
     */
    void setDefaultMaxRunTime(uint32_t maxTime) { m_defaultMaxRunTime = maxTime; }

private:
    /**
     * @brief Find task by ID
     * @param taskId Task ID to find
     * @return Iterator to task or end() if not found
     */
    std::vector<Task>::iterator findTask(uint32_t taskId);

    /**
     * @brief Execute a single task
     * @param task Task to execute
     */
    void executeTask(Task& task);

    /**
     * @brief Update CPU load statistics
     */
    void updateCPULoad();

    /**
     * @brief Clean up completed tasks
     */
    void cleanupTasks();

    /**
     * @brief Generate unique task ID
     * @return Unique task ID
     */
    uint32_t generateTaskId() { return ++m_nextTaskId; }

    // Task management
    std::vector<Task> m_tasks;
    uint32_t m_nextTaskId = 0;
    uint32_t m_defaultMaxRunTime = 50; // 50ms default max run time

    // Statistics
    uint32_t m_totalExecutionTime = 0;
    uint32_t m_lastUpdateTime = 0;
    uint8_t m_cpuLoad = 0;
    uint32_t m_tasksExecuted = 0;
    uint32_t m_tasksOverrun = 0;

    bool m_initialized = false;
};

#endif // TASK_SCHEDULER_H