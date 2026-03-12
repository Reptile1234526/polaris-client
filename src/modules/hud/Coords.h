#pragma once
#include "../Module.h"
#include "imgui.h"
#include <cmath>

class Coords : public Module {
public:
    float posX = 10, posY = 10; // screen position
    float fontSize = 1.0f;
    ImVec4 color { 1, 1, 1, 1 };
    bool showFacing = true;

    Coords() : Module("Coords", "Shows your X/Y/Z coordinates and facing direction.",
                       Category::HUD) {}

    void onRender(int sw, int sh) override {
        auto* ci = SDK::ClientInstance::get();
        if (!ci || !ci->isInGame()) return;
        auto* p = ci->localPlayer;
        if (!p) return;

        const SDK::Vec3& pos = p->position;

        // Facing direction from yaw
        const char* facing = "?";
        float yaw = fmodf(p->yaw, 360.f);
        if (yaw < 0) yaw += 360.f;
        if      (yaw <  45 || yaw >= 315) facing = "South (+Z)";
        else if (yaw <  135)              facing = "West  (-X)";
        else if (yaw <  225)              facing = "North (-Z)";
        else                              facing = "East  (+X)";

        auto* dl = ImGui::GetForegroundDrawList();
        ImU32 col = ImGui::ColorConvertFloat4ToU32(color);
        float sz   = 13.f * fontSize;

        char buf[128];
        snprintf(buf, sizeof(buf), "X: %.1f  Y: %.1f  Z: %.1f", pos.x, pos.y, pos.z);
        dl->AddText(ImGui::GetDefaultFont(), sz, { posX, posY }, col, buf);

        if (showFacing) {
            snprintf(buf, sizeof(buf), "Facing: %s", facing);
            dl->AddText(ImGui::GetDefaultFont(), sz, { posX, posY + sz + 2 }, col, buf);
        }
    }

    void onGUI() override {
        ImGui::DragFloat2("Position##coords", &posX, 1.f, 0, 4000);
        ImGui::SliderFloat("Font scale##coords", &fontSize, 0.5f, 2.f);
        ImGui::ColorEdit4("Color##coords", (float*)&color);
        ImGui::Checkbox("Show facing##coords", &showFacing);
    }
};
