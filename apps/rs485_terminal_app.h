#ifndef RS485_TERMINAL_APP_H
#define RS485_TERMINAL_APP_H

#include "base_app.h"
#include "../hal/hardware_config.h"
#include <driver/uart.h>

/**
 * @file rs485_terminal_app.h
 * @brief RS-485 Terminal Application for M5Stack Tab5
 * 
 * Provides RS-485 communication terminal with data monitoring,
 * command sending, and protocol analysis capabilities.
 */

enum class RS485BaudRate {
    BAUD_9600 = 9600,
    BAUD_19200 = 19200,
    BAUD_38400 = 38400,
    BAUD_57600 = 57600,
    BAUD_115200 = 115200
};

enum class RS485DataBits {
    DATA_5_BITS = UART_DATA_5_BITS,
    DATA_6_BITS = UART_DATA_6_BITS,
    DATA_7_BITS = UART_DATA_7_BITS,
    DATA_8_BITS = UART_DATA_8_BITS
};

enum class RS485Parity {
    PARITY_NONE = UART_PARITY_DISABLE,
    PARITY_EVEN = UART_PARITY_EVEN,
    PARITY_ODD = UART_PARITY_ODD
};

enum class RS485StopBits {
    STOP_1_BIT = UART_STOP_BITS_1,
    STOP_1_5_BITS = UART_STOP_BITS_1_5,
    STOP_2_BITS = UART_STOP_BITS_2
};

struct RS485Config {
    RS485BaudRate baudRate = RS485BaudRate::BAUD_9600;
    RS485DataBits dataBits = RS485DataBits::DATA_8_BITS;
    RS485Parity parity = RS485Parity::PARITY_NONE;
    RS485StopBits stopBits = RS485StopBits::STOP_1_BIT;
    bool flowControl = false;
    uint32_t timeout = 1000; // ms
};

class RS485TerminalApp : public BaseApp {
public:
    RS485TerminalApp();
    ~RS485TerminalApp() override;

    // BaseApp interface
    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;
    os_error_t handleEvent(uint32_t eventType, void* eventData, size_t dataSize) override;

    /**
     * @brief Initialize RS-485 hardware
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeRS485();

    /**
     * @brief Configure RS-485 communication parameters
     * @param config RS-485 configuration
     * @return OS_OK on success, error code on failure
     */
    os_error_t configureRS485(const RS485Config& config);

    /**
     * @brief Send data via RS-485
     * @param data Data to send
     * @param length Length of data
     * @return OS_OK on success, error code on failure
     */
    os_error_t sendData(const uint8_t* data, size_t length);

    /**
     * @brief Send string via RS-485
     * @param text String to send
     * @return OS_OK on success, error code on failure
     */
    os_error_t sendString(const char* text);

    /**
     * @brief Clear terminal display
     */
    void clearTerminal();

    /**
     * @brief Toggle RS-485 transceiver direction
     * @param transmit true for transmit mode, false for receive mode
     */
    void setTransmitMode(bool transmit);

    /**
     * @brief Get RS-485 statistics
     */
    void printRS485Stats() const;

private:
    /**
     * @brief Create terminal UI
     */
    void createTerminalUI();

    /**
     * @brief Create control panel UI
     */
    void createControlPanel();

    /**
     * @brief Process received data
     */
    void processReceivedData();

    /**
     * @brief Add text to terminal display
     * @param text Text to add
     * @param isTransmitted true if transmitted data, false if received
     */
    void addToTerminal(const char* text, bool isTransmitted = false);

    /**
     * @brief Format data for display (hex/ASCII)
     * @param data Data buffer
     * @param length Data length
     * @param buffer Output buffer
     * @param bufferSize Output buffer size
     */
    void formatDataForDisplay(const uint8_t* data, size_t length, 
                             char* buffer, size_t bufferSize);

    // UI event callbacks
    static void sendButtonCallback(lv_event_t* e);
    static void clearButtonCallback(lv_event_t* e);
    static void configButtonCallback(lv_event_t* e);
    static void modeButtonCallback(lv_event_t* e);

    // UART task for background processing
    static void uartTask(void* parameter);

    // RS-485 configuration
    RS485Config m_config;
    bool m_rs485Initialized = false;
    bool m_transmitMode = false;

    // UI elements
    lv_obj_t* m_terminalContainer = nullptr;
    lv_obj_t* m_terminalTextArea = nullptr;
    lv_obj_t* m_inputTextArea = nullptr;
    lv_obj_t* m_controlPanel = nullptr;
    lv_obj_t* m_sendButton = nullptr;
    lv_obj_t* m_clearButton = nullptr;
    lv_obj_t* m_configButton = nullptr;
    lv_obj_t* m_modeButton = nullptr;
    lv_obj_t* m_statusLabel = nullptr;
    lv_obj_t* m_configLabel = nullptr;

    // Communication buffers
    uint8_t* m_rxBuffer = nullptr;
    uint8_t* m_txBuffer = nullptr;
    size_t m_rxBufferSize = 0;
    size_t m_txBufferSize = 0;

    // Statistics
    uint32_t m_bytesReceived = 0;
    uint32_t m_bytesTransmitted = 0;
    uint32_t m_packetsReceived = 0;
    uint32_t m_packetsTransmitted = 0;
    uint32_t m_errorCount = 0;

    // Task management
    TaskHandle_t m_uartTaskHandle = nullptr;
    bool m_taskRunning = false;

    // Display settings
    bool m_hexDisplay = false;
    bool m_showTimestamp = true;
    bool m_autoScroll = true;

    // Configuration
    static constexpr uart_port_t UART_PORT = UART_NUM_2;
    static constexpr size_t RX_BUFFER_SIZE = 2048;
    static constexpr size_t TX_BUFFER_SIZE = 1024;
    static constexpr size_t TERMINAL_BUFFER_SIZE = 8192;
    static constexpr uint32_t UART_TASK_STACK_SIZE = 4096;
    static constexpr UBaseType_t UART_TASK_PRIORITY = 5;
};

#endif // RS485_TERMINAL_APP_H