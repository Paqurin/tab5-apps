#include "hal_manager.h"
#include <esp_log.h>
#include <esp_system.h>

static const char* TAG = "HALManager";

HALManager::~HALManager() {
    shutdown();
}

os_error_t HALManager::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing HAL Manager");

    // Initialize hardware components
    os_error_t result = initializeComponents();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize hardware components: %d", result);
        return result;
    }

    // Build hardware info string
    snprintf(m_hardwareInfo, sizeof(m_hardwareInfo),
             "M5Stack Tab5 ESP32-P4 %dx%d Display GT911 Touch",
             OS_SCREEN_WIDTH, OS_SCREEN_HEIGHT);

    m_initialized = true;
    ESP_LOGI(TAG, "HAL Manager initialized: %s", m_hardwareInfo);

    return OS_OK;
}

os_error_t HALManager::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down HAL Manager");

    // Shutdown components in reverse order
    if (m_storageHAL) {
        m_storageHAL->shutdown();
        delete m_storageHAL;
        m_storageHAL = nullptr;
    }

    if (m_powerHAL) {
        m_powerHAL->shutdown();
        delete m_powerHAL;
        m_powerHAL = nullptr;
    }

    if (m_touchHAL) {
        m_touchHAL->shutdown();
        delete m_touchHAL;
        m_touchHAL = nullptr;
    }

    if (m_displayHAL) {
        m_displayHAL->shutdown();
        delete m_displayHAL;
        m_displayHAL = nullptr;
    }

    m_initialized = false;
    ESP_LOGI(TAG, "HAL Manager shutdown complete");

    return OS_OK;
}

os_error_t HALManager::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    // Update all hardware components
    if (m_displayHAL) {
        m_displayHAL->update(deltaTime);
    }

    if (m_touchHAL) {
        m_touchHAL->update(deltaTime);
    }

    if (m_powerHAL) {
        m_powerHAL->update(deltaTime);
    }

    if (m_storageHAL) {
        m_storageHAL->update(deltaTime);
    }

    return OS_OK;
}

const char* HALManager::getHardwareInfo() const {
    return m_hardwareInfo;
}

os_error_t HALManager::selfTest() {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    ESP_LOGI(TAG, "Running hardware self-test");

    // Test display
    if (m_displayHAL && !m_displayHAL->selfTest()) {
        ESP_LOGE(TAG, "Display self-test failed");
        return OS_ERROR_HARDWARE;
    }

    // Test touch
    if (m_touchHAL && !m_touchHAL->selfTest()) {
        ESP_LOGE(TAG, "Touch self-test failed");
        return OS_ERROR_HARDWARE;
    }

    // Test power management
    if (m_powerHAL && !m_powerHAL->selfTest()) {
        ESP_LOGE(TAG, "Power management self-test failed");
        return OS_ERROR_HARDWARE;
    }

    // Test storage
    if (m_storageHAL && !m_storageHAL->selfTest()) {
        ESP_LOGE(TAG, "Storage self-test failed");
        return OS_ERROR_HARDWARE;
    }

    ESP_LOGI(TAG, "Hardware self-test passed");
    return OS_OK;
}

float HALManager::getSystemTemperature() const {
    if (m_powerHAL) {
        return m_powerHAL->getTemperature();
    }
    return -999.0f; // Invalid temperature
}

os_error_t HALManager::setLowPowerMode(bool enabled) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    if (m_lowPowerMode == enabled) {
        return OS_OK; // Already in requested mode
    }

    ESP_LOGI(TAG, "%s low power mode", enabled ? "Enabling" : "Disabling");

    // Configure components for low power mode
    if (m_displayHAL) {
        m_displayHAL->setLowPowerMode(enabled);
    }

    if (m_touchHAL) {
        m_touchHAL->setLowPowerMode(enabled);
    }

    if (m_powerHAL) {
        m_powerHAL->setLowPowerMode(enabled);
    }

    m_lowPowerMode = enabled;
    return OS_OK;
}

os_error_t HALManager::initializeComponents() {
    // Initialize display HAL first (others may depend on it)
    m_displayHAL = new DisplayHAL();
    if (!m_displayHAL || m_displayHAL->initialize() != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize Display HAL");
        return OS_ERROR_HARDWARE;
    }

    // Initialize touch HAL
    m_touchHAL = new TouchHAL();
    if (!m_touchHAL || m_touchHAL->initialize() != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize Touch HAL");
        return OS_ERROR_HARDWARE;
    }

    // Initialize power management HAL
    m_powerHAL = new PowerHAL();
    if (!m_powerHAL || m_powerHAL->initialize() != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize Power HAL");
        return OS_ERROR_HARDWARE;
    }

    // Initialize storage HAL
    m_storageHAL = new StorageHAL();
    if (!m_storageHAL || m_storageHAL->initialize() != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize Storage HAL");
        return OS_ERROR_HARDWARE;
    }

    return OS_OK;
}