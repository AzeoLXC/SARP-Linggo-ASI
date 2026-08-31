#pragma once
#include <string>
#include <functional>
#include <thread>
#include <atomic>

namespace SARPLinggo {

struct ChatItem {
    std::string timestamp;
    std::string sender;
    std::string original_text;
    std::string translated_text;
    std::string type; // "SAYS", "ME", "DO", "OUTBOUND", "ERROR"
};

class ChatlogListener {
private:
    std::string m_path;
    bool m_use_codsmp = false;
    std::function<void(const ChatItem&)> m_on_new_item;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    uint64_t m_last_offset = 0;

    void run();

public:
    ChatlogListener() = default;
    ~ChatlogListener();

    void set_path(const std::string& path) { m_path = path; }
    void set_use_codsmp(bool use) { m_use_codsmp = use; }
    void set_on_new_item(std::function<void(const ChatItem&)> cb) { m_on_new_item = cb; }
    
    std::string get_active_chatlog_path() const { return m_path; }
    ChatItem parse_line(const std::string& line);

    void start();
    void stop();
};

extern ChatlogListener* g_chat_listener;

} // namespace SARPLinggo
