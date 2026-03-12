#pragma once
#include "../Module.h"
#include "imgui.h"

class ArmorHUD : public Module {
public:
    float posX = 10, posY = 200;
    bool  showDurability = true;
    bool  colorByDurability = true; // green → yellow → red
    float iconSize = 20.f;

    ArmorHUD() : Module("ArmorHUD", "Shows equipped armor pieces and their durability.",
                         Category::HUD) {}

    void onRender(int sw, int sh) override {
        auto* ci = SDK::ClientInstance::get();
        if (!ci || !ci->isInGame() || !ci->localPlayer) return;
        auto* p = ci->localPlayer;

        auto* dl = ImGui::GetForegroundDrawList();
        float x = posX, y = posY;
        float spacing = iconSize + 4.f;

        const char* armorNames[] = { "[H]", "[C]", "[L]", "[B]" }; // helmet, chest, legs, boots

        for (int i = 0; i < 4; i++) {
            // [OFFSET] Read actual armor stack from player->armorSlots[i]
            // For now draw placeholder text
            ImU32 col = IM_COL32(200, 200, 200, 255);

            if (colorByDurability) {
                // TODO: real durability ratio from ItemStack
                // float ratio = (float)item.damage / item.maxDurability;
                // col = lerp(green, red, ratio);
            }

            dl->AddText(ImGui::GetIO().Fonts->Fonts[0], iconSize * 0.8f,
                        { x, y + i * spacing }, col, armorNames[i]);

            if (showDurability) {
                char buf[16];
                snprintf(buf, sizeof(buf), "---"); // TODO: real value
                dl->AddText(ImGui::GetIO().Fonts->Fonts[0], 11.f,
                            { x + iconSize, y + i * spacing + 4.f },
                            IM_COL32(200, 200, 200, 200), buf);
            }
        }
    }

    void onGUI() override {
        ImGui::DragFloat2("Position##armor", &posX, 1.f, 0, 4000);
        ImGui::SliderFloat("Icon size##armor", &iconSize, 12.f, 40.f);
        ImGui::Checkbox("Show durability##armor", &showDurability);
        ImGui::Checkbox("Color by durability##armor", &colorByDurability);
    }
};
