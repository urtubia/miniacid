#pragma once

#include <stddef.h>
#include <stdint.h>
#include <memory>
#include <vector>
#include <string>

#include "scene_storage.h"
#include "scenes.h"
#include "mini_tb303.h"
#include "mini_drumvoices.h"
#include "tube_distortion.h"

// ===================== Audio config =====================

static const int SAMPLE_RATE = 22050;        // Hz
static const int AUDIO_BUFFER_SAMPLES = 256; // per buffer, mono
static const int SEQ_STEPS = 16;             // 16-step sequencer
static const int NUM_303_VOICES = 2;
static const int NUM_DRUM_VOICES = DrumPatternSet::kVoices;

// ===================== Parameters =====================

class TempoDelay {
public:
  explicit TempoDelay(float sampleRate);

  void reset();
  void setSampleRate(float sr);
  void setBpm(float bpm);
  void setBeats(float beats);
  void setMix(float mix);
  void setFeedback(float fb);
  void setEnabled(bool on);
  bool isEnabled() const;

  float process(float input);

private:
  // for 2 voices at 22050 Hz, this is the max that the cardputer can handle.
  static const int kMaxDelaySeconds = 1;

  std::vector<float> buffer;
  int writeIndex;
  int delaySamples;
  float sampleRate;
  int maxDelaySamples;
  float beats;    // delay length in beats
  float mix;      // wet mix 0..1
  float feedback; // feedback 0..1
  bool enabled;
};

enum class MiniAcidParamId : uint8_t {
  MainVolume = 0,
  Count
};
class MiniAcid {
public:
  static constexpr int kMin303Note = 24; // C1
  static constexpr int kMax303Note = 71; // B4

  MiniAcid(float sampleRate, SceneStorage* sceneStorage);

  void init();
  void reset();
  void start();
  void stop();
  void setBpm(float bpm);
  float bpm() const;
  float sampleRate() const;
  bool isPlaying() const;
  int currentStep() const;
  int currentDrumPatternIndex() const;
  int current303PatternIndex(int voiceIndex = 0) const;
  int currentDrumBankIndex() const;
  int current303BankIndex(int voiceIndex = 0) const;
  bool is303Muted(int voiceIndex = 0) const;
  bool isKickMuted() const;
  bool isSnareMuted() const;
  bool isHatMuted() const;
  bool isOpenHatMuted() const;
  bool isMidTomMuted() const;
  bool isHighTomMuted() const;
  bool isRimMuted() const;
  bool isClapMuted() const;
  bool is303DelayEnabled(int voiceIndex = 0) const;
  bool is303DistortionEnabled(int voiceIndex = 0) const;
  const Parameter& parameter303(TB303ParamId id, int voiceIndex = 0) const;
  size_t copyLastAudio(int16_t *dst, size_t maxSamples) const;
  const int8_t* pattern303Steps(int voiceIndex = 0) const;
  const bool* pattern303AccentSteps(int voiceIndex = 0) const;
  const bool* pattern303SlideSteps(int voiceIndex = 0) const;
  const bool* patternKickSteps() const;
  const bool* patternSnareSteps() const;
  const bool* patternHatSteps() const;
  const bool* patternOpenHatSteps() const;
  const bool* patternMidTomSteps() const;
  const bool* patternHighTomSteps() const;
  const bool* patternRimSteps() const;
  const bool* patternClapSteps() const;
  const bool* patternDrumAccentSteps() const;
  const bool* patternKickAccentSteps() const;
  const bool* patternSnareAccentSteps() const;
  const bool* patternHatAccentSteps() const;
  const bool* patternOpenHatAccentSteps() const;
  const bool* patternMidTomAccentSteps() const;
  const bool* patternHighTomAccentSteps() const;
  const bool* patternRimAccentSteps() const;
  const bool* patternClapAccentSteps() const;
  bool songModeEnabled() const;
  void setSongMode(bool enabled);
  void toggleSongMode();
  int songLength() const;
  int currentSongPosition() const;
  int songPlayheadPosition() const;
  void setSongPosition(int position);
  void setSongPattern(int position, SongTrack track, int patternIndex);
  void clearSongPattern(int position, SongTrack track);
  int songPatternAt(int position, SongTrack track) const;
  const Song& song() const;
  int display303PatternIndex(int voiceIndex) const;
  int displayDrumPatternIndex() const;
  std::vector<std::string> getAvailableDrumEngines() const;
  void setDrumEngine(const std::string& engineName);
  std::string currentSceneName() const;
  std::vector<std::string> availableSceneNames() const;
  bool loadSceneByName(const std::string& name);
  bool saveSceneAs(const std::string& name);
  bool createNewSceneWithName(const std::string& name);

  void toggleMute303(int voiceIndex = 0);
  void toggleMuteKick();
  void toggleMuteSnare();
  void toggleMuteHat();
  void toggleMuteOpenHat();
  void toggleMuteMidTom();
  void toggleMuteHighTom();
  void toggleMuteRim();
  void toggleMuteClap();
  void toggleDelay303(int voiceIndex = 0);
  void toggleDistortion303(int voiceIndex = 0);
  void setDrumPatternIndex(int patternIndex);
  void shiftDrumPatternIndex(int delta);
  void setDrumBankIndex(int bankIndex);
  void adjust303Parameter(TB303ParamId id, int steps, int voiceIndex = 0);
  void set303Parameter(TB303ParamId id, float value, int voiceIndex = 0);
  void set303PatternIndex(int voiceIndex, int patternIndex);
  void shift303PatternIndex(int voiceIndex, int delta);
  void set303BankIndex(int voiceIndex, int bankIndex);
  void adjust303StepNote(int voiceIndex, int stepIndex, int semitoneDelta);
  void adjust303StepOctave(int voiceIndex, int stepIndex, int octaveDelta);
  void clear303StepNote(int voiceIndex, int stepIndex);
  void toggle303AccentStep(int voiceIndex, int stepIndex);
  void toggle303SlideStep(int voiceIndex, int stepIndex);
  void toggleDrumStep(int voiceIndex, int stepIndex);
  void toggleDrumAccentStep(int stepIndex);
  void setDrumAccentStep(int voiceIndex, int stepIndex, bool accent);

  void randomize303Pattern(int voiceIndex = 0);
  void randomizeDrumPattern();

  Parameter& miniParameter(MiniAcidParamId id);
  void setParameter(MiniAcidParamId id, float value);
  void adjustParameter(MiniAcidParamId id, int steps);

  void generateAudioBuffer(int16_t *buffer, size_t numSamples);

private:
  void updateSamplesPerStep();
  void advanceStep();
  float noteToFreq(int note);
  int clamp303Voice(int voiceIndex) const;
  int clamp303Step(int stepIndex) const;
  int clamp303Note(int note) const;
  const SynthPattern& synthPattern(int synthIndex) const;
  SynthPattern& editSynthPattern(int synthIndex);
  const DrumPattern& drumPattern(int drumVoiceIndex) const;
  DrumPattern& editDrumPattern(int drumVoiceIndex);
  int clampDrumVoice(int voiceIndex) const;
  void refreshSynthCaches(int synthIndex) const;
  void refreshDrumCache(int drumVoiceIndex) const;
  const SynthPattern& activeSynthPattern(int synthIndex) const;
  const DrumPattern& activeDrumPattern(int drumVoiceIndex) const;
  int songPatternIndexForTrack(SongTrack track) const;
  void applySongPositionSelection();
  void advanceSongPlayhead();
  int clampSongPosition(int position) const;

  TB303Voice voice303;
  TB303Voice voice3032;
  std::unique_ptr<DrumSynthVoice> drums;
  float sampleRateValue;
  std::string drumEngineName_;

  SceneManager sceneManager_;
  SceneStorage* sceneStorage_;
  mutable int8_t synthNotesCache_[NUM_303_VOICES][SEQ_STEPS];
  mutable bool synthAccentCache_[NUM_303_VOICES][SEQ_STEPS];
  mutable bool synthSlideCache_[NUM_303_VOICES][SEQ_STEPS];
  mutable bool drumHitCache_[NUM_DRUM_VOICES][SEQ_STEPS];
  mutable bool drumAccentCache_[NUM_DRUM_VOICES][SEQ_STEPS];
  mutable bool drumStepAccentCache_[SEQ_STEPS];

  volatile bool playing;
  volatile bool mute303;
  volatile bool mute303_2;
  volatile bool muteKick;
  volatile bool muteSnare;
  volatile bool muteHat;
  volatile bool muteOpenHat;
  volatile bool muteMidTom;
  volatile bool muteHighTom;
  volatile bool muteRim;
  volatile bool muteClap;
  volatile bool delay303Enabled;
  volatile bool delay3032Enabled;
  volatile bool distortion303Enabled;
  volatile bool distortion3032Enabled;
  volatile float bpmValue;
  volatile int currentStepIndex;
  unsigned long samplesIntoStep;
  float samplesPerStep;
  bool songMode_;
  int drumCycleIndex_;
  int songPlayheadPosition_;
  int patternModeDrumPatternIndex_;
  int patternModeDrumBankIndex_;
  int patternModeSynthPatternIndex_[NUM_303_VOICES];
  int patternModeSynthBankIndex_[NUM_303_VOICES];

  TempoDelay delay303;
  TempoDelay delay3032;
  TubeDistortion distortion303;
  TubeDistortion distortion3032;
  int16_t lastBuffer[AUDIO_BUFFER_SAMPLES];
  size_t lastBufferCount;

  void loadSceneFromStorage();
  void saveSceneToStorage();
  void applySceneStateFromManager();
  void syncSceneStateToManager();

  Parameter params[static_cast<int>(MiniAcidParamId::Count)];
};

class PatternGenerator {
public:
  static void generateRandom303Pattern(SynthPattern& pattern);
  static void generateRandomDrumPattern(DrumPatternSet& patternSet);
};

inline Parameter& MiniAcid::miniParameter(MiniAcidParamId id) {
  return params[static_cast<int>(id)];
}
