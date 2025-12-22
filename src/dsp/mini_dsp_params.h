#pragma once

class Parameter {
public:
  Parameter();
  Parameter(const char* label, const char* unit, float minValue, float maxValue, float defaultValue, float step);
  Parameter(const char* label, const char* unit, const char* const* options, int optionCount, int defaultIndex = 0);

  const char* label() const;
  const char* unit() const;
  float value() const;
  float min() const;
  float max() const;
  float step() const;
  float normalized() const;
  bool hasOptions() const;
  int optionCount() const;
  int optionIndex() const;
  const char* optionLabel() const;

  void setValue(float v);
  void addSteps(int steps);
  void setNormalized(float norm);
  void reset();

private:
  const char* _label;
  const char* _unit;
  float min_;
  float max_;
  float default_;
  float step_;
  float value_;
  const char* const* options_;
  int optionCount_;
};

inline Parameter::Parameter()
  : _label(""), _unit(""), min_(0.0f), max_(1.0f),
    default_(0.0f), step_(0.0f), value_(0.0f),
    options_(nullptr), optionCount_(0) {}

inline Parameter::Parameter(const char* label, const char* unit, float minValue, float maxValue, float defaultValue, float step)
  : _label(label), _unit(unit), min_(minValue), max_(maxValue),
    default_(defaultValue), step_(step), value_(defaultValue),
    options_(nullptr), optionCount_(0) {}

inline Parameter::Parameter(const char* label, const char* unit, const char* const* options, int optionCount, int defaultIndex)
  : _label(label), _unit(unit), min_(0.0f), max_(static_cast<float>(optionCount > 0 ? optionCount - 1 : 0)),
    default_(static_cast<float>(defaultIndex)), step_(1.0f), value_(0.0f),
    options_(options), optionCount_(optionCount) {
  if (optionCount_ < 0) optionCount_ = 0;
  int clampedDefault = defaultIndex;
  if (clampedDefault < 0) clampedDefault = 0;
  if (optionCount_ > 0 && clampedDefault >= optionCount_) clampedDefault = optionCount_ - 1;
  default_ = static_cast<float>(clampedDefault);
  value_ = static_cast<float>(clampedDefault);
}

inline const char* Parameter::label() const { return _label; }
inline const char* Parameter::unit() const { return _unit; }
inline float Parameter::value() const { return value_; }
inline float Parameter::min() const { return min_; }
inline float Parameter::max() const { return max_; }
inline float Parameter::step() const { return step_; }
inline bool Parameter::hasOptions() const { return optionCount_ > 0; }
inline int Parameter::optionCount() const { return optionCount_; }

inline float Parameter::normalized() const {
  if (hasOptions()) {
    if (optionCount_ <= 1) return 0.0f;
    float idx = static_cast<float>(optionIndex());
    return idx / static_cast<float>(optionCount_ - 1);
  }
  if (max_ <= min_) return 0.0f;
  return (value_ - min_) / (max_ - min_);
}

inline void Parameter::setValue(float v) {
  if (hasOptions()) {
    if (v < 0.0f) v = 0.0f;
    float maxIndex = static_cast<float>(optionCount_ > 0 ? optionCount_ - 1 : 0);
    if (v > maxIndex) v = maxIndex;
    value_ = static_cast<float>(static_cast<int>(v + 0.5f));
    return;
  }
  if (v < min_) v = min_;
  if (v > max_) v = max_;
  value_ = v;
}

inline void Parameter::addSteps(int steps) {
  setValue(value_ + step_ * steps);
}

inline void Parameter::setNormalized(float norm) {
  if (hasOptions()) {
    if (optionCount_ <= 0) {
      value_ = 0.0f;
      return;
    }
    if (norm < 0.0f) norm = 0.0f;
    if (norm > 1.0f) norm = 1.0f;
    float scaled = norm * static_cast<float>(optionCount_ - 1);
    value_ = static_cast<float>(static_cast<int>(scaled + 0.5f));
    return;
  }
  if (norm < 0.0f) norm = 0.0f;
  if (norm > 1.0f) norm = 1.0f;
  value_ = min_ + norm * (max_ - min_);
}

inline void Parameter::reset() {
  value_ = default_;
}

inline int Parameter::optionIndex() const {
  if (!hasOptions()) return static_cast<int>(value_);
  float v = value_;
  if (v < 0.0f) v = 0.0f;
  float maxIndex = static_cast<float>(optionCount_ - 1);
  if (v > maxIndex) v = maxIndex;
  return static_cast<int>(v + 0.5f);
}

inline const char* Parameter::optionLabel() const {
  if (!hasOptions() || !options_) return nullptr;
  int idx = optionIndex();
  if (idx < 0 || idx >= optionCount_) return nullptr;
  return options_[idx];
}
