#include "mini_drumvoices.h"
#include <math.h>

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

  // Snare (UNCHANGED)
  snareEnvAmp = 0.0f; snareToneEnv = 0.0f; snareActive = false;
  snareBp = 0.0f; snareLp = 0.0f;
  snareTonePhase = 0.0f; snareTonePhase2 = 0.0f;
  snareHpPrev = 0.0f;

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

  clapHp = 0.0f; clapPrev = 0.0f; clapAirLp = 0.0f;

  // cavity bands
  clapBpA = clapLpA = clapBpA2 = clapLpA2 = 0.0f;
  clapBpB = clapLpB = clapBpB2 = clapLpB2 = 0.0f;

  // snaps
  clapSnapPhase1 = clapSnapPhase2 = clapSnapPhase3 = 0.0f;
  clapSnapEnv1 = clapSnapEnv2 = clapSnapEnv3 = 0.0f;
  clapCrackEnv  = 0.0f;

  // multi-tap cluster
  clapTapIdx = 0; clapTapLen = (int)(0.032f * sampleRate) + 64;  // up to ~32 ms
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
  compDecimCounter = 0;
  compLastGainAmp  = 1.0f;

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
  clapD5 = (int)(0.0230f * sampleRate); // ~23 ms
  clapD6 = (int)(0.0270f * sampleRate); // ~27 ms

  clapTapLen = (int)(0.032f * sampleRate) + 64;
  if (clapTapLen < 256) clapTapLen = 256;
  if (clapTapLen > kClapTapBufMax) clapTapLen = kClapTapBufMax;

  // compressor coefficients (fixed times tuned for drums)
  float attackTime  = 0.005f;  // ~5 ms
  float releaseTime = 0.060f;  // ~60 ms
  compAttackCoeff  = 1.0f - expf(-1.0f / (attackTime  * sampleRate));
  compReleaseCoeff = 1.0f - expf(-1.0f / (releaseTime * sampleRate));
}

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
  clapEnv       = 1.0f;   // global body envelope
  clapTrans     = 1.0f;   // transient
  clapTailEnv   = 0.95f;  // tail/body
  clapNoiseSeed = frand();
  clapTime      = 0.0f;

  // shaper states
  clapHp = 0.0f; clapPrev = 0.0f; clapAirLp = 0.0f;

  // cavity bands
  clapBpA = clapLpA = clapBpA2 = clapLpA2 = 0.0f;
  clapBpB = clapLpB = clapBpB2 = clapLpB2 = 0.0f;

  // snaps & crack
  clapSnapPhase1 = clapSnapPhase2 = clapSnapPhase3 = 0.0f;
  clapSnapEnv1 = 1.0f;  // ~1.3 kHz
  clapSnapEnv2 = 1.0f;  // ~1.6 kHz
  clapSnapEnv3 = 0.9f;  // ~2.0 kHz (softer)
  clapCrackEnv = 1.0f;  // very fast transient

  // cluster buffer
  clapTapIdx = 0;
  for (int i = 0; i < clapTapLen; ++i) clapTapBuf[i] = 0.0f;
}

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
  if (!snareActive) return 0.0f;
  // --- ENVELOPES ---
  snareEnvAmp  *= 0.9985f;
  snareToneEnv *= 0.99999f;
  if (snareEnvAmp < 0.0002f) { snareActive = false; return 0.0f; }

  float n = frand();
  float f = 0.28f;
  snareBp += f * (n - snareLp - 0.20f * snareBp);
  snareLp += f * snareBp;
  float noiseHP = n - snareLp;
  float noiseOut = snareBp * 0.35f + noiseHP * 0.65f;

  snareTonePhase  += 330.0f * invSampleRate; if (snareTonePhase  >= 1.0f) snareTonePhase  -= 1.0f;
  snareTonePhase2 += 180.0f * invSampleRate; if (snareTonePhase2 >= 1.0f) snareTonePhase2 -= 1.0f;
  float toneA = sinf(2.0f * 3.14159265f * snareTonePhase);
  float toneB = sinf(2.0f * 3.14159265f * snareTonePhase2);
  float tone = (toneA * 0.55f + toneB * 0.45f) * snareToneEnv;

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

float DrumSynthVoice::processClap() {
  if (!clapActive) return 0.0f;

  // envelopes
  clapEnv     *= 0.99993f;  // overall tail (slow)
  clapTrans   *= 0.995f;    // transient (fast)
  clapTailEnv *= 0.99990f;  // tail/body (medium)

  // snaps & crack decay quickly (give “hand” crack, but die fast)
  clapSnapEnv1 *= 0.92f;
  clapSnapEnv2 *= 0.90f;
  clapSnapEnv3 *= 0.88f;
  clapCrackEnv *= 0.90f;

  clapTime    += invSampleRate;
  if (clapTime > 0.30f || clapEnv < 0.0002f) { clapActive = false; return 0.0f; }

  // Four Gaussian bursts ~13 ms apart (hands)
  const float tau = 0.0042f; // narrower => less “busy” spectrum
  const float t0  = 0.000f, t1 = 0.013f, t2 = 0.026f, t3 = 0.039f;
  const float a0  = 1.00f,   a1 = 0.80f,  a2 = 0.65f,  a3 = 0.55f;
  float burst = 0.0f, dt = 0.0f;
  dt = clapTime - t0; burst += a0 * expf(-(dt * dt) / (tau * tau));
  dt = clapTime - t1; burst += a1 * expf(-(dt * dt) / (tau * tau));
  dt = clapTime - t2; burst += a2 * expf(-(dt * dt) / (tau * tau));
  dt = clapTime - t3; burst += a3 * expf(-(dt * dt) / (tau * tau));

  // base noise
  float w = (frand() * 0.55f + clapNoiseSeed * 0.45f);

  // high-pass to remove lows + low-pass “air” to tame hiss
  const float hpAlpha = 0.955f;                 // slightly lower = less hiss
  clapHp = hpAlpha * (clapHp + w - clapPrev);
  clapPrev = w;

  const float lpAlpha = 0.15f;                  // stronger air LP
  clapAirLp += lpAlpha * (clapHp - clapAirLp);
  float bandInput = clapAirLp;

  // two independent cavity bands (each cascaded to narrow the band)
  // Formant A (lower mid cavity)
  const float bpFA = 0.29f, dampA = 0.26f;
  clapBpA  += bpFA * (bandInput - clapLpA  - dampA * clapBpA);
  clapLpA  += bpFA * clapBpA;
  clapBpA2 += bpFA * (clapBpA     - clapLpA2 - dampA * clapBpA2);
  clapLpA2 += bpFA * clapBpA2;

  // formant B - upper mid cavity
  const float bpFB = 0.33f, dampB = 0.24f;
  clapBpB  += bpFB * (bandInput - clapLpB  - dampB * clapBpB);
  clapLpB  += bpFB * clapBpB;
  clapBpB2 += bpFB * (clapBpB     - clapLpB2 - dampB * clapBpB2);
  clapLpB2 += bpFB * clapBpB2;

  // narrow bands summed for hollow feel
  float bandNarrow = clapBpA2 * 0.55f + clapBpB2 * 0.45f;

  // short tonal snaps near 1.3/1.6/2.0 kHz
  clapSnapPhase1 += 1300.0f * invSampleRate; if (clapSnapPhase1 >= 1.0f) clapSnapPhase1 -= 1.0f;
  clapSnapPhase2 += 1600.0f * invSampleRate; if (clapSnapPhase2 >= 1.0f) clapSnapPhase2 -= 1.0f;
  clapSnapPhase3 += 2000.0f * invSampleRate; if (clapSnapPhase3 >= 1.0f) clapSnapPhase3 -= 1.0f;

  float snap =
      sinf(2.0f * 3.14159265f * clapSnapPhase1) * clapSnapEnv1 * 0.50f +
      sinf(2.0f * 3.14159265f * clapSnapPhase2) * clapSnapEnv2 * 0.55f +
      sinf(2.0f * 3.14159265f * clapSnapPhase3) * clapSnapEnv3 * 0.45f;

  // extra transient crack (fast, only at burst peaks)
  float crack = (bandInput - bandNarrow) * 0.40f * clapCrackEnv;

  // body: narrow-band noise + snaps + crack, gated by burst
  float body = (bandNarrow * 0.90f + snap * 0.55f + crack * 0.50f) * burst * clapTrans;

  // tail: quieter narrow-band noise (keeps it “clappy” vs. a noise blip)
  float tail = bandNarrow * 0.48f * clapTailEnv;

  // feed-forward multi-hand cluster
  clapTapBuf[clapTapIdx] = body;
  int idx = clapTapIdx;
  int i1 = idx - clapD1; if (i1 < 0) i1 += clapTapLen;
  int i2 = idx - clapD2; if (i2 < 0) i2 += clapTapLen;
  int i3 = idx - clapD3; if (i3 < 0) i3 += clapTapLen;
  int i4 = idx - clapD4; if (i4 < 0) i4 += clapTapLen;
  int i5 = idx - clapD5; if (i5 < 0) i5 += clapTapLen;
  int i6 = idx - clapD6; if (i6 < 0) i6 += clapTapLen;

  float y = body
          + clapTapBuf[i1] * 0.55f
          + clapTapBuf[i2] * 0.40f
          + clapTapBuf[i3] * 0.28f
          + clapTapBuf[i4] * 0.20f
          + clapTapBuf[i5] * 0.13f
          + clapTapBuf[i6] * 0.09f
          + tail;

  clapTapIdx++; if (clapTapIdx >= clapTapLen) clapTapIdx = 0;

  return y * clapEnv;
}

// Bus Compressor
float DrumSynthVoice::processBus(float mixSample) {
  const int kCompDecim = 4; // for tighter response, use 2

  if (compDecimCounter == 0) {
    compAmount   = params[static_cast<int>(DrumParamId::BusCompAmount)].value();
    compThreshDb = -18.0f + 12.0f * compAmount; // -18 .. -6 dB
    compRatio    =  2.0f +  4.0f * compAmount;  // 2:1  .. 6:1
    compMakeupDb =  6.0f * compAmount;          // up to ~+6 dB
    compKneeDb   =  6.0f;

    // bus comp detector
    float inAbs  = fabsf(mixSample);
    float target = inAbs;
    float coeff  = (target > compEnv) ? compAttackCoeff : compReleaseCoeff;
    compEnv += coeff * (target - compEnv);

    // dB-domain soft knee
    float levelDb = amp_to_db(compEnv);
    float overDb  = levelDb - compThreshDb;
    float grDb    = 0.0f;
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

    // smooth gain reduction
    const float grSmooth = 0.8f;
    compGainDb = grSmooth * compGainDb + (1.0f - grSmooth) * grDb;

    // convert to amplitude + makeup once per update
    compLastGainAmp = db_to_amp(compGainDb + compMakeupDb);
  }

  compDecimCounter = (compDecimCounter + 1) % kCompDecim;
  return mixSample * compLastGainAmp;
}

const Parameter& DrumSynthVoice::parameter(DrumParamId id) const {
  return params[static_cast<int>(id)];
}

void DrumSynthVoice::setParameter(DrumParamId id, float value) {
  params[static_cast<int>(id)].setValue(value);
}