#include "song_page.h"

#include <cctype>
#include <cstdio>
#include <vector>

#include "../help_dialog_frames.h"
#include "../components/mode_button.h"

namespace {
struct SongPatternClipboard {
  bool has_pattern = false;
  int pattern_index = -1;
};

struct SongAreaClipboard {
  bool has_area = false;
  int rows = 0;
  int tracks = 0;
  std::vector<int> pattern_indices;
};

enum class UndoActionType {
  None,
  Paste,
  Cut,
  Delete,
};

struct UndoCell {
  int row;
  int track;
  int pattern_index;
};

struct UndoHistory {
  UndoActionType action_type = UndoActionType::None;
  std::vector<UndoCell> cells;
  
  void clear() {
    action_type = UndoActionType::None;
    cells.clear();
  }
  
  void saveSingleCell(int row, int track, int pattern_index) {
    cells.clear();
    cells.push_back({row, track, pattern_index});
  }
  
  void saveArea(int min_row, int max_row, int min_track, int max_track, 
                const std::vector<int>& pattern_indices) {
    cells.clear();
    int idx = 0;
    for (int r = min_row; r <= max_row; ++r) {
      for (int t = min_track; t <= max_track; ++t) {
        if (idx < static_cast<int>(pattern_indices.size())) {
          cells.push_back({r, t, pattern_indices[idx]});
        }
        ++idx;
      }
    }
  }
};

SongPatternClipboard g_song_pattern_clipboard;
SongAreaClipboard g_song_area_clipboard;
UndoHistory g_undo_history;
} // namespace

SongPage::SongPage(IGfx& gfx, MiniAcid& mini_acid, AudioGuard& audio_guard)
  : gfx_(gfx),
    mini_acid_(mini_acid),
    audio_guard_(audio_guard),
    cursor_row_(0),
    cursor_track_(0),
    scroll_row_(0),
    has_selection_(false),
    selection_start_row_(0),
    selection_start_track_(0) {
  cursor_row_ = mini_acid_.currentSongPosition();
  if (cursor_row_ < 0) cursor_row_ = 0;
  int maxSongRow = mini_acid_.songLength() - 1;
  if (maxSongRow < 0) maxSongRow = 0;
  if (cursor_row_ > maxSongRow) cursor_row_ = maxSongRow;
  if (cursor_row_ >= Song::kMaxPositions) cursor_row_ = Song::kMaxPositions - 1;
}

int SongPage::clampCursorRow(int row) const {
  int maxRow = Song::kMaxPositions - 1;
  if (maxRow < 0) maxRow = 0;
  if (row < 0) row = 0;
  if (row > maxRow) row = maxRow;
  return row;
}

int SongPage::cursorRow() const {
  return clampCursorRow(cursor_row_);
}

int SongPage::cursorTrack() const {
  int track = cursor_track_;
  if (track < 0) track = 0;
  if (track > 4) track = 4;
  return track;
}

bool SongPage::cursorOnModeButton() const { return cursorTrack() == 4; }
bool SongPage::cursorOnPlayheadLabel() const { return cursorTrack() == 3; }

void SongPage::startSelection() {
  has_selection_ = true;
  selection_start_row_ = cursor_row_;
  selection_start_track_ = cursor_track_;
}

void SongPage::updateSelection() {
  if (!has_selection_) {
    startSelection();
  }
}

void SongPage::clearSelection() {
  has_selection_ = false;
  if (mini_acid_.loopModeEnabled()) {
    withAudioGuard([&]() { mini_acid_.setLoopMode(false); });
  }
}

void SongPage::updateLoopRangeFromSelection() {
  if (!mini_acid_.loopModeEnabled()) return;
  if (!has_selection_) {
    withAudioGuard([&]() { mini_acid_.setLoopMode(false); });
    return;
  }
  int min_row, max_row, min_track, max_track;
  getSelectionBounds(min_row, max_row, min_track, max_track);
  (void)min_track;
  (void)max_track;
  withAudioGuard([&]() { mini_acid_.setLoopRange(min_row, max_row); });
}

void SongPage::getSelectionBounds(int& min_row, int& max_row, int& min_track, int& max_track) const {
  if (!has_selection_) {
    min_row = max_row = cursor_row_;
    min_track = max_track = cursor_track_;
    return;
  }
  min_row = std::min(selection_start_row_, cursor_row_);
  max_row = std::max(selection_start_row_, cursor_row_);
  min_track = std::min(selection_start_track_, cursor_track_);
  max_track = std::max(selection_start_track_, cursor_track_);
}

void SongPage::moveCursorHorizontal(int delta, bool extend_selection) {
  if (extend_selection) {
    updateSelection();
  } else {
    clearSelection();
  }
  int track = cursorTrack();
  track += delta;
  if (track < 0) track = 0;
  if (track > 4) track = 4;
  cursor_track_ = track;
  syncSongPositionToCursor();
  if (extend_selection) {
    updateLoopRangeFromSelection();
  }
}

void SongPage::moveCursorVertical(int delta, bool extend_selection) {
  if (delta == 0) return;
  if (cursorOnPlayheadLabel() || cursorOnModeButton()) {
    moveCursorHorizontal(delta, extend_selection);
    return;
  }
  if (extend_selection) {
    updateSelection();
  } else {
    clearSelection();
  }
  int row = cursorRow();
  row += delta;
  row = clampCursorRow(row);
  cursor_row_ = row;
  syncSongPositionToCursor();
  if (extend_selection) {
    updateLoopRangeFromSelection();
  }
}

void SongPage::syncSongPositionToCursor() {
  if (mini_acid_.songModeEnabled() && !mini_acid_.isPlaying()) {
    withAudioGuard([&]() { mini_acid_.setSongPosition(cursorRow()); });
  }
}

void SongPage::withAudioGuard(const std::function<void()>& fn) {
  if (audio_guard_) {
    audio_guard_(fn);
    return;
  }
  fn();
}

SongTrack SongPage::trackForColumn(int col, bool& valid) const {
  valid = true;
  if (col == 0) return SongTrack::SynthA;
  if (col == 1) return SongTrack::SynthB;
  if (col == 2) return SongTrack::Drums;
  valid = false;
  return SongTrack::Drums;
}

int SongPage::bankIndexForTrack(SongTrack track) const {
  switch (track) {
    case SongTrack::SynthA:
      return mini_acid_.current303BankIndex(0);
    case SongTrack::SynthB:
      return mini_acid_.current303BankIndex(1);
    case SongTrack::Drums:
      return mini_acid_.currentDrumBankIndex();
    default:
      return 0;
  }
}

int SongPage::patternIndexFromKey(char key) const {
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

bool SongPage::adjustSongPatternAtCursor(int delta) {
  bool trackValid = false;
  SongTrack track = trackForColumn(cursorTrack(), trackValid);
  if (!trackValid) return false;
  int row = cursorRow();
  int current = mini_acid_.songPatternAt(row, track);
  int maxPattern = kSongPatternCount - 1;
  int next = current;
  if (delta > 0) next = current < 0 ? 0 : current + 1;
  else if (delta < 0) next = current < 0 ? -1 : current - 1;
  if (next > maxPattern) next = maxPattern;
  if (next < -1) next = -1;
  if (next == current) return false;
  withAudioGuard([&]() {
    if (next < 0) mini_acid_.clearSongPattern(row, track);
    else mini_acid_.setSongPattern(row, track, next);
    if (mini_acid_.songModeEnabled() && !mini_acid_.isPlaying()) {
      mini_acid_.setSongPosition(row);
    }
  });
  return true;
}

bool SongPage::adjustSongPlayhead(int delta) {
  int len = mini_acid_.songLength();
  if (len < 1) len = 1;
  int maxPos = len - 1;
  if (maxPos < 0) maxPos = 0;
  if (maxPos >= Song::kMaxPositions) maxPos = Song::kMaxPositions - 1;
  int current = mini_acid_.songPlayheadPosition();
  int next = current + delta;
  if (next < 0) next = 0;
  if (next > maxPos) next = maxPos;
  if (next == current) return false;
  withAudioGuard([&]() { mini_acid_.setSongPosition(next); });
  setScrollToPlayhead(next);
  return true;
}

bool SongPage::assignPattern(int patternIdx) {
  bool trackValid = false;
  SongTrack track = trackForColumn(cursorTrack(), trackValid);
  if (!trackValid || cursorOnModeButton()) return false;
  int row = cursorRow();
  int bankIndex = bankIndexForTrack(track);
  int combined = songPatternFromBank(bankIndex, patternIdx);
  withAudioGuard([&]() {
    mini_acid_.setSongPattern(row, track, combined);
    if (mini_acid_.songModeEnabled() && !mini_acid_.isPlaying()) {
      mini_acid_.setSongPosition(row);
    }
  });
  return true;
}

bool SongPage::clearPattern() {
  bool trackValid = false;
  SongTrack track = trackForColumn(cursorTrack(), trackValid);
  if (!trackValid) return false;
  int row = cursorRow();
  
  // Save undo state
  int current_pattern = mini_acid_.songPatternAt(row, track);
  g_undo_history.action_type = UndoActionType::Delete;
  g_undo_history.saveSingleCell(row, cursorTrack(), current_pattern);
  
  withAudioGuard([&]() {
    mini_acid_.clearSongPattern(row, track);
    if (mini_acid_.songModeEnabled() && !mini_acid_.isPlaying()) {
      mini_acid_.setSongPosition(row);
    }
  });
  return true;
}

bool SongPage::toggleSongMode() {
  withAudioGuard([&]() { mini_acid_.toggleSongMode(); });
  return true;
}

bool SongPage::toggleLoopMode() {
  if (mini_acid_.loopModeEnabled()) {
    withAudioGuard([&]() { mini_acid_.setLoopMode(false); });
    return true;
  }
  if (!has_selection_) return false;
  int min_row, max_row, min_track, max_track;
  getSelectionBounds(min_row, max_row, min_track, max_track);
  (void)min_track;
  (void)max_track;
  withAudioGuard([&]() {
    mini_acid_.setLoopRange(min_row, max_row);
    mini_acid_.setLoopMode(true);
  });
  return true;
}

void SongPage::initModeButton(int x, int y, int w, int h) {
  auto button = std::make_shared<ModeButton>(
    [this]() { return mini_acid_.songModeEnabled(); },
    [this]() { toggleSongMode(); }
  );
  button->setBoundaries(Rect(x, y, w, h));
  mode_button_container_.addChild(button);
  mode_button_initialized_ = true;
}

void SongPage::setScrollToPlayhead(int playhead) {
  if (playhead < 0) playhead = 0;
  int rowHeight = gfx_.fontHeight() + 6;
  if (rowHeight < 8) rowHeight = 8;
  int visibleRows = (gfx_.height() - 20) / rowHeight;
  if (visibleRows < 1) visibleRows = 1;
  if (scroll_row_ > playhead) scroll_row_ = playhead;
  if (scroll_row_ + visibleRows - 1 < playhead) {
    scroll_row_ = playhead - visibleRows + 1;
    if (scroll_row_ < 0) scroll_row_ = 0;
  }
}

bool SongPage::handleEvent(UIEvent& ui_event) {
  // Handle mode button clicks
  if (mode_button_initialized_ && mode_button_container_.handleEvent(ui_event)) {
    return true;
  }

  if (ui_event.event_type == MINIACID_APPLICATION_EVENT) {
    bool trackValid = false;
    SongTrack track = trackForColumn(cursorTrack(), trackValid);
    if (!trackValid || cursorOnModeButton() || cursorOnPlayheadLabel()) {
      return false;
    }
    switch (ui_event.app_event_type) {
      case MINIACID_APP_EVENT_COPY: {
        if (has_selection_) {
          // Copy selected area
          int min_row, max_row, min_track, max_track;
          getSelectionBounds(min_row, max_row, min_track, max_track);
          
          // Only copy valid track columns (0-2)
          if (max_track > 2) max_track = 2;
          if (min_track > 2) return false;
          
          int rows = max_row - min_row + 1;
          int tracks = max_track - min_track + 1;
          g_song_area_clipboard.rows = rows;
          g_song_area_clipboard.tracks = tracks;
          g_song_area_clipboard.pattern_indices.clear();
          g_song_area_clipboard.pattern_indices.reserve(rows * tracks);
          
          for (int r = min_row; r <= max_row; ++r) {
            for (int t = min_track; t <= max_track; ++t) {
              bool valid = false;
              SongTrack song_track = trackForColumn(t, valid);
              int pattern = valid ? mini_acid_.songPatternAt(r, song_track) : -1;
              g_song_area_clipboard.pattern_indices.push_back(pattern);
            }
          }
          g_song_area_clipboard.has_area = true;
          g_song_pattern_clipboard.has_pattern = false; // Clear single-cell clipboard
        } else {
          // Copy single cell
          int row = cursorRow();
          g_song_pattern_clipboard.pattern_index = mini_acid_.songPatternAt(row, track);
          g_song_pattern_clipboard.has_pattern = true;
          g_song_area_clipboard.has_area = false; // Clear area clipboard
        }
        return true;
      }
      case MINIACID_APP_EVENT_CUT: {
        if (has_selection_) {
          // Copy selected area first
          int min_row, max_row, min_track, max_track;
          getSelectionBounds(min_row, max_row, min_track, max_track);
          
          // Only copy/clear valid track columns (0-2)
          if (max_track > 2) max_track = 2;
          if (min_track > 2) return false;
          
          int rows = max_row - min_row + 1;
          int tracks = max_track - min_track + 1;
          g_song_area_clipboard.rows = rows;
          g_song_area_clipboard.tracks = tracks;
          g_song_area_clipboard.pattern_indices.clear();
          g_song_area_clipboard.pattern_indices.reserve(rows * tracks);
          
          // Save undo state and copy/clear
          std::vector<int> old_patterns;
          old_patterns.reserve(rows * tracks);
          
          withAudioGuard([&]() {
            for (int r = min_row; r <= max_row; ++r) {
              for (int t = min_track; t <= max_track; ++t) {
                bool valid = false;
                SongTrack song_track = trackForColumn(t, valid);
                if (valid) {
                  int pattern = mini_acid_.songPatternAt(r, song_track);
                  g_song_area_clipboard.pattern_indices.push_back(pattern);
                  old_patterns.push_back(pattern);
                  mini_acid_.clearSongPattern(r, song_track);
                }
              }
            }
          });
          
          g_song_area_clipboard.has_area = true;
          g_song_pattern_clipboard.has_pattern = false; // Clear single-cell clipboard
          
          // Save undo history
          g_undo_history.action_type = UndoActionType::Cut;
          g_undo_history.saveArea(min_row, max_row, min_track, max_track, old_patterns);
        } else {
          // Cut single cell
          int row = cursorRow();
          int current_pattern = mini_acid_.songPatternAt(row, track);
          
          g_song_pattern_clipboard.pattern_index = current_pattern;
          g_song_pattern_clipboard.has_pattern = true;
          g_song_area_clipboard.has_area = false; // Clear area clipboard
          
          // Save undo state
          g_undo_history.action_type = UndoActionType::Cut;
          g_undo_history.saveSingleCell(row, cursorTrack(), current_pattern);
          
          withAudioGuard([&]() {
            mini_acid_.clearSongPattern(row, track);
          });
        }
        return true;
      }
      case MINIACID_APP_EVENT_PASTE: {
        if (g_song_area_clipboard.has_area) {
          // Paste area
          int start_row = cursorRow();
          int start_track = cursorTrack();
          if (start_track > 2) return false;
          
          // Save old patterns for undo
          std::vector<int> old_patterns;
          int min_row = start_row;
          int max_row = start_row + g_song_area_clipboard.rows - 1;
          int min_track = start_track;
          int max_track = start_track + g_song_area_clipboard.tracks - 1;
          if (max_track > 2) max_track = 2;
          
          for (int r = min_row; r <= max_row; ++r) {
            for (int t = min_track; t <= max_track; ++t) {
              if (r >= Song::kMaxPositions) {
                old_patterns.push_back(-1);
                continue;
              }
              bool valid = false;
              SongTrack song_track = trackForColumn(t, valid);
              int pattern = valid ? mini_acid_.songPatternAt(r, song_track) : -1;
              old_patterns.push_back(pattern);
            }
          }
          
          withAudioGuard([&]() {
            int idx = 0;
            for (int r = 0; r < g_song_area_clipboard.rows; ++r) {
              for (int t = 0; t < g_song_area_clipboard.tracks; ++t) {
                int target_row = start_row + r;
                int target_track = start_track + t;
                if (target_row >= Song::kMaxPositions || target_track > 2) {
                  ++idx;
                  continue;
                }
                bool valid = false;
                SongTrack song_track = trackForColumn(target_track, valid);
                if (valid && idx < static_cast<int>(g_song_area_clipboard.pattern_indices.size())) {
                  int pattern = g_song_area_clipboard.pattern_indices[idx];
                  if (pattern < 0) {
                    mini_acid_.clearSongPattern(target_row, song_track);
                  } else {
                    mini_acid_.setSongPattern(target_row, song_track, pattern);
                  }
                }
                ++idx;
              }
            }
            if (mini_acid_.songModeEnabled() && !mini_acid_.isPlaying()) {
              mini_acid_.setSongPosition(start_row);
            }
          });
          
          // Save undo history
          g_undo_history.action_type = UndoActionType::Paste;
          g_undo_history.saveArea(min_row, max_row, min_track, max_track, old_patterns);
        } else if (g_song_pattern_clipboard.has_pattern) {
          // Paste single cell
          int row = cursorRow();
          int track_idx = cursorTrack();
          int patternIndex = g_song_pattern_clipboard.pattern_index;
          
          // Save old pattern for undo
          int old_pattern = mini_acid_.songPatternAt(row, track);
          g_undo_history.action_type = UndoActionType::Paste;
          g_undo_history.saveSingleCell(row, track_idx, old_pattern);
          
          withAudioGuard([&]() {
            if (patternIndex < 0) {
              mini_acid_.clearSongPattern(row, track);
            } else {
              mini_acid_.setSongPattern(row, track, patternIndex);
            }
            if (mini_acid_.songModeEnabled() && !mini_acid_.isPlaying()) {
              mini_acid_.setSongPosition(row);
            }
          });
        } else {
          return false;
        }
        return true;
      }
      case MINIACID_APP_EVENT_UNDO: {
        if (g_undo_history.action_type == UndoActionType::None || g_undo_history.cells.empty()) {
          return false;
        }
        
        // Restore all cells from undo history
        withAudioGuard([&]() {
          for (const auto& cell : g_undo_history.cells) {
            bool valid = false;
            SongTrack song_track = trackForColumn(cell.track, valid);
            if (valid && cell.row >= 0 && cell.row < Song::kMaxPositions) {
              if (cell.pattern_index < 0) {
                mini_acid_.clearSongPattern(cell.row, song_track);
              } else {
                mini_acid_.setSongPattern(cell.row, song_track, cell.pattern_index);
              }
            }
          }
          if (mini_acid_.songModeEnabled() && !mini_acid_.isPlaying()) {
            if (!g_undo_history.cells.empty()) {
              mini_acid_.setSongPosition(g_undo_history.cells[0].row);
            }
          }
        });
        
        // Clear undo history after use
        g_undo_history.clear();
        return true;
      }
      default:
        return false;
    }
  }
  if (ui_event.event_type != MINIACID_KEY_DOWN) return false;

  if (ui_event.alt && (ui_event.scancode == MINIACID_UP || ui_event.scancode == MINIACID_DOWN)) {
    int delta = ui_event.scancode == MINIACID_UP ? 1 : -1;
    if (cursorOnPlayheadLabel()) return adjustSongPlayhead(delta);
    return adjustSongPatternAtCursor(delta);
  }

  bool handled = false;
  bool extend_selection = ui_event.shift;
  switch (ui_event.scancode) {
    case MINIACID_LEFT:
      moveCursorHorizontal(-1, extend_selection);
      handled = true;
      break;
    case MINIACID_RIGHT:
      moveCursorHorizontal(1, extend_selection);
      handled = true;
      break;
    case MINIACID_UP:
      moveCursorVertical(-1, extend_selection);
      handled = true;
      break;
    case MINIACID_DOWN:
      moveCursorVertical(1, extend_selection);
      handled = true;
      break;
    default:
      break;
  }
  if (handled) return true;

  char key = ui_event.key;
  if (!key) return false;

  if (ui_event.ctrl && (key == 'l' || key == 'L')) {
    return toggleLoopMode();
  }

  if (cursorOnModeButton() && (key == '\n' || key == '\r')) {
    return toggleSongMode();
  }

  if (key == 'm' || key == 'M') {
    return toggleSongMode();
  }

  int patternIdx = patternIndexFromKey(key);
  if (cursorOnModeButton() && patternIdx >= 0) return false;
  if (patternIdx >= 0) return assignPattern(patternIdx);

  if (key == '\b') {
    return clearPattern();
  }

  return false;
}

const std::string & SongPage::getTitle() const {
  static std::string title = "SONG";
  return title;
}

void SongPage::draw(IGfx& gfx) {
  const Rect& bounds = getBoundaries();
  int x = bounds.x;
  int y = bounds.y;
  int w = bounds.w;
  int h = bounds.h;

  int body_y = y + 2;
  int body_h = h - 2;
  if (body_h <= 0) return;

  int label_h = gfx.fontHeight();
  int header_h = label_h + 4;
  int row_h = label_h + 6;
  if (row_h < 10) row_h = 10;
  int usable_h = body_h - header_h;
  if (usable_h < row_h) usable_h = row_h;
  int visible_rows = usable_h / row_h;
  if (visible_rows < 1) visible_rows = 1;

  int song_len = mini_acid_.songLength();
  int cursor_row = cursorRow();
  int playhead = mini_acid_.songPlayheadPosition();
  bool playingSong = mini_acid_.isPlaying() && mini_acid_.songModeEnabled();
  bool loopMode = mini_acid_.loopModeEnabled();

  if (playingSong) {
    int minTarget = std::min(cursor_row, playhead);
    int maxTarget = std::max(cursor_row, playhead);
    if (minTarget < scroll_row_) scroll_row_ = minTarget;
    if (maxTarget >= scroll_row_ + visible_rows) scroll_row_ = maxTarget - visible_rows + 1;
  } else {
    if (cursor_row < scroll_row_) scroll_row_ = cursor_row;
    if (cursor_row >= scroll_row_ + visible_rows) scroll_row_ = cursor_row - visible_rows + 1;
  }
  if (scroll_row_ < 0) scroll_row_ = 0;
  int maxStart = Song::kMaxPositions - visible_rows;
  if (maxStart < 0) maxStart = 0;
  if (scroll_row_ > maxStart) scroll_row_ = maxStart;

  int pos_col_w = 20;
  int spacing = 3;
  int modeBtnW = 70;
  int track_col_w = (w - pos_col_w - spacing * 5 - modeBtnW) / 3;
  if (track_col_w < 20) track_col_w = 20;

  gfx.setTextColor(COLOR_LABEL);
  gfx.drawText(x, body_y, "POS");
  gfx.drawText(x + pos_col_w + spacing, body_y, "303A");
  gfx.drawText(x + pos_col_w + spacing + track_col_w, body_y, "303B");
  gfx.drawText(x + pos_col_w + spacing + track_col_w * 2, body_y, "Drums");
  char lenBuf[24];
  snprintf(lenBuf, sizeof(lenBuf), "PLAYHD %d:%d", playhead + 1, song_len);
  int lenX = x + pos_col_w + spacing + track_col_w * 3 + spacing + 10;
  int lenW = textWidth(gfx, lenBuf);
  bool playheadSelected = cursorOnPlayheadLabel();
  if (playheadSelected) {
    gfx.drawRect(lenX - 2, body_y - 1, lenW + 4, label_h + 2, COLOR_STEP_SELECTED);
  }
  gfx.drawText(lenX, body_y, lenBuf);

  if (loopMode) {
    int loopStart = mini_acid_.loopStartRow();
    int loopEnd = mini_acid_.loopEndRow();
    char loopBuf[24];
    snprintf(loopBuf, sizeof(loopBuf), "LOOP %d-%d", loopStart + 1, loopEnd + 1);
    int loopX = lenX + lenW + 8;
    gfx.setTextColor(IGfxColor::Yellow());
    gfx.drawText(loopX, body_y, loopBuf);
    gfx.setTextColor(COLOR_WHITE);
  }

  int modeX = x + w - modeBtnW;
  int modeY = body_y - 2 + 30;
  int modeH = header_h + row_h;
  
  if (!mode_button_initialized_) {
    initModeButton(modeX, modeY, modeBtnW - 2, modeH);
  }
  mode_button_container_.draw(gfx);
  
  bool modeSelected = cursorOnModeButton();
  if (modeSelected) {
    gfx.drawRect(modeX - 2, modeY - 2, modeBtnW + 2, modeH + 4, COLOR_STEP_SELECTED);
  }

  int row_y = body_y + header_h;
  
  // Get selection bounds for highlighting
  int sel_min_row, sel_max_row, sel_min_track, sel_max_track;
  if (has_selection_) {
    getSelectionBounds(sel_min_row, sel_max_row, sel_min_track, sel_max_track);
  }
  
  for (int i = 0; i < visible_rows; ++i) {
    int row_idx = scroll_row_ + i;
    if (row_idx >= Song::kMaxPositions) break;
    bool isCursorRow = row_idx == cursor_row;
    bool isPlayhead = playingSong && row_idx == playhead;
    IGfxColor rowHighlight = isPlayhead ? IGfxColor::Magenta() : COLOR_DARKER;
    if (isPlayhead) {
      gfx.fillRect(x, row_y - 1, w - modeBtnW - 2, row_h, rowHighlight);
    } else if (isCursorRow) {
      gfx.fillRect(x, row_y - 1, w - modeBtnW - 2, row_h, COLOR_PANEL);
    } else {
      gfx.fillRect(x, row_y - 1, w - modeBtnW - 2, row_h, COLOR_DARKER);
    }

    char posLabel[8];
    snprintf(posLabel, sizeof(posLabel), "%d", row_idx + 1);
    gfx.setTextColor(row_idx < song_len ? COLOR_WHITE : COLOR_LABEL);
    gfx.drawText(x, row_y + 2, posLabel);
    gfx.setTextColor(COLOR_WHITE);

    for (int t = 0; t < SongPosition::kTrackCount; ++t) {
      int col_x = x + pos_col_w + spacing + t * (track_col_w + spacing);
      int patternIdx = mini_acid_.songPatternAt(row_idx,
                        t == 0 ? SongTrack::SynthA : (t == 1 ? SongTrack::SynthB : SongTrack::Drums));
      bool isSelected = isCursorRow && cursorTrack() == t;
      bool inSelection = has_selection_ && 
                        row_idx >= sel_min_row && row_idx <= sel_max_row &&
                        t >= sel_min_track && t <= sel_max_track &&
                        t <= 2; // Only valid track columns
      
      if (inSelection) {
        // Draw selection highlight with a filled background
        gfx.fillRect(col_x - 1, row_y - 2, track_col_w + 2, row_h + 2 - 1, IGfxColor(0x000080));
        gfx.drawRect(col_x - 1, row_y - 2, track_col_w + 2, row_h + 2 - 1, IGfxColor::Cyan());
      } else if (isSelected) {
        gfx.drawRect(col_x - 1, row_y - 2, track_col_w + 2, row_h + 2 - 1, COLOR_STEP_SELECTED);
      }
      char label[6];
      if (patternIdx < 0) {
        snprintf(label, sizeof(label), "--");
        gfx.setTextColor(COLOR_LABEL);
      } else {
        int bankIdx = songPatternBank(patternIdx);
        int bankPattern = songPatternIndexInBank(patternIdx);
        if (bankIdx < 0 || bankIdx >= kBankCount || bankPattern < 0) {
          snprintf(label, sizeof(label), "--");
          gfx.setTextColor(COLOR_LABEL);
        } else {
          char bankLetter = static_cast<char>('A' + bankIdx);
          snprintf(label, sizeof(label), "%c%d", bankLetter, bankPattern + 1);
          gfx.setTextColor(COLOR_WHITE);
        }
      }
      int tw = textWidth(gfx, label);
      int tx = col_x + (track_col_w - tw) / 2;
      gfx.drawText(tx, row_y + (row_h - label_h) / 2 - 1, label);
      gfx.setTextColor(COLOR_WHITE);
    }
    row_y += row_h;
  }
}

std::unique_ptr<MultiPageHelpDialog> SongPage::getHelpDialog() {
  return std::make_unique<MultiPageHelpDialog>(*this);
}

int SongPage::getHelpFrameCount() const {
  return 2;
}

void SongPage::drawHelpFrame(IGfx& gfx, int frameIndex, Rect bounds) const {
  if (bounds.w <= 0 || bounds.h <= 0) return;
  switch (frameIndex) {
    case 0:
      drawHelpPageSong(gfx, bounds.x, bounds.y, bounds.w, bounds.h);
      break;
    case 1:
      drawHelpPageSongCont(gfx, bounds.x, bounds.y, bounds.w, bounds.h);
      break;
    default:
      break;
  }
}
