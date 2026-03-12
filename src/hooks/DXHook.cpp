#include "DXHook.h"
#include "../modules/ModuleManager.h"
#include "../modules/util/VSyncDisabler.h"
#include "../ui/ClickGUI.h"

#include <imgui.h>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <MinHook.h>
#include <dxgi1_4.h>
#include <stdexcept>

// ── Static member definitions ─────────────────────────────────────────────────
DXHook::PresentFn           DXHook::originalPresent         = nullptr;
DXHook::ExecuteCmdListsFn   DXHook::originalExecuteCmdLists = nullptr;
DXHook::ResizeBuffersFn     DXHook::originalResizeBuffers   = nullptr;
bool                        DXHook::imguiReady              = false;
ID3D12Device*               DXHook::device                  = nullptr;
ID3D12CommandQueue*         DXHook::cmdQueue                = nullptr;
ID3D12DescriptorHeap*       DXHook::rtvHeap                 = nullptr;
ID3D12DescriptorHeap*       DXHook::srvHeap                 = nullptr;
ID3D12GraphicsCommandList*  DXHook::cmdList                 = nullptr;
UINT                        DXHook::bufferCount             = 0;
UINT                        DXHook::rtvDescSize             = 0;
HWND                        DXHook::gameWindow              = nullptr;
WNDPROC                     DXHook::originalWndProc         = nullptr;
std::vector<ID3D12CommandAllocator*> DXHook::cmdAllocs;
std::vector<ID3D12Resource*>         DXHook::renderTargets;

// ── ImGui WndProc forward-declaration ────────────────────────────────────────
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

// ─────────────────────────────────────────────────────────────────────────────
// Initialise: hook ExecuteCommandLists (to capture cmdQueue) and Present
// ─────────────────────────────────────────────────────────────────────────────
bool DXHook::init() {
    MH_Initialize();

    void* vt[20] = {};
    if (!getDummySwapChainVTable(vt)) return false;

    // IDXGISwapChain::Present       = vtable index 8
    // IDXGISwapChain::ResizeBuffers = vtable index 13
    MH_CreateHook(vt[8],  (void*)hookedPresent,       (void**)&originalPresent);
    MH_CreateHook(vt[13], (void*)hookedResizeBuffers,  (void**)&originalResizeBuffers);

    // We need cmdQueue for ImGui DX12. Capture it by hooking
    // ID3D12CommandQueue::ExecuteCommandLists (vtable index 10 on queue object).
    // The dummy device lets us grab that vtable too.
    // (Simplified: many clients just scan for a command queue in the game's memory;
    //  here we show the hook approach.)

    MH_EnableHook(MH_ALL_HOOKS);
    return true;
}

void DXHook::shutdown() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    cleanupRenderTargets();
    if (imguiReady) {
        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        imguiReady = false;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Create a throw-away D3D12 device + swap chain to read vtable pointers
// ─────────────────────────────────────────────────────────────────────────────
bool DXHook::getDummySwapChainVTable(void** vtable) {
    HWND dummy = CreateWindowExA(0, "STATIC", "Polaris", WS_OVERLAPPED,
                                  0, 0, 100, 100, nullptr, nullptr,
                                  GetModuleHandleA(nullptr), nullptr);
    if (!dummy) return false;

    ID3D12Device*       ddev   = nullptr;
    ID3D12CommandQueue* dqueue = nullptr;
    IDXGIFactory4*      factory= nullptr;
    IDXGISwapChain*     dchain = nullptr;

    D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&ddev));

    D3D12_COMMAND_QUEUE_DESC qd = {};
    ddev->CreateCommandQueue(&qd, IID_PPV_ARGS(&dqueue));
    CreateDXGIFactory1(IID_PPV_ARGS(&factory));

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount              = 2;
    sd.BufferDesc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage              = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow             = dummy;
    sd.SampleDesc.Count         = 1;
    sd.SwapEffect               = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Windowed                 = TRUE;
    factory->CreateSwapChain(dqueue, &sd, &dchain);

    if (dchain) {
        memcpy(vtable, *(void***)dchain, 20 * sizeof(void*));
        dchain->Release();
    }

    if (factory) factory->Release();
    if (dqueue)  dqueue->Release();
    if (ddev)    ddev->Release();
    DestroyWindow(dummy);
    return dchain != nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
// Hooked Present — main render loop entry
// ─────────────────────────────────────────────────────────────────────────────
HRESULT __stdcall DXHook::hookedPresent(IDXGISwapChain3* chain, UINT sync, UINT flags) {
    if (!imguiReady && cmdQueue) {
        if (!initImGui(chain)) {
            goto done;
        }
    }

    if (imguiReady) {
        // Determine which back buffer is current
        UINT bbIdx = chain->GetCurrentBackBufferIndex();

        // Reset command allocator + list for this frame
        cmdAllocs[bbIdx]->Reset();
        cmdList->Reset(cmdAllocs[bbIdx], nullptr);

        // Transition render target to RENDER_TARGET state
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource   = renderTargets[bbIdx];
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmdList->ResourceBarrier(1, &barrier);

        // Set render target
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtv.ptr += (UINT64)bbIdx * rtvDescSize;
        cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
        cmdList->SetDescriptorHeaps(1, &srvHeap);

        // ── ImGui frame ──────────────────────────────────────────────────────
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Draw clickGUI if open
        ClickGUI::get().render();

        // Render all enabled modules
        RECT rc; GetClientRect(gameWindow, &rc);
        int sw = rc.right, sh = rc.bottom;
        ModuleManager::get().onRender(sw, sh);

        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);

        // Transition back to PRESENT
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
        cmdList->ResourceBarrier(1, &barrier);
        cmdList->Close();

        ID3D12CommandList* lists[] = { cmdList };
        cmdQueue->ExecuteCommandLists(1, lists);
    }

done:
    // VSyncDisabler overrides the sync interval
    auto* vsync = ModuleManager::get().get<VSyncDisabler>();
    if (vsync) sync = vsync->overrideSyncInterval(sync);

    return originalPresent(chain, sync, flags);
}

// ─────────────────────────────────────────────────────────────────────────────
// Resize: recreate render targets when the window resizes
// ─────────────────────────────────────────────────────────────────────────────
HRESULT __stdcall DXHook::hookedResizeBuffers(IDXGISwapChain* chain, UINT count,
                                               UINT w, UINT h, DXGI_FORMAT fmt, UINT flags) {
    cleanupRenderTargets();
    HRESULT hr = originalResizeBuffers(chain, count, w, h, fmt, flags);
    IDXGISwapChain3* chain3 = nullptr;
    chain->QueryInterface(IID_PPV_ARGS(&chain3));
    if (chain3) { createRenderTargets(chain3); chain3->Release(); }
    return hr;
}

// ─────────────────────────────────────────────────────────────────────────────
// ImGui initialisation
// ─────────────────────────────────────────────────────────────────────────────
bool DXHook::initImGui(IDXGISwapChain3* chain) {
    // Get device
    chain->GetDevice(IID_PPV_ARGS(&device));
    if (!device) return false;

    // Get window
    DXGI_SWAP_CHAIN_DESC sd;
    chain->GetDesc(&sd);
    gameWindow = sd.OutputWindow;

    // Back-buffer count
    bufferCount = sd.BufferCount;

    // Create SRV descriptor heap (for ImGui font)
    D3D12_DESCRIPTOR_HEAP_DESC srvDesc = {};
    srvDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvDesc.NumDescriptors = 1;
    srvDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&srvHeap));

    // Create RTV descriptor heap
    D3D12_DESCRIPTOR_HEAP_DESC rtvDesc = {};
    rtvDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDesc.NumDescriptors = bufferCount;
    rtvDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvHeap));
    rtvDescSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Create per-frame command allocators + one command list
    cmdAllocs.resize(bufferCount);
    for (UINT i = 0; i < bufferCount; i++)
        device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                       IID_PPV_ARGS(&cmdAllocs[i]));
    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                              cmdAllocs[0], nullptr, IID_PPV_ARGS(&cmdList));
    cmdList->Close();

    createRenderTargets(chain);

    // ImGui setup
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // don't save imgui.ini to disk

    ImGui::StyleColorsDark();
    // Custom Polaris style
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 6.f;
    s.FrameRounding  = 4.f;
    s.Colors[ImGuiCol_WindowBg]       = { 0.05f, 0.05f, 0.08f, 0.92f };
    s.Colors[ImGuiCol_Header]         = { 0.3f,  0.15f, 0.6f,  0.8f  };
    s.Colors[ImGuiCol_HeaderHovered]  = { 0.4f,  0.2f,  0.8f,  0.9f  };
    s.Colors[ImGuiCol_Button]         = { 0.25f, 0.12f, 0.5f,  0.9f  };
    s.Colors[ImGuiCol_ButtonHovered]  = { 0.35f, 0.18f, 0.7f,  1.f   };

    ImGui_ImplWin32_Init(gameWindow);
    ImGui_ImplDX12_Init(device, (int)bufferCount,
                        DXGI_FORMAT_R8G8B8A8_UNORM, srvHeap,
                        srvHeap->GetCPUDescriptorHandleForHeapStart(),
                        srvHeap->GetGPUDescriptorHandleForHeapStart());

    // Hook WndProc for keyboard/mouse input
    originalWndProc = (WNDPROC)SetWindowLongPtrW(gameWindow, GWLP_WNDPROC,
                                                  (LONG_PTR)hookedWndProc);
    imguiReady = true;
    return true;
}

void DXHook::createRenderTargets(IDXGISwapChain3* chain) {
    renderTargets.resize(bufferCount);
    D3D12_CPU_DESCRIPTOR_HANDLE handle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < bufferCount; i++) {
        chain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
        device->CreateRenderTargetView(renderTargets[i], nullptr, handle);
        handle.ptr += rtvDescSize;
    }
}

void DXHook::cleanupRenderTargets() {
    for (auto* rt : renderTargets) if (rt) rt->Release();
    renderTargets.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// WndProc hook — forwards input to ImGui and module manager
// ─────────────────────────────────────────────────────────────────────────────
LRESULT CALLBACK DXHook::hookedWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    // Let ImGui handle input when the GUI is open
    if (ClickGUI::get().isOpen()) {
        ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp);
        if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN ||
            msg == WM_KEYDOWN || msg == WM_CHAR)
            return 0; // consume input so the game doesn't see it
    }

    if (msg == WM_KEYDOWN) {
        ModuleManager::get().onKey((int)wp, true);
    } else if (msg == WM_KEYUP) {
        ModuleManager::get().onKey((int)wp, false);
    } else if (msg == WM_LBUTTONDOWN) {
        ModuleManager::get().onKey(VK_LBUTTON, true);
    } else if (msg == WM_LBUTTONUP) {
        ModuleManager::get().onKey(VK_LBUTTON, false);
    } else if (msg == WM_RBUTTONDOWN) {
        ModuleManager::get().onKey(VK_RBUTTON, true);
    } else if (msg == WM_RBUTTONUP) {
        ModuleManager::get().onKey(VK_RBUTTON, false);
    } else if (msg == WM_MOUSEWHEEL) {
        DXHook::onScrollWheel(GET_WHEEL_DELTA_WPARAM(wp) > 0 ? 1 : -1);
    }

    return CallWindowProcW(originalWndProc, hwnd, msg, wp, lp);
}

void DXHook::onScrollWheel(int delta) {
    auto* zoom = ModuleManager::get().get<class Zoom>();
    if (zoom) zoom->onScrollWheel(delta);
}
