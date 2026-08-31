#pragma once
#include <string>
#include <vector>

namespace SARPLinggo {

enum class AIProvider {
    Groq = 0,
    OpenAI,
    DeepSeek,
    Claude,
    Gemini,
    Ollama,
    OpenRouter,
    Custom
};

struct Config {
    int provider_type = 0; // 0=Groq, 1=OpenAI, 2=DeepSeek, 3=Claude, 4=Gemini, 5=Ollama, 6=OpenRouter, 7=Custom
    std::string api_key;
    std::string custom_endpoint = "https://api.groq.com/openai/v1/chat/completions";
    std::string model_name = "openai/gpt-oss-20b";
    
    std::string target_language = "Indonesian";
    std::string outbound_style = "Standard English";
    std::string chatlog_path;
    
    bool use_codsmp = false;
    bool enable_clipboard_outbound = true;
    bool enable_inbound_chatlog = true;
    bool auto_translate_ic = true;
    bool auto_translate_medo = true;
    bool developer_mode = false;
    
    float font_size = 14.0f;
    float opacity = 0.90f;
    bool click_through = false;
    int max_feed_items = 100;
    
    std::vector<std::string> api_keys;

    static std::string get_game_directory();
    static std::string detect_chatlog_path();
    static std::vector<std::string> detect_groq_api_key();
    
    std::vector<std::string> get_api_keys() const;
    void load(const std::string& filepath);
    void save(const std::string& filepath);
};

extern Config g_config;

} // namespace SARPLinggo
