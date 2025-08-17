#include "display_hal.h"
#include "hardware_config.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_psram.h>

static const char* TAG = "DisplayHAL";

DisplayHAL::~DisplayHAL() {
    shutdown();
}

os_error_t DisplayHAL::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Display HAL (%dx%d)", OS_SCREEN_WIDTH, OS_SCREEN_HEIGHT);

    // Initialize hardware pins and peripherals
    os_error_t hwResult = initializeHardware();
    if (hwResult != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize display hardware");
        return hwResult;
    }

    // Initialize LVGL if not already done
    if (!lv_is_initialized()) {
        lv_init();
    }

    // Initialize LVGL display driver
    os_error_t result = initializeLVGL();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize LVGL display");
        return result;
    }

    // Set default brightness and enable display
    setBrightness(128);
    setEnabled(true);

    m_lastFPSUpdate = millis();
    m_initialized = true;

    ESP_LOGI(TAG, "Display HAL initialized successfully");
    return OS_OK;
}

os_error_t DisplayHAL::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Display HAL");

    // Disable display
    setEnabled(false);

    // Clean up LVGL resources
    if (m_buffer1) {
        free(m_buffer1);
        m_buffer1 = nullptr;
    }
    if (m_buffer2) {
        free(m_buffer2);
        m_buffer2 = nullptr;
    }

    m_lvglDisplay = nullptr;
    m_initialized = false;

    ESP_LOGI(TAG, "Display HAL shutdown complete");
    return OS_OK;
}

os_error_t DisplayHAL::update(uint32_t deltaTime) {
    if (!m_initialized || !m_enabled) {
        return OS_OK;
    }

    // Handle LVGL tasks
    lv_timer_handler();

    // Update FPS statistics
    updateFPS();

    return OS_OK;
}

bool DisplayHAL::selfTest() {
    if (!m_initialized) {
        return false;
    }

    ESP_LOGI(TAG, "Running display self-test");

    // Basic functionality test
    if (!m_lvglDisplay) {
        ESP_LOGE(TAG, "LVGL display not initialized");
        return false;
    }

    // Test draw buffer allocation
    if (!m_buffer1) {
        ESP_LOGE(TAG, "Draw buffer not allocated");
        return false;
    }

    // Test display dimensions
    if (getWidth() != OS_SCREEN_WIDTH || getHeight() != OS_SCREEN_HEIGHT) {
        ESP_LOGE(TAG, "Invalid display dimensions");
        return false;
    }

    ESP_LOGI(TAG, "Display self-test passed");
    return true;
}

os_error_t DisplayHAL::setBrightness(uint8_t brightness) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    m_brightness = brightness;

    // Set PWM duty cycle for backlight control
    uint32_t duty = (brightness * 255) / 255; // Convert to 8-bit PWM duty
    esp_err_t ret = ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set PWM duty: %s", esp_err_to_name(ret));
        return OS_ERROR_GENERIC;
    }

    ret = ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to update PWM duty: %s", esp_err_to_name(ret));
        return OS_ERROR_GENERIC;
    }
    
    ESP_LOGD(TAG, "Set brightness to %d (PWM duty: %d)", brightness, duty);
    return OS_OK;
}

os_error_t DisplayHAL::setEnabled(bool enabled) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    if (m_enabled == enabled) {
        return OS_OK;
    }

    m_enabled = enabled;

    if (enabled) {
        ESP_LOGI(TAG, "Display enabled");
        // TODO: Power on display
    } else {
        ESP_LOGI(TAG, "Display disabled");
        // TODO: Power off display
    }

    return OS_OK;
}

os_error_t DisplayHAL::setLowPowerMode(bool enabled) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    m_lowPowerMode = enabled;

    if (enabled) {
        // Reduce refresh rate and brightness for power saving
        setBrightness(m_brightness / 2);
        ESP_LOGI(TAG, "Display low power mode enabled");
    } else {
        // Restore normal operation
        setBrightness(m_brightness);
        ESP_LOGI(TAG, "Display low power mode disabled");
    }

    return OS_OK;
}

os_error_t DisplayHAL::forceRefresh() {
    if (!m_initialized || !m_enabled) {
        return OS_ERROR_GENERIC;
    }

    // Force LVGL to refresh the entire screen
    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(m_lvglDisplay);

    m_lastRefresh = millis();
    return OS_OK;
}

void DisplayHAL::printStats() const {
    ESP_LOGI(TAG, "=== Display HAL Statistics ===");
    ESP_LOGI(TAG, "Resolution: %dx%d", getWidth(), getHeight());
    ESP_LOGI(TAG, "Enabled: %s", m_enabled ? "yes" : "no");
    ESP_LOGI(TAG, "Brightness: %d/255", m_brightness);
    ESP_LOGI(TAG, "Low power mode: %s", m_lowPowerMode ? "yes" : "no");
    ESP_LOGI(TAG, "FPS: %.1f", m_fps);
    ESP_LOGI(TAG, "Total flushes: %d", m_totalFlushes);
    ESP_LOGI(TAG, "Last refresh: %d ms ago", millis() - m_lastRefresh);
}

void DisplayHAL::lvglFlushCallback(lv_disp_drv_t* disp_drv, 
                                  const lv_area_t* area, 
                                  lv_color_t* color_p) {
    DisplayHAL* self = static_cast<DisplayHAL*>(disp_drv->user_data);
    
    if (self) {
        self->m_totalFlushes++;
    }

    // TODO: Implement actual display flushing to hardware
    // For now, just mark as ready
    lv_disp_flush_ready(disp_drv);
}

os_error_t DisplayHAL::initializeLVGL() {
    // Calculate buffer size (10 lines worth of pixels)
    uint32_t bufferSize = OS_SCREEN_WIDTH * 10;
    
    // Allocate draw buffers
    m_buffer1 = static_cast<lv_color_t*>(malloc(bufferSize * sizeof(lv_color_t)));
    if (!m_buffer1) {
        ESP_LOGE(TAG, "Failed to allocate primary draw buffer");
        return OS_ERROR_NO_MEMORY;
    }

    // Optional second buffer for better performance
    m_buffer2 = static_cast<lv_color_t*>(malloc(bufferSize * sizeof(lv_color_t)));
    if (!m_buffer2) {
        ESP_LOGW(TAG, "Failed to allocate secondary draw buffer, using single buffer");
    }

    // Initialize draw buffer
    lv_disp_draw_buf_init(&m_drawBuffer, m_buffer1, m_buffer2, bufferSize);

    // Initialize display driver
    lv_disp_drv_init(&m_displayDriver);
    m_displayDriver.hor_res = OS_SCREEN_WIDTH;
    m_displayDriver.ver_res = OS_SCREEN_HEIGHT;
    m_displayDriver.flush_cb = lvglFlushCallback;
    m_displayDriver.draw_buf = &m_drawBuffer;
    m_displayDriver.user_data = this;

    // Register the driver
    m_lvglDisplay = lv_disp_drv_register(&m_displayDriver);
    if (!m_lvglDisplay) {
        ESP_LOGE(TAG, "Failed to register LVGL display driver");
        return OS_ERROR_GENERIC;
    }

    ESP_LOGI(TAG, "LVGL display driver initialized (%s buffer)",
             m_buffer2 ? "double" : "single");

    return OS_OK;
}

os_error_t DisplayHAL::initializeHardware() {
    ESP_LOGI(TAG, "Initializing ESP32-P4 display hardware");

    // Configure GPIO pins for display control
    gpio_config_t io_conf = {};
    
    // Display reset pin
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << DISPLAY_RST_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure display reset pin: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Power enable pin
    io_conf.pin_bit_mask = (1ULL << PWR_EN_PIN);
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure power enable pin: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Tearing effect pin (input)
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << DISPLAY_TE_PIN);
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure TE pin: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Initialize PWM for backlight control
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = PWM_BACKLIGHT_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ret = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure PWM timer: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    ledc_channel_config_t ledc_channel = {
        .gpio_num = DISPLAY_BL_PIN,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ret = ledc_channel_config(&ledc_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure PWM channel: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Hardware reset sequence
    gpio_set_level(PWR_EN_PIN, 1);    // Enable power
    vTaskDelay(pdMS_TO_TICKS(10));
    
    gpio_set_level(DISPLAY_RST_PIN, 0); // Assert reset
    vTaskDelay(pdMS_TO_TICKS(10));
    
    gpio_set_level(DISPLAY_RST_PIN, 1); // Release reset
    vTaskDelay(pdMS_TO_TICKS(DISPLAY_INIT_DELAY_MS));

    // TODO: Initialize MIPI-DSI peripheral
    // This requires ESP-IDF MIPI-DSI driver configuration
    // For now, we'll use a placeholder
    ESP_LOGW(TAG, "MIPI-DSI initialization not yet implemented");

    ESP_LOGI(TAG, "Display hardware initialized successfully");
    return OS_OK;
}

void DisplayHAL::updateFPS() {
    m_frameCount++;
    
    uint32_t now = millis();
    uint32_t elapsed = now - m_lastFPSUpdate;
    
    if (elapsed >= 1000) { // Update every second
        m_fps = (float)m_frameCount * 1000.0f / elapsed;
        m_frameCount = 0;
        m_lastFPSUpdate = now;
    }
}