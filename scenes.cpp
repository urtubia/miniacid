#include "ArduinoJson-v7.4.2.h"
#include "scenes.h"

namespace {
int clampIndex(int value, int maxExclusive) {
  if (value < 0) return 0;
  if (value >= maxExclusive) return maxExclusive - 1;
  return value;
}

void clearDrumPattern(DrumPattern& pattern) {
  for (int i = 0; i < DrumPattern::kSteps; ++i) {
    pattern.steps[i].hit = false;
    pattern.steps[i].accent = false;
  }
}

void clearSynthPattern(SynthPattern& pattern) {
  for (int i = 0; i < SynthPattern::kSteps; ++i) {
    pattern.steps[i].note = -1;
    pattern.steps[i].slide = false;
    pattern.steps[i].accent = false;
  }
}

void clearSong(Song& song) {
  song.length = 1;
  for (int i = 0; i < Song::kMaxPositions; ++i) {
    for (int t = 0; t < SongPosition::kTrackCount; ++t) {
      song.positions[i].patterns[t] = -1;
    }
  }
}

void clearSceneData(Scene& scene) {
  for (int p = 0; p < Bank<DrumPatternSet>::kPatterns; ++p) {
    for (int v = 0; v < DrumPatternSet::kVoices; ++v) {
      clearDrumPattern(scene.drumBank.patterns[p].voices[v]);
    }
  }
  for (int p = 0; p < Bank<SynthPattern>::kPatterns; ++p) {
    clearSynthPattern(scene.synthABank.patterns[p]);
    clearSynthPattern(scene.synthBBank.patterns[p]);
  }
  clearSong(scene.song);
}

void serializeDrumPattern(const DrumPattern& pattern, ArduinoJson::JsonObject obj) {
  ArduinoJson::JsonArray hit = obj["hit"].to<ArduinoJson::JsonArray>();
  ArduinoJson::JsonArray accent = obj["accent"].to<ArduinoJson::JsonArray>();
  for (int i = 0; i < DrumPattern::kSteps; ++i) {
    hit.add(pattern.steps[i].hit);
    accent.add(pattern.steps[i].accent);
  }
}

void serializeDrumBank(const Bank<DrumPatternSet>& bank, ArduinoJson::JsonArray patterns) {
  for (int p = 0; p < Bank<DrumPatternSet>::kPatterns; ++p) {
    ArduinoJson::JsonArray voices = patterns.add<ArduinoJson::JsonArray>();
    for (int v = 0; v < DrumPatternSet::kVoices; ++v) {
      ArduinoJson::JsonObject voice = voices.add<ArduinoJson::JsonObject>();
      serializeDrumPattern(bank.patterns[p].voices[v], voice);
    }
  }
}

void serializeSynthPattern(const SynthPattern& pattern, ArduinoJson::JsonArray steps) {
  for (int i = 0; i < SynthPattern::kSteps; ++i) {
    ArduinoJson::JsonObject step = steps.add<ArduinoJson::JsonObject>();
    step["note"] = pattern.steps[i].note;
    step["slide"] = pattern.steps[i].slide;
    step["accent"] = pattern.steps[i].accent;
  }
}

void serializeSynthBank(const Bank<SynthPattern>& bank, ArduinoJson::JsonArray patterns) {
  for (int p = 0; p < Bank<SynthPattern>::kPatterns; ++p) {
    ArduinoJson::JsonArray steps = patterns.add<ArduinoJson::JsonArray>();
    serializeSynthPattern(bank.patterns[p], steps);
  }
}

bool deserializeBoolArray(ArduinoJson::JsonArrayConst arr, bool* dst, int expectedSize) {
  if (static_cast<int>(arr.size()) != expectedSize) return false;
  int idx = 0;
  for (ArduinoJson::JsonVariantConst value : arr) {
    if (!value.is<bool>()) return false;
    dst[idx++] = value.as<bool>();
  }
  return true;
}

bool deserializeDrumPattern(ArduinoJson::JsonVariantConst value, DrumPattern& pattern) {
  ArduinoJson::JsonObjectConst obj = value.as<ArduinoJson::JsonObjectConst>();
  if (obj.isNull()) return false;
  ArduinoJson::JsonArrayConst hit = obj["hit"].as<ArduinoJson::JsonArrayConst>();
  ArduinoJson::JsonArrayConst accent = obj["accent"].as<ArduinoJson::JsonArrayConst>();
  if (hit.isNull() || accent.isNull()) return false;
  bool hits[DrumPattern::kSteps];
  bool accents[DrumPattern::kSteps];
  if (!deserializeBoolArray(hit, hits, DrumPattern::kSteps)) return false;
  if (!deserializeBoolArray(accent, accents, DrumPattern::kSteps)) return false;
  for (int i = 0; i < DrumPattern::kSteps; ++i) {
    pattern.steps[i].hit = hits[i];
    pattern.steps[i].accent = accents[i];
  }
  return true;
}

bool deserializeDrumPatternSet(ArduinoJson::JsonVariantConst value, DrumPatternSet& patternSet) {
  ArduinoJson::JsonArrayConst voices = value.as<ArduinoJson::JsonArrayConst>();
  if (voices.isNull() || static_cast<int>(voices.size()) != DrumPatternSet::kVoices) return false;
  int v = 0;
  for (ArduinoJson::JsonVariantConst voice : voices) {
    if (!deserializeDrumPattern(voice, patternSet.voices[v])) return false;
    ++v;
  }
  return true;
}

bool deserializeDrumBank(ArduinoJson::JsonVariantConst value, Bank<DrumPatternSet>& bank) {
  ArduinoJson::JsonArrayConst patterns = value.as<ArduinoJson::JsonArrayConst>();
  if (patterns.isNull() || static_cast<int>(patterns.size()) != Bank<DrumPatternSet>::kPatterns) return false;
  int p = 0;
  for (ArduinoJson::JsonVariantConst pattern : patterns) {
    if (!deserializeDrumPatternSet(pattern, bank.patterns[p])) return false;
    ++p;
  }
  return true;
}

bool deserializeSynthPattern(ArduinoJson::JsonVariantConst value, SynthPattern& pattern) {
  ArduinoJson::JsonArrayConst steps = value.as<ArduinoJson::JsonArrayConst>();
  if (steps.isNull() || static_cast<int>(steps.size()) != SynthPattern::kSteps) return false;
  int i = 0;
  for (ArduinoJson::JsonVariantConst stepValue : steps) {
    ArduinoJson::JsonObjectConst obj = stepValue.as<ArduinoJson::JsonObjectConst>();
    if (obj.isNull()) return false;
    auto note = obj["note"];
    auto slide = obj["slide"];
    auto accent = obj["accent"];
    if (!note.is<int>() || !slide.is<bool>() || !accent.is<bool>()) return false;
    pattern.steps[i].note = note.as<int>();
    pattern.steps[i].slide = slide.as<bool>();
    pattern.steps[i].accent = accent.as<bool>();
    ++i;
  }
  return true;
}

bool deserializeSynthBank(ArduinoJson::JsonVariantConst value, Bank<SynthPattern>& bank) {
  ArduinoJson::JsonArrayConst patterns = value.as<ArduinoJson::JsonArrayConst>();
  if (patterns.isNull() || static_cast<int>(patterns.size()) != Bank<SynthPattern>::kPatterns) return false;
  int p = 0;
  for (ArduinoJson::JsonVariantConst pattern : patterns) {
    if (!deserializeSynthPattern(pattern, bank.patterns[p])) return false;
    ++p;
  }
  return true;
}

int valueToInt(ArduinoJson::JsonVariantConst value, int defaultValue) {
  if (value.is<int>()) {
    return value.as<int>();
  }
  return defaultValue;
}

float valueToFloat(ArduinoJson::JsonVariantConst value, float defaultValue) {
  if (value.is<float>() || value.is<int>()) {
    return value.as<float>();
  }
  return defaultValue;
}

bool deserializeSynthParameters(ArduinoJson::JsonVariantConst value, SynthParameters& params) {
  ArduinoJson::JsonObjectConst obj = value.as<ArduinoJson::JsonObjectConst>();
  if (obj.isNull()) return false;

  auto cutoff = obj["cutoff"];
  auto resonance = obj["resonance"];
  auto envAmount = obj["envAmount"];
  auto envDecay = obj["envDecay"];
  auto oscType = obj["oscType"];

  if (!cutoff.isNull()) {
    if (!cutoff.is<float>() && !cutoff.is<int>()) return false;
    params.cutoff = valueToFloat(cutoff, params.cutoff);
  }
  if (!resonance.isNull()) {
    if (!resonance.is<float>() && !resonance.is<int>()) return false;
    params.resonance = valueToFloat(resonance, params.resonance);
  }
  if (!envAmount.isNull()) {
    if (!envAmount.is<float>() && !envAmount.is<int>()) return false;
    params.envAmount = valueToFloat(envAmount, params.envAmount);
  }
  if (!envDecay.isNull()) {
    if (!envDecay.is<float>() && !envDecay.is<int>()) return false;
    params.envDecay = valueToFloat(envDecay, params.envDecay);
  }
  if (!oscType.isNull()) {
    if (!oscType.is<int>()) return false;
    params.oscType = oscType.as<int>();
  }

  return true;
}
}

SceneJsonObserver::SceneJsonObserver(Scene& scene, float defaultBpm)
    : target_(scene), bpm_(defaultBpm) {
  clearSong(song_);
}

SceneJsonObserver::Path SceneJsonObserver::deduceArrayPath(const Context& parent) const {
  switch (parent.path) {
  case Path::DrumBank:
    return Path::DrumPatternSet;
  case Path::SynthABank:
    return Path::SynthPattern;
  case Path::SynthBBank:
    return Path::SynthPattern;
  case Path::SynthParams:
    return Path::SynthParam;
  case Path::Song:
    return Path::SongPosition;
  default:
    return Path::Unknown;
  }
}

SceneJsonObserver::Path SceneJsonObserver::deduceObjectPath(const Context& parent) const {
  switch (parent.path) {
  case Path::DrumPatternSet:
    return Path::DrumVoice;
  case Path::SynthPattern:
    return Path::SynthStep;
  case Path::SynthParams:
    return Path::SynthParam;
  case Path::SongPositions:
    return Path::SongPosition;
  default:
    return Path::Unknown;
  }
}

int SceneJsonObserver::currentIndexFor(Path path) const {
  for (int i = stackSize_ - 1; i >= 0; --i) {
    if (stack_[i].path == path && stack_[i].type == Context::Type::Array) {
      return stack_[i].index;
    }
  }
  return -1;
}

bool SceneJsonObserver::inSynthBankA() const {
  for (int i = stackSize_ - 1; i >= 0; --i) {
    if (stack_[i].path == Path::SynthABank) return true;
    if (stack_[i].path == Path::SynthBBank) return false;
  }
  return true;
}

bool SceneJsonObserver::inSynthBankB() const {
  for (int i = stackSize_ - 1; i >= 0; --i) {
    if (stack_[i].path == Path::SynthBBank) return true;
    if (stack_[i].path == Path::SynthABank) return false;
  }
  return false;
}

void SceneJsonObserver::pushContext(Context::Type type, Path path) {
  if (stackSize_ >= kMaxStack) {
    error_ = true;
    return;
  }
  stack_[stackSize_++] = Context{type, path, 0};
}

void SceneJsonObserver::popContext() {
  if (stackSize_ == 0) {
    error_ = true;
    return;
  }
  --stackSize_;
}

void SceneJsonObserver::onObjectStart() {
  if (error_) return;
  Path path = Path::Unknown;
  if (stackSize_ == 0) {
    path = Path::Root;
  } else {
    const Context& parent = stack_[stackSize_ - 1];
    if (parent.type == Context::Type::Array) {
      path = deduceObjectPath(parent);
    } else if (parent.path == Path::Root && lastKey_ == "state") {
      path = Path::State;
    } else if (parent.path == Path::Root && lastKey_ == "song") {
      path = Path::Song;
    } else if (parent.path == Path::State && lastKey_ == "mute") {
      path = Path::Mute;
    }
  }
  pushContext(Context::Type::Object, path);
  if (path == Path::Unknown) error_ = true;
}

void SceneJsonObserver::onObjectEnd() {
  if (error_) return;
  popContext();
}

void SceneJsonObserver::onArrayStart() {
  if (error_) return;
  Path path = Path::Unknown;
  if (stackSize_ > 0) {
    const Context& parent = stack_[stackSize_ - 1];
    if (parent.type == Context::Type::Object) {
      if (parent.path == Path::Root) {
        if (lastKey_ == "drumBank") path = Path::DrumBank;
        else if (lastKey_ == "synthABank") path = Path::SynthABank;
        else if (lastKey_ == "synthBBank") path = Path::SynthBBank;
      } else if (parent.path == Path::Song) {
        if (lastKey_ == "positions") path = Path::SongPositions;
      } else if (parent.path == Path::DrumVoice) {
        if (lastKey_ == "hit") path = Path::DrumHitArray;
        else if (lastKey_ == "accent") path = Path::DrumAccentArray;
      } else if (parent.path == Path::State) {
        if (lastKey_ == "synthPatternIndex") path = Path::SynthPatternIndex;
        else if (lastKey_ == "synthBankIndex") path = Path::SynthBankIndex;
        else if (lastKey_ == "synthParams") path = Path::SynthParams;
      } else if (parent.path == Path::Mute) {
        if (lastKey_ == "drums") path = Path::MuteDrums;
        else if (lastKey_ == "synth") path = Path::MuteSynth;
      }
    } else if (parent.type == Context::Type::Array) {
      path = deduceArrayPath(parent);
    }
  }
  pushContext(Context::Type::Array, path);
  if (path == Path::Unknown) error_ = true;
}

void SceneJsonObserver::onArrayEnd() {
  if (error_) return;
  popContext();
}

void SceneJsonObserver::handlePrimitiveNumber(double value, bool isInteger) {
  (void)isInteger;
  if (error_ || stackSize_ == 0) return;
  Path path = stack_[stackSize_ - 1].path;
  if (path == Path::Song) {
    if (lastKey_ == "length") {
      int len = static_cast<int>(value);
      if (len < 1) len = 1;
      if (len > Song::kMaxPositions) len = Song::kMaxPositions;
      song_.length = len;
      hasSong_ = true;
    }
    return;
  }
  if (path == Path::SongPosition) {
    int posIdx = currentIndexFor(Path::SongPositions);
    if (posIdx < 0 || posIdx >= Song::kMaxPositions) {
      error_ = true;
      return;
    }
    int trackIdx = -1;
    if (lastKey_ == "a") trackIdx = 0;
    else if (lastKey_ == "b") trackIdx = 1;
    else if (lastKey_ == "drums") trackIdx = 2;
    if (trackIdx >= 0 && trackIdx < SongPosition::kTrackCount) {
      song_.positions[posIdx].patterns[trackIdx] = static_cast<int>(value);
      if (posIdx + 1 > song_.length) song_.length = posIdx + 1;
      hasSong_ = true;
    }
    return;
  }
  if (path == Path::DrumHitArray || path == Path::DrumAccentArray ||
      path == Path::MuteDrums || path == Path::MuteSynth) {
    handlePrimitiveBool(value != 0);
    return;
  }
  if (path == Path::SynthPatternIndex) {
    int idx = stack_[stackSize_ - 1].index;
    if (idx >= 0 && idx < 2) synthPatternIndex_[idx] = static_cast<int>(value);
    return;
  }
  if (path == Path::SynthBankIndex) {
    int idx = stack_[stackSize_ - 1].index;
    if (idx >= 0 && idx < 2) synthBankIndex_[idx] = static_cast<int>(value);
    return;
  }
  if (path == Path::SynthStep) {
    int stepIdx = currentIndexFor(Path::SynthPattern);
    bool useBankB = inSynthBankB();
    int patternIdx = currentIndexFor(useBankB ? Path::SynthBBank : Path::SynthABank);
    if (stepIdx < 0 || stepIdx >= SynthPattern::kSteps ||
        patternIdx < 0 || patternIdx >= Bank<SynthPattern>::kPatterns) {
      error_ = true;
      return;
    }
    SynthPattern& pattern = useBankB ? target_.synthBBank.patterns[patternIdx]
                                     : target_.synthABank.patterns[patternIdx];
    if (lastKey_ == "note") {
      pattern.steps[stepIdx].note = static_cast<int>(value);
    } else if (lastKey_ == "slide") {
      pattern.steps[stepIdx].slide = value != 0;
    } else if (lastKey_ == "accent") {
      pattern.steps[stepIdx].accent = value != 0;
    }
    return;
  }
  if (path == Path::SynthParam) {
    int synthIdx = currentIndexFor(Path::SynthParams);
    if (synthIdx < 0 || synthIdx >= 2) {
      error_ = true;
      return;
    }
    float fval = static_cast<float>(value);
    if (lastKey_ == "cutoff") {
      synthParameters_[synthIdx].cutoff = fval;
    } else if (lastKey_ == "resonance") {
      synthParameters_[synthIdx].resonance = fval;
    } else if (lastKey_ == "envAmount") {
      synthParameters_[synthIdx].envAmount = fval;
    } else if (lastKey_ == "envDecay") {
      synthParameters_[synthIdx].envDecay = fval;
    } else if (lastKey_ == "oscType") {
      synthParameters_[synthIdx].oscType = static_cast<int>(value);
    }
    return;
  }
  if (path == Path::State) {
    if (lastKey_ == "bpm") {
      bpm_ = static_cast<float>(value);
      return;
    }
    if (lastKey_ == "songPosition") {
      songPosition_ = static_cast<int>(value);
      return;
    }
    if (lastKey_ == "songMode") {
      songMode_ = value != 0;
      return;
    }
    int intValue = static_cast<int>(value);
    if (lastKey_ == "drumPatternIndex") {
      drumPatternIndex_ = intValue;
    } else if (lastKey_ == "drumBankIndex") {
      drumBankIndex_ = intValue;
    } else if (lastKey_ == "synthPatternIndex") {
      synthPatternIndex_[0] = intValue;
    } else if (lastKey_ == "synthBankIndex") {
      synthBankIndex_[0] = intValue;
    }
  }
}

void SceneJsonObserver::handlePrimitiveBool(bool value) {
  if (error_ || stackSize_ == 0) return;
  Path path = stack_[stackSize_ - 1].path;
  if (path == Path::DrumHitArray || path == Path::DrumAccentArray) {
    int patternIdx = currentIndexFor(Path::DrumBank);
    int voiceIdx = currentIndexFor(Path::DrumPatternSet);
    int stepIdx = stack_[stackSize_ - 1].index;
    if (patternIdx < 0 || patternIdx >= Bank<DrumPatternSet>::kPatterns ||
        voiceIdx < 0 || voiceIdx >= DrumPatternSet::kVoices ||
        stepIdx < 0 || stepIdx >= DrumPattern::kSteps) {
      error_ = true;
      return;
    }
    DrumStep& step = target_.drumBank.patterns[patternIdx].voices[voiceIdx].steps[stepIdx];
    if (path == Path::DrumHitArray) {
      step.hit = value;
    } else {
      step.accent = value;
    }
    return;
  }

  if (path == Path::MuteDrums) {
    int muteIdx = stack_[stackSize_ - 1].index;
    if (muteIdx < 0 || muteIdx >= DrumPatternSet::kVoices) {
      error_ = true;
      return;
    }
    drumMute_[muteIdx] = value;
    return;
  }

  if (path == Path::MuteSynth) {
    int muteIdx = stack_[stackSize_ - 1].index;
    if (muteIdx < 0 || muteIdx >= 2) {
      error_ = true;
      return;
    }
    synthMute_[muteIdx] = value;
    return;
  }

  if (path == Path::SynthStep) {
    int stepIdx = currentIndexFor(Path::SynthPattern);
    bool useBankB = inSynthBankB();
    int patternIdx = currentIndexFor(useBankB ? Path::SynthBBank : Path::SynthABank);
    if (patternIdx < 0 || patternIdx >= Bank<SynthPattern>::kPatterns ||
        stepIdx < 0 || stepIdx >= SynthPattern::kSteps) {
      error_ = true;
      return;
    }
    SynthPattern& pattern = useBankB ? target_.synthBBank.patterns[patternIdx]
                                     : target_.synthABank.patterns[patternIdx];
    if (lastKey_ == "slide") {
      pattern.steps[stepIdx].slide = value;
    } else if (lastKey_ == "accent") {
      pattern.steps[stepIdx].accent = value;
    }
    return;
  }

  if (path == Path::State && lastKey_ == "songMode") {
    songMode_ = value;
  }
}

void SceneJsonObserver::onNumber(int value) { handlePrimitiveNumber(static_cast<double>(value), true); }

void SceneJsonObserver::onNumber(double value) { handlePrimitiveNumber(value, false); }

void SceneJsonObserver::onBool(bool value) { handlePrimitiveBool(value); }

void SceneJsonObserver::onNull() {}

void SceneJsonObserver::onString(const std::string& value) {
  (void)value;
}

void SceneJsonObserver::onObjectKey(const std::string& key) { lastKey_ = key; }

void SceneJsonObserver::onObjectValueStart() {}

void SceneJsonObserver::onObjectValueEnd() {
  if (error_) return;
  if (stackSize_ > 0 && stack_[stackSize_ - 1].type == Context::Type::Array) {
    ++stack_[stackSize_ - 1].index;
  }
}

bool SceneJsonObserver::hadError() const { return error_; }

int SceneJsonObserver::drumPatternIndex() const { return drumPatternIndex_; }

int SceneJsonObserver::synthPatternIndex(int synthIdx) const {
  int idx = synthIdx < 0 ? 0 : synthIdx > 1 ? 1 : synthIdx;
  return synthPatternIndex_[idx];
}

int SceneJsonObserver::drumBankIndex() const { return drumBankIndex_; }

int SceneJsonObserver::synthBankIndex(int synthIdx) const {
  int idx = synthIdx < 0 ? 0 : synthIdx > 1 ? 1 : synthIdx;
  return synthBankIndex_[idx];
}

bool SceneJsonObserver::drumMute(int idx) const {
  if (idx < 0) return drumMute_[0];
  if (idx >= DrumPatternSet::kVoices) return drumMute_[DrumPatternSet::kVoices - 1];
  return drumMute_[idx];
}

bool SceneJsonObserver::synthMute(int idx) const {
  int clamped = idx < 0 ? 0 : idx > 1 ? 1 : idx;
  return synthMute_[clamped];
}

const SynthParameters& SceneJsonObserver::synthParameters(int synthIdx) const {
  int clamped = synthIdx < 0 ? 0 : synthIdx > 1 ? 1 : synthIdx;
  return synthParameters_[clamped];
}

float SceneJsonObserver::bpm() const { return bpm_; }

const Song& SceneJsonObserver::song() const { return song_; }

bool SceneJsonObserver::hasSong() const { return hasSong_; }

bool SceneJsonObserver::songMode() const { return songMode_; }

int SceneJsonObserver::songPosition() const { return songPosition_; }

void SceneManager::loadDefaultScene() {
  drumPatternIndex_ = 0;
  drumBankIndex_ = 0;
  synthPatternIndex_[0] = 0;
  synthPatternIndex_[1] = 0;
  synthBankIndex_[0] = 0;
  synthBankIndex_[1] = 0;
  for (int i = 0; i < DrumPatternSet::kVoices; ++i) drumMute_[i] = false;
  synthMute_[0] = false;
  synthMute_[1] = false;
  synthParameters_[0] = SynthParameters();
  synthParameters_[1] = SynthParameters();
  setBpm(100.0f);
  songMode_ = false;
  songPosition_ = 0;
  clearSongData(scene_.song);
  scene_.song.length = 1;
  scene_.song.positions[0].patterns[0] = 0;
  scene_.song.positions[0].patterns[1] = 0;
  scene_.song.positions[0].patterns[2] = 0;

  for (int i = 0; i < Bank<DrumPatternSet>::kPatterns; ++i) {
    for (int v = 0; v < DrumPatternSet::kVoices; ++v) {
      clearDrumPattern(scene_.drumBank.patterns[i].voices[v]);
    }
  }
  for (int i = 0; i < Bank<SynthPattern>::kPatterns; ++i) {
    clearSynthPattern(scene_.synthABank.patterns[i]);
    clearSynthPattern(scene_.synthBBank.patterns[i]);
  }

  int8_t notes[SynthPattern::kSteps] = {48, 48, 55, 55, 50, 50, 55, 55,
                                        48, 48, 55, 55, 50, 55, 50, -1};
  bool accent[SynthPattern::kSteps] = {false, true, false, true, false, true,
                                       false, true, false, true, false, true,
                                       false, true, false, false};
  bool slide[SynthPattern::kSteps] = {false, true, false, true, false, true,
                                      false, true, false, true, false, true,
                                      false, true, false, false};

  int8_t notes2[SynthPattern::kSteps] = {48, 48, 55, 55, 50, 50, 55, 55,
                                         48, 48, 55, 55, 50, 55, 50, -1};
  bool accent2[SynthPattern::kSteps] = {true,  false, true,  false, true,  false,
                                        true,  false, true,  false, true,  false,
                                        true,  false, true,  false};
  bool slide2[SynthPattern::kSteps] = {false, false, true,  false, false, false,
                                       true,  false, false, false, true,  false,
                                       false, false, true,  false};

  bool kick[DrumPattern::kSteps] = {true,  false, false, false, true,  false,
                                    false, false, true,  false, false, false,
                                    true,  false, false, false};

  bool snare[DrumPattern::kSteps] = {false, false, true,  false, false, false,
                                     true,  false, false, false, true,  false,
                                     false, false, true,  false};

  bool hat[DrumPattern::kSteps] = {true, true, true, true, true, true, true, true,
                                   true, true, true, true, true, true, true, true};

  bool openHat[DrumPattern::kSteps] = {false, false, false, true,  false, false, false, false,
                                       false, false, false, true,  false, false, false, false};

  bool midTom[DrumPattern::kSteps] = {false, false, false, false, true,  false, false, false,
                                      false, false, false, false, true,  false, false, false};

  bool highTom[DrumPattern::kSteps] = {false, false, false, false, false, false, true,  false,
                                       false, false, false, false, false, false, true,  false};

  bool rim[DrumPattern::kSteps] = {false, false, false, false, false, true,  false, false,
                                   false, false, false, false, false, true,  false, false};

  bool clap[DrumPattern::kSteps] = {false, false, false, false, false, false, false, false,
                                    false, false, false, false, true,  false, false, false};

  for (int i = 0; i < SynthPattern::kSteps; ++i) {
    scene_.synthABank.patterns[0].steps[i].note = notes[i];
    scene_.synthABank.patterns[0].steps[i].accent = accent[i];
    scene_.synthABank.patterns[0].steps[i].slide = slide[i];

    scene_.synthBBank.patterns[0].steps[i].note = notes2[i];
    scene_.synthBBank.patterns[0].steps[i].accent = accent2[i];
    scene_.synthBBank.patterns[0].steps[i].slide = slide2[i];
  }

  for (int i = 0; i < DrumPattern::kSteps; ++i) {
    bool hatVal = hat[i];
    if (openHat[i]) {
      hatVal = false;
    }
    scene_.drumBank.patterns[0].voices[0].steps[i].hit = kick[i];
    scene_.drumBank.patterns[0].voices[0].steps[i].accent = kick[i];

    scene_.drumBank.patterns[0].voices[1].steps[i].hit = snare[i];
    scene_.drumBank.patterns[0].voices[1].steps[i].accent = snare[i];

    scene_.drumBank.patterns[0].voices[2].steps[i].hit = hatVal;
    scene_.drumBank.patterns[0].voices[2].steps[i].accent = hatVal;

    scene_.drumBank.patterns[0].voices[3].steps[i].hit = openHat[i];
    scene_.drumBank.patterns[0].voices[3].steps[i].accent = openHat[i];

    scene_.drumBank.patterns[0].voices[4].steps[i].hit = midTom[i];
    scene_.drumBank.patterns[0].voices[4].steps[i].accent = midTom[i];

    scene_.drumBank.patterns[0].voices[5].steps[i].hit = highTom[i];
    scene_.drumBank.patterns[0].voices[5].steps[i].accent = highTom[i];

    scene_.drumBank.patterns[0].voices[6].steps[i].hit = rim[i];
    scene_.drumBank.patterns[0].voices[6].steps[i].accent = rim[i];

    scene_.drumBank.patterns[0].voices[7].steps[i].hit = clap[i];
    scene_.drumBank.patterns[0].voices[7].steps[i].accent = clap[i];
  }
}

Scene& SceneManager::currentScene() { return scene_; }

const Scene& SceneManager::currentScene() const { return scene_; }

const DrumPatternSet& SceneManager::getCurrentDrumPattern() const {
  return scene_.drumBank.patterns[clampPatternIndex(drumPatternIndex_)];
}

DrumPatternSet& SceneManager::editCurrentDrumPattern() {
  return scene_.drumBank.patterns[clampPatternIndex(drumPatternIndex_)];
}

const SynthPattern& SceneManager::getCurrentSynthPattern(int synthIndex) const {
  int idx = clampSynthIndex(synthIndex);
  int patternIndex = clampPatternIndex(synthPatternIndex_[idx]);
  if (idx == 0) {
    return scene_.synthABank.patterns[patternIndex];
  }
  return scene_.synthBBank.patterns[patternIndex];
}

SynthPattern& SceneManager::editCurrentSynthPattern(int synthIndex) {
  int idx = clampSynthIndex(synthIndex);
  int patternIndex = clampPatternIndex(synthPatternIndex_[idx]);
  if (idx == 0) {
    return scene_.synthABank.patterns[patternIndex];
  }
  return scene_.synthBBank.patterns[patternIndex];
}

const SynthPattern& SceneManager::getSynthPattern(int synthIndex, int patternIndex) const {
  int idx = clampSynthIndex(synthIndex);
  int pat = clampPatternIndex(patternIndex);
  if (idx == 0) {
    return scene_.synthABank.patterns[pat];
  }
  return scene_.synthBBank.patterns[pat];
}

SynthPattern& SceneManager::editSynthPattern(int synthIndex, int patternIndex) {
  int idx = clampSynthIndex(synthIndex);
  int pat = clampPatternIndex(patternIndex);
  if (idx == 0) {
    return scene_.synthABank.patterns[pat];
  }
  return scene_.synthBBank.patterns[pat];
}

const DrumPatternSet& SceneManager::getDrumPatternSet(int patternIndex) const {
  int pat = clampPatternIndex(patternIndex);
  return scene_.drumBank.patterns[pat];
}

DrumPatternSet& SceneManager::editDrumPatternSet(int patternIndex) {
  int pat = clampPatternIndex(patternIndex);
  return scene_.drumBank.patterns[pat];
}

void SceneManager::setCurrentDrumPatternIndex(int idx) {
  drumPatternIndex_ = clampPatternIndex(idx);
}

void SceneManager::setCurrentSynthPatternIndex(int synthIdx, int idx) {
  int clampedSynth = clampSynthIndex(synthIdx);
  synthPatternIndex_[clampedSynth] = clampPatternIndex(idx);
}

int SceneManager::getCurrentDrumPatternIndex() const { return drumPatternIndex_; }

int SceneManager::getCurrentSynthPatternIndex(int synthIdx) const {
  int clampedSynth = clampSynthIndex(synthIdx);
  return synthPatternIndex_[clampedSynth];
}

void SceneManager::setDrumMute(int voiceIdx, bool mute) {
  int clampedVoice = clampIndex(voiceIdx, DrumPatternSet::kVoices);
  drumMute_[clampedVoice] = mute;
}

bool SceneManager::getDrumMute(int voiceIdx) const {
  int clampedVoice = clampIndex(voiceIdx, DrumPatternSet::kVoices);
  return drumMute_[clampedVoice];
}

void SceneManager::setSynthMute(int synthIdx, bool mute) {
  int clampedSynth = clampSynthIndex(synthIdx);
  synthMute_[clampedSynth] = mute;
}

bool SceneManager::getSynthMute(int synthIdx) const {
  int clampedSynth = clampSynthIndex(synthIdx);
  return synthMute_[clampedSynth];
}

void SceneManager::setSynthParameters(int synthIdx, const SynthParameters& params) {
  int clampedSynth = clampSynthIndex(synthIdx);
  synthParameters_[clampedSynth] = params;
}

const SynthParameters& SceneManager::getSynthParameters(int synthIdx) const {
  int clampedSynth = clampSynthIndex(synthIdx);
  return synthParameters_[clampedSynth];
}

void SceneManager::setBpm(float bpm) {
  if (bpm < 40.0f) bpm = 40.0f;
  if (bpm > 200.0f) bpm = 200.0f;
  bpm_ = bpm;
}

float SceneManager::getBpm() const { return bpm_; }

const Song& SceneManager::song() const { return scene_.song; }

Song& SceneManager::editSong() { return scene_.song; }

void SceneManager::setSongPattern(int position, SongTrack track, int patternIndex) {
  int pos = position;
  if (pos < 0) pos = 0;
  if (pos >= Song::kMaxPositions) pos = Song::kMaxPositions - 1;
  int trackIdx = songTrackToIndex(track);
  if (trackIdx < 0 || trackIdx >= SongPosition::kTrackCount) return;
  int pat = patternIndex;
  if (pat < -1) pat = -1;
  if (pat >= Bank<SynthPattern>::kPatterns) pat = Bank<SynthPattern>::kPatterns - 1;
  if (pos >= scene_.song.length) setSongLength(pos + 1);
  scene_.song.positions[pos].patterns[trackIdx] = static_cast<int8_t>(pat);
}

void SceneManager::clearSongPattern(int position, SongTrack track) {
  int pos = clampSongPosition(position);
  int trackIdx = songTrackToIndex(track);
  if (trackIdx < 0 || trackIdx >= SongPosition::kTrackCount) return;
  scene_.song.positions[pos].patterns[trackIdx] = -1;
  trimSongLength();
}

int SceneManager::songPattern(int position, SongTrack track) const {
  if (position < 0 || position >= Song::kMaxPositions) return -1;
  int trackIdx = songTrackToIndex(track);
  if (trackIdx < 0 || trackIdx >= SongPosition::kTrackCount) return -1;
  if (position >= scene_.song.length) return -1;
  return scene_.song.positions[position].patterns[trackIdx];
}

void SceneManager::setSongLength(int length) {
  int clamped = clampSongLength(length);
  scene_.song.length = clamped;
  if (songPosition_ >= scene_.song.length) songPosition_ = scene_.song.length - 1;
  if (songPosition_ < 0) songPosition_ = 0;
}

int SceneManager::songLength() const {
  int len = scene_.song.length;
  if (len < 1) len = 1;
  if (len > Song::kMaxPositions) len = Song::kMaxPositions;
  return len;
}

void SceneManager::setSongPosition(int position) {
  songPosition_ = clampSongPosition(position);
}

int SceneManager::getSongPosition() const {
  return clampSongPosition(songPosition_);
}

void SceneManager::setSongMode(bool enabled) { songMode_ = enabled; }

bool SceneManager::songMode() const { return songMode_; }

void SceneManager::setCurrentBankIndex(int instrumentId, int bankIdx) {
  int clampedBank = 0; // Only a single bank exists today; retain parameter for future use
  if (instrumentId == 0) {
    drumBankIndex_ = clampedBank;
  } else {
    int synthIdx = clampSynthIndex(instrumentId - 1);
    synthBankIndex_[synthIdx] = clampedBank;
  }
}

int SceneManager::getCurrentBankIndex(int instrumentId) const {
  (void)instrumentId;
  return 0;
}

void SceneManager::setDrumStep(int voiceIdx, int step, bool hit, bool accent) {
  DrumPatternSet& patternSet = editCurrentDrumPattern();
  int clampedVoice = clampIndex(voiceIdx, DrumPatternSet::kVoices);
  int clampedStep = clampIndex(step, DrumPattern::kSteps);
  patternSet.voices[clampedVoice].steps[clampedStep].hit = hit;
  patternSet.voices[clampedVoice].steps[clampedStep].accent = accent;
}

void SceneManager::setSynthStep(int synthIdx, int step, int note, bool slide, bool accent) {
  SynthPattern& pattern = editCurrentSynthPattern(synthIdx);
  int clampedStep = clampIndex(step, SynthPattern::kSteps);
  pattern.steps[clampedStep].note = note;
  pattern.steps[clampedStep].slide = slide;
  pattern.steps[clampedStep].accent = accent;
}

void SceneManager::buildSceneDocument(ArduinoJson::JsonDocument& doc) const {
  doc.clear();
  ArduinoJson::JsonObject root = doc.to<ArduinoJson::JsonObject>();

  ArduinoJson::JsonArray drumBank = root["drumBank"].to<ArduinoJson::JsonArray>();
  serializeDrumBank(scene_.drumBank, drumBank);
  ArduinoJson::JsonArray synthABank = root["synthABank"].to<ArduinoJson::JsonArray>();
  serializeSynthBank(scene_.synthABank, synthABank);
  ArduinoJson::JsonArray synthBBank = root["synthBBank"].to<ArduinoJson::JsonArray>();
  serializeSynthBank(scene_.synthBBank, synthBBank);
  ArduinoJson::JsonObject songObj = root["song"].to<ArduinoJson::JsonObject>();
  int songLen = songLength();
  songObj["length"] = songLen;
  ArduinoJson::JsonArray songPositions = songObj["positions"].to<ArduinoJson::JsonArray>();
  for (int i = 0; i < songLen; ++i) {
    ArduinoJson::JsonObject pos = songPositions.add<ArduinoJson::JsonObject>();
    pos["a"] = scene_.song.positions[i].patterns[0];
    pos["b"] = scene_.song.positions[i].patterns[1];
    pos["drums"] = scene_.song.positions[i].patterns[2];
  }

  ArduinoJson::JsonObject state = root["state"].to<ArduinoJson::JsonObject>();
  state["drumPatternIndex"] = drumPatternIndex_;
  state["bpm"] = bpm_;
  state["songMode"] = songMode_;
  state["songPosition"] = clampSongPosition(songPosition_);

  ArduinoJson::JsonArray synthPatternIndices = state["synthPatternIndex"].to<ArduinoJson::JsonArray>();
  synthPatternIndices.add(synthPatternIndex_[0]);
  synthPatternIndices.add(synthPatternIndex_[1]);

  state["drumBankIndex"] = drumBankIndex_;
  ArduinoJson::JsonArray synthBankIndices = state["synthBankIndex"].to<ArduinoJson::JsonArray>();
  synthBankIndices.add(synthBankIndex_[0]);
  synthBankIndices.add(synthBankIndex_[1]);

  ArduinoJson::JsonObject mute = state["mute"].to<ArduinoJson::JsonObject>();
  ArduinoJson::JsonArray drumMutes = mute["drums"].to<ArduinoJson::JsonArray>();
  for (int i = 0; i < DrumPatternSet::kVoices; ++i) {
    drumMutes.add(drumMute_[i]);
  }
  ArduinoJson::JsonArray synthMutes = mute["synth"].to<ArduinoJson::JsonArray>();
  synthMutes.add(synthMute_[0]);
  synthMutes.add(synthMute_[1]);

  ArduinoJson::JsonArray synthParams = state["synthParams"].to<ArduinoJson::JsonArray>();
  for (int i = 0; i < 2; ++i) {
    ArduinoJson::JsonObject param = synthParams.add<ArduinoJson::JsonObject>();
    param["cutoff"] = synthParameters_[i].cutoff;
    param["resonance"] = synthParameters_[i].resonance;
    param["envAmount"] = synthParameters_[i].envAmount;
    param["envDecay"] = synthParameters_[i].envDecay;
    param["oscType"] = synthParameters_[i].oscType;
  }
}

bool SceneManager::applySceneDocument(const ArduinoJson::JsonDocument& doc) {
  ArduinoJson::JsonObjectConst obj = doc.as<ArduinoJson::JsonObjectConst>();
  if (obj.isNull()) return false;

  ArduinoJson::JsonArrayConst drumBank = obj["drumBank"].as<ArduinoJson::JsonArrayConst>();
  ArduinoJson::JsonArrayConst synthABank = obj["synthABank"].as<ArduinoJson::JsonArrayConst>();
  ArduinoJson::JsonArrayConst synthBBank = obj["synthBBank"].as<ArduinoJson::JsonArrayConst>();
  if (drumBank.isNull() || synthABank.isNull() || synthBBank.isNull()) return false;

  Scene loaded{};
  clearSceneData(loaded);

  if (!deserializeDrumBank(drumBank, loaded.drumBank)) return false;
  if (!deserializeSynthBank(synthABank, loaded.synthABank)) return false;
  if (!deserializeSynthBank(synthBBank, loaded.synthBBank)) return false;

  int drumPatternIndex = 0;
  int synthPatternIndexA = 0;
  int synthPatternIndexB = 0;
  int drumBankIndex = 0;
  int synthBankIndexA = 0;
  int synthBankIndexB = 0;
  bool drumMute[DrumPatternSet::kVoices] = {false, false, false, false, false, false, false, false};
  bool synthMute[2] = {false, false};
  SynthParameters synthParams[2] = {SynthParameters(), SynthParameters()};
  float bpm = bpm_;
  Song loadedSong{};
  clearSong(loadedSong);
  bool hasSongObj = false;
  bool songMode = songMode_;
  int songPosition = songPosition_;

  ArduinoJson::JsonObjectConst songObj = obj["song"].as<ArduinoJson::JsonObjectConst>();
  if (!songObj.isNull()) {
    hasSongObj = true;
    int length = valueToInt(songObj["length"], loadedSong.length);
    loadedSong.length = clampSongLength(length);
    ArduinoJson::JsonArrayConst positions = songObj["positions"].as<ArduinoJson::JsonArrayConst>();
    if (!positions.isNull()) {
      int posIdx = 0;
      for (ArduinoJson::JsonVariantConst posVal : positions) {
        if (posIdx >= Song::kMaxPositions) break;
        ArduinoJson::JsonObjectConst posObj = posVal.as<ArduinoJson::JsonObjectConst>();
        if (!posObj.isNull()) {
          auto a = posObj["a"];
          auto b = posObj["b"];
          auto d = posObj["drums"];
          if (a.is<int>()) loadedSong.positions[posIdx].patterns[0] = a.as<int>();
          if (b.is<int>()) loadedSong.positions[posIdx].patterns[1] = b.as<int>();
          if (d.is<int>()) loadedSong.positions[posIdx].patterns[2] = d.as<int>();
        }
        if (posIdx + 1 > loadedSong.length) loadedSong.length = posIdx + 1;
        ++posIdx;
      }
    }
  }

  ArduinoJson::JsonObjectConst state = obj["state"].as<ArduinoJson::JsonObjectConst>();
  if (!state.isNull()) {
    drumPatternIndex = valueToInt(state["drumPatternIndex"], drumPatternIndex);
    bpm = valueToFloat(state["bpm"], bpm);
    ArduinoJson::JsonArrayConst synthPatternIndexArr = state["synthPatternIndex"].as<ArduinoJson::JsonArrayConst>();
    if (!synthPatternIndexArr.isNull()) {
      if (synthPatternIndexArr.size() > 0) synthPatternIndexA = valueToInt(synthPatternIndexArr[0], synthPatternIndexA);
      if (synthPatternIndexArr.size() > 1) synthPatternIndexB = valueToInt(synthPatternIndexArr[1], synthPatternIndexB);
    }
    drumBankIndex = valueToInt(state["drumBankIndex"], drumBankIndex);
    ArduinoJson::JsonArrayConst synthBankIndexArr = state["synthBankIndex"].as<ArduinoJson::JsonArrayConst>();
    if (!synthBankIndexArr.isNull()) {
      if (synthBankIndexArr.size() > 0) synthBankIndexA = valueToInt(synthBankIndexArr[0], synthBankIndexA);
      if (synthBankIndexArr.size() > 1) synthBankIndexB = valueToInt(synthBankIndexArr[1], synthBankIndexB);
    }
    ArduinoJson::JsonObjectConst muteObj = state["mute"].as<ArduinoJson::JsonObjectConst>();
    if (!muteObj.isNull()) {
      ArduinoJson::JsonArrayConst drumMuteArr = muteObj["drums"].as<ArduinoJson::JsonArrayConst>();
      if (!drumMuteArr.isNull() && !deserializeBoolArray(drumMuteArr, drumMute, DrumPatternSet::kVoices)) return false;
      ArduinoJson::JsonArrayConst synthMuteArr = muteObj["synth"].as<ArduinoJson::JsonArrayConst>();
      if (!synthMuteArr.isNull() && !deserializeBoolArray(synthMuteArr, synthMute, 2)) return false;
    }
    ArduinoJson::JsonArrayConst synthParamsArr = state["synthParams"].as<ArduinoJson::JsonArrayConst>();
    if (!synthParamsArr.isNull()) {
      int idx = 0;
      for (ArduinoJson::JsonVariantConst paramValue : synthParamsArr) {
        if (idx >= 2) break;
        SynthParameters parsed = synthParams[idx];
        if (!deserializeSynthParameters(paramValue, parsed)) return false;
        synthParams[idx] = parsed;
        ++idx;
      }
    }
    songMode = state["songMode"].is<bool>() ? state["songMode"].as<bool>() : songMode;
    songPosition = valueToInt(state["songPosition"], songPosition);
  }

  if (!hasSongObj) {
    loadedSong.length = 1;
    loadedSong.positions[0].patterns[0] = clampPatternIndex(synthPatternIndexA);
    loadedSong.positions[0].patterns[1] = clampPatternIndex(synthPatternIndexB);
    loadedSong.positions[0].patterns[2] = clampPatternIndex(drumPatternIndex);
  }

  scene_ = loaded;
  scene_.song = loadedSong;
  drumPatternIndex_ = clampPatternIndex(drumPatternIndex);
  synthPatternIndex_[0] = clampPatternIndex(synthPatternIndexA);
  synthPatternIndex_[1] = clampPatternIndex(synthPatternIndexB);
  drumBankIndex_ = clampIndex(drumBankIndex, 1);
  synthBankIndex_[0] = clampIndex(synthBankIndexA, 1);
  synthBankIndex_[1] = clampIndex(synthBankIndexB, 1);
  for (int i = 0; i < DrumPatternSet::kVoices; ++i) {
    drumMute_[i] = drumMute[i];
  }
  synthMute_[0] = synthMute[0];
  synthMute_[1] = synthMute[1];
  synthParameters_[0] = synthParams[0];
  synthParameters_[1] = synthParams[1];
  setSongLength(scene_.song.length);
  songPosition_ = clampSongPosition(songPosition);
  songMode_ = songMode;
  setBpm(bpm);
  return true;
}

std::string SceneManager::dumpCurrentScene() const {
  std::string serialized;
  writeSceneJson(serialized);
  return serialized;
}

bool SceneManager::loadScene(const std::string& json) {
  size_t idx = 0;
  JsonVisitor::NextChar nextChar = [&json, &idx]() -> int {
    if (idx >= json.size()) return -1;
    return static_cast<unsigned char>(json[idx++]);
  };
  if (loadSceneEventedWithReader(nextChar)) return true;
  return loadSceneJson(json);
}

bool SceneManager::loadSceneEventedWithReader(JsonVisitor::NextChar nextChar) {
  Scene loaded{};
  clearSceneData(loaded);

  struct NextCharStream {
    JsonVisitor::NextChar next;
    int read() { return next(); }
  };

  NextCharStream stream{std::move(nextChar)};
  JsonVisitor visitor;
  SceneJsonObserver observer(loaded, bpm_);
  bool parsed = visitor.parse(stream, observer);
  if (!parsed || observer.hadError()) return false;

  scene_ = loaded;
  scene_.song = observer.song();
  drumPatternIndex_ = clampPatternIndex(observer.drumPatternIndex());
  synthPatternIndex_[0] = clampPatternIndex(observer.synthPatternIndex(0));
  synthPatternIndex_[1] = clampPatternIndex(observer.synthPatternIndex(1));
  drumBankIndex_ = clampIndex(observer.drumBankIndex(), 1);
  synthBankIndex_[0] = clampIndex(observer.synthBankIndex(0), 1);
  synthBankIndex_[1] = clampIndex(observer.synthBankIndex(1), 1);
  if (!observer.hasSong()) {
    scene_.song.length = 1;
    scene_.song.positions[0].patterns[0] = synthPatternIndex_[0];
    scene_.song.positions[0].patterns[1] = synthPatternIndex_[1];
    scene_.song.positions[0].patterns[2] = drumPatternIndex_;
  }
  for (int i = 0; i < DrumPatternSet::kVoices; ++i) {
    drumMute_[i] = observer.drumMute(i);
  }
  synthMute_[0] = observer.synthMute(0);
  synthMute_[1] = observer.synthMute(1);
  synthParameters_[0] = observer.synthParameters(0);
  synthParameters_[1] = observer.synthParameters(1);
  setSongLength(scene_.song.length);
  songPosition_ = clampSongPosition(observer.songPosition());
  songMode_ = observer.songMode();
  setBpm(observer.bpm());
  return true;
}

int SceneManager::clampPatternIndex(int idx) const {
  return clampIndex(idx, Bank<DrumPatternSet>::kPatterns);
}

int SceneManager::clampSynthIndex(int idx) const {
  if (idx < 0) return 0;
  if (idx > 1) return 1;
  return idx;
}

int SceneManager::clampSongPosition(int position) const {
  int len = songLength();
  if (len < 1) len = 1;
  if (position < 0) return 0;
  if (position >= len) return len - 1;
  if (position >= Song::kMaxPositions) return Song::kMaxPositions - 1;
  return position;
}

int SceneManager::clampSongLength(int length) const {
  if (length < 1) return 1;
  if (length > Song::kMaxPositions) return Song::kMaxPositions;
  return length;
}

int SceneManager::songTrackToIndex(SongTrack track) const {
  switch (track) {
  case SongTrack::SynthA: return 0;
  case SongTrack::SynthB: return 1;
  case SongTrack::Drums: return 2;
  default: return -1;
  }
}

void SceneManager::trimSongLength() {
  int lastUsed = -1;
  for (int pos = scene_.song.length - 1; pos >= 0; --pos) {
    bool hasData = false;
    for (int t = 0; t < SongPosition::kTrackCount; ++t) {
      if (scene_.song.positions[pos].patterns[t] >= 0) {
        hasData = true;
        break;
      }
    }
    if (hasData) {
      lastUsed = pos;
      break;
    }
  }
  int newLength = lastUsed >= 0 ? lastUsed + 1 : 1;
  scene_.song.length = clampSongLength(newLength);
  if (songPosition_ >= scene_.song.length) songPosition_ = scene_.song.length - 1;
}

void SceneManager::clearSongData(Song& song) const {
  clearSong(song);
}
