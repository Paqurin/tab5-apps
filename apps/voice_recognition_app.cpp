#include "voice_recognition_app.h"
#include "../system/os_manager.h"
#include <sstream>
#include <algorithm>
#include <cmath>

VoiceRecognitionApp::VoiceRecognitionApp() 
    : BaseApp("com.m5stack.voice", "Voice Assistant", "1.0.0"),
      m_voiceState(VoiceState::IDLE), m_currentLanguage(Language::ENGLISH),
      m_microphoneSensitivity(70), m_speakerVolume(80), m_wakeWordEnabled(false),
      m_continuousListening(false), m_networkConnected(false),
      m_currentAmplitude(0.0f), m_listeningTimeout(0), m_processingStartTime(0) {
    setDescription("Voice recognition and ChatGPT integration for hands-free operation");
    setAuthor("M5Stack");
    setPriority(AppPriority::APP_NORMAL);
}

VoiceRecognitionApp::~VoiceRecognitionApp() {
    shutdown();
}

os_error_t VoiceRecognitionApp::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Initializing Voice Recognition App");
    
    // Load settings and chat history
    loadSettings();
    loadChatHistory();
    
    // Initialize command mappings
    m_commandMappings["open calculator"] = "app:calculator";
    m_commandMappings["show tasks"] = "app:tasks";
    m_commandMappings["open contacts"] = "app:contacts";
    m_commandMappings["set alarm"] = "device:alarm";
    m_commandMappings["what time is it"] = "device:time";
    m_commandMappings["check battery"] = "device:battery";
    m_commandMappings["turn on wifi"] = "device:wifi_on";
    m_commandMappings["turn off wifi"] = "device:wifi_off";
    
    // Check network connectivity
    m_networkConnected = isNetworkAvailable();
    
    // Set memory usage estimate
    setMemoryUsage(65536); // 64KB estimated usage
    
    m_initialized = true;
    return OS_OK;
}

os_error_t VoiceRecognitionApp::update(uint32_t deltaTime) {
    // Update voice state and processing
    if (m_voiceState == VoiceState::LISTENING) {
        m_listeningTimeout += deltaTime;
        
        // Auto-stop listening after 10 seconds
        if (m_listeningTimeout > 10000) {
            stopListening();
        }
        
        // Update voice visualizer with simulated amplitude
        m_currentAmplitude = 0.3f + 0.7f * sin(m_listeningTimeout * 0.01f);
        updateVoiceVisualizer(m_currentAmplitude);
    }
    
    if (m_voiceState == VoiceState::PROCESSING) {
        // Simulate processing time
        if (m_processingStartTime > 0 && 
            (esp_timer_get_time() / 1000 - m_processingStartTime) > 2000) {
            // Simulate completion of processing
            m_voiceState = VoiceState::RESPONDING;
            hideProcessingAnimation();
            
            // For demo purposes, provide a canned response
            if (!m_currentInput.empty()) {
                handleChatGPTResponse("I understand you said: \"" + m_currentInput + "\". How can I help you with that?");
            }
        }
    }
    
    return OS_OK;
}

os_error_t VoiceRecognitionApp::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Shutting down Voice Recognition App");
    
    // Stop any ongoing voice operations
    if (m_voiceState == VoiceState::LISTENING) {
        stopListening();
    }
    if (m_voiceState == VoiceState::RESPONDING) {
        stopSpeaking();
    }
    
    // Save settings and chat history
    saveSettings();
    saveChatHistory();
    
    m_initialized = false;
    return OS_OK;
}

os_error_t VoiceRecognitionApp::createUI(lv_obj_t* parent) {
    if (!parent) {
        return OS_ERROR_INVALID_PARAM;
    }

    // Create main container
    m_uiContainer = lv_obj_create(parent);
    lv_obj_set_size(m_uiContainer, LV_HOR_RES, 
                    LV_VER_RES - 60 - 40); // Account for status bar and dock
    lv_obj_align(m_uiContainer, LV_ALIGN_CENTER, 0, 0);
    
    // Apply theme
    lv_obj_set_style_bg_color(m_uiContainer, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_opa(m_uiContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(m_uiContainer, 10, 0);

    createMainUI();
    return OS_OK;
}

os_error_t VoiceRecognitionApp::destroyUI() {
    if (m_uiContainer) {
        lv_obj_del(m_uiContainer);
        m_uiContainer = nullptr;
    }
    return OS_OK;
}

void VoiceRecognitionApp::createMainUI() {
    // Create tab view for different sections
    lv_obj_t* tabView = lv_tabview_create(m_uiContainer, LV_DIR_TOP, 50);
    lv_obj_set_size(tabView, LV_PCT(100), LV_PCT(100));
    
    // Voice controls tab
    lv_obj_t* voiceTab = lv_tabview_add_tab(tabView, "Voice");
    m_voiceControls = voiceTab;
    createVoiceControls();
    
    // Chat interface tab
    lv_obj_t* chatTab = lv_tabview_add_tab(tabView, "Chat");
    m_chatInterface = chatTab;
    createChatInterface();
    
    // Settings tab
    lv_obj_t* settingsTab = lv_tabview_add_tab(tabView, "Settings");
    m_settingsPanel = settingsTab;
    createSettingsPanel();
}

void VoiceRecognitionApp::createVoiceControls() {
    // Voice visualizer
    createVoiceVisualizer();
    
    // Status display
    m_statusLabel = lv_label_create(m_voiceControls);
    lv_label_set_text(m_statusLabel, "Ready to listen");
    lv_obj_align(m_statusLabel, LV_ALIGN_TOP_MID, 0, 150);
    lv_obj_set_style_text_font(m_statusLabel, &lv_font_montserrat_16, 0);
    
    // Command display
    m_commandLabel = lv_label_create(m_voiceControls);
    lv_label_set_text(m_commandLabel, "");
    lv_obj_align_to(m_commandLabel, m_statusLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_label_set_long_mode(m_commandLabel, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(m_commandLabel, LV_PCT(90));
    lv_obj_set_style_text_color(m_commandLabel, lv_color_hex(0x3498DB), 0);
    
    // Control buttons
    lv_obj_t* buttonContainer = lv_obj_create(m_voiceControls);
    lv_obj_set_size(buttonContainer, LV_PCT(90), 80);
    lv_obj_align(buttonContainer, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_opa(buttonContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(buttonContainer, LV_OPA_TRANSP, 0);
    
    // Listen button
    m_listenButton = lv_btn_create(buttonContainer);
    lv_obj_set_size(m_listenButton, 120, 60);
    lv_obj_align(m_listenButton, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_event_cb(m_listenButton, listenButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(m_listenButton, lv_color_hex(0x27AE60), 0);
    
    lv_obj_t* listenLabel = lv_label_create(m_listenButton);
    lv_label_set_text(listenLabel, LV_SYMBOL_AUDIO " Listen");
    lv_obj_center(listenLabel);
    
    // Stop button
    m_stopButton = lv_btn_create(buttonContainer);
    lv_obj_set_size(m_stopButton, 120, 60);
    lv_obj_align(m_stopButton, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(m_stopButton, stopButtonCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(m_stopButton, lv_color_hex(0xE74C3C), 0);
    lv_obj_add_state(m_stopButton, LV_STATE_DISABLED);
    
    lv_obj_t* stopLabel = lv_label_create(m_stopButton);
    lv_label_set_text(stopLabel, LV_SYMBOL_STOP " Stop");
    lv_obj_center(stopLabel);
    
    // Processing spinner (initially hidden)
    m_processingSpinner = lv_spinner_create(m_voiceControls, 1000, 60);
    lv_obj_set_size(m_processingSpinner, 50, 50);
    lv_obj_align_to(m_processingSpinner, m_commandLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_add_flag(m_processingSpinner, LV_OBJ_FLAG_HIDDEN);
}

void VoiceRecognitionApp::createVoiceVisualizer() {
    m_voiceVisualizer = lv_obj_create(m_voiceControls);
    lv_obj_set_size(m_voiceVisualizer, 300, 100);
    lv_obj_align(m_voiceVisualizer, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_bg_color(m_voiceVisualizer, lv_color_hex(0x2C2C2C), 0);
    lv_obj_set_style_border_color(m_voiceVisualizer, lv_color_hex(0x3498DB), 0);
    lv_obj_set_style_border_width(m_voiceVisualizer, 2, 0);
    
    // Create visualizer bars
    for (int i = 0; i < 8; i++) {
        m_visualizerBars[i] = lv_bar_create(m_voiceVisualizer);
        lv_obj_set_size(m_visualizerBars[i], 20, 80);
        lv_obj_align(m_visualizerBars[i], LV_ALIGN_BOTTOM_LEFT, 20 + i * 30, -10);
        lv_bar_set_range(m_visualizerBars[i], 0, 100);
        lv_bar_set_value(m_visualizerBars[i], 0, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(m_visualizerBars[i], lv_color_hex(0x3498DB), LV_PART_INDICATOR);
    }
}

void VoiceRecognitionApp::createChatInterface() {
    // Chat history list
    m_chatList = lv_list_create(m_chatInterface);
    lv_obj_set_size(m_chatList, LV_PCT(100), LV_PCT(80));
    lv_obj_align(m_chatInterface, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(m_chatList, lv_color_hex(0x2C2C2C), 0);
    
    // Input area
    lv_obj_t* inputContainer = lv_obj_create(m_chatInterface);
    lv_obj_set_size(inputContainer, LV_PCT(100), LV_PCT(20));
    lv_obj_align(inputContainer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(inputContainer, lv_color_hex(0x34495E), 0);
    
    // Text input
    m_textInput = lv_textarea_create(inputContainer);
    lv_obj_set_size(m_textInput, LV_PCT(70), 40);
    lv_obj_align(m_textInput, LV_ALIGN_LEFT_MID, 10, 0);
    lv_textarea_set_placeholder_text(m_textInput, "Type your message...");
    lv_obj_add_event_cb(m_textInput, textInputCallback, LV_EVENT_READY, this);
    
    // Send button
    m_sendButton = lv_btn_create(inputContainer);
    lv_obj_set_size(m_sendButton, LV_PCT(25), 40);
    lv_obj_align(m_sendButton, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_add_event_cb(m_sendButton, sendTextCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(m_sendButton, lv_color_hex(0x3498DB), 0);
    
    lv_obj_t* sendLabel = lv_label_create(m_sendButton);
    lv_label_set_text(sendLabel, "Send");
    lv_obj_center(sendLabel);
    
    // Clear history button
    m_clearHistoryButton = lv_btn_create(inputContainer);
    lv_obj_set_size(m_clearHistoryButton, 60, 25);
    lv_obj_align(m_clearHistoryButton, LV_ALIGN_TOP_RIGHT, -10, 5);
    lv_obj_add_event_cb(m_clearHistoryButton, clearHistoryCallback, LV_EVENT_CLICKED, this);
    lv_obj_set_style_bg_color(m_clearHistoryButton, lv_color_hex(0xE74C3C), 0);
    
    lv_obj_t* clearLabel = lv_label_create(m_clearHistoryButton);
    lv_label_set_text(clearLabel, "Clear");
    lv_obj_center(clearLabel);
    lv_obj_set_style_text_font(clearLabel, &lv_font_montserrat_12, 0);
    
    // Load existing chat history
    for (const auto& msg : m_chatHistory) {
        updateChatHistory(msg.content, msg.isUser);
    }
}

void VoiceRecognitionApp::createSettingsPanel() {
    lv_obj_set_layout(m_settingsPanel, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(m_settingsPanel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(m_settingsPanel, 20, 0);
    lv_obj_set_style_pad_gap(m_settingsPanel, 15, 0);
    
    // Language selection
    lv_obj_t* langContainer = lv_obj_create(m_settingsPanel);
    lv_obj_set_size(langContainer, LV_PCT(100), 50);
    lv_obj_set_style_bg_opa(langContainer, LV_OPA_TRANSP, 0);
    
    lv_obj_t* langLabel = lv_label_create(langContainer);
    lv_label_set_text(langLabel, "Language:");
    lv_obj_align(langLabel, LV_ALIGN_LEFT_MID, 0, 0);
    
    m_languageDropdown = lv_dropdown_create(langContainer);
    lv_obj_set_size(m_languageDropdown, 150, 35);
    lv_obj_align(langLabel, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_dropdown_set_options(m_languageDropdown, "English\nSpanish\nFrench\nGerman\nChinese\nJapanese");
    lv_dropdown_set_selected(m_languageDropdown, (uint16_t)m_currentLanguage);
    lv_obj_add_event_cb(m_languageDropdown, languageCallback, LV_EVENT_VALUE_CHANGED, this);
    
    // Microphone sensitivity
    lv_obj_t* micContainer = lv_obj_create(m_settingsPanel);
    lv_obj_set_size(micContainer, LV_PCT(100), 50);
    lv_obj_set_style_bg_opa(micContainer, LV_OPA_TRANSP, 0);
    
    lv_obj_t* micLabel = lv_label_create(micContainer);
    lv_label_set_text(micLabel, "Microphone Sensitivity:");
    lv_obj_align(micLabel, LV_ALIGN_LEFT_MID, 0, 0);
    
    m_sensitivitySlider = lv_slider_create(micContainer);
    lv_obj_set_size(m_sensitivitySlider, 150, 20);
    lv_obj_align(m_sensitivitySlider, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_slider_set_range(m_sensitivitySlider, 0, 100);
    lv_slider_set_value(m_sensitivitySlider, m_microphoneSensitivity, LV_ANIM_OFF);
    lv_obj_add_event_cb(m_sensitivitySlider, sensitivityCallback, LV_EVENT_VALUE_CHANGED, this);
    
    // Speaker volume
    lv_obj_t* volContainer = lv_obj_create(m_settingsPanel);
    lv_obj_set_size(volContainer, LV_PCT(100), 50);
    lv_obj_set_style_bg_opa(volContainer, LV_OPA_TRANSP, 0);
    
    lv_obj_t* volLabel = lv_label_create(volContainer);
    lv_label_set_text(volLabel, "Speaker Volume:");
    lv_obj_align(volLabel, LV_ALIGN_LEFT_MID, 0, 0);
    
    m_volumeSlider = lv_slider_create(volContainer);
    lv_obj_set_size(m_volumeSlider, 150, 20);
    lv_obj_align(m_volumeSlider, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_slider_set_range(m_volumeSlider, 0, 100);
    lv_slider_set_value(m_volumeSlider, m_speakerVolume, LV_ANIM_OFF);
    lv_obj_add_event_cb(m_volumeSlider, volumeCallback, LV_EVENT_VALUE_CHANGED, this);
    
    // Wake word toggle
    lv_obj_t* wakeContainer = lv_obj_create(m_settingsPanel);
    lv_obj_set_size(wakeContainer, LV_PCT(100), 50);
    lv_obj_set_style_bg_opa(wakeContainer, LV_OPA_TRANSP, 0);
    
    lv_obj_t* wakeLabel = lv_label_create(wakeContainer);
    lv_label_set_text(wakeLabel, "Wake Word Detection:");
    lv_obj_align(wakeLabel, LV_ALIGN_LEFT_MID, 0, 0);
    
    m_wakeWordSwitch = lv_switch_create(wakeContainer);
    lv_obj_align(m_wakeWordSwitch, LV_ALIGN_RIGHT_MID, -10, 0);
    if (m_wakeWordEnabled) {
        lv_obj_add_state(m_wakeWordSwitch, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(m_wakeWordSwitch, wakeWordCallback, LV_EVENT_VALUE_CHANGED, this);
    
    // ChatGPT API Key
    lv_obj_t* apiContainer = lv_obj_create(m_settingsPanel);
    lv_obj_set_size(apiContainer, LV_PCT(100), 80);
    lv_obj_set_style_bg_opa(apiContainer, LV_OPA_TRANSP, 0);
    
    lv_obj_t* apiLabel = lv_label_create(apiContainer);
    lv_label_set_text(apiLabel, "ChatGPT API Key:");
    lv_obj_align(apiLabel, LV_ALIGN_TOP_LEFT, 0, 0);
    
    m_apiKeyInput = lv_textarea_create(apiContainer);
    lv_obj_set_size(m_apiKeyInput, LV_PCT(100), 40);
    lv_obj_align_to(m_apiKeyInput, apiLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
    lv_textarea_set_placeholder_text(m_apiKeyInput, "Enter your OpenAI API key...");
    lv_textarea_set_password_mode(m_apiKeyInput, true);
    if (!m_chatGPTAPIKey.empty()) {
        lv_textarea_set_text(m_apiKeyInput, m_chatGPTAPIKey.c_str());
    }
    lv_obj_add_event_cb(m_apiKeyInput, apiKeyCallback, LV_EVENT_DEFOCUSED, this);
    
    // Network status
    m_networkStatusLabel = lv_label_create(m_settingsPanel);
    std::string networkText = "Network: " + std::string(m_networkConnected ? "Connected" : "Disconnected");
    lv_label_set_text(m_networkStatusLabel, networkText.c_str());
    lv_obj_set_style_text_color(m_networkStatusLabel, 
                               m_networkConnected ? lv_color_hex(0x27AE60) : lv_color_hex(0xE74C3C), 0);
}

void VoiceRecognitionApp::startListening() {
    if (m_voiceState != VoiceState::IDLE) {
        return;
    }
    
    m_voiceState = VoiceState::LISTENING;
    m_listeningTimeout = 0;
    
    lv_label_set_text(m_statusLabel, "Listening...");
    lv_obj_clear_state(m_listenButton, LV_STATE_DISABLED);
    lv_obj_clear_state(m_stopButton, LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(m_listenButton, lv_color_hex(0xE67E22), 0);
    
    log(ESP_LOG_INFO, "Started voice listening");
}

void VoiceRecognitionApp::stopListening() {
    if (m_voiceState != VoiceState::LISTENING) {
        return;
    }
    
    m_voiceState = VoiceState::PROCESSING;
    m_processingStartTime = esp_timer_get_time() / 1000;
    
    lv_label_set_text(m_statusLabel, "Processing...");
    lv_obj_add_state(m_listenButton, LV_STATE_DISABLED);
    lv_obj_add_state(m_stopButton, LV_STATE_DISABLED);
    lv_obj_set_style_bg_color(m_listenButton, lv_color_hex(0x27AE60), 0);
    
    // Reset visualizer
    for (int i = 0; i < 8; i++) {
        lv_bar_set_value(m_visualizerBars[i], 0, LV_ANIM_ON);
    }
    
    showProcessingAnimation();
    
    // For demo purposes, simulate voice input
    m_currentInput = "Hello, what can you help me with today?";
    lv_label_set_text(m_commandLabel, m_currentInput.c_str());
    
    log(ESP_LOG_INFO, "Stopped voice listening, processing input");
}

void VoiceRecognitionApp::updateVoiceVisualizer(float amplitude) {
    // Update visualizer bars with amplitude data
    for (int i = 0; i < 8; i++) {
        float barHeight = amplitude * (0.5f + 0.5f * sin((float)i * 0.8f + m_listeningTimeout * 0.01f));
        int value = (int)(barHeight * 100);
        lv_bar_set_value(m_visualizerBars[i], value, LV_ANIM_OFF);
    }
}

void VoiceRecognitionApp::showProcessingAnimation() {
    lv_obj_clear_flag(m_processingSpinner, LV_OBJ_FLAG_HIDDEN);
}

void VoiceRecognitionApp::hideProcessingAnimation() {
    lv_obj_add_flag(m_processingSpinner, LV_OBJ_FLAG_HIDDEN);
}

void VoiceRecognitionApp::handleChatGPTResponse(const std::string& response) {
    m_voiceState = VoiceState::IDLE;
    m_lastResponse = response;
    
    lv_label_set_text(m_statusLabel, "Ready to listen");
    lv_obj_clear_state(m_listenButton, LV_STATE_DISABLED);
    lv_obj_add_state(m_stopButton, LV_STATE_DISABLED);
    
    // Add to chat history
    updateChatHistory(m_currentInput, true);  // User message
    updateChatHistory(response, false);        // Assistant response
    
    // For demo, just display the response
    lv_label_set_text(m_commandLabel, response.c_str());
    
    log(ESP_LOG_INFO, "Received ChatGPT response: %s", response.c_str());
}

void VoiceRecognitionApp::updateChatHistory(const std::string& message, bool isUser) {
    // Add to internal history
    ChatMessage chatMsg;
    chatMsg.content = message;
    chatMsg.isUser = isUser;
    chatMsg.timestamp = time(nullptr);
    m_chatHistory.push_back(chatMsg);
    
    // Update UI
    std::string displayText = (isUser ? "You: " : "Assistant: ") + message;
    lv_obj_t* btn = lv_list_add_btn(m_chatList, nullptr, displayText.c_str());
    
    if (isUser) {
        lv_obj_set_style_text_color(btn, lv_color_hex(0x3498DB), 0);
    } else {
        lv_obj_set_style_text_color(btn, lv_color_hex(0x27AE60), 0);
    }
    
    // Scroll to bottom
    lv_obj_scroll_to_y(m_chatList, LV_COORD_MAX, LV_ANIM_ON);
}

bool VoiceRecognitionApp::isNetworkAvailable() {
    // For demo purposes, return true
    // In real implementation, check WiFi/network status
    return true;
}

std::string VoiceRecognitionApp::languageToString(Language lang) {
    switch (lang) {
        case Language::ENGLISH: return "English";
        case Language::SPANISH: return "Spanish";
        case Language::FRENCH: return "French";
        case Language::GERMAN: return "German";
        case Language::CHINESE: return "Chinese";
        case Language::JAPANESE: return "Japanese";
        default: return "English";
    }
}

std::string VoiceRecognitionApp::getLanguageCode(Language lang) {
    switch (lang) {
        case Language::ENGLISH: return "en";
        case Language::SPANISH: return "es";
        case Language::FRENCH: return "fr";
        case Language::GERMAN: return "de";
        case Language::CHINESE: return "zh";
        case Language::JAPANESE: return "ja";
        default: return "en";
    }
}

os_error_t VoiceRecognitionApp::loadSettings() {
    // In a real implementation, load from file system
    return OS_OK;
}

os_error_t VoiceRecognitionApp::saveSettings() {
    // In a real implementation, save to file system
    return OS_OK;
}

os_error_t VoiceRecognitionApp::loadChatHistory() {
    // In a real implementation, load from file system
    return OS_OK;
}

os_error_t VoiceRecognitionApp::saveChatHistory() {
    // In a real implementation, save to file system
    return OS_OK;
}

// Static callbacks
void VoiceRecognitionApp::listenButtonCallback(lv_event_t* e) {
    VoiceRecognitionApp* app = static_cast<VoiceRecognitionApp*>(lv_event_get_user_data(e));
    app->startListening();
}

void VoiceRecognitionApp::stopButtonCallback(lv_event_t* e) {
    VoiceRecognitionApp* app = static_cast<VoiceRecognitionApp*>(lv_event_get_user_data(e));
    app->stopListening();
}

void VoiceRecognitionApp::languageCallback(lv_event_t* e) {
    VoiceRecognitionApp* app = static_cast<VoiceRecognitionApp*>(lv_event_get_user_data(e));
    lv_obj_t* dropdown = lv_event_get_target(e);
    uint16_t selected = lv_dropdown_get_selected(dropdown);
    app->m_currentLanguage = (Language)selected;
    app->saveSettings();
}

void VoiceRecognitionApp::sensitivityCallback(lv_event_t* e) {
    VoiceRecognitionApp* app = static_cast<VoiceRecognitionApp*>(lv_event_get_user_data(e));
    lv_obj_t* slider = lv_event_get_target(e);
    app->m_microphoneSensitivity = (uint8_t)lv_slider_get_value(slider);
    app->saveSettings();
}

void VoiceRecognitionApp::volumeCallback(lv_event_t* e) {
    VoiceRecognitionApp* app = static_cast<VoiceRecognitionApp*>(lv_event_get_user_data(e));
    lv_obj_t* slider = lv_event_get_target(e);
    app->m_speakerVolume = (uint8_t)lv_slider_get_value(slider);
    app->saveSettings();
}

void VoiceRecognitionApp::wakeWordCallback(lv_event_t* e) {
    VoiceRecognitionApp* app = static_cast<VoiceRecognitionApp*>(lv_event_get_user_data(e));
    lv_obj_t* sw = lv_event_get_target(e);
    app->m_wakeWordEnabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
    app->saveSettings();
}

void VoiceRecognitionApp::apiKeyCallback(lv_event_t* e) {
    VoiceRecognitionApp* app = static_cast<VoiceRecognitionApp*>(lv_event_get_user_data(e));
    lv_obj_t* textarea = lv_event_get_target(e);
    app->m_chatGPTAPIKey = lv_textarea_get_text(textarea);
    app->saveSettings();
}

void VoiceRecognitionApp::clearHistoryCallback(lv_event_t* e) {
    VoiceRecognitionApp* app = static_cast<VoiceRecognitionApp*>(lv_event_get_user_data(e));
    app->m_chatHistory.clear();
    lv_obj_clean(app->m_chatList);
    app->saveChatHistory();
}

void VoiceRecognitionApp::textInputCallback(lv_event_t* e) {
    // Handle text input ready event
}

void VoiceRecognitionApp::sendTextCallback(lv_event_t* e) {
    VoiceRecognitionApp* app = static_cast<VoiceRecognitionApp*>(lv_event_get_user_data(e));
    const char* text = lv_textarea_get_text(app->m_textInput);
    
    if (strlen(text) > 0) {
        std::string message = text;
        lv_textarea_set_text(app->m_textInput, "");
        
        // Add user message to chat
        app->updateChatHistory(message, true);
        
        // For demo, provide a simple response
        std::string response = "I received your message: \"" + message + "\". This is a demo response.";
        app->updateChatHistory(response, false);
    }
}

extern "C" std::unique_ptr<BaseApp> createVoiceRecognitionApp() {
    return std::make_unique<VoiceRecognitionApp>();
}