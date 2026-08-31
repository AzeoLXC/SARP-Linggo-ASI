#include <windows.h>
#include "config.h"
#include "translator.h"
#include "chat_listener.h"
#include "clipboard_listener.h"
#include "overlay_gui.h"
#include "d3d9_hook.h"

namespace SARPLinggo {

DWORD WINAPI PluginMainThread(LPVOID lpParam) {
    // Wait for GTA San Andreas window initialization
    Sleep(2000);

    // Initialize Config
    std::string config_path = Config::get_game_directory() + "\\SARPLinggo.ini";
    g_config.load(config_path);

    // Initialize Groq Translator
    g_translator = new GroqTranslator();
    g_translator->set_developer_mode(g_config.developer_mode);
    g_translator->update_api_keys(g_config.get_api_keys());

    // Initialize Overlay GUI
    g_gui = new OverlayGUI();
    g_gui->set_config(&g_config);
    g_gui->set_translator(g_translator);

    // Initialize Clipboard Listener
    g_clipboard_listener = new ClipboardListener();
    g_clipboard_listener->init(g_translator, g_config.outbound_style);
    g_clipboard_listener->set_enabled(g_config.enable_clipboard_outbound);
    g_clipboard_listener->set_on_translated([](const std::string& original, const std::string& translated) {
        if (g_gui) {
            ChatItem item;
            item.timestamp = "Now";
            item.sender = "Clipboard";
            item.original_text = original;
            item.translated_text = translated;
            item.type = "OUTBOUND";
            g_gui->add_chat_card(item);
        }
    });
    g_gui->set_clipboard_listener(g_clipboard_listener);
    g_clipboard_listener->start();

    // Initialize Chatlog Listener
    g_chat_listener = new ChatlogListener();
    g_chat_listener->set_path(g_config.chatlog_path);
    g_chat_listener->set_use_codsmp(g_config.use_codsmp);
    g_chat_listener->set_on_new_item([](const ChatItem& item) {
        if (!g_config.enable_inbound_chatlog) return;
        if (item.type == "SAYS" && !g_config.auto_translate_ic) return;
        if ((item.type == "ME" || item.type == "DO") && !g_config.auto_translate_medo) return;

        if (g_translator && g_gui) {
            std::string translated = g_translator->translate_inbound(item.original_text, g_config.target_language);
            ChatItem full_item = item;
            full_item.translated_text = translated;
            g_gui->add_chat_card(full_item);
        }
    });
    g_gui->set_chat_listener(g_chat_listener);
    g_chat_listener->start();

    // Initialize DirectX 9 Hook
    D3D9Hook::init();

    return 0;
}

} // namespace SARPLinggo

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        CreateThread(NULL, 0, SARPLinggo::PluginMainThread, NULL, 0, NULL);
    } else if (fdwReason == DLL_PROCESS_DETACH) {
        if (SARPLinggo::g_clipboard_listener) {
            SARPLinggo::g_clipboard_listener->stop();
            delete SARPLinggo::g_clipboard_listener;
        }
        if (SARPLinggo::g_chat_listener) {
            SARPLinggo::g_chat_listener->stop();
            delete SARPLinggo::g_chat_listener;
        }
        SARPLinggo::D3D9Hook::shutdown();
        if (SARPLinggo::g_gui) delete SARPLinggo::g_gui;
        if (SARPLinggo::g_translator) delete SARPLinggo::g_translator;
    }
    return TRUE;
}
