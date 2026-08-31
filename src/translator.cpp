#include "translator.h"
#include "config.h"
#include <regex>
#include <sstream>
#include <iostream>
#include <iomanip>

namespace SARPLinggo {

UniversalTranslator* g_translator = nullptr;

static inline std::string escape_json(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        if (c == '"') o << "\\\"";
        else if (c == '\\') o << "\\\\";
        else if (c == '\b') o << "\\b";
        else if (c == '\f') o << "\\f";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else if ((unsigned char)c <= '\x1f') {
            o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
        } else {
            o << c;
        }
    }
    return o.str();
}

static inline std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n\"`*");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n\"`*");
    return s.substr(start, end - start + 1);
}

// --- KeyPoolManager ---

void KeyPoolManager::update_keys(const std::vector<std::string>& keys) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_keys = keys;
    for (const auto& k : keys) {
        if (m_key_states.find(k) == m_key_states.end()) {
            m_key_states[k] = KeyState();
        }
    }
    if (m_current_index >= m_keys.size()) {
        m_current_index = 0;
    }
}

std::vector<std::string> KeyPoolManager::get_keys() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_keys;
}

size_t KeyPoolManager::total_keys() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_keys.size();
}

bool KeyPoolManager::get_next_working_key(std::string& out_key, unsigned int& out_index, std::string& out_status) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_keys.empty()) {
        out_key = "";
        out_index = 0;
        out_status = "No Key Required";
        return true;
    }

    auto now = std::chrono::system_clock::now();
    size_t tries = 0;
    while (tries < m_keys.size()) {
        size_t idx = (m_current_index + tries) % m_keys.size();
        const std::string& candidate = m_keys[idx];
        auto& state = m_key_states[candidate];

        if (state.is_rate_limited && now < state.cooldown_until) {
            tries++;
            continue;
        }

        state.is_rate_limited = false;
        m_current_index = (idx + 1) % m_keys.size();
        out_key = candidate;
        out_index = static_cast<unsigned int>(idx + 1);
        out_status = "Key #" + std::to_string(out_index) + " Siap";
        return true;
    }

    out_status = "Semua API key sedang rate-limited / cooldown";
    return false;
}

void KeyPoolManager::mark_rate_limited(const std::string& key, int cooldown_minutes, const std::string& reset_time) {
    if (key.empty()) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& state = m_key_states[key];
    state.is_rate_limited = true;
    state.failure_count++;
    state.reset_time = reset_time;
    state.cooldown_until = std::chrono::system_clock::now() + std::chrono::minutes(cooldown_minutes);
}

void KeyPoolManager::update_key_metrics(const std::string& key, int remaining, int limit, const std::string& reset_time) {
    if (key.empty()) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& state = m_key_states[key];
    state.remaining_requests = remaining;
    state.total_limit = limit;
    state.reset_time = reset_time;
}

std::string KeyPoolManager::get_pool_summary() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_keys.empty()) return "0 Token Terdaftar (Direct Mode)";
    
    int active_ready = 0;
    auto now = std::chrono::system_clock::now();
    for (const auto& k : m_keys) {
        auto it = m_key_states.find(k);
        if (it != m_key_states.end()) {
            if (!it->second.is_rate_limited || now >= it->second.cooldown_until) {
                active_ready++;
            }
        }
    }
    return std::to_string(m_keys.size()) + " Token Terdaftar | Rolling Pointer: Key #" + 
           std::to_string(m_current_index + 1) + " (" + std::to_string(active_ready) + "/" + 
           std::to_string(m_keys.size()) + " Siap)";
}

// --- UniversalTranslator ---

UniversalTranslator::UniversalTranslator() {}

void UniversalTranslator::update_api_keys(const std::vector<std::string>& keys) {
    m_key_pool.update_keys(keys);
}

EndpointInfo UniversalTranslator::parse_url(const std::string& url_str) {
    EndpointInfo info;
    info.is_https = true;
    info.port = 443;
    info.path = L"/openai/v1/chat/completions";
    info.host = L"api.groq.com";

    std::string s = url_str;
    if (s.empty()) return info;

    if (s.rfind("http://", 0) == 0) {
        info.is_https = false;
        info.port = 80;
        s = s.substr(7);
    } else if (s.rfind("https://", 0) == 0) {
        info.is_https = true;
        info.port = 443;
        s = s.substr(8);
    }

    size_t slash_pos = s.find('/');
    std::string host_port = (slash_pos != std::string::npos) ? s.substr(0, slash_pos) : s;
    std::string path_part = (slash_pos != std::string::npos) ? s.substr(slash_pos) : "/";

    size_t colon_pos = host_port.find(':');
    if (colon_pos != std::string::npos) {
        std::string host_only = host_port.substr(0, colon_pos);
        int port_num = std::stoi(host_port.substr(colon_pos + 1));
        info.host = std::wstring(host_only.begin(), host_only.end());
        info.port = static_cast<unsigned short>(port_num);
    } else {
        info.host = std::wstring(host_port.begin(), host_port.end());
    }

    info.path = std::wstring(path_part.begin(), path_part.end());
    return info;
}

bool UniversalTranslator::send_winhttp_request(const std::string& endpoint, const std::string& api_key, const std::string& payload, int& out_status_code, std::string& out_body) {
    EndpointInfo ep = parse_url(endpoint);

    HINTERNET hSession = WinHttpOpen(L"SA-RP-Linggo-ASI/1.4", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    WinHttpSetTimeouts(hSession, 5000, 5000, 10000, 10000);

    HINTERNET hConnect = WinHttpConnect(hSession, ep.host.c_str(), ep.port, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD flags = ep.is_https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", ep.path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    std::wstring headers = L"Content-Type: application/json\r\n";
    if (!api_key.empty()) {
        if (g_config.provider_type == 3) {
            headers += L"x-api-key: " + std::wstring(api_key.begin(), api_key.end()) + L"\r\nanthropic-version: 2023-06-01\r\n";
        } else {
            headers += L"Authorization: Bearer " + std::wstring(api_key.begin(), api_key.end()) + L"\r\n";
        }
    }

    BOOL bResults = WinHttpSendRequest(hRequest, headers.c_str(), (DWORD)-1L, (LPVOID)payload.data(), (DWORD)payload.size(), (DWORD)payload.size(), 0);

    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, NULL);
    }

    if (bResults) {
        DWORD dwStatusCode = 0;
        DWORD dwSize = sizeof(dwStatusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);
        out_status_code = static_cast<int>(dwStatusCode);

        DWORD dwDownloaded = 0;
        std::string response_data;
        do {
            dwSize = 0;
            if (WinHttpQueryDataAvailable(hRequest, &dwSize) && dwSize > 0) {
                std::vector<char> buffer(dwSize + 1);
                if (WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) {
                    response_data.append(buffer.data(), dwDownloaded);
                }
            }
        } while (dwSize > 0);
        out_body = response_data;
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return bResults != FALSE;
}

std::string UniversalTranslator::clean_translation_output(const std::string& raw) const {
    std::string text = raw;
    std::regex think_regex("<think>[\\s\\S]*?</think>");
    text = std::regex_replace(text, think_regex, "");
    return trim(text);
}

std::string UniversalTranslator::translate_inbound(const std::string& text, const std::string& target_lang) {
    std::string key, status;
    unsigned int key_idx = 0;
    if (!m_key_pool.get_next_working_key(key, key_idx, status)) {
        if (g_config.provider_type != 5) {
            return "[API Key Belum Diisi / Token Pool Kosong]";
        }
    }

    std::string sys_prompt = "You are a professional translator for GTA SA-MP roleplay.\\nTranslate foreign input text into clear, formal, natural " + target_lang + " (Bahasa " + target_lang + " yang baik, benar, dan natural).\\n\\nCRITICAL DIRECTIVES:\\n1. OUTPUT ONLY THE FINAL TRANSLATION SENTENCE.\\n2. DO NOT use <think> tags, internal monologue, reasoning, or explanations.\\n3. Use standard natural pronouns instead of harsh street slang.";

    std::string model = g_config.model_name.empty() ? "openai/gpt-oss-20b" : g_config.model_name;
    std::string endpoint = g_config.custom_endpoint.empty() ? "https://api.groq.com/openai/v1/chat/completions" : g_config.custom_endpoint;

    std::string payload;
    if (g_config.provider_type == 3) {
        payload = "{\"model\":\"" + model + "\",\"max_tokens\":1024,\"system\":\"" + sys_prompt + "\",\"messages\":[{\"role\":\"user\",\"content\":\"" + escape_json(text) + "\"}]}";
    } else {
        payload = "{\"model\":\"" + model + "\",\"messages\":[{\"role\":\"system\",\"content\":\"" + 
                  sys_prompt + "\"},{\"role\":\"user\",\"content\":\"" + 
                  escape_json(text) + "\"}],\"temperature\":0.1,\"max_tokens\":1024}";
    }

    int status_code = 0;
    std::string body;
    if (send_winhttp_request(endpoint, key, payload, status_code, body) && status_code == 200) {
        size_t c_pos = body.find("\"content\":");
        if (c_pos != std::string::npos) {
            size_t q1 = body.find('\"', c_pos + 10);
            if (q1 != std::string::npos) {
                size_t q2 = q1 + 1;
                while (q2 < body.length()) {
                    if (body[q2] == '\"' && body[q2 - 1] != '\\') break;
                    q2++;
                }
                std::string raw_content = body.substr(q1 + 1, q2 - q1 - 1);
                return clean_translation_output(raw_content);
            }
        }
        size_t t_pos = body.find("\"text\":");
        if (t_pos != std::string::npos) {
            size_t q1 = body.find('\"', t_pos + 7);
            if (q1 != std::string::npos) {
                size_t q2 = q1 + 1;
                while (q2 < body.length()) {
                    if (body[q2] == '\"' && body[q2 - 1] != '\\') break;
                    q2++;
                }
                std::string raw_content = body.substr(q1 + 1, q2 - q1 - 1);
                return clean_translation_output(raw_content);
            }
        }
    } else if (status_code == 429) {
        m_key_pool.mark_rate_limited(key, 5, "429 Rate Limited");
    }

    return "[AI Error Status " + std::to_string(status_code) + "]";
}

std::string UniversalTranslator::translate_outbound(const std::string& text, const std::string& style) {
    std::string key, status;
    unsigned int key_idx = 0;
    if (!m_key_pool.get_next_working_key(key, key_idx, status)) {
        if (g_config.provider_type != 5) {
            return "[API Key Belum Diisi / Token Pool Kosong]";
        }
    }

    std::string sys_prompt;
    if (text.rfind("/me ", 0) == 0) {
        if (style == "American Hood") {
            sys_prompt = "You are an expert GTA SA-MP roleplay translator. Translate the input action into authentic third-person physical action roleplay for GTA SA-MP. Use vivid, authentic hood/street action descriptions (e.g. 'slides the key into the ignition', 'fires up the bike', 'tucks the piece'). DO NOT append spoken dialogue slang (like 'homie', 'cuz', 'bro') to physical actions unless the input contains actual quoted dialogue. Output MUST start with '/me '. OUTPUT ONLY the final translated '/me [ENGLISH ACTION]'.";
        } else {
            sys_prompt = "You are an expert GTA SA-MP roleplay translator. Translate the input action into proper third-person physical action roleplay in Standard English for GTA SA-MP. Output MUST start with '/me '. OUTPUT ONLY final translated '/me [ENGLISH ACTION]'.";
        }
    } else if (text.rfind("/do ", 0) == 0) {
        if (style == "American Hood") {
            sys_prompt = "You are an expert GTA SA-MP roleplay translator. Translate the environment or state input into a clear third-person environment, situation, or fact description for GTA SA-MP roleplay. DO NOT use spoken dialogue slang. Output MUST start with '/do '. OUTPUT ONLY the final translated '/do [ENGLISH DESCRIPTION]'.";
        } else {
            sys_prompt = "You are an expert GTA SA-MP roleplay translator. Translate the environment or state input into a proper third-person environment, situation, or fact description in Standard English for GTA SA-MP roleplay. Output MUST start with '/do '. OUTPUT ONLY final translated '/do [ENGLISH DESCRIPTION]'.";
        }
    } else {
        if (style == "American Hood") {
            sys_prompt = "You are an expert GTA SA-MP roleplay translator converting Indonesian text into raw, authentic American Hood / Ghetto / Gangster street spoken English (using AAVE terms like 'finna', 'nawmsayin', 'trippin', 'fool', 'homie', 'cuz', 'aight', 'pop', 'strapped', 'loc'). Preserve all swear words and aggression. DO NOT use <think> tags or internal monologue. OUTPUT ONLY final translated English text without quotes.";
        } else {
            sys_prompt = "You are an expert GTA SA-MP roleplay translator converting Indonesian text into proper, clear Standard English dialogue. Preserve swear words and tone accurately. DO NOT use <think> tags or internal monologue. OUTPUT ONLY final translated English text without quotes.";
        }
    }

    std::string model = g_config.model_name.empty() ? "openai/gpt-oss-20b" : g_config.model_name;
    std::string endpoint = g_config.custom_endpoint.empty() ? "https://api.groq.com/openai/v1/chat/completions" : g_config.custom_endpoint;

    std::string payload;
    if (g_config.provider_type == 3) {
        payload = "{\"model\":\"" + model + "\",\"max_tokens\":1024,\"system\":\"" + sys_prompt + "\",\"messages\":[{\"role\":\"user\",\"content\":\"" + escape_json(text) + "\"}]}";
    } else {
        payload = "{\"model\":\"" + model + "\",\"messages\":[{\"role\":\"system\",\"content\":\"" + 
                  sys_prompt + "\"},{\"role\":\"user\",\"content\":\"" + 
                  escape_json(text) + "\"}],\"temperature\":0.3,\"max_tokens\":1024}";
    }

    int status_code = 0;
    std::string body;
    if (send_winhttp_request(endpoint, key, payload, status_code, body) && status_code == 200) {
        size_t c_pos = body.find("\"content\":");
        if (c_pos != std::string::npos) {
            size_t q1 = body.find('\"', c_pos + 10);
            if (q1 != std::string::npos) {
                size_t q2 = q1 + 1;
                while (q2 < body.length()) {
                    if (body[q2] == '\"' && body[q2 - 1] != '\\') break;
                    q2++;
                }
                std::string raw_content = body.substr(q1 + 1, q2 - q1 - 1);
                return clean_translation_output(raw_content);
            }
        }
        size_t t_pos = body.find("\"text\":");
        if (t_pos != std::string::npos) {
            size_t q1 = body.find('\"', t_pos + 7);
            if (q1 != std::string::npos) {
                size_t q2 = q1 + 1;
                while (q2 < body.length()) {
                    if (body[q2] == '\"' && body[q2 - 1] != '\\') break;
                    q2++;
                }
                std::string raw_content = body.substr(q1 + 1, q2 - q1 - 1);
                return clean_translation_output(raw_content);
            }
        }
    } else if (status_code == 429) {
        m_key_pool.mark_rate_limited(key, 5, "429 Rate Limited");
    }

    return "[AI Error Status " + std::to_string(status_code) + "]";
}

void UniversalTranslator::check_rpd_quota(std::string& out_report) {
    std::stringstream ss;
    auto keys = m_key_pool.get_keys();
    if (keys.empty()) {
        if (g_config.provider_type == 5) {
            out_report = "Ollama Local: Ready (No API Key Required)";
        } else {
            out_report = "Tidak ada API key terdaftar";
        }
        return;
    }

    std::string model = g_config.model_name.empty() ? "openai/gpt-oss-20b" : g_config.model_name;
    std::string endpoint = g_config.custom_endpoint.empty() ? "https://api.groq.com/openai/v1/chat/completions" : g_config.custom_endpoint;

    int idx = 1;
    for (const auto& k : keys) {
        std::string payload;
        if (g_config.provider_type == 3) {
            payload = "{\"model\":\"" + model + "\",\"max_tokens\":1,\"messages\":[{\"role\":\"user\",\"content\":\"ping\"}]}";
        } else {
            payload = "{\"model\":\"" + model + "\",\"messages\":[{\"role\":\"user\",\"content\":\"ping\"}],\"max_tokens\":1}";
        }

        int status_code = 0;
        std::string body;
        bool ok = send_winhttp_request(endpoint, k, payload, status_code, body);
        ss << "Key #" << idx++ << ": ";
        if (ok && status_code == 200) {
            ss << "[OK] Active / Siap\n";
        } else if (status_code == 429) {
            ss << "[!] Rate-Limited (429)\n";
        } else if (status_code == 401) {
            ss << "[X] Invalid Key (401)\n";
        } else {
            ss << "Status " << status_code << "\n";
        }
    }
    out_report = ss.str();
}

} // namespace SARPLinggo
