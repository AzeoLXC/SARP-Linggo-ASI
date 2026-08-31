#include "clipboard_listener.h"
#include "translator.h"
#include <windows.h>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace SARPLinggo {

ClipboardListener* g_clipboard_listener = nullptr;

ClipboardListener::~ClipboardListener() {
    stop();
}

void ClipboardListener::init(GroqTranslator* translator, const std::string& style) {
    m_translator = translator;
    m_style = style;
}

void ClipboardListener::start() {
    if (m_running) return;
    m_running = true;
    m_thread = std::thread(&ClipboardListener::run, this);
}

void ClipboardListener::stop() {
    if (!m_running) return;
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

std::string ClipboardListener::get_clipboard_text() {
    if (!OpenClipboard(NULL)) return "";
    HANDLE hData = GetClipboardData(CF_TEXT);
    if (!hData) {
        CloseClipboard();
        return "";
    }
    char* pszText = static_cast<char*>(GlobalLock(hData));
    if (!pszText) {
        CloseClipboard();
        return "";
    }
    std::string text(pszText);
    GlobalUnlock(hData);
    CloseClipboard();
    return text;
}

bool ClipboardListener::set_clipboard_text(const std::string& text) {
    if (!OpenClipboard(NULL)) return false;
    EmptyClipboard();
    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (!hGlob) {
        CloseClipboard();
        return false;
    }
    memcpy(GlobalLock(hGlob), text.c_str(), text.size() + 1);
    GlobalUnlock(hGlob);
    SetClipboardData(CF_TEXT, hGlob);
    CloseClipboard();
    return true;
}

static inline std::string to_lower(const std::string& s) {
    std::string res = s;
    std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c){ return std::tolower(c); });
    return res;
}

bool ClipboardListener::is_indonesian_text(const std::string& text) {
    if (text.rfind("/me ", 0) == 0 || text.rfind("/do ", 0) == 0) {
        return true;
    }

    // List of Indonesian common markers
    const char* id_markers[] = {
        "apa", "apakah", "siapa", "dimana", "kapan", "mengapa", "kenapa", "bagaimana", "gimana",
        "kamu", "saya", "aku", "dia", "mereka", "kita", "kami", "anda", "ini", "itu", "yang",
        "dan", "atau", "tidak", "gak", "nggak", "ngga", "ga", "tak", "bukan", "ada", "bisa",
        "bila", "jika", "kalau", "sudah", "udah", "belum", "akan", "mau", "ingin", "harus",
        "adalah", "lagi", "sedang", "dapat", "sama", "dengan", "ke", "di", "dari", "untuk",
        "pada", "kabar", "baik", "tolong", "makasih", "terima", "kasih", "mas", "mbak", "gan",
        "min", "halo", "selamat", "pagi", "siang", "malam", "sore", "iya", "ya", "enggak",
        "gua", "gue", "lu", "sampe", "sampai", "bener", "benar", "sih", "dong", "kan",
        "lah", "deh", "kok", "noh", "tuh", "nih", "nanti", "kemarin", "besok", "mana",
        "sini", "situ", "sana", "brapa", "berapa", "bang", "orang", "kerja", "jalan", "makan",
        "minum", "beli", "jual", "rumah", "mobil", "motor", "anjir", "anjg", "jir", "goblok",
        "tolol", "bego", "kontol", "memek", "peler", "bgst", "woi", "ganteng", "cantik",
        "harga", "tawar", "sewa", "flat", "gubuk", "dijual", "dicari", "butuh", "surat",
        "lengkap", "insu", "up", "dana", "tapi", "jadi", "karena", "sebab", "oleh", "bunuh",
        "cepat", "minta", "maaf", "bahkan", "supaya", "agar", "setiap", "terus", "biar",
        "tahu", "tau", "ganti", "dulu", "duluan", "pasti", "mohon", "coba", "bikin", "buat",
        "pukul", "tendang", "tembak", "lari", "pergi", "pulang", "datang", "bawa", "taruh",
        "kasi", "kasih", "kasii", "keterlaluan", "kencing", "celana", "hahaha", "wkwk"
    };

    std::string lower = to_lower(text);
    std::stringstream ss(lower);
    std::string word;
    int id_hits = 0;
    int total_words = 0;

    while (ss >> word) {
        // Strip punctuation
        word.erase(std::remove_if(word.begin(), word.end(), [](char c){ return ispunct(c); }), word.end());
        if (word.empty()) continue;
        total_words++;

        for (const char* marker : id_markers) {
            if (word == marker) {
                id_hits++;
                break;
            }
        }
    }

    if (total_words == 0) return false;
    return (id_hits > 0);
}

void ClipboardListener::run() {
    while (m_running) {
        if (!m_enabled || !m_translator) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        std::string current = get_clipboard_text();
        if (!current.empty() && current != m_last_clipboard) {
            m_last_clipboard = current;

            // Check if text starts with URLs or file paths or already handled
            if (current.find("http://") == 0 || current.find("https://") == 0 ||
                current.find("c:\\") == 0 || current.find("C:\\") == 0 ||
                current.find("d:\\") == 0 || current.find("D:\\") == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                continue;
            }

            if (is_indonesian_text(current)) {
                std::string translated = m_translator->translate_outbound(current, m_style);
                if (!translated.empty() && translated.find("[Groq Error") == std::string::npos && translated.find("[Lisensi") == std::string::npos) {
                    m_last_clipboard = translated;
                    set_clipboard_text(translated);
                    if (m_on_translated) {
                        m_on_translated(current, translated);
                    }
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

} // namespace SARPLinggo
