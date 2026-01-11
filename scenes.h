#pragma once

#include <stdint.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>
#include <utility>
#include "ArduinoJson-v7.4.2.h"
#include "json_evented.h"

namespace scene_json_detail {
inline bool writeChunk(std::string& writer, const char* data, size_t len) {
  writer.append(data, len);
  return true;
}

template <typename Writer>
auto writeChunkImpl(Writer& writer, const char* data, size_t len, int)
    -> decltype(writer.write(reinterpret_cast<const uint8_t*>(data), len), bool()) {
  size_t written = writer.write(reinterpret_cast<const uint8_t*>(data), len);
  return written == len;
}

template <typename Writer>
auto writeChunkImpl(Writer& writer, const char* data, size_t len, long)
    -> decltype(writer.write(data, len), bool()) {
  auto written = writer.write(data, len);
  return static_cast<size_t>(written) == len;
}

template <typename Writer>
auto writeChunkImpl(Writer& writer, const char* data, size_t len, ...)
    -> decltype(writer.write(static_cast<uint8_t>(0)), bool()) {
  for (size_t i = 0; i < len; ++i) {
    if (writer.write(static_cast<uint8_t>(data[i])) != 1) return false;
  }
  return true;
}

template <typename Writer>
bool writeChunk(Writer& writer, const char* data, size_t len) {
  return writeChunkImpl(writer, data, len, 0);
}
} // namespace scene_json_detail

struct DrumStep {
  bool hit;
  bool accent;
};

struct DrumPattern {
  static constexpr int kSteps = 16;
  DrumStep steps[kSteps];
};

struct DrumPatternSet {
  static constexpr int kVoices = 8;
  DrumPattern voices[kVoices];
};

struct SynthStep {
  int note; 
  bool slide;
  bool accent;
};

struct SynthPattern {
  static constexpr int kSteps = 16;
  SynthStep steps[kSteps];
};

struct SynthParameters {
  float cutoff = 800.0f;
  float resonance = 0.6f;
  float envAmount = 400.0f;
  float envDecay = 420.0f;
  int oscType = 0;
};

enum class SongTrack : uint8_t {
  SynthA = 0,
  SynthB = 1,
  Drums = 2,
};

struct SongPosition {
  static constexpr int kTrackCount = 3;
  int8_t patterns[kTrackCount] = {-1, -1, -1};
};

struct Song {
  static constexpr int kMaxPositions = 128;
  SongPosition positions[kMaxPositions];
  int length = 1;
};

template <typename PatternType>
struct Bank {
  static constexpr int kPatterns = 8;
  PatternType patterns[kPatterns];
};

static constexpr int kBankCount = 4;
static constexpr int kSongPatternCount = kBankCount * Bank<SynthPattern>::kPatterns;

inline int clampSongPatternIndex(int idx) {
  if (idx < -1) return -1;
  int max = kSongPatternCount - 1;
  if (idx > max) return max;
  return idx;
}

inline int songPatternBank(int songPatternIdx) {
  if (songPatternIdx < 0) return -1;
  return songPatternIdx / Bank<SynthPattern>::kPatterns;
}

inline int songPatternIndexInBank(int songPatternIdx) {
  if (songPatternIdx < 0) return -1;
  return songPatternIdx % Bank<SynthPattern>::kPatterns;
}

inline int songPatternFromBank(int bankIndex, int patternIndex) {
  if (bankIndex < 0 || patternIndex < 0) return -1;
  if (bankIndex >= kBankCount) bankIndex = kBankCount - 1;
  if (patternIndex >= Bank<SynthPattern>::kPatterns) {
    patternIndex = Bank<SynthPattern>::kPatterns - 1;
  }
  return bankIndex * Bank<SynthPattern>::kPatterns + patternIndex;
}

struct Scene {
  Bank<DrumPatternSet> drumBanks[kBankCount];
  Bank<SynthPattern> synthABanks[kBankCount];
  Bank<SynthPattern> synthBBanks[kBankCount];
  Song song;
};

class SceneJsonObserver : public JsonObserver {
public:
  explicit SceneJsonObserver(Scene& scene, float defaultBpm = 100.0f);

  void onObjectStart() override;
  void onObjectEnd() override;
  void onArrayStart() override;
  void onArrayEnd() override;
  void onNumber(int value) override;
  void onNumber(double value) override;
  void onBool(bool value) override;
  void onNull() override;
  void onString(const std::string& value) override;
  void onObjectKey(const std::string& key) override;
  void onObjectValueStart() override;
  void onObjectValueEnd() override;

  bool hadError() const;
  int drumPatternIndex() const;
  int synthPatternIndex(int synthIdx) const;
  int drumBankIndex() const;
  int synthBankIndex(int synthIdx) const;
  bool drumMute(int idx) const;
  bool synthMute(int idx) const;
  bool synthDistortionEnabled(int idx) const;
  bool synthDelayEnabled(int idx) const;
  const SynthParameters& synthParameters(int synthIdx) const;
  float bpm() const;
  const Song& song() const;
  bool hasSong() const;
  bool songMode() const;
  int songPosition() const;
  const std::string& drumEngineName() const;

private:
  enum class Path {
    Root,
    DrumBanks,
    DrumBank,
    DrumPatternSet,
    DrumVoice,
    DrumHitArray,
    DrumAccentArray,
    SynthABanks,
    SynthABank,
    SynthBBanks,
    SynthBBank,
    SynthPattern,
    SynthStep,
    State,
    SynthPatternIndex,
    SynthBankIndex,
    Mute,
    MuteDrums,
    MuteSynth,
    SynthDistortion,
    SynthDelay,
    SynthParams,
    SynthParam,
    Song,
    SongPositions,
    SongPosition,
    Unknown,
  };

  struct Context {
    enum class Type { Object, Array };
    Type type;
    Path path;
    int index;
  };

  Path deduceArrayPath(const Context& parent) const;
  Path deduceObjectPath(const Context& parent) const;
  int currentIndexFor(Path path) const;
  bool inSynthBankA() const;
  bool inSynthBankB() const;
  void pushContext(Context::Type type, Path path);
  void popContext();
  void handlePrimitiveNumber(double value, bool isInteger);
  void handlePrimitiveBool(bool value);

  static constexpr int kMaxStack = 16;
  Context stack_[kMaxStack];
  int stackSize_ = 0;
  std::string lastKey_;
  Scene& target_;
  bool error_ = false;
  int drumPatternIndex_ = 0;
  int synthPatternIndex_[2] = {0, 0};
  int drumBankIndex_ = 0;
  int synthBankIndex_[2] = {0, 0};
  bool drumMute_[DrumPatternSet::kVoices] = {false, false, false, false, false, false, false, false};
  bool synthMute_[2] = {false, false};
  bool synthDistortion_[2] = {false, false};
  bool synthDelay_[2] = {false, false};
  SynthParameters synthParameters_[2];
  float bpm_ = 100.0f;
  Song song_;
  bool hasSong_ = false;
  bool songMode_ = false;
  int songPosition_ = 0;
  std::string drumEngineName_ = "808";
};

class SceneManager {
public:
  void loadDefaultScene();
  Scene& currentScene();
  const Scene& currentScene() const;

  const DrumPatternSet& getCurrentDrumPattern() const;
  DrumPatternSet& editCurrentDrumPattern();

  const SynthPattern& getCurrentSynthPattern(int synthIndex) const;
  SynthPattern& editCurrentSynthPattern(int synthIndex);
  const SynthPattern& getSynthPattern(int synthIndex, int patternIndex) const;
  SynthPattern& editSynthPattern(int synthIndex, int patternIndex);
  const DrumPatternSet& getDrumPatternSet(int patternIndex) const;
  DrumPatternSet& editDrumPatternSet(int patternIndex);

  void setCurrentDrumPatternIndex(int idx);
  void setCurrentSynthPatternIndex(int synthIdx, int idx);
  int getCurrentDrumPatternIndex() const;
  int getCurrentSynthPatternIndex(int synthIdx) const;

  void setCurrentBankIndex(int instrumentId, int bankIdx);
  int getCurrentBankIndex(int instrumentId) const;

  void setDrumStep(int voiceIdx, int step, bool hit, bool accent);
  void setSynthStep(int synthIdx, int step, int note, bool slide, bool accent);

  std::string dumpCurrentScene() const;
  bool loadScene(const std::string& json);

  void setDrumMute(int voiceIdx, bool mute);
  bool getDrumMute(int voiceIdx) const;
  void setSynthMute(int synthIdx, bool mute);
  bool getSynthMute(int synthIdx) const;
  void setSynthDistortionEnabled(int synthIdx, bool enabled);
  bool getSynthDistortionEnabled(int synthIdx) const;
  void setSynthDelayEnabled(int synthIdx, bool enabled);
  bool getSynthDelayEnabled(int synthIdx) const;
  void setSynthParameters(int synthIdx, const SynthParameters& params);
  const SynthParameters& getSynthParameters(int synthIdx) const;
  void setDrumEngineName(const std::string& name);
  const std::string& getDrumEngineName() const;
  void setBpm(float bpm);
  float getBpm() const;

  const Song& song() const;
  Song& editSong();
  void setSongPattern(int position, SongTrack track, int patternIndex);
  void clearSongPattern(int position, SongTrack track);
  int songPattern(int position, SongTrack track) const;
  void setSongLength(int length);
  int songLength() const;
  void setSongPosition(int position);
  int getSongPosition() const;
  void setSongMode(bool enabled);
  bool songMode() const;

  template <typename TWriter>
  bool writeSceneJson(TWriter&& writer) const;
  template <typename TReader>
  bool loadSceneJson(TReader&& reader);
  template <typename TReader>
  bool loadSceneEvented(TReader&& reader);

  // static constexpr size_t sceneJsonCapacity();

private:
  int clampPatternIndex(int idx) const;
  int clampBankIndex(int idx) const;
  int clampSynthIndex(int idx) const;
  int clampSongPosition(int position) const;
  int clampSongLength(int length) const;
  int songTrackToIndex(SongTrack track) const;
  void trimSongLength();
  void clearSongData(Song& song) const;
  void buildSceneDocument(ArduinoJson::JsonDocument& doc) const;
  bool applySceneDocument(const ArduinoJson::JsonDocument& doc);
  bool loadSceneEventedWithReader(JsonVisitor::NextChar nextChar);

  Scene scene_;
  int drumPatternIndex_ = 0;
  int synthPatternIndex_[2] = {0, 0};
  int drumBankIndex_ = 0;
  int synthBankIndex_[2] = {0, 0};
  bool drumMute_[DrumPatternSet::kVoices] = {false, false, false, false, false, false, false, false};
  bool synthMute_[2] = {false, false};
  bool synthDistortion_[2] = {false, false};
  bool synthDelay_[2] = {false, false};
  SynthParameters synthParameters_[2];
  float bpm_ = 100.0f;
  bool songMode_ = false;
  int songPosition_ = 0;
  std::string drumEngineName_ = "808";
};

// inline constexpr size_t SceneManager::sceneJsonCapacity() {
//   constexpr size_t drumPatternJsonSize = JSON_OBJECT_SIZE(2) + 2 * JSON_ARRAY_SIZE(DrumPattern::kSteps);
//   constexpr size_t drumPatternSetJsonSize = JSON_ARRAY_SIZE(DrumPatternSet::kVoices) +
//                                             DrumPatternSet::kVoices * drumPatternJsonSize;
//   constexpr size_t drumBankJsonSize = JSON_ARRAY_SIZE(Bank<DrumPatternSet>::kPatterns) +
//                                       Bank<DrumPatternSet>::kPatterns * drumPatternSetJsonSize;

//   constexpr size_t synthStepJsonSize = JSON_OBJECT_SIZE(3);
//   constexpr size_t synthPatternJsonSize = JSON_ARRAY_SIZE(SynthPattern::kSteps) +
//                                           SynthPattern::kSteps * synthStepJsonSize;
//   constexpr size_t synthBankJsonSize = JSON_ARRAY_SIZE(Bank<SynthPattern>::kPatterns) +
//                                        Bank<SynthPattern>::kPatterns * synthPatternJsonSize;

//   constexpr size_t stateJsonSize = JSON_OBJECT_SIZE(4) + JSON_ARRAY_SIZE(2) + JSON_ARRAY_SIZE(2);

//   return JSON_OBJECT_SIZE(4) + drumBankJsonSize + synthBankJsonSize * 2 + stateJsonSize + 64; // small headroom
// }

template <typename TWriter>
bool SceneManager::writeSceneJson(TWriter&& writer) const {
  using WriterType = typename std::remove_reference<TWriter>::type;
  WriterType& out = writer;

  auto writeChunk = [&](const char* data, size_t len) -> bool {
    return scene_json_detail::writeChunk(out, data, len);
  };
  auto writeLiteral = [&](const char* literal) -> bool {
    return writeChunk(literal, std::strlen(literal));
  };
  auto writeChar = [&](char c) -> bool {
    return writeChunk(&c, 1);
  };
  auto writeBool = [&](bool value) -> bool {
    return writeLiteral(value ? "true" : "false");
  };
  auto writeInt = [&](int value) -> bool {
    char buffer[16];
    int written = std::snprintf(buffer, sizeof(buffer), "%d", value);
    if (written < 0 || written >= static_cast<int>(sizeof(buffer))) return false;
    return writeChunk(buffer, static_cast<size_t>(written));
  };
  auto writeFloat = [&](float value) -> bool {
    char buffer[24];
    int written = std::snprintf(buffer, sizeof(buffer), "%.6g", static_cast<double>(value));
    if (written < 0 || written >= static_cast<int>(sizeof(buffer))) return false;
    return writeChunk(buffer, static_cast<size_t>(written));
  };
  auto writeBoolArray = [&](const bool* values, int count) -> bool {
    for (int i = 0; i < count; ++i) {
      if (i > 0 && !writeChar(',')) return false;
      if (!writeBool(values[i])) return false;
    }
    return true;
  };
  auto writeString = [&](const std::string& value) -> bool {
    if (!writeChar('"')) return false;
    for (char ch : value) {
      if (ch == '"' || ch == '\\') {
        if (!writeChar('\\')) return false;
      }
      if (!writeChar(ch)) return false;
    }
    return writeChar('"');
  };
  auto writeDrumPattern = [&](const DrumPattern& pattern) -> bool {
    if (!writeLiteral("{\"hit\":[")) return false;
    for (int i = 0; i < DrumPattern::kSteps; ++i) {
      if (i > 0 && !writeChar(',')) return false;
      if (!writeBool(pattern.steps[i].hit)) return false;
    }
    if (!writeLiteral("],\"accent\":[")) return false;
    for (int i = 0; i < DrumPattern::kSteps; ++i) {
      if (i > 0 && !writeChar(',')) return false;
      if (!writeBool(pattern.steps[i].accent)) return false;
    }
    return writeLiteral("]}");
  };
  auto writeDrumBank = [&](const Bank<DrumPatternSet>& bank) -> bool {
    if (!writeChar('[')) return false;
    for (int p = 0; p < Bank<DrumPatternSet>::kPatterns; ++p) {
      if (p > 0 && !writeChar(',')) return false;
      if (!writeChar('[')) return false;
      for (int v = 0; v < DrumPatternSet::kVoices; ++v) {
        if (v > 0 && !writeChar(',')) return false;
        if (!writeDrumPattern(bank.patterns[p].voices[v])) return false;
      }
      if (!writeChar(']')) return false;
    }
    return writeChar(']');
  };
  auto writeDrumBanks = [&](const Bank<DrumPatternSet>* banks) -> bool {
    if (!writeChar('[')) return false;
    for (int b = 0; b < kBankCount; ++b) {
      if (b > 0 && !writeChar(',')) return false;
      if (!writeDrumBank(banks[b])) return false;
    }
    return writeChar(']');
  };
  auto writeSynthPattern = [&](const SynthPattern& pattern) -> bool {
    if (!writeChar('[')) return false;
    for (int i = 0; i < SynthPattern::kSteps; ++i) {
      if (i > 0 && !writeChar(',')) return false;
      if (!writeLiteral("{\"note\":")) return false;
      if (!writeInt(pattern.steps[i].note)) return false;
      if (!writeLiteral(",\"slide\":")) return false;
      if (!writeBool(pattern.steps[i].slide)) return false;
      if (!writeLiteral(",\"accent\":")) return false;
      if (!writeBool(pattern.steps[i].accent)) return false;
      if (!writeChar('}')) return false;
    }
    return writeChar(']');
  };
  auto writeSynthBank = [&](const Bank<SynthPattern>& bank) -> bool {
    if (!writeChar('[')) return false;
    for (int p = 0; p < Bank<SynthPattern>::kPatterns; ++p) {
      if (p > 0 && !writeChar(',')) return false;
      if (!writeSynthPattern(bank.patterns[p])) return false;
    }
    return writeChar(']');
  };
  auto writeSynthBanks = [&](const Bank<SynthPattern>* banks) -> bool {
    if (!writeChar('[')) return false;
    for (int b = 0; b < kBankCount; ++b) {
      if (b > 0 && !writeChar(',')) return false;
      if (!writeSynthBank(banks[b])) return false;
    }
    return writeChar(']');
  };

  if (!writeChar('{')) return false;

  if (!writeLiteral("\"drumBanks\":")) return false;
  if (!writeDrumBanks(scene_.drumBanks)) return false;

  if (!writeLiteral(",\"synthABanks\":")) return false;
  if (!writeSynthBanks(scene_.synthABanks)) return false;

  if (!writeLiteral(",\"synthBBanks\":")) return false;
  if (!writeSynthBanks(scene_.synthBBanks)) return false;

  if (!writeLiteral(",\"song\":{")) return false;
  int songLen = songLength();
  if (!writeLiteral("\"length\":")) return false;
  if (!writeInt(songLen)) return false;
  if (!writeLiteral(",\"positions\":[")) return false;
  for (int i = 0; i < songLen; ++i) {
    if (i > 0 && !writeChar(',')) return false;
    if (!writeChar('{')) return false;
    if (!writeLiteral("\"a\":")) return false;
    if (!writeInt(scene_.song.positions[i].patterns[0])) return false;
    if (!writeLiteral(",\"b\":")) return false;
    if (!writeInt(scene_.song.positions[i].patterns[1])) return false;
    if (!writeLiteral(",\"drums\":")) return false;
    if (!writeInt(scene_.song.positions[i].patterns[2])) return false;
    if (!writeChar('}')) return false;
  }
  if (!writeChar(']')) return false;
  if (!writeChar('}')) return false;

  if (!writeLiteral(",\"state\":{")) return false;
  if (!writeLiteral("\"drumPatternIndex\":")) return false;
  if (!writeInt(drumPatternIndex_)) return false;
  if (!writeLiteral(",\"bpm\":")) return false;
  if (!writeFloat(bpm_)) return false;
  if (!writeLiteral(",\"songMode\":")) return false;
  if (!writeBool(songMode_)) return false;
  if (!writeLiteral(",\"songPosition\":")) return false;
  if (!writeInt(clampSongPosition(songPosition_))) return false;
  if (!writeLiteral(",\"synthPatternIndex\":[")) return false;
  if (!writeInt(synthPatternIndex_[0])) return false;
  if (!writeChar(',')) return false;
  if (!writeInt(synthPatternIndex_[1])) return false;
  if (!writeChar(']')) return false;
  if (!writeLiteral(",\"drumBankIndex\":")) return false;
  if (!writeInt(drumBankIndex_)) return false;
  if (!writeLiteral(",\"drumEngine\":")) return false;
  if (!writeString(drumEngineName_)) return false;
  if (!writeLiteral(",\"synthBankIndex\":[")) return false;
  if (!writeInt(synthBankIndex_[0])) return false;
  if (!writeChar(',')) return false;
  if (!writeInt(synthBankIndex_[1])) return false;
  if (!writeChar(']')) return false;
  if (!writeLiteral(",\"mute\":{")) return false;
  if (!writeLiteral("\"drums\":[")) return false;
  if (!writeBoolArray(drumMute_, DrumPatternSet::kVoices)) return false;
  if (!writeLiteral("],\"synth\":[")) return false;
  if (!writeBoolArray(synthMute_, 2)) return false;
  if (!writeChar(']')) return false;
  if (!writeChar('}')) return false;
  if (!writeLiteral(",\"synthParams\":[")) return false;
  for (int i = 0; i < 2; ++i) {
    if (i > 0 && !writeChar(',')) return false;
    if (!writeLiteral("{\"cutoff\":")) return false;
    if (!writeFloat(synthParameters_[i].cutoff)) return false;
    if (!writeLiteral(",\"resonance\":")) return false;
    if (!writeFloat(synthParameters_[i].resonance)) return false;
    if (!writeLiteral(",\"envAmount\":")) return false;
    if (!writeFloat(synthParameters_[i].envAmount)) return false;
    if (!writeLiteral(",\"envDecay\":")) return false;
    if (!writeFloat(synthParameters_[i].envDecay)) return false;
    if (!writeLiteral(",\"oscType\":")) return false;
    if (!writeInt(synthParameters_[i].oscType)) return false;
    if (!writeChar('}')) return false;
  }
  if (!writeChar(']')) return false;
  if (!writeLiteral(",\"synthDistortion\":[")) return false;
  if (!writeBool(synthDistortion_[0])) return false;
  if (!writeChar(',')) return false;
  if (!writeBool(synthDistortion_[1])) return false;
  if (!writeChar(']')) return false;
  if (!writeLiteral(",\"synthDelay\":[")) return false;
  if (!writeBool(synthDelay_[0])) return false;
  if (!writeChar(',')) return false;
  if (!writeBool(synthDelay_[1])) return false;
  if (!writeChar(']')) return false;
  if (!writeChar('}')) return false;

  if (!writeChar('}')) return false;
  return true;
}

template <typename TReader>
bool SceneManager::loadSceneJson(TReader&& reader) {
  ArduinoJson::JsonDocument doc;
  auto error = ArduinoJson::deserializeJson(doc, std::forward<TReader>(reader));
  if (error) return false;
  return applySceneDocument(doc);
}

template <typename TReader>
bool SceneManager::loadSceneEvented(TReader&& reader) {
  JsonVisitor::NextChar nextChar = [&reader]() -> int { return reader.read(); };
  return loadSceneEventedWithReader(nextChar);
}
