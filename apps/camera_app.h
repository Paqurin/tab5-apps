#ifndef CAMERA_APP_H
#define CAMERA_APP_H

#include "base_app.h"
#include "../hal/hardware_config.h"

// Camera support is conditional based on availability
#ifdef ESP_CAMERA_SUPPORTED
#include <esp_camera.h>
#else
// Stub definitions for camera types
typedef struct {
    uint8_t* buf;
    size_t len;
} camera_fb_t;

#define CAMERA_GRAB_WHEN_EMPTY 0

typedef enum {
    FRAMESIZE_QVGA,
    FRAMESIZE_VGA,
    FRAMESIZE_HD,
    FRAMESIZE_UXGA
} framesize_t;

typedef enum {
    GAINCEILING_2X = 0,
    GAINCEILING_4X,
    GAINCEILING_8X,
    GAINCEILING_16X,
    GAINCEILING_32X,
    GAINCEILING_64X,
    GAINCEILING_128X
} gainceiling_t;

typedef enum {
    PIXFORMAT_JPEG
} pixformat_t;

typedef struct {
    int (*set_framesize)(void* sensor, framesize_t framesize);
    int (*set_brightness)(void* sensor, int level);
    int (*set_contrast)(void* sensor, int level);
    int (*set_saturation)(void* sensor, int level);
    int (*set_special_effect)(void* sensor, int effect);
    int (*set_whitebal)(void* sensor, int enable);
    int (*set_awb_gain)(void* sensor, int enable);
    int (*set_wb_mode)(void* sensor, int mode);
    int (*set_exposure_ctrl)(void* sensor, int enable);
    int (*set_aec2)(void* sensor, int enable);
    int (*set_ae_level)(void* sensor, int level);
    int (*set_aec_value)(void* sensor, int value);
    int (*set_gain_ctrl)(void* sensor, int enable);
    int (*set_agc_gain)(void* sensor, int gain);
    int (*set_gainceiling)(void* sensor, int gainceiling);
    int (*set_bpc)(void* sensor, int enable);
    int (*set_wpc)(void* sensor, int enable);
    int (*set_raw_gma)(void* sensor, int enable);
    int (*set_lenc)(void* sensor, int enable);
    int (*set_hmirror)(void* sensor, int enable);
    int (*set_vflip)(void* sensor, int enable);
} sensor_t;

typedef struct {
    int ledc_channel;
    int ledc_timer;
    int pin_d0, pin_d1, pin_d2, pin_d3, pin_d4, pin_d5, pin_d6, pin_d7;
    int pin_xclk, pin_pclk, pin_vsync, pin_href;
    int pin_sccb_sda, pin_sccb_scl;
    int pin_pwdn, pin_reset;
    int xclk_freq_hz;
    pixformat_t pixel_format;
    framesize_t frame_size;
    int jpeg_quality;
    int fb_count;
    int grab_mode;
} camera_config_t;
#endif

/**
 * @file camera_app.h
 * @brief Camera Application for M5Stack Tab5
 * 
 * Provides camera functionality using the SC2356 2MP sensor
 * via MIPI-CSI interface with live preview, photo capture,
 * and video recording capabilities.
 */

enum class CameraMode {
    PREVIEW,
    PHOTO,
    VIDEO,
    SETTINGS
};

enum class CaptureResolution {
    QVGA_320x240,
    VGA_640x480,
    HD_1280x720,
    FULL_1600x1200
};

class CameraApp : public BaseApp {
public:
    CameraApp();
    ~CameraApp() override;

    // BaseApp interface
    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;
    os_error_t handleEvent(uint32_t eventType, void* eventData, size_t dataSize) override;

    /**
     * @brief Initialize camera hardware
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeCamera();

    /**
     * @brief Capture a photo
     * @return OS_OK on success, error code on failure
     */
    os_error_t capturePhoto();

    /**
     * @brief Start video recording
     * @return OS_OK on success, error code on failure
     */
    os_error_t startVideoRecording();

    /**
     * @brief Stop video recording
     * @return OS_OK on success, error code on failure
     */
    os_error_t stopVideoRecording();

    /**
     * @brief Set camera resolution
     * @param resolution Desired resolution
     * @return OS_OK on success, error code on failure
     */
    os_error_t setResolution(CaptureResolution resolution);

    /**
     * @brief Set camera mode
     * @param mode Camera mode to set
     */
    void setCameraMode(CameraMode mode) { m_currentMode = mode; }

    /**
     * @brief Get current camera mode
     * @return Current camera mode
     */
    CameraMode getCameraMode() const { return m_currentMode; }

    /**
     * @brief Toggle camera on/off
     * @return OS_OK on success, error code on failure
     */
    os_error_t toggleCamera();

    /**
     * @brief Get camera statistics
     */
    void printCameraStats() const;

private:
    /**
     * @brief Create preview UI
     */
    void createPreviewUI();

    /**
     * @brief Create camera controls
     */
    void createCameraControls();

    /**
     * @brief Update camera preview
     */
    void updatePreview();

    /**
     * @brief Save captured image
     * @param fb Camera frame buffer
     * @return OS_OK on success, error code on failure
     */
    os_error_t saveImage(camera_fb_t* fb);

    /**
     * @brief Handle button events
     * @param button Which button was pressed
     */
    void handleButtonPress(int button);

    // UI event callbacks
    static void captureButtonCallback(lv_event_t* e);
    static void modeButtonCallback(lv_event_t* e);
    static void settingsButtonCallback(lv_event_t* e);

    // Camera state
    CameraMode m_currentMode = CameraMode::PREVIEW;
    CaptureResolution m_resolution = CaptureResolution::HD_1280x720;
    bool m_cameraInitialized = false;
    bool m_recording = false;
    uint32_t m_recordingStartTime = 0;

    // UI elements
    lv_obj_t* m_previewContainer = nullptr;
    lv_obj_t* m_previewImage = nullptr;
    lv_obj_t* m_controlPanel = nullptr;
    lv_obj_t* m_captureButton = nullptr;
    lv_obj_t* m_modeButton = nullptr;
    lv_obj_t* m_settingsButton = nullptr;
    lv_obj_t* m_statusLabel = nullptr;
    lv_obj_t* m_resolutionLabel = nullptr;

    // Statistics
    uint32_t m_photosCaptured = 0;
    uint32_t m_videosRecorded = 0;
    uint32_t m_totalFrames = 0;
    uint32_t m_lastFrameTime = 0;
    float m_frameRate = 0.0f;

    // Image buffer for preview
    lv_img_dsc_t m_previewImageDesc;
    uint8_t* m_previewBuffer = nullptr;
    size_t m_previewBufferSize = 0;

    // Configuration
    static constexpr uint32_t PREVIEW_UPDATE_INTERVAL = 33; // ~30 FPS
    static constexpr uint32_t PREVIEW_WIDTH = 320;
    static constexpr uint32_t PREVIEW_HEIGHT = 240;
    static constexpr size_t MAX_FILENAME_LENGTH = 64;
};

#endif // CAMERA_APP_H