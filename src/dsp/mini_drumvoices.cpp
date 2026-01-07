#include "mini_drumvoices.h"
#include <math.h>

// ---------------- Utility ----------------
static inline float fast_tanhf(float x) {
  // 3rd-order polynomial approximation is plenty for drum drive
  const float x2 = x * x;
  return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

DrumSynthVoice::DrumSynthVoice(float sampleRate)
  : sampleRate(sampleRate), invSampleRate(0.0f), rngState(0x12345678u) {
  setSampleRate(sampleRate);
  reset();
}

void DrumSynthVoice::reset() {
  // Kick
  kickPhase = 0.0f; kickFreq = 55.0f;
  kickEnvAmp = 0.0f; kickEnvPitch = 0.0f; kickClickEnv = 0.0f;
  kickActive = false;

  // Snare
  snareEnvAmp = 0.0f; snareToneEnv = 0.0f; snareActive = false;
  snareBp = 0.0f; snareLp = 0.0f;
  snareTonePhase = 0.0f; snareTonePhase2 = 0.0f;

  // Hats
  hatEnvAmp = 0.0f; hatToneEnv = 0.0f; hatActive = false;
  hatHp = 0.0f; hatPrev = 0.0f;
  for (int i = 0; i < 6; ++i) hatPh[i] = 0.0f;

  openHatEnvAmp = 0.0f; openHatToneEnv = 0.0f; openHatActive = false;
  openHatHp = 0.0f; openHatPrev = 0.0f;
  for (int i = 0; i < 6; ++i) openHatPh[i] = 0.0f;

  // Toms
  midTomPhase = 0.0f; midTomEnv = 0.0f; midTomPitchEnv = 0.0f; midTomActive = false;
  highTomPhase = 0.0f; highTomEnv = 0.0f; highTomPitchEnv = 0.0f; highTomActive = false;

  // Rim
  rimPhase = 0.0f; rimEnv = 0.0f; rimBp = 0.0f; rimLp = 0.0f; rimActive = false;

  // Clap
  clapEnv = 0.0f; clapTrans = 0.0f; clapNoiseSeed = 0.0f; clapActive = false;
  clapTime = 0.0f; clapIdx = 0; clapCombLen = (int)(0.008f * sampleRate); // ~8 ms
  if (clapCombLen < 64) clapCombLen = 64;
  if (clapCombLen > kClapBufMax) clapCombLen = kClapBufMax;
  for (int i = 0; i < kClapBufMax; ++i) clapBuf[i] = 0.0f;

  // Params
  params[static_cast<int>(DrumParamId::MainVolume)] = Parameter("vol", "", 0.0f, 1.0f, 0.8f, 1.0f / 128);
}

void DrumSynthVoice::setSampleRate(float sampleRateHz) {
  if (sampleRateHz <= 0.0f) sampleRateHz = 44100.0f;
  sampleRate = sampleRateHz;
  invSampleRate = 1.0f / sampleRate;

  // Hat oscillator target freqs (approx 808 metal partials, non-harmonic)
  const float f[6] = { 2150.0f, 2700.0f, 3200.0f, 4100.0f, 5300.0f, 6600.0f };
  const float fo[6] = { 1900.0f, 2500.0f, 3000.0f, 3900.0f, 5100.0f, 6300.0f };
  for (int i = 0; i < 6; ++i) {
    hatInc[i] = f[i] * invSampleRate;
    openHatInc[i] = fo[i] * invSampleRate;
  }

  // Clamp comb length to buffer
  clapCombLen = (int)(0.008f * sampleRate); // ~8 ms
  if (clapCombLen < 64) clapCombLen = 64;
  if (clapCombLen > kClapBufMax) clapCombLen = kClapBufMax;
}

// ---------------- RNG ----------------
float DrumSynthVoice::frand() {
  // xorshift32, returns [-1, 1]
  uint32_t x = rngState;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  rngState = x;
  const float u = (float)(x) * (1.0f / 4294967296.0f); // [0,1)
  return u * 2.0f - 1.0f;
}

// ---------------- Triggers ----------------
void DrumSynthVoice::triggerKick() {
  kickActive = true;
  kickPhase = 0.0f;
  // Tight 606-like envelopes
  kickEnvAmp   = 1.15f;     // fast decay, but a bit hot initially
  kickEnvPitch = 1.0f;      // quick pitch drop
  kickClickEnv = 1.0f;      // short click at onset
  kickFreq     = 60.0f;     // start slightly above final for punch
}

void DrumSynthVoice::triggerSnare() {
  snareActive = true;
  snareEnvAmp  = 1.1f;      // noise sustain
  snareToneEnv = 1.0f;      // tone tick
  snareTonePhase = 0.0f;
  snareTonePhase2 = 0.0f;
}

void DrumSynthVoice::triggerHat() {
  hatActive = true;
  hatEnvAmp  = 0.85f;       // closed hat, shorter
  hatToneEnv = 1.0f;
  // choke open-hat tail
  openHatEnvAmp *= 0.25f;
  for (int i = 0; i < 6; ++i) hatPh[i] = 0.0f;
}

void DrumSynthVoice::triggerOpenHat() {
  openHatActive = true;
  openHatEnvAmp  = 0.95f;   // open hat, longer
  openHatToneEnv = 1.0f;
  for (int i = 0; i < 6; ++i) openHatPh[i] = 0.0f;
}

void DrumSynthVoice::triggerMidTom() {
  midTomActive = true;
  midTomEnv      = 1.0f;
  midTomPitchEnv = 1.0f; // small sweep for character
  midTomPhase    = 0.0f;
}

void DrumSynthVoice::triggerHighTom() {
  highTomActive = true;
  highTomEnv      = 1.0f;
  highTomPitchEnv = 1.0f;
  highTomPhase    = 0.0f;
}

void DrumSynthVoice::triggerRim() {
  rimActive = true;
  rimEnv   = 1.0f;
  rimPhase = 0.0f;
  rimBp = 0.0f;
  rimLp = 0.0f;
}

void DrumSynthVoice::triggerClap() {
  clapActive = true;
  clapEnv    = 1.0f;
  clapTrans  = 1.0f;
  clapNoiseSeed = frand(); // per-hit color
  clapTime   = 0.0f;
  clapIdx    = 0;
}

// ---------------- Processors ----------------
float DrumSynthVoice::processKick() {
  if (!kickActive) return 0.0f;

  // Envelope: fast amp & pitch decay for tight 606-like thump
  kickEnvAmp   *= 0.9965f;         // ~150–200 ms
  kickEnvPitch *= 0.985f;          // fast pitch drop
  kickClickEnv *= 0.92f;           // very short transient

  if (kickEnvAmp < 0.0006f) { kickActive = false; return 0.0f; }

  // pitch factor
  float p = kickEnvPitch * kickEnvPitch;
  float f = 48.0f + 120.0f * p;    // start higher, drop quickly
  kickFreq = f;

  // oscillator
  kickPhase += kickFreq * invSampleRate;
  if (kickPhase >= 1.0f) kickPhase -= 1.0f;

  float body = sinf(2.0f * 3.14159265f * kickPhase);

  // slight drive gives 808/606 style compression feel
  float driven = fast_tanhf(body * (2.6f + 0.7f * kickEnvAmp));

  // click: short high-frequency burst mixed in at onset
  float click = (frand() * 0.5f + 0.5f) * kickClickEnv * 0.3f;

  return (driven * 0.9f + click) * kickEnvAmp;
}

float DrumSynthVoice::processSnare() {
  if (!snareActive) return 0.0f;

  // Envelopes: long noise tail, short tone tick
  snareEnvAmp  *= 0.9983f;     // slow decay
  snareToneEnv *= 0.94f;       // short tick
  if (snareEnvAmp < 0.0002f) { snareActive = false; return 0.0f; }

  // Noise: bright with slight HP emphasis
  float n = frand();           // white
  // crude bandpass around ~1–2 kHz using two one-poles
  const float f = 0.30f;
  snareBp += f * (n - snareLp - 0.22f * snareBp);
  snareLp += f * snareBp;
  float noiseHP = n - snareLp;
  float noiseOut = snareBp * 0.38f + noiseHP * 0.62f;

  // Tones: ~330 Hz and ~180 Hz, short
  snareTonePhase  += 330.0f * invSampleRate; if (snareTonePhase  >= 1.0f) snareTonePhase  -= 1.0f;
  snareTonePhase2 += 180.0f * invSampleRate; if (snareTonePhase2 >= 1.0f) snareTonePhase2 -= 1.0f;
  float toneA = sinf(2.0f * 3.14159265f * snareTonePhase);
  float toneB = sinf(2.0f * 3.14159265f * snareTonePhase2);
  float tone = (toneA * 0.55f + toneB * 0.45f) * snareToneEnv;

  float out = noiseOut * 0.78f + tone * 0.55f;
  return out * snareEnvAmp;
}

float DrumSynthVoice::processHat() {
  if (!hatActive) return 0.0f;

  // Closed hat envelopes: short
  hatEnvAmp  *= 0.996f;  // short tail
  hatToneEnv *= 0.90f;
  if (hatEnvAmp < 0.0005f) { hatActive = false; return 0.0f; }

  // Metal partials: six squares at non-harmonic freqs
  float metal = 0.0f;
  for (int i = 0; i < 6; ++i) {
    hatPh[i] += hatInc[i];
    if (hatPh[i] >= 1.0f) hatPh[i] -= 1.0f;
    metal += (hatPh[i] < 0.5f ? 1.0f : -1.0f); // square
  }
  metal = (metal / 6.0f) * hatToneEnv;

  // Add a little noise for sizzle
  float n = frand() * 0.6f;

  // HP filter (emphasize crispness)
  const float alpha = 0.93f;
  hatHp = alpha * (hatHp + n + metal - hatPrev);
  hatPrev = n + metal;

  // Simple BP tilt
  float out = hatHp * 0.8f + metal * 0.35f;
  return out * hatEnvAmp * 0.75f;
}

float DrumSynthVoice::processOpenHat() {
  if (!openHatActive) return 0.0f;

  openHatEnvAmp  *= 0.9988f;  // longer decay than closed
  openHatToneEnv *= 0.94f;
  if (openHatEnvAmp < 0.0004f) { openHatActive = false; return 0.0f; }

  float metal = 0.0f;
  for (int i = 0; i < 6; ++i) {
    openHatPh[i] += openHatInc[i];
    if (openHatPh[i] >= 1.0f) openHatPh[i] -= 1.0f;
    metal += (openHatPh[i] < 0.5f ? 1.0f : -1.0f);
  }
  metal = (metal / 6.0f) * openHatToneEnv;

  float n = frand() * 0.5f;

  const float alpha = 0.94f;
  openHatHp = alpha * (openHatHp + n + metal - openHatPrev);
  openHatPrev = n + metal;

  float out = openHatHp * 0.65f + metal * 0.55f;
  return out * openHatEnvAmp * 0.8f;
}

float DrumSynthVoice::processMidTom() {
  if (!midTomActive) return 0.0f;

  midTomEnv      *= 0.9991f;     // medium tail
  midTomPitchEnv *= 0.9975f;     // small pitch drop
  if (midTomEnv < 0.0003f) { midTomActive = false; return 0.0f; }

  float base = 170.0f;           // ~808 mid
  float freq = base + 15.0f * (midTomPitchEnv * midTomPitchEnv);
  midTomPhase += freq * invSampleRate; if (midTomPhase >= 1.0f) midTomPhase -= 1.0f;

  float tone = sinf(2.0f * 3.14159265f * midTomPhase);
  // slight noise + subtle drive
  float slightNoise = frand() * 0.03f;
  float driven = fast_tanhf(tone * 2.0f);

  return (driven * 0.9f + slightNoise) * midTomEnv * 0.85f;
}

float DrumSynthVoice::processHighTom() {
  if (!highTomActive) return 0.0f;

  highTomEnv      *= 0.9990f;
  highTomPitchEnv *= 0.997f;
  if (highTomEnv < 0.0003f) { highTomActive = false; return 0.0f; }

  float base = 230.0f;           // ~808 high
  float freq = base + 18.0f * (highTomPitchEnv * highTomPitchEnv);
  highTomPhase += freq * invSampleRate; if (highTomPhase >= 1.0f) highTomPhase -= 1.0f;

  float tone = sinf(2.0f * 3.14159265f * highTomPhase);
  float slightNoise = frand() * 0.028f;
  float driven = fast_tanhf(tone * 2.0f);

  return (driven * 0.88f + slightNoise) * highTomEnv * 0.8f;
}

float DrumSynthVoice::processRim() {
  if (!rimActive) return 0.0f;

  rimEnv *= 0.9978f; // very short
  if (rimEnv < 0.0006f) { rimActive = false; return 0.0f; }

  // Short tick + bandpass ~1.4 kHz for woody rim shot
  rimPhase += 1400.0f * invSampleRate; if (rimPhase >= 1.0f) rimPhase -= 1.0f;
  float tick = sinf(2.0f * 3.14159265f * rimPhase) * 0.6f;

  float n = frand();
  const float f = 0.35f;
  rimBp += f * (n - rimLp - 0.30f * rimBp);
  rimLp += f * rimBp;
  float bp = rimBp;

  return (tick * 0.6f + bp * 0.7f) * rimEnv * 0.9f;
}

float DrumSynthVoice::processClap() {
  if (!clapActive) return 0.0f;

  clapEnv   *= 0.99985f;         // long-ish noise body
  clapTrans *= 0.995f;           // transient decays quicker
  clapTime  += invSampleRate;
  if (clapEnv < 0.00025f) { clapActive = false; return 0.0f; }

  // 808 clap: series of short bursts (3–4) followed by a short reverb tail.
  // Burst gates at ~12 ms intervals, decreasing strength.
  float burstGate = 0.0f;
  if (clapTime < 0.012f)       burstGate = 1.0f;
  else if (clapTime < 0.024f)  burstGate = 0.85f;
  else if (clapTime < 0.036f)  burstGate = 0.7f;
  else if (clapTime < 0.048f)  burstGate = 0.55f;

  // Noise color per hit
  float noise = (frand() * 0.7f + clapNoiseSeed * 0.3f) * burstGate;

  // Resonant bandpass around 1 kHz to 1.6 kHz (simple two one-poles)
  static float bp = 0.0f, lp = 0.0f;
  const float f = 0.32f;
  bp += f * (noise - lp - 0.25f * bp);
  lp += f * bp;
  float colored = bp * 0.7f + (noise - lp) * 0.3f;

  // Short comb “reverb” (~8 ms). Use a simple feedback comb.
  // y[n] = x[n] + 0.5 * y[n - D]
  int readIdx = clapIdx - clapCombLen;
  if (readIdx < 0) readIdx += kClapBufMax;
  float tail = clapBuf[readIdx] * 0.5f;
  float y = colored * clapTrans + tail;
  clapBuf[clapIdx] = y;
  clapIdx++; if (clapIdx >= kClapBufMax) clapIdx = 0;

  return y * clapEnv;
}

// ---------------- Params ----------------
const Parameter& DrumSynthVoice::parameter(DrumParamId id) const {
  return params[static_cast<int>(id)];
}

void DrumSynthVoice::setParameter(DrumParamId id, float value) {
  params[static_cast<int>(id)].setValue(value);
}
