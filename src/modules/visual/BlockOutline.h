#pragma once
#include "../Module.h"
#include "imgui.h"

class BlockOutline : public Module {
public:
    ImVec4 color     { 1.f, 1.f, 1.f, 0.8f };
    float  thickness = 1.5f;
    bool   fill      = false;
    ImVec4 fillColor { 1.f, 1.f, 1.f, 0.1f };

    BlockOutline() : Module("Block Outline", "Draws a custom outline around the targeted block.",
                             Category::Visual) {}

    void onWorldRender(const SDK::RenderContext& ctx) override {
        // [OFFSET] Get the block the player is looking at:
        //   SDK::HitResult& hr = ci->localPlayer->getLookAtResult();
        //   if (hr.type != HitResultType::Block) return;
        //   SDK::Vec3 blockPos = hr.blockPos;
        //
        // Then project the 8 corners of the block AABB to screen using ctx.worldToScreen()
        // and draw lines between them via ImGui::GetForegroundDrawList().

        // Example placeholder rendering of a 1x1x1 cube at origin:
        SDK::Vec3 origin { 0, 0, 0 }; // TODO: replace with actual hit block position

        const SDK::Vec3 corners[8] = {
            {origin.x,     origin.y,     origin.z    },
            {origin.x+1,   origin.y,     origin.z    },
            {origin.x+1,   origin.y+1,   origin.z    },
            {origin.x,     origin.y+1,   origin.z    },
            {origin.x,     origin.y,     origin.z+1  },
            {origin.x+1,   origin.y,     origin.z+1  },
            {origin.x+1,   origin.y+1,   origin.z+1  },
            {origin.x,     origin.y+1,   origin.z+1  },
        };

        // Project all corners
        SDK::Vec2 scr[8];
        bool      vis[8];
        for (int i = 0; i < 8; i++)
            vis[i] = ctx.worldToScreen(corners[i], scr[i]);

        const int edges[12][2] = {
            {0,1},{1,2},{2,3},{3,0},  // back face
            {4,5},{5,6},{6,7},{7,4},  // front face
            {0,4},{1,5},{2,6},{3,7},  // connecting edges
        };

        auto* dl    = ImGui::GetForegroundDrawList();
        ImU32 col   = ImGui::ColorConvertFloat4ToU32(color);
        ImU32 fcol  = ImGui::ColorConvertFloat4ToU32(fillColor);

        for (auto& e : edges) {
            if (!vis[e[0]] || !vis[e[1]]) continue;
            dl->AddLine({ scr[e[0]].x, scr[e[0]].y },
                        { scr[e[1]].x, scr[e[1]].y }, col, thickness);
        }

        // Optional fill on visible faces (simplified — just fills convex hull)
        if (fill) {
            // TODO: proper face fill
        }
    }

    void onGUI() override {
        ImGui::ColorEdit4("Color##bo",      (float*)&color);
        ImGui::SliderFloat("Thickness##bo", &thickness, 0.5f, 5.f);
        ImGui::Checkbox("Fill##bo", &fill);
        if (fill) ImGui::ColorEdit4("Fill color##bo", (float*)&fillColor);
    }
};
