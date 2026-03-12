#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
// Polaris Injector — injects PolarisClient.dll into Minecraft.Windows.exe
// ─────────────────────────────────────────────────────────────────────────────

DWORD findProcessId(const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe = { sizeof(pe) };
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, name) == 0) { pid = pe.th32ProcessID; break; }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

bool inject(DWORD pid, const char* dllPath) {
    HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!proc) { std::cerr << "OpenProcess failed: " << GetLastError() << "\n"; return false; }

    size_t pathLen = strlen(dllPath) + 1;
    LPVOID remote  = VirtualAllocEx(proc, nullptr, pathLen, MEM_COMMIT | MEM_RESERVE,
                                    PAGE_READWRITE);
    if (!remote) { CloseHandle(proc); return false; }

    WriteProcessMemory(proc, remote, dllPath, pathLen, nullptr);

    HANDLE thread = CreateRemoteThread(proc, nullptr, 0,
        (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"),
        remote, 0, nullptr);

    if (!thread) { VirtualFreeEx(proc, remote, 0, MEM_RELEASE); CloseHandle(proc); return false; }

    WaitForSingleObject(thread, 5000);
    VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
    CloseHandle(thread);
    CloseHandle(proc);
    return true;
}

int main() {
    std::cout << "Polaris Client Injector\n";
    std::cout << "Waiting for Minecraft.Windows.exe";

    DWORD pid = 0;
    while (!pid) {
        pid = findProcessId(L"Minecraft.Windows.exe");
        if (!pid) { std::cout << "."; Sleep(1000); }
    }
    std::cout << "\nFound Minecraft! PID: " << pid << "\n";

    // DLL must be in the same directory as this injector
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string dir = std::string(exePath);
    dir = dir.substr(0, dir.find_last_of("\\/"));
    std::string dllPath = dir + "\\PolarisClient.dll";

    std::cout << "Injecting: " << dllPath << "\n";

    if (inject(pid, dllPath.c_str())) {
        std::cout << "Injected! Press RIGHT SHIFT in-game to open the GUI.\n";
        std::cout << "Press INSERT to unload.\n";
    } else {
        std::cerr << "Injection failed.\n";
    }

    std::cout << "Press Enter to exit...";
    std::cin.get();
    return 0;
}
