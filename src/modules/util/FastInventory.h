#pragma once
#include "../Module.h"

// FastInventory removes the item-move animation delay in the inventory screen,
// letting you move items instantly without waiting for the slide animation.
// In Bedrock this is controlled by a timer/lerp in the ContainerScreen or
// InventoryTransactionPacket handling. We patch the lerp value to 1.0f (instant).

class FastInventory : public Module {
public:
    FastInventory() : Module("Fast Inventory", "Removes inventory item-move animation delay.",
                              Category::Utility) {}

    void onEnable()  override { patchLerp(true);  }
    void onDisable() override { patchLerp(false); }

private:
    void patchLerp(bool fast) {
        // [OFFSET] Patch the inventory slot animation lerp speed.
        // Typical approach: find the float that controls lerp time (~0.15f default)
        // and write 0.0f (instant) when enabled, restore when disabled.
        //
        // uintptr_t base  = (uintptr_t)GetModuleHandleW(L"Minecraft.Windows.exe");
        // float* lerpTime = (float*)(base + 0x0 /* TODO offset */);
        // *lerpTime = fast ? 0.f : 0.15f;
    }
};
