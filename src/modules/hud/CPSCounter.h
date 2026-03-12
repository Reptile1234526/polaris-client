#pragma once
#include "../Module.h"
#include "imgui.h"
#include <deque>
#include <chrono>

class CPSCounter : public Module {
public:
    float posX = 10, posY = 60;
    ImVec4 color { 1, 1, 0.2f, 1 };

    CPSCounter() : Module("CPS Counter", "Shows left and right clicks per second.",
                           Category::HUD) {}

    // Called from the DX hook's WndProc hook — record click times
    void recordClick(bool right) {
        auto now = std::chrono::steady_clock::now();
        (right ? rClicks : lClicks).push_back(now);
    }

    void onRender(int sw, int sh) override {
        auto now  = std::chrono::steady_clock::now();
        auto cutoff = now - std::chrono::seconds(1);

        // Prune clicks older than 1 second
        while (!lClicks.empty() && lClicks.front() < cutoff) lClicks.pop_front();
        while (!rClicks.empty() && rClicks.front() < cutoff) rClicks.pop_front();

        auto* dl = ImGui::GetForegroundDrawList();
        ImU32 col = ImGui::ColorConvertFloat4ToU32(color);

        char buf[64];
        snprintf(buf, sizeof(buf), "CPS: %d L | %d R",
                 (int)lClicks.size(), (int)rClicks.size());
        dl->AddText(ImGui::GetIO().Fonts->Fonts[0], 13.f, { posX, posY }, col, buf);
    }

    bool onKey(int vk, bool down) override {
        if (!down) return false;
        if (vk == VK_LBUTTON) { recordClick(false); return false; }
        if (vk == VK_RBUTTON) { recordClick(true);  return false; }
        return false;
    }

    void onGUI() override {
        ImGui::DragFloat2("Position##cps", &posX, 1.f, 0, 4000);
        ImGui::ColorEdit4("Color##cps", (float*)&color);
    }

private:
    using TimePoint = std::chrono::steady_clock::time_point;
    std::deque<TimePoint> lClicks, rClicks;
};
