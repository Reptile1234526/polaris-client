#include <Windows.h>
#include <TlHelp32.h>
#include <CommDlg.h>
#include <string>
#include <dwmapi.h>

// ── Control IDs ──────────────────────────────────────────────────────────────
#define IDC_DLL_PATH  101
#define IDC_BROWSE    102
#define IDC_INJECT    103
#define IDC_STATUS    104
#define IDC_TITLE     105

// ── Theme colours (matches Polaris SMP / ClickGUI palette) ───────────────────
static const COLORREF BG       = RGB(0x03, 0x03, 0x11);   // #030311 deep navy
static const COLORREF BG_CARD  = RGB(0x10, 0x10, 0x28);   // slightly lighter card
static const COLORREF ACCENT   = RGB(0x81, 0x8C, 0xF8);   // #818cf8 indigo
static const COLORREF TEXT     = RGB(0xE2, 0xE8, 0xF0);   // #e2e8f0
static const COLORREF MUTED    = RGB(0x94, 0xA3, 0xB8);   // #94a3b8
static const COLORREF BTN_BG   = RGB(0x25, 0x1E, 0x52);   // dark indigo button
static const COLORREF BTN_HOV  = RGB(0x35, 0x2A, 0x72);

// ── GDI resources (created once) ─────────────────────────────────────────────
static HBRUSH hBrBg    = nullptr;
static HBRUSH hBrCard  = nullptr;
static HBRUSH hBrBtn   = nullptr;
static HBRUSH hBrBtnH  = nullptr;
static HFONT  hFontBig = nullptr;
static HFONT  hFontMed = nullptr;
static HFONT  hFontSm  = nullptr;

static bool   s_btnHover = false;   // tracks hover on Inject button

// ── Find Minecraft.Windows.exe ───────────────────────────────────────────────
static DWORD FindMinecraft() {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe{ sizeof(pe) };
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"Minecraft.Windows.exe") == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

// ── LoadLibraryW injection ────────────────────────────────────────────────────
static bool Inject(const std::wstring& dll, DWORD pid) {
    HANDLE hProc = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE,
        FALSE, pid);
    if (!hProc) return false;

    size_t  sz  = (dll.size() + 1) * sizeof(wchar_t);
    LPVOID  mem = VirtualAllocEx(hProc, nullptr, sz,
                                  MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!mem) { CloseHandle(hProc); return false; }

    if (!WriteProcessMemory(hProc, mem, dll.c_str(), sz, nullptr)) {
        VirtualFreeEx(hProc, mem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }

    auto loadLib = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));

    HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0, loadLib, mem, 0, nullptr);
    if (!hThread) {
        VirtualFreeEx(hProc, mem, 0, MEM_RELEASE);
        CloseHandle(hProc);
        return false;
    }

    WaitForSingleObject(hThread, 6000);
    VirtualFreeEx(hProc, mem, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProc);
    return true;
}

// ── Draw a rounded rectangle in GDI (approximated via regions) ───────────────
static void FillRoundRect(HDC dc, const RECT& r, int radius, COLORREF col) {
    HRGN rgn = CreateRoundRectRgn(r.left, r.top, r.right, r.bottom, radius, radius);
    HBRUSH br = CreateSolidBrush(col);
    FillRgn(dc, rgn, br);
    DeleteObject(br);
    DeleteObject(rgn);
}

// ── Browse for DLL ────────────────────────────────────────────────────────────
static void DoBrowse(HWND hwnd) {
    wchar_t buf[MAX_PATH] = {};
    OPENFILENAMEW ofn{ sizeof(ofn) };
    ofn.hwndOwner   = hwnd;
    ofn.lpstrFile   = buf;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrFilter = L"DLL Files\0*.dll\0All Files\0*.*\0";
    ofn.lpstrTitle  = L"Select Polaris DLL";
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (GetOpenFileNameW(&ofn))
        SetDlgItemTextW(hwnd, IDC_DLL_PATH, buf);
}

// ── Do the injection and update status ───────────────────────────────────────
static void DoInject(HWND hwnd) {
    wchar_t buf[MAX_PATH] = {};
    GetDlgItemTextW(hwnd, IDC_DLL_PATH, buf, MAX_PATH);

    if (!buf[0]) {
        SetDlgItemTextW(hwnd, IDC_STATUS, L"Select a DLL first.");
        return;
    }
    DWORD pid = FindMinecraft();
    if (!pid) {
        SetDlgItemTextW(hwnd, IDC_STATUS,
            L"Minecraft not found — launch it first.");
        return;
    }
    SetDlgItemTextW(hwnd, IDC_STATUS, L"Injecting...");
    UpdateWindow(hwnd);

    if (Inject(buf, pid))
        SetDlgItemTextW(hwnd, IDC_STATUS,
            L"Injected! Press Right Shift in-game to open the menu.");
    else
        SetDlgItemTextW(hwnd, IDC_STATUS,
            L"Failed — try running as Administrator.");
}

// ── Subclass proc for the edit box (dark background) ─────────────────────────
static WNDPROC g_origEditProc = nullptr;
static LRESULT CALLBACK EditSubclassProc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_ERASEBKGND) {
        HDC dc = (HDC)wp;
        RECT r; GetClientRect(h, &r);
        FillRect(dc, &r, hBrCard);
        return 1;
    }
    return CallWindowProcW(g_origEditProc, h, msg, wp, lp);
}

// ── Main window procedure ─────────────────────────────────────────────────────
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {

    case WM_CREATE: {
        // Edit (DLL path)
        HWND hEdit = CreateWindowExW(WS_EX_CLIENTEDGE,
            L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            20, 80, 300, 28,
            hwnd, (HMENU)IDC_DLL_PATH, nullptr, nullptr);
        SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFontSm, TRUE);

        // Subclass edit for dark bg
        g_origEditProc = (WNDPROC)SetWindowLongPtrW(
            hEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);

        // Browse button
        HWND hBrowse = CreateWindowW(L"BUTTON", L"Browse…",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            328, 80, 72, 28,
            hwnd, (HMENU)IDC_BROWSE, nullptr, nullptr);
        SendMessageW(hBrowse, WM_SETFONT, (WPARAM)hFontSm, TRUE);

        // Inject button
        HWND hInject = CreateWindowW(L"BUTTON", L"INJECT",
            WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
            20, 124, 380, 38,
            hwnd, (HMENU)IDC_INJECT, nullptr, nullptr);
        SendMessageW(hInject, WM_SETFONT, (WPARAM)hFontMed, TRUE);

        // Status label
        HWND hStatus = CreateWindowW(L"STATIC", L"Select a DLL and click Inject.",
            WS_CHILD | WS_VISIBLE | SS_CENTER,
            20, 172, 380, 20,
            hwnd, (HMENU)IDC_STATUS, nullptr, nullptr);
        SendMessageW(hStatus, WM_SETFONT, (WPARAM)hFontSm, TRUE);

        // Enable dark title bar (Windows 10 1809+)
        BOOL dark = TRUE;
        DwmSetWindowAttribute(hwnd,
            DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
        break;
    }

    case WM_ERASEBKGND: {
        HDC dc = (HDC)wp;
        RECT r; GetClientRect(hwnd, &r);
        FillRect(dc, &r, hBrBg);
        return 1;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);

        // Logo bar at top
        RECT top = { 0, 0, 420, 64 };
        FillRect(dc, &top, hBrCard);

        // Accent line under logo bar
        RECT line = { 0, 62, 420, 64 };
        HBRUSH acBr = CreateSolidBrush(ACCENT);
        FillRect(dc, &line, acBr);
        DeleteObject(acBr);

        // Title text
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, TEXT);
        SelectObject(dc, hFontBig);
        RECT titleR = { 20, 14, 280, 52 };
        DrawTextW(dc, L"Polaris Injector", -1, &titleR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        // Version tag
        SelectObject(dc, hFontSm);
        SetTextColor(dc, MUTED);
        RECT verR = { 280, 14, 400, 52 };
        DrawTextW(dc, L"v1.0", -1, &verR, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

        // "DLL Path" label
        SetTextColor(dc, MUTED);
        SelectObject(dc, hFontSm);
        RECT labelR = { 20, 58, 200, 76 };
        DrawTextW(dc, L"DLL PATH", -1, &labelR, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

        EndPaint(hwnd, &ps);
        break;
    }

    // Colour subcontrols
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORSTATIC: {
        HDC dc = (HDC)wp;
        SetBkColor(dc, BG_CARD);
        SetTextColor(dc, TEXT);
        return (LRESULT)hBrCard;
    }
    case WM_CTLCOLORBTN: {
        HDC dc = (HDC)wp;
        SetBkColor(dc, BG);
        return (LRESULT)hBrBg;
    }

    // Owner-draw Inject button
    case WM_DRAWITEM: {
        auto* di = reinterpret_cast<DRAWITEMSTRUCT*>(lp);
        if (di->CtlID != IDC_INJECT) break;
        bool hover   = (di->itemState & ODS_HOTLIGHT) || s_btnHover;
        bool pressed = (di->itemState & ODS_SELECTED);

        COLORREF bg = pressed ? ACCENT : (hover ? BTN_HOV : BTN_BG);
        RECT r = di->rcItem;
        FillRoundRect(di->hDC, r, 8, bg);

        // Subtle border
        HPEN pen = CreatePen(PS_SOLID, 1, ACCENT);
        HPEN old = (HPEN)SelectObject(di->hDC, pen);
        HBRUSH nobr = (HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH oldbr = (HBRUSH)SelectObject(di->hDC, nobr);
        RoundRect(di->hDC, r.left, r.top, r.right, r.bottom, 8, 8);
        SelectObject(di->hDC, old);
        SelectObject(di->hDC, oldbr);
        DeleteObject(pen);

        SetBkMode(di->hDC, TRANSPARENT);
        SetTextColor(di->hDC, pressed ? BG : TEXT);
        SelectObject(di->hDC, hFontMed);
        DrawTextW(di->hDC, L"INJECT", -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return TRUE;
    }

    // Hover tracking for Inject button glow
    case WM_MOUSEMOVE: {
        HWND hInj = GetDlgItem(hwnd, IDC_INJECT);
        RECT r; GetWindowRect(hInj, &r);
        POINT pt{ LOWORD(lp), HIWORD(lp) };
        ClientToScreen(hwnd, &pt);
        bool over = PtInRect(&r, pt);
        if (over != s_btnHover) {
            s_btnHover = over;
            InvalidateRect(hInj, nullptr, FALSE);
        }
        break;
    }

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_BROWSE: DoBrowse(hwnd); break;
        case IDC_INJECT: DoInject(hwnd); break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ── Entry point ───────────────────────────────────────────────────────────────
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int) {
    // Create GDI resources
    hBrBg   = CreateSolidBrush(BG);
    hBrCard = CreateSolidBrush(BG_CARD);
    hBrBtn  = CreateSolidBrush(BTN_BG);
    hBrBtnH = CreateSolidBrush(BTN_HOV);

    hFontBig = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hFontMed = CreateFontW(16, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    hFontSm  = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    // Register window class
    WNDCLASSEXW wc{ sizeof(wc) };
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = hBrBg;
    wc.lpszClassName = L"PolarisInjector";
    wc.hIcon         = LoadIconW(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    // Fixed-size window (420 × 210)
    HWND hwnd = CreateWindowExW(0,
        L"PolarisInjector", L"Polaris Injector",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 420, 220,
        nullptr, nullptr, hInst, nullptr);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Cleanup
    DeleteObject(hBrBg);  DeleteObject(hBrCard);
    DeleteObject(hBrBtn); DeleteObject(hBrBtnH);
    DeleteObject(hFontBig); DeleteObject(hFontMed); DeleteObject(hFontSm);
    return static_cast<int>(msg.wParam);
}
