#pragma once
#include "../Module.h"
#include "imgui.h"

// BetterTooltips draws an enriched item tooltip overlay near the cursor
// when hovering over an item in the inventory. Extra info includes:
// item ID, max stack size, durability bar, enchantment list, food value.

class BetterTooltips : public Module {
public:
    bool showItemId    = true;
    bool showStack     = true;
    bool showDurability= true;
    bool showFood      = true;
    bool showEnchants  = true;

    BetterTooltips() : Module("Better Tooltips", "Shows extra item info in inventory tooltips.",
                               Category::Utility) {}

    void onRender(int sw, int sh) override {
        // Only show when the inventory screen is open
        auto* ci = SDK::ClientInstance::get();
        if (!ci || !ci->isInGame()) return;

        // [OFFSET] Detect if inventory is open and get hovered item:
        // SDK::ItemStack* hoveredItem = ci->getHoveredInventoryItem();
        // if (!hoveredItem || !hoveredItem->valid()) return;

        // Placeholder — render a sample tooltip near cursor
        ImVec2 mousePos = ImGui::GetMousePos();
        auto*  dl       = ImGui::GetForegroundDrawList();

        // TODO: replace with real item data
        const char* lines[] = {
            "Item: iron_sword",
            "ID: 307  Stack: 1",
            "Durability: 200 / 250",
            "Food: --",
            "Enchant: Sharpness II",
        };

        float lineH = 16.f, pad = 6.f;
        float bw = 160.f, bh = 5 * lineH + pad * 2;
        float bx = mousePos.x + 12, by = mousePos.y;

        dl->AddRectFilled({ bx, by }, { bx + bw, by + bh }, IM_COL32(20, 20, 20, 200), 4.f);
        dl->AddRect(      { bx, by }, { bx + bw, by + bh }, IM_COL32(120, 80, 200, 200), 4.f);

        int idx = 0;
        if (showItemId)     dl->AddText(ImGui::GetDefaultFont(), 12.f, {bx+pad, by+pad+idx++*lineH}, IM_COL32(220,220,220,255), lines[0]);
        if (showStack)      dl->AddText(ImGui::GetDefaultFont(), 12.f, {bx+pad, by+pad+idx++*lineH}, IM_COL32(180,180,180,255), lines[1]);
        if (showDurability) dl->AddText(ImGui::GetDefaultFont(), 12.f, {bx+pad, by+pad+idx++*lineH}, IM_COL32(100,200,100,255), lines[2]);
        if (showFood)       dl->AddText(ImGui::GetDefaultFont(), 12.f, {bx+pad, by+pad+idx++*lineH}, IM_COL32(220,160,60, 255), lines[3]);
        if (showEnchants)   dl->AddText(ImGui::GetDefaultFont(), 12.f, {bx+pad, by+pad+idx++*lineH}, IM_COL32(160,100,255,255), lines[4]);
    }

    void onGUI() override {
        ImGui::Checkbox("Show item ID##bt",     &showItemId);
        ImGui::Checkbox("Show stack size##bt",  &showStack);
        ImGui::Checkbox("Show durability##bt",  &showDurability);
        ImGui::Checkbox("Show food value##bt",  &showFood);
        ImGui::Checkbox("Show enchants##bt",    &showEnchants);
    }
};
