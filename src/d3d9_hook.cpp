#include "d3d9_hook.h"
#include "overlay_gui.h"
#include "imgui.h"
#include "backends/imgui_impl_dx9.h"
#include "backends/imgui_impl_win32.h"
#include "vendor/minhook/MinHook.h"
#include <iostream>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace SARPLinggo {

bool D3D9Hook::m_ready = false;
HWND InputHook::m_hwnd = NULL;
WNDPROC InputHook::m_original_wndproc = NULL;
bool InputHook::m_ready = false;

typedef HRESULT(WINAPI* Present_t)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
typedef HRESULT(WINAPI* Reset_t)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
typedef BOOL(WINAPI* SetCursorPos_t)(int, int);
typedef BOOL(WINAPI* ClipCursor_t)(const RECT*);
typedef int(WINAPI* ShowCursor_t)(BOOL);

static Present_t oPresent = nullptr;
static Reset_t oReset = nullptr;
static SetCursorPos_t oSetCursorPos = nullptr;
static ClipCursor_t oClipCursor = nullptr;
static ShowCursor_t oShowCursor = nullptr;

static bool imgui_initialized = false;
static IDirect3DDevice9* s_device = nullptr;

static BOOL WINAPI hkSetCursorPos(int X, int Y) {
    if (g_gui && g_gui->is_toggled() && g_gui->is_in_cursor_mode()) {
        return TRUE;
    }
    return oSetCursorPos ? oSetCursorPos(X, Y) : TRUE;
}

static BOOL WINAPI hkClipCursor(const RECT* lpRect) {
    if (g_gui && g_gui->is_toggled() && g_gui->is_in_cursor_mode()) {
        return TRUE;
    }
    return oClipCursor ? oClipCursor(lpRect) : TRUE;
}

static int WINAPI hkShowCursor(BOOL bShow) {
    if (g_gui && g_gui->is_toggled() && g_gui->is_in_cursor_mode()) {
        return oShowCursor ? oShowCursor(TRUE) : 1;
    }
    return oShowCursor ? oShowCursor(bShow) : 0;
}

LRESULT CALLBACK InputHook::WndProcHook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (g_gui) {
        if (uMsg == WM_KEYDOWN) {
            bool shift_down = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            if (shift_down && wParam == 'H') {
                g_gui->toggle_visibility();
                return 0;
            }
            if (shift_down && wParam == VK_RETURN) {
                g_gui->toggle_cursor_mode();
                return 0;
            }
        }

        if (g_gui->is_toggled() && g_gui->is_in_cursor_mode()) {
            if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) {
                return 1;
            }
        }
    }

    return CallWindowProcA(m_original_wndproc, hWnd, uMsg, wParam, lParam);
}

bool InputHook::init(HWND hwnd) {
    if (m_ready) return true;
    m_hwnd = hwnd;
    m_original_wndproc = (WNDPROC)SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProcHook);
    m_ready = (m_original_wndproc != NULL);
    return m_ready;
}

void InputHook::shutdown() {
    if (m_ready && m_hwnd && m_original_wndproc) {
        SetWindowLongPtrA(m_hwnd, GWLP_WNDPROC, (LONG_PTR)m_original_wndproc);
        m_ready = false;
    }
}

static void imgui_shutdown() {
    if (imgui_initialized) {
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        imgui_initialized = false;
    }
}

static HRESULT WINAPI hkPresent(IDirect3DDevice9* pDevice, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion) {
    s_device = pDevice;

    if (!imgui_initialized) {
        D3DDEVICE_CREATION_PARAMETERS params;
        if (SUCCEEDED(pDevice->GetCreationParameters(&params))) {
            InputHook::init(params.hFocusWindow);

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

            ImGui::StyleColorsDark();

            ImGui_ImplWin32_Init(params.hFocusWindow);
            ImGui_ImplDX9_Init(pDevice);
            imgui_initialized = true;
        }
    }

    if (imgui_initialized && g_gui) {
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        g_gui->render();

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    }

    return oPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
}

static HRESULT WINAPI hkReset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    if (imgui_initialized) {
        ImGui_ImplDX9_InvalidateDeviceObjects();
    }
    HRESULT hr = oReset(pDevice, pPresentationParameters);
    if (SUCCEEDED(hr) && imgui_initialized) {
        ImGui_ImplDX9_CreateDeviceObjects();
    }
    return hr;
}

static bool GetD3D9VTable(void** vtable) {
    IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D) return false;

    HWND hWnd = CreateWindowExA(0, "BUTTON", "Dummy", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, NULL, NULL);
    if (!hWnd) {
        pD3D->Release();
        return false;
    }

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = hWnd;

    IDirect3DDevice9* pDummyDevice = nullptr;
    HRESULT hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDummyDevice);
    if (FAILED(hr) || !pDummyDevice) {
        DestroyWindow(hWnd);
        pD3D->Release();
        return false;
    }

    memcpy(vtable, *reinterpret_cast<void***>(pDummyDevice), 119 * sizeof(void*));

    pDummyDevice->Release();
    DestroyWindow(hWnd);
    pD3D->Release();
    return true;
}

bool D3D9Hook::init() {
    if (m_ready) return true;

    if (MH_Initialize() != MH_OK) {
        return false;
    }

    void* d3d9_vtable[119];
    if (!GetD3D9VTable(d3d9_vtable)) {
        return false;
    }

    // Hook Present (Index 17) and Reset (Index 16)
    if (MH_CreateHook(d3d9_vtable[17], (void*)&hkPresent, (void**)&oPresent) != MH_OK) {
        return false;
    }
    if (MH_CreateHook(d3d9_vtable[16], (void*)&hkReset, (void**)&oReset) != MH_OK) {
        return false;
    }

    // Hook cursor functions for locked/unlocked cursor mode
    HMODULE hUser32 = GetModuleHandleA("user32.dll");
    if (hUser32) {
        void* pSetCursorPos = (void*)GetProcAddress(hUser32, "SetCursorPos");
        if (pSetCursorPos) MH_CreateHook(pSetCursorPos, (void*)&hkSetCursorPos, (void**)&oSetCursorPos);

        void* pClipCursor = (void*)GetProcAddress(hUser32, "ClipCursor");
        if (pClipCursor) MH_CreateHook(pClipCursor, (void*)&hkClipCursor, (void**)&oClipCursor);

        void* pShowCursor = (void*)GetProcAddress(hUser32, "ShowCursor");
        if (pShowCursor) MH_CreateHook(pShowCursor, (void*)&hkShowCursor, (void**)&oShowCursor);
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        return false;
    }

    m_ready = true;
    return true;
}

void D3D9Hook::shutdown() {
    if (m_ready) {
        imgui_shutdown();
        InputHook::shutdown();
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        m_ready = false;
    }
}

} // namespace SARPLinggo
