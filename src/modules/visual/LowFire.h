#pragma once
#include "../Module.h"

// LowFire shrinks the fire overlay texture so it barely covers the screen.
// In Bedrock, the fire HUD overlay is drawn via a UI element ("fire_screen_*").
// The standard technique is to patch the vertex positions when the fire quad
// is submitted to the renderer, or to override the texture UV to an empty region.

class LowFire : public Module {
public:
    float scale = 0.15f; // fraction of normal fire size (0 = invisible, 1 = full)

    LowFire() : Module("LowFire", "Reduces the fire overlay to a small sliver at the bottom.",
                        Category::Visual) {}

    void onEnable() override {
        // [HOOK] Hook the UI renderer call that draws the fire overlay quad.
        // In vanilla Bedrock, search for the function that submits the "fire_screen_*"
        // texture and scale the vertex Y coordinates by `scale`.
        applyPatch(scale);
    }

    void onDisable() override { applyPatch(1.f); }

    void onGUI() override {
        if (ImGui::SliderFloat("Scale##lf", &scale, 0.f, 1.f))
            if (enabled) applyPatch(scale);
    }

private:
    void applyPatch(float s) {
        // TODO: patch the fire overlay draw call
        // The exact implementation depends on reverse-engineering the UI quad submission.
    }
};

#include "imgui.h"
