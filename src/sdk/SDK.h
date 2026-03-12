#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Polaris Client — Bedrock SDK stubs
// Offsets are for Minecraft.Windows.exe and change with every game update.
// Use a tool like Cheat Engine or IDA to find updated offsets.
// Tagged with [OFFSET] so you can search and update them quickly.
// ─────────────────────────────────────────────────────────────────────────────

namespace SDK {

// ── Math types ───────────────────────────────────────────────────────────────
struct Vec2 { float x, y; };
struct Vec3 { float x, y, z;
    float dist(const Vec3& o) const {
        float dx = x-o.x, dy = y-o.y, dz = z-o.z;
        return sqrtf(dx*dx + dy*dy + dz*dz);
    }
};
struct AABB { Vec3 min, max; };

// ── Item / inventory ──────────────────────────────────────────────────────────
struct ItemDescriptor {
    char pad[0x08];                         // [OFFSET] internal item id
    uint16_t itemId;                        // [OFFSET]
    uint16_t auxValue;
};

struct ItemStack {
    char      pad0[0x08];
    ItemDescriptor* descriptor;             // [OFFSET] 0x08
    uint8_t   count;                        // [OFFSET] 0x18
    uint16_t  damage;                       // [OFFSET] 0x1A
    char      pad1[0x04];
    // helpers
    bool valid() const { return descriptor && count > 0; }
    int  maxDurability() const;             // look up via item registry
};

struct PlayerInventory {
    char      pad[0x08];
    ItemStack slots[36];                    // [OFFSET] base — 36 hotbar+inv slots
    // Armour slots are stored separately in the player's equipment component
};

// ── Player list entry (for TabList / Ping) ────────────────────────────────────
struct NetworkIdentifier { uint64_t id; };

struct PlayerListEntry {
    NetworkIdentifier netId;
    std::string       xuid;
    std::string       name;
    int               ping;                 // ms
    // skin texture ptr omitted for brevity
};

// ── Actor / Entity ───────────────────────────────────────────────────────────
class Actor {
public:
    char pad0[0x100];
    Vec3 position;                          // [OFFSET] 0x100 — approximate, verify
    Vec3 velocity;                          // [OFFSET] 0x10C
    char pad1[0x50];
    AABB aabb;                              // [OFFSET] 0x160 — bounding box
    char pad2[0x80];
    float yaw;                              // [OFFSET] 0x1F0
    float pitch;                            // [OFFSET] 0x1F4

    Vec3 getEyePos() const { return { position.x, position.y + 1.62f, position.z }; }
    AABB getAABB()   const { return aabb; }

    // vtable index varies — verify with IDA
    virtual std::string getName() { return ""; }
};

// ── LocalPlayer ──────────────────────────────────────────────────────────────
class LocalPlayer : public Actor {
public:
    char pad3[0x50];
    PlayerInventory* inventory;             // [OFFSET] relative to Actor base
    char pad4[0x20];
    float health;                           // [OFFSET]
    float maxHealth;
    char pad5[0x10];
    ItemStack* armorSlots[4];               // [OFFSET] helmet=0 chest=1 legs=2 boots=3

    int   getArmorDurability(int slot) const;
    float getFOV() const;
    void  setFOV(float fov);

    // Reach: distance to targeted entity / block
    float getReachDistance() const;
    Actor* getTargetActor() const;
};

// ── Level / World ─────────────────────────────────────────────────────────────
class Level {
public:
    std::vector<PlayerListEntry> getPlayerList() const;
};

// ── ClientInstance ────────────────────────────────────────────────────────────
class ClientInstance {
public:
    // [OFFSET] Static singleton — update for each Bedrock version
    static ClientInstance* get() {
        static uintptr_t base = (uintptr_t)GetModuleHandleW(L"Minecraft.Windows.exe");
        // [OFFSET] Replace 0x0 with real offset from reverse engineering
        return *reinterpret_cast<ClientInstance**>(base + 0x0 /* TODO */);
    }

    char pad0[0x3B8];
    LocalPlayer* localPlayer;               // [OFFSET] 0x3B8 — verify
    char pad1[0x50];
    Level* level;                           // [OFFSET]

    HWND getWindow() const;
    float getGamma() const;
    void  setGamma(float g);
    bool  isInGame() const { return localPlayer != nullptr && level != nullptr; }
};

// ── RenderContext helpers ────────────────────────────────────────────────────
// Passed to visual modules for world-space rendering.
// Screen projection is done via ImGui DrawList in overlay mode.
struct RenderContext {
    float viewProj[16]; // MVP matrix — read from game renderer
    int   screenW, screenH;

    // Project a world-space point to screen coords.
    // Returns false if behind camera.
    bool worldToScreen(const Vec3& world, Vec2& out) const;
};

} // namespace SDK
