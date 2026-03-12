#pragma once
#include "../Module.h"
#include "imgui.h"
#include <vector>
#include <string>

class TabList : public Module {
public:
    int   maxPlayers = 20;
    bool  showPing   = true;
    float rowHeight  = 22.f;
    float width      = 220.f;

    TabList() : Module("Tab List", "Enhanced player list overlay with ping.",
                        Category::HUD) {}

    bool onKey(int vk, bool down) override {
        // Show when Tab is held (like vanilla), but as an overlay
        visible = (GetAsyncKeyState(VK_TAB) & 0x8000) != 0;
        return false;
    }

    void onRender(int sw, int sh) override {
        if (!(GetAsyncKeyState(VK_TAB) & 0x8000)) return;

        auto* ci = SDK::ClientInstance::get();

        // [OFFSET] Get player list from level
        // std::vector<SDK::PlayerListEntry> players = ci->level->getPlayerList();
        std::vector<std::pair<std::string, int>> players = {
            // Placeholder — replace with real data
            { "Player1", 23 }, { "Player2", 78 }, { "Player3", 145 }
        };

        float panelW  = width;
        float panelH  = (float)(std::min((int)players.size(), maxPlayers) + 1) * rowHeight + 10.f;
        float panelX  = (sw - panelW) * 0.5f;
        float panelY  = 40.f;

        auto* dl = ImGui::GetForegroundDrawList();
        dl->AddRectFilled({ panelX, panelY }, { panelX + panelW, panelY + panelH },
                          IM_COL32(0, 0, 0, 180), 6.f);
        dl->AddRect(      { panelX, panelY }, { panelX + panelW, panelY + panelH },
                          IM_COL32(255, 255, 255, 60), 6.f);

        // Header
        dl->AddText(ImGui::GetIO().Fonts->Fonts[0], 13.f, { panelX + 8, panelY + 6 },
                    IM_COL32(255, 255, 255, 200), "Players");

        int count = 0;
        for (auto& [name, ping] : players) {
            if (count >= maxPlayers) break;
            float ry = panelY + (count + 1) * rowHeight + 6.f;

            // Alternating row background
            if (count % 2 == 0)
                dl->AddRectFilled({ panelX + 2, ry }, { panelX + panelW - 2, ry + rowHeight },
                                  IM_COL32(255, 255, 255, 15));

            // Head placeholder (16×16 square) — TODO: render actual skin head texture
            dl->AddRectFilled({ panelX + 6, ry + 3 }, { panelX + 20, ry + 17 },
                              IM_COL32(100, 80, 60, 200));

            dl->AddText(ImGui::GetIO().Fonts->Fonts[0], 13.f, { panelX + 26, ry + 4 },
                        IM_COL32(255, 255, 255, 220), name.c_str());

            if (showPing) {
                ImU32 pingCol;
                if      (ping <  50) pingCol = IM_COL32(50,  255,  80, 220);
                else if (ping < 100) pingCol = IM_COL32(255, 220,   0, 220);
                else if (ping < 200) pingCol = IM_COL32(255, 140,   0, 220);
                else                 pingCol = IM_COL32(255,  50,  50, 220);

                char pingBuf[16];
                snprintf(pingBuf, sizeof(pingBuf), "%d ms", ping);
                auto tsz = ImGui::CalcTextSize(pingBuf);
                dl->AddText(ImGui::GetIO().Fonts->Fonts[0], 13.f,
                            { panelX + panelW - tsz.x - 8, ry + 4 }, pingCol, pingBuf);
            }
            count++;
        }
    }

    void onGUI() override {
        ImGui::SliderFloat("Row height##tab", &rowHeight, 16.f, 36.f);
        ImGui::SliderFloat("Width##tab",      &width,     140.f, 400.f);
        ImGui::SliderInt("Max players##tab",  &maxPlayers, 5, 60);
        ImGui::Checkbox("Show ping##tab",     &showPing);
    }
};
