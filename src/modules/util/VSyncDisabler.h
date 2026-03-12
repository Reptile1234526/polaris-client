#pragma once
#include "../Module.h"

// VSyncDisabler forces syncInterval = 0 in the DXGI Present call,
// uncapping the frame rate regardless of the in-game VSync setting.
// The DXHook already intercepts Present — it passes syncInterval through
// to this module so we can override it.

class VSyncDisabler : public Module {
public:
    VSyncDisabler() : Module("VSync Disabler", "Forces VSync off for uncapped frame rate.",
                              Category::Utility) {}

    // Called by DXHook::hookedPresent before forwarding to the original.
    // Returns the syncInterval to actually use.
    UINT overrideSyncInterval(UINT original) const {
        return enabled ? 0u : original;
    }
};
