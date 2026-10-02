// Copyright (c) Shinya Ishikawa. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#ifndef EFFECT_H_
#define EFFECT_H_
#define LGFX_USE_V1
#include <M5GFX.h>
#include "DrawContext.h"
#include "Drawable.h"
#include "icons/sleepy_zzz.h"
#include "icons/doubt_stamp.h"
#include "icons/happy_sparkle.h"

namespace m5avatar {

class Effect final : public Drawable {
 private:
  void drawChillMark(M5Canvas *spi, uint32_t x, uint32_t y, uint32_t r,
                     uint16_t color) {
    drawChillMark(spi, x, y, r, color, 0);
  }

  void drawChillMark(M5Canvas *spi, uint32_t x, uint32_t y, uint32_t r,
                     uint16_t color, float offset) {
    uint32_t h = r + abs(r * 0.2 * offset);
    spi->fillRect(x - (r / 2), y, 3, h / 2, color);
    spi->fillRect(x, y, 3, h * 3 / 4, color);
    spi->fillRect(x + (r / 2), y, 3, h, color);
  }

  void drawAngerMark(M5Canvas *spi, uint32_t x, uint32_t y, uint32_t r,
                     uint16_t color, uint32_t bColor) {
    drawAngerMark(spi, x, y, r, color, bColor, 0);
  }

  void drawAngerMark(M5Canvas *spi, uint32_t x, uint32_t y, uint32_t r,
                     uint16_t color, uint16_t bColor, float offset) {
    r = r + abs(r * 0.4 * offset);
    spi->fillRect(x - (r / 3), y - r, (r * 2) / 3, r * 2, color);
    spi->fillRect(x - r, y - (r / 3), r * 2, (r * 2) / 3, color);
    spi->fillRect(x - (r / 3) + 2, y - r, ((r * 2) / 3) - 4, r * 2, bColor);
    spi->fillRect(x - r, y - (r / 3) + 2, r * 2, ((r * 2) / 3) - 4, bColor);
  }

  // Restored 2026-09-28 (Stackchan project) - the original vector heart mark,
  // removed in the 2026-09-14 icon rebuild in favour of the static
  // happy_sparkle bitmap. Now used by the Affection mood's icon override
  // (see the EffectIconOverride::Heart case in draw() below) rather than
  // Expression::Happy directly - Joy ended up keeping the sparkle instead
  // after live user review, see EmotionController.cpp's own mapping note.
  void drawHeartMark(M5Canvas *spi, uint32_t x, uint32_t y, uint32_t r,
                 uint16_t color, float offset) {
    r = r + floor(r * 0.4 * offset);
    spi->fillCircle(x - r / 2, y, r / 2, color);
    spi->fillCircle(x + r / 2, y, r / 2, color);
    float a = (sqrt(2) * r) / 4.0;
    spi->fillTriangle(x, y, x - r / 2 - a, y + a, x + r / 2 + a, y + a, color);
    spi->fillTriangle(x, y + (r / 2) + 2 * a, x - r / 2 - a, y + a,
                      x + r / 2 + a, y + a, color);
  }

 public:
  // constructor
  Effect() = default;
  ~Effect() = default;
  Effect(const Effect &other) = default;
  Effect &operator=(const Effect &other) = default;
  void draw(M5Canvas *spi, BoundingRect rect, DrawContext *ctx) override {
    // Local patch, 2026-09-08 (Stackchan project): these icons used to draw
    // in primaryColor, the SAME color as the eyes/eyebrows/mouth - there was
    // no way to make e.g. the Happy heart red without recoloring the whole
    // face's line art. Started as one shared COLOR_EFFECT slot, then split
    // same-day into one slot per icon (ColorPalette.h/.cpp) so each
    // expression gets its own color instead of a single accent.
    auto effectColor = [&](const char *key) {
      return ctx->getColorDepth() == 1 ? (uint16_t)1 : ctx->getColorPalette()->get(key);
    };
    uint16_t bgColor = ctx->getColorDepth() == 1 ? ERACER_COLOR : ctx->getColorPalette()->get(COLOR_BACKGROUND);
    float offset = ctx->getBreath();
    Expression exp = ctx->getExpression();

    // Added 2026-09-28 (Stackchan project) - lets a mood force a specific
    // icon independent of the base Expression, so e.g. Affection can show
    // the heart while keeping Neutral's own eyes/mouth shape (Happy's shape
    // reads as too big/delighted for a quiet fondness) rather than switching
    // to Happy just to get its icon. Checked before the switch below so it
    // overrides whatever the current Expression would otherwise draw.
    switch (ctx->getIconOverride()) {
      case EffectIconOverride::Sparkle:
        spi->drawXBitmap(262, 32, happy_sparkle, happy_sparkle_width, happy_sparkle_height,
                          effectColor(COLOR_EFFECT_SPARKLE));
        return;
      case EffectIconOverride::Heart:
        drawHeartMark(spi, 280, 50, 12, effectColor(COLOR_EFFECT_HAPPY), offset);
        return;
      case EffectIconOverride::None:
        break;
    }

    switch (exp) {
      // Doubt/Sleepy REPLACED 2026-09-14 - the original vector marks
      // (sweat-drop/bubbles) didn't read as their intended emotions on
      // real hardware (sweat-drop reads as nervous not doubtful, bubbles
      // don't read as "tired" at all). Redrawn purikura/manga-style as real
      // pixel art (see lib/M5Stack-Avatar/src/icons/) and rendered via
      // drawXBitmap() - the same proven 1-bit bitmap path this library
      // already uses for its own eye graphics (see faces/eye_small.h) -
      // rather than adding more vector-primitive draw functions, since the
      // new shapes (a stamp outline, a kirakira sparkle burst) are more
      // detailed than fillRect/fillTriangle compose well. Positions keep
      // roughly the same on-screen corner the old marks used.
      //
      // Happy's heart REVERTED 2026-09-28 (see drawHeartMark() above) - the
      // sparkle put here by the same 2026-09-14 rebuild didn't read as well
      // for Joy on real hardware. This case is now only the fallback for a
      // plain "happy" expression set some other way (e.g. the manual
      // expression-cycle button/set_expression) - the mood reactions
      // themselves now always pass an explicit iconOverride above.
      case Expression::Doubt:
        // Position matches Sleepy's corner anchor (274->270, 90->13) per
        // user feedback - both are mutually exclusive so sharing the same
        // top-right spot is fine, and it reads more consistent than Doubt
        // sitting lower/more central like the old sweat-drop did.
        spi->drawXBitmap(270, 13, doubt_stamp, doubt_stamp_width, doubt_stamp_height,
                          effectColor(COLOR_EFFECT_DOUBT));
        break;
      case Expression::Angry:
        drawAngerMark(spi, 280, 50, 12, effectColor(COLOR_EFFECT_ANGRY), bgColor, offset);
        break;
      case Expression::Happy:
        drawHeartMark(spi, 280, 50, 12, effectColor(COLOR_EFFECT_HAPPY), offset);
        break;
      case Expression::Sad:
        drawChillMark(spi, 270, 0, 30, effectColor(COLOR_EFFECT_SAD), offset);
        break;
      case Expression::Sleepy:
        spi->drawXBitmap(258, 13, sleepy_zzz, sleepy_zzz_width, sleepy_zzz_height,
                          effectColor(COLOR_EFFECT_SLEEPY));
        break;
      default:
        // noop
        break;
    }
  }
};

}  // namespace m5avatar

#endif  // EFFECT_H_
