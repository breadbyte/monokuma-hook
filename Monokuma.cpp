#include "Monokuma.h"

#include <iostream>
#include <windows.h>
#include <detours/detours.h>

// Define our addresses.
int baseAddr = (int) GetModuleHandle(nullptr);
int exeBase = 0x30000;
int stdoutFuncAddr = ((baseAddr + 0x130B00) - exeBase);
int screenFuncAddr = ((baseAddr + 0x0435B1) - exeBase);

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
    std::cout << std::endl;
    printf("[X: %i] [Y: %i]  %s", x, y, buffer); // temp until we can get imgui/d3d screen write working
    //tagRECT r = tagRECT();
    //r.left = x;
    //r.top = y;
    //r.bottom = x + 64;
    //r.right = y + 64;

    // todo both nullptr
    //assert(D3DDATA::d3d_dev != nullptr);
    //assert(D3DDATA::pDevice != nullptr);

    //if (D3DDATA::font == nullptr) {
    //    auto retval2 = D3DXCreateFont(reinterpret_cast<LPDIRECT3DDEVICE9>(&D3DDATA::pDevice), 12, 0, FW_NORMAL, 1, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &D3DDATA::font);
    //    println("[Monokuma] Called D3DXCreateFont");
    //    switch (retval2) {
    //        case D3DERR_INVALIDCALL:
    //            assert(false);
    //            throw std::exception();
    //        case D3DXERR_INVALIDDATA:
    //            assert(false);
    //            throw std::exception();
    //        case E_OUTOFMEMORY:
    //            assert(false);
    //            throw std::exception();
    //        default:
    //            break;
    //    }
    //    println("[Monokuma] Font created");
    //}

    //auto result = D3DDATA::font->DrawTextA(NULL, buffer, strlen(buffer), &r, DT_SINGLELINE | DT_NOCLIP,
    //                                 D3DCOLOR_ARGB(1, 1, 1, 1));
    //if (result == 0) {
    //    printf("Failed to draw [X: %i] [Y: %i] [%s]", x, y, buffer);
    //}
}

typedef void (*oConsoleStdoutPrintFunc)(const char* pattern, ...);
typedef void (*oConsoleStderrPrintFunc)(const char* pattern, ...);
typedef void (*oScreenPrintFunc)(int xPos, int yPos, const char* buffer);

oConsoleStdoutPrintFunc stdoutPrintFuncReal    = (oConsoleStdoutPrintFunc)  stdoutFuncAddr;
oConsoleStderrPrintFunc stderrPrintFuncReal    = (oConsoleStderrPrintFunc)  ((baseAddr + 0x0435B2) - exeBase);
oScreenPrintFunc        screenPrintFuncReal    = (oScreenPrintFunc)         screenFuncAddr;


[[noreturn]] VOID WINAPI ListenKeyPressDebug(){
    bool debounce = false;
    int lobyte = 0x000000FF;

    while (true) {
        int *addr = (int*)((baseAddr + 0x2D84B0) - exeBase);

        // Listen for keypress. (the - key beside 0 in the number strip)
        auto keystate = GetAsyncKeyState(VK_OEM_MINUS) & 0x8000;
        if (keystate) {
            if (debounce)
                continue;

            debounce = true;

            if ((lobyte & *addr) == 0x00) {
                printf("\n[Monokuma] Debug Menu Closed");
            }
            else {
                printf("\n[Monokuma] Debug Menu Opened");
            }

            // Toggle the debug menu byte.
            *addr = *addr ^ 0x00000001;
            continue;
        }
        else {
            debounce = false;
        }
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
            printf("\n[Monokuma] ((Base Addr + Func VA) - EXE Base) = Target Address\n");
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
            D3DDATA();

            // Fire and forget our keypress threads
            {
                CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(ListenKeyPressDebug),
                             NULL, 0, NULL);
                CreateThread(NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(ListenKeyPressAddrHelper),
                             NULL, 0, NULL);
            }

            // ((Base Image + Static Func VA) - Static EXE Base) = Target Address
            printf("[Monokuma] Make sure the patch is applied, so the debug menu can show up!\n");
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
