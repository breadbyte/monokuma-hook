#include "Monokuma.h"
#include "kiero/kiero.h"

#include <detours/detours.h>
#include <windows.h>

#include <d3d9.h>
#include <d3dx9.h>
#include <assert.h>
#include <iostream>

#include <imgui.h>
#include "kiero/examples/imgui/impl/win32_impl.h"
#include "kiero/examples/imgui/imgui/examples/imgui_impl_win32.h"
#include "kiero/examples/imgui/imgui/examples/imgui_impl_dx9.h"
#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

int stdoutFuncAddr = ((BaseAddress + 0x130B00) - ExecutableBase);
int stderrFuncAddr = ((BaseAddress + 0x0435A9) - ExecutableBase);
int screenFuncAddr = ((BaseAddress + 0x0435B1) - ExecutableBase);
bool isImguiInit = false;
bool isWantImgui = false;

ScreenPrintCommandBuffer cmdBuf = ScreenPrintCommandBuffer();

bool bAttachConsole() {
    // Detach any existing console. Useful for development
    FreeConsole();
    if (AllocConsole()) {
        FILE *dummy;
        freopen_s(&dummy, "CONOUT$", "w", stderr);
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout.clear();
        std::cerr.clear();
        std::clog.clear();

        std::cout << "[Monokuma] Rise and shine, ursine!" << std::endl;
        return true;
    }
    else return false;
}
void println(const char* pattern, ...) {
    va_list args;
            va_start(args, pattern);
    vfprintf(stdout, pattern, args);
            va_end(args);
    std::cout << std::endl;
}
void printerr(const char* pattern, ...) {
    va_list args;
    va_start(args, pattern);
    vfprintf(stdout, pattern, args);
    va_end(args);
    std::cout << std::endl;
}
void println_screen(int x, int y, const char* buffer) {
    auto str = std::string(buffer);

    auto scr = ScreenPrintCommand{x, y, str};
    cmdBuf.push(scr);
}

typedef void (*oConsoleStdoutPrintFunc)(const char* pattern, ...);
typedef void (*oConsoleStderrPrintFunc)(const char* pattern, ...);
typedef void (*oScreenPrintFunc)(int xPos, int yPos, const char* buffer);

oConsoleStdoutPrintFunc stdoutPrintFuncReal    = (oConsoleStdoutPrintFunc)  stdoutFuncAddr;
oConsoleStderrPrintFunc stderrPrintFuncReal    = (oConsoleStderrPrintFunc)  stderrFuncAddr;
oScreenPrintFunc        screenPrintFuncReal    = (oScreenPrintFunc)         screenFuncAddr;

typedef long(__stdcall* ResetEx)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*, D3DDISPLAYMODEEX*);
typedef long(__stdcall* PresentEx)(LPDIRECT3DDEVICE9EX, const RECT*, const RECT*, HWND, const RGNDATA*, DWORD);

static ResetEx oResetEx = NULL;
static PresentEx oPresentEx = NULL;

static char exeBaseStr[16];
static char baseAddrStr[16];

long __stdcall hkResetEx(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pFullscreenDisplayMode)
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    long result = oResetEx(pDevice, pPresentationParameters, pFullscreenDisplayMode);
    ImGui_ImplDX9_CreateDeviceObjects();

    return result;
}
long __stdcall hkPresentEx(LPDIRECT3DDEVICE9EX pDeviceEx, const RECT* pSourceRect,const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion, DWORD dwFlags) {

    if (!isImguiInit) {
        D3DDEVICE_CREATION_PARAMETERS params;
        pDeviceEx->GetCreationParameters(&params);

        impl::win32::init(params.hFocusWindow);

        ImGui::CreateContext();
        ImGui_ImplWin32_Init(params.hFocusWindow);
        ImGui_ImplDX9_Init(pDeviceEx);

        isImguiInit = true;
    }

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Manually called imgui.
    if (isWantImgui) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
        ImGui::Begin("Utilities", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::BeginTabBar("##UtilitiesTabBar", ImGuiTabBarFlags_Reorderable);
        if (ImGui::BeginTabItem("State")) {
            switch (CurrentDebugMenu) {
                case NONE:
                    ImGui::Text("Current State: None");
                    break;
                case DEBUG:
                    ImGui::Text("Current State: Debug Menu");
                    break;
                case CAMERA:
                    ImGui::Text("Current State: Camera Placement");
                    break;
                case FORCED_OPEN:
                    ImGui::Text("Current State: Forced Open");
                    break;
            }
            ImGui::Text("AVG FPS: %f", ImGui::GetIO().Framerate);
            _itoa_s(ExecutableBase, exeBaseStr, 16 , 16);
            _itoa_s(BaseAddress, baseAddrStr, 16, 16);
            ImGui::InputText("EXE Base", exeBaseStr, 16, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsUppercase);
            ImGui::InputText("Base Address", baseAddrStr, 16, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsUppercase);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
        ImGui::End();
    } else {
        // Only draw the cursor if imgui is called.
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
        ImGui::GetIO().MouseDrawCursor = false;
    }

    if (CurrentDebugMenu != NONE) {
        ImGui::Begin("Debug Menu", NULL, ImGuiWindowFlags_AlwaysAutoResize);

        auto wndPos = ImGui::GetWindowPos();
        auto cmds = cmdBuf.pull();

        if (cmds.empty()) {
            ImGui::SetCursorScreenPos(ImVec2(wndPos.x + 0 + 5, wndPos.y + 0 + 20));
            ImGui::TextColored(ImVec4(255, 0, 255, 255), "%s", "Nobody here but us chickens!");
        }

        for (auto &cmd : cmds) {

            // [x + 5], [y + 20] to account for default window decoration
            ImGui::SetCursorScreenPos(ImVec2(wndPos.x + cmd.xPos + 5, wndPos.y + cmd.yPos + 20));
            ImGui::TextColored(ImVec4(255, 0, 255, 255), "%s", cmd.text.c_str());
        }

        ImGui::End();
    }

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

    return oPresentEx(pDeviceEx, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);
}

// F5 - Debug Menu
[[noreturn]] VOID WINAPI ListenKeyPressDebug(){
    bool debounce = false;
    int lobyte = 0x000000FF;

    int* debugByte = (int*)((BaseAddress + 0x2D84B0) - ExecutableBase); // Debug Toggle

    while (true) {
        auto keypress = GetAsyncKeyState(VK_F5) & 0x8000;
        if (keypress) {
            if (debounce)
                continue;
            debounce = true;

            if (CurrentDebugMenu != NONE)
                if (CurrentDebugMenu != DEBUG)
                    continue;

            // Toggle the debug menu bytes.
            *debugByte = *debugByte ^ 0x00000001;

            if ((lobyte & *debugByte) == 0x00) {
                CurrentDebugMenu = NONE;
                SetInputState(UNLOCKED);
                printf("[Monokuma] Debug Menu Closed\n");
            } else {
                CurrentDebugMenu = DEBUG;
                SetInputState(LOCKED);
                printf("[Monokuma] Debug Menu Opened\n");
            }

            continue;
        }
        else {
            debounce = false;
        }

        // Check if we closed the debug menu in-game so we can set our state.
        if ((lobyte & *debugByte) == 0x00 && CurrentDebugMenu == DEBUG) {
            CurrentDebugMenu = NONE;
            SetInputState(UNLOCKED);
            printf("[Monokuma] Debug Menu Closed\n");
        }
    }
}
// F6 - Camera Debug Menu
[[noreturn]] VOID WINAPI ListenKeyPressPlayerCameraDebug() {
    bool debounce = false;

    // Player Camera Debug Byte
    int* debugByte    = (int*)((BaseAddress + 0x36CC60) - ExecutableBase); // Debug Toggle

    while (true) {
        auto keypress = GetAsyncKeyState(VK_F6) & 0x8000;
        if (keypress) {
            if (debounce)
                continue;
            debounce = true;

            if (CurrentDebugMenu != NONE)
                if (CurrentDebugMenu != CAMERA)
                    continue;

            switch (*debugByte) {
                case 0x06:
                    *debugByte = 0x01;
                    CurrentDebugMenu = NONE;

                    printf("[Monokuma] Camera Debug Menu Closed\n");
                    break;
                case 0x01:
                    *debugByte = 0x06;
                    CurrentDebugMenu = CAMERA;

                    printf("[Monokuma] Camera Debug Menu Opened\n");
                    break;
                default:
                    printf("[Monokuma] Cannot open Camera Debug Menu, currently in invalid state!\n");
                    break;
            }

        }
        else {
            debounce = false;
        }

        if (*debugByte == 0x01 && CurrentDebugMenu == CAMERA) {
            CurrentDebugMenu = NONE;

            printf("[Monokuma] Camera Debug Menu Closed\n");
        }
    }
}
// F9 - Force Debug Menu On (Development Option)
[[noreturn]] VOID WINAPI ListenKeyPressForceDebugMenu() {
    bool debounce = false;

    while (true) {
        auto keypress = (GetAsyncKeyState(VK_F9) & 0x8000);
        if (keypress) {
            if (debounce)
                continue;
            debounce = true;

            if (CurrentDebugMenu != NONE)
                if (CurrentDebugMenu != FORCED_OPEN)
                    continue;

            if (CurrentDebugMenu == FORCED_OPEN) {
                CurrentDebugMenu = NONE;
                printf("[Monokuma] Debug Menu Unforced Open\n");
            } else {
                CurrentDebugMenu = FORCED_OPEN;
                printf("[Monokuma] Debug Menu Forced Open\n");
            }

            continue;

        }
        else {
            debounce = false;
        }
    }
}
// F10 - Utilities Overlay
[[noreturn]] VOID WINAPI ListenKeyPressImguiOverlay(){
    bool debounce = false;

    while (true) {
        auto wantImgui = (GetAsyncKeyState(VK_F10) & 0x8000);
        if (wantImgui) {
            if (debounce)
                continue;

            debounce = true;

            isWantImgui = !isWantImgui;
            SetInputState(isWantImgui ? LOCKED : UNLOCKED);
        }
        else {
            debounce = false;
        }
    }
}

VOID WINAPI ApplyPatch(){
    patch_dr1_us_exe();
}

int __CLRCALL_PURE_OR_STDCALL kieroInitThread()
{
    if (kiero::init(kiero::RenderType::D3D9EX) == kiero::Status::Success) {
        oResetEx      = reinterpret_cast<ResetEx>(kiero::getMethodsTable()[132]);
        oPresentEx    = reinterpret_cast<PresentEx>(kiero::getMethodsTable()[121]);
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        DetourAttach(&(PVOID &) oPresentEx, hkPresentEx);
        DetourAttach(&(PVOID &) oResetEx, hkResetEx);

        DetourTransactionCommit();
        printf("[Monokuma] Kiero D3D9 Hook initialized\n");
        return 1;
    }
    printf("[Monokuma] Failed to initialize Kiero D3D9 Hook\n");
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            if (DetourIsHelperProcess()) {
                return TRUE;
            }
            if (!bAttachConsole()) {
                return false;
            }

            // Fire and forget our keypress threads
            CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(ListenKeyPressDebug), NULL, 0, NULL);
            CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(ListenKeyPressPlayerCameraDebug), NULL, 0, NULL);
            CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(ListenKeyPressForceDebugMenu), NULL, 0, NULL);
            CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(ListenKeyPressImguiOverlay), NULL, 0, NULL);

            // Fire and forget our d3d9 hook
            CreateThread(NULL, 0,  reinterpret_cast<LPTHREAD_START_ROUTINE>(kieroInitThread), NULL, 0, NULL);

            // Fire and forget our patcher thread
            CreateThread(NULL, 0,  reinterpret_cast<LPTHREAD_START_ROUTINE>(ApplyPatch), NULL, 0, NULL);

            printf("[Monokuma] Base addr for EXE is %Xh\n", BaseAddress);
            printf("[Monokuma] EXE base is %Xh\n", ExecutableBase);
            printf("[Monokuma] Addr for func stdoutPrintFunc is %Xh.\n", stdoutFuncAddr);
            printf("[Monokuma] Addr for func screenPrintFunc is %Xh.\n", screenFuncAddr);
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourAttach(&(PVOID &) stdoutPrintFuncReal, println);
            DetourAttach(&(PVOID &) screenPrintFuncReal, println_screen);
            DetourAttach(&(PVOID &) stderrPrintFuncReal, printerr);
            DetourTransactionCommit();

            printf("[Monokuma] Detoured.\n");
            printf("[Monokuma] Enjoy!\n\n");
            break;

        case DLL_PROCESS_DETACH:
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourDetach(&(PVOID &) stdoutPrintFuncReal, println);
            DetourDetach(&(PVOID &) screenPrintFuncReal, println_screen);
            DetourDetach(&(PVOID &) oPresentEx, hkPresentEx);
            DetourDetach(&(PVOID &) oResetEx, hkResetEx);
            DetourDetach(&(PVOID &) stderrPrintFuncReal, printerr);
            DetourTransactionCommit();

            printf("\n\n[Monokuma] Detour detached.\n");
            std::cout << "[Monokuma] So long, bear well!" << std::endl;
            unpatch_dr1_us_exe();
            FreeConsole();
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
        default:
            return false;
    }
    return TRUE;
}
