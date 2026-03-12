#pragma once
#include "../Module.h"
#include "imgui.h"

class PingDisplay : public Module {
public:
    float posX = 10, posY = 340;
    bool  colorByPing = true;
    ImVec4 color { 1, 1, 1, 1 };

    PingDisplay() : Module("Ping Display", "Shows your current ping to the server.",
                            Category::HUD) {}

    void onRender(int sw, int sh) override {
        auto* ci = SDK::ClientInstance::get();
        if (!ci || !ci->isInGame()) return;

        // [OFFSET] Read ping from level player list — find own entry
        // auto entries = ci->level->getPlayerList();
        // int ping = entries.empty() ? 0 : entries[0].ping; // your entry
        int ping = 0; // TODO: real value

        ImU32 col;
        if (colorByPing) {
            if      (ping <  50) col = IM_COL32(50, 255, 80, 255);
            else if (ping < 100) col = IM_COL32(255, 220, 0, 255);
            else if (ping < 200) col = IM_COL32(255, 140, 0, 255);
            else                 col = IM_COL32(255, 50, 50, 255);
        } else {
            col = ImGui::ColorConvertFloat4ToU32(color);
        }

        char buf[32];
        snprintf(buf, sizeof(buf), "Ping: %d ms", ping);
        ImGui::GetForegroundDrawList()->AddText(
            ImGui::GetIO().Fonts->Fonts[0], 13.f, { posX, posY }, col, buf);
    }

    void onGUI() override {
        ImGui::DragFloat2("Position##ping", &posX, 1.f, 0, 4000);
        ImGui::Checkbox("Color by ping##ping", &colorByPing);
        if (!colorByPing) ImGui::ColorEdit4("Color##ping", (float*)&color);
    }
};
