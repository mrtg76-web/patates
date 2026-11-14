#include "PluginEditor.h"

namespace ui {

juce::Font font(float size, bool bold) {
  return juce::Font(juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain));
}

static juce::Colour bankColour(int engine) {
  return engine < 8 ? col::bankNew : (engine < 16 ? col::bankPitched : col::bankNoise);
}

// ------------------------------------------------------------------ look and feel

LookAndFeel::LookAndFeel() {
  setColour(juce::ComboBox::backgroundColourId, col::control);
  setColour(juce::ComboBox::outlineColourId, col::controlLine);
  setColour(juce::ComboBox::textColourId, col::text);
  setColour(juce::ComboBox::arrowColourId, col::dim);
  setColour(juce::PopupMenu::backgroundColourId, col::panel);
  setColour(juce::PopupMenu::textColourId, col::text);
  setColour(juce::PopupMenu::highlightedBackgroundColourId, col::accent);
  setColour(juce::PopupMenu::highlightedTextColourId, col::bg);
  setColour(juce::PopupMenu::headerTextColourId, col::dim);
  setColour(juce::TextButton::buttonColourId, col::control);
  setColour(juce::TextButton::textColourOffId, col::text);
  setColour(juce::TextButton::textColourOnId, col::text);
  setColour(juce::Slider::textBoxTextColourId, col::text);
  setColour(juce::BubbleComponent::backgroundColourId, col::control);
  setColour(juce::BubbleComponent::outlineColourId, col::controlLine);
  setColour(juce::TooltipWindow::backgroundColourId, col::control);
  setColour(juce::TooltipWindow::textColourId, col::text);
}

void LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float start,
                                   float end, juce::Slider& s) {
  const float size = static_cast<float>(juce::jmin(w, h));
  const auto c = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y), static_cast<float>(w),
                                        static_cast<float>(h)).getCentre();
  const float thick = juce::jmax(3.0f, size * 0.1f);
  const float r = size * 0.5f - thick * 0.5f - 1.0f;
  const bool bipolar = s.getProperties()["bipolar"];
  const float angle = start + pos * (end - start);

  juce::Path track;
  track.addCentredArc(c.x, c.y, r, r, 0.0f, start, end, true);
  g.setColour(col::border);
  g.strokePath(track, juce::PathStrokeType(thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

  const float from = bipolar ? (start + end) * 0.5f : start;
  if (std::abs(angle - from) > 0.001f) {
    juce::Path value;
    value.addCentredArc(c.x, c.y, r, r, 0.0f, juce::jmin(from, angle), juce::jmax(from, angle), true);
    g.setColour(s.isEnabled() ? col::accent : col::dim);
    g.strokePath(value, juce::PathStrokeType(thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
  }
  // pointer dot on the inner edge
  const float inner = r - thick * 1.4f;
  g.setColour(col::text.withAlpha(s.isEnabled() ? 0.9f : 0.4f));
  const auto dot = c.getPointOnCircumference(inner, angle);
  const float d = juce::jmax(3.0f, size * 0.06f);
  g.fillEllipse(dot.x - d * 0.5f, dot.y - d * 0.5f, d, d);

  if (s.getProperties()["showValue"]) {
    g.setColour(col::text);
    g.setFont(font(size * 0.19f, true));
    g.drawFittedText(s.getTextFromValue(s.getValue()), juce::Rectangle<int>(x, y, w, h).reduced(static_cast<int>(thick * 2)),
                     juce::Justification::centred, 1);
  }
}

void LookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                   juce::Slider::SliderStyle, juce::Slider& s) {
  const float cy = y + h * 0.5f;
  const auto track = juce::Rectangle<float>(static_cast<float>(x), cy - 2.0f, static_cast<float>(w), 4.0f);
  g.setColour(col::border);
  g.fillRoundedRectangle(track, 2.0f);
  const bool bipolar = s.getProperties()["bipolar"];
  const float from = bipolar ? x + w * 0.5f : static_cast<float>(x);
  g.setColour(col::accent);
  g.fillRoundedRectangle(juce::Rectangle<float>(juce::jmin(from, pos), cy - 2.0f, std::abs(pos - from), 4.0f), 2.0f);
  g.setColour(col::text);
  g.fillEllipse(pos - 6.0f, cy - 6.0f, 12.0f, 12.0f);
}

void LookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) {
  const auto r = juce::Rectangle<float>(0, 0, static_cast<float>(w), static_cast<float>(h)).reduced(0.5f);
  g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
  g.fillRoundedRectangle(r, 6.0f);
  g.setColour(box.findColour(juce::ComboBox::outlineColourId));
  g.drawRoundedRectangle(r, 6.0f, 1.0f);
  juce::Path arrow;
  const float ax = w - 12.0f, ay = h * 0.5f;
  arrow.addTriangle(ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
  g.setColour(col::dim);
  g.fillPath(arrow);
}

void LookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label) {
  label.setBounds(6, 0, box.getWidth() - 22, box.getHeight());
  label.setFont(getComboBoxFont(box));
}

void LookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over,
                                       bool down) {
  const auto r = b.getLocalBounds().toFloat().reduced(0.5f);
  g.setColour(down ? col::controlLine : (over ? col::control.brighter(0.1f) : col::control));
  g.fillRoundedRectangle(r, r.getHeight() * 0.5f);
  g.setColour(col::controlLine);
  g.drawRoundedRectangle(r, r.getHeight() * 0.5f, 1.0f);
}

// ------------------------------------------------------------------ knob

Knob::Knob(juce::AudioProcessorValueTreeState& state, const juce::String& id, const juce::String& text, bool isBig,
           bool bipolar)
    : label(text), big(isBig) {
  slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
  slider.setMouseDragSensitivity(big ? 260 : 200);
  slider.setVelocityModeParameters(1.0, 1, 0.0, false, juce::ModifierKeys::altModifier);
  slider.getProperties().set("bipolar", bipolar);
  slider.getProperties().set("showValue", big);
  addAndMakeVisible(slider);
  attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state, id, slider);
  if (auto* p = state.getParameter(id)) {
    slider.setDoubleClickReturnValue(true, p->convertFrom0to1(p->getDefaultValue()));
    slider.setTooltip(p->getName(64) + U("  ·  double-click: reset, Alt-drag: fine"));
  }
  slider.onValueChange = [this] { repaint(); };
}

void Knob::resized() {
  auto r = getLocalBounds();
  const int textH = big ? 18 : 28;
  const int size = juce::jmin(r.getWidth(), r.getHeight() - textH);
  slider.setBounds(r.removeFromTop(size).withSizeKeepingCentre(size, size));
}

void Knob::paint(juce::Graphics& g) {
  auto r = getLocalBounds();
  r.removeFromTop(slider.getBottom() + 4);
  if (big) {
    g.setColour(col::soft);
    g.setFont(font(11.5f, true));
    g.drawText(label.toUpperCase(), r, juce::Justification::centredTop);
  } else {
    g.setColour(col::dim);
    g.setFont(font(11.0f));
    g.drawText(label, r.removeFromTop(13), juce::Justification::centred);
    g.setColour(col::text);
    g.setFont(font(11.0f, true));
    g.drawText(slider.getTextFromValue(slider.getValue()), r.removeFromTop(13), juce::Justification::centred);
  }
}

// ------------------------------------------------------------------ segments

Segments::Segments(juce::AudioProcessorValueTreeState& state, const juce::String& id,
                   std::vector<std::pair<juce::String, float>> opts)
    : options(std::move(opts)) {
  if (auto* p = state.getParameter(id)) {
    attachment = std::make_unique<juce::ParameterAttachment>(*p, [this](float v) {
      current = v;
      repaint();
    });
    attachment->sendInitialUpdate();
  }
}

int Segments::indexAt(juce::Point<float> p) const {
  const float w = getWidth() / static_cast<float>(options.size());
  return juce::jlimit(0, static_cast<int>(options.size()) - 1, static_cast<int>(p.x / w));
}

void Segments::mouseDown(const juce::MouseEvent& e) {
  if (attachment) attachment->setValueAsCompleteGesture(options[static_cast<size_t>(indexAt(e.position))].second);
}

void Segments::paint(juce::Graphics& g) {
  auto r = getLocalBounds().toFloat();
  g.setColour(col::bg);
  g.fillRoundedRectangle(r, 7.0f);
  g.setColour(col::controlLine);
  g.drawRoundedRectangle(r.reduced(0.5f), 7.0f, 1.0f);
  const float w = (r.getWidth() - 4.0f) / static_cast<float>(options.size());
  for (size_t i = 0; i < options.size(); ++i) {
    auto cell = juce::Rectangle<float>(2.0f + w * static_cast<float>(i), 2.0f, w, r.getHeight() - 4.0f);
    const bool on = std::abs(options[i].second - current) < 0.01f;
    if (on) {
      g.setColour(col::accent);
      g.fillRoundedRectangle(cell.reduced(1.0f), 5.0f);
    }
    g.setColour(on ? col::bg : col::soft);
    g.setFont(font(12.0f, true));
    g.drawText(options[i].first, cell, juce::Justification::centred);
  }
}

// ------------------------------------------------------------------ engine list

EngineList::EngineList(juce::AudioProcessorValueTreeState& state) {
  if (auto* p = state.getParameter(ids::engine)) {
    attachment = std::make_unique<juce::ParameterAttachment>(*p, [this](float v) {
      current = juce::roundToInt(v);
      repaint();
      if (onChange) onChange(current);
    });
    attachment->sendInitialUpdate();
  }
}

static constexpr float kListTop = 58.0f, kGroupH = 24.0f, kRowH = 21.0f;

juce::Rectangle<float> EngineList::rowBounds(int e) const {
  const int group = e / 8;
  const float y = kListTop + group * (kGroupH + 8 * kRowH + 6.0f) + kGroupH + (e % 8) * kRowH;
  return {10.0f, y, getWidth() - 20.0f, kRowH - 2.0f};
}

int EngineList::engineAt(juce::Point<float> p) const {
  for (int e = 0; e < orgu::kNumEngines; ++e)
    if (rowBounds(e).expanded(0.0f, 1.0f).contains(p)) return e;
  return -1;
}

void EngineList::mouseDown(const juce::MouseEvent& e) {
  const int idx = engineAt(e.position);
  if (idx >= 0 && attachment) attachment->setValueAsCompleteGesture(static_cast<float>(idx));
}

void EngineList::mouseMove(const juce::MouseEvent& e) {
  const int h = engineAt(e.position);
  if (h != hover) {
    hover = h;
    setMouseCursor(h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    repaint();
  }
}

void EngineList::mouseExit(const juce::MouseEvent&) {
  hover = -1;
  repaint();
}

void EngineList::paint(juce::Graphics& g) {
  g.fillAll(col::sidebar);
  g.setColour(col::border);
  g.fillRect(getWidth() - 1, 0, 1, getHeight());

  g.setColour(col::text);
  g.setFont(font(21.0f, true));
  g.drawText(U("ÖRGÜ"), 18, 14, 90, 28, juce::Justification::centredLeft);
  g.setColour(col::dim);
  g.setFont(font(11.0f));
  g.drawText("24 engines", 102, 20, 90, 20, juce::Justification::centredLeft);

  const char* groups[3] = {"NEW", "PITCHED", "NOISE / PERCUSSION"};
  for (int gi = 0; gi < 3; ++gi) {
    const auto first = rowBounds(gi * 8);
    g.setColour(col::dim);
    g.setFont(font(10.5f, true));
    g.drawText(groups[gi], 18, static_cast<int>(first.getY() - kGroupH + 4), 180, 16, juce::Justification::centredLeft);
  }
  for (int e = 0; e < orgu::kNumEngines; ++e) {
    auto r = rowBounds(e);
    const bool on = e == current;
    if (on) {
      g.setColour(col::accent.withAlpha(0.16f));
      g.fillRoundedRectangle(r, 4.0f);
    } else if (e == hover) {
      g.setColour(col::tile);
      g.fillRoundedRectangle(r, 4.0f);
    }
    g.setColour(on ? col::accent : bankColour(e).withAlpha(0.55f));
    g.fillRoundedRectangle(r.getX(), r.getY() + 3.0f, on ? 3.0f : 2.0f, r.getHeight() - 6.0f, 1.0f);
    g.setColour(on ? col::text : col::soft);
    g.setFont(font(12.0f, on));
    g.drawText(orgu::engineName(e), r.withTrimmedLeft(12.0f), juce::Justification::centredLeft);
    g.setColour(col::dim.withAlpha(0.7f));
    g.setFont(font(10.0f));
    g.drawText(juce::String(e + 1), r.withTrimmedRight(6.0f), juce::Justification::centredRight);
  }
}

// ------------------------------------------------------------------ scope

Scope::Scope(OrguProcessor& p) : proc(p), out(orgu::kScopeSize), aux(orgu::kScopeSize) {
  setInterceptsMouseClicks(false, false);
  startTimerHz(30);
}

void Scope::timerCallback() {
  const int n = 2048;
  proc.synth().copyScope(out.data(), aux.data(), n);
  // trigger on a rising zero crossing so periodic waves stand still
  start = n - kWindow;
  for (int i = n - kWindow; i > n - kWindow - 900 && i > 1; --i) {
    if (out[static_cast<size_t>(i - 1)] < 0.0f && out[static_cast<size_t>(i)] >= 0.0f) {
      start = i;
      break;
    }
  }
  float peak = 0.0f;
  for (int i = 0; i < kWindow; ++i)
    peak = juce::jmax(peak, std::abs(out[static_cast<size_t>(start + i)]), std::abs(aux[static_cast<size_t>(start + i)]));
  const float target = 0.85f / juce::jmax(peak, 0.08f);
  gain += (target - gain) * 0.2f;
  voices = proc.synth().activeVoiceCount();
  repaint();
}

void Scope::paint(juce::Graphics& g) {
  auto r = getLocalBounds().toFloat();
  g.setColour(col::panel);
  g.fillRoundedRectangle(r, 12.0f);
  g.setColour(col::border);
  g.drawRoundedRectangle(r.reduced(0.5f), 12.0f, 1.0f);

  const float mid = r.getCentreY();
  g.setColour(col::border);
  g.drawHorizontalLine(static_cast<int>(mid), r.getX() + 1.0f, r.getRight() - 1.0f);
  g.setColour(col::border.withAlpha(0.6f));
  for (int i = 1; i < 4; ++i) g.drawVerticalLine(static_cast<int>(r.getX() + r.getWidth() * i / 4.0f), r.getY() + 1, r.getBottom() - 1);

  const float amp = r.getHeight() * 0.45f;
  auto trace = [&](const std::vector<float>& v) {
    juce::Path p;
    for (int i = 0; i < kWindow; i += 2) {
      const float x = r.getX() + r.getWidth() * static_cast<float>(i) / (kWindow - 1);
      const float y = mid - juce::jlimit(-1.0f, 1.0f, v[static_cast<size_t>(start + i)] * gain) * amp;
      if (i == 0) p.startNewSubPath(x, y);
      else p.lineTo(x, y);
    }
    return p;
  };
  g.saveState();
  g.reduceClipRegion(getLocalBounds().reduced(2));
  g.setColour(col::aux.withAlpha(0.55f));
  g.strokePath(trace(aux), juce::PathStrokeType(1.5f));
  g.setColour(col::accent);
  g.strokePath(trace(out), juce::PathStrokeType(2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
  g.restoreState();

  g.setFont(font(11.0f, true));
  g.setColour(col::accent);
  g.drawText(U("● OUT"), static_cast<int>(r.getRight()) - 120, 10, 50, 16, juce::Justification::centredLeft);
  g.setColour(col::aux);
  g.drawText(U("● AUX"), static_cast<int>(r.getRight()) - 64, 10, 50, 16, juce::Justification::centredLeft);
  g.setColour(col::dim);
  g.setFont(font(11.0f));
  g.drawText(juce::String(voices) + (voices == 1 ? " voice" : " voices"), 14, 10, 120, 16,
             juce::Justification::centredLeft);
}

// ------------------------------------------------------------------ mod row

ModRow::ModRow(juce::AudioProcessorValueTreeState& state, int slot) {
  for (int i = 0; i < orgu::kNumSources; ++i) source.addItem(orgu::modSourceName(i), i + 1);
  for (int i = 0; i < orgu::kNumDests; ++i) dest.addItem(orgu::modDestName(i), i + 1);
  addAndMakeVisible(source);
  addAndMakeVisible(dest);
  amount.setSliderStyle(juce::Slider::LinearHorizontal);
  amount.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  amount.getProperties().set("bipolar", true);
  amount.setDoubleClickReturnValue(true, 0.0);
  amount.onValueChange = [this] { repaint(); };
  addAndMakeVisible(amount);
  srcAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(state, ids::slotSrc(slot), source);
  dstAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(state, ids::slotDst(slot), dest);
  amtAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state, ids::slotAmt(slot), amount);
}

void ModRow::resized() {
  auto r = getLocalBounds();
  source.setBounds(r.removeFromLeft(92));
  r.removeFromLeft(20);
  dest.setBounds(r.removeFromLeft(92));
  r.removeFromLeft(10);
  r.removeFromRight(44);
  amount.setBounds(r);
}

void ModRow::paint(juce::Graphics& g) {
  g.setColour(col::dim);
  g.setFont(font(12.0f));
  g.drawText(U("→"), 92, 0, 20, getHeight(), juce::Justification::centred);
  g.setColour(col::soft);
  g.setFont(font(11.5f, true));
  g.drawText(amount.getTextFromValue(amount.getValue()), getWidth() - 44, 0, 44, getHeight(),
             juce::Justification::centredRight);
}

ShapeBox::ShapeBox(juce::AudioProcessorValueTreeState& state, const juce::String& id) {
  for (int i = 0; i < orgu::kNumLfoShapes; ++i) box.addItem(orgu::lfoShapeName(i), i + 1);
  addAndMakeVisible(box);
  att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(state, id, box);
}

}  // namespace ui

// ------------------------------------------------------------------ editor

using ui::col::accent;

OrguEditor::OrguEditor(OrguProcessor& p)
    : AudioProcessorEditor(&p),
      proc(p),
      engines(p.apvts),
      scope(p),
      voices(p.apvts, ids::voices, {{"1", 1}, {"2", 2}, {"4", 4}, {"6", 6}, {"8", 8}}),
      ampMode(p.apvts, ids::ampMode, {{"LPG", 0}, {"ADSR", 1}}),
      stereo(p.apvts, ids::stereo, {{"MIX", 0}, {"L / R", 1}}),
      frequency(p.apvts, ids::frequency, "Frequency", true, true),
      harmonics(p.apvts, ids::harmonics, "Harmonics", true),
      timbre(p.apvts, ids::timbre, "Timbre", true),
      morph(p.apvts, ids::morph, "Morph", true),
      fmEnv(p.apvts, ids::fmEnv, "FM", false, true),
      timbreEnv(p.apvts, ids::timbreEnv, "Timbre", false, true),
      morphEnv(p.apvts, ids::morphEnv, "Morph", false, true),
      lpgDecay(p.apvts, ids::lpgDecay, "Decay", false),
      lpgColor(p.apvts, ids::lpgColor, "Color", false),
      attack(p.apvts, ids::attack, "Attack", false),
      decay(p.apvts, ids::decay, "Decay", false),
      sustain(p.apvts, ids::sustain, "Sustain", false),
      release(p.apvts, ids::release, "Release", false),
      glide(p.apvts, ids::glide, "Glide", false),
      bend(p.apvts, ids::bendRange, "Bend", false),
      lfo1Rate(p.apvts, ids::lfo1Rate, "LFO 1", false),
      lfo2Rate(p.apvts, ids::lfo2Rate, "LFO 2", false),
      menvAttack(p.apvts, ids::menvAttack, "Attack", false),
      menvDecay(p.apvts, ids::menvDecay, "Decay", false),
      outAux(p.apvts, ids::outAux, "Out / Aux", false),
      level(p.apvts, ids::level, "Level", false),
      lfo1Shape(p.apvts, ids::lfo1Shape),
      lfo2Shape(p.apvts, ids::lfo2Shape) {
  setLookAndFeel(&laf);
  for (juce::Component* c : std::initializer_list<juce::Component*>{
           &engines, &scope, &voices, &ampMode, &stereo, &frequency, &harmonics, &timbre, &morph, &fmEnv,
           &timbreEnv, &morphEnv, &lpgDecay, &lpgColor, &attack, &decay, &sustain, &release, &glide, &bend,
           &lfo1Rate, &lfo2Rate, &menvAttack, &menvDecay, &outAux, &level, &lfo1Shape, &lfo2Shape, &prevPreset,
           &nextPreset, &presetButton, &saveButton, &loadButton})
    addAndMakeVisible(c);
  for (int i = 0; i < orgu::kNumSlots; ++i) {
    modRows[static_cast<size_t>(i)] = std::make_unique<ui::ModRow>(p.apvts, i);
    addAndMakeVisible(*modRows[static_cast<size_t>(i)]);
  }
  engines.onChange = [this](int) { repaint(); };
  prevPreset.onClick = [this] { stepPreset(-1); };
  nextPreset.onClick = [this] { stepPreset(1); };
  presetButton.onClick = [this] { showPresetMenu(); };
  saveButton.onClick = [this] {
    chooser = std::make_unique<juce::FileChooser>("Save preset", OrguProcessor::presetFolder().getChildFile(proc.presetName + ".orgu"), "*.orgu");
    chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
                             juce::FileBrowserComponent::warnAboutOverwriting,
                         [this](const juce::FileChooser& fc) {
                           auto f = fc.getResult();
                           if (f != juce::File()) proc.savePresetFile(f.withFileExtension("orgu"));
                         });
  };
  loadButton.onClick = [this] {
    chooser = std::make_unique<juce::FileChooser>("Load preset", OrguProcessor::presetFolder(), "*.orgu");
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                         [this](const juce::FileChooser& fc) {
                           auto f = fc.getResult();
                           if (f.existsAsFile()) proc.loadPresetFile(f);
                         });
  };
  setSize(1000, 720);
  startTimerHz(10);
  timerCallback();
}

OrguEditor::~OrguEditor() { setLookAndFeel(nullptr); }

void OrguEditor::timerCallback() {
  if (shownPreset != proc.presetName) {
    shownPreset = proc.presetName;
    presetButton.setButtonText(shownPreset);
  }
  const int amp = juce::roundToInt(proc.apvts.getRawParameterValue(ids::ampMode)->load());
  if (amp != shownAmp) {
    shownAmp = amp;
    // in LPG mode the AMP envelope only shapes the Chiptune engine
    for (auto* k : {&attack, &decay, &sustain, &release}) k->setAlpha(amp == orgu::AmpAdsr ? 1.0f : 0.45f);
  }
}

void OrguEditor::stepPreset(int delta) {
  const int n = proc.getNumPrograms();
  const int cur = proc.currentPreset < 0 ? 0 : proc.currentPreset;
  proc.loadFactoryPreset((cur + delta + n) % n);
  timerCallback();
}

void OrguEditor::showPresetMenu() {
  juce::PopupMenu m;
  m.addSectionHeader("Factory");
  for (int i = 0; i < proc.getNumPrograms(); ++i)
    m.addItem(i + 1, proc.getProgramName(i), true, i == proc.currentPreset);
  juce::Array<juce::File> user = OrguProcessor::presetFolder().findChildFiles(juce::File::findFiles, false, "*.orgu");
  user.sort();
  if (!user.isEmpty()) {
    m.addSectionHeader("User");
    for (int i = 0; i < user.size(); ++i) m.addItem(1000 + i, user[i].getFileNameWithoutExtension());
  }
  m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetButton), [this, user](int r) {
    if (r >= 1000) proc.loadPresetFile(user[r - 1000]);
    else if (r > 0) proc.loadFactoryPreset(r - 1);
    timerCallback();
  });
}

void OrguEditor::card(juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title) const {
  g.setColour(ui::col::panel);
  g.fillRoundedRectangle(r.toFloat(), 10.0f);
  g.setColour(ui::col::border);
  g.drawRoundedRectangle(r.toFloat().reduced(0.5f), 10.0f, 1.0f);
  if (title.isNotEmpty()) {
    g.setColour(ui::col::dim);
    g.setFont(ui::font(10.5f, true));
    g.drawText(title, r.getX() + 12, r.getY() + 8, r.getWidth() - 24, 14, juce::Justification::centredLeft);
  }
}

void OrguEditor::paint(juce::Graphics& g) {
  g.fillAll(ui::col::bg);
  const int x0 = 232;
  const int e = engines.selected();
  g.setColour(ui::col::text);
  g.setFont(ui::font(26.0f, true));
  g.drawText(orgu::engineName(e), x0, 10, 420, 34, juce::Justification::centredLeft);
  g.setColour(ui::col::dim);
  g.setFont(ui::font(12.5f));
  g.drawText(juce::String("Engine ") + juce::String(e + 1) + " / 24  " + ui::U("·") + "  " + orgu::engineHint(e),
             x0, 46, 470, 18, juce::Justification::centredLeft);
  g.setFont(ui::font(10.5f, true));
  g.drawText("VOICES", getWidth() - 12 - 190 - 60, 50, 54, 16, juce::Justification::centredRight);

  for (int i = 0; i < 4; ++i) card(g, rMacro[i], {});
  card(g, rEnvMod, ui::U("ENVELOPE → MOD"));
  card(g, rLpg, "LOW PASS GATE");
  card(g, rAmp, "AMP");
  card(g, rPlay, "PLAY");
  card(g, rMod, "MODULATION");
  card(g, rLfo, ui::U("LFO · MOD ENV"));
  card(g, rOut, "OUTPUT");
}

void OrguEditor::resized() {
  engines.setBounds(0, 0, 220, getHeight());
  const int x0 = 232, W = getWidth() - x0 - 12;

  // header: presets + voices
  auto bar = juce::Rectangle<int>(getWidth() - 12 - 330, 12, 330, 28);
  loadButton.setBounds(bar.removeFromRight(56));
  bar.removeFromRight(6);
  saveButton.setBounds(bar.removeFromRight(56));
  bar.removeFromRight(10);
  nextPreset.setBounds(bar.removeFromRight(28));
  bar.removeFromRight(4);
  prevPreset.setBounds(bar.removeFromLeft(28));
  bar.removeFromLeft(4);
  presetButton.setBounds(bar);
  voices.setBounds(getWidth() - 12 - 190, 46, 190, 26);

  rScope = {x0, 84, W, 150};
  scope.setBounds(rScope);

  const int gap = 12;
  const int mw = (W - 3 * gap) / 4;
  ui::Knob* macros[4] = {&frequency, &harmonics, &timbre, &morph};
  for (int i = 0; i < 4; ++i) {
    rMacro[i] = {x0 + i * (mw + gap), 246, mw, 120};
    macros[i]->setBounds(rMacro[i].withSizeKeepingCentre(100, 100).translated(0, 0));
  }

  const int y3 = 378, h3 = 140;
  rEnvMod = {x0, y3, 170, h3};
  rLpg = {rEnvMod.getRight() + gap, y3, 120, h3};
  rAmp = {rLpg.getRight() + gap, y3, 250, h3};
  rPlay = {rAmp.getRight() + gap, y3, x0 + W - (rAmp.getRight() + gap), h3};

  auto placeRow = [](juce::Rectangle<int> card, std::initializer_list<juce::Component*> items, int knob = 44) {
    auto r = card.reduced(8, 0).withTrimmedTop(34).withHeight(knob + 30);
    const int w = r.getWidth() / static_cast<int>(items.size());
    for (auto* c : items) c->setBounds(r.removeFromLeft(w).withSizeKeepingCentre(juce::jmin(w, knob + 16), knob + 30));
  };
  placeRow(rEnvMod, {&fmEnv, &timbreEnv, &morphEnv});
  placeRow(rLpg, {&lpgDecay, &lpgColor});
  placeRow(rAmp, {&attack, &decay, &sustain, &release});
  ampMode.setBounds(rAmp.getRight() - 112, rAmp.getY() + 6, 104, 22);
  placeRow(rPlay, {&glide, &bend});

  const int y4 = 530, h4 = getHeight() - 12 - y4;
  rMod = {x0, y4, 356, h4};
  rLfo = {rMod.getRight() + gap, y4, 256, h4};
  rOut = {rLfo.getRight() + gap, y4, x0 + W - (rLfo.getRight() + gap), h4};
  for (int i = 0; i < orgu::kNumSlots; ++i)
    modRows[static_cast<size_t>(i)]->setBounds(rMod.getX() + 12, rMod.getY() + 34 + i * 33, rMod.getWidth() - 24, 26);

  auto lfo = rLfo.reduced(10, 0).withTrimmedTop(30);
  auto c1 = lfo.removeFromLeft(62);
  lfo.removeFromLeft(6);
  auto c2 = lfo.removeFromLeft(62);
  lfo.removeFromLeft(6);
  lfo1Rate.setBounds(c1.removeFromTop(74).withSizeKeepingCentre(60, 74));
  lfo1Shape.setBounds(c1.removeFromTop(30).withSizeKeepingCentre(62, 24));
  lfo2Rate.setBounds(c2.removeFromTop(74).withSizeKeepingCentre(60, 74));
  lfo2Shape.setBounds(c2.removeFromTop(30).withSizeKeepingCentre(62, 24));
  auto envRow = lfo.removeFromTop(74);
  menvAttack.setBounds(envRow.removeFromLeft(envRow.getWidth() / 2).withSizeKeepingCentre(56, 74));
  menvDecay.setBounds(envRow.withSizeKeepingCentre(56, 74));

  auto out = rOut.reduced(8, 0).withTrimmedTop(30);
  auto knobs = out.removeFromTop(74);
  outAux.setBounds(knobs.removeFromLeft(knobs.getWidth() / 2).withSizeKeepingCentre(54, 74));
  level.setBounds(knobs.withSizeKeepingCentre(54, 74));
  out.removeFromTop(8);
  stereo.setBounds(out.removeFromTop(24));
}
