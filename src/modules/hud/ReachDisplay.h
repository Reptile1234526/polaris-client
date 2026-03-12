#pragma once
#include "../Module.h"
#include "imgui.h"

class ReachDisplay : public Module {
public:
    float posX = 10, posY = 360;
    int   decimals = 2;
    ImVec4 color { 1, 1, 1, 1 };

    ReachDisplay() : Module("Reach Display", "Shows the distance to your currently targeted entity.",
                             Category::HUD) {}

    void onRender(int sw, int sh) override {
        auto* ci = SDK::ClientInstance::get();
        if (!ci || !ci->isInGame() || !ci->localPlayer) return;

        // [OFFSET] Get targeted entity from LocalPlayer::getTargetActor()
        // SDK::Actor* target = ci->localPlayer->getTargetActor();
        // if (!target) return;
        // float dist = ci->localPlayer->position.dist(target->position);
        float dist = 0.f; // TODO: real value

        char fmt[16], buf[32];
        snprintf(fmt, sizeof(fmt), "Reach: %%.%df", decimals);
        snprintf(buf,  sizeof(buf),  fmt, dist);

        ImGui::GetForegroundDrawList()->AddText(
            ImGui::GetDefaultFont(), 13.f, { posX, posY },
            ImGui::ColorConvertFloat4ToU32(color), buf);
    }

    void onGUI() override {
        ImGui::DragFloat2("Position##reach", &posX, 1.f, 0, 4000);
        ImGui::SliderInt("Decimals##reach", &decimals, 0, 4);
        ImGui::ColorEdit4("Color##reach", (float*)&color);
    }
};
