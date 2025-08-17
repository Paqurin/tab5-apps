#include "camera_app.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <esp_vfs_fat.h>

// Camera function stubs for when ESP camera library is not available
#ifndef ESP_CAMERA_SUPPORTED
int esp_camera_init(camera_config_t* config) { return -1; }
void esp_camera_deinit() {}
camera_fb_t* esp_camera_fb_get() { return nullptr; }
void esp_camera_fb_return(camera_fb_t* fb) {}
sensor_t* esp_camera_sensor_get() { return nullptr; }
#endif

static const char* TAG = "CameraApp";

CameraApp::CameraApp() 
    : BaseApp("camera", "Camera", "1.0.0") {
    setDescription("Camera application with SC2356 2MP sensor");
    setAuthor("M5Stack Tab5 OS");
    setPriority(AppPriority::APP_HIGH);
}

CameraApp::~CameraApp() {
    shutdown();
}

os_error_t CameraApp::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Initializing Camera Application");

    // Initialize camera hardware
    os_error_t result = initializeCamera();
    if (result != OS_OK) {
        log(ESP_LOG_ERROR, "Failed to initialize camera hardware");
        return result;
    }

    // Allocate preview buffer
    m_previewBufferSize = PREVIEW_WIDTH * PREVIEW_HEIGHT * 2; // RGB565
    m_previewBuffer = (uint8_t*)heap_caps_malloc(m_previewBufferSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!m_previewBuffer) {
        log(ESP_LOG_ERROR, "Failed to allocate preview buffer");
        return OS_ERROR_NO_MEMORY;
    }

    // Initialize preview image descriptor
    m_previewImageDesc.header.always_zero = 0;
    m_previewImageDesc.header.w = PREVIEW_WIDTH;
    m_previewImageDesc.header.h = PREVIEW_HEIGHT;
    m_previewImageDesc.data_size = m_previewBufferSize;
    m_previewImageDesc.header.cf = LV_IMG_CF_TRUE_COLOR;
    m_previewImageDesc.data = m_previewBuffer;

    setMemoryUsage(m_previewBufferSize + 4096); // Preview buffer + overhead
    m_initialized = true;

    log(ESP_LOG_INFO, "Camera application initialized successfully");
    return OS_OK;
}

os_error_t CameraApp::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    // Update preview if in preview mode
    if (m_currentMode == CameraMode::PREVIEW && m_cameraInitialized) {
        uint32_t now = millis();
        if (now - m_lastFrameTime >= PREVIEW_UPDATE_INTERVAL) {
            updatePreview();
            m_lastFrameTime = now;
        }
    }

    // Update recording timer if recording
    if (m_recording) {
        uint32_t recordingTime = (millis() - m_recordingStartTime) / 1000;
        if (m_statusLabel) {
            char timeStr[32];
            snprintf(timeStr, sizeof(timeStr), "REC %02d:%02d", recordingTime / 60, recordingTime % 60);
            lv_label_set_text(m_statusLabel, timeStr);
        }
    }

    return OS_OK;
}

os_error_t CameraApp::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Shutting down Camera Application");

    // Stop recording if active
    if (m_recording) {
        stopVideoRecording();
    }

    // Deinitialize camera
    if (m_cameraInitialized) {
        esp_camera_deinit();
        m_cameraInitialized = false;
    }

    // Free preview buffer
    if (m_previewBuffer) {
        heap_caps_free(m_previewBuffer);
        m_previewBuffer = nullptr;
    }

    m_initialized = false;
    return OS_OK;
}

os_error_t CameraApp::createUI(lv_obj_t* parent) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    log(ESP_LOG_INFO, "Creating Camera UI");

    // Create main container
    m_uiContainer = lv_obj_create(parent);
    lv_obj_set_size(m_uiContainer, LV_HOR_RES, LV_VER_RES - OS_STATUS_BAR_HEIGHT - OS_DOCK_HEIGHT);
    lv_obj_align(m_uiContainer, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(m_uiContainer, lv_color_black(), 0);
    lv_obj_set_style_border_opa(m_uiContainer, LV_OPA_TRANSP, 0);

    createPreviewUI();
    createCameraControls();

    return OS_OK;
}

os_error_t CameraApp::destroyUI() {
    log(ESP_LOG_INFO, "Destroying Camera UI");

    if (m_uiContainer) {
        lv_obj_del(m_uiContainer);
        m_uiContainer = nullptr;
        m_previewContainer = nullptr;
        m_previewImage = nullptr;
        m_controlPanel = nullptr;
        m_captureButton = nullptr;
        m_modeButton = nullptr;
        m_settingsButton = nullptr;
        m_statusLabel = nullptr;
        m_resolutionLabel = nullptr;
    }

    return OS_OK;
}

os_error_t CameraApp::handleEvent(uint32_t eventType, void* eventData, size_t dataSize) {
    // Handle camera-specific events
    return OS_OK;
}

os_error_t CameraApp::initializeCamera() {
    log(ESP_LOG_INFO, "Initializing SC2356 camera hardware");

    // Camera configuration for SC2356 via MIPI-CSI
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = CSI_D0_P_PIN;
    config.pin_d1 = CSI_D0_N_PIN;
    config.pin_d2 = CSI_D1_P_PIN;
    config.pin_d3 = CSI_D1_N_PIN;
    config.pin_d4 = -1; // Not used for MIPI-CSI
    config.pin_d5 = -1;
    config.pin_d6 = -1;
    config.pin_d7 = -1;
    config.pin_xclk = CSI_CLK_P_PIN;
    config.pin_pclk = CSI_CLK_N_PIN;
    config.pin_vsync = -1;
    config.pin_href = -1;
    config.pin_sccb_sda = GT911_SDA_PIN; // Reuse I2C pins
    config.pin_sccb_scl = GT911_SCL_PIN;
    config.pin_pwdn = CAMERA_PWR_PIN;
    config.pin_reset = CAMERA_RST_PIN;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_HD;
    config.jpeg_quality = 12;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

    // Initialize camera
    esp_err_t ret = esp_camera_init(&config);
    if (ret != ESP_OK) {
        log(ESP_LOG_ERROR, "Camera init failed: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Configure camera sensor settings
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        // SC2356 specific settings
        s->set_brightness(s, 0);     // -2 to 2
        s->set_contrast(s, 0);       // -2 to 2
        s->set_saturation(s, 0);     // -2 to 2
        s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect)
        s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
        s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
        s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled
        s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
        s->set_aec2(s, 0);           // 0 = disable , 1 = enable
        s->set_ae_level(s, 0);       // -2 to 2
        s->set_aec_value(s, 300);    // 0 to 1200
        s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
        s->set_agc_gain(s, 0);       // 0 to 30
        s->set_gainceiling(s, (gainceiling_t)0); // 0 to 6
        s->set_bpc(s, 0);            // 0 = disable , 1 = enable
        s->set_wpc(s, 1);            // 0 = disable , 1 = enable
        s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
        s->set_lenc(s, 1);           // 0 = disable , 1 = enable
        s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
        s->set_vflip(s, 0);          // 0 = disable , 1 = enable
    }

    m_cameraInitialized = true;
    log(ESP_LOG_INFO, "SC2356 camera initialized successfully");
    return OS_OK;
}

os_error_t CameraApp::capturePhoto() {
    if (!m_cameraInitialized) {
        return OS_ERROR_GENERIC;
    }

    log(ESP_LOG_INFO, "Capturing photo");

    // Capture frame
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        log(ESP_LOG_ERROR, "Camera capture failed");
        return OS_ERROR_GENERIC;
    }

    // Save image
    os_error_t result = saveImage(fb);
    
    // Return frame buffer
    esp_camera_fb_return(fb);

    if (result == OS_OK) {
        m_photosCaptured++;
        log(ESP_LOG_INFO, "Photo captured successfully (#%d)", m_photosCaptured);
    }

    return result;
}

os_error_t CameraApp::startVideoRecording() {
    if (!m_cameraInitialized || m_recording) {
        return OS_ERROR_GENERIC;
    }

    log(ESP_LOG_INFO, "Starting video recording");
    
    m_recording = true;
    m_recordingStartTime = millis();
    
    // TODO: Implement actual video recording
    // This would involve setting up video encoder and file writing
    
    return OS_OK;
}

os_error_t CameraApp::stopVideoRecording() {
    if (!m_recording) {
        return OS_ERROR_GENERIC;
    }

    log(ESP_LOG_INFO, "Stopping video recording");
    
    m_recording = false;
    m_videosRecorded++;
    
    // TODO: Finalize video file
    
    return OS_OK;
}

os_error_t CameraApp::setResolution(CaptureResolution resolution) {
    if (!m_cameraInitialized) {
        return OS_ERROR_GENERIC;
    }

    framesize_t framesize;
    switch (resolution) {
        case CaptureResolution::QVGA_320x240:
            framesize = FRAMESIZE_QVGA;
            break;
        case CaptureResolution::VGA_640x480:
            framesize = FRAMESIZE_VGA;
            break;
        case CaptureResolution::HD_1280x720:
            framesize = FRAMESIZE_HD;
            break;
        case CaptureResolution::FULL_1600x1200:
            framesize = FRAMESIZE_UXGA;
            break;
        default:
            return OS_ERROR_INVALID_PARAM;
    }

    sensor_t* s = esp_camera_sensor_get();
    if (s && s->set_framesize(s, framesize) == 0) {
        m_resolution = resolution;
        log(ESP_LOG_INFO, "Camera resolution changed to %d", (int)resolution);
        return OS_OK;
    }

    return OS_ERROR_GENERIC;
}

os_error_t CameraApp::toggleCamera() {
    if (m_cameraInitialized) {
        esp_camera_deinit();
        m_cameraInitialized = false;
        log(ESP_LOG_INFO, "Camera disabled");
    } else {
        return initializeCamera();
    }
    return OS_OK;
}

void CameraApp::printCameraStats() const {
    log(ESP_LOG_INFO, "Camera Statistics:");
    log(ESP_LOG_INFO, "  Photos captured: %d", m_photosCaptured);
    log(ESP_LOG_INFO, "  Videos recorded: %d", m_videosRecorded);
    log(ESP_LOG_INFO, "  Total frames: %d", m_totalFrames);
    log(ESP_LOG_INFO, "  Frame rate: %.1f FPS", m_frameRate);
    log(ESP_LOG_INFO, "  Current mode: %d", (int)m_currentMode);
    log(ESP_LOG_INFO, "  Resolution: %d", (int)m_resolution);
    log(ESP_LOG_INFO, "  Recording: %s", m_recording ? "YES" : "NO");
}

void CameraApp::createPreviewUI() {
    // Create preview container
    m_previewContainer = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_previewContainer, LV_HOR_RES - 200, LV_VER_RES - 200);
    lv_obj_align(m_previewContainer, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(m_previewContainer, lv_color_black(), 0);
    lv_obj_set_style_border_color(m_previewContainer, lv_color_white(), 0);
    lv_obj_set_style_border_width(m_previewContainer, 2, 0);

    // Create preview image
    m_previewImage = lv_img_create(m_previewContainer);
    lv_obj_center(m_previewImage);

    // Status label
    m_statusLabel = lv_label_create(m_uiContainer);
    lv_label_set_text(m_statusLabel, "READY");
    lv_obj_set_style_text_color(m_statusLabel, lv_color_white(), 0);
    lv_obj_align(m_statusLabel, LV_ALIGN_TOP_RIGHT, -10, 10);

    // Resolution label
    m_resolutionLabel = lv_label_create(m_uiContainer);
    lv_label_set_text(m_resolutionLabel, "HD 1280x720");
    lv_obj_set_style_text_color(m_resolutionLabel, lv_color_white(), 0);
    lv_obj_align(m_resolutionLabel, LV_ALIGN_TOP_RIGHT, -10, 40);
}

void CameraApp::createCameraControls() {
    // Create control panel
    m_controlPanel = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_controlPanel, 180, LV_VER_RES - 100);
    lv_obj_align(m_controlPanel, LV_ALIGN_TOP_RIGHT, -10, 80);
    lv_obj_set_style_bg_color(m_controlPanel, lv_color_hex(0x333333), 0);

    // Capture button
    m_captureButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_captureButton, 160, 50);
    lv_obj_align(m_captureButton, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_add_event_cb(m_captureButton, captureButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* captureLabel = lv_label_create(m_captureButton);
    lv_label_set_text(captureLabel, "CAPTURE");
    lv_obj_center(captureLabel);

    // Mode button
    m_modeButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_modeButton, 160, 40);
    lv_obj_align(m_modeButton, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_add_event_cb(m_modeButton, modeButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* modeLabel = lv_label_create(m_modeButton);
    lv_label_set_text(modeLabel, "PREVIEW");
    lv_obj_center(modeLabel);

    // Settings button
    m_settingsButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_settingsButton, 160, 40);
    lv_obj_align(m_settingsButton, LV_ALIGN_TOP_MID, 0, 120);
    lv_obj_add_event_cb(m_settingsButton, settingsButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* settingsLabel = lv_label_create(m_settingsButton);
    lv_label_set_text(settingsLabel, "SETTINGS");
    lv_obj_center(settingsLabel);
}

void CameraApp::updatePreview() {
    if (!m_cameraInitialized || !m_previewImage) {
        return;
    }

    // Capture frame for preview
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb) {
        // TODO: Convert JPEG to RGB565 for LVGL display
        // This is a simplified implementation
        // Real implementation would decode JPEG and scale to preview size
        
        lv_img_set_src(m_previewImage, &m_previewImageDesc);
        
        m_totalFrames++;
        
        // Calculate frame rate
        uint32_t now = millis();
        static uint32_t lastFpsUpdate = 0;
        static uint32_t frameCount = 0;
        frameCount++;
        
        if (now - lastFpsUpdate >= 1000) {
            m_frameRate = frameCount * 1000.0f / (now - lastFpsUpdate);
            frameCount = 0;
            lastFpsUpdate = now;
        }
        
        esp_camera_fb_return(fb);
    }
}

os_error_t CameraApp::saveImage(camera_fb_t* fb) {
    if (!fb) {
        return OS_ERROR_INVALID_PARAM;
    }

    // Generate filename
    char filename[MAX_FILENAME_LENGTH];
    snprintf(filename, sizeof(filename), "/sdcard/IMG_%05d.jpg", m_photosCaptured + 1);

    // Save to SD card
    FILE* file = fopen(filename, "wb");
    if (!file) {
        log(ESP_LOG_ERROR, "Failed to open file for writing: %s", filename);
        return OS_ERROR_FILESYSTEM;
    }

    size_t written = fwrite(fb->buf, 1, fb->len, file);
    fclose(file);

    if (written != fb->len) {
        log(ESP_LOG_ERROR, "Failed to write complete image");
        return OS_ERROR_FILESYSTEM;
    }

    log(ESP_LOG_INFO, "Image saved: %s (%d bytes)", filename, fb->len);
    return OS_OK;
}

void CameraApp::captureButtonCallback(lv_event_t* e) {
    CameraApp* app = static_cast<CameraApp*>(lv_event_get_user_data(e));
    if (app) {
        if (app->m_currentMode == CameraMode::VIDEO) {
            if (app->m_recording) {
                app->stopVideoRecording();
            } else {
                app->startVideoRecording();
            }
        } else {
            app->capturePhoto();
        }
    }
}

void CameraApp::modeButtonCallback(lv_event_t* e) {
    CameraApp* app = static_cast<CameraApp*>(lv_event_get_user_data(e));
    if (app && app->m_modeButton) {
        // Cycle through modes
        int mode = (int)app->m_currentMode;
        mode = (mode + 1) % 3; // Preview, Photo, Video
        app->m_currentMode = (CameraMode)mode;
        
        lv_obj_t* label = lv_obj_get_child(app->m_modeButton, 0);
        if (label) {
            const char* modeNames[] = {"PREVIEW", "PHOTO", "VIDEO"};
            lv_label_set_text(label, modeNames[mode]);
        }
    }
}

void CameraApp::settingsButtonCallback(lv_event_t* e) {
    CameraApp* app = static_cast<CameraApp*>(lv_event_get_user_data(e));
    if (app) {
        // Cycle through resolutions
        int res = (int)app->m_resolution;
        res = (res + 1) % 4;
        app->setResolution((CaptureResolution)res);
        
        if (app->m_resolutionLabel) {
            const char* resNames[] = {"QVGA 320x240", "VGA 640x480", "HD 1280x720", "FULL 1600x1200"};
            lv_label_set_text(app->m_resolutionLabel, resNames[res]);
        }
    }
}