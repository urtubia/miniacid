#pragma once

#include "display.h"

inline constexpr IGfxColor COLOR_WHITE = IGfxColor(0xFFFFFF);
inline constexpr IGfxColor COLOR_BLACK = IGfxColor(0x000000);
inline constexpr IGfxColor COLOR_GRAY = IGfxColor(0x202020);
inline constexpr IGfxColor COLOR_DARKER = IGfxColor(0x101010);
inline constexpr IGfxColor COLOR_WAVE = IGfxColor(0x00FF90);
inline constexpr IGfxColor COLOR_PANEL = IGfxColor(0x181818);
inline constexpr IGfxColor COLOR_ACCENT = IGfxColor(0x10B0FF);
inline constexpr IGfxColor COLOR_SLIDE = IGfxColor(0x0090FF);
inline constexpr IGfxColor COLOR_303_NOTE = IGfxColor(0x00606F);
inline constexpr IGfxColor COLOR_STEP_HILIGHT = IGfxColor(0xFFFFFF);
inline constexpr IGfxColor COLOR_DRUM_KICK = IGfxColor(0xB03030);
inline constexpr IGfxColor COLOR_DRUM_SNARE = IGfxColor(0x7090FF);
inline constexpr IGfxColor COLOR_DRUM_HAT = IGfxColor(0xB0B0B0);
inline constexpr IGfxColor COLOR_DRUM_OPEN_HAT = IGfxColor(0xE3C14B);
inline constexpr IGfxColor COLOR_DRUM_MID_TOM = IGfxColor(0x7DC7FF);
inline constexpr IGfxColor COLOR_DRUM_HIGH_TOM = IGfxColor(0x9AE3FF);
inline constexpr IGfxColor COLOR_DRUM_RIM = IGfxColor(0xFF7D8D);
inline constexpr IGfxColor COLOR_DRUM_CLAP = IGfxColor(0xFFC1E0);
inline constexpr IGfxColor COLOR_LABEL = IGfxColor(0xCCCCCC);
inline constexpr IGfxColor COLOR_MUTE_BACKGROUND = IGfxColor::Purple();
inline constexpr IGfxColor COLOR_GRAY_DARKER = IGfxColor(0x202020);
inline constexpr IGfxColor COLOR_KNOB_1 = IGfxColor::Orange();
inline constexpr IGfxColor COLOR_KNOB_2 = IGfxColor::Cyan();
inline constexpr IGfxColor COLOR_KNOB_3 = IGfxColor::Magenta();
inline constexpr IGfxColor COLOR_KNOB_4 = IGfxColor::Green();
inline constexpr IGfxColor COLOR_KNOB_CONTROL = IGfxColor::Yellow();
inline constexpr IGfxColor COLOR_STEP_SELECTED = IGfxColor::Orange();
inline constexpr IGfxColor COLOR_PATTERN_SELECTED_FILL = IGfxColor::Blue();
inline constexpr IGfxColor WAVE_COLORS[] = {
  COLOR_WAVE,
  IGfxColor::Cyan(),
  IGfxColor::Magenta(),
  IGfxColor::Yellow(),
  IGfxColor::White(),
};
inline constexpr int NUM_WAVE_COLORS = static_cast<int>(sizeof(WAVE_COLORS) / sizeof(WAVE_COLORS[0]));

