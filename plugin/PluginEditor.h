#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

namespace ui {

namespace col {
const juce::Colour bg{0xff0d1220}, panel{0xff111a2e}, border{0xff222c47}, sidebar{0xff0f1628},
    control{0xff182037}, controlLine{0xff2a3452}, tile{0xff141d33}, text{0xffe8ecf5},
    dim{0xff98a2b8}, soft{0xffc3cadb}, accent{0xffff9a3c}, aux{0xff4fd1e8},
    bankNew{0xff7be08a}, bankPitched{0xffffb070}, bankNoise{0xffc9a6ff};
}

juce::Font font(float size, bool bold = false);
inline juce::String U(const char* s) { return juce::String::fromUTF8(s); }

class LookAndFeel : public juce::LookAndFeel_V4 {
 public:
  LookAndFeel();
  void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end,
                        juce::Slider&) override;
  void drawLinearSlider(juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                        juce::Slider::SliderStyle, juce::Slider&) override;
  void drawComboBox(juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
  void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
  juce::Font getComboBoxFont(juce::ComboBox&) override { return font(12.0f); }
  juce::Font getPopupMenuFont() override { return font(13.0f); }
  void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
  juce::Font getTextButtonFont(juce::TextButton&, int) override { return font(12.0f, true); }
};

// Ring knob bound to one parameter. Big knobs show the value inside the ring.
class Knob : public juce::Component {
 public:
  Knob(juce::AudioProcessorValueTreeState& state, const juce::String& id, const juce::String& label, bool big,
       bool bipolar = false);
  void paint(juce::Graphics&) override;
  void resized() override;
  juce::Slider slider;

 private:
  juce::String label;
  bool big;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

// Row of buttons that each set a parameter to a fixed value.
class Segments : public juce::Component {
 public:
  Segments(juce::AudioProcessorValueTreeState& state, const juce::String& id,
           std::vector<std::pair<juce::String, float>> options);
  void paint(juce::Graphics&) override;
  void mouseDown(const juce::MouseEvent&) override;

 private:
  int indexAt(juce::Point<float>) const;
  std::vector<std::pair<juce::String, float>> options;
  float current = 0.0f;
  std::unique_ptr<juce::ParameterAttachment> attachment;
};

class EngineList : public juce::Component {
 public:
  explicit EngineList(juce::AudioProcessorValueTreeState& state);
  void paint(juce::Graphics&) override;
  void mouseDown(const juce::MouseEvent&) override;
  void mouseMove(const juce::MouseEvent&) override;
  void mouseExit(const juce::MouseEvent&) override;
  std::function<void(int)> onChange;
  int selected() const { return current; }

 private:
  juce::Rectangle<float> rowBounds(int engine) const;
  int engineAt(juce::Point<float>) const;
  int current = 13, hover = -1;
  std::unique_ptr<juce::ParameterAttachment> attachment;
};

class Scope : public juce::Component, private juce::Timer {
 public:
  explicit Scope(OrguProcessor& p);
  void paint(juce::Graphics&) override;

 private:
  void timerCallback() override;
  OrguProcessor& proc;
  std::vector<float> out, aux;
  int start = 0;
  float gain = 2.0f;
  int voices = 0;
  static constexpr int kWindow = 960;
};

class ModRow : public juce::Component {
 public:
  ModRow(juce::AudioProcessorValueTreeState& state, int slot);
  void resized() override;
  void paint(juce::Graphics&) override;

 private:
  juce::ComboBox source, dest;
  juce::Slider amount;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> srcAtt, dstAtt;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amtAtt;
};

class ShapeBox : public juce::Component {
 public:
  ShapeBox(juce::AudioProcessorValueTreeState& state, const juce::String& id);
  void resized() override { box.setBounds(getLocalBounds()); }
  juce::ComboBox box;

 private:
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att;
};

}  // namespace ui

class OrguEditor : public juce::AudioProcessorEditor, private juce::Timer {
 public:
  explicit OrguEditor(OrguProcessor&);
  ~OrguEditor() override;
  void paint(juce::Graphics&) override;
  void resized() override;

 private:
  void timerCallback() override;
  void showPresetMenu();
  void stepPreset(int delta);
  void card(juce::Graphics&, juce::Rectangle<int>, const juce::String& title) const;

  OrguProcessor& proc;
  ui::LookAndFeel laf;
  ui::EngineList engines;
  ui::Scope scope;
  ui::Segments voices, ampMode, stereo;
  ui::Knob frequency, harmonics, timbre, morph;
  ui::Knob fmEnv, timbreEnv, morphEnv, lpgDecay, lpgColor;
  ui::Knob attack, decay, sustain, release, glide, bend;
  ui::Knob lfo1Rate, lfo2Rate, menvAttack, menvDecay, outAux, level;
  ui::ShapeBox lfo1Shape, lfo2Shape;
  std::array<std::unique_ptr<ui::ModRow>, orgu::kNumSlots> modRows;
  juce::TextButton prevPreset{"<"}, nextPreset{">"}, presetButton, saveButton{"Save"}, loadButton{"Load"};
  std::unique_ptr<juce::FileChooser> chooser;
  juce::String shownPreset;
  int shownEngine = -1;
  int shownAmp = -1;

  // layout (computed in resized, used by paint)
  juce::Rectangle<int> rScope, rMacro[4], rEnvMod, rLpg, rAmp, rPlay, rMod, rLfo, rOut;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrguEditor)
};
