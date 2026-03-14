// ─────────────────────────────────────────────────────────────────────────────
// Polaris Launcher — Win32 GUI launcher / injector
// Dark borderless window, sidebar navigation, async injection
// ─────────────────────────────────────────────────────────────────────────────
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

#include <Windows.h>
#include <objidl.h>   // Required for IStream when WIN32_LEAN_AND_MEAN is set
#include <gdiplus.h>
#include <dwmapi.h>
#include <TlHelp32.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <string>
#include <thread>
#include <atomic>
#include <functional>

using namespace Gdiplus;

// ── Colours ───────────────────────────────────────────────────────────────────
static const Color kBg     (255,  9,   9,  14);
static const Color kPanel  (255, 16,  16,  22);
static const Color kPanel2 (255, 22,  22,  30);
static const Color kAccent (255,110,  60, 220);
static const Color kAccentH(255,130,  80, 240);
static const Color kText   (255,220, 220, 240);
static const Color kSub    (255,120, 120, 145);
static const Color kGreen  (255, 70, 210, 120);
static const Color kRed    (255,220,  70,  70);

// ── Layout constants ──────────────────────────────────────────────────────────
static const int   WIN_W   = 780;
static const int   WIN_H   = 490;
static const int   TITLE_H = 46;
static const int   SIDE_W  = 155;

// ── State ─────────────────────────────────────────────────────────────────────
static HWND           g_hwnd        = nullptr;
static int            g_nav         = 0;   // 0 = Home, 1 = Settings
static std::wstring   g_status      = L"Ready";
static Color          g_statusCol   = kSub;
static std::atomic<bool> g_busy     = false;

// Dragging
static bool g_dragging = false;
static POINT g_dragOrigin{};
static POINT g_winOrigin{};

// Hover tracking
static int g_hoverNav = -1;
static bool g_hoverLaunch = false;
static bool g_hoverClose  = false;

// GDI+ font cache
static FontFamily* g_fontTitle  = nullptr;
static FontFamily* g_fontUI     = nullptr;

// ── Helpers ───────────────────────────────────────────────────────────────────
static std::wstring GetExeDir() {
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    PathRemoveFileSpecW(buf);
    return buf;
}

static DWORD FindPID(const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe{ sizeof(pe) };
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe))
        do { if (_wcsicmp(pe.szExeFile, name) == 0) { pid = pe.th32ProcessID; break; } }
        while (Process32NextW(snap, &pe));
    CloseHandle(snap);
    return pid;
}

static bool Inject(DWORD pid, const std::wstring& dll) {
    char dllA[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, dll.c_str(), -1, dllA, MAX_PATH, nullptr, nullptr);

    HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!proc) return false;

    size_t len  = strlen(dllA) + 1;
    LPVOID rem  = VirtualAllocEx(proc, nullptr, len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!rem) { CloseHandle(proc); return false; }

    WriteProcessMemory(proc, rem, dllA, len, nullptr);
    HANDLE th = CreateRemoteThread(proc, nullptr, 0,
        (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"),
        rem, 0, nullptr);
    if (!th) { VirtualFreeEx(proc, rem, 0, MEM_RELEASE); CloseHandle(proc); return false; }
    WaitForSingleObject(th, 8000);
    VirtualFreeEx(proc, rem, 0, MEM_RELEASE);
    CloseHandle(th);
    CloseHandle(proc);
    return true;
}

static void SetStatus(const std::wstring& msg, Color col) {
    g_status    = msg;
    g_statusCol = col;
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

// ── Launch thread ─────────────────────────────────────────────────────────────
static void DoLaunch() {
    if (g_busy.exchange(true)) return;

    DWORD pid = FindPID(L"Minecraft.Windows.exe");

    if (!pid) {
        // Tell the user to open Minecraft, then wait for it
        SetStatus(L"Open Minecraft from the Start Menu, then wait...", kSub);

        for (int i = 0; i < 600 && !pid; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            pid = FindPID(L"Minecraft.Windows.exe");
        }

        if (!pid) {
            SetStatus(L"Timed out - open Minecraft and try again", kRed);
            g_busy = false;
            return;
        }

        // Give the game a moment to initialise
        SetStatus(L"Minecraft found! Waiting for it to load...", kSub);
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }

    SetStatus(L"Injecting PolarisClient.dll...", kSub);

    std::wstring dll = GetExeDir() + L"\\PolarisClient.dll";
    if (!PathFileExistsW(dll.c_str())) {
        SetStatus(L"PolarisClient.dll not found next to launcher", kRed);
        g_busy = false;
        return;
    }

    if (Inject(pid, dll))
        SetStatus(L"Injected! Press RIGHT SHIFT in-game to open the GUI", kGreen);
    else
        SetStatus(L"Injection failed — try running as Administrator", kRed);

    g_busy = false;
}

// ── GDI+ helpers ──────────────────────────────────────────────────────────────
static void DrawRoundRect(Graphics& g, const Brush& b, float x, float y, float w, float h, float r) {
    GraphicsPath path;
    path.AddArc(x,         y,         r*2, r*2, 180, 90);
    path.AddArc(x+w-r*2,  y,         r*2, r*2, 270, 90);
    path.AddArc(x+w-r*2,  y+h-r*2,   r*2, r*2,   0, 90);
    path.AddArc(x,         y+h-r*2,   r*2, r*2,  90, 90);
    path.CloseFigure();
    g.FillPath(&b, &path);
}

static void DrawText2(Graphics& g, const std::wstring& txt, FontFamily* ff,
                      float size, int style, const Color& col,
                      float x, float y, float w, float h,
                      StringAlignment ha = StringAlignmentNear,
                      StringAlignment va = StringAlignmentCenter) {
    Font font(ff, size, style, UnitPixel);
    SolidBrush br(col);
    RectF rc(x, y, w, h);
    StringFormat sf;
    sf.SetAlignment(ha);
    sf.SetLineAlignment(va);
    g.DrawString(txt.c_str(), -1, &font, rc, &sf, &br);
}

// ── Painting ──────────────────────────────────────────────────────────────────
static void Paint(HDC hdc) {
    RECT cr; GetClientRect(g_hwnd, &cr);
    const int W = cr.right, H = cr.bottom;

    Bitmap   bmp(W, H);
    Graphics g(&bmp);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    // ── Background ───────────────────────────────────────────────────────────
    SolidBrush bgBr(kBg);
    DrawRoundRect(g, bgBr, 0, 0, (float)W, (float)H, 12.f);

    // ── Title bar ────────────────────────────────────────────────────────────
    // Accent gradient strip at very top
    {
        LinearGradientBrush grad(PointF(0, 0), PointF((float)W, 0),
                                 Color(180, 80, 30, 180), Color(0, 0, 0, 0));
        g.FillRectangle(&grad, 0, 0, W, 3);
    }

    // Logo text
    DrawText2(g, L"POLARIS", g_fontTitle, 22.f, FontStyleBold, kAccent,
              SIDE_W + 16.f, 0, 220, (float)TITLE_H);
    DrawText2(g, L"CLIENT", g_fontTitle, 22.f, FontStyleBold, kText,
              SIDE_W + 16.f + 110.f, 0, 100, (float)TITLE_H);

    // Version badge
    {
        SolidBrush bdg(Color(255, 30, 30, 40));
        DrawRoundRect(g, bdg, SIDE_W + 240.f, 14.f, 48.f, 18.f, 4.f);
        DrawText2(g, L"v0.1", g_fontUI, 11.f, FontStyleBold, kSub,
                  SIDE_W + 240.f, 14.f, 48.f, 18.f,
                  StringAlignmentCenter, StringAlignmentCenter);
    }

    // Close button
    {
        Color closecol = g_hoverClose ? kRed : kSub;
        DrawText2(g, L"X", g_fontUI, 14.f, FontStyleBold, closecol,
                  (float)(W - 36), 0, 30.f, (float)TITLE_H,
                  StringAlignmentCenter, StringAlignmentCenter);
    }

    // Title bar bottom border
    {
        SolidBrush sep(Color(255, 30, 30, 40));
        g.FillRectangle(&sep, SIDE_W, TITLE_H - 1, W - SIDE_W, 1);
    }

    // ── Sidebar ───────────────────────────────────────────────────────────────
    {
        SolidBrush sideBr(kPanel);
        DrawRoundRect(g, sideBr, 0, 0, (float)SIDE_W, (float)H, 12.f);
        // Fill right side of sidebar roundrect to make it flat on right
        g.FillRectangle(&sideBr, (float)(SIDE_W - 12), 0.f, 12.f, (float)H);
    }

    // Polaris logo mark in sidebar
    {
        // Glowing circle
        for (int i = 3; i >= 1; i--) {
            SolidBrush glowBr(Color((BYTE)(20 * i), 110, 60, 220));
            float r = 28.f + i * 4.f;
            g.FillEllipse(&glowBr, SIDE_W / 2.f - r, 20.f - r + 36.f, r * 2, r * 2);
        }
        SolidBrush cirBr(kAccent);
        g.FillEllipse(&cirBr, SIDE_W / 2.f - 24.f, 12.f, 48.f, 48.f);
        DrawText2(g, L"P", g_fontTitle, 26.f, FontStyleBold, kText,
                  0, 12.f, (float)SIDE_W, 48.f,
                  StringAlignmentCenter, StringAlignmentCenter);
    }

    // Nav items
    const wchar_t* navLabels[] = { L"Home", L"Settings" };
    const wchar_t* navIcons[]  = { L"H", L"S" };
    for (int i = 0; i < 2; i++) {
        float ny = 90.f + i * 50.f;
        bool  sel = g_nav == i;
        bool  hov = g_hoverNav == i;

        if (sel) {
            // Accent left bar
            SolidBrush abar(kAccent);
            DrawRoundRect(g, abar, 0, ny + 6.f, 4.f, 32.f, 2.f);
            // Highlight bg
            SolidBrush hbg(Color(60, 110, 60, 220));
            DrawRoundRect(g, hbg, 8.f, ny + 2.f, SIDE_W - 16.f, 40.f, 6.f);
        } else if (hov) {
            SolidBrush hbg(Color(30, 255, 255, 255));
            DrawRoundRect(g, hbg, 8.f, ny + 2.f, SIDE_W - 16.f, 40.f, 6.f);
        }

        Color tc = sel ? kAccent : (hov ? kText : kSub);
        DrawText2(g, navIcons[i], g_fontUI, 15.f, FontStyleRegular, tc,
                  16.f, ny, 30.f, 44.f,
                  StringAlignmentCenter, StringAlignmentCenter);
        DrawText2(g, navLabels[i], g_fontUI, 13.f, FontStyleRegular, tc,
                  50.f, ny, SIDE_W - 60.f, 44.f,
                  StringAlignmentNear, StringAlignmentCenter);
    }

    // Sidebar bottom: discord/close hint
    DrawText2(g, L"Press INSERT in-game to unload",
              g_fontUI, 9.5f, FontStyleRegular, kSub,
              4.f, (float)(H - 30), (float)SIDE_W - 8.f, 24.f,
              StringAlignmentCenter, StringAlignmentCenter);

    // ── Content area ──────────────────────────────────────────────────────────
    const float CX = (float)SIDE_W + 12.f;
    const float CY = (float)TITLE_H + 12.f;
    const float CW = W - SIDE_W - 24.f;
    const float CH = H - TITLE_H - 24.f;

    if (g_nav == 0) {
        // ── Home ─────────────────────────────────────────────────────────────

        // Status card
        {
            SolidBrush card(kPanel);
            DrawRoundRect(g, card, CX, CY, CW, 64.f, 8.f);

            // Status dot
            SolidBrush dot(g_statusCol);
            g.FillEllipse(&dot, CX + 18.f, CY + 22.f, 12.f, 12.f);

            DrawText2(g, L"Status", g_fontUI, 11.f, FontStyleBold, kSub,
                      CX + 40.f, CY, CW - 50.f, 32.f,
                      StringAlignmentNear, StringAlignmentFar);
            DrawText2(g, g_status, g_fontUI, 13.f, FontStyleRegular, g_statusCol,
                      CX + 40.f, CY + 32.f, CW - 50.f, 28.f,
                      StringAlignmentNear, StringAlignmentCenter);
        }

        // Features card
        {
            float fy = CY + 76.f;
            SolidBrush card(kPanel);
            DrawRoundRect(g, card, CX, fy, CW, 178.f, 8.f);

            DrawText2(g, L"FEATURES", g_fontUI, 10.f, FontStyleBold, kAccent,
                      CX + 16.f, fy + 12.f, 200.f, 20.f);

            const wchar_t* feats[] = {
                L"*  HUD modules - CPS, Coords, Ping, Armor, Keystrokes",
                L"*  Visual - Fullbright, Hitbox, Block Outline, Zoom",
                L"*  Utility - Fast Inventory, Drop Inventory, VSync Disabler",
                L"*  Click GUI  (Right Shift in-game)",
                L"*  Per-module keybinds & settings",
            };
            for (int i = 0; i < 5; i++) {
                DrawText2(g, feats[i], g_fontUI, 11.5f, FontStyleRegular, kText,
                          CX + 16.f, fy + 36.f + i * 26.f, CW - 32.f, 24.f,
                          StringAlignmentNear, StringAlignmentCenter);
            }
        }

        // ── Launch button ────────────────────────────────────────────────────
        {
            float bx = CX + (CW - 280.f) / 2.f;
            float by = CY + 270.f;
            Color bc = g_busy ? kSub : (g_hoverLaunch ? kAccentH : kAccent);

            // Outer glow
            if (!g_busy && g_hoverLaunch) {
                for (int i = 3; i >= 1; i--) {
                    SolidBrush gb(Color((BYTE)(18 * i), 110, 60, 220));
                    DrawRoundRect(g, gb, bx - i * 3.f, by - i * 3.f,
                                  280.f + i * 6.f, 52.f + i * 6.f, 10.f + i * 2.f);
                }
            }

            SolidBrush btnBr(bc);
            DrawRoundRect(g, btnBr, bx, by, 280.f, 52.f, 8.f);

            const wchar_t* label = g_busy ? L"Working..." : L"Inject into Minecraft";
            DrawText2(g, label, g_fontUI, 14.f, FontStyleBold, kText,
                      bx, by, 280.f, 52.f,
                      StringAlignmentCenter, StringAlignmentCenter);
        }

        // Hint below button
        DrawText2(g, L"Open Minecraft first, then click the button",
                  g_fontUI, 10.f, FontStyleRegular, kSub,
                  CX, CY + 330.f, CW, 20.f,
                  StringAlignmentCenter, StringAlignmentCenter);

    } else {
        // ── Settings ─────────────────────────────────────────────────────────
        SolidBrush card(kPanel);
        DrawRoundRect(g, card, CX, CY, CW, 100.f, 8.f);

        DrawText2(g, L"SETTINGS", g_fontUI, 10.f, FontStyleBold, kAccent,
                  CX + 16.f, CY + 12.f, 200.f, 20.f);
        DrawText2(g, L"DLL path:  <same folder as launcher>\\PolarisClient.dll",
                  g_fontUI, 11.5f, FontStyleRegular, kText,
                  CX + 16.f, CY + 38.f, CW - 32.f, 24.f);
        DrawText2(g, L"Game process:  Minecraft.Windows.exe",
                  g_fontUI, 11.5f, FontStyleRegular, kText,
                  CX + 16.f, CY + 64.f, CW - 32.f, 24.f);
    }

    // ── Blit to screen ────────────────────────────────────────────────────────
    Graphics screen(hdc);
    screen.DrawImage(&bmp, 0, 0);
}

// ── Hit-test helpers ──────────────────────────────────────────────────────────
static RECT LaunchBtnRect() {
    float CX = (float)SIDE_W + 12.f;
    float CY = (float)TITLE_H + 12.f;
    float CW = WIN_W - SIDE_W - 24.f;
    float bx = CX + (CW - 280.f) / 2.f;
    float by = CY + 270.f;
    return { (LONG)bx, (LONG)by, (LONG)(bx + 280), (LONG)(by + 52) };
}

static RECT CloseRect()  { return { WIN_W - 36, 0, WIN_W, TITLE_H }; }
static RECT NavRect(int i) {
    float ny = 90.f + i * 50.f;
    return { 0, (LONG)(ny + 2), SIDE_W, (LONG)(ny + 42) };
}

static bool InRect(RECT r, int x, int y) {
    return x >= r.left && x < r.right && y >= r.top && y < r.bottom;
}

// ── Window Procedure ──────────────────────────────────────────────────────────
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    int mx = LOWORD(lParam), my = HIWORD(lParam);

    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
        Paint(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;

    case WM_NCHITTEST: {
        POINT pt{ (short)LOWORD(lParam), (short)HIWORD(lParam) };
        ScreenToClient(hwnd, &pt);
        // Close button
        if (InRect(CloseRect(), pt.x, pt.y)) return HTCLIENT;
        // Title bar drag (above content, not in sidebar nav)
        if (pt.y < TITLE_H) return HTCAPTION;
        return HTCLIENT;
    }

    case WM_MOUSEMOVE: {
        bool dirty = false;

        bool hc = InRect(CloseRect(), mx, my);
        if (hc != g_hoverClose)  { g_hoverClose  = hc;  dirty = true; }

        bool hl = InRect(LaunchBtnRect(), mx, my) && g_nav == 0;
        if (hl != g_hoverLaunch) { g_hoverLaunch = hl;  dirty = true; }

        int hn = -1;
        for (int i = 0; i < 2; i++) if (InRect(NavRect(i), mx, my)) { hn = i; break; }
        if (hn != g_hoverNav)    { g_hoverNav    = hn;  dirty = true; }

        if (dirty) InvalidateRect(hwnd, nullptr, FALSE);

        // Track mouse leave
        TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
        return 0;
    }
    case WM_MOUSELEAVE:
        g_hoverClose = g_hoverLaunch = false; g_hoverNav = -1;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_LBUTTONDOWN: {
        if (InRect(CloseRect(), mx, my)) { PostQuitMessage(0); return 0; }

        for (int i = 0; i < 2; i++) {
            if (InRect(NavRect(i), mx, my)) {
                g_nav = i;
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }
        }

        if (g_nav == 0 && InRect(LaunchBtnRect(), mx, my) && !g_busy) {
            std::thread(DoLaunch).detach();
            return 0;
        }
        return 0;
    }

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) PostQuitMessage(0);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ── Entry point ───────────────────────────────────────────────────────────────
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int) {
    // Init GDI+
    GdiplusStartupInput gsi;
    ULONG_PTR           gToken;
    GdiplusStartup(&gToken, &gsi, nullptr);

    g_fontTitle = new FontFamily(L"Segoe UI");
    g_fontUI    = new FontFamily(L"Segoe UI");

    // Register window class
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"PolarisLauncher";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassExW(&wc);

    // Centre window on screen
    int sx = GetSystemMetrics(SM_CXSCREEN);
    int sy = GetSystemMetrics(SM_CYSCREEN);
    int wx = (sx - WIN_W) / 2, wy = (sy - WIN_H) / 2;

    g_hwnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        L"PolarisLauncher", L"Polaris Launcher",
        WS_POPUP | WS_VISIBLE,
        wx, wy, WIN_W, WIN_H,
        nullptr, nullptr, hInst, nullptr);

    // Round the window corners via DWM
    DWMNCRENDERINGPOLICY ncrp = DWMNCRP_ENABLED;
    DwmSetWindowAttribute(g_hwnd, DWMWA_NCRENDERING_POLICY, &ncrp, sizeof(ncrp));
    BOOL useDark = TRUE;
    DwmSetWindowAttribute(g_hwnd, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &useDark, sizeof(useDark));

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    delete g_fontTitle;
    delete g_fontUI;
    GdiplusShutdown(gToken);
    return 0;
}
