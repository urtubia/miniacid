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
  float snareHpPrev; // extra high-pass memory

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

  // ----- Clap (revamped) -----
  float clapEnv;         // overall body envelope (longer tail)
  float clapTrans;       // transient envelope (fast)
  float clapTailEnv;     // separate tail envelope
  float clapNoiseSeed;   // per-hit color
  bool  clapActive;
  float clapTime;        // seconds since trigger

  // noise shaper states
  float clapHp, clapPrev;
  float clapBp, clapLp;      // ~1.3 kHz region
  float clapBp2, clapLp2;    // ~2.2 kHz region

  // tonal snap (very short)
  float clapSnapPhase;
  float clapSnapEnv;

  // feed-forward multi-tap cluster (no feedback; safe on ESP)
  static const int kClapTapBufMax = 1024;
  float clapTapBuf[kClapTapBufMax];
  int   clapTapIdx;
  int   clapD1, clapD2, clapD3, clapD4; // sample delays
  int   clapTapLen;                      // ring size (<= kClapTapBufMax)

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
