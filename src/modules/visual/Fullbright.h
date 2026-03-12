#pragma once
#include "../Module.h"

class Fullbright : public Module {
public:
    float gamma = 100.f;

    Fullbright() : Module("Fullbright", "Maximises brightness so caves look fully lit.",
                           Category::Visual, VK_F5) {}

    void onEnable()  override { applyGamma(gamma); }
    void onDisable() override { applyGamma(1.f);   } // restore default

    void onTick(SDK::ClientInstance* ci) override {
        // Re-apply every tick in case the game resets gamma
        if (ci && ci->getGamma() != gamma)
            ci->setGamma(gamma);
    }

    void onGUI() override {
        if (ImGui::SliderFloat("Gamma##fb", &gamma, 5.f, 1000.f))
            if (enabled) applyGamma(gamma);
    }

private:
    void applyGamma(float g) {
        auto* ci = SDK::ClientInstance::get();
        if (ci) ci->setGamma(g);
    }
};

#include "imgui.h" // needed for onGUI — put at top in real project
