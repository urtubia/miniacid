
#include "mini_drumvoices.h"
#include <math.h>

// ---------------- Utility ----------------
static inline float fast_tanhf(float x) {
  const float x2 = x * x;
  return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}
static inline float db_to_amp(float db) { return powf(10.0f, db * 0.05f); }
static inline float amp_to_db(float a) {
  const float eps = 1e-12f;
  return 20.0f * log10f(fabsf(a) + eps);
}

DrumSynthVoice::DrumSynthVoice(float sampleRate)
  : sampleRate(sampleRate), invSampleRate(0.0f), rngState(0x12345678u),
    compEnv(0.0f), compAttackCoeff(0.0f), compReleaseCoeff(0.0f),
    compGainDb(0.0f), compMakeupDb(0.0f), compThreshDb(-12.0f),
    compRatio(3.0f), compKneeDb(6.0f), compAmount(0.35f) {
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

  // Clap (revamped)
  clapEnv = 0.0f; clapTrans = 0.0f; clapTailEnv = 0.0f;
  clapNoiseSeed = 0.0f; clapActive = false; clapTime = 0.0f;
  clapHp = 0.0f; clapPrev = 0.0f; clapBp = 0.0f; clapLp = 0.0f;
  clapBp2 = 0.0f; clapLp2 = 0.0f;
  clapSnapPhase = 0.0f; clapSnapEnv = 0.0f;
  clapTapIdx = 0; clapTapLen = (int)(0.020f * sampleRate) + 64; // > 19ms + margin
  if (clapTapLen < 256) clapTapLen = 256;
  if (clapTapLen > kClapTapBufMax) clapTapLen = kClapTapBufMax;
  for (int i = 0; i < kClapTapBufMax; ++i) clapTapBuf[i] = 0.0f;

  // Bus compressor defaults
  compAmount   = 0.35f;
  compThreshDb = -18.0f + 12.0f * compAmount;       // -18 .. -6 dB
  compRatio    =  2.0f   + 4.0f  * compAmount;      // 2:1 .. 6:1
  compKneeDb   =  6.0f;
  compMakeupDb =  6.0f * compAmount;
  compEnv      =  0.0f;
  compGainDb   =  0.0f;

  // Params
  params[static_cast<int>(DrumParamId::MainVolume)]    = Parameter("vol", "Main volume", 0.0f, 1.0f, 0.8f, 1.0f / 128);
  params[static_cast<int>(DrumParamId::BusCompAmount)] = Parameter("comp", "Bus comp amount", 0.0f, 1.0f, compAmount, 1.0f / 128);
}

void DrumSynthVoice::setSampleRate(float sampleRateHz) {
  if (sampleRateHz <= 0.0f) sampleRateHz = 44100.0f;
  sampleRate = sampleRateHz;
  invSampleRate = 1.0f / sampleRate;

  // Hat oscillator target freqs (approx 808 metal partials, non-harmonic)
  const float f[6]  = { 2150.0f, 2700.0f, 3200.0f, 4100.0f, 5300.0f, 6600.0f };
  const float fo[6] = { 1900.0f, 2500.0f, 3000.0f, 3900.0f, 5100.0f, 6300.0f };
  for (int i = 0; i < 6; ++i) {
    hatInc[i]     = f[i]  * invSampleRate;
    openHatInc[i] = fo[i] * invSampleRate;
  }

  // Multi-tap feed-forward delays for clap cluster (in samples)
  clapD1 = (int)(0.0045f * sampleRate); // ~4.5 ms
  clapD2 = (int)(0.0090f * sampleRate); // ~9 ms
  clapD3 = (int)(0.0140f * sampleRate); // ~14 ms
  clapD4 = (int)(0.0190f * sampleRate); // ~19 ms
  clapTapLen = (int)(0.020f * sampleRate) + 64;
  if (clapTapLen < 256) clapTapLen = 256;
  if (clapTapLen > kClapTapBufMax) clapTapLen = kClapTapBufMax;

  // Compressor coefficients (fixed times tuned for drums)
  float attackTime  = 0.005f;  // ~5 ms
  float releaseTime = 0.060f;  // ~60 ms
  compAttackCoeff  = 1.0f - expf(-1.0f / (attackTime  * sampleRate));
  compReleaseCoeff = 1.0f - expf(-1.0f / (releaseTime * sampleRate));
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
  kickEnvAmp   = 1.15f;
  kickEnvPitch = 1.0f;
  kickClickEnv = 1.0f;
  kickFreq     = 60.0f;
}

void DrumSynthVoice::triggerSnare() {
  snareActive = true;
  snareEnvAmp  = 1.1f;
  snareToneEnv = 1.0f;
  snareTonePhase = 0.0f;
  snareTonePhase2 = 0.0f;
}

void DrumSynthVoice::triggerHat() {
  hatActive = true;
  hatEnvAmp  = 0.85f;
  hatToneEnv = 1.0f;
  openHatEnvAmp *= 0.25f; // choke
  for (int i = 0; i < 6; ++i) hatPh[i] = 0.0f;
}

void DrumSynthVoice::triggerOpenHat() {
  openHatActive = true;
  openHatEnvAmp  = 0.95f;
  openHatToneEnv = 1.0f;
  for (int i = 0; i < 6; ++i) openHatPh[i] = 0.0f;
}

void DrumSynthVoice::triggerMidTom() {
  midTomActive = true;
  midTomEnv      = 1.0f;
  midTomPitchEnv = 1.0f;
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
  clapActive    = true;
  clapEnv       = 1.0f;   // global body
  clapTrans     = 1.0f;   // transient
  clapTailEnv   = 0.90f;  // independent tail body
  clapNoiseSeed = frand();
  clapTime      = 0.0f;

  // shaper states
  clapHp = 0.0f; clapPrev = 0.0f;
  clapBp = 0.0f; clapLp = 0.0f;
  clapBp2 = 0.0f; clapLp2 = 0.0f;

  // snap tone
  clapSnapPhase = 0.0f;
  clapSnapEnv   = 1.0f;

  // cluster buffer
  clapTapIdx = 0;
  for (int i = 0; i < clapTapLen; ++i) clapTapBuf[i] = 0.0f;
}

// ---------------- Processors: Voices ----------------
float DrumSynthVoice::processKick() {
  if (!kickActive) return 0.0f;

  kickEnvAmp   *= 0.9965f;
  kickEnvPitch *= 0.985f;
  kickClickEnv *= 0.92f;

  if (kickEnvAmp < 0.0006f) { kickActive = false; return 0.0f; }

  float p = kickEnvPitch * kickEnvPitch;
  float f = 48.0f + 120.0f * p;
  kickFreq = f;

  kickPhase += kickFreq * invSampleRate;
  if (kickPhase >= 1.0f) kickPhase -= 1.0f;

  float body = sinf(2.0f * 3.14159265f * kickPhase);
  float driven = fast_tanhf(body * (2.6f + 0.7f * kickEnvAmp));
  float click = (frand() * 0.5f + 0.5f) * kickClickEnv * 0.3f;

  return (driven * 0.9f + click) * kickEnvAmp;
}

float DrumSynthVoice::processSnare() {
  if (!snareActive)
    return 0.0f;
  // --- ENVELOPES ---
  // 808: Long noise decay, short tone decay
  snareEnvAmp *= 0.9985f; // slow decay, long tail
  snareToneEnv *= 0.99999f; // short tone "tick"
  if (snareEnvAmp < 0.0002f) {
    snareActive = false;
    return 0.0f;
  }
  // --- NOISE PROCESSING ---
  float n = frand(); // assume 0.0–1.0 random
  // 808: Noise is brighter with a bit of highpass emphasis
  // simple bandpass around ~1–2 kHz
  float f = 0.28f;
  snareBp += f * (n - snareLp - 0.20f * snareBp);
  snareLp += f * snareBp;
  // high fizz (808 has a lot of it)
  float noiseHP = n - snareLp; // crude highpass
  float noiseOut = snareBp * 0.35f + noiseHP * 0.65f;
  // --- TONE (two sines, tuned to classic 808) ---
  // ~330 Hz + ~180 Hz slight mix, short decay
  snareTonePhase += 330.0f * invSampleRate;
  if (snareTonePhase >= 1.0f) snareTonePhase -= 1.0f;
  snareTonePhase2 += 180.0f * invSampleRate;
  if (snareTonePhase2 >= 1.0f) snareTonePhase2 -= 1.0f;
  float toneA = sinf(2.0f * 3.14159265f * snareTonePhase);
  float toneB = sinf(2.0f * 3.14159265f * snareTonePhase2);
  float tone = (toneA * 0.55f + toneB * 0.45f) * snareToneEnv;
  // --- MIX ---
  // 808: tone only supports transient, noise dominates sustain
  float out = noiseOut * 0.75f + tone * 0.65f;
  return out * snareEnvAmp;
}

float DrumSynthVoice::processHat() {
  if (!hatActive) return 0.0f;

  hatEnvAmp  *= 0.996f;
  hatToneEnv *= 0.90f;
  if (hatEnvAmp < 0.0005f) { hatActive = false; return 0.0f; }

  float metal = 0.0f;
  for (int i = 0; i < 6; ++i) {
    hatPh[i] += hatInc[i];
    if (hatPh[i] >= 1.0f) hatPh[i] -= 1.0f;
    metal += (hatPh[i] < 0.5f ? 1.0f : -1.0f);
  }
  metal = (metal / 6.0f) * hatToneEnv;

  float n = frand() * 0.6f;

  const float alpha = 0.93f;
  hatHp = alpha * (hatHp + n + metal - hatPrev);
  hatPrev = n + metal;

  float out = hatHp * 0.8f + metal * 0.35f;
  return out * hatEnvAmp * 0.75f;
}

float DrumSynthVoice::processOpenHat() {
  if (!openHatActive) return 0.0f;

  openHatEnvAmp  *= 0.9988f;
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

  midTomEnv      *= 0.9991f;
  midTomPitchEnv *= 0.9975f;
  if (midTomEnv < 0.0003f) { midTomActive = false; return 0.0f; }

  float base = 170.0f;
  float freq = base + 15.0f * (midTomPitchEnv * midTomPitchEnv);
  midTomPhase += freq * invSampleRate; if (midTomPhase >= 1.0f) midTomPhase -= 1.0f;

  float tone = sinf(2.0f * 3.14159265f * midTomPhase);
  float slightNoise = frand() * 0.03f;
  float driven = fast_tanhf(tone * 2.0f);

  return (driven * 0.9f + slightNoise) * midTomEnv * 0.85f;
}

float DrumSynthVoice::processHighTom() {
  if (!highTomActive) return 0.0f;

  highTomEnv      *= 0.9990f;
  highTomPitchEnv *= 0.997f;
  if (highTomEnv < 0.0003f) { highTomActive = false; return 0.0f; }

  float base = 230.0f;
  float freq = base + 18.0f * (highTomPitchEnv * highTomPitchEnv);
  highTomPhase += freq * invSampleRate; if (highTomPhase >= 1.0f) highTomPhase -= 1.0f;

  float tone = sinf(2.0f * 3.14159265f * highTomPhase);
  float slightNoise = frand() * 0.028f;
  float driven = fast_tanhf(tone * 2.0f);

  return (driven * 0.88f + slightNoise) * highTomEnv * 0.8f;
}

float DrumSynthVoice::processRim() {
  if (!rimActive) return 0.0f;

  rimEnv *= 0.9978f;
  if (rimEnv < 0.0006f) { rimActive = false; return 0.0f; }

  rimPhase += 1400.0f * invSampleRate; if (rimPhase >= 1.0f) rimPhase -= 1.0f;
  float tick = sinf(2.0f * 3.14159265f * rimPhase) * 0.6f;

  float n = frand();
  const float f = 0.35f;
  rimBp += f * (n - rimLp - 0.30f * rimBp);
  rimLp += f * rimBp;
  float bp = rimBp;

  return (tick * 0.6f + bp * 0.7f) * rimEnv * 0.9f;
}

// ---------------- Clap (hand-snap + multi-tap cluster, no feedback) ----------------
float DrumSynthVoice::processClap() {
  if (!clapActive) return 0.0f;

  // Envelopes
  clapEnv     *= 0.99992f;  // global tail (slow)
  clapTrans   *= 0.9950f;   // transient (fast)
  clapTailEnv *= 0.99988f;  // tail/body
  clapSnapEnv *= 0.93f;     // very fast snap decay
  clapTime    += invSampleRate;

  if (clapTime > 0.28f || clapEnv < 0.0002f) { clapActive = false; return 0.0f; }

  // Four Gaussian bursts at ~13 ms spacing
  const float tau = 0.0055f; // width controls "hand softness"
  const float t0  = 0.000f, t1 = 0.013f, t2 = 0.026f, t3 = 0.039f;
  const float a0  = 1.00f,   a1 = 0.82f, a2 = 0.68f, a3 = 0.60f;

  float burst = 0.0f, dt = 0.0f;
  dt = clapTime - t0; burst += a0 * expf(-(dt * dt) / (tau * tau));
  dt = clapTime - t1; burst += a1 * expf(-(dt * dt) / (tau * tau));
  dt = clapTime - t2; burst += a2 * expf(-(dt * dt) / (tau * tau));
  dt = clapTime - t3; burst += a3 * expf(-(dt * dt) / (tau * tau));

  // White noise with per-hit color
  float w = (frand() * 0.75f + clapNoiseSeed * 0.25f);

  // Bright shaper: HP + two gentle BP bands (~1.3 kHz & ~2.2 kHz)
  const float hpAlpha = 0.970f;     // brighter than before
  clapHp = hpAlpha * (clapHp + w - clapPrev);
  clapPrev = w;
  float hpOut = clapHp;

  const float bpF1 = 0.32f;  // ~1.3 kHz band
  clapBp  += bpF1 * (hpOut - clapLp  - 0.25f * clapBp);
  clapLp  += bpF1 * clapBp;
  float band1 = clapBp * 0.65f + (hpOut - clapLp) * 0.35f;

  const float bpF2 = 0.38f;  // ~2.2 kHz band (slightly brighter)
  clapBp2 += bpF2 * (hpOut - clapLp2 - 0.22f * clapBp2);
  clapLp2 += bpF2 * clapBp2;
  float band2 = clapBp2 * 0.60f + (hpOut - clapLp2) * 0.40f;

  // Short tonal "snap" around 1.5 kHz (very brief)
  clapSnapPhase += 1500.0f * invSampleRate; if (clapSnapPhase >= 1.0f) clapSnapPhase -= 1.0f;
  float snapTone = sinf(2.0f * 3.14159265f * clapSnapPhase) * clapSnapEnv;

  // Body signal: bursts + snap
  float colored = band1 * 0.58f + band2 * 0.42f;
  float body = (colored * burst * clapTrans) + (snapTone * 0.45f * burst);

  // Tail (longer, quieter) to make it read as a clap rather than a noise blip
  float tail = (band1 * 0.50f + band2 * 0.35f) * clapTailEnv;

  // Feed-forward multi-tap cluster: emulate multiple hands (no feedback)
  // y = body + a1*x[n-d1] + a2*x[n-d2] + a3*x[n-d3] + a4*x[n-d4] + tail
  // Write body into ring buffer, then read taps.
  clapTapBuf[clapTapIdx] = body;
  int idx = clapTapIdx;
  int i1 = idx - clapD1; if (i1 < 0) i1 += clapTapLen;
  int i2 = idx - clapD2; if (i2 < 0) i2 += clapTapLen;
  int i3 = idx - clapD3; if (i3 < 0) i3 += clapTapLen;
  int i4 = idx - clapD4; if (i4 < 0) i4 += clapTapLen;

  float y = body
          + clapTapBuf[i1] * 0.60f
          + clapTapBuf[i2] * 0.45f
          + clapTapBuf[i3] * 0.30f
          + clapTapBuf[i4] * 0.20f
          + tail;

  // advance ring index
  clapTapIdx++; if (clapTapIdx >= clapTapLen) clapTapIdx = 0;

  // global body envelope
  return y * clapEnv;
}

// ---------------- Bus Compressor ----------------
float DrumSynthVoice::processBus(float mixSample) {
  // Update parameter cache (in case UI changed it)
  compAmount   = params[static_cast<int>(DrumParamId::BusCompAmount)].value();
  compThreshDb = -18.0f + 12.0f * compAmount; // -18 .. -6 dB
  compRatio    =  2.0f +  4.0f * compAmount;  // 2:1  .. 6:1
  compMakeupDb =  6.0f * compAmount;          // up to ~+6 dB
  compKneeDb   =  6.0f;                       // fixed knee

  // Detector: peak-ish envelope follower
  float inAbs = fabsf(mixSample);
  float target = inAbs;
  float coeff  = (target > compEnv) ? compAttackCoeff : compReleaseCoeff;
  compEnv += coeff * (target - compEnv);

  // Envelope to dB and soft-knee gain reduction
  float levelDb = amp_to_db(compEnv);
  float overDb = levelDb - compThreshDb;
  float grDb = 0.0f;
  if (overDb <= -compKneeDb * 0.5f) {
    grDb = 0.0f;
  } else if (overDb < compKneeDb * 0.5f) {
    float x = (overDb + compKneeDb * 0.5f) / compKneeDb; // 0..1
    float kneeGain = (1.0f / compRatio - 1.0f) * (x * x);
    grDb = kneeGain * compKneeDb; // negative
  } else {
    float levelOutDb = compThreshDb + overDb / compRatio;
    grDb = levelOutDb - levelDb; // negative
  }

  // Smooth gain reduction
  const float grSmooth = 0.8f;
  compGainDb = grSmooth * compGainDb + (1.0f - grSmooth) * grDb;

  // Apply makeup and reduction
  float out = mixSample * db_to_amp(compGainDb + compMakeupDb);
  return out;
}

// ---------------- Params ----------------
const Parameter& DrumSynthVoice::parameter(DrumParamId id) const {
  return params[static_cast<int>(id)];
}

void DrumSynthVoice::setParameter(DrumParamId id, float value) {
  params[static_cast<int>(id)].setValue(value);
}
