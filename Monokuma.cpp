#include "Monokuma.h"
#include "kiero/kiero.h"

#include <windows.h>
#include <detours/detours.h>

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

int baseAddr = (int) GetModuleHandle(nullptr);
int exeBase = 0x30000;
int stdoutFuncAddr = ((baseAddr + 0x130B00) - exeBase);
int screenFuncAddr = ((baseAddr + 0x0435B1) - exeBase);
bool isImguiInit = false;

// Did the user manually call Imgui?
bool isWantImgui = false;

// Did the user call the debug menu?
bool isWantDebugMenu = false;

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
oConsoleStderrPrintFunc stderrPrintFuncReal    = (oConsoleStderrPrintFunc)  ((baseAddr + 0x0435B2) - exeBase);
oScreenPrintFunc        screenPrintFuncReal    = (oScreenPrintFunc)         screenFuncAddr;

typedef long(__stdcall* EndScene)(LPDIRECT3DDEVICE9);
typedef long(__stdcall* Reset)(LPDIRECT3DDEVICE9, D3DPRESENT_PARAMETERS*);

static Reset oReset = NULL;
static EndScene oEndScene = NULL;


static char inputBuffer[16];
static char finalAddr[16];
static char exeBaseStr[16];
static char baseAddrStr[16];
bool hadOutput = false;

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
            _itoa_s(exeBase, exeBaseStr,16 ,16);
            _itoa_s(baseAddr, baseAddrStr, 16, 16);

            ImGui::Text("Calculate Virtual Address to Current Address");
            ImGui::Text("((Base Address + Virtual Address) - EXE Base)");
            ImGui::InputText("EXE Base", exeBaseStr, 16, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsUppercase);
            ImGui::InputText("Base Address", baseAddrStr, 16, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsUppercase);
            ImGui::InputText("Enter your VA here", inputBuffer, 16, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);

            // ((Base Addr + VA) - EXE Base) = Target Address
            _itoa_s(((unsigned int) (baseAddr + strtol(inputBuffer, NULL, 16) - exeBase)), finalAddr, 16, 16);
            ImGui::InputText("Target Address", finalAddr, 16,  ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsUppercase);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("A to VA")) {
            _itoa_s(exeBase, exeBaseStr,16 ,16);
            _itoa_s(baseAddr, baseAddrStr, 16, 16);

            ImGui::Text("Calculate Virtual Address to Current Address");
            ImGui::Text("((Virtual Address - Base Address) + EXE Base)");
            ImGui::InputText("EXE Base", exeBaseStr, 16, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsUppercase);
            ImGui::InputText("Base Address", baseAddrStr, 16, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsUppercase);
            ImGui::InputText("Enter your Addr here", inputBuffer, 16, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);

            _itoa_s(((unsigned int) ((strtol(inputBuffer, NULL, 16) - baseAddr)  + exeBase)), finalAddr, 16, 16);
            ImGui::InputText("Target VA", finalAddr, 16,  ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsUppercase);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
        ImGui::End();
    } else {
        // Only draw the cursor if imgui is called.
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
        ImGui::GetIO().MouseDrawCursor = false;
    }

    if (isWantDebugMenu) {
        ImGui::Begin("Debug Menu", NULL, ImGuiWindowFlags_AlwaysAutoResize);

        auto wndPos = ImGui::GetWindowPos();
        auto cmds = cmdBuf.pull();

        if (cmds.empty() && !hadOutput) {
            ImGui::SetCursorScreenPos(ImVec2(wndPos.x + 0 + 5, wndPos.y + 0 + 20));
            ImGui::TextColored(ImVec4(255, 0, 255, 255), "%s", "Nobody here but us chickens!");
        }

        for (auto &cmd : cmds) {

            // [x + 5], [y + 20] to account for default window decoration
            ImGui::SetCursorScreenPos(ImVec2(wndPos.x + cmd.xPos + 5, wndPos.y + cmd.yPos + 20));
            ImGui::TextColored(ImVec4(255, 0, 255, 255), "%s", cmd.text.c_str());

            hadOutput = true;
        }

        ImGui::End();
    } else {
        hadOutput = false;
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

// Thanks [https://stackoverflow.com/a/48737037]
void PatchInt(int* dst, int* src, int size)
{
    DWORD oldprotect;
    VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);
    memcpy(dst, src, size);
    VirtualProtect(dst, size, oldprotect, &oldprotect);
}
void PatchChar(char* dst, char* src, int size)
{
    DWORD oldprotect;
    VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);
    memcpy(dst, src, size);
    VirtualProtect(dst, size, oldprotect, &oldprotect);
}

[[noreturn]] VOID WINAPI ListenKeyPressDebug(){
    bool debounce = false;
    bool enabled = false;
    bool forceOpen = false;
    int lobyte = 0x000000FF;

    int* debugByte    = (int*)((baseAddr + 0x2D84B0) - exeBase); // Debug Toggle

    // Camera Lock Bytes
    char* cameraByte1 = (char*)((baseAddr + 0x9AD02) - exeBase); // should be 0x00 in debug mode - code
    char* cameraByte2 = (char*)((baseAddr + 0x9AD0C) - exeBase); // should be 0x01 in debug mode - code
    int* cameraByteA  = (int*)((baseAddr + 0x36CC60) - exeBase); // Should toggle after the camera bytes.
    int* cameraByteB  = (int*)((baseAddr + 0x36CC68) - exeBase); // Should toggle after the camera bytes.

    int one = 1;
    int zero = 0;

    while (true) {
        // Listen for keypress. (the - key beside 0 in the number strip)
        auto debugKeypress = GetAsyncKeyState(VK_OEM_MINUS) & 0x8000;
        auto forceOpenKeypress = GetAsyncKeyState(VK_F7) * 0x8000;

        // Force Debug Menu Open (for development purposes)
        if (forceOpenKeypress && !enabled) {
            // Note to self: Debounce before doing anything
            if (debounce)
                continue;
            debounce = true;

            forceOpen = !forceOpen;
            enabled = forceOpen;
            isWantDebugMenu = forceOpen;
            if (forceOpen) {
                printf("[Monokuma] Debug Menu Forced Open\n");
            } else {
                printf("[Monokuma] Debug Menu Unforced Open\n");
            }
            continue;
        }

        // Debug Menu Opened
        // todo: imgui only opens on keyup but forceopen opens on keydown?
        if (debugKeypress && !forceOpen) {
            // Note to self: Debounce before doing anything
            if (debounce)
                continue;
            debounce = true;

            // Toggle the debug menu bytes.
            *debugByte = *debugByte ^ 0x00000001;

            if ((lobyte & *debugByte) == 0x00) {
                enabled = false;
                PatchInt((int *) cameraByte1, &one, 1);
                PatchInt((int *) cameraByte2, &zero, 1);

                // Reset our camera debug byte.
                *cameraByteA = *cameraByteA ^ 0x00000001;
                *cameraByteB = *cameraByteB ^ 0x00000001;

                printf("[Monokuma] Debug Menu Closed\n");
            } else {
                enabled = true;
                PatchInt((int *) cameraByte1, &zero, 1);
                PatchInt((int *) cameraByte2, &one, 1);

                // Reset our camera debug byte.
                *cameraByteA = *cameraByteA ^ 0x00000001;
                *cameraByteB = *cameraByteB ^ 0x00000001;

                printf("[Monokuma] Debug Menu Opened\n");
            }

            continue;
        }

        debounce = false;

        // Check if we closed the debug menu in-game so we can set our state.
        if ((lobyte & *debugByte) == 0x00 && enabled && !forceOpen) {

            enabled = false;
            PatchInt((int *) cameraByte1, &one, 1);
            PatchInt((int *) cameraByte2, &zero, 1);

            // Reset our camera debug byte.
            *cameraByteA = *cameraByteA ^ 0x00000001;
            *cameraByteB = *cameraByteB ^ 0x00000001;

            printf("[Monokuma] Debug Menu Closed\n");
        }

        isWantDebugMenu = enabled;
    }
}
[[noreturn]] VOID WINAPI ListenKeyPressImguiOverlay(){
    bool debounce = false;

    while (true) {
        auto wantImgui = (GetAsyncKeyState(VK_OEM_PLUS) & 0x8000) && (GetAsyncKeyState(VK_LSHIFT) & 0x8000);
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
            CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(ListenKeyPressImguiOverlay), NULL, 0, NULL);

            // Fire and forget our d3d9 hook
            CreateThread(NULL, 0,  reinterpret_cast<LPTHREAD_START_ROUTINE>(kieroInitThread), NULL, 0, NULL);

            // Fire and forget our patcher thread
            CreateThread(NULL, 0,  reinterpret_cast<LPTHREAD_START_ROUTINE>(ApplyPatch), NULL, 0, NULL);

            printf("[Monokuma] Base addr for EXE is %Xh\n", baseAddr);
            printf("[Monokuma] EXE base is %Xh\n", exeBase);
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