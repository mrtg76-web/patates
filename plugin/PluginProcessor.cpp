#include "PluginProcessor.h"

#include "PluginEditor.h"

namespace {

using NRange = juce::NormalisableRange<float>;

NRange skewed(float lo, float hi, float centre, float step = 0.0f) {
  NRange r(lo, hi, step);
  r.setSkewForCentre(centre);
  return r;
}

juce::String percent(float v, int) { return juce::String(juce::roundToInt(v * 100.0f)) + "%"; }
juce::String bipolar(float v, int) {
  const int p = juce::roundToInt(v * 100.0f);
  return (p > 0 ? "+" : "") + juce::String(p) + "%";
}
juce::String seconds(float v, int) {
  return v < 1.0f ? juce::String(juce::roundToInt(v * 1000.0f)) + " ms" : juce::String(v, 2) + " s";
}

struct FactoryPreset {
  const char* name;
  std::vector<std::pair<const char*, float>> values;   // parameter id → real value
};

// Values are in real units (choices by index). Anything not listed stays at its default.
const std::vector<FactoryPreset>& factoryPresets() {
  static const std::vector<FactoryPreset> list = {
    {"Init", {}},
    {"Glass Wavetable", {{"engine", 13}, {"harmonics", 0.35f}, {"timbre", 0.6f}, {"morph", 0.4f}, {"timbreEnv", 0.3f},
                         {"ampMode", 1}, {"attack", 0.01f}, {"decay", 0.8f}, {"sustain", 0.6f}, {"release", 1.2f},
                         {"lfo1Rate", 0.3f}, {"slot3Src", 1}, {"slot3Dst", 4}, {"slot3Amt", 0.15f}}},
    {"Analog Pad", {{"engine", 8}, {"harmonics", 0.25f}, {"timbre", 0.4f}, {"morph", 0.6f}, {"ampMode", 1},
                    {"attack", 0.6f}, {"decay", 1.0f}, {"sustain", 0.8f}, {"release", 1.5f}, {"stereo", 1},
                    {"lfo1Rate", 0.2f}, {"slot3Src", 1}, {"slot3Dst", 2}, {"slot3Amt", 0.08f}}},
    {"Acid Line", {{"engine", 0}, {"voices", 1}, {"glide", 0.08f}, {"harmonics", 0.7f}, {"timbre", 0.3f},
                   {"morph", 0.9f}, {"lpgDecay", 0.35f}, {"lpgColor", 0.7f}, {"timbreEnv", 0.5f}, {"frequency", -12}}},
    {"Phase Bass", {{"engine", 1}, {"voices", 1}, {"harmonics", 0.3f}, {"timbre", 0.5f}, {"morph", 0.4f},
                    {"timbreEnv", 0.6f}, {"ampMode", 1}, {"attack", 0.001f}, {"decay", 0.3f}, {"sustain", 0.5f},
                    {"release", 0.2f}, {"frequency", -12}}},
    {"DX Keys", {{"engine", 2}, {"harmonics", 0.1f}, {"timbre", 0.45f}, {"morph", 0.5f}, {"ampMode", 1},
                 {"attack", 0.002f}, {"decay", 1.2f}, {"sustain", 0.5f}, {"release", 0.6f}}},
    {"DX Brass", {{"engine", 3}, {"harmonics", 0.3f}, {"timbre", 0.5f}, {"morph", 0.5f}, {"ampMode", 1},
                  {"attack", 0.05f}, {"decay", 0.6f}, {"sustain", 0.8f}, {"release", 0.4f}}},
    {"Wave Terrain", {{"engine", 5}, {"ampMode", 1}, {"attack", 0.2f}, {"sustain", 0.9f}, {"release", 1.0f},
                      {"lfo1Rate", 0.1f}, {"slot3Src", 1}, {"slot3Dst", 4}, {"slot3Amt", 0.2f},
                      {"lfo2Rate", 0.07f}, {"slot4Src", 2}, {"slot4Dst", 3}, {"slot4Amt", 0.2f}}},
    {"String Machine", {{"engine", 6}, {"harmonics", 0.2f}, {"ampMode", 1}, {"attack", 0.3f}, {"decay", 0.5f},
                        {"sustain", 0.9f}, {"release", 1.0f}, {"stereo", 1}}},
    {"Chip Arp", {{"engine", 7}, {"voices", 4}, {"harmonics", 0.4f}, {"timbre", 0.3f}, {"ampMode", 1},
                  {"attack", 0.001f}, {"decay", 0.3f}, {"sustain", 0.7f}, {"release", 0.2f}}},
    {"FM Bell", {{"engine", 10}, {"harmonics", 0.55f}, {"timbre", 0.5f}, {"morph", 0.2f}, {"lpgDecay", 0.75f},
                 {"lpgColor", 0.3f}, {"timbreEnv", 0.4f}}},
    {"Formant Choir", {{"engine", 11}, {"timbre", 0.4f}, {"morph", 0.6f}, {"ampMode", 1}, {"attack", 0.4f},
                       {"sustain", 0.9f}, {"release", 1.0f}, {"lfo1Rate", 0.2f}, {"slot3Src", 1},
                       {"slot3Dst", 3}, {"slot3Amt", 0.1f}}},
    {"Drawbar Organ", {{"engine", 12}, {"harmonics", 0.4f}, {"timbre", 0.3f}, {"morph", 0.6f}, {"ampMode", 1},
                       {"attack", 0.005f}, {"decay", 0.1f}, {"sustain", 1.0f}, {"release", 0.1f}, {"outAux", 1.0f}}},
    {"Chord Stabs", {{"engine", 14}, {"harmonics", 0.3f}, {"timbre", 0.4f}, {"morph", 0.3f}, {"lpgDecay", 0.4f},
                     {"voices", 2}}},
    {"Talking", {{"engine", 15}, {"morph", 0.3f}, {"morphEnv", 0.4f}, {"lpgDecay", 0.6f}}},
    {"Swarm Drone", {{"engine", 16}, {"harmonics", 0.4f}, {"timbre", 0.6f}, {"morph", 0.7f}, {"ampMode", 1},
                     {"attack", 1.5f}, {"sustain", 1.0f}, {"release", 3.0f}}},
    {"Plucked String", {{"engine", 19}, {"harmonics", 0.3f}, {"timbre", 0.6f}, {"morph", 0.5f}}},
    {"Modal Bells", {{"engine", 20}, {"harmonics", 0.6f}, {"timbre", 0.5f}, {"morph", 0.7f}}},
    {"Kick", {{"engine", 21}, {"voices", 1}, {"harmonics", 0.5f}, {"timbre", 0.4f}, {"morph", 0.5f}, {"frequency", -12}}},
    {"Snare", {{"engine", 22}, {"harmonics", 0.5f}, {"timbre", 0.5f}, {"morph", 0.4f}}},
    {"Hats", {{"engine", 23}, {"harmonics", 0.5f}, {"timbre", 0.6f}, {"morph", 0.3f}}},
  };
  return list;
}

}  // namespace

juce::AudioProcessorValueTreeState::ParameterLayout OrguProcessor::createLayout() {
  using namespace juce;
  AudioProcessorValueTreeState::ParameterLayout layout;
  auto pct = AudioParameterFloatAttributes().withStringFromValueFunction(percent);
  auto bip = AudioParameterFloatAttributes().withStringFromValueFunction(bipolar);
  auto sec = AudioParameterFloatAttributes().withStringFromValueFunction(seconds);
  auto add = [&](std::unique_ptr<RangedAudioParameter> p) { layout.add(std::move(p)); };
  int v = 1;
  auto pid = [&](const String& id) { return ParameterID(id, v); };

  StringArray engines;
  for (int i = 0; i < orgu::kNumEngines; ++i) engines.add(orgu::engineName(i));
  add(std::make_unique<AudioParameterChoice>(pid(ids::engine), "Engine", engines, 13));
  add(std::make_unique<AudioParameterFloat>(pid(ids::frequency), "Frequency", NRange(-36.0f, 36.0f, 0.01f), 0.0f,
      AudioParameterFloatAttributes().withStringFromValueFunction([](float x, int) {
        return (x > 0.005f ? "+" : "") + String(x, 1) + " st"; })));
  add(std::make_unique<AudioParameterFloat>(pid(ids::harmonics), "Harmonics", NRange(0, 1), 0.5f, pct));
  add(std::make_unique<AudioParameterFloat>(pid(ids::timbre), "Timbre", NRange(0, 1), 0.5f, pct));
  add(std::make_unique<AudioParameterFloat>(pid(ids::morph), "Morph", NRange(0, 1), 0.5f, pct));
  add(std::make_unique<AudioParameterFloat>(pid(ids::fmEnv), "FM Env Amount", NRange(-1, 1), 0.0f, bip));
  add(std::make_unique<AudioParameterFloat>(pid(ids::timbreEnv), "Timbre Env Amount", NRange(-1, 1), 0.0f, bip));
  add(std::make_unique<AudioParameterFloat>(pid(ids::morphEnv), "Morph Env Amount", NRange(-1, 1), 0.0f, bip));
  add(std::make_unique<AudioParameterFloat>(pid(ids::lpgDecay), "LPG Decay", NRange(0, 1), 0.5f, pct));
  add(std::make_unique<AudioParameterFloat>(pid(ids::lpgColor), "LPG Color", NRange(0, 1), 0.5f, pct));
  add(std::make_unique<AudioParameterFloat>(pid(ids::outAux), "Out / Aux", NRange(0, 1), 0.0f, pct));
  add(std::make_unique<AudioParameterBool>(pid(ids::stereo), "Stereo Out/Aux", false));
  add(std::make_unique<AudioParameterFloat>(pid(ids::level), "Level", NRange(-48.0f, 6.0f, 0.1f), -6.0f,
      AudioParameterFloatAttributes().withStringFromValueFunction([](float x, int) { return String(x, 1) + " dB"; })));
  add(std::make_unique<AudioParameterChoice>(pid(ids::ampMode), "Amp Mode", StringArray{"LPG", "ADSR"}, 0));
  add(std::make_unique<AudioParameterFloat>(pid(ids::attack), "Attack", skewed(0.001f, 10.0f, 0.3f), 0.005f, sec));
  add(std::make_unique<AudioParameterFloat>(pid(ids::decay), "Decay", skewed(0.001f, 10.0f, 0.6f), 0.4f, sec));
  add(std::make_unique<AudioParameterFloat>(pid(ids::sustain), "Sustain", NRange(0, 1), 0.8f, pct));
  add(std::make_unique<AudioParameterFloat>(pid(ids::release), "Release", skewed(0.001f, 10.0f, 0.6f), 0.4f, sec));
  add(std::make_unique<AudioParameterInt>(pid(ids::voices), "Voices", 1, orgu::kMaxVoices, 8));
  add(std::make_unique<AudioParameterFloat>(pid(ids::glide), "Glide", skewed(0.0f, 2.0f, 0.2f), 0.0f, sec));
  add(std::make_unique<AudioParameterInt>(pid(ids::bendRange), "Bend NRange", 0, 24, 2,
      AudioParameterIntAttributes().withStringFromValueFunction([](int x, int) {
        return String::fromUTF8("\xc2\xb1") + String(x) + " st"; })));
  StringArray shapes;
  for (int i = 0; i < orgu::kNumLfoShapes; ++i) shapes.add(orgu::lfoShapeName(i));
  auto hz = AudioParameterFloatAttributes().withStringFromValueFunction([](float x, int) {
    return x < 1.0f ? String(x, 2) + " Hz" : String(x, 1) + " Hz"; });
  add(std::make_unique<AudioParameterFloat>(pid(ids::lfo1Rate), "LFO 1 Rate", skewed(0.01f, 30.0f, 2.0f), 1.0f, hz));
  add(std::make_unique<AudioParameterChoice>(pid(ids::lfo1Shape), "LFO 1 Shape", shapes, 0));
  add(std::make_unique<AudioParameterFloat>(pid(ids::lfo2Rate), "LFO 2 Rate", skewed(0.01f, 30.0f, 2.0f), 0.25f, hz));
  add(std::make_unique<AudioParameterChoice>(pid(ids::lfo2Shape), "LFO 2 Shape", shapes, 1));
  add(std::make_unique<AudioParameterFloat>(pid(ids::menvAttack), "Mod Env Attack", skewed(0.001f, 5.0f, 0.2f), 0.01f, sec));
  add(std::make_unique<AudioParameterFloat>(pid(ids::menvDecay), "Mod Env Decay", skewed(0.01f, 10.0f, 0.8f), 0.5f, sec));

  StringArray srcs, dsts;
  for (int i = 0; i < orgu::kNumSources; ++i) srcs.add(orgu::modSourceName(i));
  for (int i = 0; i < orgu::kNumDests; ++i) dsts.add(orgu::modDestName(i));
  const int defSrc[orgu::kNumSlots] = {orgu::SrcVelocity, orgu::SrcModWheel, orgu::SrcLfo1, orgu::SrcModEnv};
  const int defDst[orgu::kNumSlots] = {orgu::DstLevel, orgu::DstTimbre, orgu::DstFrequency, orgu::DstMorph};
  const float defAmt[orgu::kNumSlots] = {0.7f, 0.5f, 0.0f, 0.0f};
  for (int i = 0; i < orgu::kNumSlots; ++i) {
    const String n = "Mod " + String(i + 1);
    add(std::make_unique<AudioParameterChoice>(pid(ids::slotSrc(i)), n + " Source", srcs, defSrc[i]));
    add(std::make_unique<AudioParameterChoice>(pid(ids::slotDst(i)), n + " Destination", dsts, defDst[i]));
    add(std::make_unique<AudioParameterFloat>(pid(ids::slotAmt(i)), n + " Amount", NRange(-1.0f, 1.0f, 0.001f), defAmt[i], bip));
  }
  return layout;
}

OrguProcessor::OrguProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "ORGU", createLayout()) {
  auto raw = [this](const juce::String& id) { return Raw{apvts.getRawParameterValue(id)}; };
  engine = raw(ids::engine);
  frequency = raw(ids::frequency);
  harmonics = raw(ids::harmonics);
  timbre = raw(ids::timbre);
  morph = raw(ids::morph);
  fmEnv = raw(ids::fmEnv);
  timbreEnv = raw(ids::timbreEnv);
  morphEnv = raw(ids::morphEnv);
  lpgDecay = raw(ids::lpgDecay);
  lpgColor = raw(ids::lpgColor);
  outAux = raw(ids::outAux);
  stereo = raw(ids::stereo);
  level = raw(ids::level);
  ampMode = raw(ids::ampMode);
  attack = raw(ids::attack);
  decay = raw(ids::decay);
  sustain = raw(ids::sustain);
  release = raw(ids::release);
  voices = raw(ids::voices);
  glide = raw(ids::glide);
  bendRange = raw(ids::bendRange);
  lfo1Rate = raw(ids::lfo1Rate);
  lfo1Shape = raw(ids::lfo1Shape);
  lfo2Rate = raw(ids::lfo2Rate);
  lfo2Shape = raw(ids::lfo2Shape);
  menvAttack = raw(ids::menvAttack);
  menvDecay = raw(ids::menvDecay);
  for (int i = 0; i < orgu::kNumSlots; ++i) {
    slotSrc[i] = raw(ids::slotSrc(i));
    slotDst[i] = raw(ids::slotDst(i));
    slotAmt[i] = raw(ids::slotAmt(i));
  }
  apvts.state.setProperty("presetName", presetName, nullptr);
}

bool OrguProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  const auto out = layouts.getMainOutputChannelSet();
  return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void OrguProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  scratch.resize(static_cast<size_t>(juce::jmax(samplesPerBlock, 4096)));
  synth_.setSampleRate(sampleRate);
  synth_.reset();
}

void OrguProcessor::pullParams() {
  orgu::Params& p = synth_.params;
  p.engine = juce::roundToInt(engine.get());
  p.frequency = frequency.get();
  p.harmonics = harmonics.get();
  p.timbre = timbre.get();
  p.morph = morph.get();
  p.fmEnv = fmEnv.get();
  p.timbreEnv = timbreEnv.get();
  p.morphEnv = morphEnv.get();
  p.lpgDecay = lpgDecay.get();
  p.lpgColor = lpgColor.get();
  p.outAux = outAux.get();
  p.stereo = stereo.get() > 0.5f;
  p.levelDb = level.get();
  p.ampMode = juce::roundToInt(ampMode.get());
  p.attack = attack.get();
  p.decay = decay.get();
  p.sustain = sustain.get();
  p.release = release.get();
  p.voices = juce::roundToInt(voices.get());
  p.glide = glide.get();
  p.bendRange = bendRange.get();
  p.lfo[0].rateHz = lfo1Rate.get();
  p.lfo[0].shape = juce::roundToInt(lfo1Shape.get());
  p.lfo[1].rateHz = lfo2Rate.get();
  p.lfo[1].shape = juce::roundToInt(lfo2Shape.get());
  p.modEnvAttack = menvAttack.get();
  p.modEnvDecay = menvDecay.get();
  for (int i = 0; i < orgu::kNumSlots; ++i) {
    p.slots[i].source = juce::roundToInt(slotSrc[i].get());
    p.slots[i].dest = juce::roundToInt(slotDst[i].get());
    p.slots[i].amount = slotAmt[i].get();
  }
}

void OrguProcessor::handleMidi(const juce::MidiMessage& m) {
  if (m.isNoteOn()) synth_.noteOn(m.getNoteNumber(), m.getFloatVelocity());
  else if (m.isNoteOff()) synth_.noteOff(m.getNoteNumber());
  else if (m.isPitchWheel()) synth_.pitchBend((m.getPitchWheelValue() - 8192) / 8192.0f);
  else if (m.isController()) {
    const int cc = m.getControllerNumber();
    if (cc == 1) synth_.modWheel(m.getControllerValue() / 127.0f);
    else if (cc == 64) synth_.sustainPedal(m.getControllerValue() >= 64);
    else if (cc == 120 || cc == 123) synth_.allNotesOff();
  } else if (m.isChannelPressure()) synth_.aftertouch(m.getChannelPressureValue() / 127.0f);
  else if (m.isAftertouch()) synth_.aftertouch(m.getAfterTouchValue() / 127.0f);
  else if (m.isAllNotesOff() || m.isAllSoundOff()) synth_.allNotesOff();
}

void OrguProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
  juce::ScopedNoDenormals noDenormals;
  pullParams();
  const int n = buffer.getNumSamples();
  float* l = buffer.getWritePointer(0);
  float* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
  std::vector<float>& tmp = scratch;
  if (r == nullptr) {
    if (tmp.size() < static_cast<size_t>(n)) tmp.resize(static_cast<size_t>(n));
    r = tmp.data();
  }
  int pos = 0;
  for (const auto meta : midi) {
    const int at = juce::jlimit(0, n, meta.samplePosition);
    if (at > pos) {
      synth_.process(l + pos, r + pos, at - pos);
      pos = at;
    }
    handleMidi(meta.getMessage());
  }
  if (pos < n) synth_.process(l + pos, r + pos, n - pos);
  if (buffer.getNumChannels() == 1)
    for (int i = 0; i < n; ++i) l[i] = 0.5f * (l[i] + r[i]);
  for (int c = 2; c < buffer.getNumChannels(); ++c) buffer.clear(c, 0, n);
  midi.clear();
}

juce::AudioProcessorEditor* OrguProcessor::createEditor() { return new OrguEditor(*this); }

// ------------------------------------------------------------------ presets & state

int OrguProcessor::getNumPrograms() { return static_cast<int>(factoryPresets().size()); }

const juce::String OrguProcessor::getProgramName(int index) {
  const auto& list = factoryPresets();
  return juce::isPositiveAndBelow(index, static_cast<int>(list.size())) ? juce::String(list[index].name) : juce::String();
}

void OrguProcessor::setCurrentProgram(int index) { loadFactoryPreset(index); }

void OrguProcessor::loadFactoryPreset(int index) {
  const auto& list = factoryPresets();
  if (!juce::isPositiveAndBelow(index, static_cast<int>(list.size()))) return;
  for (auto* param : getParameters())
    if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(param))
      ranged->setValueNotifyingHost(ranged->getDefaultValue());
  for (const auto& [id, value] : list[index].values)
    if (auto* ranged = apvts.getParameter(id))
      ranged->setValueNotifyingHost(ranged->convertTo0to1(value));
  currentPreset = index;
  presetName = list[index].name;
  apvts.state.setProperty("presetName", presetName, nullptr);
}

juce::File OrguProcessor::presetFolder() {
  auto dir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Orgu Presets");
  dir.createDirectory();
  return dir;
}

bool OrguProcessor::savePresetFile(const juce::File& f) {
  presetName = f.getFileNameWithoutExtension();
  apvts.state.setProperty("presetName", presetName, nullptr);
  auto xml = apvts.copyState().createXml();
  return xml != nullptr && xml->writeTo(f);
}

bool OrguProcessor::loadPresetFile(const juce::File& f) {
  auto xml = juce::XmlDocument::parse(f);
  if (xml == nullptr || !xml->hasTagName(apvts.state.getType())) return false;
  apvts.replaceState(juce::ValueTree::fromXml(*xml));
  presetName = f.getFileNameWithoutExtension();
  apvts.state.setProperty("presetName", presetName, nullptr);
  currentPreset = -1;
  return true;
}

void OrguProcessor::getStateInformation(juce::MemoryBlock& destData) {
  apvts.state.setProperty("presetName", presetName, nullptr);
  apvts.state.setProperty("presetIndex", currentPreset, nullptr);
  if (auto xml = apvts.copyState().createXml()) copyXmlToBinary(*xml, destData);
}

void OrguProcessor::setStateInformation(const void* data, int sizeInBytes) {
  if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
    if (xml->hasTagName(apvts.state.getType())) {
      apvts.replaceState(juce::ValueTree::fromXml(*xml));
      presetName = apvts.state.getProperty("presetName", "Init").toString();
      currentPreset = apvts.state.getProperty("presetIndex", 0);
    }
  }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OrguProcessor(); }
