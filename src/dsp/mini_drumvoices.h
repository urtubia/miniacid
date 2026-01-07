
#pragma once
#include <stdint.h>
#include "mini_dsp_params.h"

// Public parameter ids (kept for compatibility + bus comp)
enum class DrumParamId : uint8_t {
  MainVolume = 0,
  BusCompAmount,   // 0..1 one-knob compressor
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

  // Bus processing (apply one-knob compressor to the mixed sum)
  float processBus(float mixSample);

  // Snare
  float snareHpPrev;   // extra high-pass memory

  // Parameters
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
  float snareBp;
  float snareLp;
  float snareTonePhase;
  float snareTonePhase2;

  // ----- Closed Hat (metallic) -----
  float hatEnvAmp;
  float hatToneEnv;
  bool  hatActive;
  float hatHp;           // HP filter state
  float hatPrev;
  float hatPh[6];
  float hatInc[6];

  // ----- Open Hat -----
  float openHatEnvAmp;
  float openHatToneEnv;
  bool  openHatActive;
  float openHatHp;
  float openHatPrev;
  float openHatPh[6];
  float openHatInc[6];

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
  float rimBp;
  float rimLp;
  bool  rimActive;

  // ----- Clap (simplified, no comb/diffusion) -----
  float clapEnv;         // overall body envelope (longer tail)
  float clapTrans;       // transient envelope
  float clapTailEnv;     // separate tail envelope
  float clapNoiseSeed;   // per-hit color
  bool  clapActive;
  float clapTime;        // seconds since trigger

  // noise shaper states
  float clapHp, clapPrev;
  float clapBp, clapLp;

  // Sample rate
  float sampleRate;
  float invSampleRate;

  // ----- One-knob Bus Compressor -----
  float compEnv;         // detector envelope
  float compAttackCoeff;
  float compReleaseCoeff;
  float compGainDb;      // smoothed gain reduction
  float compMakeupDb;    // auto makeup (dB)
  float compThreshDb;    // mapped from knob (-18 .. -6 dB)
  float compRatio;       // mapped from knob (2:1 .. 6:1)
  float compKneeDb;      // soft knee width (fixed ~6 dB)
  float compAmount;      // 0..1 parameter cache

  // Global params
  Parameter params[static_cast<int>(DrumParamId::Count)];
};
