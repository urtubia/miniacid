#pragma once

#include <stdint.h>

#include "mini_dsp_params.h"

class ChamberlinFilter {
public:
  explicit ChamberlinFilter(float sampleRate);
  void reset();
  void setSampleRate(float sr);
  float process(float input, float cutoffHz, float resonance);
private:
  float _lp;
  float _bp;
  float _sampleRate;
};

enum class TB303ParamId : uint8_t {
  Cutoff = 0,
  Resonance,
  EnvAmount,
  EnvDecay,
  Oscillator,
  MainVolume,
  Count
};

class TB303Voice {
public:
  explicit TB303Voice(float sampleRate);

  void reset();
  void setSampleRate(float sampleRate);
  void startNote(float freqHz, bool accent, bool slideFlag);
  void release();
  float process();
  const Parameter& parameter(TB303ParamId id) const;
  void setParameter(TB303ParamId id, float value);
  void adjustParameter(TB303ParamId id, int steps);
  float parameterValue(TB303ParamId id) const;
  int oscillatorIndex() const;

private:
  float oscSaw();
  float oscSquare(float saw);
  float oscSuperSaw();
  float oscillatorSample();
  float svfProcess(float input);
  void initParameters();

  static constexpr int kSuperSawOscCount = 6;

  float phase;
  float superPhases[kSuperSawOscCount];
  float freq;       // current frequency (Hz)
  float targetFreq; // slide target
  float slideSpeed; // how fast we slide toward target
  float env;        // filter envelope value
  bool gate;        // note on/off
  bool slide;       // slide flag for next note
  float amp;        // amplitude

  float sampleRate;
  float invSampleRate;
  float nyquist;

  Parameter params[static_cast<int>(TB303ParamId::Count)];
  ChamberlinFilter filter;
};
