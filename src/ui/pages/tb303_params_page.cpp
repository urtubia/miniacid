#include "tb303_params_page.h"

#include <cstdarg>
#include <cstdio>

namespace {
struct Knob {
  const char* label;
  float value;
  float minValue;
  float maxValue;
  const char* unit;

  void draw(IGfx& gfx, int cx, int cy, int radius, IGfxColor ringColor,
            IGfxColor indicatorColor) const {
    float norm = 0.0f;
    if (maxValue > minValue)
      norm = (value - minValue) / (maxValue - minValue);
    norm = std::clamp(norm, 0.0f, 1.0f);

    gfx.drawKnobFace(cx, cy, radius, ringColor, COLOR_BLACK);

    constexpr float kDegToRad = 3.14159265f / 180.0f;

    float degAngle = (135.0f + norm * 270.0f);
    if (degAngle >= 360.0f)
      degAngle -= 360.0f;
    float angle = (degAngle) * kDegToRad;

    int ix = cx + static_cast<int>(roundf(cosf(angle) * (radius - 2)));
    int iy = cy + static_cast<int>(roundf(sinf(angle) * (radius - 2)));

    drawLineColored(gfx, cx, cy, ix, iy, indicatorColor);

    gfx.setTextColor(COLOR_LABEL);
    int label_x = cx - textWidth(gfx, label) / 2;
    gfx.drawText(label_x, cy + radius + 6, label);

    char buf[48];
    if (unit && unit[0]) {
      snprintf(buf, sizeof(buf), "%.0f %s", value, unit);
    } else {
      snprintf(buf, sizeof(buf), "%.2f", value);
    }
    int val_x = cx - textWidth(gfx, buf) / 2;
    gfx.drawText(val_x, cy - radius - 14, buf);
  }
};
} // namespace

Synth303ParamsPage::Synth303ParamsPage(IGfx& gfx, MiniAcid& mini_acid, AudioGuard& audio_guard, int voice_index) :
    gfx_(gfx),
    mini_acid_(mini_acid),
    audio_guard_(audio_guard),
    voice_index_(voice_index)
{
}

void Synth303ParamsPage::draw(IGfx& gfx, int x, int y, int w, int h)
{
  char buf[128];
  auto print = [&](int px, int py, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    gfx_.drawText(px, py, buf);
  };

  int center_y = y + h / 2 - 5;


  int x_margin = -10;
  int usable_w = w - x_margin * 2;


  int radius = 18;
  int spacing = usable_w / 5;
  
  gfx_.drawLine(x + x_margin, y, x + x_margin , h);
  gfx_.drawLine(x + x_margin + usable_w, y, x + x_margin + usable_w, h);
  
  int cx1 = x + x_margin + spacing * 1;
  int cx2 = x + x_margin + spacing * 2;
  int cx3 = x + x_margin + spacing * 3;
  int cx4 = x + x_margin + spacing * 4;

  const Parameter& pCut = mini_acid_.parameter303(TB303ParamId::Cutoff, voice_index_);
  const Parameter& pRes = mini_acid_.parameter303(TB303ParamId::Resonance, voice_index_);
  const Parameter& pEnv = mini_acid_.parameter303(TB303ParamId::EnvAmount, voice_index_);
  const Parameter& pDec = mini_acid_.parameter303(TB303ParamId::EnvDecay, voice_index_);
  const Parameter& pOsc = mini_acid_.parameter303(TB303ParamId::Oscillator, voice_index_);

  Knob cutoff{pCut.label(), pCut.value(), pCut.min(), pCut.max(), pCut.unit()};
  Knob res{pRes.label(), pRes.value(), pRes.min(), pRes.max(), pRes.unit()};
  Knob env{pEnv.label(), pEnv.value(), pEnv.min(), pEnv.max(), pEnv.unit()};
  Knob dec{pDec.label(), pDec.value(), pDec.min(), pDec.max(), pDec.unit()};

  cutoff.draw(gfx_, cx1, center_y, radius, COLOR_KNOB_1, COLOR_KNOB_1);
  res.draw(gfx_, cx2, center_y, radius, COLOR_KNOB_2, COLOR_KNOB_2);
  env.draw(gfx_, cx3, center_y, radius, COLOR_KNOB_3, COLOR_KNOB_3);
  dec.draw(gfx_, cx4, center_y, radius, COLOR_KNOB_4, COLOR_KNOB_4);

  int delta_y_for_controls = 35;
  int delta_x_for_controls = -9;

  gfx_.setTextColor(COLOR_KNOB_CONTROL);
  print(cx1 + delta_x_for_controls, center_y + delta_y_for_controls, "A/Z");
  print(cx2 + delta_x_for_controls, center_y + delta_y_for_controls, "S/X");
  print(cx3 + delta_x_for_controls, center_y + delta_y_for_controls, "D/C");
  print(cx4 + delta_x_for_controls, center_y + delta_y_for_controls, "F/V");

  const char* oscLabel = pOsc.optionLabel();
  if (!oscLabel) oscLabel = "";
  gfx_.setTextColor(COLOR_WHITE);
  snprintf(buf, sizeof(buf), "OSC: %s (T/G)", oscLabel);
  // int oscTextX = x + x_margin + (usable_w - textWidth(gfx, buf)) / 2;
  int oscTextX = x + x_margin + 10;
  gfx_.drawText(oscTextX, y + h - 10, buf);

  gfx_.setTextColor(COLOR_WHITE);
}

void Synth303ParamsPage::withAudioGuard(const std::function<void()>& fn) {
  if (audio_guard_) {
    audio_guard_(fn);
    return;
  }
  fn();
}

const std::string & Synth303ParamsPage::getTitle() const 
{
  static std::string title = voice_index_ == 0 ? "303A PARAMS" : "303B PARAMS";
  return title;
}

bool Synth303ParamsPage::handleEvent(UIEvent& ui_event) 
{
  bool event_handled = false;
  int steps = 5;
  switch(ui_event.key){
    case 't':
      withAudioGuard([&]() {
        mini_acid_.adjust303Parameter(TB303ParamId::Oscillator, 1, voice_index_);
      });
      event_handled = true;
      break;
    case 'g':
      withAudioGuard([&]() {
        mini_acid_.adjust303Parameter(TB303ParamId::Oscillator, -1, voice_index_);
      });
      event_handled = true;
      break;
    case 'a':
      withAudioGuard([&]() { 
        mini_acid_.adjust303Parameter(TB303ParamId::Cutoff, steps, voice_index_); 
      });
      event_handled = true;
      break;
    case 'z':
      withAudioGuard([&]() { 
        mini_acid_.adjust303Parameter(TB303ParamId::Cutoff, -steps, voice_index_);
      });
      event_handled = true;
      break;
    case 's':
      withAudioGuard([&]() { 
        mini_acid_.adjust303Parameter(TB303ParamId::Resonance, steps, voice_index_);
      });
      event_handled = true;
      break;
    case 'x':
      withAudioGuard([&]() { 
        mini_acid_.adjust303Parameter(TB303ParamId::Resonance, -steps, voice_index_);
      });
      event_handled = true;
      break;
    case 'd':
      withAudioGuard([&]() { 
        mini_acid_.adjust303Parameter(TB303ParamId::EnvAmount, steps, voice_index_);
      });
      event_handled = true;
      break;
    case 'c':
      withAudioGuard([&]() { 
        mini_acid_.adjust303Parameter(TB303ParamId::EnvAmount, -steps, voice_index_);
      });
      event_handled = true;
      break;
    case 'f':
      withAudioGuard([&]() { 
        mini_acid_.adjust303Parameter(TB303ParamId::EnvDecay, steps, voice_index_);
      });
      event_handled = true;
      break;
    case 'v':
      withAudioGuard([&]() { 
        mini_acid_.adjust303Parameter(TB303ParamId::EnvDecay, -1, voice_index_);
      });
      event_handled = true;
      break;
    case 'm':
      withAudioGuard([&]() { 
        mini_acid_.toggleDelay303(voice_index_);
      });
      break;
    default:
      break;
  }
  return event_handled;
}
