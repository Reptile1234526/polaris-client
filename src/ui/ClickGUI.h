#pragma once
#include <imgui.h>
#include "../modules/ModuleManager.h"
#include <Windows.h>

// ─────────────────────────────────────────────────────────────────────────────
// ClickGUI — settings / module toggle overlay (opened with Right Shift by default)
// ─────────────────────────────────────────────────────────────────────────────
class ClickGUI {
public:
    static ClickGUI& get() { static ClickGUI inst; return inst; }

    bool isOpen() const { return open; }

    void toggle() { open = !open;
        // Show / hide the system cursor when the GUI opens/closes
        ShowCursor(open);
    }

    void render() {
        if (!open) return;

        ImGui::SetNextWindowSize({ 560, 420 }, ImGuiCond_Once);
        ImGui::SetNextWindowPos({ 100, 100 }, ImGuiCond_Once);

        ImGui::Begin("Polaris Client", &open,
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

        // Sidebar: category tabs
        ImGui::BeginChild("sidebar", { 120, 0 }, true);
        const char* cats[] = { "HUD", "Visual", "Utility" };
        for (int i = 0; i < 3; i++) {
            bool sel = selectedCat == i;
            if (sel) ImGui::PushStyleColor(ImGuiCol_Button,
                         { 0.35f, 0.18f, 0.7f, 1.f });
            if (ImGui::Button(cats[i], { -1, 32 })) selectedCat = i;
            if (sel) ImGui::PopStyleColor();
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // Module list
        ImGui::BeginChild("modules", { 0, 0 }, false);
        Category targetCat = (Category)selectedCat;

        for (auto& mod : ModuleManager::get().getAll()) {
            if (mod->category != targetCat) continue;

            // Toggle checkbox
            bool en = mod->enabled;
            if (ImGui::Checkbox(("##en_" + mod->name).c_str(), &en))
                mod->setEnabled(en);
            ImGui::SameLine();

            // Collapsible settings panel
            bool expanded = ImGui::CollapsingHeader(mod->name.c_str());
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("%s", mod->description.c_str());

            if (expanded) {
                ImGui::Indent(16.f);
                mod->onGUI();
                ImGui::Unindent(16.f);
            }
        }
        ImGui::EndChild();
        ImGui::End();

        // Close on Escape or Right Shift
        if (ImGui::IsKeyPressed(ImGuiKey_Escape) ||
            ImGui::IsKeyPressed(ImGuiKey_RightShift))
            toggle();
    }

private:
    bool open        = false;
    int  selectedCat = 0; // 0=HUD, 1=Visual, 2=Utility
};
