#pragma once
#include "../Module.h"
#include "imgui.h"
#include <chrono>

class ComboCounter : public Module {
public:
    float posX = 10, posY = 300;
    int   combo    = 0;
    float timeout  = 3.f; // seconds before combo resets
    ImVec4 color   { 1.f, 0.5f, 0.f, 1.f };

    ComboCounter() : Module("Combo Counter", "Counts consecutive hits on entities.",
                             Category::HUD) {}

    // Call this when a hit is registered (hook EntityDamageSource or hurtTime change)
    void registerHit() {
        combo++;
        lastHit = std::chrono::steady_clock::now();
    }

    void resetCombo() { combo = 0; }

    void onTick(SDK::ClientInstance* ci) override {
        if (combo == 0) return;
        auto elapsed = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - lastHit).count();
        if (elapsed > timeout) combo = 0;
    }

    void onRender(int sw, int sh) override {
        if (combo < 2) return; // only show from 2+ hits
        auto* dl = ImGui::GetForegroundDrawList();
        char buf[32];
        snprintf(buf, sizeof(buf), "%d combo", combo);
        ImU32 col = ImGui::ColorConvertFloat4ToU32(color);
        dl->AddText(ImGui::GetDefaultFont(), 16.f, { posX, posY }, col, buf);
    }

    void onGUI() override {
        ImGui::DragFloat2("Position##combo", &posX, 1.f, 0, 4000);
        ImGui::SliderFloat("Reset timeout##combo", &timeout, 1.f, 10.f);
        ImGui::ColorEdit4("Color##combo", (float*)&color);
    }

private:
    std::chrono::steady_clock::time_point lastHit;
};
