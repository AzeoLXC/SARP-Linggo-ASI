#pragma once
#include <string>
#include <functional>
#include <thread>
#include <atomic>

namespace SARPLinggo {

class UniversalTranslator;

class ClipboardListener {
private:
    UniversalTranslator* m_translator = nullptr;
    std::string m_style = "Standard English";
    std::function<void(const std::string&, const std::string&)> m_on_translated;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_enabled{true};
    std::string m_last_clipboard;

    void run();

public:
    ClipboardListener() = default;
    ~ClipboardListener() { stop(); }

    void init(UniversalTranslator* translator, const std::string& style);
    void set_enabled(bool enabled) { m_enabled = enabled; }
    void set_style(const std::string& style) { m_style = style; }
    void set_on_translated(std::function<void(const std::string&, const std::string&)> cb) { m_on_translated = cb; }

    static std::string get_clipboard_text();
    static bool set_clipboard_text(const std::string& text);
    static bool is_indonesian_text(const std::string& text);

    void start();
    void stop();
};

extern ClipboardListener* g_clipboard_listener;

} // namespace SARPLinggo
