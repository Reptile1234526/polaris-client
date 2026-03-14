#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// DXHook — DirectX 12 Present hook + ImGui overlay
// ─────────────────────────────────────────────────────────────────────────────

class DXHook {
public:
    static bool init();
    static void shutdown();

    // Called by ModuleManager when a scroll-wheel event is detected in WndProc
    static void onScrollWheel(int delta);

    // ── ImGui / DX12 state (public so DoRenderFrame helper can access) ────────
    static bool                              imguiReady;
    static ID3D12Device*                     device;
    static ID3D12CommandQueue*               cmdQueue;
    static ID3D12DescriptorHeap*             rtvHeap;
    static ID3D12DescriptorHeap*             srvHeap;
    static ID3D12GraphicsCommandList*        cmdList;
    static std::vector<ID3D12CommandAllocator*> cmdAllocs;
    static std::vector<ID3D12Resource*>      renderTargets;
    static UINT                              bufferCount;
    static UINT                              rtvDescSize;
    static HWND                              gameWindow;
    static ID3D12Fence*                      fence;
    static HANDLE                            fenceEvent;
    static std::vector<UINT64>               fenceValues;
    static UINT64                            fenceCounter;
    static bool  initImGui(IDXGISwapChain3* chain);

private:
    using PresentFn            = HRESULT(__stdcall*)(IDXGISwapChain3*, UINT, UINT);
    using ExecuteCmdListsFn    = void(__stdcall*)(ID3D12CommandQueue*, UINT,
                                                  ID3D12CommandList* const*);
    using ResizeBuffersFn      = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT, UINT,
                                                     DXGI_FORMAT, UINT);

    static PresentFn          originalPresent;
    static ExecuteCmdListsFn  originalExecuteCmdLists;
    static ResizeBuffersFn    originalResizeBuffers;

    static HRESULT __stdcall hookedPresent(IDXGISwapChain3* chain, UINT sync, UINT flags);
    static void    __stdcall hookedExecuteCmdLists(ID3D12CommandQueue* queue, UINT count,
                                                   ID3D12CommandList* const* lists);
    static HRESULT __stdcall hookedResizeBuffers(IDXGISwapChain* chain, UINT count,
                                                 UINT w, UINT h, DXGI_FORMAT fmt, UINT flags);

    static WNDPROC   originalWndProc;
    static LRESULT CALLBACK hookedWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    static void  cleanupRenderTargets();
    static void  createRenderTargets(IDXGISwapChain3* chain);
    static void* getVTableEntry(void* obj, int index);
    static bool  getDummySwapChainVTable(void** scVtable, void** cqVtable);
};
