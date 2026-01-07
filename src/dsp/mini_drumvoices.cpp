#include "mini_drumvoices.h"
#include <math.h>
#include <stdlib.h>

// Utility: fast clamp
static inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

DrumSynthVoice::DrumSynthVoice(float sr)
  : sampleRate(sr),
    invSampleRate(0.0f) {
  setSampleRate(sr);
  reset();
}

void DrumSynthVoice::reset() {
  // Kick (808-ish)
  kickPhase = 0.0f;
  kickEnvAmp = 0.0f;
  kickEnvPitch = 0.0f;
  kickActive = false;

  // Snare
  snareEnvAmp = 0.0f;
  snareToneEnv = 0.0f;
  snareActive = false;
  snareBp = 0.0f;
  snareLp = 0.0f;
  snareTonePhase = 0.0f;
  snareTonePhase2 = 0.0f;

  // 606 Schmitt-trigger oscillator set (analysis-derived)
  hatOscFreqs[0] = 245.10f;
  hatOscFreqs[1] = 308.60f;
  hatOscFreqs[2] = 367.60f;
  hatOscFreqs[3] = 416.60f;
  hatOscFreqs[4] = 438.50f;
  hatOscFreqs[5] = 625.00f;

  // Hats state
  hatEnvAmp = 0.0f; hatToneEnv = 0.0f; hatActive = false;
  openHatEnvAmp = 0.0f; openHatToneEnv = 0.0f; openHatActive = false;
  for (int i = 0; i < 6; ++i) { hatPhases[i] = 0.0f; openHatPhases[i] = 0.0f; }

  // Biquad BP coefficients
  // CH center ~7100 Hz, Q ~0.9 (classic 606 hats band-pass)
  {
    float f0 = 7100.0f;
    float w0 = 2.0f * 3.14159265f * f0 / sampleRate;
    float alpha = sinf(w0) / (2.0f * 0.9f);
    float cosw0 = cosf(w0);
    float b0 = alpha, b1 = 0.0f, b2 = -alpha;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cosw0;
    float a2 = 1.0f - alpha;
    bp_b0_ch = b0 / a0; bp_b1_ch = b1 / a0; bp_b2_ch = b2 / a0; bp_a1_ch = a1 / a0; bp_a2_ch = a2 / a0;
  }
  // OH center slightly higher (~7800 Hz), Q ~0.85 for brighter, higher perceived pitch
  {
    float f0 = 7800.0f;
    float w0 = 2.0f * 3.14159265f * f0 / sampleRate;
    float alpha = sinf(w0) / (2.0f * 0.85f);
    float cosw0 = cosf(w0);
    float b0 = alpha, b1 = 0.0f, b2 = -alpha;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cosw0;
    float a2 = 1.0f - alpha;
    bp_b0_oh = b0 / a0; bp_b1_oh = b1 / a0; bp_b2_oh = b2 / a0; bp_a1_oh = a1 / a0; bp_a2_oh = a2 / a0;
  }

  hatBP_x1 = hatBP_x2 = hatBP_y1 = hatBP_y2 = 0.0f;
  openHatBP_x1 = openHatBP_x2 = openHatBP_y1 = openHatBP_y2 = 0.0f;

  // Post-VCA HP (one-pole): y[n] = a*(y[n-1] + x[n] - x[n-1])
  hatHP_y1 = hatHP_x1 = 0.0f;
  openHatHP_y1 = openHatHP_x1 = 0.0f;

  // Toms
  midTomPhase = 0.0f; midTomEnv = 0.0f; midTomActive = false;
  highTomPhase = 0.0f; highTomEnv = 0.0f; highTomActive = false;

  // Rim
  rimPhase = 0.0f; rimEnv = 0.0f; rimActive = false;

  // Clap
  clapEnv = 0.0f; clapTrans = 0.0f; clapNoise = 0.0f; clapActive = false; clapDelay = 0.0f;

  // Parameters
  params[static_cast<int>(DrumParamId::MainVolume)] = Parameter("vol", "", 0.0f, 1.0f, 0.8f, 1.0f / 128);
}

void DrumSynthVoice::setSampleRate(float sampleRateHz) {
  if (sampleRateHz <= 0.0f) sampleRateHz = 44100.0f;
  sampleRate = sampleRateHz;
  invSampleRate = 1.0f / sampleRate;
}

// --- Triggers ---
void DrumSynthVoice::triggerKick() {
  kickActive = true;
  kickPhase = 0.0f;
  kickEnvAmp = 1.0f;        // main amplitude envelope
  kickEnvPitch = 1.0f;      // fast attack pitch envelope
}

void DrumSynthVoice::triggerSnare() {
  snareActive = true;
  snareEnvAmp = 1.0f;
  snareToneEnv = 1.0f;
  snareTonePhase = 0.0f;
  snareTonePhase2 = 0.0f;
}

void DrumSynthVoice::triggerHat() {
  hatActive = true;
  hatEnvAmp = 1.0f;
  hatToneEnv = 1.0f;
  for (int i = 0; i < 6; ++i) hatPhases[i] = 0.0f;
  // choke open-hat tail (606 behavior)
  openHatEnvAmp *= 0.3f;
}

void DrumSynthVoice::triggerOpenHat() {
  openHatActive = true;
  openHatEnvAmp = 1.0f;
  openHatToneEnv = 1.0f;
  for (int i = 0; i < 6; ++i) openHatPhases[i] = 0.0f;
}

void DrumSynthVoice::triggerMidTom() {
  midTomActive = true; midTomEnv = 1.0f; midTomPhase = 0.0f;
}

void DrumSynthVoice::triggerHighTom() {
  highTomActive = true; highTomEnv = 1.0f; highTomPhase = 0.0f;
}

void DrumSynthVoice::triggerRim() {
  rimActive = true; rimEnv = 1.0f; rimPhase = 0.0f;
}

void DrumSynthVoice::triggerClap() {
  clapActive = true; clapEnv = 1.0f; clapTrans = 1.0f; clapNoise = frand(); clapDelay = 0.0f;
}

float DrumSynthVoice::frand() {
  return (float)rand() / (float)RAND_MAX * 2.0f - 1.0f;
}

// --- Kick (TR-808-flavored): decaying sine with short click and subtle pitch drop ---
float DrumSynthVoice::processKick() {
  if (!kickActive) return 0.0f;

  // amplitude & pitch envelopes
  kickEnvAmp   *= 0.9996f;        // long tail (808-style)
  kickEnvPitch *= 0.9925f;        // very quick drop for attack punch
  if (kickEnvAmp < 0.0003f) { kickActive = false; return 0.0f; }

  // Base frequency near 50–56 Hz; add small transient pitch rise then drop
  float baseF = 55.0f;
  float pitchAmt = 20.0f;         // transient amount
  float f = baseF + pitchAmt * (kickEnvPitch * kickEnvPitch);

  // integrate phase
  kickPhase += f * invSampleRate;
  if (kickPhase >= 1.0f) kickPhase -= 1.0f;
  float sine = sinf(2.0f * 3.14159265f * kickPhase);

  // short click (filtered noise burst)
  float click = clampf((kickEnvPitch > 0.6f) ? frand() * 0.2f : 0.0f, -0.2f, 0.2f);

  // tone: sine through gentle drive
  float tone = tanhf((sine * 2.4f) + click);
  return tone * kickEnvAmp * 0.95f;
}

// --- Snare (606-leaning: brighter noise + short tonal tick) ---
float DrumSynthVoice::processSnare() {
  if (!snareActive) return 0.0f;

  // envelopes
  snareEnvAmp  *= 0.9985f;   // slow-ish noise decay
  snareToneEnv *= 0.97f;     // very short tone
  if (snareEnvAmp < 0.0002f) { snareActive = false; return 0.0f; }

  // brighter noise
  float n = frand();
  float hp = n - snareLp;                // crude HP
  float bpCoeff = 0.22f;                 // narrower band-pass (~2–3 kHz region)
  snareBp += bpCoeff * (hp - 0.27f * snareBp);
  snareLp += bpCoeff * snareBp;
  float noiseOut = (hp * 0.55f + snareBp * 0.45f);

  // tonal tick (two short sines)
  snareTonePhase  += 260.0f * invSampleRate; if (snareTonePhase  >= 1.0f) snareTonePhase  -= 1.0f;
  snareTonePhase2 += 420.0f * invSampleRate; if (snareTonePhase2 >= 1.0f) snareTonePhase2 -= 1.0f;
  float toneA = sinf(2.0f * 3.14159265f * snareTonePhase);
  float toneB = sinf(2.0f * 3.14159265f * snareTonePhase2);
  float tone  = (toneA * 0.6f + toneB * 0.4f) * (snareToneEnv * 0.25f);

  float out = tanhf(noiseOut * 1.6f + tone * 0.75f);
  return out * snareEnvAmp * 0.9f;
}

// --- Biquad BP apply helper ---
static inline float biquad_bp(float x, float b0, float b1, float b2, float a1, float a2,
                              float& x1, float& x2, float& y1, float& y2) {
  float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
  x2 = x1; x1 = x; y2 = y1; y1 = y;
  return y;
}

// --- Closed Hat (TR-606-style): six squares -> BP ~7.1 kHz -> VCA -> HPF ---
float DrumSynthVoice::processHat() {
  if (!hatActive) return 0.0f;

  hatEnvAmp *= 0.9965f;      // fast but audible CH decay
  hatToneEnv *= 0.98f;
  if (hatEnvAmp < 0.0006f) { hatActive = false; return 0.0f; }

  // six square oscillators
  float mix = 0.0f;
  for (int i = 0; i < 6; ++i) {
    hatPhases[i] += hatOscFreqs[i] * invSampleRate;
    if (hatPhases[i] >= 1.0f) hatPhases[i] -= 1.0f;
    float sq = (hatPhases[i] < 0.5f) ? 1.0f : -1.0f;
    mix += sq;
  }
  mix *= (1.0f / 6.0f);

  // band-pass around ~7.1k (robust biquad, CH coefficients)
  float bp = biquad_bp(mix, bp_b0_ch, bp_b1_ch, bp_b2_ch, bp_a1_ch, bp_a2_ch,
                       hatBP_x1, hatBP_x2, hatBP_y1, hatBP_y2);

  // VCA + gentle saturation
  float vca = bp * hatEnvAmp;
  float driven = tanhf(vca * 1.8f);

  // post-VCA high-pass (a ~ 0.98)
  const float a = 0.98f;
  float yhp = a * (hatHP_y1 + driven - hatHP_x1);
  hatHP_y1 = yhp; hatHP_x1 = driven;

  return yhp * 0.9f;
}

// --- Open Hat (brighter): same source; higher BP center; longer decay; choke by CH trigger) ---
float DrumSynthVoice::processOpenHat() {
  if (!openHatActive) return 0.0f;

  openHatEnvAmp *= 0.9990f;   // longer decay than CH
  openHatToneEnv *= 0.992f;
  if (openHatEnvAmp < 0.0005f) { openHatActive = false; return 0.0f; }

  float mix = 0.0f;
  for (int i = 0; i < 6; ++i) {
    openHatPhases[i] += hatOscFreqs[i] * invSampleRate;
    if (openHatPhases[i] >= 1.0f) openHatPhases[i] -= 1.0f;
    float sq = (openHatPhases[i] < 0.5f) ? 1.0f : -1.0f;
    mix += sq;
  }
  mix *= (1.0f / 6.0f);

  // band-pass with OH coefficients (higher center for brighter pitch)
  float bp = biquad_bp(mix, bp_b0_oh, bp_b1_oh, bp_b2_oh, bp_a1_oh, bp_a2_oh,
                       openHatBP_x1, openHatBP_x2, openHatBP_y1, openHatBP_y2);

  float vca = bp * openHatEnvAmp;
  float driven = tanhf(vca * 1.65f);

  const float a = 0.985f; // slightly less HP than CH to keep airy top
  float yhp = a * (openHatHP_y1 + driven - openHatHP_x1);
  openHatHP_y1 = yhp; openHatHP_x1 = driven;

  return yhp * 0.95f;
}

// --- Toms ---
float DrumSynthVoice::processMidTom() {
  if (!midTomActive) return 0.0f;
  midTomEnv *= 0.99925f;
  if (midTomEnv < 0.0003f) { midTomActive = false; return 0.0f; }
  float freq = 180.0f;
  midTomPhase += freq * invSampleRate;
  if (midTomPhase >= 1.0f) midTomPhase -= 1.0f;
  float tone = sinf(2.0f * 3.14159265f * midTomPhase);
  float slightNoise = frand() * 0.05f;
  return (tone * 0.9f + slightNoise) * midTomEnv * 0.8f;
}

float DrumSynthVoice::processHighTom() {
  if (!highTomActive) return 0.0f;
  highTomEnv *= 0.99915f;
  if (highTomEnv < 0.0003f) { highTomActive = false; return 0.0f; }
  float freq = 240.0f;
  highTomPhase += freq * invSampleRate;
  if (highTomPhase >= 1.0f) highTomPhase -= 1.0f;
  float tone = sinf(2.0f * 3.14159265f * highTomPhase);
  float slightNoise = frand() * 0.04f;
  return (tone * 0.88f + slightNoise) * highTomEnv * 0.75f;
}

// --- Rim ---
float DrumSynthVoice::processRim() {
  if (!rimActive) return 0.0f;
  rimEnv *= 0.9985f;
  if (rimEnv < 0.0004f) { rimActive = false; return 0.0f; }
  rimPhase += 900.0f * invSampleRate;
  if (rimPhase >= 1.0f) rimPhase -= 1.0f;
  float tone = sinf(2.0f * 3.14159265f * rimPhase);
  float click = (frand() * 0.6f + 0.4f) * rimEnv;
  return (tone * 0.5f + click) * rimEnv * 0.8f;
}

// --- Clap ---
float DrumSynthVoice::processClap() {
  if (!clapActive) return 0.0f;
  clapEnv  *= 0.99992f;
  clapTrans *= 0.9985f;
  clapDelay += invSampleRate;
  if (clapEnv < 0.0002f) { clapActive = false; return 0.0f; }

  float burst = 0.0f;
  if (clapDelay < 0.024f) burst = 1.0f;
  else if (clapDelay < 0.048f) burst = 0.8f;
  else if (clapDelay < 0.072f) burst = 0.6f;
  float noise = frand() * 0.7f + clapNoise * 0.3f;
  float tone = sinf(2.0f * 3.14159265f * 1100.0f * clapDelay);
  float out = (noise * 0.7f + tone * 0.3f) * clapTrans * burst;
  return out * clapEnv;
}

const Parameter& DrumSynthVoice::parameter(DrumParamId id) const {
  return params[static_cast<int>(id)];
}

void DrumSynthVoice::setParameter(DrumParamId id, float value) {
  params[static_cast<int>(id)].setValue(value);
}
