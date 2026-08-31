#include "overlay_gui.h"
#include "imgui.h"
#include <windows.h>
#include <commdlg.h>
#include <cstring>
#include <fstream>
#include <algorithm>

namespace SARPLinggo {

OverlayGUI* g_gui = nullptr;

struct ProviderPreset {
    const char* name;
    const char* default_endpoint;
    const char* default_model;
    const char* key_hint;
};

static const ProviderPreset PROVIDER_PRESETS[] = {
    { "Groq Cloud (Fast & Free Tier)", "https://api.groq.com/openai/v1/chat/completions", "openai/gpt-oss-20b", "gsk_..." },
    { "OpenAI (GPT-4o / GPT-4o-mini)", "https://api.openai.com/v1/chat/completions", "gpt-4o-mini", "sk-..." },
    { "DeepSeek (V3 / R1 Reasoner)", "https://api.deepseek.com/chat/completions", "deepseek-chat", "sk-..." },
    { "Anthropic Claude (Messages API)", "https://api.anthropic.com/v1/messages", "claude-3-5-haiku-20241022", "sk-ant-..." },
    { "Google Gemini (OpenAI Compat)", "https://generativelanguage.googleapis.com/v1beta/openai/chat/completions", "gemini-1.5-flash", "AIzaSy..." },
    { "Ollama (Local Offline LLM)", "http://127.0.0.1:11434/v1/chat/completions", "llama3.2", "Tidak butuh key (Local)" },
    { "OpenRouter (Universal Router)", "https://openrouter.ai/api/v1/chat/completions", "meta-llama/llama-3.1-8b-instruct", "sk-or-..." },
    { "Custom OpenAI Endpoint", "https://your-custom-api.com/v1/chat/completions", "custom-model", "API Key / Token" }
};

void OverlayGUI::set_config(Config* cfg) {
    m_config = cfg;
    if (m_config) {
        m_selected_provider = m_config->provider_type;
        if (m_selected_provider < 0 || m_selected_provider > 7) m_selected_provider = 0;

        strncpy(m_api_keys_input, m_config->api_key.c_str(), sizeof(m_api_keys_input) - 1);
        strncpy(m_custom_endpoint_input, m_config->custom_endpoint.c_str(), sizeof(m_custom_endpoint_input) - 1);
        strncpy(m_model_name_input, m_config->model_name.c_str(), sizeof(m_model_name_input) - 1);
        strncpy(m_chatlog_path_input, m_config->chatlog_path.c_str(), sizeof(m_chatlog_path_input) - 1);
    }
}

void OverlayGUI::add_chat_card(const ChatItem& item) {
    m_feed_items.push_back(item);
    if (m_config && m_feed_items.size() > static_cast<size_t>(m_config->max_feed_items)) {
        m_feed_items.erase(m_feed_items.begin());
    }
}

void OverlayGUI::render() {
    if (!m_visible) return;

    ImGui::SetNextWindowSize(ImVec2(620, 520), ImGuiCond_FirstUseEver);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
    if (m_config && m_config->click_through && !m_cursor_mode) {
        flags |= ImGuiWindowFlags_NoInputs;
    }

    if (ImGui::Begin("SARP-Linggo Native ASI v1.4 (Open-Source Multi-AI)", &m_visible, flags)) {
        if (ImGui::BeginTabBar("MainTabBar")) {
            
            // TAB 1: Live Feed
            if (ImGui::BeginTabItem("Live Feed")) {
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Mode Status: %s", 
                                   m_cursor_mode ? "[INTERAKSI AKTIF (Shift+Enter)]" : "[IN-GAME OVERLAY]");
                
                ImGui::SameLine(ImGui::GetWindowWidth() - 110);
                if (ImGui::Button("Bersihkan Feed")) {
                    m_feed_items.clear();
                }

                ImGui::Separator();

                ImGui::BeginChild("FeedScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
                for (size_t i = 0; i < m_feed_items.size(); ++i) {
                    const auto& item = m_feed_items[i];
                    ImGui::PushID(static_cast<int>(i));

                    if (item.type == "OUTBOUND") {
                        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.4f, 1.0f), "[OUTBOUND %s] (%s):", item.sender.c_str(), item.timestamp.c_str());
                    } else if (item.type == "ME" || item.type == "DO") {
                        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "[ROLEPLAY %s] (%s):", item.type.c_str(), item.timestamp.c_str());
                    } else {
                        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "[SAYS] %s (%s):", item.sender.c_str(), item.timestamp.c_str());
                    }

                    ImGui::TextWrapped("Asli : %s", item.original_text.c_str());
                    ImGui::TextColored(ImVec4(0.6f, 1.0f, 0.8f, 1.0f), "Hasil: %s", item.translated_text.c_str());
                    
                    if (ImGui::Button("Salin Hasil")) {
                        ClipboardListener::set_clipboard_text(item.translated_text);
                    }
                    ImGui::Separator();
                    ImGui::PopID();
                }

                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                    ImGui::SetScrollHereY(1.0f);
                }
                ImGui::EndChild();

                ImGui::EndTabItem();
            }

            // TAB 2: Outbound Assistant
            if (ImGui::BeginTabItem("Outbound Assistant")) {
                ImGui::Text("Tulis kalimat / roleplay bahasa Indonesia yang ingin ditranslate:");
                ImGui::InputTextMultiline("##OutboundInput", m_outbound_input, sizeof(m_outbound_input), ImVec2(-1, 80));

                ImGui::Text("Pilihan Gaya Bahasa Terjemahan:");
                const char* styles[] = { "Standard English", "American Hood", "Formal Professional", "Casual Slang" };
                static int current_style = 0;
                if (ImGui::Combo("##StyleCombo", &current_style, styles, IM_ARRAYSIZE(styles))) {
                    if (m_config) m_config->outbound_style = styles[current_style];
                }

                if (ImGui::Button("Terjemahkan & Salin Ke Clipboard (CTRL+V)", ImVec2(-1, 35))) {
                    if (m_translator && strlen(m_outbound_input) > 0) {
                        std::string res = m_translator->translate_outbound(m_outbound_input, m_config ? m_config->outbound_style : "Standard English");
                        strncpy(m_outbound_result, res.c_str(), sizeof(m_outbound_result) - 1);
                        ClipboardListener::set_clipboard_text(res);
                        
                        ChatItem item;
                        item.timestamp = "Now";
                        item.sender = "Anda";
                        item.original_text = m_outbound_input;
                        item.translated_text = res;
                        item.type = "OUTBOUND";
                        add_chat_card(item);
                    }
                }

                if (strlen(m_outbound_result) > 0) {
                    ImGui::Separator();
                    ImGui::Text("Hasil Terjemahan (Telah disalin otomatis):");
                    ImGui::BeginChild("OutboundResultChild", ImVec2(-1, 80), true);
                    ImGui::TextWrapped("%s", m_outbound_result);
                    ImGui::EndChild();
                    if (ImGui::Button("Salin Ulang Hasil")) {
                        ClipboardListener::set_clipboard_text(m_outbound_result);
                    }
                }

                ImGui::EndTabItem();
            }

            // TAB 3: Pengaturan AI & Provider
            if (ImGui::BeginTabItem("Pengaturan AI")) {
                ImGui::TextColored(ImVec4(0.3f, 0.9f, 1.0f, 1.0f), "Pilih AI Provider & Layanan:");
                
                const char* provider_names[] = {
                    "Groq Cloud (Fast & Free Tier)",
                    "OpenAI (Official GPT-4o)",
                    "DeepSeek (DeepSeek-V3 / R1)",
                    "Anthropic Claude (Official)",
                    "Google Gemini (API)",
                    "Ollama (Local Offline LLM)",
                    "OpenRouter (Multi-Model Router)",
                    "Custom Endpoint (OpenAI Compatible)"
                };

                if (ImGui::Combo("##ProviderCombo", &m_selected_provider, provider_names, IM_ARRAYSIZE(provider_names))) {
                    if (m_config && m_selected_provider >= 0 && m_selected_provider < (int)IM_ARRAYSIZE(PROVIDER_PRESETS)) {
                        m_config->provider_type = m_selected_provider;
                        strncpy(m_custom_endpoint_input, PROVIDER_PRESETS[m_selected_provider].default_endpoint, sizeof(m_custom_endpoint_input) - 1);
                        strncpy(m_model_name_input, PROVIDER_PRESETS[m_selected_provider].default_model, sizeof(m_model_name_input) - 1);
                        m_config->custom_endpoint = m_custom_endpoint_input;
                        m_config->model_name = m_model_name_input;
                    }
                }

                ImGui::Spacing();
                ImGui::Text("Endpoint URL API:");
                ImGui::InputText("##EndpointInput", m_custom_endpoint_input, sizeof(m_custom_endpoint_input));

                ImGui::Spacing();
                ImGui::Text("Nama Model AI:");
                ImGui::InputText("##ModelInput", m_model_name_input, sizeof(m_model_name_input));

                ImGui::Spacing();
                ImGui::Text("API Key Pool (Multi-Token Dynamic Rolling Pool):");
                ImGui::InputTextMultiline("##APIKeysInput", m_api_keys_input, sizeof(m_api_keys_input), ImVec2(-1, 60));
                ImGui::TextDisabled("Format: %s (Pisahkan koma atau baris baru untuk multi-token)", PROVIDER_PRESETS[m_selected_provider].key_hint);

                if (m_selected_provider == 0) {
                    if (ImGui::Button("Auto-Detect Groq Key Dari Folder Downloads")) {
                        auto detected = Config::detect_groq_api_key();
                        if (!detected.empty()) {
                            std::string comb;
                            for (size_t i = 0; i < detected.size(); ++i) {
                                comb += detected[i] + (i + 1 < detected.size() ? ", " : "");
                            }
                            strncpy(m_api_keys_input, comb.c_str(), sizeof(m_api_keys_input) - 1);
                        }
                    }
                }

                ImGui::Separator();
                ImGui::Text("Path File SAMP chatlog.txt:");
                ImGui::InputText("##ChatlogPathInput", m_chatlog_path_input, sizeof(m_chatlog_path_input));
                ImGui::SameLine();
                if (ImGui::Button("Browse...")) {
                    OPENFILENAMEA ofn;
                    char szFile[MAX_PATH] = {0};
                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
                    ofn.nFilterIndex = 1;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                    if (GetOpenFileNameA(&ofn)) {
                        strncpy(m_chatlog_path_input, szFile, sizeof(m_chatlog_path_input) - 1);
                    }
                }

                if (ImGui::Button("Auto-Detect Chatlog Path")) {
                    std::string auto_path = Config::detect_chatlog_path();
                    if (!auto_path.empty()) {
                        strncpy(m_chatlog_path_input, auto_path.c_str(), sizeof(m_chatlog_path_input) - 1);
                    }
                }

                ImGui::Separator();
                ImGui::Text("Saklar Fitur Terjemahan (Enable / Disable):");
                if (m_config) {
                    ImGui::Checkbox("Master: Aktifkan Penerjemah Chatlog Game (Inbound)", &m_config->enable_inbound_chatlog);
                    ImGui::Checkbox("Terjemahkan Chat In-Character (SAYS)", &m_config->auto_translate_ic);
                    ImGui::Checkbox("Terjemahkan Roleplay /me dan /do", &m_config->auto_translate_medo);
                    if (ImGui::Checkbox("Master: Aktifkan Penangkap Clipboard (Outbound CTRL+C -> Inggris)", &m_config->enable_clipboard_outbound)) {
                        if (m_clipboard_listener) m_clipboard_listener->set_enabled(m_config->enable_clipboard_outbound);
                    }
                }

                ImGui::Separator();
                if (ImGui::Button("Uji Koneksi & Kuota API Pool")) {
                    if (m_translator) {
                        m_translator->check_rpd_quota(m_rpd_report);
                    }
                }

                if (!m_rpd_report.empty()) {
                    ImGui::Text("Status Koneksi Pool:");
                    ImGui::TextWrapped("%s", m_rpd_report.c_str());
                }

                ImGui::Separator();
                if (ImGui::Button("Simpan Pengaturan Ke INI", ImVec2(-1, 30))) {
                    if (m_config) {
                        m_config->provider_type = m_selected_provider;
                        m_config->api_key = m_api_keys_input;
                        m_config->custom_endpoint = m_custom_endpoint_input;
                        m_config->model_name = m_model_name_input;
                        m_config->chatlog_path = m_chatlog_path_input;
                        m_config->save(Config::get_game_directory() + "\\SARPLinggo.ini");
                        if (m_translator) m_translator->update_api_keys(m_config->get_api_keys());
                        if (m_chat_listener) m_chat_listener->set_path(m_config->chatlog_path);
                    }
                }

                ImGui::EndTabItem();
            }

            // TAB 4: Lisensi & Info
            if (ImGui::BeginTabItem("Info & Lisensi")) {
                ImGui::Text("=== SA-RP Linggo ASI Mod v1.4 ===");
                ImGui::Text("Pengembang: SA-RP Linggo Open-Source Team");
                ImGui::Text("Status Sistem: Aktif (DirectX9 Native Hook)");
                ImGui::Text("Lisensi: Fully Open Source (Free / MIT License)");
                ImGui::Separator();
                ImGui::Text("Pintasan Keyboard:");
                ImGui::BulletText("Shift + H    : Sembunyikan / Tampilkan Menu Overlay");
                ImGui::BulletText("Shift + Enter: Buka / Kunci Kursor Mouse Interaksi");
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[+] Multi-Provider Support:");
                ImGui::BulletText("Mendukung Groq Cloud, OpenAI, DeepSeek, Claude, Gemini, Ollama, OpenRouter.");
                ImGui::BulletText("Mendukung Custom API Endpoint (Local LM Studio, vLLM, Text-Gen-WebUI, dll).");

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

} // namespace SARPLinggo
