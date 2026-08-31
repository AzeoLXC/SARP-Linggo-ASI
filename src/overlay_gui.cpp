#include "overlay_gui.h"
#include "imgui.h"
#include <commdlg.h>
#include <cstring>
#include <shlobj.h>

namespace SARPLinggo {

OverlayGUI* g_gui = nullptr;

void OverlayGUI::set_config(Config* cfg) {
    m_config = cfg;
    if (m_config) {
        strncpy(m_api_keys_input, m_config->groq_api_key.c_str(), sizeof(m_api_keys_input) - 1);
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

    ImGuiIO& io = ImGui::GetIO();
    if (m_cursor_mode) {
        io.MouseDrawCursor = true;
    } else {
        io.MouseDrawCursor = false;
    }

    ImGui::SetNextWindowSize(ImVec2(480, 520), ImGuiCond_FirstUseEver);
    
    std::string title = "SA-RP Linggo v1.3  [Shift+H: Hide Overlay | Shift+Enter: " + 
                        std::string(m_cursor_mode ? "Unlocked]" : "Locked]");
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;
    if (!m_cursor_mode) {
        flags |= ImGuiWindowFlags_NoInputs;
    }

    if (m_config) {
        ImGui::SetNextWindowBgAlpha(m_config->opacity);
    }

    if (ImGui::Begin(title.c_str(), nullptr, flags)) {
        if (ImGui::BeginTabBar("MainTabBar")) {
            
            // TAB 1: Feed Chat
            if (ImGui::BeginTabItem("Feed Chat")) {
                ImGui::Text("Riwayat terjemahan pesan game:");
                ImGui::SameLine();
                if (ImGui::Button("Hapus Feed")) {
                    m_feed_items.clear();
                }

                ImGui::Separator();
                ImGui::BeginChild("FeedScrollRegion", ImVec2(0, 0), true);
                for (const auto& card : m_feed_items) {
                    ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                    if (card.type == "ME") color = ImVec4(0.8f, 0.4f, 1.0f, 1.0f);
                    else if (card.type == "DO") color = ImVec4(0.4f, 0.8f, 1.0f, 1.0f);
                    else if (card.type == "OUTBOUND") color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
                    else if (card.type == "ERROR") color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);

                    ImGui::TextColored(color, "[%s] %s (%s)", card.timestamp.c_str(), card.sender.c_str(), card.type.c_str());
                    ImGui::TextWrapped("%s", card.original_text.c_str());
                    if (!card.translated_text.empty()) {
                        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.2f, 1.0f), "-> %s", card.translated_text.c_str());
                    }
                    ImGui::Separator();
                }
                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
                    ImGui::SetScrollHereY(1.0f);
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            // TAB 2: Terjemah Keluar (Outbound)
            if (ImGui::BeginTabItem("Terjemah Keluar")) {
                ImGui::Text("Tulis teks Bahasa Indonesia atau aksi /me /do di bawah:");
                ImGui::InputTextMultiline("##OutboundInput", m_outbound_input, sizeof(m_outbound_input), ImVec2(-1, 80));

                ImGui::Text("Gaya Bahasa Inggris (Outbound Style):");
                const char* styles[] = { "Standard English", "American Hood" };
                static int current_style_idx = 0;
                if (m_config && m_config->outbound_style == "American Hood") current_style_idx = 1;
                else current_style_idx = 0;

                if (ImGui::Combo("##StyleCombo", &current_style_idx, styles, IM_ARRAYSIZE(styles))) {
                    if (m_config) {
                        m_config->outbound_style = styles[current_style_idx];
                        if (m_clipboard_listener) m_clipboard_listener->set_style(m_config->outbound_style);
                    }
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

            // TAB 3: Pengaturan
            if (ImGui::BeginTabItem("Pengaturan")) {
                ImGui::Text("Groq AI API Key Pool (Multi-Token Dynamic Rolling Pool):");
                ImGui::InputTextMultiline("##APIKeysInput", m_api_keys_input, sizeof(m_api_keys_input), ImVec2(-1, 70));
                ImGui::TextDisabled("Masukkan banyak token dipisahkan koma, spasi, atau baris baru (gsk_...)");

                if (ImGui::Button("Auto-Detect Key Dari Folder Downloads")) {
                    auto detected = Config::detect_groq_api_key();
                    if (!detected.empty()) {
                        std::string comb;
                        for (size_t i = 0; i < detected.size(); ++i) {
                            comb += detected[i] + (i + 1 < detected.size() ? ", " : "");
                        }
                        strncpy(m_api_keys_input, comb.c_str(), sizeof(m_api_keys_input) - 1);
                        if (m_config) {
                            m_config->groq_api_key = comb;
                            if (m_translator) m_translator->update_api_keys(m_config->get_api_keys());
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

                if (ImGui::Button("Auto-Detect Path")) {
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
                if (ImGui::Button("Cek Kuota RPD Semua Token Pool")) {
                    if (m_translator) {
                        m_translator->check_rpd_quota(m_rpd_report);
                    }
                }

                if (!m_rpd_report.empty()) {
                    ImGui::Text("Status Token Pool:");
                    ImGui::TextWrapped("%s", m_rpd_report.c_str());
                }

                ImGui::Separator();
                if (ImGui::Button("Simpan Pengaturan Ke INI", ImVec2(-1, 30))) {
                    if (m_config) {
                        m_config->groq_api_key = m_api_keys_input;
                        m_config->chatlog_path = m_chatlog_path_input;
                        m_config->save(Config::get_game_directory() + "\\SARPLinggo.ini");
                        if (m_translator) m_translator->update_api_keys(m_config->get_api_keys());
                        if (m_chat_listener) m_chat_listener->set_path(m_config->chatlog_path);
                    }
                }

                ImGui::EndTabItem();
            }

            // TAB 4: Lisensi & Info
            if (ImGui::BeginTabItem("Lisensi & Info")) {
                ImGui::Text("=== SA-RP Linggo ASI Mod v1.3 ===");
                ImGui::Text("Pengembang: SA-RP Linggo Team");
                ImGui::Text("Status Sistem: Aktif (DirectX9 Native Hook)");
                ImGui::Text("Lisensi: Fully Open Source (Free / No License Required)");
                ImGui::Separator();
                ImGui::Text("Pintasan Keyboard:");
                ImGui::BulletText("Shift + H    : Hide / Show Overlay Window");
                ImGui::BulletText("Shift + Enter: Unlock / Lock Cursor Interaksi");
                ImGui::Separator();
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "[+] Bebas Digunakan: Cukup isi Groq API Key di tab Setelan.");

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

} // namespace SARPLinggo
