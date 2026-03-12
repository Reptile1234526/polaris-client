#pragma once
#include "../Module.h"
#include "imgui.h"

class Crosshair : public Module {
public:
    // Style: 0=cross, 1=dot, 2=circle, 3=cross+dot
    int   style    = 0;
    float size     = 10.f;
    float thickness= 1.5f;
    float gap      = 3.f;   // gap from centre for cross style
    ImVec4 color   { 1, 1, 1, 1 };
    bool  outline  = true;

    Crosshair() : Module("Crosshair", "Custom crosshair replacing the default.",
                          Category::HUD) {}

    void onRender(int sw, int sh) override {
        auto* dl = ImGui::GetForegroundDrawList();
        float cx = sw * 0.5f, cy = sh * 0.5f;
        ImU32 col = ImGui::ColorConvertFloat4ToU32(color);
        ImU32 out = IM_COL32(0, 0, 0, 200);
        float t = thickness;

        auto drawCross = [&]() {
            // Horizontal
            if (outline) dl->AddLine({cx - size - 1, cy}, {cx - gap - 1, cy}, out, t + 2);
            dl->AddLine({cx - size, cy}, {cx - gap, cy}, col, t);
            if (outline) dl->AddLine({cx + gap + 1, cy}, {cx + size + 1, cy}, out, t + 2);
            dl->AddLine({cx + gap, cy}, {cx + size, cy}, col, t);
            // Vertical
            if (outline) dl->AddLine({cx, cy - size - 1}, {cx, cy - gap - 1}, out, t + 2);
            dl->AddLine({cx, cy - size}, {cx, cy - gap}, col, t);
            if (outline) dl->AddLine({cx, cy + gap + 1}, {cx, cy + size + 1}, out, t + 2);
            dl->AddLine({cx, cy + gap}, {cx, cy + size}, col, t);
        };

        auto drawDot = [&]() {
            if (outline) dl->AddCircleFilled({cx, cy}, 3.5f, out);
            dl->AddCircleFilled({cx, cy}, 2.5f, col);
        };

        if (style == 0) { drawCross(); }
        else if (style == 1) { drawDot(); }
        else if (style == 2) {
            if (outline) dl->AddCircle({cx, cy}, size + 1, out, 0, t + 2);
            dl->AddCircle({cx, cy}, size, col, 0, t);
        } else {
            drawCross();
            drawDot();
        }
    }

    void onGUI() override {
        const char* styles[] = { "Cross", "Dot", "Circle", "Cross + Dot" };
        ImGui::Combo("Style##ch", &style, styles, 4);
        ImGui::SliderFloat("Size##ch",      &size,      2.f,  30.f);
        ImGui::SliderFloat("Thickness##ch", &thickness, 0.5f, 5.f);
        ImGui::SliderFloat("Gap##ch",       &gap,       0.f,  15.f);
        ImGui::ColorEdit4("Color##ch",  (float*)&color);
        ImGui::Checkbox("Outline##ch",  &outline);
    }
};
