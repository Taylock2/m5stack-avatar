// Copyright (c) Shinya Ishikawa. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.
//
// Locally added (not upstream), 2026-09-29 (Stackchan project) - "tears building
// up" while Sadness is shown, loosely inspired by a small particle-fluid-sim
// sketch the user found (gravity + damping, grid-repel for many-particle contact)
// rather than a static teardrop shape. Settled (after several rounds of live
// feedback) on large manga-style teardrops, up to two per eye at once - one
// growing in place while the previous one is still falling, but never two
// growing at the same time (that read as "too fast"/a spray rather than
// individual tears) - no repulsion/multi-particle contact needed at this scale
// (see TearEffect.cpp for the fuller version history).

#ifndef TEAR_EFFECT_H_
#define TEAR_EFFECT_H_
#define LGFX_USE_V1
#include <M5GFX.h>

namespace m5avatar {
namespace TearEffect {

// State lives here as file-scope statics (see TearEffect.cpp), not as per-Eye-
// instance members, purely so FaceController's own periodic update() tick (real
// elapsed dt + IMU-derived, face-rotation-counter-rotated gravity) can drive the
// physics without new plumbing through Avatar->Face->Eye just to reach two
// instances - Eye::draw() only ever READS current particle state via draw() below.

// true allows a new drop to start building once the previous one (if any) has
// fully fallen; false just stops starting NEW ones - a drop already
// building/falling finishes naturally rather than vanishing abruptly.
void setActive(bool active);

// Called once per frame (not per-eye, unlike draw() below) - gx/gy is screen-space
// gravity DIRECTION (not necessarily unit length; see FaceController's caller for
// how it's derived from the IMU and counter-rotated for any active face rotation).
void update(float dtSeconds, float gx, float gy);

// Called from Eye::draw(), once per eye per frame - anchorX/Y is that eye's
// current on-screen center (post-gaze-offset), r its radius (assumed effectively
// constant across a face's lifetime - cached internally, so the first frame or
// two after startup use an approximate default). No-ops harmlessly if that eye
// has no drop currently building/falling.
void draw(M5Canvas *spi, int anchorX, int anchorY, int r, bool isLeft);

}  // namespace TearEffect
}  // namespace m5avatar

#endif  // TEAR_EFFECT_H_
