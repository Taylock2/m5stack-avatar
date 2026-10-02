#include "TearEffect.h"

#include <cmath>
#include <cstdint>

namespace m5avatar {
namespace TearEffect {
namespace {

// Big, over-the-top manga-style tears per 2026-09-29 live feedback (the original
// fine spray of tiny 1px dots didn't read at all) - large teardrop shapes that
// grow in place ("welling up") before dropping and streaming down.
//
// Pacing settled after two rounds of live feedback: not strictly one-at-a-time
// (a drop is allowed to start building again once the previous one has begun
// FALLING, so a new one can be welling up while the last one is still visibly
// on its way down) but also not the original fast/concurrent spray - the fix
// there was simply a much slower BUILD_HOLD_MS, not limiting concurrency itself.
// Two slots per eye is enough for "one building + one falling" at a time.
constexpr int SLOTS_PER_EYE = 2;
constexpr uint32_t BUILD_HOLD_MS = 2200;  // how long a drop grows in place before falling - the pacing knob
constexpr float TEAR_SIZE = 6.0f;         // bulb radius in pixels at full size - the "how big" knob
constexpr float DAMPING_PER_SEC = 0.85f;  // fraction of velocity RETAINED per second, while falling
constexpr float GRAVITY_SCALE = 55.0f;
constexpr int SUBSTEPS = 3;
constexpr float FALL_DESPAWN_PX = 170.0f;  // fixed pixels of fall - most of a 240px-tall screen

struct Drop {
    bool active = false;   // exists at all (building or falling)
    bool falling = false;  // false = still growing in place, true = released and falling
    uint32_t holdMs = 0;   // elapsed time growing, while !falling
    float x = 0.0f, y = 0.0f, vx = 0.0f, vy = 0.0f;
};

struct EyeState {
    Drop drops[SLOTS_PER_EYE];
};

EyeState g_left, g_right;
bool g_active = false;
float g_lastKnownR = 8.0f;             // updated by draw(), used by the next update() tick
float g_gravDx = 0.0f, g_gravDy = 1.0f;  // updated by update(), used by the next draw() call - already unit length (sin/cos of an angle)

void stepEye(EyeState& eye, float dt, float gx, float gy, float r) {
    // poolTopY is the drop's fixed anchor/duct point (see drawTear()) - the
    // tapering point sits exactly AT this position now (not offset further up
    // from it, unlike the earlier center-scaling version), so this only needs
    // to clear the eye's own bottom edge (r) plus a small margin, not also the
    // tear's own reach.
    const float poolTopY = r + 3.0f;
    const float fallDespawnY = poolTopY + FALL_DESPAWN_PX;
    const uint32_t dtMs = (uint32_t)(dt * 1000.0f);
    const float subDt = dt / SUBSTEPS;
    const float damping = powf(DAMPING_PER_SEC, subDt);

    bool anyBuilding = false;
    for (auto& d : eye.drops) {
        if (d.active && !d.falling) anyBuilding = true;
    }

    for (auto& d : eye.drops) {
        if (!d.active) {
            // A free slot - start a new drop building here, but only if no OTHER
            // drop in this eye is still building (a falling one doesn't block
            // this - that's the whole point of allowing some overlap).
            if (g_active && !anyBuilding) {
                d.active = true;
                d.falling = false;
                d.holdMs = 0;
                d.x = 0.0f;
                d.y = poolTopY;
                d.vx = d.vy = 0.0f;
                anyBuilding = true;
            }
            continue;
        }

        if (!d.falling) {
            // Growing in place - no physics yet, just a timer (see drawTear()'s
            // caller for how holdMs maps to rendered size).
            d.holdMs += dtMs;
            if (d.holdMs >= BUILD_HOLD_MS) {
                d.falling = true;
            }
            continue;
        }

        // Falling - free gravity physics, no walls (nothing left to hold it in place).
        for (int s = 0; s < SUBSTEPS; s++) {
            d.vx += gx * GRAVITY_SCALE * subDt;
            d.vy += gy * GRAVITY_SCALE * subDt;
            d.vx *= damping;
            d.vy *= damping;
            d.x += d.vx * subDt;
            d.y += d.vy * subDt;
        }
        if (d.y > fallDespawnY) {
            d.active = false;  // fell far enough - recycle this slot
        }
    }
}

// Classic manga teardrop silhouette: a round bulb with a tapering point above
// it (reuses the same fillCircle/fillTriangle vector-primitive approach Effect.h's
// drawHeartMark() already established), plus a small highlight gleam near the top
// of the bulb - the detail that sells "glossy anime tear" rather than a plain blob.
//
// Rotates with gravity (dx,dy - a unit vector, "which way is down" right now) so
// the bulb (rounded base) always leads INTO the gravity direction and the
// tapering point always trails opposite it, like a real drop, rather than
// staying fixed pointing straight up regardless of tilt.
//
// (anchorX,anchorY) is the FIXED point the tear hangs FROM (the tear duct) - a
// real bug found live: the first version took the BULB's center as fixed and
// scaled the whole shape outward from there as size grew, so the visible
// starting point drifted away from the eye as it "built up" instead of staying
// put while the bulb grows and extends below/away from it, the way an actual
// forming tear does. The bulb center is now DERIVED (anchor + 1.9*size along
// down_dir) rather than passed in - it's the thing that moves/grows away from
// the anchor as size increases, not the reference point itself.
void drawTear(M5Canvas* spi, int anchorX, int anchorY, float size, float dx, float dy,
              uint16_t color, uint16_t highlightColor) {
    int r = (int)lroundf(size);
    if (r < 1) return;
    const float sx = -dy, sy = dx;  // side_dir: down_dir rotated 90deg CCW

    int bulbX = anchorX + (int)lroundf(dx * r * 1.9f);
    int bulbY = anchorY + (int)lroundf(dy * r * 1.9f);

    spi->fillCircle(bulbX, bulbY, r, color);
    int baseX = bulbX - (int)lroundf(dx * r * 0.15f);
    int baseY = bulbY - (int)lroundf(dy * r * 0.15f);
    int leftX = baseX + (int)lroundf(sx * r);
    int leftY = baseY + (int)lroundf(sy * r);
    int rightX = baseX - (int)lroundf(sx * r);
    int rightY = baseY - (int)lroundf(sy * r);
    // The tapering point IS the anchor now, not a separate offset from it.
    spi->fillTriangle(leftX, leftY, rightX, rightY, anchorX, anchorY, color);

    int highlightR = r / 4;
    if (highlightR < 1) highlightR = 1;
    // -(down_dir)*(r/3) + side_dir*(r/3) relative to the bulb center, expanded
    // directly in dx/dy (checked this reduces to the original (x-r/3, y-r/3) at
    // down_dir=(0,1) before using it).
    int hlX = bulbX - (int)lroundf((dx + dy) * r / 3.0f);
    int hlY = bulbY + (int)lroundf((dx - dy) * r / 3.0f);
    spi->fillCircle(hlX, hlY, highlightR, highlightColor);
}

}  // namespace

void setActive(bool active) { g_active = active; }

void update(float dtSeconds, float gx, float gy) {
    g_gravDx = gx;
    g_gravDy = gy;
    stepEye(g_left, dtSeconds, gx, gy, g_lastKnownR);
    stepEye(g_right, dtSeconds, gx, gy, g_lastKnownR);
}

void draw(M5Canvas* spi, int anchorX, int anchorY, int r, bool isLeft) {
    g_lastKnownR = (float)r;
    EyeState& eye = isLeft ? g_left : g_right;
    constexpr uint16_t TEAR_COLOR = 0x2D7C;       // saturated blue-cyan
    constexpr uint16_t HIGHLIGHT_COLOR = 0xCFFF;  // pale near-white gleam

    for (auto& d : eye.drops) {
        if (!d.active) continue;
        // Growing in place: render size ramps 0->TEAR_SIZE over BUILD_HOLD_MS -
        // this IS the "building up" visual, not just a fixed-size shape sitting
        // still.
        float size = d.falling ? TEAR_SIZE : TEAR_SIZE * ((float)d.holdMs / (float)BUILD_HOLD_MS);
        int px = anchorX + (int)lroundf(d.x);
        int py = anchorY + (int)lroundf(d.y);
        drawTear(spi, px, py, size, g_gravDx, g_gravDy, TEAR_COLOR, HIGHLIGHT_COLOR);
    }
}

}  // namespace TearEffect
}  // namespace m5avatar
