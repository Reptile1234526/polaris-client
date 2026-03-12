#pragma once
#include "../Module.h"

// NoHurtCam prevents the red-vignette camera shake when taking damage.
// In Bedrock, the hurt-cam tilt is applied in the camera/view update function.
// We hook it and zero out the tilt angle when enabled.

class NoHurtCam : public Module {
public:
    NoHurtCam() : Module("No Hurt Cam", "Disables the red screen shake when you take damage.",
                          Category::Visual) {}

    void onEnable() override {
        // [HOOK] Hook the function that applies the hurt-cam rotation/tilt.
        // Signature varies by version. Search for calls to the hurtTime field on the player.
        // When hooked, if NoHurtCam is enabled, write 0 to the tilt value before it's applied.
        installHook();
    }

    void onDisable() override { removeHook(); }

private:
    void installHook() {
        // TODO: use MinHook to hook the camera tilt function
        // Example:
        //   MH_CreateHook((LPVOID)CAMERA_TILT_ADDR, &hookedTilt, (LPVOID*)&origTilt);
        //   MH_EnableHook((LPVOID)CAMERA_TILT_ADDR);
    }

    void removeHook() {
        // TODO: MH_DisableHook / MH_RemoveHook
    }
};
