#include "power_manager.h"
#include "os_manager.h"
#include <esp_log.h>
#include <driver/gpio.h>
#include <esp_pm.h>
#include <esp_wifi.h>
// Conditional Bluetooth include
#ifdef CONFIG_BT_ENABLED
#include <esp_bt.h>
#endif

static const char* TAG = "PowerManager";

PowerManager::~PowerManager() {
    shutdown();
}

os_error_t PowerManager::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Power Manager");

    // Initialize GPIO for power management
    os_error_t result = initializeGPIO();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize GPIO");
        return result;
    }

    // Configure sleep wake-up sources
    result = configureSleepWakeup();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to configure sleep wake-up");
        return result;
    }

    // Initialize power management configuration
    esp_pm_config_t pm_config = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 80,
        .light_sleep_enable = true
    };
    
    esp_err_t ret = esp_pm_configure(&pm_config);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to configure power management: %s", esp_err_to_name(ret));
    }

    m_lastActivity = millis();
    m_initialized = true;

    ESP_LOGI(TAG, "Power Manager initialized successfully");
    return OS_OK;
}

os_error_t PowerManager::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Power Manager");

    // Disable all 5V outputs
    set5VOutput1(false);
    set5VOutput2(false);

    m_initialized = false;
    return OS_OK;
}

os_error_t PowerManager::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    // Process button events
    processButtonEvents();

    // Update battery level
    updateBatteryLevel();

    // Check if we should enter sleep mode
    if (shouldEnterSleep()) {
        ESP_LOGI(TAG, "Entering light sleep due to inactivity");
        enterSleep(PowerState::LIGHT_SLEEP);
    }

    return OS_OK;
}

os_error_t PowerManager::enterSleep(PowerState sleepType) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    ESP_LOGI(TAG, "Entering sleep mode: %d", (int)sleepType);

    switch (sleepType) {
        case PowerState::LIGHT_SLEEP:
            m_currentState = PowerState::LIGHT_SLEEP;
            
            // Configure sleep timer
            esp_sleep_enable_timer_wakeup(60000000); // 60 seconds
            
            // Enter light sleep
            esp_light_sleep_start();
            
            // We wake up here
            m_currentState = PowerState::ACTIVE;
            m_lastActivity = millis();
            m_wakeupCount++;
            
            ESP_LOGI(TAG, "Woke up from light sleep");
            break;

        case PowerState::DEEP_SLEEP:
            m_currentState = PowerState::DEEP_SLEEP;
            
            // Disable WiFi and Bluetooth before deep sleep
            setWiFiEnabled(false);
            setBluetoothEnabled(false);
            
            // Configure deep sleep timer
            esp_sleep_enable_timer_wakeup(5 * 60 * 1000000); // 5 minutes
            
            ESP_LOGI(TAG, "Entering deep sleep");
            esp_deep_sleep_start();
            // Device will restart after deep sleep
            break;

        case PowerState::SHUTDOWN:
            // Disable all peripherals and outputs
            set5VOutput1(false);
            set5VOutput2(false);
            setWiFiEnabled(false);
            setBluetoothEnabled(false);
            
            ESP_LOGI(TAG, "System shutdown requested");
            // Note: Actual shutdown depends on hardware implementation
            break;

        default:
            return OS_ERROR_INVALID_PARAM;
    }

    return OS_OK;
}

WakeupReason PowerManager::wakeUp() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI(TAG, "Wake up from timer");
            return WakeupReason::TIMER;
            
        case ESP_SLEEP_WAKEUP_EXT0:
        case ESP_SLEEP_WAKEUP_EXT1:
            ESP_LOGI(TAG, "Wake up from external interrupt");
            return WakeupReason::POWER_BUTTON;
            
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            ESP_LOGI(TAG, "Wake up from touch");
            return WakeupReason::TOUCH_INPUT;
            
        default:
            ESP_LOGI(TAG, "Wake up from unknown source");
            return WakeupReason::UNKNOWN;
    }
}

os_error_t PowerManager::setWiFiEnabled(bool enabled) {
    if (m_wifiEnabled == enabled) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "%s WiFi", enabled ? "Enabling" : "Disabling");
    
    esp_err_t ret;
    if (enabled) {
        ret = esp_wifi_start();
        gpio_set_level(WIFI_EN_PIN, 1);
    } else {
        ret = esp_wifi_stop();
        gpio_set_level(WIFI_EN_PIN, 0);
    }

    if (ret == ESP_OK) {
        m_wifiEnabled = enabled;
        return OS_OK;
    } else {
        ESP_LOGE(TAG, "Failed to %s WiFi: %s", enabled ? "enable" : "disable", esp_err_to_name(ret));
        return OS_ERROR_GENERIC;
    }
}

os_error_t PowerManager::setBluetoothEnabled(bool enabled) {
    if (m_bluetoothEnabled == enabled) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "%s Bluetooth", enabled ? "Enabling" : "Disabling");
    
    #ifdef CONFIG_BT_ENABLED
    esp_err_t ret;
    if (enabled) {
        ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
        gpio_set_level(BT_EN_PIN, 1);
    } else {
        ret = esp_bt_controller_disable();
        gpio_set_level(BT_EN_PIN, 0);
    }

    if (ret == ESP_OK) {
        m_bluetoothEnabled = enabled;
        return OS_OK;
    } else {
        ESP_LOGE(TAG, "Failed to %s Bluetooth: %s", enabled ? "enable" : "disable", esp_err_to_name(ret));
        return OS_ERROR_GENERIC;
    }
    #else
    ESP_LOGW(TAG, "Bluetooth not supported in this build");
    gpio_set_level(BT_EN_PIN, enabled ? 1 : 0);
    m_bluetoothEnabled = enabled;
    return OS_OK;
    #endif
}

os_error_t PowerManager::set5VOutput1(bool enabled) {
    ESP_LOGI(TAG, "%s 5V Output 1", enabled ? "Enabling" : "Disabling");
    
    gpio_set_level(LPW5209_EN1_PIN, enabled ? 1 : 0);
    m_5vOutput1Enabled = enabled;
    
    // Small delay to allow output to stabilize
    vTaskDelay(pdMS_TO_TICKS(10));
    
    return OS_OK;
}

os_error_t PowerManager::set5VOutput2(bool enabled) {
    ESP_LOGI(TAG, "%s 5V Output 2", enabled ? "Enabling" : "Disabling");
    
    gpio_set_level(LPW5209_EN2_PIN, enabled ? 1 : 0);
    m_5vOutput2Enabled = enabled;
    
    // Small delay to allow output to stabilize
    vTaskDelay(pdMS_TO_TICKS(10));
    
    return OS_OK;
}

bool PowerManager::has5VOutputFault() const {
    bool fault1 = gpio_get_level(LPW5209_FAULT1_PIN) == 0; // Active low
    bool fault2 = gpio_get_level(LPW5209_FAULT2_PIN) == 0; // Active low
    
    if (fault1 || fault2) {
        ESP_LOGW(TAG, "5V Output fault detected: Output1=%s, Output2=%s", 
                 fault1 ? "FAULT" : "OK", fault2 ? "FAULT" : "OK");
    }
    
    return fault1 || fault2;
}

ButtonEvent PowerManager::getLastButtonEvent() {
    ButtonEvent event = m_buttonEvent;
    m_buttonEvent = ButtonEvent::NONE;
    return event;
}

void PowerManager::printPowerStats() const {
    ESP_LOGI(TAG, "Power Statistics:");
    ESP_LOGI(TAG, "  Current State: %d", (int)m_currentState);
    ESP_LOGI(TAG, "  Battery Level: %d%%", m_batteryLevel);
    ESP_LOGI(TAG, "  WiFi: %s", m_wifiEnabled ? "ON" : "OFF");
    ESP_LOGI(TAG, "  Bluetooth: %s", m_bluetoothEnabled ? "ON" : "OFF");
    ESP_LOGI(TAG, "  5V Output 1: %s", m_5vOutput1Enabled ? "ON" : "OFF");
    ESP_LOGI(TAG, "  5V Output 2: %s", m_5vOutput2Enabled ? "ON" : "OFF");
    ESP_LOGI(TAG, "  Total Wakeups: %d", m_wakeupCount);
    ESP_LOGI(TAG, "  Last Activity: %d ms ago", millis() - m_lastActivity);
}

os_error_t PowerManager::initializeGPIO() {
    gpio_config_t io_conf = {};

    // Configure 5V output enable pins (outputs)
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << LPW5209_EN1_PIN) | (1ULL << LPW5209_EN2_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure 5V enable pins: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Configure fault detection pins (inputs)
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << LPW5209_FAULT1_PIN) | (1ULL << LPW5209_FAULT2_PIN);
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure fault pins: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Configure wireless enable pins (outputs)
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << WIFI_EN_PIN) | (1ULL << BT_EN_PIN);
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure wireless enable pins: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Configure power button interrupt pin
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = 1ULL << PMS150G_INT_PIN;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure power button pin: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Install GPIO ISR service
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to install GPIO ISR service: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Add ISR handler for power button
    ret = gpio_isr_handler_add(PMS150G_INT_PIN, powerButtonISR, this);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add power button ISR: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Initialize outputs to safe states
    gpio_set_level(LPW5209_EN1_PIN, 0);
    gpio_set_level(LPW5209_EN2_PIN, 0);
    gpio_set_level(WIFI_EN_PIN, 1);  // WiFi on by default
    gpio_set_level(BT_EN_PIN, 1);   // Bluetooth on by default

    return OS_OK;
}

os_error_t PowerManager::configureSleepWakeup() {
    // Configure EXT1 wakeup (power button) - ESP32-P4 doesn't support EXT0
    uint64_t ext1_mask = (1ULL << PMS150G_INT_PIN);
    esp_err_t ret = esp_sleep_enable_ext1_wakeup(ext1_mask, ESP_EXT1_WAKEUP_ANY_LOW);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure EXT1 wakeup: %s", esp_err_to_name(ret));
        return OS_ERROR_GENERIC;
    }

    // Configure touchpad wakeup if available
    #if defined(HW_HAS_TOUCH) && HW_HAS_TOUCH
    ret = esp_sleep_enable_touchpad_wakeup();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to configure touchpad wakeup: %s", esp_err_to_name(ret));
    }
    #endif

    return OS_OK;
}

void IRAM_ATTR PowerManager::powerButtonISR(void* arg) {
    PowerManager* pm = static_cast<PowerManager*>(arg);
    uint32_t now = millis();
    
    if (gpio_get_level(PMS150G_INT_PIN) == 0) {
        // Button pressed
        pm->m_buttonPressed = true;
        pm->m_buttonPressTime = now;
    } else {
        // Button released
        if (pm->m_buttonPressed) {
            uint32_t pressDuration = now - pm->m_buttonPressTime;
            
            if (pressDuration > LONG_PRESS_THRESHOLD) {
                pm->m_buttonEvent = ButtonEvent::LONG_PRESS;
            } else if (pressDuration > DEBOUNCE_TIME) {
                // Check for double press
                if (now - pm->m_lastButtonPress < DOUBLE_PRESS_WINDOW) {
                    pm->m_buttonEvent = ButtonEvent::DOUBLE_PRESS;
                } else {
                    pm->m_buttonEvent = ButtonEvent::SHORT_PRESS;
                }
                pm->m_lastButtonPress = now;
            }
            
            pm->m_buttonPressed = false;
        }
    }
}

void PowerManager::processButtonEvents() {
    // This is called from the main loop to handle button events
    // The actual detection happens in the ISR
}

void PowerManager::updateBatteryLevel() {
    // TODO: Implement actual battery level reading
    // This would typically involve reading from an ADC connected to a voltage divider
    // For now, we'll simulate a slowly decreasing battery
    static uint32_t lastBatteryUpdate = 0;
    uint32_t now = millis();
    
    if (now - lastBatteryUpdate > 60000) { // Update every minute
        if (m_batteryLevel > 0 && (now % 600000) == 0) { // Decrease every 10 minutes
            m_batteryLevel--;
        }
        lastBatteryUpdate = now;
    }
}

bool PowerManager::shouldEnterSleep() const {
    uint32_t timeSinceActivity = millis() - m_lastActivity;
    return (m_currentState == PowerState::ACTIVE) && 
           (timeSinceActivity > m_sleepTimeout);
}