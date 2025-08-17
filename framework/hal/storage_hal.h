#ifndef STORAGE_HAL_H
#define STORAGE_HAL_H

#include "../system/os_config.h"
#include <vector>
#include <string>

/**
 * @file storage_hal.h
 * @brief Storage Hardware Abstraction Layer for M5Stack Tab5
 * 
 * Manages persistent storage including internal flash, SD card,
 * and configuration storage with file system abstraction.
 */

enum class HALStorageType {
    INTERNAL_FLASH,
    SD_CARD,
    EXTERNAL_USB
};

struct HALStorageInfo {
    HALStorageType type;
    bool mounted;
    uint64_t totalSize;
    uint64_t freeSize;
    uint64_t usedSize;
    std::string mountPoint;
    std::string filesystem;
};

struct HALFileInfo {
    std::string name;
    std::string path;
    uint64_t size;
    uint32_t timestamp;
    bool isDirectory;
};

class StorageHAL {
public:
    StorageHAL() = default;
    ~StorageHAL();

    /**
     * @brief Initialize storage systems
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown storage systems
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update storage status
     * @param deltaTime Time elapsed since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Perform storage self-test
     * @return true if test passes, false otherwise
     */
    bool selfTest();

    /**
     * @brief Mount a storage device
     * @param type Storage type to mount
     * @param mountPoint Mount point path
     * @return OS_OK on success, error code on failure
     */
    os_error_t mount(HALStorageType type, const char* mountPoint = nullptr);

    /**
     * @brief Unmount a storage device
     * @param type Storage type to unmount
     * @return OS_OK on success, error code on failure
     */
    os_error_t unmount(HALStorageType type);

    /**
     * @brief Check if storage is mounted
     * @param type Storage type to check
     * @return true if mounted, false otherwise
     */
    bool isMounted(HALStorageType type) const;

    /**
     * @brief Get storage information
     * @param type Storage type
     * @return Storage information structure
     */
    HALStorageInfo getStorageInfo(HALStorageType type) const;

    /**
     * @brief Get available storage types
     * @return Vector of available storage types
     */
    std::vector<HALStorageType> getAvailableStorage() const;

    /**
     * @brief Create directory
     * @param path Directory path to create
     * @return OS_OK on success, error code on failure
     */
    os_error_t createDirectory(const char* path);

    /**
     * @brief Remove directory
     * @param path Directory path to remove
     * @param recursive True to remove recursively
     * @return OS_OK on success, error code on failure
     */
    os_error_t removeDirectory(const char* path, bool recursive = false);

    /**
     * @brief Check if path exists
     * @param path Path to check
     * @return true if exists, false otherwise
     */
    bool exists(const char* path) const;

    /**
     * @brief Get file/directory information
     * @param path Path to query
     * @return File information or empty if not found
     */
    HALFileInfo getFileInfo(const char* path) const;

    /**
     * @brief List directory contents
     * @param path Directory path to list
     * @return Vector of file information
     */
    std::vector<HALFileInfo> listDirectory(const char* path) const;

    /**
     * @brief Read file contents
     * @param path File path to read
     * @param buffer Buffer to store data
     * @param bufferSize Size of buffer
     * @return Number of bytes read or -1 on error
     */
    int readFile(const char* path, void* buffer, size_t bufferSize);

    /**
     * @brief Write file contents
     * @param path File path to write
     * @param data Data to write
     * @param dataSize Size of data
     * @param append True to append, false to overwrite
     * @return Number of bytes written or -1 on error
     */
    int writeFile(const char* path, const void* data, size_t dataSize, bool append = false);

    /**
     * @brief Delete file
     * @param path File path to delete
     * @return OS_OK on success, error code on failure
     */
    os_error_t deleteFile(const char* path);

    /**
     * @brief Copy file
     * @param sourcePath Source file path
     * @param destPath Destination file path
     * @return OS_OK on success, error code on failure
     */
    os_error_t copyFile(const char* sourcePath, const char* destPath);

    /**
     * @brief Get free space on storage
     * @param type Storage type
     * @return Free space in bytes
     */
    uint64_t getFreeSpace(HALStorageType type) const;

    /**
     * @brief Get total space on storage
     * @param type Storage type
     * @return Total space in bytes
     */
    uint64_t getTotalSpace(HALStorageType type) const;

    /**
     * @brief Format storage (WARNING: Destroys all data)
     * @param type Storage type to format
     * @return OS_OK on success, error code on failure
     */
    os_error_t format(HALStorageType type);

    /**
     * @brief Sync all pending writes to storage
     * @return OS_OK on success, error code on failure
     */
    os_error_t sync();

    /**
     * @brief Print storage statistics
     */
    void printStats() const;

private:
    /**
     * @brief Initialize internal flash storage
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeInternalFlash();

    /**
     * @brief Initialize SD card storage
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeSDCard();

    /**
     * @brief Get mount point for storage type
     * @param type Storage type
     * @return Mount point string
     */
    std::string getMountPoint(HALStorageType type) const;

    /**
     * @brief Update storage statistics
     */
    void updateStorageStats();

    // Storage status
    std::vector<HALStorageInfo> m_storageDevices;
    bool m_initialized = false;

    // Statistics
    uint32_t m_totalReads = 0;
    uint32_t m_totalWrites = 0;
    uint64_t m_bytesRead = 0;
    uint64_t m_bytesWritten = 0;
    uint32_t m_lastStatsUpdate = 0;
};

#endif // STORAGE_HAL_H