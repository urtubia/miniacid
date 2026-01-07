#pragma once
#include <stdint.h>
#include "mini_dsp_params.h"

// Public parameter ids (kept for compatibility)
enum class DrumParamId : uint8_t {
  MainVolume = 0,
  Count
};

class DrumSynthVoice {
public:
  explicit DrumSynthVoice(float sampleRate);
  void reset();
  void setSampleRate(float sampleRate);

  // Triggers
  void triggerKick();
  void triggerSnare();
  void triggerHat();
  void triggerOpenHat();
  void triggerMidTom();
  void triggerHighTom();
  void triggerRim();
  void triggerClap();

  // Audio processors (one sample per call)
  float processKick();
  float processSnare();
  float processHat();
  float processOpenHat();
  float processMidTom();
  float processHighTom();
  float processRim();
  float processClap();

  // Parameters (kept)
  const Parameter& parameter(DrumParamId id) const;
  void setParameter(DrumParamId id, float value);

private:
  // Fast RNG [-1, 1]
  float frand();
  uint32_t rngState;

  // ----- Kick (606-tight) -----
  float kickPhase;
  float kickFreq;
  float kickEnvAmp;
  float kickEnvPitch;
  float kickClickEnv;
  bool  kickActive;

  // ----- Snare -----
  float snareEnvAmp;     // noise amp
  float snareToneEnv;    // tone tick
  bool  snareActive;
  // simple state for noise filters
  float snareBp;
  float snareLp;
  // two tone oscillators
  float snareTonePhase;
  float snareTonePhase2;

  // ----- Closed Hat (metallic) -----
  float hatEnvAmp;
  float hatToneEnv;
  bool  hatActive;
  float hatHp;           // HP filter state
  float hatPrev;
  // six square oscillators
  float hatPh[6];

  // ----- Open Hat -----
  float openHatEnvAmp;
  float openHatToneEnv;
  bool  openHatActive;
  float openHatHp;
  float openHatPrev;
  float openHatPh[6];

  // ----- Toms -----
  float midTomPhase;
  float midTomEnv;
  float midTomPitchEnv;
  bool  midTomActive;

  float highTomPhase;
  float highTomEnv;
  float highTomPitchEnv;
  bool  highTomActive;

  // ----- Rimshot -----
  float rimPhase;
  float rimEnv;
  float rimBp;           // bandpass state
  float rimLp;
  bool  rimActive;

  // ----- Clap -----
  float clapEnv;
  float clapTrans;       // transient env
  float clapNoiseSeed;   // seed per hit
  bool  clapActive;
  float clapTime;        // seconds since trigger
  // mini comb/reverb
  static const int kClapBufMax = 1024;
  float clapBuf[kClapBufMax];
  int   clapIdx;
  int   clapCombLen;

  // Sample rate
  float sampleRate;
  float invSampleRate;

  // Hat oscillator freqs (phase increments computed per sampleRate)
  float hatInc[6];
  float openHatInc[6];

  // Global params
  Parameter params[static_cast<int>(DrumParamId::Count)];
};
