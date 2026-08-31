#pragma once
#include <windows.h>
#include <d3d9.h>
#include <vector>
#include <string>
#include "config.h"
#include "translator.h"
#include "chat_listener.h"
#include "clipboard_listener.h"
#include "licensing.h"

namespace SARPLinggo {

class ChatlogListener;

class OverlayGUI {
private:
    bool m_visible = true;
    bool m_cursor_mode = false;
    Config* m_config = nullptr;
    GroqTranslator* m_translator = nullptr;
    ClipboardListener* m_clipboard_listener = nullptr;
    ChatlogListener* m_chat_listener = nullptr;
    LicenseManager* m_license_mgr = nullptr;

    std::vector<ChatItem> m_feed_items;
    std::string m_status_message;
    std::string m_rpd_report;
    std::string m_license_message;
    char m_outbound_input[1024] = {0};
    char m_outbound_result[1024] = {0};
    char m_api_keys_input[4096] = {0};
    char m_chatlog_path_input[MAX_PATH] = {0};
    char m_license_token_input[256] = {0};

public:
    OverlayGUI() = default;
    ~OverlayGUI() = default;

    void set_config(Config* cfg);
    void set_translator(GroqTranslator* tr) { m_translator = tr; }
    void set_clipboard_listener(ClipboardListener* cl) { m_clipboard_listener = cl; }
    void set_chat_listener(ChatlogListener* cl) { m_chat_listener = cl; }
    void set_license_manager(LicenseManager* lm) { m_license_mgr = lm; }
    void set_status(const std::string& status) { m_status_message = status; }

    void toggle_visibility() { m_visible = !m_visible; }
    void toggle_cursor_mode() { m_cursor_mode = !m_cursor_mode; }
    bool is_toggled() const { return m_visible; }
    bool is_in_cursor_mode() const { return m_cursor_mode; }

    void add_chat_card(const ChatItem& item);
    void render();
};

extern OverlayGUI* g_gui;

} // namespace SARPLinggo
