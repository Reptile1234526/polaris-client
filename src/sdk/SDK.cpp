#include "SDK.h"

namespace SDK {

// ── ItemStack ─────────────────────────────────────────────────────────────────
int ItemStack::maxDurability() const { return 0; }

// ── LocalPlayer ───────────────────────────────────────────────────────────────
int   LocalPlayer::getArmorDurability(int) const { return 0; }
float LocalPlayer::getFOV()              const { return 70.f; }
void  LocalPlayer::setFOV(float)               {}
float LocalPlayer::getReachDistance()   const { return 3.f; }
Actor* LocalPlayer::getTargetActor()    const { return nullptr; }

// ── Level ─────────────────────────────────────────────────────────────────────
std::vector<PlayerListEntry> Level::getPlayerList() const { return {}; }

// ── ClientInstance ────────────────────────────────────────────────────────────
HWND  ClientInstance::getWindow()       const { return nullptr; }
float ClientInstance::getGamma()        const { return 1.f; }
void  ClientInstance::setGamma(float)         {}

// ── RenderContext ─────────────────────────────────────────────────────────────
bool RenderContext::worldToScreen(const Vec3& world, Vec2& out) const {
    // w = viewProj[3]*x + viewProj[7]*y + viewProj[11]*z + viewProj[15]
    float w = viewProj[3]  * world.x + viewProj[7]  * world.y
            + viewProj[11] * world.z + viewProj[15];
    if (w <= 0.f) return false;
    float x = viewProj[0] * world.x + viewProj[4] * world.y
            + viewProj[8] * world.z + viewProj[12];
    float y = viewProj[1] * world.x + viewProj[5] * world.y
            + viewProj[9] * world.z + viewProj[13];
    out.x = (x / w + 1.f) * 0.5f * screenW;
    out.y = (1.f - y / w) * 0.5f * screenH;
    return true;
}

} // namespace SDK
