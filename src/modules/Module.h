#pragma once
#include <string>
#include <functional>
#include <Windows.h>
#include "../sdk/SDK.h"

// ─────────────────────────────────────────────────────────────────────────────
// Module — base class for every Polaris feature
// ─────────────────────────────────────────────────────────────────────────────

enum class Category { HUD, Visual, Utility };

class Module {
public:
    const std::string name;
    const std::string description;
    const Category    category;
    int               keybind;   // VK_* key code, 0 = none
    bool              enabled  = false;
    bool              visible  = true; // show in HUD / click GUI

    Module(const std::string& name, const std::string& desc,
           Category cat, int keybind = 0)
        : name(name), description(desc), category(cat), keybind(keybind) {}

    virtual ~Module() = default;

    void toggle() {
        enabled = !enabled;
        if (enabled) onEnable(); else onDisable();
    }
    void setEnabled(bool v) { if (v != enabled) toggle(); }

    // ── Overridable callbacks ────────────────────────────────────────────────
    virtual void onEnable()  {}
    virtual void onDisable() {}

    // Called every game tick — game logic, state polling
    virtual void onTick(SDK::ClientInstance* ci) {}

    // Called inside the DX12 Present hook — draw ImGui HUD elements
    // Use ImGui::GetWindowDrawList() or ImGui::GetForegroundDrawList()
    virtual void onRender(int screenW, int screenH) {}

    // Called inside the ClickGUI to draw per-module settings widgets
    virtual void onGUI() {}

    // Called on keyboard/mouse input — return true to consume the event
    virtual bool onKey(int vk, bool down) { return false; }

    // World-space render (hitboxes, outlines, etc.)
    virtual void onWorldRender(const SDK::RenderContext& ctx) {}
};
