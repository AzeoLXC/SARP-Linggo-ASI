#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <map>
#include <chrono>

namespace SARPLinggo {

struct KeyState {
    int failure_count = 0;
    int remaining_requests = 1000;
    int total_limit = 1000;
    std::string reset_time;
    std::chrono::system_clock::time_point cooldown_until;
    bool is_rate_limited = false;
};

class KeyPoolManager {
private:
    std::vector<std::string> m_keys;
    std::map<std::string, KeyState> m_key_states;
    size_t m_current_index = 0;
    mutable std::mutex m_mutex;

public:
    KeyPoolManager() = default;
    ~KeyPoolManager() = default;

    void update_keys(const std::vector<std::string>& keys);
    std::vector<std::string> get_keys() const;
    size_t total_keys() const;

    bool get_next_working_key(std::string& out_key, unsigned int& out_index, std::string& out_status);
    void mark_rate_limited(const std::string& key, int cooldown_minutes, const std::string& reset_time);
    void update_key_metrics(const std::string& key, int remaining, int limit, const std::string& reset_time);
    std::string get_pool_summary() const;
};

struct EndpointInfo {
    std::wstring host;
    unsigned short port;
    std::wstring path;
    bool is_https;
};

class UniversalTranslator {
private:
    KeyPoolManager m_key_pool;
    bool m_developer_mode = false;

    static EndpointInfo parse_url(const std::string& url_str);
    bool send_winhttp_request(const std::string& endpoint, const std::string& api_key, const std::string& payload, int& out_status_code, std::string& out_body);
    std::string clean_translation_output(const std::string& raw) const;

public:
    UniversalTranslator();
    ~UniversalTranslator() = default;

    void set_developer_mode(bool dev) { m_developer_mode = dev; }
    void update_api_keys(const std::vector<std::string>& keys);
    
    std::string translate_inbound(const std::string& text, const std::string& target_lang = "Indonesian");
    std::string translate_outbound(const std::string& text, const std::string& style = "Standard English");
    
    void check_rpd_quota(std::string& out_report);
    std::string get_pool_summary() const { return m_key_pool.get_pool_summary(); }
};

extern UniversalTranslator* g_translator;

} // namespace SARPLinggo
