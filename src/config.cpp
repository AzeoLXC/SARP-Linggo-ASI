#include "config.h"
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

namespace SARPLinggo {

Config g_config;

static inline std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string Config::get_game_directory() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string path(buffer);
    size_t pos = path.find_last_of("\\/");
    return (pos != std::string::npos) ? path.substr(0, pos) : ".";
}

std::string Config::detect_chatlog_path() {
    char docPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, 0, docPath))) {
        std::string p1 = std::string(docPath) + "\\GTA San Andreas User Files\\SAMP\\chatlog.txt";
        if (fs::exists(p1)) return p1;
        std::string p2 = std::string(docPath) + "\\Documents\\GTA San Andreas User Files\\SAMP\\chatlog.txt";
        if (fs::exists(p2)) return p2;
    }
    
    const char* userProfile = getenv("USERPROFILE");
    if (userProfile) {
        std::string pOneDrive = std::string(userProfile) + "\\OneDrive\\Documents\\GTA San Andreas User Files\\SAMP\\chatlog.txt";
        if (fs::exists(pOneDrive)) return pOneDrive;
        
        std::string pDocs = std::string(userProfile) + "\\Documents\\GTA San Andreas User Files\\SAMP\\chatlog.txt";
        if (fs::exists(pDocs)) return pDocs;
    }

    std::string pPub = "C:\\Users\\Public\\Documents\\GTA San Andreas User Files\\SAMP\\chatlog.txt";
    if (fs::exists(pPub)) return pPub;

    return "";
}

std::vector<std::string> Config::detect_groq_api_key() {
    std::vector<std::string> keys;
    const char* userProfile = getenv("USERPROFILE");
    if (!userProfile) return keys;

    std::string dlPath = std::string(userProfile) + "\\Downloads";
    if (!fs::exists(dlPath)) return keys;

    for (const auto& entry : fs::directory_iterator(dlPath)) {
        if (entry.is_regular_file()) {
            std::string filename = entry.path().filename().string();
            if (filename.find("gsk_") != std::string::npos && filename.ends_with(".txt")) {
                std::ifstream f(entry.path());
                std::string line;
                while (std::getline(f, line)) {
                    std::string trimmed = trim(line);
                    if (trimmed.rfind("gsk_", 0) == 0 && trimmed.length() >= 20) {
                        keys.push_back(trimmed);
                    }
                }
            }
        }
    }
    return keys;
}

std::vector<std::string> Config::get_api_keys() const {
    std::vector<std::string> res;
    std::stringstream ss(groq_api_key);
    std::string item;
    while (std::getline(ss, item, ',')) {
        std::string t = trim(item);
        if (!t.empty()) res.push_back(t);
    }
    return res;
}

void Config::load(const std::string& filepath) {
    if (!fs::exists(filepath)) {
        chatlog_path = detect_chatlog_path();
        return;
    }

    char buf[1024];
    GetPrivateProfileStringA("Settings", "GroqAPIKey", "", buf, sizeof(buf), filepath.c_str());
    groq_api_key = buf;

    GetPrivateProfileStringA("Settings", "GroqModel", "openai/gpt-oss-20b", buf, sizeof(buf), filepath.c_str());
    groq_model = buf;

    GetPrivateProfileStringA("Settings", "TargetLanguage", "Indonesian", buf, sizeof(buf), filepath.c_str());
    target_language = buf;

    GetPrivateProfileStringA("Settings", "OutboundStyle", "Standard English", buf, sizeof(buf), filepath.c_str());
    outbound_style = buf;

    GetPrivateProfileStringA("Settings", "ChatlogPath", "", buf, sizeof(buf), filepath.c_str());
    chatlog_path = buf;
    if (chatlog_path.empty()) {
        chatlog_path = detect_chatlog_path();
    }

    GetPrivateProfileStringA("Settings", "LicenseToken", "", buf, sizeof(buf), filepath.c_str());
    license_token = buf;

    use_codsmp = GetPrivateProfileIntA("Settings", "UseCodSMP", 0, filepath.c_str()) != 0;
    enable_clipboard_outbound = GetPrivateProfileIntA("Settings", "EnableClipboardOutbound", 1, filepath.c_str()) != 0;
    enable_inbound_chatlog = GetPrivateProfileIntA("Settings", "EnableInboundChatlog", 1, filepath.c_str()) != 0;
    auto_translate_ic = GetPrivateProfileIntA("Settings", "AutoTranslateIC", 1, filepath.c_str()) != 0;
    auto_translate_medo = GetPrivateProfileIntA("Settings", "AutoTranslateMeDo", 1, filepath.c_str()) != 0;
    developer_mode = GetPrivateProfileIntA("Settings", "DeveloperMode", 0, filepath.c_str()) != 0;

    GetPrivateProfileStringA("Settings", "FontSize", "14.0", buf, sizeof(buf), filepath.c_str());
    font_size = static_cast<float>(atof(buf));

    GetPrivateProfileStringA("Settings", "Opacity", "0.90", buf, sizeof(buf), filepath.c_str());
    opacity = static_cast<float>(atof(buf));

    click_through = GetPrivateProfileIntA("Settings", "ClickThrough", 0, filepath.c_str()) != 0;
    max_feed_items = GetPrivateProfileIntA("Settings", "MaxFeedItems", 100, filepath.c_str());
}

void Config::save(const std::string& filepath) {
    WritePrivateProfileStringA("Settings", "GroqAPIKey", groq_api_key.c_str(), filepath.c_str());
    WritePrivateProfileStringA("Settings", "GroqModel", groq_model.c_str(), filepath.c_str());
    WritePrivateProfileStringA("Settings", "TargetLanguage", target_language.c_str(), filepath.c_str());
    WritePrivateProfileStringA("Settings", "OutboundStyle", outbound_style.c_str(), filepath.c_str());
    WritePrivateProfileStringA("Settings", "ChatlogPath", chatlog_path.c_str(), filepath.c_str());
    WritePrivateProfileStringA("Settings", "LicenseToken", license_token.c_str(), filepath.c_str());
    WritePrivateProfileStringA("Settings", "UseCodSMP", use_codsmp ? "1" : "0", filepath.c_str());
    WritePrivateProfileStringA("Settings", "EnableClipboardOutbound", enable_clipboard_outbound ? "1" : "0", filepath.c_str());
    WritePrivateProfileStringA("Settings", "EnableInboundChatlog", enable_inbound_chatlog ? "1" : "0", filepath.c_str());
    WritePrivateProfileStringA("Settings", "AutoTranslateIC", auto_translate_ic ? "1" : "0", filepath.c_str());
    WritePrivateProfileStringA("Settings", "AutoTranslateMeDo", auto_translate_medo ? "1" : "0", filepath.c_str());
    WritePrivateProfileStringA("Settings", "DeveloperMode", developer_mode ? "1" : "0", filepath.c_str());
    WritePrivateProfileStringA("Settings", "FontSize", std::to_string(font_size).c_str(), filepath.c_str());
    WritePrivateProfileStringA("Settings", "Opacity", std::to_string(opacity).c_str(), filepath.c_str());
    WritePrivateProfileStringA("Settings", "ClickThrough", click_through ? "1" : "0", filepath.c_str());
    WritePrivateProfileStringA("Settings", "MaxFeedItems", std::to_string(max_feed_items).c_str(), filepath.c_str());
}

} // namespace SARPLinggo
