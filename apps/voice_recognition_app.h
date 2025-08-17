#ifndef VOICE_RECOGNITION_APP_H
#define VOICE_RECOGNITION_APP_H

#include "base_app.h"
#include <vector>
#include <string>
#include <map>

enum class VoiceState {
    IDLE,
    LISTENING,
    PROCESSING,
    RESPONDING,
    ERROR
};

enum class Language {
    ENGLISH,
    SPANISH,
    FRENCH,
    GERMAN,
    CHINESE,
    JAPANESE
};

struct VoiceCommand {
    std::string command;
    std::string action;
    std::vector<std::string> parameters;
    time_t timestamp;
};

struct ChatMessage {
    std::string content;
    bool isUser;
    time_t timestamp;
};

class VoiceRecognitionApp : public BaseApp {
public:
    VoiceRecognitionApp();
    ~VoiceRecognitionApp() override;

    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;

private:
    void createMainUI();
    void createVoiceControls();
    void createChatInterface();
    void createSettingsPanel();
    void createLanguageSelector();
    void createVoiceVisualizer();
    
    // Voice recognition functions
    void startListening();
    void stopListening();
    void processVoiceInput();
    void handleVoiceCommand(const std::string& command);
    void parseCommand(const std::string& input, VoiceCommand& cmd);
    
    // ChatGPT integration
    void sendToChatGPT(const std::string& message);
    void handleChatGPTResponse(const std::string& response);
    void updateChatHistory(const std::string& message, bool isUser);
    
    // Text-to-speech functions
    void speakText(const std::string& text);
    void stopSpeaking();
    
    // Command processing
    void executeSystemCommand(const VoiceCommand& cmd);
    void executeAppCommand(const VoiceCommand& cmd);
    void executeDeviceCommand(const VoiceCommand& cmd);
    
    // Utility functions
    std::string languageToString(Language lang);
    std::string getLanguageCode(Language lang);
    void updateVoiceVisualizer(float amplitude);
    void showProcessingAnimation();
    void hideProcessingAnimation();
    
    // Settings functions
    void setLanguage(Language lang);
    void setMicrophoneSensitivity(uint8_t sensitivity);
    void setSpeakerVolume(uint8_t volume);
    void toggleWakeWord(bool enabled);
    void setChatGPTAPIKey(const std::string& apiKey);
    
    // Network functions
    bool isNetworkAvailable();
    void connectToNetwork();
    
    os_error_t loadSettings();
    os_error_t saveSettings();
    os_error_t loadChatHistory();
    os_error_t saveChatHistory();
    
    // Event callbacks
    static void listenButtonCallback(lv_event_t* e);
    static void stopButtonCallback(lv_event_t* e);
    static void settingsButtonCallback(lv_event_t* e);
    static void languageCallback(lv_event_t* e);
    static void sensitivityCallback(lv_event_t* e);
    static void volumeCallback(lv_event_t* e);
    static void wakeWordCallback(lv_event_t* e);
    static void apiKeyCallback(lv_event_t* e);
    static void clearHistoryCallback(lv_event_t* e);
    static void textInputCallback(lv_event_t* e);
    static void sendTextCallback(lv_event_t* e);
    
    // Data
    std::vector<VoiceCommand> m_commandHistory;
    std::vector<ChatMessage> m_chatHistory;
    std::map<std::string, std::string> m_commandMappings;
    
    // State
    VoiceState m_voiceState;
    Language m_currentLanguage;
    uint8_t m_microphoneSensitivity;
    uint8_t m_speakerVolume;
    bool m_wakeWordEnabled;
    bool m_continuousListening;
    bool m_networkConnected;
    std::string m_chatGPTAPIKey;
    std::string m_currentInput;
    std::string m_lastResponse;
    
    // Audio processing
    float m_currentAmplitude;
    uint32_t m_listeningTimeout;
    uint32_t m_processingStartTime;
    
    // UI elements
    lv_obj_t* m_voiceControls = nullptr;
    lv_obj_t* m_chatInterface = nullptr;
    lv_obj_t* m_settingsPanel = nullptr;
    lv_obj_t* m_voiceVisualizer = nullptr;
    
    // Voice control UI
    lv_obj_t* m_listenButton = nullptr;
    lv_obj_t* m_stopButton = nullptr;
    lv_obj_t* m_statusLabel = nullptr;
    lv_obj_t* m_commandLabel = nullptr;
    lv_obj_t* m_processingSpinner = nullptr;
    
    // Chat interface UI
    lv_obj_t* m_chatList = nullptr;
    lv_obj_t* m_textInput = nullptr;
    lv_obj_t* m_sendButton = nullptr;
    lv_obj_t* m_clearHistoryButton = nullptr;
    
    // Settings UI
    lv_obj_t* m_languageDropdown = nullptr;
    lv_obj_t* m_sensitivitySlider = nullptr;
    lv_obj_t* m_volumeSlider = nullptr;
    lv_obj_t* m_wakeWordSwitch = nullptr;
    lv_obj_t* m_apiKeyInput = nullptr;
    lv_obj_t* m_networkStatusLabel = nullptr;
    
    // Voice visualizer UI
    lv_obj_t* m_visualizerBars[8] = {nullptr};
    lv_obj_t* m_visualizerCanvas = nullptr;
};

#endif // VOICE_RECOGNITION_APP_H