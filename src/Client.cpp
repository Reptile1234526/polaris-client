#include "Client.h"
#include "hooks/DXHook.h"
#include "modules/ModuleManager.h"
#include "ui/ClickGUI.h"
#include <Windows.h>
#include <thread>
#include <chrono>

void Client::init() {
    // Wait for the game to finish loading before hooking
    while (!GetModuleHandleW(L"Minecraft.Windows.exe"))
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    ModuleManager::get().init();

    if (!DXHook::init()) {
        MessageBoxA(nullptr, "Polaris: DXHook init failed.", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    running = true;

    // Main loop — tick modules and handle Right Shift to open GUI
    while (running) {
        auto* ci = SDK::ClientInstance::get();
        if (ci && ci->isInGame())
            ModuleManager::get().onTick(ci);

        // Right Shift toggles the ClickGUI
        static bool rshiftWas = false;
        bool rshiftNow = (GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0;
        if (rshiftNow && !rshiftWas)
            ClickGUI::get().toggle();
        rshiftWas = rshiftNow;

        // INSERT key unloads the client (eject)
        if (GetAsyncKeyState(VK_INSERT) & 0x8000) {
            running = false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    shutdown();
}

void Client::shutdown() {
    DXHook::shutdown();
}
