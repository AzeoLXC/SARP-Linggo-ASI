#include "chat_listener.h"
#include <windows.h>
#include <fstream>
#include <regex>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

namespace SARPLinggo {

ChatlogListener* g_chat_listener = nullptr;

ChatlogListener::~ChatlogListener() {
    stop();
}

void ChatlogListener::start() {
    if (m_running) return;
    m_running = true;
    m_thread = std::thread(&ChatlogListener::run, this);
}

void ChatlogListener::stop() {
    if (!m_running) return;
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

static inline std::string strip_color_codes(const std::string& input) {
    std::regex hex_color("\\{[A-Fa-f0-9]{6}\\}");
    return std::regex_replace(input, hex_color, "");
}

ChatItem ChatlogListener::parse_line(const std::string& raw_line) {
    ChatItem item;
    if (raw_line.empty()) return item;

    std::string clean = strip_color_codes(raw_line);
    
    // Ignore spam / system prefixes
    const char* ignore_prefixes[] = {
        "PAYCHECK:", "SERVER:", "MOTD:", "MASK:", "Ad:", "Contact Info:",
        "[ADMIN]", "[NEWS]", "[AD]", "[RADIO]", "[R]", "Screenshot",
        "Connecting to", "Connected."
    };
    for (const char* pfx : ignore_prefixes) {
        if (clean.find(pfx) != std::string::npos) {
            return item;
        }
    }

    std::smatch match;
    std::string time_str;
    std::regex time_pattern("^\\[(\\d{2}:\\d{2}:\\d{2})\\]\\s*");
    if (std::regex_search(clean, match, time_pattern)) {
        time_str = match[1].str();
        clean = clean.substr(match.length());
    }

    // Roleplay /me: * Name action (( Name )) or * Name action
    std::regex me_pattern("^\\*\\s+([A-Za-z0-9_]+)\\s+(.+)$");
    // Roleplay /do: * environment/action (( Name ))
    std::regex do_pattern("^\\*\\s+(.+?)\\s*\\(\\(\\s*([A-Za-z0-9_]+)\\s*\\)\\)$");
    // In-character speech: Name says/shouts/whispers: text
    std::regex speech_pattern("^([A-Za-z0-9_]+)\\s+(says|shouts|whispers)(?:\\s+\\([^\\)]+\\))?:\\s*(.+)$");

    if (std::regex_match(clean, match, do_pattern)) {
        item.timestamp = time_str;
        item.sender = match[2].str();
        item.original_text = "* " + match[1].str() + " (( " + match[2].str() + " ))";
        item.type = "DO";
        return item;
    }

    if (std::regex_match(clean, match, speech_pattern)) {
        item.timestamp = time_str;
        item.sender = match[1].str();
        item.original_text = match[3].str();
        item.type = "SAYS";
        return item;
    }

    if (std::regex_match(clean, match, me_pattern)) {
        item.timestamp = time_str;
        item.sender = match[1].str();
        item.original_text = "* " + match[1].str() + " " + match[2].str();
        item.type = "ME";
        return item;
    }

    return item;
}

void ChatlogListener::run() {
    while (m_running) {
        if (m_path.empty() || !fs::exists(m_path)) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        // Open non-locking read
        HANDLE hFile = CreateFileA(m_path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        LARGE_INTEGER size;
        if (GetFileSizeEx(hFile, &size)) {
            uint64_t current_size = static_cast<uint64_t>(size.QuadPart);
            if (m_last_offset == 0) {
                m_last_offset = current_size;
            } else if (current_size < m_last_offset) {
                // File reset
                m_last_offset = 0;
            } else if (current_size > m_last_offset) {
                LARGE_INTEGER move_pos;
                move_pos.QuadPart = m_last_offset;
                SetFilePointerEx(hFile, move_pos, NULL, FILE_BEGIN);

                DWORD to_read = static_cast<DWORD>(current_size - m_last_offset);
                std::vector<char> buf(to_read + 1, 0);
                DWORD bytes_read = 0;
                if (ReadFile(hFile, buf.data(), to_read, &bytes_read, NULL) && bytes_read > 0) {
                    m_last_offset += bytes_read;
                    std::string chunk(buf.data(), bytes_read);
                    std::stringstream ss(chunk);
                    std::string line;
                    while (std::getline(ss, line)) {
                        if (!line.empty() && line.back() == '\r') line.pop_back();
                        ChatItem item = parse_line(line);
                        if (!item.original_text.empty() && m_on_new_item) {
                            m_on_new_item(item);
                        }
                    }
                }
            }
        }
        CloseHandle(hFile);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

} // namespace SARPLinggo
