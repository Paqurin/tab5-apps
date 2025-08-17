#include "file_manager_app.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <algorithm>
#include <ctime>

static const char* TAG = "FileManager";

FileManagerApp::FileManagerApp() 
    : BaseApp("file_manager", "File Manager", "1.0.0") {
    setDescription("File manager with SD card and USB storage support");
    setAuthor("M5Stack Tab5 OS");
    setPriority(AppPriority::APP_NORMAL);
}

FileManagerApp::~FileManagerApp() {
    shutdown();
}

os_error_t FileManagerApp::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Initializing File Manager Application");

    // Create or get storage service
    m_storageService = new StorageService();
    m_ownStorageService = true;
    
    os_error_t result = m_storageService->initialize();
    if (result != OS_OK) {
        log(ESP_LOG_ERROR, "Failed to initialize storage service");
        return result;
    }

    // Set initial path based on available storage
    if (m_storageService->isSDCardAvailable()) {
        m_currentStorage = StorageType::SD_CARD;
        m_currentPath = "/sdcard";
    } else if (m_storageService->isUSBStorageAvailable()) {
        m_currentStorage = StorageType::USB_STORAGE;
        m_currentPath = "/usb";
    } else {
        m_currentPath = "/";
    }

    setMemoryUsage(64 * 1024); // 64KB for file lists and UI
    m_initialized = true;

    log(ESP_LOG_INFO, "File Manager application initialized successfully");
    return OS_OK;
}

os_error_t FileManagerApp::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    // Update storage service
    if (m_storageService) {
        m_storageService->update(deltaTime);
    }

    // Update storage indicator if storage status changed
    updateStorageIndicator();

    return OS_OK;
}

os_error_t FileManagerApp::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Shutting down File Manager Application");

    // Cleanup storage service
    if (m_storageService && m_ownStorageService) {
        m_storageService->shutdown();
        delete m_storageService;
        m_storageService = nullptr;
    }

    m_initialized = false;
    return OS_OK;
}

os_error_t FileManagerApp::createUI(lv_obj_t* parent) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    log(ESP_LOG_INFO, "Creating File Manager UI");

    // Create main container
    m_uiContainer = lv_obj_create(parent);
    lv_obj_set_size(m_uiContainer, LV_HOR_RES, LV_VER_RES - OS_STATUS_BAR_HEIGHT - OS_DOCK_HEIGHT);
    lv_obj_align(m_uiContainer, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(m_uiContainer, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_opa(m_uiContainer, LV_OPA_TRANSP, 0);

    createToolbarUI();
    createFileBrowserUI();
    createStatusBarUI();

    // Load initial directory
    refreshDirectory();

    return OS_OK;
}

os_error_t FileManagerApp::destroyUI() {
    log(ESP_LOG_INFO, "Destroying File Manager UI");

    if (m_uiContainer) {
        lv_obj_del(m_uiContainer);
        m_uiContainer = nullptr;
        m_mainContainer = nullptr;
        m_toolbarContainer = nullptr;
        m_breadcrumbLabel = nullptr;
        m_fileListContainer = nullptr;
        m_fileList = nullptr;
        m_statusContainer = nullptr;
        m_storageLabel = nullptr;
        m_selectionLabel = nullptr;
        
        // Toolbar buttons
        m_upButton = nullptr;
        m_homeButton = nullptr;
        m_refreshButton = nullptr;
        m_copyButton = nullptr;
        m_moveButton = nullptr;
        m_deleteButton = nullptr;
        m_propertiesButton = nullptr;
        m_storageButton = nullptr;
        m_newFolderButton = nullptr;
        
        // Dialog elements
        m_dialogContainer = nullptr;
        m_confirmDialog = nullptr;
    }

    return OS_OK;
}

os_error_t FileManagerApp::handleEvent(uint32_t eventType, void* eventData, size_t dataSize) {
    // Handle file manager specific events
    return OS_OK;
}

os_error_t FileManagerApp::navigateToDirectory(const std::string& path) {
    if (!m_storageService) {
        return OS_ERROR_GENERIC;
    }

    log(ESP_LOG_INFO, "Navigating to directory: %s", path.c_str());

    // Check if path exists
    if (!m_storageService->fileExists(path)) {
        log(ESP_LOG_ERROR, "Directory does not exist: %s", path.c_str());
        return OS_ERROR_NOT_FOUND;
    }

    m_currentPath = path;
    return refreshDirectory();
}

os_error_t FileManagerApp::refreshDirectory() {
    if (!m_storageService) {
        return OS_ERROR_GENERIC;
    }

    log(ESP_LOG_DEBUG, "Refreshing directory: %s", m_currentPath.c_str());

    // List directory contents
    os_error_t result = m_storageService->listDirectory(m_currentPath, m_currentFiles);
    if (result != OS_OK) {
        log(ESP_LOG_ERROR, "Failed to list directory: %s", m_currentPath.c_str());
        return result;
    }

    // Update UI
    updateFileList();
    updateBreadcrumb();
    updateStorageInfo();

    m_selectedIndex = -1;
    m_filesAccessed++;

    return OS_OK;
}

void FileManagerApp::selectFile(size_t index) {
    if (index >= m_currentFiles.size()) {
        return;
    }

    m_selectedIndex = index;
    
    // Update selection label
    if (m_selectionLabel) {
        const FileInfo& file = m_currentFiles[index];
        char selectionText[128];
        if (file.isDirectory) {
            snprintf(selectionText, sizeof(selectionText), "Folder: %s", file.name.c_str());
        } else {
            char sizeText[32];
            formatFileSize(file.size, sizeText, sizeof(sizeText));
            snprintf(selectionText, sizeof(selectionText), "File: %s (%s)", file.name.c_str(), sizeText);
        }
        lv_label_set_text(m_selectionLabel, selectionText);
    }

    log(ESP_LOG_DEBUG, "Selected file: %s", m_currentFiles[index].name.c_str());
}

os_error_t FileManagerApp::executeFileOperation(FileOperation operation, const std::string& targetPath) {
    if (!m_storageService || m_selectedIndex < 0 || m_selectedIndex >= m_currentFiles.size()) {
        return OS_ERROR_INVALID_PARAM;
    }

    const FileInfo& selectedFile = m_currentFiles[m_selectedIndex];
    os_error_t result = OS_OK;

    switch (operation) {
        case FileOperation::COPY:
            if (!targetPath.empty()) {
                result = m_storageService->copyFile(selectedFile.path, targetPath);
                if (result == OS_OK) {
                    log(ESP_LOG_INFO, "File copied: %s -> %s", selectedFile.path.c_str(), targetPath.c_str());
                }
            }
            break;

        case FileOperation::MOVE:
            if (!targetPath.empty()) {
                result = m_storageService->moveFile(selectedFile.path, targetPath);
                if (result == OS_OK) {
                    log(ESP_LOG_INFO, "File moved: %s -> %s", selectedFile.path.c_str(), targetPath.c_str());
                    refreshDirectory(); // Refresh to update file list
                }
            }
            break;

        case FileOperation::DELETE:
            result = m_storageService->deleteFile(selectedFile.path);
            if (result == OS_OK) {
                log(ESP_LOG_INFO, "File deleted: %s", selectedFile.path.c_str());
                refreshDirectory(); // Refresh to update file list
            }
            break;

        default:
            result = OS_ERROR_INVALID_PARAM;
            break;
    }

    if (result == OS_OK) {
        m_operationsPerformed++;
    }

    return result;
}

os_error_t FileManagerApp::switchStorage(StorageType type) {
    switch (type) {
        case StorageType::SD_CARD:
            if (m_storageService && m_storageService->isSDCardAvailable()) {
                m_currentStorage = type;
                m_currentPath = "/sdcard";
                return navigateToDirectory(m_currentPath);
            }
            break;

        case StorageType::USB_STORAGE:
            if (m_storageService && m_storageService->isUSBStorageAvailable()) {
                m_currentStorage = type;
                m_currentPath = "/usb";
                return navigateToDirectory(m_currentPath);
            }
            break;

        default:
            return OS_ERROR_INVALID_PARAM;
    }

    return OS_ERROR_NOT_AVAILABLE;
}

void FileManagerApp::updateStorageInfo() {
    if (!m_storageService || !m_storageLabel) {
        return;
    }

    const StorageDevice* device = nullptr;
    if (m_currentStorage == StorageType::SD_CARD) {
        device = &m_storageService->getSDCardInfo();
    } else if (m_currentStorage == StorageType::USB_STORAGE) {
        device = &m_storageService->getUSBStorageInfo();
    }

    if (device && device->status == StorageStatus::MOUNTED) {
        char storageText[128];
        char totalText[32], freeText[32];
        formatFileSize(device->totalSize, totalText, sizeof(totalText));
        formatFileSize(device->freeSize, freeText, sizeof(freeText));
        
        snprintf(storageText, sizeof(storageText), "%s: %s free of %s", 
                device->label.c_str(), freeText, totalText);
        lv_label_set_text(m_storageLabel, storageText);
    } else {
        lv_label_set_text(m_storageLabel, "No storage available");
    }
}

void FileManagerApp::createFileBrowserUI() {
    // Create main file browser container
    m_mainContainer = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_mainContainer, LV_HOR_RES - 20, LV_VER_RES - 150);
    lv_obj_align(m_mainContainer, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(m_mainContainer, lv_color_black(), 0);
    lv_obj_set_style_border_color(m_mainContainer, lv_color_white(), 0);
    lv_obj_set_style_border_width(m_mainContainer, 1, 0);

    // Create breadcrumb
    m_breadcrumbLabel = lv_label_create(m_mainContainer);
    lv_obj_align(m_breadcrumbLabel, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_text_color(m_breadcrumbLabel, lv_color_hex(0x00AAFF), 0);
    lv_label_set_text(m_breadcrumbLabel, "/");

    // Create file list container
    m_fileListContainer = lv_obj_create(m_mainContainer);
    lv_obj_set_size(m_fileListContainer, LV_HOR_RES - 60, LV_VER_RES - 220);
    lv_obj_align(m_fileListContainer, LV_ALIGN_TOP_LEFT, 10, 40);
    lv_obj_set_style_bg_color(m_fileListContainer, lv_color_hex(0x2A2A2A), 0);

    // Create file list
    m_fileList = lv_list_create(m_fileListContainer);
    lv_obj_set_size(m_fileList, LV_HOR_RES - 80, LV_VER_RES - 240);
    lv_obj_center(m_fileList);
    lv_obj_set_style_bg_color(m_fileList, lv_color_hex(0x2A2A2A), 0);
    lv_obj_add_event_cb(m_fileList, fileListCallback, LV_EVENT_CLICKED, this);
}

void FileManagerApp::createToolbarUI() {
    // Create toolbar container
    m_toolbarContainer = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_toolbarContainer, LV_HOR_RES - 20, 50);
    lv_obj_align(m_toolbarContainer, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_bg_color(m_toolbarContainer, lv_color_hex(0x333333), 0);

    // Create toolbar buttons
    int buttonWidth = 80;
    int spacing = 10;
    int startX = spacing;

    // Up button
    m_upButton = lv_btn_create(m_toolbarContainer);
    lv_obj_set_size(m_upButton, buttonWidth, 35);
    lv_obj_align(m_upButton, LV_ALIGN_LEFT_MID, startX, 0);
    lv_obj_add_event_cb(m_upButton, upButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* upLabel = lv_label_create(m_upButton);
    lv_label_set_text(upLabel, "Up");
    lv_obj_center(upLabel);
    startX += buttonWidth + spacing;

    // Home button
    m_homeButton = lv_btn_create(m_toolbarContainer);
    lv_obj_set_size(m_homeButton, buttonWidth, 35);
    lv_obj_align(m_homeButton, LV_ALIGN_LEFT_MID, startX, 0);
    lv_obj_add_event_cb(m_homeButton, homeButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* homeLabel = lv_label_create(m_homeButton);
    lv_label_set_text(homeLabel, "Home");
    lv_obj_center(homeLabel);
    startX += buttonWidth + spacing;

    // Refresh button
    m_refreshButton = lv_btn_create(m_toolbarContainer);
    lv_obj_set_size(m_refreshButton, buttonWidth, 35);
    lv_obj_align(m_refreshButton, LV_ALIGN_LEFT_MID, startX, 0);
    lv_obj_add_event_cb(m_refreshButton, refreshButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* refreshLabel = lv_label_create(m_refreshButton);
    lv_label_set_text(refreshLabel, "Refresh");
    lv_obj_center(refreshLabel);
    startX += buttonWidth + spacing;

    // Copy button
    m_copyButton = lv_btn_create(m_toolbarContainer);
    lv_obj_set_size(m_copyButton, buttonWidth, 35);
    lv_obj_align(m_copyButton, LV_ALIGN_LEFT_MID, startX, 0);
    lv_obj_add_event_cb(m_copyButton, copyButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* copyLabel = lv_label_create(m_copyButton);
    lv_label_set_text(copyLabel, "Copy");
    lv_obj_center(copyLabel);
    startX += buttonWidth + spacing;

    // Delete button
    m_deleteButton = lv_btn_create(m_toolbarContainer);
    lv_obj_set_size(m_deleteButton, buttonWidth, 35);
    lv_obj_align(m_deleteButton, LV_ALIGN_LEFT_MID, startX, 0);
    lv_obj_add_event_cb(m_deleteButton, deleteButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* deleteLabel = lv_label_create(m_deleteButton);
    lv_label_set_text(deleteLabel, "Delete");
    lv_obj_center(deleteLabel);
    startX += buttonWidth + spacing;

    // Storage button
    m_storageButton = lv_btn_create(m_toolbarContainer);
    lv_obj_set_size(m_storageButton, buttonWidth, 35);
    lv_obj_align(m_storageButton, LV_ALIGN_RIGHT_MID, -spacing, 0);
    lv_obj_add_event_cb(m_storageButton, storageButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_t* storageLabel = lv_label_create(m_storageButton);
    lv_label_set_text(storageLabel, "Storage");
    lv_obj_center(storageLabel);
}

void FileManagerApp::createStatusBarUI() {
    // Create status container
    m_statusContainer = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_statusContainer, LV_HOR_RES - 20, 40);
    lv_obj_align(m_statusContainer, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_bg_color(m_statusContainer, lv_color_hex(0x333333), 0);

    // Storage label
    m_storageLabel = lv_label_create(m_statusContainer);
    lv_obj_align(m_storageLabel, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_text_color(m_storageLabel, lv_color_white(), 0);
    lv_label_set_text(m_storageLabel, "Storage: Not Available");

    // Selection label
    m_selectionLabel = lv_label_create(m_statusContainer);
    lv_obj_align(m_selectionLabel, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_text_color(m_selectionLabel, lv_color_hex(0x00AAFF), 0);
    lv_label_set_text(m_selectionLabel, "No selection");
}

void FileManagerApp::updateFileList() {
    if (!m_fileList) {
        return;
    }

    // Clear existing list
    lv_obj_clean(m_fileList);

    // Add files to list
    for (size_t i = 0; i < m_currentFiles.size(); i++) {
        const FileInfo& file = m_currentFiles[i];
        
        lv_obj_t* btn = lv_list_add_btn(m_fileList, getFileTypeIcon(file), file.name.c_str());
        lv_obj_set_height(btn, 30);
        
        // Set button color based on file type
        if (file.isDirectory) {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x4A4A4A), 0);
        } else {
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x3A3A3A), 0);
        }
        
        // Store index in user data
        lv_obj_set_user_data(btn, (void*)i);
    }
}

void FileManagerApp::updateBreadcrumb() {
    if (!m_breadcrumbLabel) {
        return;
    }

    // Simplify path display
    std::string displayPath = m_currentPath;
    if (displayPath.length() > 60) {
        displayPath = "..." + displayPath.substr(displayPath.length() - 57);
    }
    
    lv_label_set_text(m_breadcrumbLabel, displayPath.c_str());
}

void FileManagerApp::updateStorageIndicator() {
    updateStorageInfo();
}

void FileManagerApp::formatFileSize(uint64_t size, char* buffer, size_t bufferSize) {
    if (size < 1024) {
        snprintf(buffer, bufferSize, "%llu B", size);
    } else if (size < 1024 * 1024) {
        snprintf(buffer, bufferSize, "%.1f KB", size / 1024.0);
    } else if (size < 1024 * 1024 * 1024) {
        snprintf(buffer, bufferSize, "%.1f MB", size / (1024.0 * 1024.0));
    } else {
        snprintf(buffer, bufferSize, "%.1f GB", size / (1024.0 * 1024.0 * 1024.0));
    }
}

void FileManagerApp::formatTimestamp(time_t timestamp, char* buffer, size_t bufferSize) {
    struct tm* timeinfo = localtime(&timestamp);
    strftime(buffer, bufferSize, "%Y-%m-%d %H:%M", timeinfo);
}

const char* FileManagerApp::getFileTypeIcon(const FileInfo& fileInfo) {
    if (fileInfo.isDirectory) {
        return LV_SYMBOL_DIRECTORY;
    }
    
    // Simple file type detection based on extension
    std::string name = fileInfo.name;
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    
    if (name.find(".jpg") != std::string::npos || name.find(".png") != std::string::npos ||
        name.find(".bmp") != std::string::npos || name.find(".gif") != std::string::npos) {
        return LV_SYMBOL_IMAGE;
    } else if (name.find(".mp3") != std::string::npos || name.find(".wav") != std::string::npos ||
               name.find(".ogg") != std::string::npos) {
        return LV_SYMBOL_AUDIO;
    } else if (name.find(".mp4") != std::string::npos || name.find(".avi") != std::string::npos ||
               name.find(".mov") != std::string::npos) {
        return LV_SYMBOL_VIDEO;
    } else if (name.find(".txt") != std::string::npos || name.find(".log") != std::string::npos) {
        return LV_SYMBOL_FILE;
    } else {
        return LV_SYMBOL_FILE;
    }
}

// UI Event Callbacks
void FileManagerApp::fileListCallback(lv_event_t* e) {
    FileManagerApp* app = static_cast<FileManagerApp*>(lv_event_get_user_data(e));
    lv_obj_t* btn = lv_event_get_target(e);
    
    if (app && btn) {
        size_t index = (size_t)lv_obj_get_user_data(btn);
        if (index < app->m_currentFiles.size()) {
            const FileInfo& file = app->m_currentFiles[index];
            
            if (file.isDirectory) {
                // Navigate to directory
                app->navigateToDirectory(file.path);
            } else {
                // Select file
                app->selectFile(index);
            }
        }
    }
}

void FileManagerApp::upButtonCallback(lv_event_t* e) {
    FileManagerApp* app = static_cast<FileManagerApp*>(lv_event_get_user_data(e));
    if (app) {
        // Navigate to parent directory
        std::string parentPath = app->m_currentPath;
        size_t lastSlash = parentPath.find_last_of('/');
        if (lastSlash != std::string::npos && lastSlash > 0) {
            parentPath = parentPath.substr(0, lastSlash);
            app->navigateToDirectory(parentPath);
        }
    }
}

void FileManagerApp::homeButtonCallback(lv_event_t* e) {
    FileManagerApp* app = static_cast<FileManagerApp*>(lv_event_get_user_data(e));
    if (app) {
        // Navigate to storage root
        std::string homePath;
        if (app->m_currentStorage == StorageType::SD_CARD) {
            homePath = "/sdcard";
        } else if (app->m_currentStorage == StorageType::USB_STORAGE) {
            homePath = "/usb";
        } else {
            homePath = "/";
        }
        app->navigateToDirectory(homePath);
    }
}

void FileManagerApp::refreshButtonCallback(lv_event_t* e) {
    FileManagerApp* app = static_cast<FileManagerApp*>(lv_event_get_user_data(e));
    if (app) {
        app->refreshDirectory();
    }
}

void FileManagerApp::copyButtonCallback(lv_event_t* e) {
    FileManagerApp* app = static_cast<FileManagerApp*>(lv_event_get_user_data(e));
    if (app && app->m_selectedIndex >= 0) {
        // Set pending copy operation
        app->m_pendingOperation = FileOperation::COPY;
        app->m_operationSourcePath = app->m_currentFiles[app->m_selectedIndex].path;
        ESP_LOGI(TAG, "Copy operation prepared for: %s", app->m_operationSourcePath.c_str());
    }
}

void FileManagerApp::deleteButtonCallback(lv_event_t* e) {
    FileManagerApp* app = static_cast<FileManagerApp*>(lv_event_get_user_data(e));
    if (app && app->m_selectedIndex >= 0) {
        app->executeFileOperation(FileOperation::DELETE);
    }
}

void FileManagerApp::storageButtonCallback(lv_event_t* e) {
    FileManagerApp* app = static_cast<FileManagerApp*>(lv_event_get_user_data(e));
    if (app) {
        // Toggle between SD card and USB storage
        if (app->m_currentStorage == StorageType::SD_CARD) {
            app->switchStorage(StorageType::USB_STORAGE);
        } else {
            app->switchStorage(StorageType::SD_CARD);
        }
    }
}

void FileManagerApp::propertiesButtonCallback(lv_event_t* e) {
    // Not implemented in this basic version
}

void FileManagerApp::newFolderButtonCallback(lv_event_t* e) {
    // Not implemented in this basic version
}

void FileManagerApp::moveButtonCallback(lv_event_t* e) {
    // Not implemented in this basic version
}