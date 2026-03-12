#pragma once
#include "../Module.h"
#include "imgui.h"

class DropInventory : public Module {
public:
    int   dropKey    = VK_DELETE;
    bool  dropAll    = true;   // drop entire inventory vs. just hotbar
    bool  keepArmor  = true;   // never drop equipped armor
    bool  keepTools  = true;   // never drop pickaxe/sword/axe

    DropInventory() : Module("Drop Inventory", "Drops your entire inventory with a single key.",
                              Category::Utility, VK_DELETE) {}

    bool onKey(int vk, bool down) override {
        if (!down || vk != dropKey) return false;
        doDrop();
        return true;
    }

    void onGUI() override {
        ImGui::Checkbox("Drop full inventory (not just hotbar)##di", &dropAll);
        ImGui::Checkbox("Keep armor##di",   &keepArmor);
        ImGui::Checkbox("Keep tools##di",   &keepTools);
        ImGui::Text("Key: DEL (change via keybind)");
    }

private:
    bool isTool(const SDK::ItemStack& item) {
        // TODO: check item name/id against tool ids
        return false;
    }

    void doDrop() {
        auto* ci = SDK::ClientInstance::get();
        if (!ci || !ci->isInGame() || !ci->localPlayer) return;
        auto* inv = ci->localPlayer->inventory;
        if (!inv) return;

        int count = dropAll ? 36 : 9; // 9 = hotbar only
        for (int i = 0; i < count; i++) {
            auto& slot = inv->slots[i];
            if (!slot.valid()) continue;
            if (keepArmor) {
                // TODO: check if slot is an armor slot
            }
            if (keepTools && isTool(slot)) continue;
            // [OFFSET] Call the game's "drop item" function for this slot
            // Equivalent to pressing Q on the item in the inventory screen
            // ci->localPlayer->dropSlot(i, true /* drop all */);
        }
    }
};
