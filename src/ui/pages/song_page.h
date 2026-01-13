#pragma once

#include "../ui_core.h"
#include "../pages/help_dialog.h"
#include "../ui_colors.h"
#include "../ui_utils.h"

class SongPage : public IPage, public IMultiHelpFramesProvider {
 public:
  SongPage(IGfx& gfx, MiniAcid& mini_acid, AudioGuard& audio_guard);
  void draw(IGfx& gfx) override;
  bool handleEvent(UIEvent& ui_event) override;
  const std::string & getTitle() const override;
  std::unique_ptr<MultiPageHelpDialog> getHelpDialog() override;
  int getHelpFrameCount() const override;
  void drawHelpFrame(IGfx& gfx, int frameIndex, Rect bounds) const override;

  void setScrollToPlayhead(int playhead);
 private:
  void initModeButton(int x, int y, int w, int h);
  int clampCursorRow(int row) const;
  int cursorRow() const;
  int cursorTrack() const;
  bool cursorOnModeButton() const;
  bool cursorOnPlayheadLabel() const;
  void moveCursorHorizontal(int delta, bool extend_selection);
  void moveCursorVertical(int delta, bool extend_selection);
  void syncSongPositionToCursor();
  void withAudioGuard(const std::function<void()>& fn);
  void startSelection();
  void updateSelection();
  void clearSelection();
  void updateLoopRangeFromSelection();
  void getSelectionBounds(int& min_row, int& max_row, int& min_track, int& max_track) const;
  SongTrack trackForColumn(int col, bool& valid) const;
  int bankIndexForTrack(SongTrack track) const;
  int patternIndexFromKey(char key) const;
  bool adjustSongPatternAtCursor(int delta);
  bool adjustSongPlayhead(int delta);
  bool assignPattern(int patternIdx);
  bool clearPattern();
  bool toggleSongMode();
  bool toggleLoopMode();

  IGfx& gfx_;
  MiniAcid& mini_acid_;
  AudioGuard& audio_guard_;
  int cursor_row_;
  int cursor_track_;
  int scroll_row_;
  bool has_selection_;
  int selection_start_row_;
  int selection_start_track_;
  Container mode_button_container_;
  bool mode_button_initialized_ = false;
};
