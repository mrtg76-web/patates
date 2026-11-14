#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "OrguSynth.h"

namespace ids {
inline const juce::String engine = "engine", frequency = "frequency", harmonics = "harmonics",
                          timbre = "timbre", morph = "morph", fmEnv = "fmEnv", timbreEnv = "timbreEnv",
                          morphEnv = "morphEnv", lpgDecay = "lpgDecay", lpgColor = "lpgColor",
                          outAux = "outAux", stereo = "stereo", level = "level", ampMode = "ampMode",
                          attack = "attack", decay = "decay", sustain = "sustain", release = "release",
                          voices = "voices", glide = "glide", bendRange = "bendRange",
                          lfo1Rate = "lfo1Rate", lfo1Shape = "lfo1Shape", lfo2Rate = "lfo2Rate",
                          lfo2Shape = "lfo2Shape", menvAttack = "menvAttack", menvDecay = "menvDecay";
inline juce::String slotSrc(int i) { return "slot" + juce::String(i + 1) + "Src"; }
inline juce::String slotDst(int i) { return "slot" + juce::String(i + 1) + "Dst"; }
inline juce::String slotAmt(int i) { return "slot" + juce::String(i + 1) + "Amt"; }
}  // namespace ids

class OrguProcessor : public juce::AudioProcessor {
 public:
  OrguProcessor();
  ~OrguProcessor() override = default;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override {}
  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
  using AudioProcessor::processBlock;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }

  const juce::String getName() const override { return "Orgu"; }
  bool acceptsMidi() const override { return true; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 2.0; }

  int getNumPrograms() override;
  int getCurrentProgram() override { return currentPreset; }
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int, const juce::String&) override {}

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  juce::AudioProcessorValueTreeState apvts;
  orgu::Synth& synth() { return synth_; }

  // presets
  int currentPreset = 0;
  juce::String presetName { "Init" };
  void loadFactoryPreset(int index);
  bool savePresetFile(const juce::File& f);
  bool loadPresetFile(const juce::File& f);
  static juce::File presetFolder();

 private:
  static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
  void pullParams();
  void handleMidi(const juce::MidiMessage& m);

  orgu::Synth synth_;
  std::vector<float> scratch;
  struct Raw {
    std::atomic<float>* p = nullptr;
    float get() const { return p ? p->load() : 0.0f; }
  };
  Raw frequency, harmonics, timbre, morph, fmEnv, timbreEnv, morphEnv, lpgDecay, lpgColor, outAux,
      stereo, level, ampMode, attack, decay, sustain, release, voices, glide, bendRange, lfo1Rate,
      lfo1Shape, lfo2Rate, lfo2Shape, menvAttack, menvDecay, engine;
  Raw slotSrc[orgu::kNumSlots], slotDst[orgu::kNumSlots], slotAmt[orgu::kNumSlots];

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrguProcessor)
};
