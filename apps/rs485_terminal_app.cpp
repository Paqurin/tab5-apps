#include "rs485_terminal_app.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <driver/gpio.h>
#include <sys/time.h>
#include <string.h>

static const char* TAG = "RS485Terminal";

RS485TerminalApp::RS485TerminalApp() 
    : BaseApp("rs485_terminal", "RS-485 Terminal", "1.0.0") {
    setDescription("RS-485 communication terminal with monitoring");
    setAuthor("M5Stack Tab5 OS");
    setPriority(AppPriority::APP_NORMAL);
}

RS485TerminalApp::~RS485TerminalApp() {
    shutdown();
}

os_error_t RS485TerminalApp::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Initializing RS-485 Terminal Application");

    // Allocate communication buffers
    m_rxBufferSize = RX_BUFFER_SIZE;
    m_txBufferSize = TX_BUFFER_SIZE;
    
    m_rxBuffer = (uint8_t*)heap_caps_malloc(m_rxBufferSize, MALLOC_CAP_DMA);
    m_txBuffer = (uint8_t*)heap_caps_malloc(m_txBufferSize, MALLOC_CAP_DMA);
    
    if (!m_rxBuffer || !m_txBuffer) {
        log(ESP_LOG_ERROR, "Failed to allocate communication buffers");
        return OS_ERROR_NO_MEMORY;
    }

    // Initialize RS-485 hardware
    os_error_t result = initializeRS485();
    if (result != OS_OK) {
        log(ESP_LOG_ERROR, "Failed to initialize RS-485 hardware");
        return result;
    }

    setMemoryUsage(m_rxBufferSize + m_txBufferSize + TERMINAL_BUFFER_SIZE);
    m_initialized = true;

    log(ESP_LOG_INFO, "RS-485 Terminal application initialized successfully");
    return OS_OK;
}

os_error_t RS485TerminalApp::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    // Process received data if available
    if (m_rs485Initialized) {
        processReceivedData();
    }

    return OS_OK;
}

os_error_t RS485TerminalApp::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Shutting down RS-485 Terminal Application");

    // Stop UART task
    if (m_uartTaskHandle) {
        m_taskRunning = false;
        vTaskDelete(m_uartTaskHandle);
        m_uartTaskHandle = nullptr;
    }

    // Deinitialize UART
    if (m_rs485Initialized) {
        uart_driver_delete(UART_PORT);
        m_rs485Initialized = false;
    }

    // Free buffers
    if (m_rxBuffer) {
        heap_caps_free(m_rxBuffer);
        m_rxBuffer = nullptr;
    }
    if (m_txBuffer) {
        heap_caps_free(m_txBuffer);
        m_txBuffer = nullptr;
    }

    m_initialized = false;
    return OS_OK;
}

os_error_t RS485TerminalApp::createUI(lv_obj_t* parent) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    log(ESP_LOG_INFO, "Creating RS-485 Terminal UI");

    // Create main container
    m_uiContainer = lv_obj_create(parent);
    lv_obj_set_size(m_uiContainer, LV_HOR_RES, LV_VER_RES - OS_STATUS_BAR_HEIGHT - OS_DOCK_HEIGHT);
    lv_obj_align(m_uiContainer, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(m_uiContainer, lv_color_black(), 0);
    lv_obj_set_style_border_opa(m_uiContainer, LV_OPA_TRANSP, 0);

    createTerminalUI();
    createControlPanel();

    return OS_OK;
}

os_error_t RS485TerminalApp::destroyUI() {
    log(ESP_LOG_INFO, "Destroying RS-485 Terminal UI");

    if (m_uiContainer) {
        lv_obj_del(m_uiContainer);
        m_uiContainer = nullptr;
        m_terminalContainer = nullptr;
        m_terminalTextArea = nullptr;
        m_inputTextArea = nullptr;
        m_controlPanel = nullptr;
        m_sendButton = nullptr;
        m_clearButton = nullptr;
        m_configButton = nullptr;
        m_modeButton = nullptr;
        m_statusLabel = nullptr;
        m_configLabel = nullptr;
    }

    return OS_OK;
}

os_error_t RS485TerminalApp::handleEvent(uint32_t eventType, void* eventData, size_t dataSize) {
    // Handle RS-485 specific events
    return OS_OK;
}

os_error_t RS485TerminalApp::initializeRS485() {
    log(ESP_LOG_INFO, "Initializing RS-485 hardware");

    // Configure UART
    uart_config_t uart_config = {
        .baud_rate = (int)m_config.baudRate,
        .data_bits = (uart_word_length_t)m_config.dataBits,
        .parity = (uart_parity_t)m_config.parity,
        .stop_bits = (uart_stop_bits_t)m_config.stopBits,
        .flow_ctrl = m_config.flowControl ? UART_HW_FLOWCTRL_CTS_RTS : UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };

    esp_err_t ret = uart_param_config(UART_PORT, &uart_config);
    if (ret != ESP_OK) {
        log(ESP_LOG_ERROR, "Failed to configure UART: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Set UART pins
    ret = uart_set_pin(UART_PORT, RS485_TX_PIN, RS485_RX_PIN, 
                       RS485_RTS_PIN, RS485_CTS_PIN);
    if (ret != ESP_OK) {
        log(ESP_LOG_ERROR, "Failed to set UART pins: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Install UART driver
    ret = uart_driver_install(UART_PORT, m_rxBufferSize, m_txBufferSize, 0, NULL, 0);
    if (ret != ESP_OK) {
        log(ESP_LOG_ERROR, "Failed to install UART driver: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Configure RS-485 mode
    ret = uart_set_mode(UART_PORT, UART_MODE_RS485_HALF_DUPLEX);
    if (ret != ESP_OK) {
        log(ESP_LOG_ERROR, "Failed to set RS-485 mode: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Configure RS-485 direction pin
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL << RS485_DE_PIN;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        log(ESP_LOG_ERROR, "Failed to configure RS-485 DE pin: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Start in receive mode
    setTransmitMode(false);

    // Create UART processing task
    m_taskRunning = true;
    BaseType_t taskResult = xTaskCreate(
        uartTask,
        "RS485_Task",
        UART_TASK_STACK_SIZE,
        this,
        UART_TASK_PRIORITY,
        &m_uartTaskHandle
    );

    if (taskResult != pdPASS) {
        log(ESP_LOG_ERROR, "Failed to create UART task");
        return OS_ERROR_GENERIC;
    }

    m_rs485Initialized = true;
    log(ESP_LOG_INFO, "RS-485 hardware initialized successfully");
    return OS_OK;
}

os_error_t RS485TerminalApp::configureRS485(const RS485Config& config) {
    if (!m_rs485Initialized) {
        return OS_ERROR_GENERIC;
    }

    log(ESP_LOG_INFO, "Configuring RS-485: %d baud, %d data bits", 
        (int)config.baudRate, (int)config.dataBits);

    m_config = config;

    uart_config_t uart_config = {
        .baud_rate = (int)config.baudRate,
        .data_bits = (uart_word_length_t)config.dataBits,
        .parity = (uart_parity_t)config.parity,
        .stop_bits = (uart_stop_bits_t)config.stopBits,
        .flow_ctrl = config.flowControl ? UART_HW_FLOWCTRL_CTS_RTS : UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
    };

    esp_err_t ret = uart_param_config(UART_PORT, &uart_config);
    if (ret != ESP_OK) {
        log(ESP_LOG_ERROR, "Failed to reconfigure UART: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Update config label if UI is active
    if (m_configLabel) {
        char configText[64];
        snprintf(configText, sizeof(configText), "%d,%d,%s,%s", 
                (int)config.baudRate, (int)config.dataBits,
                config.parity == RS485Parity::PARITY_NONE ? "N" : 
                config.parity == RS485Parity::PARITY_EVEN ? "E" : "O",
                config.stopBits == RS485StopBits::STOP_1_BIT ? "1" : "2");
        lv_label_set_text(m_configLabel, configText);
    }

    return OS_OK;
}

os_error_t RS485TerminalApp::sendData(const uint8_t* data, size_t length) {
    if (!m_rs485Initialized || !data || length == 0) {
        return OS_ERROR_INVALID_PARAM;
    }

    // Switch to transmit mode
    setTransmitMode(true);
    
    // Small delay to ensure transceiver is ready
    vTaskDelay(pdMS_TO_TICKS(1));

    // Send data
    int written = uart_write_bytes(UART_PORT, data, length);
    if (written != length) {
        log(ESP_LOG_ERROR, "Failed to send complete data: %d/%d bytes", written, length);
        setTransmitMode(false);
        return OS_ERROR_GENERIC;
    }

    // Wait for transmission to complete
    uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(m_config.timeout));

    // Switch back to receive mode
    setTransmitMode(false);

    // Update statistics
    m_bytesTransmitted += length;
    m_packetsTransmitted++;

    // Add to terminal display
    char displayBuffer[512];
    formatDataForDisplay(data, length, displayBuffer, sizeof(displayBuffer));
    addToTerminal(displayBuffer, true);

    log(ESP_LOG_DEBUG, "Sent %d bytes via RS-485", length);
    return OS_OK;
}

os_error_t RS485TerminalApp::sendString(const char* text) {
    if (!text) {
        return OS_ERROR_INVALID_PARAM;
    }

    size_t length = strlen(text);
    return sendData((const uint8_t*)text, length);
}

void RS485TerminalApp::clearTerminal() {
    if (m_terminalTextArea) {
        lv_textarea_set_text(m_terminalTextArea, "");
        log(ESP_LOG_INFO, "Terminal cleared");
    }
}

void RS485TerminalApp::setTransmitMode(bool transmit) {
    m_transmitMode = transmit;
    gpio_set_level(RS485_DE_PIN, transmit ? 1 : 0);
    
    // Update status if UI is active
    if (m_statusLabel) {
        lv_label_set_text(m_statusLabel, transmit ? "TX" : "RX");
        lv_obj_set_style_text_color(m_statusLabel, 
                                   transmit ? lv_color_hex(0xFF6B6B) : lv_color_hex(0x51CF66), 0);
    }
}

void RS485TerminalApp::printRS485Stats() const {
    log(ESP_LOG_INFO, "RS-485 Statistics:");
    log(ESP_LOG_INFO, "  Bytes RX: %u", m_bytesReceived);
    log(ESP_LOG_INFO, "  Bytes TX: %u", m_bytesTransmitted);
    log(ESP_LOG_INFO, "  Packets RX: %u", m_packetsReceived);
    log(ESP_LOG_INFO, "  Packets TX: %u", m_packetsTransmitted);
    log(ESP_LOG_INFO, "  Errors: %u", m_errorCount);
    log(ESP_LOG_INFO, "  Baud Rate: %d", (int)m_config.baudRate);
    log(ESP_LOG_INFO, "  Mode: %s", m_transmitMode ? "TX" : "RX");
}

void RS485TerminalApp::createTerminalUI() {
    // Create terminal container
    m_terminalContainer = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_terminalContainer, LV_HOR_RES - 200, LV_VER_RES - 150);
    lv_obj_align(m_terminalContainer, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(m_terminalContainer, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_color(m_terminalContainer, lv_color_white(), 0);
    lv_obj_set_style_border_width(m_terminalContainer, 1, 0);

    // Create terminal text area (output)
    m_terminalTextArea = lv_textarea_create(m_terminalContainer);
    lv_obj_set_size(m_terminalTextArea, LV_HOR_RES - 220, LV_VER_RES - 220);
    lv_obj_align(m_terminalTextArea, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_textarea_set_text(m_terminalTextArea, "RS-485 Terminal Ready\n");
    lv_obj_set_style_bg_color(m_terminalTextArea, lv_color_black(), 0);
    lv_obj_set_style_text_color(m_terminalTextArea, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(m_terminalTextArea, &lv_font_montserrat_12, 0);
    // Hide cursor by setting it to last position
    lv_textarea_set_cursor_pos(m_terminalTextArea, LV_TEXTAREA_CURSOR_LAST);

    // Create input text area
    m_inputTextArea = lv_textarea_create(m_uiContainer);
    lv_obj_set_size(m_inputTextArea, LV_HOR_RES - 220, 50);
    lv_obj_align(m_inputTextArea, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_textarea_set_placeholder_text(m_inputTextArea, "Enter command...");
    lv_obj_set_style_bg_color(m_inputTextArea, lv_color_hex(0x333333), 0);
    lv_obj_set_style_text_color(m_inputTextArea, lv_color_white(), 0);
}

void RS485TerminalApp::createControlPanel() {
    // Create control panel
    m_controlPanel = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_controlPanel, 180, LV_VER_RES - 50);
    lv_obj_align(m_controlPanel, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_style_bg_color(m_controlPanel, lv_color_hex(0x333333), 0);

    // Status label
    m_statusLabel = lv_label_create(m_controlPanel);
    lv_label_set_text(m_statusLabel, "RX");
    lv_obj_set_style_text_color(m_statusLabel, lv_color_hex(0x51CF66), 0);
    lv_obj_set_style_text_font(m_statusLabel, &lv_font_montserrat_14, 0);
    lv_obj_align(m_statusLabel, LV_ALIGN_TOP_MID, 0, 10);

    // Configuration label
    m_configLabel = lv_label_create(m_controlPanel);
    lv_label_set_text(m_configLabel, "9600,8,N,1");
    lv_obj_set_style_text_color(m_configLabel, lv_color_white(), 0);
    lv_obj_align(m_configLabel, LV_ALIGN_TOP_MID, 0, 35);

    // Send button
    m_sendButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_sendButton, 160, 40);
    lv_obj_align(m_sendButton, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_add_event_cb(m_sendButton, sendButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* sendLabel = lv_label_create(m_sendButton);
    lv_label_set_text(sendLabel, "SEND");
    lv_obj_center(sendLabel);

    // Clear button
    m_clearButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_clearButton, 160, 40);
    lv_obj_align(m_clearButton, LV_ALIGN_TOP_MID, 0, 120);
    lv_obj_add_event_cb(m_clearButton, clearButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* clearLabel = lv_label_create(m_clearButton);
    lv_label_set_text(clearLabel, "CLEAR");
    lv_obj_center(clearLabel);

    // Config button
    m_configButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_configButton, 160, 40);
    lv_obj_align(m_configButton, LV_ALIGN_TOP_MID, 0, 170);
    lv_obj_add_event_cb(m_configButton, configButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* configLabel = lv_label_create(m_configButton);
    lv_label_set_text(configLabel, "CONFIG");
    lv_obj_center(configLabel);

    // Mode button (HEX/ASCII toggle)
    m_modeButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_modeButton, 160, 40);
    lv_obj_align(m_modeButton, LV_ALIGN_TOP_MID, 0, 220);
    lv_obj_add_event_cb(m_modeButton, modeButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* modeLabel = lv_label_create(m_modeButton);
    lv_label_set_text(modeLabel, "ASCII");
    lv_obj_center(modeLabel);
}

void RS485TerminalApp::processReceivedData() {
    size_t length;
    uart_get_buffered_data_len(UART_PORT, &length);
    
    if (length > 0) {
        if (length > m_rxBufferSize) {
            length = m_rxBufferSize;
        }
        
        int bytesRead = uart_read_bytes(UART_PORT, m_rxBuffer, length, pdMS_TO_TICKS(10));
        if (bytesRead > 0) {
            // Update statistics
            m_bytesReceived += bytesRead;
            m_packetsReceived++;
            
            // Add to terminal display
            char displayBuffer[512];
            formatDataForDisplay(m_rxBuffer, bytesRead, displayBuffer, sizeof(displayBuffer));
            addToTerminal(displayBuffer, false);
        }
    }
}

void RS485TerminalApp::addToTerminal(const char* text, bool isTransmitted) {
    if (!m_terminalTextArea || !text) {
        return;
    }

    // Get current time for timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm* timeinfo = localtime(&tv.tv_sec);
    
    char timestamp[32];
    if (m_showTimestamp) {
        snprintf(timestamp, sizeof(timestamp), "[%02d:%02d:%02d.%03ld] ", 
                timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, tv.tv_usec / 1000);
    } else {
        timestamp[0] = '\0';
    }

    // Format the line with direction indicator
    char line[1024];
    snprintf(line, sizeof(line), "%s%s %s\n", 
            timestamp, isTransmitted ? "TX:" : "RX:", text);

    // Add to text area
    lv_textarea_add_text(m_terminalTextArea, line);

    // Auto-scroll to bottom if enabled
    if (m_autoScroll) {
        lv_textarea_set_cursor_pos(m_terminalTextArea, LV_TEXTAREA_CURSOR_LAST);
    }
}

void RS485TerminalApp::formatDataForDisplay(const uint8_t* data, size_t length, 
                                          char* buffer, size_t bufferSize) {
    if (!data || !buffer || bufferSize == 0) {
        return;
    }

    if (m_hexDisplay) {
        // Format as hex
        size_t pos = 0;
        for (size_t i = 0; i < length && pos < bufferSize - 4; i++) {
            pos += snprintf(buffer + pos, bufferSize - pos, "%02X ", data[i]);
        }
        if (pos > 0 && pos < bufferSize) {
            buffer[pos - 1] = '\0'; // Remove trailing space
        }
    } else {
        // Format as ASCII (with printable character substitution)
        size_t copyLength = (length < bufferSize - 1) ? length : bufferSize - 1;
        for (size_t i = 0; i < copyLength; i++) {
            buffer[i] = (data[i] >= 32 && data[i] <= 126) ? data[i] : '.';
        }
        buffer[copyLength] = '\0';
    }
}

void RS485TerminalApp::sendButtonCallback(lv_event_t* e) {
    RS485TerminalApp* app = static_cast<RS485TerminalApp*>(lv_event_get_user_data(e));
    if (app && app->m_inputTextArea) {
        const char* text = lv_textarea_get_text(app->m_inputTextArea);
        if (text && strlen(text) > 0) {
            app->sendString(text);
            app->sendString("\r\n"); // Add CRLF
            lv_textarea_set_text(app->m_inputTextArea, ""); // Clear input
        }
    }
}

void RS485TerminalApp::clearButtonCallback(lv_event_t* e) {
    RS485TerminalApp* app = static_cast<RS485TerminalApp*>(lv_event_get_user_data(e));
    if (app) {
        app->clearTerminal();
    }
}

void RS485TerminalApp::configButtonCallback(lv_event_t* e) {
    RS485TerminalApp* app = static_cast<RS485TerminalApp*>(lv_event_get_user_data(e));
    if (app) {
        // Cycle through common baud rates
        RS485BaudRate rates[] = {
            RS485BaudRate::BAUD_9600, RS485BaudRate::BAUD_19200,
            RS485BaudRate::BAUD_38400, RS485BaudRate::BAUD_57600,
            RS485BaudRate::BAUD_115200
        };
        
        // Find current rate and switch to next
        for (int i = 0; i < 5; i++) {
            if (app->m_config.baudRate == rates[i]) {
                RS485Config newConfig = app->m_config;
                newConfig.baudRate = rates[(i + 1) % 5];
                app->configureRS485(newConfig);
                break;
            }
        }
    }
}

void RS485TerminalApp::modeButtonCallback(lv_event_t* e) {
    RS485TerminalApp* app = static_cast<RS485TerminalApp*>(lv_event_get_user_data(e));
    if (app && app->m_modeButton) {
        app->m_hexDisplay = !app->m_hexDisplay;
        
        lv_obj_t* label = lv_obj_get_child(app->m_modeButton, 0);
        if (label) {
            lv_label_set_text(label, app->m_hexDisplay ? "HEX" : "ASCII");
        }
    }
}

void RS485TerminalApp::uartTask(void* parameter) {
    RS485TerminalApp* app = static_cast<RS485TerminalApp*>(parameter);
    
    while (app->m_taskRunning) {
        // This task handles background UART processing if needed
        // Main processing is done in processReceivedData()
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    vTaskDelete(NULL);
}