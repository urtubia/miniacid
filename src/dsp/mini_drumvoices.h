
#pragma once
#include <stdint.h>
#include "mini_dsp_params.h"

enum class DrumParamId : uint8_t {
  MainVolume = 0,
  BusCompAmount,   // 0..1
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
  float processSnare();    // unchanged from your version
  float processHat();
  float processOpenHat();
  float processMidTom();
  float processHighTom();
  float processRim();
  float processClap();     // updated

  // Bus processing
  float processBus(float mixSample);

  // Snare
  float snareHpPrev; // extra high-pass memory

  // Parameters - for later
  const Parameter& parameter(DrumParamId id) const;
  void setParameter(DrumParamId id, float value);

private:
  // Fast RNG [-1, 1]
  float frand();
  uint32_t rngState;

  // Kick (606-tight)
  float kickPhase, kickFreq, kickEnvAmp, kickEnvPitch, kickClickEnv;
  bool  kickActive;

  // Snare
  float snareEnvAmp, snareToneEnv;
  bool  snareActive;
  float snareBp, snareLp, snareTonePhase, snareTonePhase2;

  // Closed Hat (metallic)
  float hatEnvAmp, hatToneEnv;
  bool  hatActive;
  float hatHp, hatPrev;
  float hatPh[6], hatInc[6];

  // Open Hat
  float openHatEnvAmp, openHatToneEnv;
  bool  openHatActive;
  float openHatHp, openHatPrev;
  float openHatPh[6], openHatInc[6];

  // Toms
  float midTomPhase, midTomEnv, midTomPitchEnv;
  bool  midTomActive;

  float highTomPhase, highTomEnv, highTomPitchEnv;
  bool  highTomActive;

  // Rimshot
  float rimPhase, rimEnv, rimBp, rimLp;
  bool  rimActive;

  // Clap (hollow, multi-hand)
  float clapEnv;         // overall body envelope (slow)
  float clapTrans;       // transient envelope (fast)
  float clapTailEnv;     // tail/body envelope (medium)
  float clapNoiseSeed;   // per-hit color
  bool  clapActive;
  float clapTime;        // seconds since trigger

  // Noise shaper & cavity states
  float clapHp, clapPrev;       // high-pass
  float clapAirLp;              // air low-pass (tames hiss)

  // Two independent cavity bands (each cascaded to narrow the band)
  float clapBpA, clapLpA, clapBpA2, clapLpA2;  // formant A (~mid cavity)
  float clapBpB, clapLpB, clapBpB2, clapLpB2;  // formant B (~upper-mid cavity)

  // Tonal snaps (very short) + extra crack envelope
  float clapSnapPhase1, clapSnapPhase2, clapSnapPhase3;
  float clapSnapEnv1,   clapSnapEnv2,   clapSnapEnv3;
  float clapCrackEnv;

  // feed-forward multi-tap cluster (no feedback)
  static const int kClapTapBufMax = 2048;  // big enough for ~30 ms @ 44.1 kHz
  float clapTapBuf[kClapTapBufMax];
  int   clapTapIdx;
  int   clapD1, clapD2, clapD3, clapD4, clapD5, clapD6; // sample delays
  int   clapTapLen;                      // ring size (<= kClapTapBufMax)

  // Sample rate
  float sampleRate, invSampleRate;

  // Bus Compressor
  float compEnv, compAttackCoeff, compReleaseCoeff;
  float compGainDb, compMakeupDb, compThreshDb, compRatio, compKneeDb, compAmount;
  int   compDecimCounter;   // sub-rate update counter
  float compLastGainAmp;    // last applied amplitude gain
  
  // Global params - for later
  Parameter params[static_cast<int>(DrumParamId::Count)];
};
