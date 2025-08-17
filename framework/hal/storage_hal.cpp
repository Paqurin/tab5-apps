#include "storage_hal.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <esp_vfs.h>
#include <esp_spiffs.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <sys/stat.h>
#include <dirent.h>

static const char* TAG = "StorageHAL";

StorageHAL::~StorageHAL() {
    shutdown();
}

os_error_t StorageHAL::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Storage HAL");

    // Initialize internal flash storage (SPIFFS)
    os_error_t result = initializeInternalFlash();
    if (result != OS_OK) {
        ESP_LOGW(TAG, "Failed to initialize internal flash storage");
    }

    // Initialize SD card storage
    result = initializeSDCard();
    if (result != OS_OK) {
        ESP_LOGW(TAG, "Failed to initialize SD card storage");
    }

    // Create default directories
    createDirectory("/storage/apps");
    createDirectory("/storage/data");
    createDirectory("/storage/config");
    createDirectory("/storage/logs");

    m_lastStatsUpdate = millis();
    m_initialized = true;

    ESP_LOGI(TAG, "Storage HAL initialized with %d storage devices", m_storageDevices.size());
    return OS_OK;
}

os_error_t StorageHAL::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Storage HAL");

    // Sync all pending writes
    sync();

    // Unmount all storage devices
    for (const auto& storage : m_storageDevices) {
        if (storage.mounted) {
            if (storage.type == HALStorageType::INTERNAL_FLASH) {
                esp_vfs_spiffs_unregister(nullptr);
            } else if (storage.type == HALStorageType::SD_CARD) {
                esp_vfs_fat_sdcard_unmount(storage.mountPoint.c_str(), nullptr);
            }
        }
    }

    m_storageDevices.clear();
    m_initialized = false;

    ESP_LOGI(TAG, "Storage HAL shutdown complete");
    return OS_OK;
}

os_error_t StorageHAL::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    uint32_t currentTime = millis();
    
    // Update storage statistics every 30 seconds
    if (currentTime - m_lastStatsUpdate >= 30000) {
        updateStorageStats();
        m_lastStatsUpdate = currentTime;
    }

    return OS_OK;
}

bool StorageHAL::selfTest() {
    if (!m_initialized) {
        return false;
    }

    ESP_LOGI(TAG, "Running storage self-test");

    bool allTestsPassed = true;

    // Test each mounted storage device
    for (const auto& storage : m_storageDevices) {
        if (!storage.mounted) continue;

        ESP_LOGI(TAG, "Testing storage: %s", storage.mountPoint.c_str());

        // Test write/read/delete
        std::string testFile = storage.mountPoint + "/test_file.txt";
        const char* testData = "Storage self-test data";
        
        // Write test
        if (writeFile(testFile.c_str(), testData, strlen(testData)) < 0) {
            ESP_LOGE(TAG, "Write test failed for %s", storage.mountPoint.c_str());
            allTestsPassed = false;
            continue;
        }

        // Read test
        char readBuffer[64];
        int bytesRead = readFile(testFile.c_str(), readBuffer, sizeof(readBuffer));
        if (bytesRead < 0 || strcmp(readBuffer, testData) != 0) {
            ESP_LOGE(TAG, "Read test failed for %s", storage.mountPoint.c_str());
            allTestsPassed = false;
        }

        // Delete test
        if (deleteFile(testFile.c_str()) != OS_OK) {
            ESP_LOGE(TAG, "Delete test failed for %s", storage.mountPoint.c_str());
            allTestsPassed = false;
        }
    }

    if (allTestsPassed) {
        ESP_LOGI(TAG, "Storage self-test passed");
    } else {
        ESP_LOGE(TAG, "Storage self-test failed");
    }

    return allTestsPassed;
}

bool StorageHAL::isMounted(HALStorageType type) const {
    for (const auto& storage : m_storageDevices) {
        if (storage.type == type) {
            return storage.mounted;
        }
    }
    return false;
}

HALStorageInfo StorageHAL::getStorageInfo(HALStorageType type) const {
    for (const auto& storage : m_storageDevices) {
        if (storage.type == type) {
            return storage;
        }
    }
    
    // Return empty info if not found
    HALStorageInfo info;
    info.type = type;
    info.mounted = false;
    return info;
}

std::vector<HALStorageType> StorageHAL::getAvailableStorage() const {
    std::vector<HALStorageType> available;
    for (const auto& storage : m_storageDevices) {
        if (storage.mounted) {
            available.push_back(storage.type);
        }
    }
    return available;
}

os_error_t StorageHAL::createDirectory(const char* path) {
    if (!path) {
        return OS_ERROR_INVALID_PARAM;
    }

    if (mkdir(path, 0755) == 0 || errno == EEXIST) {
        return OS_OK;
    }

    ESP_LOGE(TAG, "Failed to create directory %s: %s", path, strerror(errno));
    return OS_ERROR_FILESYSTEM;
}

bool StorageHAL::exists(const char* path) const {
    if (!path) return false;
    
    struct stat st;
    return stat(path, &st) == 0;
}

HALFileInfo StorageHAL::getFileInfo(const char* path) const {
    HALFileInfo info;
    if (!path) return info;

    struct stat st;
    if (stat(path, &st) == 0) {
        info.name = strrchr(path, '/') ? strrchr(path, '/') + 1 : path;
        info.path = path;
        info.size = st.st_size;
        info.timestamp = st.st_mtime;
        info.isDirectory = S_ISDIR(st.st_mode);
    }

    return info;
}

std::vector<HALFileInfo> StorageHAL::listDirectory(const char* path) const {
    std::vector<HALFileInfo> files;
    if (!path) return files;

    DIR* dir = opendir(path);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory %s", path);
        return files;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        std::string fullPath = std::string(path) + "/" + entry->d_name;
        HALFileInfo info = getFileInfo(fullPath.c_str());
        if (!info.name.empty()) {
            files.push_back(info);
        }
    }

    closedir(dir);
    return files;
}

int StorageHAL::readFile(const char* path, void* buffer, size_t bufferSize) {
    if (!path || !buffer || bufferSize == 0) {
        return -1;
    }

    FILE* file = fopen(path, "rb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file for reading: %s", path);
        return -1;
    }

    size_t bytesRead = fread(buffer, 1, bufferSize, file);
    fclose(file);

    m_totalReads++;
    m_bytesRead += bytesRead;

    return (int)bytesRead;
}

int StorageHAL::writeFile(const char* path, const void* data, size_t dataSize, bool append) {
    if (!path || !data || dataSize == 0) {
        return -1;
    }

    FILE* file = fopen(path, append ? "ab" : "wb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", path);
        return -1;
    }

    size_t bytesWritten = fwrite(data, 1, dataSize, file);
    fclose(file);

    m_totalWrites++;
    m_bytesWritten += bytesWritten;

    return (int)bytesWritten;
}

os_error_t StorageHAL::deleteFile(const char* path) {
    if (!path) {
        return OS_ERROR_INVALID_PARAM;
    }

    if (unlink(path) == 0) {
        return OS_OK;
    }

    ESP_LOGE(TAG, "Failed to delete file %s: %s", path, strerror(errno));
    return OS_ERROR_FILESYSTEM;
}

os_error_t StorageHAL::copyFile(const char* sourcePath, const char* destPath) {
    if (!sourcePath || !destPath) {
        return OS_ERROR_INVALID_PARAM;
    }

    // Simple copy implementation
    char buffer[1024];
    int bytesRead = readFile(sourcePath, buffer, sizeof(buffer));
    
    if (bytesRead < 0) {
        return OS_ERROR_FILESYSTEM;
    }

    if (writeFile(destPath, buffer, bytesRead) < 0) {
        return OS_ERROR_FILESYSTEM;
    }

    return OS_OK;
}

os_error_t StorageHAL::sync() {
    // Force all cached writes to storage
    sync();
    return OS_OK;
}

void StorageHAL::printStats() const {
    ESP_LOGI(TAG, "=== Storage HAL Statistics ===");
    ESP_LOGI(TAG, "Total reads: %d", m_totalReads);
    ESP_LOGI(TAG, "Total writes: %d", m_totalWrites);
    ESP_LOGI(TAG, "Bytes read: %llu", m_bytesRead);
    ESP_LOGI(TAG, "Bytes written: %llu", m_bytesWritten);
    
    ESP_LOGI(TAG, "=== Storage Devices ===");
    for (const auto& storage : m_storageDevices) {
        ESP_LOGI(TAG, "Type: %d, Mounted: %s, Mount: %s", 
                (int)storage.type, storage.mounted ? "yes" : "no", 
                storage.mountPoint.c_str());
        if (storage.mounted) {
            ESP_LOGI(TAG, "  Total: %llu MB, Free: %llu MB, Used: %llu MB",
                    storage.totalSize / (1024*1024),
                    storage.freeSize / (1024*1024),
                    storage.usedSize / (1024*1024));
        }
    }
}

os_error_t StorageHAL::initializeInternalFlash() {
    ESP_LOGI(TAG, "Initializing internal flash storage (SPIFFS)");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/storage",
        .partition_label = nullptr,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPIFFS: %s", esp_err_to_name(ret));
        return OS_ERROR_FILESYSTEM;
    }

    // Get SPIFFS info
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(nullptr, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS info: %s", esp_err_to_name(ret));
    }

    // Add to storage devices list
    HALStorageInfo info;
    info.type = HALStorageType::INTERNAL_FLASH;
    info.mounted = true;
    info.totalSize = total;
    info.usedSize = used;
    info.freeSize = total - used;
    info.mountPoint = "/storage";
    info.filesystem = "SPIFFS";

    m_storageDevices.push_back(info);

    ESP_LOGI(TAG, "SPIFFS initialized: %d KB total, %d KB used", 
             total / 1024, used / 1024);

    return OS_OK;
}

os_error_t StorageHAL::initializeSDCard() {
    ESP_LOGI(TAG, "Initializing SD card storage");

    // TODO: Implement actual SD card initialization
    // This would typically involve:
    // - Configure SPI/SDMMC pins
    // - Initialize SD card driver
    // - Mount FAT filesystem

    ESP_LOGW(TAG, "SD card initialization not implemented yet");
    return OS_ERROR_NOT_SUPPORTED;
}

void StorageHAL::updateStorageStats() {
    for (auto& storage : m_storageDevices) {
        if (!storage.mounted) continue;

        if (storage.type == HALStorageType::INTERNAL_FLASH) {
            size_t total = 0, used = 0;
            if (esp_spiffs_info(nullptr, &total, &used) == ESP_OK) {
                storage.totalSize = total;
                storage.usedSize = used;
                storage.freeSize = total - used;
            }
        }
        // TODO: Update stats for other storage types
    }
}