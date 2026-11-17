#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"

// Örgü editor — design B "panel + genişleme": a Plaits-style hardware panel on the left and an
// expansion panel (scope, envelopes, LFOs, mod matrix, output) on the right, drawn in Kum's
// light hardware-panel language. Designed at 1200 x 720 and scaled as a whole.
namespace ui {

namespace col {
const juce::Colour panel{0xffd6d8d2}, border{0xffb9bbb4}, divider{0xffb4b6af}, ink{0xff1d1f1c},
    inkSoft{0xff4a4d47}, inkMid{0xff3a3d38}, knob{0xff1b1c1a}, knobSmall{0xff2a2b28}, knobShadow{0xff9fa29a},
    pointer{0xfff4f4f0}, red{0xffe5483a}, ledOff{0xff9fa29a}, green{0xff3c9a4a}, amber{0xffd9a21b},
    buttonFace{0xffeceee8}, badgeLine{0xff8f928a}, lcd{0xff1a1c19}, lcdGrid{0xff2c2f2a}, lcdText{0xffa9ada4},
    out{0xffff9a3c}, aux{0xff4fd1e8};
}

juce::Colour bankColour(int bank);
juce::Font panelFont(float size, bool bold = true);
juce::Font monoFont(float size);
inline juce::String U(const char* s) { return juce::String::fromUTF8(s); }

class LookAndFeel : public juce::LookAndFeel_V4 {
 public:
  LookAndFeel();
  void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end,
                        juce::Slider&) override;
  void drawLinearSlider(juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                        juce::Slider::SliderStyle, juce::Slider&) override;
  juce::Font getPopupMenuFont() override { return panelFont(13.0f, false); }
};

// Rotary knob bound to one parameter. Big = black with scale, Medium = small face with scale,
// Small = small face, Tiny = no ticks. A red pointer marks an envelope / bipolar amount.
class Knob : public juce::Slider {
 public:
  enum Style { Big, Medium, Small, Tiny };
  Knob(juce::AudioProcessorValueTreeState& state, const juce::String& paramId, Style s, bool redPointer = false);
  Style style;
  bool red;
  juce::String paramId;
  std::function<void(const juce::String&)> onTouch;
  void mouseDown(const juce::MouseEvent&) override;

 private:
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

// Push button drawn in the panel style.
class PanelButton : public juce::Component {
 public:
  enum Look { Round, Badge };
  PanelButton(juce::String text, Look look);
  void paint(juce::Graphics&) override;
  void mouseDown(const juce::MouseEvent&) override;
  void mouseUp(const juce::MouseEvent&) override;
  juce::String text;
  Look look;
  bool lit = false;
  std::function<void()> onPress;

 private:
  bool pressed = false;
};

// Row of badge cells that each set a parameter to a fixed (real-unit) value.
class Segments : public juce::Component {
 public:
  Segments(juce::AudioProcessorValueTreeState& state, const juce::String& id,
           std::vector<std::pair<juce::String, float>> options);
  void paint(juce::Graphics&) override;
  void mouseDown(const juce::MouseEvent&) override;

 private:
  std::vector<std::pair<juce::String, float>> options;
  float current = 0.0f;
  std::unique_ptr<juce::ParameterAttachment> attachment;
};

// Badge showing a choice parameter; click opens a menu of its choices.
class ChoiceBadge : public juce::Component {
 public:
  ChoiceBadge(juce::AudioProcessorValueTreeState& state, const juce::String& id, juce::StringArray shownNames,
              bool lit = false);
  void paint(juce::Graphics&) override;
  void mouseDown(const juce::MouseEvent&) override;

 private:
  juce::StringArray names;
  bool lit;
  int current = 0;
  juce::RangedAudioParameter* param = nullptr;
  std::unique_ptr<juce::ParameterAttachment> attachment;
};

// Dark LCD oscilloscope: OUT (orange) and AUX (cyan).
class Scope : public juce::Component {
 public:
  explicit Scope(OrguProcessor& p);
  void paint(juce::Graphics&) override;
  void update();          // called from the editor timer
  float peak = 0.0f;      // output peak of the last window (for the meter)

 private:
  OrguProcessor& proc;
  std::vector<float> out, aux;
  int start = 0;
  float gain = 2.0f;
  int voices = 0, maxVoices = 8;
  static constexpr int kWindow = 960;
};

}  // namespace ui

class OrguEditor : public juce::AudioProcessorEditor, private juce::Timer {
 public:
  explicit OrguEditor(OrguProcessor&);
  ~OrguEditor() override;
  void paint(juce::Graphics&) override;
  void resized() override;

  static constexpr int kW = 1200, kH = 720;

 private:
  void timerCallback() override;
  void showValue(const juce::String& paramId);
  void selectEngine(int engine);
  int engine() const;
  void showEngineMenu();
  void showPresetMenu();
  void stepPreset(int delta);
  void savePreset();
  void loadPreset();

  class Content : public juce::Component {
   public:
    explicit Content(OrguEditor& e) : ed(e) {}
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    OrguEditor& ed;
  };

  OrguProcessor& proc;
  ui::LookAndFeel laf;
  Content content{*this};
  ui::Scope scope;

  // left panel
  ui::Knob frequency, harmonics, timbre, morph, fmEnv, timbreEnv, morphEnv, lpgDecay, lpgColor;
  ui::PanelButton prevEngine{"Önceki motor", ui::PanelButton::Round};
  ui::PanelButton nextEngine{"Sonraki motor", ui::PanelButton::Round};
  // expansion panel
  ui::Segments ampMode, voices;
  ui::Knob attack, decay, sustain, release, glide, bend;
  ui::Knob lfo1Rate, lfo2Rate, menvAttack, menvDecay, outAux, level;
  ui::ChoiceBadge lfo1Shape, lfo2Shape;
  std::vector<std::unique_ptr<ui::ChoiceBadge>> modSrc, modDst;
  std::vector<std::unique_ptr<juce::Slider>> modAmt;
  std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> modAmtAtt;
  ui::PanelButton stereoButton{"ST", ui::PanelButton::Badge};
  // header
  ui::PanelButton prevPreset{U8("‹"), ui::PanelButton::Badge}, nextPreset{U8("›"), ui::PanelButton::Badge};
  ui::PanelButton presetButton{"", ui::PanelButton::Badge};
  ui::PanelButton saveButton{"KAYDET", ui::PanelButton::Badge}, loadButton{"", ui::PanelButton::Badge};

  std::unique_ptr<juce::FileChooser> chooser;
  std::unique_ptr<juce::ComponentBoundsConstrainer> constrainer;
  juce::String readout;
  juce::uint32 readoutTime = 0;
  float shownLevel = 0.0f;
  int shownAmp = -1;

  static juce::String U8(const char* s) { return juce::String::fromUTF8(s); }

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrguEditor)
};
