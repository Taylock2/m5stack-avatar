// Copyright (c) Shinya Ishikawa. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full
// license information.

#ifndef DRAWCONTEXT_H_
#define DRAWCONTEXT_H_

#define ERACER_COLOR 0x0000

#include "ColorPalette.h"
#include "Expression.h"
#include "Gaze.h"
#include "M5GFX.h"

#ifndef ARDUINO
#include <string>
typedef std::string String;
#endif  // ARDUINO

namespace m5avatar {
enum BatteryIconStatus { discharging, charging, invisible, unknown };
// Stackchan project patch: lets a caller force a specific icon overlay
// regardless of the current Expression (Effect.h's icon switch is normally
// keyed on Expression, but e.g. an "Affection" mood wants the heart icon
// while keeping Neutral's own softer eyes/mouth shape, not Happy's). None =
// fall through to the normal Expression-keyed icon.
enum class EffectIconOverride : uint8_t { None, Sparkle, Heart };
class DrawContext {
 private:
  Expression expression;
  float breath;

  // left eye
  Gaze leftGaze;
  float leftEyeOpenRatio;
  // right eye
  Gaze rightGaze;
  float rightEyeOpenRatio;

  float mouthOpenRatio;

  ColorPalette* const palette;
  String speechText;
  float rotation = 0.0;
  float scale = 1.0;
  int colorDepth = 1;
  BatteryIconStatus batteryIconStatus = BatteryIconStatus::invisible;
  int32_t batteryLevel = 0;
  const lgfx::IFont* speechFont =
      nullptr;  // = &fonts::lgfxJapanGothicP_16; //  = &fonts::efontCN_10;
  EffectIconOverride iconOverride = EffectIconOverride::None;

 public:
  DrawContext() = delete;
  DrawContext(Expression expression, float breath, ColorPalette* const palette,
              Gaze rightGaze, float rightEyeOpenRatio, Gaze leftGaze,
              float leftEyeOpenRatio, float mouthOpenRatio, String speechText,
              BatteryIconStatus batteryIconStatus, int32_t batteryLevel,
              const lgfx::IFont* speechFont);
  DrawContext(Expression expression, float breath, ColorPalette* const palette,
              Gaze rightGaze, float rightEyeOpenRatio, Gaze leftGaze,
              float leftEyeOpenRatio, float mouthOpenRatio, String speechText,
              float rotation, float scale, int colorDepth,
              BatteryIconStatus batteryIconStatus, int32_t batteryLevel,
              const lgfx::IFont* speechFont,
              EffectIconOverride iconOverride = EffectIconOverride::None);
  ~DrawContext() = default;
  DrawContext(const DrawContext& other) = delete;
  DrawContext& operator=(const DrawContext& other) = delete;
  Expression getExpression() const;
  float getBreath() const;
  float getRightEyeOpenRatio() const;
  Gaze getRightGaze() const;
  float getLeftEyeOpenRatio() const;
  Gaze getLeftGaze() const;
  float getMouthOpenRatio() const;
  float getScale() const;
  float getRotation() const;
  ColorPalette* const getColorPalette() const;
  String getspeechText() const;
  int getColorDepth() const;
  BatteryIconStatus getBatteryIconStatus() const;
  int32_t getBatteryLevel() const;
  const lgfx::IFont* getSpeechFont() const;
  EffectIconOverride getIconOverride() const;
};
}  // namespace m5avatar

#endif  // DRAWCONTEXT_H_
