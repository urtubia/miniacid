#include "miniacid_display.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <utility>
#include <string>
#if defined(ARDUINO)
#include <Arduino.h>
#else
#include <chrono>
#endif

#include "ui_colors.h"
#include "ui_utils.h"
#include "pages/drum_sequencer_page.h"
#include "pages/help_page.h"
#include "pages/pattern_edit_page.h"
#include "pages/project_page.h"
#include "pages/song_page.h"
#include "pages/tb303_params_page.h"
#include "pages/waveform_page.h"

namespace {
unsigned long nowMillis() {
#if defined(ARDUINO)
  return millis();
#else
  using clock = std::chrono::steady_clock;
  static const auto start = clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start);
  return static_cast<unsigned long>(elapsed.count());
#endif
}
} // namespace

MiniAcidDisplay::MiniAcidDisplay(IGfx& gfx, MiniAcid& mini_acid)
    : gfx_(gfx), mini_acid_(mini_acid) {
  splash_start_ms_ = nowMillis();
  gfx_.setFont(GfxFont::kFont5x7);

  pages_.push_back(std::make_unique<Synth303ParamsPage>(gfx_, mini_acid_, audio_guard_, 0));
  pages_.push_back(std::make_unique<PatternEditPage>(gfx_, mini_acid_, audio_guard_, 0));
  pages_.push_back(std::make_unique<Synth303ParamsPage>(gfx_, mini_acid_, audio_guard_, 1));
  pages_.push_back(std::make_unique<PatternEditPage>(gfx_, mini_acid_, audio_guard_, 1));
  pages_.push_back(std::make_unique<DrumSequencerPage>(gfx_, mini_acid_, audio_guard_));
  pages_.push_back(std::make_unique<SongPage>(gfx_, mini_acid_, audio_guard_));
  pages_.push_back(std::make_unique<ProjectPage>(gfx_, mini_acid_, audio_guard_));
  pages_.push_back(std::make_unique<WaveformPage>(gfx_, mini_acid_, audio_guard_));
  pages_.push_back(std::make_unique<HelpPage>());
}

MiniAcidDisplay::~MiniAcidDisplay() = default;

void MiniAcidDisplay::setAudioGuard(AudioGuard guard) {
  audio_guard_ = std::move(guard);
}

void MiniAcidDisplay::dismissSplash() {
  splash_active_ = false;
}

void MiniAcidDisplay::nextPage() {
  page_index_ = (page_index_ + 1) % static_cast<int>(pages_.size());
}

void MiniAcidDisplay::previousPage() {
  page_index_ = (page_index_ - 1 + static_cast<int>(pages_.size())) % static_cast<int>(pages_.size());
}

void MiniAcidDisplay::update() {
  if (splash_active_) {
    unsigned long now = nowMillis();
    if (now - splash_start_ms_ >= 5000UL) splash_active_ = false;
  }
  if (splash_active_) {
    drawSplashScreen();
    return;
  }

  gfx_.setFont(GfxFont::kFont5x7);
  gfx_.startWrite();
  gfx_.clear(COLOR_BLACK);
  gfx_.setTextColor(COLOR_WHITE);
  char buf[128];

  auto print = [&](int x, int y, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    gfx_.drawText(x, y, buf);
  };

  int margin = 4;

  int content_x = margin;
  int content_w = gfx_.width() - margin * 2;
  int content_y = margin;
  int content_h = 110;
  if (content_h < 0) content_h = 0;

  if (pages_[page_index_]) {
    int title_h = drawPageTitle(content_x, content_y, content_w, pages_[page_index_]->getTitle().c_str());
    pages_[page_index_]->draw(gfx_, content_x, content_y + title_h, content_w, content_h - title_h);
    if (help_dialog_visible_ && pages_[page_index_]->hasHelpDialog()) {
      drawHelpDialog(*pages_[page_index_], content_x, content_y + title_h, content_w, content_h - title_h);
    }
  }

  drawMutesSection(margin, content_h + margin, gfx_.width() - margin * 2, gfx_.height() - content_h - margin );

  int hint_w = textWidth(gfx_, "[< 0/0 >]");
  drawPageHint(gfx_.width() - hint_w - margin, margin + 2);

  gfx_.flush();
  gfx_.endWrite();
}

void MiniAcidDisplay::drawSplashScreen() {
  gfx_.startWrite();
  gfx_.clear(COLOR_BLACK);

  auto centerText = [&](int y, const char* text, IGfxColor color) {
    if (!text) return;
    int x = (gfx_.width() - textWidth(gfx_, text)) / 2;
    if (x < 0) x = 0;
    gfx_.setTextColor(color);
    gfx_.drawText(x, y, text);
  };

  gfx_.setFont(GfxFont::kFreeSerif18pt);
  int title_h = gfx_.fontHeight();
  gfx_.setFont(GfxFont::kFont5x7);
  int small_h = gfx_.fontHeight();

  int gap = 6;
  int total_h = title_h + gap + small_h * 2;
  int start_y = (gfx_.height() - total_h) / 2;
  if (start_y < 6) start_y = 6;

  gfx_.setFont(GfxFont::kFreeMono24pt);
  centerText(start_y, "ajrAcid", COLOR_ACCENT);

  gfx_.setFont(GfxFont::kFont5x7);
  int info_y = start_y + title_h + gap;
  centerText(info_y, "Use keys [ ] to move around", COLOR_WHITE);
  centerText(info_y + small_h, "Space - to start/stop sound", COLOR_WHITE);
  centerText(info_y + 2 * small_h, "ESC - for help on each page", COLOR_WHITE);

  gfx_.flush();
  gfx_.endWrite();
}

void MiniAcidDisplay::drawHelpDialog(IPage& page, int x, int y, int w, int h) {
  if (w <= 4 || h <= 4) return;

  int dialog_margin = 2;
  int dialog_x = x + dialog_margin;
  int dialog_y = y + dialog_margin;
  int dialog_w = w - dialog_margin * 2;
  int dialog_h = h - dialog_margin * 2;
  if (dialog_w <= 4 || dialog_h <= 4) return;

  gfx_.fillRect(dialog_x, dialog_y, dialog_w, dialog_h, COLOR_DARKER);
  gfx_.drawRect(dialog_x, dialog_y, dialog_w, dialog_h, COLOR_WHITE);

  int legend_h = gfx_.fontHeight() + 4;
  if (legend_h < 10) legend_h = 10;
  int legend_y = dialog_y + dialog_h - legend_h;
  if (legend_y <= dialog_y + 2) return;

  gfx_.setTextColor(COLOR_LABEL);
  gfx_.drawLine(dialog_x + 2, legend_y, dialog_x + dialog_w - 3, legend_y);

  const char* legend = "ESC to close help";
  int legend_x = dialog_x + (dialog_w - textWidth(gfx_, legend)) / 2;
  if (legend_x < dialog_x + 4) legend_x = dialog_x + 4;
  int legend_text_y = legend_y + (legend_h - gfx_.fontHeight()) / 2;
  gfx_.drawText(legend_x, legend_text_y, legend);
  gfx_.setTextColor(COLOR_WHITE);

  int body_x = dialog_x + 4;
  int body_y = dialog_y + 4;
  int body_w = dialog_w - 8;
  int body_h = legend_y - body_y - 2;
  if (body_w <= 0 || body_h <= 0) return;

  page.drawHelpBody(gfx_, body_x, body_y, body_w, body_h);
}

void MiniAcidDisplay::drawMutesSection(int x, int y, int w, int h) {
  int lh = 16;
  int yy = y;

  int rect_w = w / 10;

  gfx_.setTextColor(COLOR_WHITE);

  if (!mini_acid_.is303Muted(0)) {
    gfx_.fillRect(x + rect_w * 0 + 1, y + 1, rect_w - 2 - 1, h - 2, COLOR_MUTE_BACKGROUND);
  }
  gfx_.drawRect(x + rect_w * 0 + 1, y + 1, rect_w - 2 - 1, h - 2, COLOR_WHITE);
  gfx_.drawText(x + rect_w * 0 + 6, yy + 6, "S1");

  if (!mini_acid_.is303Muted(1)) {
    gfx_.fillRect(x + rect_w * 1 + 1, y + 1, rect_w - 2 - 1, h - 2, COLOR_MUTE_BACKGROUND);
  }
  gfx_.drawRect(x + rect_w * 1 + 1, y + 1, rect_w - 2 - 1, h - 2, COLOR_WHITE);
  gfx_.drawText(x + rect_w * 1 + 6, yy + 6, "S2");

  if (!mini_acid_.isKickMuted()) {
    gfx_.fillRect(x + rect_w * 2 + 1, y + 1, rect_w - 2 - 1, h - 2, COLOR_MUTE_BACKGROUND);
  }
  gfx_.drawRect(x + rect_w * 2 + 1, y + 1, rect_w - 2 - 1, h - 2, COLOR_WHITE);
  gfx_.drawText(x + rect_w * 2 + 6, yy + 6, "BD");

  if (!mini_acid_.isSnareMuted()) {
    gfx_.fillRect(x + rect_w * 3 + 1, y + 1, rect_w - 2 - 1, h - 2, COLOR_MUTE_BACKGROUND);
  }
  gfx_.drawRect(x + rect_w * 3 + 1, y + 1, rect_w - 2 - 1, h - 2, COLOR_WHITE);
  gfx_.drawText(x + rect_w * 3 + 6, yy + 6, "SD");

  if (!mini_acid_.isHatMuted()) {
    gfx_.fillRect(x + rect_w * 4 + 1, y + 1, rect_w - 2 - 1, h - 2, COLOR_MUTE_BACKGROUND);
  }
  gfx_.drawRect(x + rect_w * 4 + 1, y + 1, rect_w - 2 - 1, h - 2, COLOR_WHITE);
  gfx_.drawText(x + rect_w * 4 + 6, yy + 6, "CH");

  if (!mini_acid_.isOpenHatMuted()) {
    gfx_.fillRect(x + rect_w * 5 + 1, y + 1, rect_w - 2 - 1, h - 2, COLOR_MUTE_BACKGROUND);
  }
  gfx_.drawRect(x + rect_w * 5 + 1, y + 1, rect_w - 2 - 1, h - 2, COLOR_WHITE);
  gfx_.drawText(x + rect_w * 5 + 6, yy + 6, "OH");

  if (!mini_acid_.isMidTomMuted()) {
    gfx_.fillRect(x + rect_w * 6 + 1, y + 1, rect_w - 2 - 1, h - 2, COLOR_MUTE_BACKGROUND);
  }
  gfx_.drawRect(x + rect_w * 6 + 1, y + 1, rect_w - 2 - 1, h - 2, COLOR_WHITE);
  gfx_.drawText(x + rect_w * 6 + 6, yy + 6, "MT");

  if (!mini_acid_.isHighTomMuted()) {
    gfx_.fillRect(x + rect_w * 7 + 1, y + 1, rect_w - 2 - 1, h - 2, COLOR_MUTE_BACKGROUND);
  }
  gfx_.drawRect(x + rect_w * 7 + 1, y + 1, rect_w - 2 - 1, h - 2, COLOR_WHITE);
  gfx_.drawText(x + rect_w * 7 + 6, yy + 6, "HT");

  if (!mini_acid_.isRimMuted()) {
    gfx_.fillRect(x + rect_w * 8 + 1, y + 1, rect_w - 2 - 1, h - 2, COLOR_MUTE_BACKGROUND);
  }
  gfx_.drawRect(x + rect_w * 8 + 1, y + 1, rect_w - 2 - 1, h - 2, COLOR_WHITE);
  gfx_.drawText(x + rect_w * 8 + 6, yy + 6, "RS");

  if (!mini_acid_.isClapMuted()) {
    gfx_.fillRect(x + rect_w * 9 + 1, y + 1, rect_w - 2 - 1, h - 2, COLOR_MUTE_BACKGROUND);
  }
  gfx_.drawRect(x + rect_w * 9 + 1, y + 1, rect_w - 2 - 1, h - 2, COLOR_WHITE);
  gfx_.drawText(x + rect_w * 9 + 6, yy + 6, "CP");
}

int MiniAcidDisplay::drawPageTitle(int x, int y, int w, const char* text) {
  if (w <= 0 || !text) return 0;

  int transport_info_w = 60;

  w = w - transport_info_w; 

  constexpr int kTitleHeight = 11;
  constexpr int kReservedRight = 60; 


  int title_w = w;
  if (title_w > kReservedRight)
    title_w -= kReservedRight;

  gfx_.fillRect(x, y, title_w, kTitleHeight, COLOR_WHITE);

  int text_x = x + (title_w - textWidth(gfx_, text)) / 2;
  if (text_x < x) text_x = x;
  gfx_.setTextColor(COLOR_BLACK);
  gfx_.drawText(text_x, y + 1, text);
  gfx_.setTextColor(COLOR_WHITE);

  {
    int info_x = x + title_w + 2;
    int info_y = y + 1;
    char buf[32];
    IGfxColor transportColor = mini_acid_.songModeEnabled() ? IGfxColor::Green() : IGfxColor::Blue();
    if (mini_acid_.isPlaying()) {
      gfx_.fillRect(info_x, info_y - 1, transport_info_w - 4, kTitleHeight, transportColor);
    } else {
      gfx_.fillRect(info_x, info_y - 1, transport_info_w - 4, kTitleHeight, IGfxColor::Gray());
    }
    snprintf(buf, sizeof(buf), "  %0.0fbpm", mini_acid_.bpm());
    if (mini_acid_.isPlaying()) {
      IGfxColor textColor = mini_acid_.songModeEnabled() ? IGfxColor::Black() : IGfxColor::White();
      gfx_.setTextColor(textColor);
    }
    gfx_.drawText(info_x, info_y, buf);
    gfx_.setTextColor(IGfxColor::White());
  }
  return kTitleHeight;
}

void MiniAcidDisplay::drawPageHint(int x, int y) {
  char buf[32];
  snprintf(buf, sizeof(buf), "[< %d/%d >]", page_index_ + 1, static_cast<int>(pages_.size()));
  gfx_.setTextColor(COLOR_LABEL);
  gfx_.drawText(x, y, buf);
  gfx_.setTextColor(COLOR_WHITE);
}

bool MiniAcidDisplay::handleEvent(UIEvent event) {
  if (event.event_type == MINIACID_KEY_DOWN && event.scancode == MINIACID_ESCAPE) {
    if (page_index_ >= 0 && page_index_ < static_cast<int>(pages_.size()) && pages_[page_index_]) {
      if (pages_[page_index_]->hasHelpDialog()) {
        help_dialog_visible_ = !help_dialog_visible_;
        update();
        return true;
      }
    }
  }

  if (help_dialog_visible_) {
    if (page_index_ >= 0 && page_index_ < static_cast<int>(pages_.size()) && pages_[page_index_]) {
      bool handled = pages_[page_index_]->handleHelpEvent(event);
      if (handled) update();
    }
    return true;
  }

  switch(event.event_type) {
    case MINIACID_KEY_DOWN:
      if (event.key == '-') {
        mini_acid_.adjustParameter(MiniAcidParamId::MainVolume, -5);
        return true;
      } else if (event.key == '=') {
        mini_acid_.adjustParameter(MiniAcidParamId::MainVolume, 5);
        return true;
      }
      break;
    default:
      break;
  }

  if (page_index_ >= 0 && page_index_ < static_cast<int>(pages_.size()) && pages_[page_index_]) {
    bool handled = pages_[page_index_]->handleEvent(event);
    if (handled) update();
    return handled;
  }
  return false;
}
