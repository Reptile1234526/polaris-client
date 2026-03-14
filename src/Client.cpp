#include "Client.h"
#include "hooks/DXHook.h"
#include "modules/ModuleManager.h"
#include "ui/ClickGUI.h"
#include <Windows.h>
#include <thread>
#include <chrono>
#include <fstream>

// Simple log to %TEMP%\polaris.log
static std::ofstream g_log;
static void Log(const char* msg) {
    OutputDebugStringA(("[Polaris] " + std::string(msg) + "\n").c_str());
    if (!g_log.is_open())
        g_log.open("C:\\Users\\Public\\polaris.log", std::ios::out | std::ios::trunc);
    if (g_log.is_open()) { g_log << msg << std::endl; g_log.flush(); }
}

void Client::init() {
    Log("Client::init started");

    // Wait for the game to finish loading before hooking
    while (!GetModuleHandleW(L"Minecraft.Windows.exe"))
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Log("Minecraft module found, waiting 3s...");
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    Log("Initialising ModuleManager...");
    ModuleManager::get().init();

    Log("Initialising DXHook...");
    if (!DXHook::init()) {
        Log("DXHook init FAILED");
        MessageBoxA(nullptr, "Polaris: DXHook init failed.\nCheck %TEMP%\\polaris.log", "Error", MB_OK | MB_ICONERROR);
        return;
    }
    Log("DXHook init OK");

    running = true;

    // Main loop — tick modules and handle Right Shift to open GUI
    while (running) {
        auto* ci = SDK::ClientInstance::get();
        if (ci && ci->isInGame())
            ModuleManager::get().onTick(ci);

        // L key toggles the ClickGUI
        static bool lkeyWas = false;
        bool lkeyNow = (GetAsyncKeyState('L') & 0x8000) != 0;
        if (lkeyNow && !lkeyWas)
            ClickGUI::get().toggle();
        lkeyWas = lkeyNow;

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
