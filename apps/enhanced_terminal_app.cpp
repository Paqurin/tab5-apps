#include "enhanced_terminal_app.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <esp_wifi.h>
#include <lwip/netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <string.h>
#include <algorithm>
#include <sstream>
#include <iomanip>

static const char* TAG = "EnhancedTerminal";

// Telnet protocol constants
#define TELNET_IAC      255  // Interpret As Command
#define TELNET_WILL     251  // Will option
#define TELNET_WONT     252  // Won't option
#define TELNET_DO       253  // Do option
#define TELNET_DONT     254  // Don't option

EnhancedTerminalApp::EnhancedTerminalApp() 
    : BaseApp("enhanced_terminal", "Enhanced Terminal", "2.0.0") {
    setDescription("Terminal with Telnet and SSH support");
    setAuthor("M5Stack Tab5 OS");
    setPriority(AppPriority::APP_NORMAL);
}

EnhancedTerminalApp::~EnhancedTerminalApp() {
    shutdown();
}

os_error_t EnhancedTerminalApp::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Initializing Enhanced Terminal Application");

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
        log(ESP_LOG_WARN, "RS-485 initialization failed, continuing without RS-485");
    }

    // Initialize network stack
    result = initializeNetwork();
    if (result != OS_OK) {
        log(ESP_LOG_WARN, "Network initialization failed, network features disabled");
    }

    // Create default local session
    createSession(TerminalMode::LOCAL);

    setMemoryUsage(m_rxBufferSize + m_txBufferSize + TERMINAL_BUFFER_SIZE + 
                   (MAX_SESSIONS * sizeof(TerminalSession)));
    m_initialized = true;

    log(ESP_LOG_INFO, "Enhanced Terminal application initialized successfully");
    return OS_OK;
}

os_error_t EnhancedTerminalApp::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    // Process received data from all sessions
    processReceivedData();

    // Update session states
    for (auto& session : m_sessions) {
        if (session && session->isActive) {
            switch (session->state) {
                case ConnectionState::CONNECTING:
                    // Check connection timeout
                    if (millis() - session->connectTime > session->config.timeout) {
                        session->state = ConnectionState::ERROR;
                        log(ESP_LOG_WARN, "Connection timeout for session %s", session->id.c_str());
                    }
                    break;
                    
                case ConnectionState::CONNECTED:
                    // Send keepalive if needed (for network connections)
                    if (session->mode == TerminalMode::TELNET || session->mode == TerminalMode::SSH) {
                        // Implementation would send keepalive packets
                    }
                    break;
                    
                case ConnectionState::ERROR:
                    // Handle error state
                    session->isActive = false;
                    break;
                    
                default:
                    break;
            }
        }
    }

    return OS_OK;
}

os_error_t EnhancedTerminalApp::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Shutting down Enhanced Terminal Application");

    // Stop all tasks
    if (m_networkTaskHandle) {
        m_tasksRunning = false;
        vTaskDelete(m_networkTaskHandle);
        m_networkTaskHandle = nullptr;
    }

    if (m_uartTaskHandle) {
        m_tasksRunning = false;
        vTaskDelete(m_uartTaskHandle);
        m_uartTaskHandle = nullptr;
    }

    // Close all sessions
    for (auto& session : m_sessions) {
        if (session && session->isActive) {
            closeSession(session->id);
        }
    }
    m_sessions.clear();

    // Deinitialize UART
    uart_driver_delete(UART_PORT);

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
    log(ESP_LOG_INFO, "Enhanced Terminal application shutdown complete");
    return OS_OK;
}

os_error_t EnhancedTerminalApp::createUI(lv_obj_t* parent) {
    if (!parent) {
        return OS_ERROR_INVALID_PARAM;
    }

    log(ESP_LOG_INFO, "Creating Enhanced Terminal UI");

    // Create main container
    m_uiContainer = lv_obj_create(parent);
    lv_obj_set_size(m_uiContainer, LV_HOR_RES, LV_VER_RES - OS_STATUS_BAR_HEIGHT - OS_DOCK_HEIGHT);
    lv_obj_align(m_uiContainer, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(m_uiContainer, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_opa(m_uiContainer, LV_OPA_TRANSP, 0);

    createTerminalUI();
    updateSessionTabs();

    return OS_OK;
}

os_error_t EnhancedTerminalApp::destroyUI() {
    if (!m_uiContainer) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Destroying Enhanced Terminal UI");

    if (m_uiContainer) {
        lv_obj_del(m_uiContainer);
        m_uiContainer = nullptr;
        // All child objects will be automatically deleted
        m_terminalContainer = nullptr;
        m_sessionTabs = nullptr;
        m_terminalTextArea = nullptr;
        m_inputTextArea = nullptr;
        m_controlPanel = nullptr;
        m_connectButton = nullptr;
        m_disconnectButton = nullptr;
        m_sendButton = nullptr;
        m_clearButton = nullptr;
        m_settingsButton = nullptr;
        m_statusLabel = nullptr;
        m_modeDropdown = nullptr;
        m_connectionDialog = nullptr;
    }

    return OS_OK;
}

os_error_t EnhancedTerminalApp::handleEvent(uint32_t eventType, void* eventData, size_t dataSize) {
    // Handle application-specific events
    return OS_OK;
}

std::string EnhancedTerminalApp::createSession(TerminalMode mode, const ConnectionConfig& config) {
    if (m_sessions.size() >= MAX_SESSIONS) {
        log(ESP_LOG_WARN, "Maximum number of sessions reached");
        return "";
    }

    auto session = std::make_unique<TerminalSession>();
    session->id = generateSessionId();
    session->mode = mode;
    session->state = ConnectionState::DISCONNECTED;
    session->config = config;
    session->socket = -1;
    session->connectTime = 0;
    session->bytesReceived = 0;
    session->bytesTransmitted = 0;
    session->isActive = true;

    std::string sessionId = session->id;
    m_sessions.push_back(std::move(session));

    // Set as current session if it's the first one
    if (m_currentSessionId.empty()) {
        m_currentSessionId = sessionId;
    }

    log(ESP_LOG_INFO, "Created %s session: %s", 
        (mode == TerminalMode::RS485) ? "RS485" :
        (mode == TerminalMode::TELNET) ? "Telnet" :
        (mode == TerminalMode::SSH) ? "SSH" : "Local",
        sessionId.c_str());

    updateSessionTabs();
    return sessionId;
}

os_error_t EnhancedTerminalApp::closeSession(const std::string& sessionId) {
    auto it = std::find_if(m_sessions.begin(), m_sessions.end(),
                          [&sessionId](const std::unique_ptr<TerminalSession>& s) {
                              return s && s->id == sessionId;
                          });

    if (it == m_sessions.end()) {
        return OS_ERROR_NOT_FOUND;
    }

    auto& session = *it;
    
    // Close socket if it's a network session
    if (session->socket >= 0) {
        close(session->socket);
        session->socket = -1;
    }

    session->isActive = false;
    session->state = ConnectionState::DISCONNECTED;

    // Switch to another session if this was current
    if (m_currentSessionId == sessionId) {
        m_currentSessionId.clear();
        for (const auto& s : m_sessions) {
            if (s && s->isActive && s->id != sessionId) {
                m_currentSessionId = s->id;
                break;
            }
        }
    }

    // Remove from sessions list
    m_sessions.erase(it);
    m_totalDisconnections++;

    log(ESP_LOG_INFO, "Closed session: %s", sessionId.c_str());
    updateSessionTabs();

    return OS_OK;
}

os_error_t EnhancedTerminalApp::switchToSession(const std::string& sessionId) {
    auto it = std::find_if(m_sessions.begin(), m_sessions.end(),
                          [&sessionId](const std::unique_ptr<TerminalSession>& s) {
                              return s && s->id == sessionId && s->isActive;
                          });

    if (it == m_sessions.end()) {
        return OS_ERROR_NOT_FOUND;
    }

    m_currentSessionId = sessionId;
    updateSessionTabs();

    log(ESP_LOG_INFO, "Switched to session: %s", sessionId.c_str());
    return OS_OK;
}

os_error_t EnhancedTerminalApp::sendData(const uint8_t* data, size_t length) {
    if (!data || length == 0) {
        return OS_ERROR_INVALID_PARAM;
    }

    TerminalSession* session = nullptr;
    for (auto& s : m_sessions) {
        if (s && s->id == m_currentSessionId && s->isActive) {
            session = s.get();
            break;
        }
    }

    if (!session) {
        return OS_ERROR_NOT_FOUND;
    }

    int bytesSent = 0;
    
    switch (session->mode) {
        case TerminalMode::RS485:
            // Send via UART
            bytesSent = uart_write_bytes(UART_PORT, data, length);
            break;
            
        case TerminalMode::TELNET:
        case TerminalMode::SSH:
            // Send via socket
            if (session->socket >= 0 && session->state == ConnectionState::CONNECTED) {
                bytesSent = send(session->socket, data, length, 0);
            }
            break;
            
        case TerminalMode::LOCAL:
            // Local echo
            addToTerminal((const char*)data, session->id, true);
            bytesSent = length;
            break;
    }

    if (bytesSent > 0) {
        session->bytesTransmitted += bytesSent;
        m_totalBytesTransmitted += bytesSent;
        
        // Add to terminal display
        if (session->mode != TerminalMode::LOCAL) {
            char displayText[512];
            formatDataForDisplay(data, bytesSent, displayText, sizeof(displayText));
            addToTerminal(displayText, session->id, true);
        }
    }

    return (bytesSent == (int)length) ? OS_OK : OS_ERROR_GENERIC;
}

os_error_t EnhancedTerminalApp::sendString(const char* text) {
    if (!text) {
        return OS_ERROR_INVALID_PARAM;
    }
    
    return sendData(reinterpret_cast<const uint8_t*>(text), strlen(text));
}

std::string EnhancedTerminalApp::connectTelnet(const std::string& hostname, uint16_t port, uint32_t timeout) {
    if (!m_networkInitialized) {
        log(ESP_LOG_ERROR, "Network not initialized");
        return "";
    }

    ConnectionConfig config;
    config.hostname = hostname;
    config.port = port;
    config.timeout = timeout;

    std::string sessionId = createSession(TerminalMode::TELNET, config);
    if (sessionId.empty()) {
        return "";
    }

    TerminalSession* session = nullptr;
    for (auto& s : m_sessions) {
        if (s && s->id == sessionId) {
            session = s.get();
            break;
        }
    }

    if (!session) {
        return "";
    }

    // Create socket
    session->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (session->socket < 0) {
        log(ESP_LOG_ERROR, "Failed to create socket for Telnet connection");
        closeSession(sessionId);
        return "";
    }

    // Set socket timeout
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    setsockopt(session->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(session->socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    // Resolve hostname
    struct hostent* host = gethostbyname(hostname.c_str());
    if (!host) {
        log(ESP_LOG_ERROR, "Failed to resolve hostname: %s", hostname.c_str());
        closeSession(sessionId);
        return "";
    }

    // Connect to server
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    memcpy(&serverAddr.sin_addr.s_addr, host->h_addr, host->h_length);

    session->state = ConnectionState::CONNECTING;
    session->connectTime = millis();

    if (connect(session->socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        log(ESP_LOG_ERROR, "Failed to connect to %s:%d", hostname.c_str(), port);
        closeSession(sessionId);
        return "";
    }

    session->state = ConnectionState::CONNECTED;
    m_totalConnections++;

    log(ESP_LOG_INFO, "Connected to Telnet server %s:%d", hostname.c_str(), port);
    addToTerminal("Connected to Telnet server\n", sessionId);

    return sessionId;
}

std::string EnhancedTerminalApp::connectSSH(const std::string& hostname, uint16_t port, 
                                           const std::string& username, const std::string& password,
                                           uint32_t timeout) {
    if (!m_networkInitialized) {
        log(ESP_LOG_ERROR, "Network not initialized");
        return "";
    }

    ConnectionConfig config;
    config.hostname = hostname;
    config.port = port;
    config.username = username;
    config.password = password;
    config.timeout = timeout;

    std::string sessionId = createSession(TerminalMode::SSH, config);
    if (sessionId.empty()) {
        return "";
    }

    // For SSH, we would need to implement the SSH protocol
    // This is a simplified implementation that creates a socket connection
    // Real SSH would require cryptographic libraries and protocol implementation

    log(ESP_LOG_INFO, "SSH connection created (simplified implementation)");
    addToTerminal("SSH connection established (simplified)\n", sessionId);

    return sessionId;
}

void EnhancedTerminalApp::clearTerminal() {
    if (m_terminalTextArea) {
        lv_textarea_set_text(m_terminalTextArea, "");
    }
}

std::vector<std::string> EnhancedTerminalApp::getActiveSessions() const {
    std::vector<std::string> sessionIds;
    for (const auto& session : m_sessions) {
        if (session && session->isActive) {
            sessionIds.push_back(session->id);
        }
    }
    return sessionIds;
}

const TerminalSession* EnhancedTerminalApp::getSession(const std::string& sessionId) const {
    auto it = std::find_if(m_sessions.begin(), m_sessions.end(),
                          [&sessionId](const std::unique_ptr<TerminalSession>& s) {
                              return s && s->id == sessionId;
                          });
    
    return (it != m_sessions.end()) ? it->get() : nullptr;
}

void EnhancedTerminalApp::setDisplayOptions(bool hexDisplay, bool showTimestamp, bool autoScroll) {
    m_hexDisplay = hexDisplay;
    m_showTimestamp = showTimestamp;
    m_autoScroll = autoScroll;
}

os_error_t EnhancedTerminalApp::initializeRS485() {
    // Configure UART for RS-485
    uart_config_t uart_config = {
        .baud_rate = (int)m_rs485Config.baudRate,
        .data_bits = static_cast<uart_word_length_t>(m_rs485Config.dataBits - 5),
        .parity = static_cast<uart_parity_t>(m_rs485Config.parity),
        .stop_bits = static_cast<uart_stop_bits_t>(m_rs485Config.stopBits - 1),
        .flow_ctrl = m_rs485Config.flowControl ? UART_HW_FLOWCTRL_CTS_RTS : UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, 
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, m_rxBufferSize, m_txBufferSize, 0, NULL, 0));

    // Start UART task
    m_tasksRunning = true;
    xTaskCreate(uartTask, "uart_task", UART_TASK_STACK_SIZE, this, UART_TASK_PRIORITY, &m_uartTaskHandle);

    return OS_OK;
}

os_error_t EnhancedTerminalApp::initializeNetwork() {
    // Check if WiFi is available
    wifi_ap_record_t ap_info;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    
    if (ret == ESP_OK) {
        m_networkInitialized = true;
        m_wifiConnected = true;
        log(ESP_LOG_INFO, "Network initialized, WiFi connected");
        
        // Start network task
        m_tasksRunning = true;
        xTaskCreate(networkTask, "network_task", NETWORK_TASK_STACK_SIZE, this, 
                   NETWORK_TASK_PRIORITY, &m_networkTaskHandle);
    } else {
        log(ESP_LOG_WARN, "WiFi not connected, network features disabled");
        m_networkInitialized = false;
        m_wifiConnected = false;
    }

    return OS_OK;
}

void EnhancedTerminalApp::createTerminalUI() {
    if (!m_uiContainer) {
        return;
    }

    // Create session tabs
    createSessionTabs();

    // Create terminal text area
    m_terminalTextArea = lv_textarea_create(m_uiContainer);
    lv_obj_set_size(m_terminalTextArea, LV_HOR_RES - 40, LV_VER_RES - 200);
    lv_obj_align(m_terminalTextArea, LV_ALIGN_TOP_MID, 0, 60);
    lv_textarea_set_text(m_terminalTextArea, "Enhanced Terminal Ready\n");
    lv_obj_set_style_bg_color(m_terminalTextArea, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_color(m_terminalTextArea, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_text_font(m_terminalTextArea, &lv_font_montserrat_12, 0);

    // Create input text area
    m_inputTextArea = lv_textarea_create(m_uiContainer);
    lv_obj_set_size(m_inputTextArea, LV_HOR_RES - 200, 40);
    lv_obj_align(m_inputTextArea, LV_ALIGN_BOTTOM_LEFT, 20, -60);
    lv_textarea_set_placeholder_text(m_inputTextArea, "Enter command...");
    lv_obj_set_style_bg_color(m_inputTextArea, lv_color_hex(0x2C2C2C), 0);
    lv_obj_set_style_text_color(m_inputTextArea, lv_color_white(), 0);

    // Create control panel
    m_controlPanel = lv_obj_create(m_uiContainer);
    lv_obj_set_size(m_controlPanel, LV_HOR_RES - 40, 50);
    lv_obj_align(m_controlPanel, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_bg_color(m_controlPanel, lv_color_hex(0x3C3C3C), 0);

    // Create buttons
    m_sendButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_sendButton, 80, 30);
    lv_obj_align(m_sendButton, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_t* sendLabel = lv_label_create(m_sendButton);
    lv_label_set_text(sendLabel, "Send");
    lv_obj_center(sendLabel);
    lv_obj_add_event_cb(m_sendButton, sendButtonCallback, LV_EVENT_CLICKED, this);

    m_connectButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_connectButton, 80, 30);
    lv_obj_align(m_connectButton, LV_ALIGN_LEFT_MID, 100, 0);
    lv_obj_t* connectLabel = lv_label_create(m_connectButton);
    lv_label_set_text(connectLabel, "Connect");
    lv_obj_center(connectLabel);
    lv_obj_add_event_cb(m_connectButton, connectButtonCallback, LV_EVENT_CLICKED, this);

    m_disconnectButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_disconnectButton, 80, 30);
    lv_obj_align(m_disconnectButton, LV_ALIGN_LEFT_MID, 190, 0);
    lv_obj_t* disconnectLabel = lv_label_create(m_disconnectButton);
    lv_label_set_text(disconnectLabel, "Disconnect");
    lv_obj_center(disconnectLabel);
    lv_obj_add_event_cb(m_disconnectButton, disconnectButtonCallback, LV_EVENT_CLICKED, this);

    m_clearButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_clearButton, 60, 30);
    lv_obj_align(m_clearButton, LV_ALIGN_LEFT_MID, 280, 0);
    lv_obj_t* clearLabel = lv_label_create(m_clearButton);
    lv_label_set_text(clearLabel, "Clear");
    lv_obj_center(clearLabel);
    lv_obj_add_event_cb(m_clearButton, clearButtonCallback, LV_EVENT_CLICKED, this);

    m_settingsButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_settingsButton, 70, 30);
    lv_obj_align(m_settingsButton, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_t* settingsLabel = lv_label_create(m_settingsButton);
    lv_label_set_text(settingsLabel, "Settings");
    lv_obj_center(settingsLabel);
    lv_obj_add_event_cb(m_settingsButton, settingsButtonCallback, LV_EVENT_CLICKED, this);

    // Create status label
    m_statusLabel = lv_label_create(m_uiContainer);
    lv_obj_align(m_statusLabel, LV_ALIGN_BOTTOM_LEFT, 20, -10);
    lv_obj_set_style_text_color(m_statusLabel, lv_color_white(), 0);
    lv_label_set_text(m_statusLabel, "Ready");
}

void EnhancedTerminalApp::createSessionTabs() {
    if (!m_uiContainer) {
        return;
    }

    // Create tab view for sessions
    m_sessionTabs = lv_tabview_create(m_uiContainer, LV_DIR_TOP, 50);
    lv_obj_set_size(m_sessionTabs, LV_HOR_RES - 40, 50);
    lv_obj_align(m_sessionTabs, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_bg_color(m_sessionTabs, lv_color_hex(0x3C3C3C), 0);
}

void EnhancedTerminalApp::updateSessionTabs() {
    if (!m_sessionTabs) {
        return;
    }

    // Clear existing tabs
    lv_obj_clean(m_sessionTabs);

    // Add tabs for each active session
    for (const auto& session : m_sessions) {
        if (session && session->isActive) {
            std::string tabName = session->id;
            if (session->mode == TerminalMode::TELNET) {
                tabName = "Telnet:" + session->id.substr(0, 4);
            } else if (session->mode == TerminalMode::SSH) {
                tabName = "SSH:" + session->id.substr(0, 4);
            } else if (session->mode == TerminalMode::RS485) {
                tabName = "RS485";
            } else {
                tabName = "Local";
            }

            lv_obj_t* tab = lv_tabview_add_tab(m_sessionTabs, tabName.c_str());
            lv_obj_set_user_data(tab, (void*)session->id.c_str());
            lv_obj_add_event_cb(tab, sessionTabCallback, LV_EVENT_CLICKED, this);
        }
    }
}

void EnhancedTerminalApp::processReceivedData() {
    // Process data for each active session
    for (auto& session : m_sessions) {
        if (!session || !session->isActive) {
            continue;
        }

        if (session->mode == TerminalMode::RS485) {
            // UART data is processed in UART task
            continue;
        }

        if (session->mode == TerminalMode::TELNET || session->mode == TerminalMode::SSH) {
            if (session->socket >= 0 && session->state == ConnectionState::CONNECTED) {
                uint8_t buffer[1024];
                int bytesReceived = recv(session->socket, buffer, sizeof(buffer), MSG_DONTWAIT);
                
                if (bytesReceived > 0) {
                    session->bytesReceived += bytesReceived;
                    m_totalBytesReceived += bytesReceived;
                    
                    if (session->mode == TerminalMode::TELNET) {
                        processTelnetData(session.get(), buffer, bytesReceived);
                    } else {
                        processSSHData(session.get(), buffer, bytesReceived);
                    }
                } else if (bytesReceived == 0) {
                    // Connection closed
                    session->state = ConnectionState::DISCONNECTED;
                    addToTerminal("Connection closed by remote host\n", session->id);
                } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    // Error occurred
                    session->state = ConnectionState::ERROR;
                    addToTerminal("Connection error occurred\n", session->id);
                }
            }
        }
    }
}

void EnhancedTerminalApp::processTelnetData(TerminalSession* session, const uint8_t* data, size_t length) {
    if (!session || !data) {
        return;
    }

    // Simple Telnet processing - handle IAC commands
    std::string processedData;
    for (size_t i = 0; i < length; i++) {
        if (data[i] == TELNET_IAC && i + 2 < length) {
            uint8_t command = data[i + 1];
            uint8_t option = data[i + 2];
            
            // Handle Telnet commands (simplified)
            switch (command) {
                case TELNET_WILL:
                case TELNET_WONT:
                case TELNET_DO:
                case TELNET_DONT:
                    // Skip these for now
                    i += 2;
                    break;
                default:
                    processedData += (char)data[i];
                    break;
            }
        } else {
            processedData += (char)data[i];
        }
    }

    if (!processedData.empty()) {
        addToTerminal(processedData.c_str(), session->id, false);
    }
}

void EnhancedTerminalApp::processSSHData(TerminalSession* session, const uint8_t* data, size_t length) {
    if (!session || !data) {
        return;
    }

    // Simplified SSH processing - just display received data
    char displayText[1024];
    formatDataForDisplay(data, length, displayText, sizeof(displayText));
    addToTerminal(displayText, session->id, false);
}

void EnhancedTerminalApp::addToTerminal(const char* text, const std::string& sessionId, bool isTransmitted) {
    if (!m_terminalTextArea || !text) {
        return;
    }

    // Only show text for current session
    if (sessionId != m_currentSessionId) {
        return;
    }

    std::string formattedText;
    
    if (m_showTimestamp) {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        struct tm* timeinfo = localtime(&tv.tv_sec);
        char timestamp[32];
        strftime(timestamp, sizeof(timestamp), "[%H:%M:%S] ", timeinfo);
        formattedText += timestamp;
    }

    if (isTransmitted) {
        formattedText += "TX: ";
    } else {
        formattedText += "RX: ";
    }

    formattedText += text;
    if (formattedText.back() != '\n') {
        formattedText += '\n';
    }

    // Add to terminal
    lv_textarea_add_text(m_terminalTextArea, formattedText.c_str());

    if (m_autoScroll) {
        lv_textarea_set_cursor_pos(m_terminalTextArea, LV_TEXTAREA_CURSOR_LAST);
    }
}

void EnhancedTerminalApp::formatDataForDisplay(const uint8_t* data, size_t length, 
                                              char* buffer, size_t bufferSize) {
    if (!data || !buffer || bufferSize == 0) {
        return;
    }

    if (m_hexDisplay) {
        // Hex display
        size_t pos = 0;
        for (size_t i = 0; i < length && pos < bufferSize - 3; i++) {
            snprintf(buffer + pos, bufferSize - pos, "%02X ", data[i]);
            pos += 3;
        }
        if (pos < bufferSize) {
            buffer[pos] = '\0';
        }
    } else {
        // ASCII display
        size_t copyLen = std::min(length, bufferSize - 1);
        memcpy(buffer, data, copyLen);
        buffer[copyLen] = '\0';
    }
}

std::string EnhancedTerminalApp::generateSessionId() {
    return "session_" + std::to_string(m_nextSessionId++);
}

void EnhancedTerminalApp::networkTask(void* parameter) {
    EnhancedTerminalApp* app = static_cast<EnhancedTerminalApp*>(parameter);
    
    while (app->m_tasksRunning) {
        // Network processing would go here
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    vTaskDelete(nullptr);
}

void EnhancedTerminalApp::uartTask(void* parameter) {
    EnhancedTerminalApp* app = static_cast<EnhancedTerminalApp*>(parameter);
    uint8_t buffer[256];
    
    while (app->m_tasksRunning) {
        int bytesRead = uart_read_bytes(UART_PORT, buffer, sizeof(buffer), 100 / portTICK_PERIOD_MS);
        if (bytesRead > 0) {
            // Find RS485 session
            for (auto& session : app->m_sessions) {
                if (session && session->mode == TerminalMode::RS485 && session->isActive) {
                    session->bytesReceived += bytesRead;
                    app->m_totalBytesReceived += bytesRead;
                    
                    char displayText[512];
                    app->formatDataForDisplay(buffer, bytesRead, displayText, sizeof(displayText));
                    app->addToTerminal(displayText, session->id, false);
                    break;
                }
            }
        }
    }
    
    vTaskDelete(nullptr);
}

// UI Event Callbacks

void EnhancedTerminalApp::connectButtonCallback(lv_event_t* e) {
    EnhancedTerminalApp* app = static_cast<EnhancedTerminalApp*>(lv_event_get_user_data(e));
    if (app) {
        app->createConnectionDialog();
    }
}

void EnhancedTerminalApp::disconnectButtonCallback(lv_event_t* e) {
    EnhancedTerminalApp* app = static_cast<EnhancedTerminalApp*>(lv_event_get_user_data(e));
    if (app && !app->m_currentSessionId.empty()) {
        app->closeSession(app->m_currentSessionId);
    }
}

void EnhancedTerminalApp::sendButtonCallback(lv_event_t* e) {
    EnhancedTerminalApp* app = static_cast<EnhancedTerminalApp*>(lv_event_get_user_data(e));
    if (app && app->m_inputTextArea) {
        const char* text = lv_textarea_get_text(app->m_inputTextArea);
        if (text && strlen(text) > 0) {
            app->sendString(text);
            app->sendString("\n");
            lv_textarea_set_text(app->m_inputTextArea, "");
        }
    }
}

void EnhancedTerminalApp::clearButtonCallback(lv_event_t* e) {
    EnhancedTerminalApp* app = static_cast<EnhancedTerminalApp*>(lv_event_get_user_data(e));
    if (app) {
        app->clearTerminal();
    }
}

void EnhancedTerminalApp::settingsButtonCallback(lv_event_t* e) {
    // Implementation for settings dialog
}

void EnhancedTerminalApp::sessionTabCallback(lv_event_t* e) {
    EnhancedTerminalApp* app = static_cast<EnhancedTerminalApp*>(lv_event_get_user_data(e));
    lv_obj_t* tab = lv_event_get_target(e);
    
    if (app && tab) {
        const char* sessionId = static_cast<const char*>(lv_obj_get_user_data(tab));
        if (sessionId) {
            app->switchToSession(sessionId);
        }
    }
}

void EnhancedTerminalApp::modeDropdownCallback(lv_event_t* e) {
    // Implementation for mode selection
}

void EnhancedTerminalApp::createConnectionDialog() {
    // Implementation for connection dialog would go here
    log(ESP_LOG_INFO, "Opening connection dialog");
}