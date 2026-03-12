#pragma once
#include "../Module.h"
#include "imgui.h"
#include <Windows.h>

class Keystrokes : public Module {
public:
    float posX = 10, posY = 100;
    float keySize = 30.f;
    ImVec4 bgColor     { 0, 0, 0, 0.5f };
    ImVec4 activeColor { 1, 1, 1, 0.8f };
    ImVec4 textColor   { 1, 1, 1, 1.f  };

    Keystrokes() : Module("Keystrokes", "Displays W/A/S/D, Space, LMB and RMB keys.",
                           Category::HUD) {}

    void onRender(int sw, int sh) override {
        auto* dl = ImGui::GetForegroundDrawList();
        float s = keySize, g = 3.f; // key size + gap

        struct Key { const char* label; float ox, oy; int vk; };
        Key keys[] = {
            { "W",     s + g,     0,         'W'          },
            { "A",     0,         s + g,     'A'          },
            { "S",     s + g,     s + g,     'S'          },
            { "D",     (s+g)*2,   s + g,     'D'          },
            { "SPC",   s + g,     (s+g)*2,   VK_SPACE     },
            { "LMB",   0,         (s+g)*3,   VK_LBUTTON   },
            { "RMB",   (s+g)*2,   (s+g)*3,   VK_RBUTTON   },
        };

        for (auto& k : keys) {
            bool   held = (GetAsyncKeyState(k.vk) & 0x8000) != 0;
            ImVec2 tl   = { posX + k.ox, posY + k.oy };
            ImVec2 br   = { tl.x + s, tl.y + s };
            ImU32  bg   = ImGui::ColorConvertFloat4ToU32(held ? activeColor : bgColor);
            ImU32  fg   = ImGui::ColorConvertFloat4ToU32(textColor);

            dl->AddRectFilled(tl, br, bg, 4.f);
            dl->AddRect(tl, br, IM_COL32(255,255,255,80), 4.f);

            // Centre the label
            auto tsz = ImGui::CalcTextSize(k.label);
            ImVec2 tp = { tl.x + (s - tsz.x) * 0.5f, tl.y + (s - tsz.y) * 0.5f };
            dl->AddText(tp, fg, k.label);
        }
    }

    void onGUI() override {
        ImGui::DragFloat2("Position##ks", &posX, 1.f, 0, 4000);
        ImGui::SliderFloat("Key size##ks", &keySize, 20.f, 60.f);
        ImGui::ColorEdit4("Background##ks", (float*)&bgColor);
        ImGui::ColorEdit4("Active##ks",     (float*)&activeColor);
        ImGui::ColorEdit4("Text##ks",       (float*)&textColor);
    }
};
