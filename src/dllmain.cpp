#include "Client.h"
#include <Windows.h>

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hInstance);
        // Spawn a thread so DllMain returns immediately (Windows requirement)
        CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
            Client::getInstance().init();
            // Eject the DLL cleanly after shutdown
            FreeLibraryAndExitThread((HMODULE)GetModuleHandleW(L"PolarisClient.dll"), 0);
            return 0;
        }, nullptr, 0, nullptr);
    }
    return TRUE;
}
