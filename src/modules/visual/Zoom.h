#pragma once
#include "../Module.h"
#include "imgui.h"

class Zoom : public Module {
public:
    float zoomFOV    = 15.f;   // FOV when zoomed in
    float smoothness = 8.f;    // lerp speed
    bool  scrollZoom = true;   // scroll wheel adjusts zoom level
    int   zoomKey    = 'C';

    Zoom() : Module("Zoom", "Zooms in like a spyglass while holding a key.",
                    Category::Visual, 'C') {}

    void onTick(SDK::ClientInstance* ci) override {
        if (!ci || !ci->isInGame() || !ci->localPlayer) return;
        bool holding = (GetAsyncKeyState(zoomKey) & 0x8000) != 0;

        // Smoothly lerp toward target FOV
        // [OFFSET] ci->localPlayer->setFOV(targetFOV) or patch the FOV value in memory
        float target = holding ? zoomFOV : 70.f; // 70 = Bedrock default FOV
        currentFOV  += (target - currentFOV) / smoothness;

        // TODO: apply currentFOV to the game renderer
        // ci->localPlayer->setFOV(currentFOV);
    }

    bool onKey(int vk, bool down) override {
        if (scrollZoom && enabled && (GetAsyncKeyState(zoomKey) & 0x8000)) {
            // WM_MOUSEWHEEL is handled separately in DXHook's WndProc hook
            // Scroll up → decrease zoomFOV (more zoom), scroll down → less zoom
        }
        return false;
    }

    void onScrollWheel(int delta) {
        if (!enabled || !(GetAsyncKeyState(zoomKey) & 0x8000)) return;
        zoomFOV = std::clamp(zoomFOV - delta * 2.f, 2.f, 60.f);
    }

    void onGUI() override {
        ImGui::SliderFloat("Zoom FOV##zm",   &zoomFOV,    2.f,  60.f);
        ImGui::SliderFloat("Smoothness##zm", &smoothness, 1.f, 20.f);
        ImGui::Checkbox("Scroll to adjust##zm", &scrollZoom);
        ImGui::Text("Zoom key: C (rebind via keybind field)");
    }

private:
    float currentFOV = 70.f;
};
