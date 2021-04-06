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
bool isWantImgui = false;
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
    //std::cout << std::endl;
    auto str = std::string(buffer);

    //auto alloc = (char*)malloc(strlen(buffer)+1);
    //auto copy = strcpy_s(alloc, strlen(buffer), buffer);

    //assert(copy == 0);

    auto scr = ScreenPrintCommand{x, y, str};
    cmdBuf.push(scr);

    //printf("Command sent: [%i:%i] %s\n", scr.xPos, scr.yPos, scr.text);

    //printf("[X: %i] [Y: %i]  %s", x, y, buffer); // temp until we can get imgui/d3d screen write working
    //ImGui_ImplDX9_NewFrame();
    //ImGui_ImplWin32_NewFrame();

    //auto ovlDrw = ImGui::GetForegroundDrawList();
    //ovlDrw->AddLine(ImVec2(x, y), ImVec2(0,0), IM_COL32(255,0,0,1));
    //ovlDrw->AddText(ImVec2((float)x, (float)y), IM_COL32(0,0,255,255), "str.c_str()");
    //ovlDrw->AddText(ImVec2(x,y), IM_COL32(0,0,255,255), str.c_str());

    //auto globalDrawlist = ImGui::GetOverlayDrawList();
    //globalDrawlist->AddText(ImVec2(x, y), IM_COL32(0,0,150,1), str.c_str());
    //ImGui::Render();
    //ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

typedef void (*oConsoleStdoutPrintFunc)(const char* pattern, ...);
typedef void (*oConsoleStderrPrintFunc)(const char* pattern, ...);
typedef void (*oScreenPrintFunc)(int xPos, int yPos, const char* buffer);

oConsoleStdoutPrintFunc stdoutPrintFuncReal    = (oConsoleStdoutPrintFunc)  stdoutFuncAddr;
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
        ImGui::GetIO().MouseDrawCursor = false;
    }

    if (isWantDebugMenu) {

        //ImGui::ShowDemoWindow();
        //ImGui::GetIO().MouseDrawCursor = true;
        ImGui::Begin("Debug Menu", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        //ImGui::SetWindowPos(ImVec2(0, 0));
        //ImGui::ShowDemoWindow();
        //ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
        //imguiDrawList = ImGui::GetForegroundDrawList();
        //imguiDrawList->AddText(ImVec2(((int)0), ((int)0)), IM_COL32(0,0,255,255), "Hello ImGui screen draw!");
        //auto globalDrawlist = ImGui::GetOverlayDrawList();
        //auto wndDrw = ImGui::GetWindowDrawList();
        //auto fgDrw = ImGui::GetForegroundDrawList();
        //auto bgDrw = ImGui::GetBackgroundDrawList();
        auto test = ImGui::GetWindowDrawList();
        auto wndPos = ImGui::GetWindowPos();
        auto cmds = cmdBuf.pull();
        for (auto &cmd : cmds) {

            // [x + 5], [y + 20] to account for default window decoration
            ImGui::SetCursorScreenPos(ImVec2(wndPos.x + cmd.xPos + 5, wndPos.y + cmd.yPos + 20));

            ImGui::TextColored(ImVec4(255, 0, 255, 255), "%s", cmd.text.c_str());
            //test->AddText(ImVec2(cmd.xPos, cmd.yPos), IM_COL32(255, 0, 255, 255), cmd.text.c_str());
            //printf("Drawing [%i:%i] %s\n", cmd.xPos, cmd.yPos, cmd.text);
            //bgDrw->AddText(ImVec2(cmd.xPos, cmd.yPos), IM_COL32(255, 0, 255, 255), cmd.text.c_str());
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

// Thanks [https://stackoverflow.com/a/48737037]
void Patch(int* dst, int* src, int size)
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
    char* cameraByte1 = (char*)((baseAddr + 0x9AD02) - exeBase); // should be 0x00 in debug mode - code
    char* cameraByte2 = (char*)((baseAddr + 0x9AD0C) - exeBase); // should be 0x01 in debug mode - code
    int* cameraByteA  = (int*)((baseAddr + 0x36CC60) - exeBase); // Should toggle after the camera bytes.
    int* cameraByteB  = (int*)((baseAddr + 0x36CC68) - exeBase); // Should toggle after the camera bytes.

    int one = 1;
    int zero = 0;

    while (true) {
        // Listen for keypress. (the - key beside 0 in the number strip)
        auto keystate = GetAsyncKeyState(VK_OEM_MINUS) & 0x8000;

        if (keystate) {
            // Note to self: Debounce before doing anything
            if (debounce)
                continue;
            debounce = true;

            if (GetAsyncKeyState(VK_LSHIFT) & 0x8000) {
                forceOpen = !forceOpen;
                enabled = forceOpen;
                isWantDebugMenu = forceOpen;
                if (forceOpen){
                    printf("[Monokuma] Debug Menu Forced Open\n");
                }
                else {
                    printf("[Monokuma] Debug Menu Unforced Open\n");
                }
                continue;
            }

            if (forceOpen) {
                continue;
            }

            // Toggle the debug menu bytes.
            *debugByte = *debugByte ^ 0x00000001;


            if ((lobyte & *debugByte) == 0x00) {
                enabled = false;
                Patch((int *) cameraByte1, &one, 1);
                Patch((int *) cameraByte2, &zero, 1);

                // Reset our camera debug byte.
                *cameraByteA = *cameraByteA ^ 0x00000001;
                *cameraByteB = *cameraByteB ^ 0x00000001;

                printf("[Monokuma] Debug Menu Closed\n");
            } else {
                enabled = true;
                Patch((int *) cameraByte1, &zero, 1);
                Patch((int *) cameraByte2, &one, 1);

                // Reset our camera debug byte.
                *cameraByteA = *cameraByteA ^ 0x00000001;
                *cameraByteB = *cameraByteB ^ 0x00000001;

                printf("[Monokuma] Debug Menu Opened\n");
            }

            continue;
        } else {
            debounce = false;
        }

        if ((lobyte & *debugByte) == 0x00 && enabled) {
            if (forceOpen)
                continue;

            enabled = false;
            Patch((int*)cameraByte1, &one, 1);
            Patch((int*)cameraByte2, &zero, 1);

            // Reset our camera debug byte.
            *cameraByteA = *cameraByteA ^ 0x00000001;
            *cameraByteB = *cameraByteB ^ 0x00000001;

            printf("[Monokuma] Debug Menu Closed\n");
        }

        isWantDebugMenu = enabled;
    }
}
[[noreturn]] VOID WINAPI ListenKeyPressAddrHelper(){
    bool debounce = false;
    while (true) {
        // Listen for keypress. (the + key beside - in the number strip)
        auto addrPrint = GetAsyncKeyState(VK_OEM_PLUS) & 0x8000;

        if (addrPrint) {
            if (debounce)
                continue;

            debounce = true;

            std::cout << std::endl;
            // Just a quick little reference guide for converting VA->TA and vice versa.
            printf("\n[Monokuma] ((Base Addr + VA) - EXE Base) = Target Address\n");
            printf("[Monokuma] Base addr for EXE is %Xh\n", baseAddr);
            printf("[Monokuma] EXE base is %Xh\n", exeBase);
            printf("[Monokuma] Addr for func stdoutPrintFunc is %Xh.\n", stdoutFuncAddr);
            printf("[Monokuma] Addr for func screenPrintFunc is %Xh.\n", screenFuncAddr);
        }
        else {
            debounce = false;
        }
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

int __CLRCALL_PURE_OR_STDCALL kieroInitThread()
{
    if (kiero::init(kiero::RenderType::D3D9) == kiero::Status::Success) {
        //oEndScene = (EndScene) ;
        //oReset = (Reset) ;
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

            printf("[Monokuma] Base addr for EXE is %Xh\n", baseAddr);
            printf("[Monokuma] EXE base is %Xh\n", exeBase);
            printf("[Monokuma] Addr for func stdoutPrintFunc is %Xh.\n", stdoutFuncAddr);
            //printf("[Monokuma] Assuming base addr %X for func stderrPrintFunc.\n", (baseAddr + 0x0435B2) - exeBase);
            printf("[Monokuma] Addr for func screenPrintFunc is %Xh.\n", screenFuncAddr);
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            //DetourAttach(&(PVOID &) D3DCreate9ExReal, Direct3DCreate9ExTake);
            DetourAttach(&(PVOID &) stdoutPrintFuncReal, println);
            DetourAttach(&(PVOID &) screenPrintFuncReal, println_screen);
            //DetourAttach(&(PVOID &) Direct3DCreate9ExRealFuncA, Direct3DCreate9ExTakeParam);
            //DetourAttach(&(PVOID &) d3d9createDeviceExFunc, CreateDeviceExTakeParam);

            //DetourAttach(&(PVOID &) stderrPrintFuncReal, printerr); // Broken function, outputs garbage.
            DetourTransactionCommit();

            printf("[Monokuma] Detoured.\n");
            printf("[Monokuma] Enjoy!\n\n");
            break;

        case DLL_PROCESS_DETACH:
            DetourTransactionBegin();
            DetourUpdateThread(GetCurrentThread());
            DetourDetach(&(PVOID &) stdoutPrintFuncReal, println);
            DetourDetach(&(PVOID &) screenPrintFuncReal, println_screen);
            //DetourDetach(&(PVOID &) D3DCreate9ExReal, Direct3DCreate9ExTake);
            //DetourDetach(&(PVOID &) Direct3DCreate9ExRealFuncA, Direct3DCreate9ExTakeParam);
            //DetourDetach(&(PVOID &) d3d9createDeviceExFunc, CreateDeviceExTakeParam);

            //DetourDetach(&(PVOID &) stderrPrintFuncReal, printerr); // Broken function, outputs garbage.
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