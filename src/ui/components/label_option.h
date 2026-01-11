#pragma once

#include <string>
#include <vector>

#include "../ui_core.h"
#include "../ui_utils.h"

class LabelOptionComponent : public FocusableComponent {
 public:
  LabelOptionComponent(const char* label, IGfxColor label_color,
                       IGfxColor value_color);

  void setOptions(std::vector<std::string> options);
  void setOptionIndex(int index);
  int optionIndex() const { return option_index_; }

  bool handleEvent(UIEvent& ui_event) override;
  void draw(IGfx& gfx) override;

 private:
  const char* currentOption() const;

  std::string label_;
  IGfxColor label_color_;
  IGfxColor value_color_;
  std::vector<std::string> options_;
  int option_index_ = 0;
};
