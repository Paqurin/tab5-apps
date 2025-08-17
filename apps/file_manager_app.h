#ifndef FILE_MANAGER_APP_H
#define FILE_MANAGER_APP_H

#include "base_app.h"
#include "../services/storage_service.h"

/**
 * @file file_manager_app.h
 * @brief File Manager Application for M5Stack Tab5
 * 
 * Provides file management capabilities with support for
 * SD card and USB storage devices including file operations,
 * directory navigation, and storage device management.
 */

enum class FileManagerView {
    BROWSER,
    PROPERTIES,
    COPY_MOVE,
    STORAGE_INFO
};

enum class FileOperation {
    NONE,
    COPY,
    MOVE,
    DELETE
};

class FileManagerApp : public BaseApp {
public:
    FileManagerApp();
    ~FileManagerApp() override;

    // BaseApp interface
    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;
    os_error_t handleEvent(uint32_t eventType, void* eventData, size_t dataSize) override;

    /**
     * @brief Navigate to directory
     * @param path Directory path
     * @return OS_OK on success, error code on failure
     */
    os_error_t navigateToDirectory(const std::string& path);

    /**
     * @brief Refresh current directory
     * @return OS_OK on success, error code on failure
     */
    os_error_t refreshDirectory();

    /**
     * @brief Select file or directory
     * @param index File index in current list
     */
    void selectFile(size_t index);

    /**
     * @brief Execute file operation
     * @param operation Operation to perform
     * @param targetPath Target path for copy/move operations
     * @return OS_OK on success, error code on failure
     */
    os_error_t executeFileOperation(FileOperation operation, const std::string& targetPath = "");

    /**
     * @brief Switch storage device
     * @param type Storage type to switch to
     * @return OS_OK on success, error code on failure
     */
    os_error_t switchStorage(StorageType type);

    /**
     * @brief Get current storage statistics
     */
    void updateStorageInfo();

private:
    /**
     * @brief Create file browser UI
     */
    void createFileBrowserUI();

    /**
     * @brief Create toolbar UI
     */
    void createToolbarUI();

    /**
     * @brief Create status bar UI
     */
    void createStatusBarUI();

    /**
     * @brief Update file list display
     */
    void updateFileList();

    /**
     * @brief Update navigation breadcrumb
     */
    void updateBreadcrumb();

    /**
     * @brief Update storage indicator
     */
    void updateStorageIndicator();

    /**
     * @brief Show file properties dialog
     * @param fileInfo File information to display
     */
    void showFileProperties(const FileInfo& fileInfo);

    /**
     * @brief Show confirmation dialog
     * @param message Message to display
     * @param callback Callback function for confirmation
     */
    void showConfirmationDialog(const std::string& message, void(*callback)(bool, void*), void* userData);

    /**
     * @brief Format file size for display
     * @param size Size in bytes
     * @param buffer Output buffer
     * @param bufferSize Buffer size
     */
    void formatFileSize(uint64_t size, char* buffer, size_t bufferSize);

    /**
     * @brief Format timestamp for display
     * @param timestamp Unix timestamp
     * @param buffer Output buffer
     * @param bufferSize Buffer size
     */
    void formatTimestamp(time_t timestamp, char* buffer, size_t bufferSize);

    /**
     * @brief Get file type icon
     * @param fileInfo File information
     * @return Icon symbol
     */
    const char* getFileTypeIcon(const FileInfo& fileInfo);

    // UI event callbacks
    static void fileListCallback(lv_event_t* e);
    static void upButtonCallback(lv_event_t* e);
    static void homeButtonCallback(lv_event_t* e);
    static void refreshButtonCallback(lv_event_t* e);
    static void copyButtonCallback(lv_event_t* e);
    static void moveButtonCallback(lv_event_t* e);
    static void deleteButtonCallback(lv_event_t* e);
    static void propertiesButtonCallback(lv_event_t* e);
    static void storageButtonCallback(lv_event_t* e);
    static void newFolderButtonCallback(lv_event_t* e);

    // Storage service
    StorageService* m_storageService = nullptr;
    bool m_ownStorageService = false;

    // Current state
    FileManagerView m_currentView = FileManagerView::BROWSER;
    StorageType m_currentStorage = StorageType::SD_CARD;
    std::string m_currentPath;
    std::vector<FileInfo> m_currentFiles;
    int m_selectedIndex = -1;
    FileOperation m_pendingOperation = FileOperation::NONE;
    std::string m_operationSourcePath;

    // UI elements
    lv_obj_t* m_mainContainer = nullptr;
    lv_obj_t* m_toolbarContainer = nullptr;
    lv_obj_t* m_breadcrumbLabel = nullptr;
    lv_obj_t* m_fileListContainer = nullptr;
    lv_obj_t* m_fileList = nullptr;
    lv_obj_t* m_statusContainer = nullptr;
    lv_obj_t* m_storageLabel = nullptr;
    lv_obj_t* m_selectionLabel = nullptr;

    // Toolbar buttons
    lv_obj_t* m_upButton = nullptr;
    lv_obj_t* m_homeButton = nullptr;
    lv_obj_t* m_refreshButton = nullptr;
    lv_obj_t* m_copyButton = nullptr;
    lv_obj_t* m_moveButton = nullptr;
    lv_obj_t* m_deleteButton = nullptr;
    lv_obj_t* m_propertiesButton = nullptr;
    lv_obj_t* m_storageButton = nullptr;
    lv_obj_t* m_newFolderButton = nullptr;

    // Dialog elements
    lv_obj_t* m_dialogContainer = nullptr;
    lv_obj_t* m_confirmDialog = nullptr;

    // Statistics
    uint32_t m_filesAccessed = 0;
    uint32_t m_operationsPerformed = 0;

    // Configuration
    static constexpr size_t MAX_PATH_LENGTH = 256;
    static constexpr size_t MAX_FILENAME_LENGTH = 64;
    static constexpr uint32_t REFRESH_INTERVAL = 30000; // 30 seconds
};

#endif // FILE_MANAGER_APP_H