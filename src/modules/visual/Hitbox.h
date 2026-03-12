#pragma once
#include "../Module.h"
#include "imgui.h"

class Hitbox : public Module {
public:
    ImVec4 color     { 1.f, 0.f, 0.f, 0.8f };
    float  thickness = 1.0f;
    float  maxRange  = 64.f;
    bool   players   = true;
    bool   mobs      = true;

    Hitbox() : Module("Hitbox", "Renders entity bounding boxes.", Category::Visual) {}

    void onWorldRender(const SDK::RenderContext& ctx) override {
        auto* ci = SDK::ClientInstance::get();
        if (!ci || !ci->isInGame()) return;

        // [OFFSET] Iterate entities in the level:
        // for (auto* entity : ci->level->getEntities()) {
        //     bool isPlayer = entity->isPlayer();
        //     bool isMob    = entity->isMob();
        //     if (!((players && isPlayer) || (mobs && isMob))) continue;
        //     if (entity == ci->localPlayer) continue;
        //     float dist = ci->localPlayer->position.dist(entity->position);
        //     if (dist > maxRange) continue;
        //     renderAABB(ctx, entity->getAABB());
        // }
    }

    void onGUI() override {
        ImGui::ColorEdit4("Color##hb",      (float*)&color);
        ImGui::SliderFloat("Thickness##hb", &thickness, 0.5f, 4.f);
        ImGui::SliderFloat("Max range##hb", &maxRange,  10.f, 128.f);
        ImGui::Checkbox("Players##hb", &players);
        ImGui::Checkbox("Mobs##hb",    &mobs);
    }

private:
    void renderAABB(const SDK::RenderContext& ctx, const SDK::AABB& box) {
        const SDK::Vec3 corners[8] = {
            {box.min.x, box.min.y, box.min.z}, {box.max.x, box.min.y, box.min.z},
            {box.max.x, box.max.y, box.min.z}, {box.min.x, box.max.y, box.min.z},
            {box.min.x, box.min.y, box.max.z}, {box.max.x, box.min.y, box.max.z},
            {box.max.x, box.max.y, box.max.z}, {box.min.x, box.max.y, box.max.z},
        };
        SDK::Vec2 scr[8]; bool vis[8];
        for (int i = 0; i < 8; i++) vis[i] = ctx.worldToScreen(corners[i], scr[i]);

        const int edges[12][2] = {
            {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}
        };
        auto* dl  = ImGui::GetForegroundDrawList();
        ImU32 col = ImGui::ColorConvertFloat4ToU32(color);
        for (auto& e : edges)
            if (vis[e[0]] && vis[e[1]])
                dl->AddLine({scr[e[0]].x, scr[e[0]].y},
                            {scr[e[1]].x, scr[e[1]].y}, col, thickness);
    }
};
