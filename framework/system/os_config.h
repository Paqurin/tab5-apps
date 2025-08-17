#ifndef OS_CONFIG_H
#define OS_CONFIG_H

#include <Arduino.h>
#include "../hal/hardware_config.h"

/**
 * @file os_config.h
 * @brief Core operating system configuration for M5Stack Tab5
 * 
 * This file contains system-wide configuration constants and settings
 * for the M5Tab5 ESP32-P4 operating system.
 */

// Hardware Configuration (from hardware_config.h)
#define OS_SCREEN_WIDTH         DISPLAY_WIDTH
#define OS_SCREEN_HEIGHT        DISPLAY_HEIGHT
#define OS_SCREEN_BPP           DISPLAY_COLOR_DEPTH
#define OS_PSRAM_ENABLED        HW_HAS_PSRAM

// Memory Configuration - Optimized for 32MB PSRAM
#define OS_MAX_APPS             12
#define OS_APP_STACK_SIZE       16384
#define OS_SYSTEM_HEAP_SIZE     (2 * 1024 * 1024)   // 2MB for system
#define OS_APP_HEAP_SIZE        (4 * 1024 * 1024)   // 4MB for apps
#define OS_BUFFER_POOL_SIZE     (1 * 1024 * 1024)   // 1MB for buffers

// PSRAM Configuration
#define OS_PSRAM_HEAP_SIZE      (16 * 1024 * 1024)  // 16MB PSRAM heap
#define OS_DISPLAY_BUFFER_SIZE  (4 * 1024 * 1024)   // 4MB for display buffers
#define OS_GRAPHICS_CACHE_SIZE  (2 * 1024 * 1024)   // 2MB graphics cache

// Task Configuration - Increased for better multitasking
#define OS_MAX_TASKS            32
#define OS_TASK_PRIORITY_HIGH   3
#define OS_TASK_PRIORITY_NORMAL 2
#define OS_TASK_PRIORITY_LOW    1
#define OS_TASK_PRIORITY_IDLE   0

// Event System Configuration - Increased for HD display
#define OS_MAX_EVENT_LISTENERS  64
#define OS_EVENT_QUEUE_SIZE     128

// Touch Configuration
#define OS_TOUCH_THRESHOLD      10
#define OS_TOUCH_DEBOUNCE_MS    50
#define OS_GESTURE_TIMEOUT_MS   500

// File System Configuration - Enhanced for media files
#define OS_MAX_FILES_OPEN       16
#define OS_MAX_FILENAME_LEN     256
#define OS_STORAGE_MOUNT_POINT  "/storage"
#define OS_LARGE_FILE_BUFFER    (512 * 1024)  // 512KB for large file operations

// UI Configuration
#define OS_UI_REFRESH_RATE      30  // FPS
#define OS_UI_ANIMATION_TIME    200 // ms
#define OS_STATUS_BAR_HEIGHT    40
#define OS_DOCK_HEIGHT          60

// System Timing
#define OS_WATCHDOG_TIMEOUT_MS  30000
#define OS_IDLE_TIMEOUT_MS      300000  // 5 minutes
#define OS_SLEEP_CHECK_MS       1000

// Debug Configuration
#ifdef DEBUG
#define OS_DEBUG_ENABLED        1
#define OS_LOG_LEVEL            3  // 0=None, 1=Error, 2=Warn, 3=Info, 4=Debug
#else
#define OS_DEBUG_ENABLED        0
#define OS_LOG_LEVEL            1
#endif

// Version Information
#define OS_VERSION_MAJOR        1
#define OS_VERSION_MINOR        0
#define OS_VERSION_PATCH        0
#define OS_VERSION_STRING       "1.0.0"

// Error Codes
typedef enum {
    OS_OK = 0,
    OS_ERROR_GENERIC = -1,
    OS_ERROR_NO_MEMORY = -2,
    OS_ERROR_INVALID_PARAM = -3,
    OS_ERROR_NOT_FOUND = -4,
    OS_ERROR_TIMEOUT = -5,
    OS_ERROR_BUSY = -6,
    OS_ERROR_NOT_SUPPORTED = -7,
    OS_ERROR_HARDWARE = -8,
    OS_ERROR_FILESYSTEM = -9,
    OS_ERROR_PERMISSION = -10,
    OS_ERROR_NOT_AVAILABLE = -11,
    OS_ERROR_NOT_IMPLEMENTED = -12
} os_error_t;

// Forward declarations
class OSManager;
class AppManager;
class UIManager;
class TouchManager;
class EventSystem;

#endif // OS_CONFIG_H