// Smoke test of the real processor + editor without a host (Linux dev only).
// Usage: orgu_plugin_test [snapshot.png]
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../plugin/PluginEditor.h"
#include "../plugin/PluginProcessor.h"

static int failures = 0;
#define CHECK(c, msg)                         \
  do {                                        \
    if (!(c)) {                               \
      std::printf("FAIL: %s\n", msg);         \
      ++failures;                             \
    }                                         \
  } while (0)

static float renderNote(OrguProcessor& p, int blocks) {
  juce::AudioBuffer<float> buf(2, 512);
  float peak = 0.0f;
  for (int b = 0; b < blocks; ++b) {
    juce::MidiBuffer midi;
    if (b == 0) midi.addEvent(juce::MidiMessage::noteOn(1, 48, 0.9f), 17);
    if (b == blocks / 2) midi.addEvent(juce::MidiMessage::noteOff(1, 48), 3);
    buf.clear();
    p.processBlock(buf, midi);
    peak = juce::jmax(peak, buf.getMagnitude(0, 0, 512), buf.getMagnitude(1, 0, 512));
  }
  return peak;
}

int main(int argc, char** argv) {
  juce::ScopedJuceInitialiser_GUI init;
  OrguProcessor p;
  p.setPlayConfigDetails(0, 2, 44100.0, 512);
  p.prepareToPlay(44100.0, 512);

  for (int i = 0; i < p.getNumPrograms(); ++i) {
    p.setCurrentProgram(i);
    const float peak = renderNote(p, 60);
    std::printf("preset %2d %-16s peak %.3f\n", i, p.getProgramName(i).toRawUTF8(), peak);
    CHECK(peak > 0.01f && peak <= 1.0f, ("preset level " + p.getProgramName(i)).toRawUTF8());
  }

  // state round trip
  p.setCurrentProgram(3);
  juce::MemoryBlock state;
  p.getStateInformation(state);
  p.setCurrentProgram(0);
  p.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
  CHECK(p.presetName == "Acid Line", "preset name restored");
  CHECK(juce::roundToInt(p.apvts.getRawParameterValue(ids::voices)->load()) == 1, "voices restored");

  // editor builds and paints
  p.setCurrentProgram(1);
  renderNote(p, 20);
  {
    juce::MidiBuffer midi;
    midi.addEvent(juce::MidiMessage::noteOn(1, 52, 0.9f), 0);
    juce::AudioBuffer<float> buf(2, 512);
    for (int b = 0; b < 30; ++b) { p.processBlock(buf, midi); midi.clear(); }
  }
  std::unique_ptr<juce::AudioProcessorEditor> ed(p.createEditor());
  CHECK(ed->getWidth() == 1000 && ed->getHeight() == 720, "editor size");
  juce::MessageManager::getInstance()->runDispatchLoopUntil(200);
  auto img = ed->createComponentSnapshot(ed->getLocalBounds(), true, 1.0f);
  if (argc > 1) {
    juce::File f(juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]));
    f.deleteFile();
    juce::FileOutputStream os(f);
    juce::PNGImageFormat().writeImageToStream(img, os);
  }
  ed.reset();

  std::printf(failures == 0 ? "plugin smoke test passed\n" : "%d failures\n", failures);
  return failures == 0 ? 0 : 1;
}
