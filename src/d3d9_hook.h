#pragma once
#include <windows.h>
#include <d3d9.h>

namespace SARPLinggo {

class D3D9Hook {
public:
    static bool m_ready;
    static bool init();
    static void shutdown();
    static bool is_ready() { return m_ready; }
};

class InputHook {
public:
    static HWND m_hwnd;
    static WNDPROC m_original_wndproc;
    static bool m_ready;

    static bool init(HWND hwnd);
    static void shutdown();
    static HWND get_hwnd() { return m_hwnd; }
    static LRESULT CALLBACK WndProcHook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

} // namespace SARPLinggo
