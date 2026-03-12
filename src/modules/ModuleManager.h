#pragma once
#include <vector>
#include <memory>
#include <algorithm>
#include "Module.h"

// Forward-declare all modules
#include "hud/ArmorHUD.h"
#include "hud/CPSCounter.h"
#include "hud/Coords.h"
#include "hud/ComboCounter.h"
#include "hud/Crosshair.h"
#include "hud/Keystrokes.h"
#include "hud/PingDisplay.h"
#include "hud/ReachDisplay.h"
#include "hud/TabList.h"
#include "visual/BlockOutline.h"
#include "visual/Fullbright.h"
#include "visual/Hitbox.h"
#include "visual/LowFire.h"
#include "visual/NoHurtCam.h"
#include "visual/Zoom.h"
#include "util/BetterTooltips.h"
#include "util/DropInventory.h"
#include "util/FastInventory.h"
#include "util/VSyncDisabler.h"

class ModuleManager {
public:
    static ModuleManager& get() {
        static ModuleManager inst;
        return inst;
    }

    void init() {
        add<ArmorHUD>();
        add<CPSCounter>();
        add<Coords>();
        add<ComboCounter>();
        add<Crosshair>();
        add<Keystrokes>();
        add<PingDisplay>();
        add<ReachDisplay>();
        add<TabList>();
        add<BlockOutline>();
        add<Fullbright>();
        add<Hitbox>();
        add<LowFire>();
        add<NoHurtCam>();
        add<Zoom>();
        add<BetterTooltips>();
        add<DropInventory>();
        add<FastInventory>();
        add<VSyncDisabler>();
    }

    void onTick(SDK::ClientInstance* ci) {
        for (auto& m : modules)
            if (m->enabled) m->onTick(ci);
    }

    void onRender(int w, int h) {
        for (auto& m : modules)
            if (m->enabled && m->visible) m->onRender(w, h);
    }

    void onWorldRender(const SDK::RenderContext& ctx) {
        for (auto& m : modules)
            if (m->enabled) m->onWorldRender(ctx);
    }

    bool onKey(int vk, bool down) {
        // Toggle modules by keybind
        for (auto& m : modules)
            if (m->keybind && vk == m->keybind && down) { m->toggle(); return true; }
        // Pass to modules for per-module handling
        for (auto& m : modules)
            if (m->enabled && m->onKey(vk, down)) return true;
        return false;
    }

    const std::vector<std::unique_ptr<Module>>& getAll() const { return modules; }

    template<typename T>
    T* get() {
        for (auto& m : modules)
            if (auto* p = dynamic_cast<T*>(m.get())) return p;
        return nullptr;
    }

private:
    std::vector<std::unique_ptr<Module>> modules;

    template<typename T, typename... Args>
    void add(Args&&... args) {
        modules.push_back(std::make_unique<T>(std::forward<Args>(args)...));
    }
};
