#include "mini_tb303.h"

#include <math.h>
#include <stdlib.h>

namespace {
const char* const kOscillatorOptions[] = {"saw", "sqr", "spr"};
} // namespace

ChamberlinFilter::ChamberlinFilter(float sampleRate) : _lp(0.0f), _bp(0.0f), _sampleRate(sampleRate) {
  if (_sampleRate <= 0.0f) _sampleRate = 44100.0f;
}

void ChamberlinFilter::reset() {
  _lp = 0.0f;
  _bp = 0.0f;
}

void ChamberlinFilter::setSampleRate(float sr) {
  if (sr <= 0.0f) sr = 44100.0f;
  _sampleRate = sr;
}

float ChamberlinFilter::process(float input, float cutoffHz, float resonance) {
  float f = 2.0f * sinf(3.14159265f * cutoffHz / _sampleRate);
  if (!isfinite(f))
    f = 0.0f;
  float q = 1.0f / (1.0f + resonance * 4.0f);
  if (q < 0.06f)
    q = 0.06f;

  float hp = input - _lp - q * _bp;
  _bp += f * hp;
  _lp += f * _bp;

  _bp = tanhf(_bp * 1.3f);

  // Keep states bounded to avoid numeric blowups
  const float kStateLimit = 50.0f;
  if (_lp > kStateLimit) _lp = kStateLimit;
  if (_lp < -kStateLimit) _lp = -kStateLimit;
  if (_bp > kStateLimit) _bp = kStateLimit;
  if (_bp < -kStateLimit) _bp = -kStateLimit;

  return _lp;
}

TB303Voice::TB303Voice(float sampleRate)
  : sampleRate(sampleRate),
    invSampleRate(0.0f),
    nyquist(0.0f),
    filter(sampleRate) {
  setSampleRate(sampleRate);
  reset();
}

void TB303Voice::reset() {
  initParameters();
  phase = 0.0f;
  for (int i = 0; i < kSuperSawOscCount; ++i) {
    float seed = (static_cast<float>(i) + 1.0f) * 0.137f;
    superPhases[i] = seed - floorf(seed);
  }
  freq = 110.0f;
  targetFreq = 110.0f;
  slideSpeed = 0.001f;
  env = 0.0f;
  gate = false;
  slide = false;
  amp = 0.3f;
  filter.reset();
}

void TB303Voice::setSampleRate(float sampleRateHz) {
  if (sampleRateHz <= 0.0f) sampleRateHz = 44100.0f;
  sampleRate = sampleRateHz;
  invSampleRate = 1.0f / sampleRate;
  nyquist = sampleRate * 0.5f;
  filter.setSampleRate(sampleRate);
}

void TB303Voice::startNote(float freqHz, bool accent, bool slideFlag) {
  slide = slideFlag;

  if (!slide) {
    freq = freqHz;
  }
  targetFreq = freqHz;

  gate = true;
  env = accent ? 2.0f : 1.0f;
}

void TB303Voice::release() { gate = false; }

float TB303Voice::oscSaw() {
  phase += freq * invSampleRate;
  if (phase >= 1.0f) {
    phase -= 1.0f;
  }
  return 2.0f * phase - 1.0f;
}

float TB303Voice::oscSquare(float saw) {
  return saw >= 0.0f ? 1.0f : -1.0f;
}

float TB303Voice::oscSuperSaw() {
  static const float kSuperSawDetune[kSuperSawOscCount] = {
    -0.019f, 0.019f, -0.012f, 0.012f, -0.0065f, 0.0065f
  };

  float basePhaseInc = freq * invSampleRate;
  phase += basePhaseInc;
  if (phase >= 1.0f) {
    phase -= 1.0f;
  }

  float sum = 2.0f * phase - 1.0f;

  for (int i = 0; i < kSuperSawOscCount; ++i) {
    float detunedFreq = freq * (1.0f + kSuperSawDetune[i]);
    float inc = detunedFreq * invSampleRate;
    superPhases[i] += inc;
    if (superPhases[i] >= 1.0f) {
      superPhases[i] -= floorf(superPhases[i]);
    } else if (superPhases[i] < 0.0f) {
      superPhases[i] += 1.0f;
    }
    sum += 2.0f * superPhases[i] - 1.0f;
  }

  // constexpr float kGain = 1.0f / (1.0f + TB303Voice::kSuperSawOscCount);
  constexpr float kGain = 1.0f / (TB303Voice::kSuperSawOscCount - 1);
  return sum * kGain;
}

float TB303Voice::oscillatorSample() {
  int oscIdx = oscillatorIndex();
  if (oscIdx == 1) {
    float saw = oscSaw();
    return oscSquare(saw);
  }
  if (oscIdx == 2) {
    return oscSuperSaw();
  }
  return oscSaw();
}

float TB303Voice::svfProcess(float input) {
  // Slide toward target frequency
  freq += (targetFreq - freq) * slideSpeed;
  if (!isfinite(freq))
    freq = targetFreq;

  // Envelope decay
  if (gate || env > 0.0001f) {
    float decayMs = parameterValue(TB303ParamId::EnvDecay);
    float decaySamples = decayMs * sampleRate * 0.001f;
    if (decaySamples < 1.0f)
      decaySamples = 1.0f;
    // 0.01 represents roughly -40 dB, a practical "off" point for the envelope.
    constexpr float kDecayTargetLog = -4.60517019f; // ln(0.01f)
    float decayCoeff = expf(kDecayTargetLog / decaySamples);
    env *= decayCoeff;
  }

  float cutoffHz = parameterValue(TB303ParamId::Cutoff) + parameterValue(TB303ParamId::EnvAmount) * env;
  if (cutoffHz < 50.0f)
    cutoffHz = 50.0f;
  float maxCutoff = nyquist * 0.9f;
  if (cutoffHz > maxCutoff)
    cutoffHz = maxCutoff;


  return filter.process(input, cutoffHz, parameterValue(TB303ParamId::Resonance));
}

float TB303Voice::process() {
  if (!gate && env < 0.0001f) {
    return 0.0f;
  }

  float osc = oscillatorSample();
  float out = svfProcess(osc);

  return out * amp;
}

const Parameter& TB303Voice::parameter(TB303ParamId id) const {
  return params[static_cast<int>(id)];
}

void TB303Voice::setParameter(TB303ParamId id, float value) {
  params[static_cast<int>(id)].setValue(value);
}

void TB303Voice::adjustParameter(TB303ParamId id, int steps) {
  params[static_cast<int>(id)].addSteps(steps);
}

float TB303Voice::parameterValue(TB303ParamId id) const {
  return params[static_cast<int>(id)].value();
}

int TB303Voice::oscillatorIndex() const {
  return params[static_cast<int>(TB303ParamId::Oscillator)].optionIndex();
}

void TB303Voice::initParameters() {
  params[static_cast<int>(TB303ParamId::Cutoff)] = Parameter("cut", "Hz", 60.0f, 2500.0f, 800.0f, (2500.f - 60.0f) / 128);
  params[static_cast<int>(TB303ParamId::Resonance)] = Parameter("res", "", 0.05f, 0.85f, 0.6f, (0.85f - 0.05f) / 128);
  params[static_cast<int>(TB303ParamId::EnvAmount)] = Parameter("env", "Hz", 0.0f, 2000.0f, 400.0f, (2000.0f - 0.0f) / 128);
  params[static_cast<int>(TB303ParamId::EnvDecay)] = Parameter("dec", "ms", 20.0f, 2200.0f, 420.0f, (2200.0f - 20.0f) / 128);
  params[static_cast<int>(TB303ParamId::Oscillator)] = Parameter("osc", "", kOscillatorOptions, 3, 0);
  params[static_cast<int>(TB303ParamId::MainVolume)] = Parameter("vol", "", 0.0f, 1.0f, 0.8f, 1.0f / 128);
}
