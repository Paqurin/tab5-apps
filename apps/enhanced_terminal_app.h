#ifndef ENHANCED_TERMINAL_APP_H
#define ENHANCED_TERMINAL_APP_H

#include "base_app.h"
#include "../hal/hardware_config.h"
#include <driver/uart.h>
#include <lwip/sockets.h>
#include <vector>
#include <string>
#include <memory>

/**
 * @file enhanced_terminal_app.h
 * @brief Enhanced Terminal Application with Telnet and SSH support
 * 
 * Provides RS-485, Telnet, and SSH connectivity with advanced terminal features.
 */

enum class TerminalMode {
    RS485,
    TELNET,
    SSH,
    LOCAL
};

enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    AUTHENTICATING,
    ERROR
};

struct ConnectionConfig {
    std::string hostname;
    uint16_t port;
    std::string username;
    std::string password;
    std::string privateKey;
    uint32_t timeout;
    bool useSSL;
};

struct TerminalSession {
    std::string id;
    TerminalMode mode;
    ConnectionState state;
    ConnectionConfig config;
    int socket;
    uint32_t connectTime;
    uint32_t bytesReceived;
    uint32_t bytesTransmitted;
    std::string buffer;
    bool isActive;
};

class EnhancedTerminalApp : public BaseApp {
public:
    EnhancedTerminalApp();
    ~EnhancedTerminalApp() override;

    // BaseApp interface
    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;
    os_error_t handleEvent(uint32_t eventType, void* eventData, size_t dataSize) override;

    /**
     * @brief Create new terminal session
     * @param mode Terminal mode (RS485, Telnet, SSH, Local)
     * @param config Connection configuration
     * @return Session ID or empty string on failure
     */
    std::string createSession(TerminalMode mode, const ConnectionConfig& config = {});

    /**
     * @brief Close terminal session
     * @param sessionId Session ID to close
     * @return OS_OK on success, error code on failure
     */
    os_error_t closeSession(const std::string& sessionId);

    /**
     * @brief Switch to terminal session
     * @param sessionId Session ID to switch to
     * @return OS_OK on success, error code on failure
     */
    os_error_t switchToSession(const std::string& sessionId);

    /**
     * @brief Send data to current session
     * @param data Data to send
     * @param length Length of data
     * @return OS_OK on success, error code on failure
     */
    os_error_t sendData(const uint8_t* data, size_t length);

    /**
     * @brief Send string to current session
     * @param text String to send
     * @return OS_OK on success, error code on failure
     */
    os_error_t sendString(const char* text);

    /**
     * @brief Connect to Telnet server
     * @param hostname Server hostname/IP
     * @param port Server port (default 23)
     * @param timeout Connection timeout in ms
     * @return Session ID or empty string on failure
     */
    std::string connectTelnet(const std::string& hostname, uint16_t port = 23, uint32_t timeout = 10000);

    /**
     * @brief Connect to SSH server
     * @param hostname Server hostname/IP
     * @param port Server port (default 22)
     * @param username Username for authentication
     * @param password Password for authentication
     * @param timeout Connection timeout in ms
     * @return Session ID or empty string on failure
     */
    std::string connectSSH(const std::string& hostname, uint16_t port = 22, 
                          const std::string& username = "", const std::string& password = "",
                          uint32_t timeout = 10000);

    /**
     * @brief Clear terminal display
     */
    void clearTerminal();

    /**
     * @brief Get list of active sessions
     * @return Vector of session IDs
     */
    std::vector<std::string> getActiveSessions() const;

    /**
     * @brief Get session information
     * @param sessionId Session ID
     * @return Session info or nullptr if not found
     */
    const TerminalSession* getSession(const std::string& sessionId) const;

    /**
     * @brief Set terminal display options
     * @param hexDisplay Show hex display
     * @param showTimestamp Show timestamps
     * @param autoScroll Auto-scroll terminal
     */
    void setDisplayOptions(bool hexDisplay, bool showTimestamp, bool autoScroll);

private:
    /**
     * @brief Initialize RS-485 hardware
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeRS485();

    /**
     * @brief Initialize network stack
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeNetwork();

    /**
     * @brief Create terminal UI
     */
    void createTerminalUI();

    /**
     * @brief Create connection dialog
     */
    void createConnectionDialog();

    /**
     * @brief Create session tabs
     */
    void createSessionTabs();

    /**
     * @brief Update session tabs
     */
    void updateSessionTabs();

    /**
     * @brief Process received data for all sessions
     */
    void processReceivedData();

    /**
     * @brief Process Telnet protocol
     * @param session Session to process
     * @param data Received data
     * @param length Data length
     */
    void processTelnetData(TerminalSession* session, const uint8_t* data, size_t length);

    /**
     * @brief Process SSH protocol
     * @param session Session to process
     * @param data Received data
     * @param length Data length
     */
    void processSSHData(TerminalSession* session, const uint8_t* data, size_t length);

    /**
     * @brief Add text to terminal display
     * @param text Text to add
     * @param sessionId Session ID
     * @param isTransmitted true if transmitted data, false if received
     */
    void addToTerminal(const char* text, const std::string& sessionId, bool isTransmitted = false);

    /**
     * @brief Format data for display (hex/ASCII)
     * @param data Data buffer
     * @param length Data length
     * @param buffer Output buffer
     * @param bufferSize Output buffer size
     */
    void formatDataForDisplay(const uint8_t* data, size_t length, 
                             char* buffer, size_t bufferSize);

    /**
     * @brief Generate unique session ID
     * @return Unique session ID
     */
    std::string generateSessionId();

    /**
     * @brief Network task for handling connections
     * @param parameter Task parameter
     */
    static void networkTask(void* parameter);

    /**
     * @brief UART task for RS-485 processing
     * @param parameter Task parameter
     */
    static void uartTask(void* parameter);

    // UI event callbacks
    static void connectButtonCallback(lv_event_t* e);
    static void disconnectButtonCallback(lv_event_t* e);
    static void sendButtonCallback(lv_event_t* e);
    static void clearButtonCallback(lv_event_t* e);
    static void settingsButtonCallback(lv_event_t* e);
    static void sessionTabCallback(lv_event_t* e);
    static void modeDropdownCallback(lv_event_t* e);

    // Terminal sessions
    std::vector<std::unique_ptr<TerminalSession>> m_sessions;
    std::string m_currentSessionId;
    uint32_t m_nextSessionId = 1;

    // Network configuration
    bool m_networkInitialized = false;
    bool m_wifiConnected = false;

    // UI elements
    lv_obj_t* m_terminalContainer = nullptr;
    lv_obj_t* m_sessionTabs = nullptr;
    lv_obj_t* m_terminalTextArea = nullptr;
    lv_obj_t* m_inputTextArea = nullptr;
    lv_obj_t* m_controlPanel = nullptr;
    lv_obj_t* m_connectButton = nullptr;
    lv_obj_t* m_disconnectButton = nullptr;
    lv_obj_t* m_sendButton = nullptr;
    lv_obj_t* m_clearButton = nullptr;
    lv_obj_t* m_settingsButton = nullptr;
    lv_obj_t* m_statusLabel = nullptr;
    lv_obj_t* m_modeDropdown = nullptr;

    // Connection dialog
    lv_obj_t* m_connectionDialog = nullptr;
    lv_obj_t* m_hostnameInput = nullptr;
    lv_obj_t* m_portInput = nullptr;
    lv_obj_t* m_usernameInput = nullptr;
    lv_obj_t* m_passwordInput = nullptr;
    lv_obj_t* m_connectDialogButton = nullptr;
    lv_obj_t* m_cancelDialogButton = nullptr;

    // Communication buffers
    uint8_t* m_rxBuffer = nullptr;
    uint8_t* m_txBuffer = nullptr;
    size_t m_rxBufferSize = 0;
    size_t m_txBufferSize = 0;

    // Task management
    TaskHandle_t m_networkTaskHandle = nullptr;
    TaskHandle_t m_uartTaskHandle = nullptr;
    bool m_tasksRunning = false;

    // Display settings
    bool m_hexDisplay = false;
    bool m_showTimestamp = true;
    bool m_autoScroll = true;
    bool m_localEcho = false;

    // RS-485 configuration (inherited from original)
    struct {
        uint32_t baudRate = 9600;
        uint8_t dataBits = 8;
        uint8_t parity = 0;
        uint8_t stopBits = 1;
        bool flowControl = false;
    } m_rs485Config;

    // Statistics
    uint32_t m_totalConnections = 0;
    uint32_t m_totalDisconnections = 0;
    uint32_t m_totalBytesReceived = 0;
    uint32_t m_totalBytesTransmitted = 0;

    // Configuration constants
    static constexpr uart_port_t UART_PORT = UART_NUM_2;
    static constexpr size_t RX_BUFFER_SIZE = 4096;
    static constexpr size_t TX_BUFFER_SIZE = 2048;
    static constexpr size_t TERMINAL_BUFFER_SIZE = 16384;
    static constexpr uint32_t NETWORK_TASK_STACK_SIZE = 8192;
    static constexpr uint32_t UART_TASK_STACK_SIZE = 4096;
    static constexpr UBaseType_t NETWORK_TASK_PRIORITY = 6;
    static constexpr UBaseType_t UART_TASK_PRIORITY = 5;
    static constexpr size_t MAX_SESSIONS = 8;
    static constexpr uint32_t CONNECTION_TIMEOUT = 30000; // 30 seconds
    static constexpr uint32_t KEEPALIVE_INTERVAL = 60000; // 1 minute
};

#endif // ENHANCED_TERMINAL_APP_H