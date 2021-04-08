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
// todo
oConsoleStderrPrintFunc stderrPrintFuncReal    = (oConsoleStderrPrintFunc)  ((BaseAddress + 0x0435B2) - ExecutableBase);
oScreenPrintFunc        screenPrintFuncReal    = (oScreenPrintFunc)         screenFuncAddr;

typedef long(__stdcall* EndScene)(LPDIRECT3DDEVICE9);
typedef long(__stdcall* Reset)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);

static Reset oReset = NULL;
static EndScene oEndScene = NULL;

static char inputBuffer[16];
static char finalAddr[16];
static char exeBaseStr[16];
static char baseAddrStr[16];

long __stdcall hkEndScene(LPDIRECT3DDEVICE9 pDevice) {
    if (!isImguiInit) {
        D3DDEVICE_CREATION_PARAMETERS params;
        pDevice->GetCreationParameters(&params);

        impl::win32::init(params.hFocusWindow);

        ImGui::CreateContext();
        ImGui_ImplWin32_Init(params.hFocusWindow);
        ImGui_ImplDX9_Init(pDevice);

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
        if (ImGui::BeginTabItem("VA to A")) {
            _itoa_s(ExecutableBase, exeBaseStr, 16 , 16);
            _itoa_s(BaseAddress, baseAddrStr, 16, 16);

            ImGui::Text("Calculate Virtual Address to Current Address");
            ImGui::Text("((Base Address + Virtual Address) - EXE Base)");
            ImGui::InputText("EXE Base", exeBaseStr, 16, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsUppercase);
            ImGui::InputText("Base Address", baseAddrStr, 16, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsUppercase);
            ImGui::InputText("Enter your VA here", inputBuffer, 16, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);

            // ((Base Addr + VA) - EXE Base) = Target Address
            _itoa_s(((unsigned int) (BaseAddress + strtol(inputBuffer, NULL, 16) - ExecutableBase)), finalAddr, 16, 16);
            ImGui::InputText("Target Address", finalAddr, 16,  ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsUppercase);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("A to VA")) {
            _itoa_s(ExecutableBase, exeBaseStr, 16 , 16);
            _itoa_s(BaseAddress, baseAddrStr, 16, 16);

            ImGui::Text("Calculate Virtual Address to Current Address");
            ImGui::Text("((Virtual Address - Base Address) + EXE Base)");
            ImGui::InputText("EXE Base", exeBaseStr, 16, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsUppercase);
            ImGui::InputText("Base Address", baseAddrStr, 16, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsUppercase);
            ImGui::InputText("Enter your Addr here", inputBuffer, 16, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);

            _itoa_s(((unsigned int) ((strtol(inputBuffer, NULL, 16) - BaseAddress) + ExecutableBase)), finalAddr, 16, 16);
            ImGui::InputText("Target VA", finalAddr, 16,  ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsUppercase);
            ImGui::EndTabItem();
        }
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

    return oEndScene(pDevice);
}
long __stdcall hkReset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    long result = oReset(pDevice, pPresentationParameters);
    ImGui_ImplDX9_CreateDeviceObjects();

    return result;
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
                SetCameraState(UNLOCKED);
                printf("[Monokuma] Debug Menu Closed\n");
            } else {
                CurrentDebugMenu = DEBUG;
                SetCameraState(LOCKED);
                printf("[Monokuma] Debug Menu Opened\n");
            }

            continue;
        }

        debounce = false;

        // Check if we closed the debug menu in-game so we can set our state.
        if ((lobyte & *debugByte) == 0x00 && CurrentDebugMenu == DEBUG) {
            CurrentDebugMenu = NONE;
            SetCameraState(UNLOCKED);
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
        // Listen for keypress.
        auto debugKeypress = GetAsyncKeyState(VK_F6) & 0x8000;

        if (debugKeypress) {
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
                    SetCameraState(UNLOCKED);
                    printf("[Monokuma] Camera Debug Menu Closed\n");
                    break;
                case 0x01:
                    *debugByte = 0x06;
                    CurrentDebugMenu = CAMERA;
                    SetCameraState(LOCKED);
                    printf("[Monokuma] Camera Debug Menu Opened\n");
                    break;
                default:
                    printf("[Monokuma] Cannot open Camera Debug Menu, currently in invalid state!\n");
                    break;
            }

            debounce = false;
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

        debounce = false;
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
    if (kiero::init(kiero::RenderType::D3D9) == kiero::Status::Success) {
        // TODO: Reduce kiero minhook dependency by using detours instead
        //DetourAttach((void**)&oEndScene, hkEndScene);
        //DetourAttach(&(PVOID &) kiero::getMethodsTable()[42], hkEndScene);
        //DetourAttach(&(PVOID &) kiero::getMethodsTable()[16], hkReset);
        kiero::bind(16, (void**)&oReset, hkReset);
        kiero::bind(42, (void**)&oEndScene, hkEndScene);
        printf("[Monokuma] Kiero D3D9 Hook initialized\n");
        printf("[Monokuma] If the patch is applied, the debug menu should show up.\n");
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

            printf("[Monokuma] Base addr for EXE is %Xh\n", baseAddr);
            printf("[Monokuma] EXE base is %Xh\n", exeBase);
            printf("[Monokuma] Base addr for EXE is %Xh\n", BaseAddress);
            printf("[Monokuma] EXE base is %Xh\n", ExecutableBase);
            printf("[Monokuma] Addr for func stdoutPrintFunc is %Xh.\n", stdoutFuncAddr);
            printf("[Monokuma] Addr for func screenPrintFunc is %Xh.\n", screenFuncAddr);
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourAttach(&(PVOID &) stdoutPrintFuncReal, println);
            DetourAttach(&(PVOID &) screenPrintFuncReal, println_screen);

            //DetourAttach(&(PVOID &) stderrPrintFuncReal, printerr); // TODO Broken function, outputs garbage.
            DetourTransactionCommit();

            printf("[Monokuma] Detoured.\n");
            printf("[Monokuma] Enjoy!\n\n");
            break;

        case DLL_PROCESS_DETACH:
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourDetach(&(PVOID &) stdoutPrintFuncReal, println);
            DetourDetach(&(PVOID &) screenPrintFuncReal, println_screen);

            //DetourDetach(&(PVOID &) stderrPrintFuncReal, printerr); // TODO Broken function, outputs garbage.
            DetourTransactionCommit();

            printf("\n\n[Monokuma] Detour detached.\n");
            std::cout << "[Monokuma] So long, bear well!" << std::endl;
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
