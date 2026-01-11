#include "drum_sequencer_page.h"

#include <cctype>
#include <utility>

#include "../help_dialog_frames.h"
#include "../components/bank_selection_bar.h"
#include "../components/label_option.h"
#include "../components/pattern_selection_bar.h"

namespace {
struct DrumPatternClipboard {
  bool has_pattern = false;
  DrumPatternSet pattern{};
};

DrumPatternClipboard g_drum_pattern_clipboard;

class DrumSequencerGridComponent : public Component {
 public:
  struct Callbacks {
    std::function<void(int step, int voice)> onToggle;
    std::function<void(int step)> onToggleAccent;
    std::function<int()> cursorStep;
    std::function<int()> cursorVoice;
    std::function<bool()> gridFocused;
    std::function<int()> currentStep;
  };

  DrumSequencerGridComponent(MiniAcid& mini_acid, Callbacks callbacks)
      : mini_acid_(mini_acid), callbacks_(std::move(callbacks)) {}

  bool handleEvent(UIEvent& ui_event) override {
    if (ui_event.event_type != MINIACID_MOUSE_DOWN) return false;
    if (ui_event.button != MOUSE_BUTTON_LEFT) return false;
    if (!contains(ui_event.x, ui_event.y)) return false;

    GridLayout layout{};
    if (!computeLayout(layout)) return false;
    int step = (ui_event.x - layout.grid_x) / layout.cell_w;
    if (step < 0 || step >= SEQ_STEPS) {
      return false;
    }

    if (ui_event.y >= layout.accent_y && ui_event.y < layout.accent_bottom) {
      if (callbacks_.onToggleAccent) {
        callbacks_.onToggleAccent(step);
      }
      return true;
    }

    if (ui_event.y < layout.grid_y || ui_event.y >= layout.grid_bottom) return false;
    int voice = (ui_event.y - layout.grid_y) / layout.stripe_h;
    if (voice < 0 || voice >= NUM_DRUM_VOICES) return false;
    if (callbacks_.onToggle) callbacks_.onToggle(step, voice);
    return true;
  }

  void draw(IGfx& gfx) override {
    GridLayout layout{};
    if (!computeLayout(layout)) return;

    const char* voiceLabels[NUM_DRUM_VOICES] = {"BD", "SD", "CH", "OH", "MT", "HT", "RS", "CP"};
    for (int v = 0; v < NUM_DRUM_VOICES; ++v) {
      int labelStripeH = layout.stripe_h;
      if (labelStripeH < 3) labelStripeH = 3;
      int ly = layout.grid_y + v * labelStripeH + (labelStripeH - gfx.fontHeight()) / 2;
      gfx.setTextColor(COLOR_LABEL);
      gfx.drawText(layout.bounds_x, ly, voiceLabels[v]);
    }
    gfx.setTextColor(COLOR_WHITE);

    int cursorStep = callbacks_.cursorStep ? callbacks_.cursorStep() : 0;
    int cursorVoice = callbacks_.cursorVoice ? callbacks_.cursorVoice() : 0;
    bool gridFocus = callbacks_.gridFocused ? callbacks_.gridFocused() : false;

    const bool* kick = mini_acid_.patternKickSteps();
    const bool* snare = mini_acid_.patternSnareSteps();
    const bool* hat = mini_acid_.patternHatSteps();
    const bool* openHat = mini_acid_.patternOpenHatSteps();
    const bool* midTom = mini_acid_.patternMidTomSteps();
    const bool* highTom = mini_acid_.patternHighTomSteps();
    const bool* rim = mini_acid_.patternRimSteps();
    const bool* clap = mini_acid_.patternClapSteps();
    const bool* accentSteps = mini_acid_.patternDrumAccentSteps();
    int highlight = callbacks_.currentStep ? callbacks_.currentStep() : 0;

    const bool* hits[NUM_DRUM_VOICES] = {kick, snare, hat, openHat, midTom, highTom, rim, clap};
    const IGfxColor colors[NUM_DRUM_VOICES] = {COLOR_DRUM_KICK, COLOR_DRUM_SNARE, COLOR_DRUM_HAT,
                                               COLOR_DRUM_OPEN_HAT, COLOR_DRUM_MID_TOM,
                                               COLOR_DRUM_HIGH_TOM, COLOR_DRUM_RIM, COLOR_DRUM_CLAP};

    for (int i = 0; i < SEQ_STEPS; ++i) {
      int cw = layout.cell_w;
      int cx = layout.grid_x + i * cw;
      IGfxColor fill = accentSteps[i] ? COLOR_ACCENT : COLOR_GRAY_DARKER;
      gfx.fillRect(cx, layout.accent_y, cw - 1, layout.accent_h, fill);
      gfx.drawRect(cx, layout.accent_y, cw - 1, layout.accent_h, COLOR_WHITE);
      if (highlight == i) {
        gfx.drawRect(cx - 1, layout.accent_y - 1, cw + 1, layout.accent_h + 1, COLOR_STEP_HILIGHT);
      }
    }

    // grid cells
    for (int i = 0; i < SEQ_STEPS; ++i) {
      int cw = layout.cell_w;
      int ch = layout.stripe_h;
      if (ch < 3) ch = 3;
      int cx = layout.grid_x + i * cw;
      for (int v = 0; v < NUM_DRUM_VOICES; ++v) {
        int cy = layout.grid_y + v * layout.stripe_h;
        bool hit = hits[v][i];
        IGfxColor fill = hit ? colors[v] : COLOR_GRAY;
        if (!hit && (i % 4 == 0)) {
          fill = COLOR_LIGHT_GRAY;
        }
        gfx.fillRect(cx, cy, cw - 1, ch - 1, fill);
        if (highlight == i) {
          gfx.drawRect(cx - 1, cy - 1, cw + 1, ch + 1, COLOR_STEP_HILIGHT);
        }
        if (gridFocus && i == cursorStep && v == cursorVoice) {
          gfx.drawRect(cx, cy, cw - 1, ch - 1, COLOR_STEP_SELECTED);
        }
      }
    }
  }

 private:
  struct GridLayout {
    int bounds_x = 0;
    int bounds_y = 0;
    int bounds_w = 0;
    int bounds_h = 0;
    int grid_x = 0;
    int grid_y = 0;
    int grid_w = 0;
    int grid_h = 0;
    int grid_right = 0;
    int grid_bottom = 0;
    int cell_w = 0;
    int stripe_h = 0;
    int accent_y = 0;
    int accent_h = 0;
    int accent_bottom = 0;
    int accent_gap = 0;
  };

  bool computeLayout(GridLayout& layout) const {
    const Rect& bounds = getBoundaries();
    layout.bounds_x = bounds.x;
    layout.bounds_y = bounds.y;
    layout.bounds_w = bounds.w;
    layout.bounds_h = bounds.h;
    if (layout.bounds_w <= 0 || layout.bounds_h <= 0) return false;

    int label_w = 18;
    layout.grid_x = layout.bounds_x + label_w;
    layout.grid_y = layout.bounds_y;
    layout.grid_w = layout.bounds_w - label_w;
    layout.grid_h = layout.bounds_h;
    if (layout.grid_w < 8) layout.grid_w = 8;

    layout.cell_w = layout.grid_w / SEQ_STEPS;
    if (layout.cell_w < 2) return false;
    layout.accent_h = 4;
    layout.accent_gap = 2;
    if (layout.bounds_h < (NUM_DRUM_VOICES * 3 + layout.accent_h + layout.accent_gap)) {
      layout.accent_h = 3;
      layout.accent_gap = 1;
    }
    layout.accent_y = layout.bounds_y;
    layout.accent_bottom = layout.accent_y + layout.accent_h;

    layout.grid_y = layout.bounds_y + layout.accent_h + layout.accent_gap;
    layout.grid_h = layout.bounds_h - (layout.accent_h + layout.accent_gap);
    if (layout.grid_h < NUM_DRUM_VOICES * 3) return false;

    layout.stripe_h = layout.grid_h / NUM_DRUM_VOICES;
    if (layout.stripe_h < 3) layout.stripe_h = 3;

    layout.grid_right = layout.grid_x + layout.cell_w * SEQ_STEPS;
    layout.grid_bottom = layout.grid_y + layout.stripe_h * NUM_DRUM_VOICES;
    return true;
  }

  MiniAcid& mini_acid_;
  Callbacks callbacks_;
};

class DrumSequencerMainPage : public Container {
 public:
  DrumSequencerMainPage(MiniAcid& mini_acid, AudioGuard& audio_guard);
  void draw(IGfx& gfx) override;
  bool handleEvent(UIEvent& ui_event) override;

 private:
  int activeDrumPatternCursor() const;
  int activeDrumStep() const;
  int activeDrumVoice() const;
  int activeBankCursor() const;
  void setDrumPatternCursor(int cursorIndex);
  void moveDrumCursor(int delta);
  void moveDrumCursorVertical(int delta);
  void focusPatternRow();
  void focusGrid();
  bool patternRowFocused() const;
  int patternIndexFromKey(char key) const;
  int bankIndexFromKey(char key) const;
  void setBankIndex(int bankIndex);
  bool bankRowFocused() const;
  void withAudioGuard(const std::function<void()>& fn);

  MiniAcid& mini_acid_;
  AudioGuard& audio_guard_;
  int drum_step_cursor_;
  int drum_voice_cursor_;
  int drum_pattern_cursor_;
  int bank_index_;
  int bank_cursor_;
  bool bank_focus_;
  bool drum_pattern_focus_;
  std::shared_ptr<Component> grid_component_;
  std::shared_ptr<PatternSelectionBarComponent> pattern_bar_;
  std::shared_ptr<BankSelectionBarComponent> bank_bar_;
};

class GlobalDrumSettingsPage : public Container {
 public:
  explicit GlobalDrumSettingsPage(MiniAcid& mini_acid);
  bool handleEvent(UIEvent& ui_event) override;
  void draw(IGfx& gfx) override;

 private:
  void applyDrumEngineSelection();

  MiniAcid& mini_acid_;
  std::vector<std::string> drum_engine_options_;
  std::shared_ptr<LabelOptionComponent> character_control_;
};

DrumSequencerMainPage::DrumSequencerMainPage(MiniAcid& mini_acid, AudioGuard& audio_guard)
  : mini_acid_(mini_acid),
    audio_guard_(audio_guard),
    drum_step_cursor_(0),
    drum_voice_cursor_(0),
    drum_pattern_cursor_(0),
    bank_index_(0),
    bank_cursor_(0),
    bank_focus_(false),
    drum_pattern_focus_(true) {
  int drumIdx = mini_acid_.currentDrumPatternIndex();
  if (drumIdx < 0 || drumIdx >= Bank<DrumPatternSet>::kPatterns) drumIdx = 0;
  drum_pattern_cursor_ = drumIdx;
  bank_index_ = mini_acid_.currentDrumBankIndex();
  bank_cursor_ = bank_index_;
  pattern_bar_ = std::make_shared<PatternSelectionBarComponent>("PATTERN");
  bank_bar_ = std::make_shared<BankSelectionBarComponent>("BANK", "ABCD");
  PatternSelectionBarComponent::Callbacks pattern_callbacks;
  pattern_callbacks.onSelect = [this](int index) {
    if (mini_acid_.songModeEnabled()) return;
    drum_pattern_focus_ = true;
    bank_focus_ = false;
    setDrumPatternCursor(index);
    withAudioGuard([&]() { mini_acid_.setDrumPatternIndex(index); });
  };
  pattern_bar_->setCallbacks(std::move(pattern_callbacks));
  BankSelectionBarComponent::Callbacks bank_callbacks;
  bank_callbacks.onSelect = [this](int index) {
    if (mini_acid_.songModeEnabled()) return;
    bank_focus_ = true;
    drum_pattern_focus_ = false;
    bank_cursor_ = index;
    setBankIndex(index);
  };
  bank_bar_->setCallbacks(std::move(bank_callbacks));
  DrumSequencerGridComponent::Callbacks callbacks;
  callbacks.onToggle = [this](int step, int voice) {
    focusGrid();
    drum_step_cursor_ = step;
    drum_voice_cursor_ = voice;
    withAudioGuard([&]() { mini_acid_.toggleDrumStep(voice, step); });
  };
  callbacks.onToggleAccent = [this](int step) {
    focusGrid();
    drum_step_cursor_ = step;
    withAudioGuard([&]() { mini_acid_.toggleDrumAccentStep(step); });
  };
  callbacks.cursorStep = [this]() { return activeDrumStep(); };
  callbacks.cursorVoice = [this]() { return activeDrumVoice(); };
  callbacks.gridFocused = [this]() { return !patternRowFocused() && !bankRowFocused(); };
  callbacks.currentStep = [this]() { return mini_acid_.currentStep(); };
  grid_component_ = std::make_shared<DrumSequencerGridComponent>(mini_acid_, std::move(callbacks));
  addChild(grid_component_);
}

int DrumSequencerMainPage::activeDrumPatternCursor() const {
  int idx = drum_pattern_cursor_;
  if (idx < 0) idx = 0;
  if (idx >= Bank<DrumPatternSet>::kPatterns)
    idx = Bank<DrumPatternSet>::kPatterns - 1;
  return idx;
}

int DrumSequencerMainPage::activeDrumStep() const {
  int idx = drum_step_cursor_;
  if (idx < 0) idx = 0;
  if (idx >= SEQ_STEPS) idx = SEQ_STEPS - 1;
  return idx;
}

int DrumSequencerMainPage::activeDrumVoice() const {
  int idx = drum_voice_cursor_;
  if (idx < 0) idx = 0;
  if (idx >= NUM_DRUM_VOICES) idx = NUM_DRUM_VOICES - 1;
  return idx;
}

int DrumSequencerMainPage::activeBankCursor() const {
  int cursor = bank_cursor_;
  if (cursor < 0) cursor = 0;
  if (cursor >= kBankCount) cursor = kBankCount - 1;
  return cursor;
}

void DrumSequencerMainPage::setDrumPatternCursor(int cursorIndex) {
  int cursor = cursorIndex;
  if (cursor < 0) cursor = 0;
  if (cursor >= Bank<DrumPatternSet>::kPatterns)
    cursor = Bank<DrumPatternSet>::kPatterns - 1;
  drum_pattern_cursor_ = cursor;
}

void DrumSequencerMainPage::moveDrumCursor(int delta) {
  if (mini_acid_.songModeEnabled()) {
    drum_pattern_focus_ = false;
    bank_focus_ = false;
  }
  if (bank_focus_) {
    int cursor = activeBankCursor();
    cursor = (cursor + delta) % kBankCount;
    if (cursor < 0) cursor += kBankCount;
    bank_cursor_ = cursor;
    return;
  }
  if (drum_pattern_focus_) {
    int cursor = activeDrumPatternCursor();
    cursor = (cursor + delta) % Bank<DrumPatternSet>::kPatterns;
    if (cursor < 0) cursor += Bank<DrumPatternSet>::kPatterns;
    drum_pattern_cursor_ = cursor;
    return;
  }
  int step = activeDrumStep();
  step = (step + delta) % SEQ_STEPS;
  if (step < 0) step += SEQ_STEPS;
  drum_step_cursor_ = step;
}

void DrumSequencerMainPage::moveDrumCursorVertical(int delta) {
  if (delta == 0) return;
  if (mini_acid_.songModeEnabled()) {
    drum_pattern_focus_ = false;
    bank_focus_ = false;
  }
  if (bank_focus_) {
    if (delta > 0) {
      drum_pattern_focus_ = true;
      bank_focus_ = false;
    }
    return;
  }
  if (drum_pattern_focus_) {
    if (delta > 0) {
      drum_pattern_focus_ = false;
    }
    if (delta < 0 && !mini_acid_.songModeEnabled()) {
      bank_cursor_ = bank_index_;
      bank_focus_ = true;
      drum_pattern_focus_ = false;
    }
    return;
  }

  int voice = activeDrumVoice();
  int newVoice = voice + delta;
  if (newVoice < 0 || newVoice >= NUM_DRUM_VOICES) {
    drum_pattern_focus_ = true;
    drum_pattern_cursor_ = activeDrumStep() % Bank<DrumPatternSet>::kPatterns;
    return;
  }
  drum_voice_cursor_ = newVoice;
}

void DrumSequencerMainPage::focusPatternRow() {
  setDrumPatternCursor(drum_pattern_cursor_);
  drum_pattern_focus_ = true;
  bank_focus_ = false;
}

void DrumSequencerMainPage::focusGrid() {
  drum_pattern_focus_ = false;
  bank_focus_ = false;
  drum_step_cursor_ = activeDrumStep();
  drum_voice_cursor_ = activeDrumVoice();
}

bool DrumSequencerMainPage::patternRowFocused() const {
  if (mini_acid_.songModeEnabled()) return false;
  return drum_pattern_focus_;
}

bool DrumSequencerMainPage::bankRowFocused() const {
  if (mini_acid_.songModeEnabled()) return false;
  return bank_focus_;
}

int DrumSequencerMainPage::patternIndexFromKey(char key) const {
  switch (std::tolower(static_cast<unsigned char>(key))) {
    case 'q': return 0;
    case 'w': return 1;
    case 'e': return 2;
    case 'r': return 3;
    case 't': return 4;
    case 'y': return 5;
    case 'u': return 6;
    case 'i': return 7;
    default: return -1;
  }
}

int DrumSequencerMainPage::bankIndexFromKey(char key) const {
  switch (key) {
    case '1': return 0;
    case '2': return 1;
    case '3': return 2;
    case '4': return 3;
    default: return -1;
  }
}

void DrumSequencerMainPage::setBankIndex(int bankIndex) {
  if (bankIndex < 0) bankIndex = 0;
  if (bankIndex >= kBankCount) bankIndex = kBankCount - 1;
  if (bank_index_ == bankIndex) return;
  bank_index_ = bankIndex;
  withAudioGuard([&]() { mini_acid_.setDrumBankIndex(bank_index_); });
}

void DrumSequencerMainPage::withAudioGuard(const std::function<void()>& fn) {
  if (audio_guard_) {
    audio_guard_(fn);
    return;
  }
  fn();
}

bool DrumSequencerMainPage::handleEvent(UIEvent& ui_event) {
  if (pattern_bar_ && pattern_bar_->handleEvent(ui_event)) return true;
  if (bank_bar_ && bank_bar_->handleEvent(ui_event)) return true;
  if (Container::handleEvent(ui_event)) return true;

  if (ui_event.event_type == MINIACID_APPLICATION_EVENT) {
    switch (ui_event.app_event_type) {
      case MINIACID_APP_EVENT_COPY: {
        const bool* hits[NUM_DRUM_VOICES] = {
          mini_acid_.patternKickSteps(),
          mini_acid_.patternSnareSteps(),
          mini_acid_.patternHatSteps(),
          mini_acid_.patternOpenHatSteps(),
          mini_acid_.patternMidTomSteps(),
          mini_acid_.patternHighTomSteps(),
          mini_acid_.patternRimSteps(),
          mini_acid_.patternClapSteps()
        };
        const bool* accents[NUM_DRUM_VOICES] = {
          mini_acid_.patternKickAccentSteps(),
          mini_acid_.patternSnareAccentSteps(),
          mini_acid_.patternHatAccentSteps(),
          mini_acid_.patternOpenHatAccentSteps(),
          mini_acid_.patternMidTomAccentSteps(),
          mini_acid_.patternHighTomAccentSteps(),
          mini_acid_.patternRimAccentSteps(),
          mini_acid_.patternClapAccentSteps()
        };
        for (int v = 0; v < NUM_DRUM_VOICES; ++v) {
          for (int i = 0; i < SEQ_STEPS; ++i) {
            g_drum_pattern_clipboard.pattern.voices[v].steps[i].hit = hits[v][i];
            g_drum_pattern_clipboard.pattern.voices[v].steps[i].accent = accents[v][i];
          }
        }
        g_drum_pattern_clipboard.has_pattern = true;
        return true;
      }
      case MINIACID_APP_EVENT_PASTE: {
        if (!g_drum_pattern_clipboard.has_pattern) return false;
        bool current_hits[NUM_DRUM_VOICES][SEQ_STEPS];
        bool current_accents[NUM_DRUM_VOICES][SEQ_STEPS];
        const bool* hits[NUM_DRUM_VOICES] = {
          mini_acid_.patternKickSteps(),
          mini_acid_.patternSnareSteps(),
          mini_acid_.patternHatSteps(),
          mini_acid_.patternOpenHatSteps(),
          mini_acid_.patternMidTomSteps(),
          mini_acid_.patternHighTomSteps(),
          mini_acid_.patternRimSteps(),
          mini_acid_.patternClapSteps()
        };
        const bool* accents[NUM_DRUM_VOICES] = {
          mini_acid_.patternKickAccentSteps(),
          mini_acid_.patternSnareAccentSteps(),
          mini_acid_.patternHatAccentSteps(),
          mini_acid_.patternOpenHatAccentSteps(),
          mini_acid_.patternMidTomAccentSteps(),
          mini_acid_.patternHighTomAccentSteps(),
          mini_acid_.patternRimAccentSteps(),
          mini_acid_.patternClapAccentSteps()
        };
        for (int v = 0; v < NUM_DRUM_VOICES; ++v) {
          for (int i = 0; i < SEQ_STEPS; ++i) {
            current_hits[v][i] = hits[v][i];
            current_accents[v][i] = accents[v][i];
          }
        }
        const DrumPatternSet& src = g_drum_pattern_clipboard.pattern;
        withAudioGuard([&]() {
          for (int v = 0; v < NUM_DRUM_VOICES; ++v) {
            for (int i = 0; i < SEQ_STEPS; ++i) {
              bool desiredHit = src.voices[v].steps[i].hit;
              bool desiredAccent = src.voices[v].steps[i].accent && desiredHit;
              if (current_hits[v][i] != desiredHit) {
                mini_acid_.toggleDrumStep(v, i);
              }
              if (current_accents[v][i] != desiredAccent) {
                mini_acid_.setDrumAccentStep(v, i, desiredAccent);
              }
            }
          }
        });
        return true;
      }
      default:
        return false;
    }
  }
  if (ui_event.event_type != MINIACID_KEY_DOWN) return false;
  bool handled = false;
  switch (ui_event.scancode) {
    case MINIACID_LEFT:
      moveDrumCursor(-1);
      handled = true;
      break;
    case MINIACID_RIGHT:
      moveDrumCursor(1);
      handled = true;
      break;
    case MINIACID_UP:
      moveDrumCursorVertical(-1);
      handled = true;
      break;
    case MINIACID_DOWN:
      moveDrumCursorVertical(1);
      handled = true;
      break;
    default:
      break;
  }

  if (handled) return true;

  char key = ui_event.key;
  if (!key) return false;

  /*
  int bankIdx = bankIndexFromKey(key);
  if (bankIdx >= 0) {
    setBankIndex(bankIdx);
    if (!mini_acid_.songModeEnabled()) {
      bank_focus_ = true;
      drum_pattern_focus_ = false;
    }
    return true;
  }
    */

  if (key == '\n' || key == '\r') {
    if (bankRowFocused()) {
      if (mini_acid_.songModeEnabled()) return true;
      setBankIndex(activeBankCursor());
    } else if (patternRowFocused()) {
      int cursor = activeDrumPatternCursor();
      withAudioGuard([&]() { mini_acid_.setDrumPatternIndex(cursor); });
    } else {
      int step = activeDrumStep();
      int voice = activeDrumVoice();
      withAudioGuard([&]() { mini_acid_.toggleDrumStep(voice, step); });
    }
    return true;
  }

  int patternIdx = patternIndexFromKey(key);
  bool patternKeyReserved = false;
  if (patternIdx >= 0) {
    char lowerKey = static_cast<char>(std::tolower(static_cast<unsigned char>(key)));
    patternKeyReserved = (lowerKey == 'w');
    if (!patternKeyReserved || patternRowFocused()) {
      if (mini_acid_.songModeEnabled()) return true;
      focusPatternRow();
      setDrumPatternCursor(patternIdx);
      withAudioGuard([&]() { mini_acid_.setDrumPatternIndex(patternIdx); });
      return true;
    }
  }

  char lowerKey = static_cast<char>(std::tolower(static_cast<unsigned char>(key)));
  switch (lowerKey) {
    case 'w': {
      focusGrid();
      int step = activeDrumStep();
      withAudioGuard([&]() { mini_acid_.toggleDrumAccentStep(step); });
      return true;
    }
    default:
      break;
  }

  return false;
}

void DrumSequencerMainPage::draw(IGfx& gfx) {
  bank_index_ = mini_acid_.currentDrumBankIndex();
  const Rect& bounds = getBoundaries();
  int x = bounds.x;
  int y = bounds.y;
  int w = bounds.w;
  int h = bounds.h;

  int body_y = y + 2;
  int body_h = h - 2;
  if (body_h <= 0) return;

  bool songMode = mini_acid_.songModeEnabled();
  bool bankFocus = !songMode && bankRowFocused();
  int bankCursor = activeBankCursor();

  int selectedPattern = mini_acid_.displayDrumPatternIndex();
  int patternCursor = activeDrumPatternCursor();
  bool patternFocus = !songMode && patternRowFocused();
  if (songMode && selectedPattern >= 0) patternCursor = selectedPattern;
  PatternSelectionBarComponent::State pattern_state;
  pattern_state.pattern_count = Bank<DrumPatternSet>::kPatterns;
  pattern_state.selected_index = selectedPattern;
  pattern_state.cursor_index = patternCursor;
  pattern_state.show_cursor = patternFocus;
  pattern_state.song_mode = songMode;
  pattern_bar_->setState(pattern_state);
  pattern_bar_->setBoundaries(Rect{x, body_y, w, 0});
  int pattern_bar_h = pattern_bar_->barHeight(gfx);
  pattern_bar_->setBoundaries(Rect{x, body_y, w, pattern_bar_h});
  pattern_bar_->draw(gfx);

  BankSelectionBarComponent::State bank_state;
  bank_state.bank_count = kBankCount;
  bank_state.selected_index = bank_index_;
  bank_state.cursor_index = bankCursor;
  bank_state.show_cursor = bankFocus;
  bank_state.song_mode = songMode;
  bank_bar_->setState(bank_state);
  bank_bar_->setBoundaries(Rect{x, body_y - 1, w, 0});
  int bank_bar_h = bank_bar_->barHeight(gfx);
  bank_bar_->setBoundaries(Rect{x, body_y - 1, w, bank_bar_h});
  bank_bar_->draw(gfx);

  // labels for voices
  int grid_top = body_y + pattern_bar_h + 5;
  int grid_h = body_h - (grid_top - body_y);
  if (grid_h <= 0) {
    if (grid_component_) {
      grid_component_->setBoundaries(Rect{0, 0, 0, 0});
    }
    return;
  }

  if (grid_component_) {
    grid_component_->setBoundaries(Rect{x, grid_top, w, grid_h});
  }
  Container::draw(gfx);
}
} // namespace

GlobalDrumSettingsPage::GlobalDrumSettingsPage(MiniAcid& mini_acid)
  : mini_acid_(mini_acid) {
  character_control_ = std::make_shared<LabelOptionComponent>(
      "Character", COLOR_LABEL, COLOR_WHITE);
  drum_engine_options_ = mini_acid_.getAvailableDrumEngines();
  if (drum_engine_options_.empty()) {
    drum_engine_options_ = {"808", "909", "606"};
  }
  character_control_->setOptions(drum_engine_options_);
  addChild(character_control_);
}

bool GlobalDrumSettingsPage::handleEvent(UIEvent& ui_event) {
  int before = character_control_ ? character_control_->optionIndex() : -1;
  bool handled = Container::handleEvent(ui_event);
  int after = character_control_ ? character_control_->optionIndex() : -1;
  if (before != after) {
    applyDrumEngineSelection();
  }
  return handled;
}

void GlobalDrumSettingsPage::draw(IGfx& gfx) {
  const Rect& bounds = getBoundaries();
  if (bounds.w <= 0 || bounds.h <= 0) return;
  int x = bounds.x;
  int y = bounds.y;
  int w = bounds.w;

  gfx.setTextColor(COLOR_LABEL);
  gfx.drawText(x, y, "GLOBAL SETTINGS");
  gfx.setTextColor(COLOR_WHITE);

  int row_y = y + gfx.fontHeight() + 4;
  if (character_control_) {
    character_control_->setBoundaries(Rect{x, row_y, w, gfx.fontHeight()});
  }
  Container::draw(gfx);
}

void GlobalDrumSettingsPage::applyDrumEngineSelection() {
  if (!character_control_) return;
  int index = character_control_->optionIndex();
  if (index < 0 || index >= static_cast<int>(drum_engine_options_.size())) return;
  mini_acid_.setDrumEngine(drum_engine_options_[index]);
}

DrumSequencerPage::DrumSequencerPage(IGfx& gfx, MiniAcid& mini_acid, AudioGuard& audio_guard) {
  (void)gfx;
  addPage(std::make_shared<DrumSequencerMainPage>(mini_acid, audio_guard));
  addPage(std::make_shared<GlobalDrumSettingsPage>(mini_acid));
}

const std::string & DrumSequencerPage::getTitle() const {
  static std::string title = "DRUM SEQUENCER";
  return title;
}

std::unique_ptr<MultiPageHelpDialog> DrumSequencerPage::getHelpDialog() {
  return std::make_unique<MultiPageHelpDialog>(*this);
}

int DrumSequencerPage::getHelpFrameCount() const {
  return 1;
}

void DrumSequencerPage::drawHelpFrame(IGfx& gfx, int frameIndex, Rect bounds) const {
  if (bounds.w <= 0 || bounds.h <= 0) return;
  switch (frameIndex) {
    case 0:
      drawHelpPageDrumPatternEdit(gfx, bounds.x, bounds.y, bounds.w, bounds.h);
      break;
    default:
      break;
  }
}
