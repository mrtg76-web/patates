#include "PluginEditor.h"

#include <cmath>

namespace ui {

namespace {
constexpr float kStart = juce::MathConstants<float>::pi * 1.25f;
constexpr float kEnd = juce::MathConstants<float>::pi * 2.75f;
}  // namespace

juce::Colour bankColour(int bank) { return bank == 0 ? col::green : (bank == 1 ? col::amber : col::red); }

juce::Font panelFont(float size, bool bold) {
  juce::Font f(juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain));
  f.setExtraKerningFactor(0.08f);
  return f;
}

juce::Font monoFont(float size) {
  return juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), size, juce::Font::plain));
}

// ------------------------------------------------------------------ look and feel

LookAndFeel::LookAndFeel() {
  setColour(juce::PopupMenu::backgroundColourId, col::buttonFace);
  setColour(juce::PopupMenu::textColourId, col::ink);
  setColour(juce::PopupMenu::headerTextColourId, col::inkSoft);
  setColour(juce::PopupMenu::highlightedBackgroundColourId, col::ink);
  setColour(juce::PopupMenu::highlightedTextColourId, col::pointer);
}

void LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end,
                                   juce::Slider& s) {
  auto* k = dynamic_cast<Knob*>(&s);
  const auto style = k ? k->style : Knob::Small;
  const bool redPointer = k && k->red;
  const float angle = start + pos * (end - start);

  auto area = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h);
  const float size = juce::jmin(area.getWidth(), area.getHeight());
  const float margin = style == Knob::Big ? size * 0.075f
                       : style == Knob::Tiny ? size * 0.06f
                                             : size * 0.12f;
  const float r = size * 0.5f - margin;
  const auto c = area.getCentre();

  const int ticks = (style == Knob::Big || style == Knob::Medium) ? 11 : (style == Knob::Small ? 1 : 0);
  g.setColour(col::inkSoft);
  for (int i = 0; i < ticks; ++i) {
    const float a = ticks == 1 ? 0.0f : start + (end - start) * (float)i / (float)(ticks - 1);
    const bool major = ticks == 1 || i == 0 || i == ticks - 1 || i == ticks / 2;
    const float r0 = r + margin * 0.35f, r1 = r + margin * (major ? 0.95f : 0.7f);
    g.drawLine({{c.x + r0 * std::sin(a), c.y - r0 * std::cos(a)}, {c.x + r1 * std::sin(a), c.y - r1 * std::cos(a)}},
               major ? 2.0f : 1.2f);
  }

  const float shadowOffset = style == Knob::Big ? r * 0.08f : r * 0.05f;
  g.setColour(col::knobShadow);
  g.fillEllipse(c.x - r, c.y - r + shadowOffset, 2 * r, 2 * r);

  const auto face = style == Knob::Big ? col::knob : col::knobSmall;
  g.setGradientFill(juce::ColourGradient(face.brighter(0.18f), c.x, c.y - r, face, c.x, c.y + r, false));
  g.fillEllipse(c.x - r, c.y - r, 2 * r, 2 * r);
  g.setColour(juce::Colours::black.withAlpha(0.35f));
  g.drawEllipse(c.x - r, c.y - r, 2 * r, 2 * r, 1.0f);

  const bool big = style == Knob::Big;
  const float pw = juce::jmax(2.5f, r * (big ? 0.075f : 0.13f));
  const float inset = r * (big ? 0.14f : 0.16f);
  const float len = r * (big ? 0.55f : 0.6f);
  juce::Path p;
  p.addRoundedRectangle(-pw * 0.5f, -r + inset, pw, len, pw * 0.5f);
  g.setColour(redPointer ? col::red : col::pointer);
  g.fillPath(p, juce::AffineTransform::rotation(angle).translated(c.x, c.y));
}

// Bipolar amount bar of the mod matrix.
void LookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                   juce::Slider::SliderStyle, juce::Slider&) {
  const float cy = (float)y + (float)h * 0.5f;
  const float mid = (float)x + (float)w * 0.5f;
  g.setColour(col::ledOff);
  g.fillRoundedRectangle((float)x, cy - 2.0f, (float)w, 4.0f, 2.0f);
  if (std::abs(pos - mid) > 0.5f) {
    g.setColour(col::red);
    g.fillRoundedRectangle(juce::jmin(pos, mid), cy - 2.0f, std::abs(pos - mid), 4.0f, 2.0f);
  }
  g.setColour(col::inkSoft);
  g.fillRect(mid - 1.0f, cy - 6.0f, 2.0f, 12.0f);
  g.setColour(col::ink);
  g.fillRoundedRectangle(pos - 3.0f, cy - 6.0f, 6.0f, 12.0f, 2.0f);
}

// ------------------------------------------------------------------ knob

Knob::Knob(juce::AudioProcessorValueTreeState& state, const juce::String& id, Style s, bool redPointer)
    : style(s), red(redPointer), paramId(id) {
  setSliderStyle(juce::Slider::RotaryVerticalDrag);
  setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  setRotaryParameters(kStart, kEnd, true);
  setMouseDragSensitivity(style == Big ? 260 : 200);
  setVelocityModeParameters(1.0, 1, 0.0, false, juce::ModifierKeys::altModifier);
  attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state, id, *this);
  if (auto* p = state.getParameter(id)) {
    setDoubleClickReturnValue(true, p->convertFrom0to1(p->getDefaultValue()));
    setTooltip(p->getName(64) + U("  ·  çift tık: sıfırla, Alt-sürükle: ince ayar"));
  }
  onValueChange = [this] {
    if (onTouch) onTouch(paramId);
  };
}

void Knob::mouseDown(const juce::MouseEvent& e) {
  if (onTouch) onTouch(paramId);
  juce::Slider::mouseDown(e);
}

// ------------------------------------------------------------------ buttons

PanelButton::PanelButton(juce::String t, Look l) : text(std::move(t)), look(l) {
  setMouseCursor(juce::MouseCursor::PointingHandCursor);
  if (look == Round) setTitle(text);
}

void PanelButton::paint(juce::Graphics& g) {
  auto b = getLocalBounds().toFloat().reduced(1.0f);
  if (look == Round) {
    const float d = juce::jmin(b.getWidth(), b.getHeight());
    auto r = b.withSizeKeepingCentre(d, d);
    g.setColour(col::knobShadow);
    g.fillEllipse(r.translated(0.0f, 2.0f).reduced(1.0f));
    g.setColour(pressed ? col::buttonFace.darker(0.15f) : col::buttonFace);
    g.fillEllipse(r.reduced(1.0f));
    g.setColour(col::ink);
    g.drawEllipse(r.reduced(2.0f), 2.0f);
    return;
  }
  const bool on = lit || pressed;
  if (on) {
    g.setColour(col::ink);
    g.fillRoundedRectangle(b, 4.0f);
  }
  g.setColour(on ? col::ink : col::badgeLine);
  g.drawRoundedRectangle(b, 4.0f, 1.0f);
  g.setColour(on ? col::pointer : col::inkMid);
  g.setFont(monoFont(12.0f));
  g.drawFittedText(text, b.toNearestInt().reduced(4, 0), juce::Justification::centred, 1);
}

void PanelButton::mouseDown(const juce::MouseEvent&) {
  pressed = true;
  repaint();
  if (onPress) onPress();
}

void PanelButton::mouseUp(const juce::MouseEvent&) {
  pressed = false;
  repaint();
}

// ------------------------------------------------------------------ segments

Segments::Segments(juce::AudioProcessorValueTreeState& state, const juce::String& id,
                   std::vector<std::pair<juce::String, float>> opts)
    : options(std::move(opts)) {
  setMouseCursor(juce::MouseCursor::PointingHandCursor);
  if (auto* p = state.getParameter(id)) {
    attachment = std::make_unique<juce::ParameterAttachment>(*p, [this](float v) {
      current = v;
      repaint();
    });
    attachment->sendInitialUpdate();
  }
}

void Segments::mouseDown(const juce::MouseEvent& e) {
  const float w = (float)getWidth() / (float)options.size();
  const int i = juce::jlimit(0, (int)options.size() - 1, (int)(e.position.x / w));
  if (attachment) attachment->setValueAsCompleteGesture(options[(size_t)i].second);
}

void Segments::paint(juce::Graphics& g) {
  auto b = getLocalBounds().toFloat().reduced(0.5f);
  const float w = b.getWidth() / (float)options.size();
  g.saveState();
  juce::Path clip;
  clip.addRoundedRectangle(b, 4.0f);
  g.reduceClipRegion(clip);
  for (size_t i = 0; i < options.size(); ++i) {
    auto cell = juce::Rectangle<float>(b.getX() + w * (float)i, b.getY(), w, b.getHeight());
    const bool on = std::abs(options[i].second - current) < 0.01f;
    if (on) {
      g.setColour(col::ink);
      g.fillRect(cell);
    }
    if (i > 0) {
      g.setColour(col::badgeLine);
      g.fillRect(cell.getX(), cell.getY(), 1.0f, cell.getHeight());
    }
    g.setColour(on ? col::pointer : col::inkMid);
    g.setFont(monoFont(12.0f));
    g.drawText(options[i].first, cell, juce::Justification::centred);
  }
  g.restoreState();
  g.setColour(col::badgeLine);
  g.drawRoundedRectangle(b, 4.0f, 1.0f);
}

// ------------------------------------------------------------------ choice badge

ChoiceBadge::ChoiceBadge(juce::AudioProcessorValueTreeState& state, const juce::String& id, juce::StringArray shown,
                         bool isLit)
    : names(std::move(shown)), lit(isLit) {
  setMouseCursor(juce::MouseCursor::PointingHandCursor);
  param = state.getParameter(id);
  if (param != nullptr) {
    attachment = std::make_unique<juce::ParameterAttachment>(*param, [this](float v) {
      current = juce::roundToInt(v);
      repaint();
    });
    attachment->sendInitialUpdate();
  }
}

void ChoiceBadge::paint(juce::Graphics& g) {
  auto b = getLocalBounds().toFloat().reduced(0.5f);
  if (lit) {
    g.setColour(col::ink);
    g.fillRoundedRectangle(b, 4.0f);
  }
  g.setColour(lit ? col::ink : col::badgeLine);
  g.drawRoundedRectangle(b, 4.0f, 1.0f);
  g.setColour(lit ? col::pointer : col::inkMid);
  g.setFont(monoFont(juce::jmin(12.0f, (float)getHeight() * 0.6f)));
  g.drawFittedText(names[current], getLocalBounds().reduced(4, 0), juce::Justification::centred, 1);
}

void ChoiceBadge::mouseDown(const juce::MouseEvent&) {
  if (param == nullptr) return;
  juce::PopupMenu m;
  for (int i = 0; i < names.size(); ++i) m.addItem(i + 1, names[i], true, i == current);
  m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [this](int r) {
    if (r > 0 && attachment) attachment->setValueAsCompleteGesture((float)(r - 1));
  });
}

// ------------------------------------------------------------------ scope

Scope::Scope(OrguProcessor& p) : proc(p), out(orgu::kScopeSize), aux(orgu::kScopeSize) {
  setInterceptsMouseClicks(false, false);
}

void Scope::update() {
  const int n = 2048;
  proc.synth().copyScope(out.data(), aux.data(), n);
  // trigger on a rising zero crossing so periodic waves stand still
  start = n - kWindow;
  for (int i = n - kWindow; i > n - kWindow - 900 && i > 1; --i) {
    if (out[(size_t)(i - 1)] < 0.0f && out[(size_t)i] >= 0.0f) {
      start = i;
      break;
    }
  }
  float pk = 0.0f, pkOut = 0.0f;
  for (int i = 0; i < kWindow; ++i) {
    pkOut = juce::jmax(pkOut, std::abs(out[(size_t)(start + i)]));
    pk = juce::jmax(pk, pkOut, std::abs(aux[(size_t)(start + i)]));
  }
  peak = pkOut;
  const float target = 0.85f / juce::jmax(pk, 0.08f);
  gain += (target - gain) * 0.2f;
  voices = proc.synth().activeVoiceCount();
  maxVoices = juce::roundToInt(proc.apvts.getRawParameterValue(ids::voices)->load());
  repaint();
}

void Scope::paint(juce::Graphics& g) {
  auto r = getLocalBounds().toFloat();
  g.setColour(col::lcd);
  g.fillRoundedRectangle(r, 6.0f);

  g.setColour(col::lcdGrid);
  for (int i = 1; i < 8; ++i) g.drawVerticalLine((int)(r.getWidth() * (float)i / 8.0f), 1.0f, r.getBottom() - 1.0f);
  const float mid = r.getCentreY();
  g.setColour(col::lcdGrid.brighter(0.15f));
  g.drawHorizontalLine((int)mid, 1.0f, r.getRight() - 1.0f);

  const float amp = r.getHeight() * 0.42f;
  auto trace = [&](const std::vector<float>& v) {
    juce::Path p;
    for (int i = 0; i < kWindow; i += 2) {
      const float x = r.getWidth() * (float)i / (float)(kWindow - 1);
      const float y = mid - juce::jlimit(-1.0f, 1.0f, v[(size_t)(start + i)] * gain) * amp;
      if (i == 0) p.startNewSubPath(x, y);
      else p.lineTo(x, y);
    }
    return p;
  };
  g.saveState();
  g.reduceClipRegion(getLocalBounds().reduced(2));
  g.setColour(col::aux.withAlpha(0.8f));
  g.strokePath(trace(aux), juce::PathStrokeType(1.5f));
  g.setColour(col::out);
  g.strokePath(trace(out), juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
  g.restoreState();

  // inner shadow along the top edge
  g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(0.55f), 0.0f, 0.0f,
                                         juce::Colours::transparentBlack, 0.0f, 8.0f, false));
  g.fillRect(r.withHeight(8.0f).reduced(3.0f, 0.0f));

  g.setFont(monoFont(11.0f));
  g.setColour(col::out);
  g.drawText("OUT", 12, 8, 32, 14, juce::Justification::centredLeft);
  g.setColour(col::aux);
  g.drawText("AUX", 48, 8, 32, 14, juce::Justification::centredLeft);
  g.setColour(col::lcdText);
  g.drawText(juce::String(voices) + " / " + juce::String(maxVoices) + U(" SES"), getWidth() - 132, 8, 120, 14,
             juce::Justification::centredRight);
}

}  // namespace ui

// ================================================================== editor

using namespace ui;

namespace {
// layout (base coordinates, 1200 x 720)
constexpr int kLx = 40, kLw = 470;            // left (Plaits) panel
constexpr int kDivX = 540;
constexpr int kRx = 572, kRw = 588;           // expansion panel
constexpr int kLedX = kLx + kLw / 2 - 6, kLedY = 122, kLedStep = 15, kLedSize = 10;
constexpr int kBankY = 238, kBankX0 = kLx + 120, kBankStep = 84;

const char* const kBankNames[3] = {"YEN\xc4\xb0", "PERDEL\xc4\xb0", "G\xc3\x9cR\xc3\x9cLT\xc3\x9c"};

juce::StringArray upperNames(const char* (*fn)(int), int n) {
  juce::StringArray a;
  for (int i = 0; i < n; ++i) a.add(juce::String(fn(i)).toUpperCase());
  return a;
}

juce::StringArray lfoShapeNames() {
  return {"SIN", juce::String::fromUTF8("\xc3\x9c\xc3\x87G"), "TST", "KARE", "S&H"};
}

juce::String shortEngine(int e) {
  juce::String s = orgu::engineShortName(e);
  if (e >= 2 && e <= 4) s += juce::String::fromUTF8("\xc2\xb7") + juce::String(e - 1);  // FM6·1..3
  return s;
}
}  // namespace

OrguEditor::OrguEditor(OrguProcessor& p)
    : AudioProcessorEditor(&p),
      proc(p),
      scope(p),
      frequency(p.apvts, ids::frequency, Knob::Big),
      harmonics(p.apvts, ids::harmonics, Knob::Big),
      timbre(p.apvts, ids::timbre, Knob::Medium),
      morph(p.apvts, ids::morph, Knob::Medium),
      fmEnv(p.apvts, ids::fmEnv, Knob::Small, true),
      timbreEnv(p.apvts, ids::timbreEnv, Knob::Small, true),
      morphEnv(p.apvts, ids::morphEnv, Knob::Small, true),
      lpgDecay(p.apvts, ids::lpgDecay, Knob::Tiny),
      lpgColor(p.apvts, ids::lpgColor, Knob::Tiny),
      ampMode(p.apvts, ids::ampMode, {{"LPG", 0}, {"ADSR", 1}}),
      voices(p.apvts, ids::voices, {{"1", 1}, {"2", 2}, {"4", 4}, {"6", 6}, {"8", 8}}),
      attack(p.apvts, ids::attack, Knob::Small),
      decay(p.apvts, ids::decay, Knob::Small),
      sustain(p.apvts, ids::sustain, Knob::Small),
      release(p.apvts, ids::release, Knob::Small),
      glide(p.apvts, ids::glide, Knob::Small),
      bend(p.apvts, ids::bendRange, Knob::Small),
      lfo1Rate(p.apvts, ids::lfo1Rate, Knob::Small),
      lfo2Rate(p.apvts, ids::lfo2Rate, Knob::Small),
      menvAttack(p.apvts, ids::menvAttack, Knob::Small),
      menvDecay(p.apvts, ids::menvDecay, Knob::Small),
      outAux(p.apvts, ids::outAux, Knob::Small),
      level(p.apvts, ids::level, Knob::Small),
      lfo1Shape(p.apvts, ids::lfo1Shape, lfoShapeNames(), true),
      lfo2Shape(p.apvts, ids::lfo2Shape, lfoShapeNames(), true) {
  setLookAndFeel(&laf);
  addAndMakeVisible(content);
  content.addAndMakeVisible(scope);

  for (Knob* k : {&frequency, &harmonics, &timbre, &morph, &fmEnv, &timbreEnv, &morphEnv, &lpgDecay, &lpgColor,
                  &attack, &decay, &sustain, &release, &glide, &bend, &lfo1Rate, &lfo2Rate, &menvAttack,
                  &menvDecay, &outAux, &level}) {
    content.addAndMakeVisible(*k);
    k->onTouch = [this](const juce::String& id) { showValue(id); };
  }
  for (juce::Component* c : std::initializer_list<juce::Component*>{
           &prevEngine, &nextEngine, &ampMode, &voices, &lfo1Shape, &lfo2Shape, &stereoButton, &prevPreset,
           &nextPreset, &presetButton, &saveButton, &loadButton})
    content.addAndMakeVisible(c);

  const auto srcNames = upperNames(orgu::modSourceName, orgu::kNumSources);
  const auto dstNames = upperNames(orgu::modDestName, orgu::kNumDests);
  for (int i = 0; i < orgu::kNumSlots; ++i) {
    modSrc.push_back(std::make_unique<ChoiceBadge>(p.apvts, ids::slotSrc(i), srcNames));
    modDst.push_back(std::make_unique<ChoiceBadge>(p.apvts, ids::slotDst(i), dstNames));
    auto s = std::make_unique<juce::Slider>(juce::Slider::LinearHorizontal, juce::Slider::NoTextBox);
    s->setDoubleClickReturnValue(true, 0.0);
    s->setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    modAmtAtt.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.apvts, ids::slotAmt(i), *s));
    const juce::String id = ids::slotAmt(i);
    s->onValueChange = [this, id] { showValue(id); };
    content.addAndMakeVisible(*modSrc.back());
    content.addAndMakeVisible(*modDst.back());
    content.addAndMakeVisible(*s);
    modAmt.push_back(std::move(s));
  }

  loadButton.text = U8("AÇ");
  prevEngine.onPress = [this] { selectEngine((engine() + orgu::kNumEngines - 1) % orgu::kNumEngines); };
  nextEngine.onPress = [this] { selectEngine((engine() + 1) % orgu::kNumEngines); };
  stereoButton.onPress = [this] {
    if (auto* prm = proc.apvts.getParameter(ids::stereo)) {
      prm->beginChangeGesture();
      prm->setValueNotifyingHost(prm->getValue() > 0.5f ? 0.0f : 1.0f);
      prm->endChangeGesture();
      showValue(ids::stereo);
    }
  };
  prevPreset.onPress = [this] { stepPreset(-1); };
  nextPreset.onPress = [this] { stepPreset(1); };
  presetButton.onPress = [this] { showPresetMenu(); };
  saveButton.onPress = [this] { savePreset(); };
  loadButton.onPress = [this] { loadPreset(); };

  constrainer = std::make_unique<juce::ComponentBoundsConstrainer>();
  constrainer->setFixedAspectRatio((double)kW / (double)kH);
  constrainer->setSizeLimits(kW * 6 / 10, kH * 6 / 10, kW * 2, kH * 2);
  setConstrainer(constrainer.get());
  setResizable(true, true);
  setSize(kW, kH);

  // ---------------------------------------------------------------- layout (base coordinates)
  // left panel
  prevEngine.setBounds(kLx + 70, 140, 52, 52);
  nextEngine.setBounds(kLx + kLw - 122, 140, 52, 52);
  frequency.setBounds(kLx + 50, 290, 140, 140);
  harmonics.setBounds(kLx + kLw - 190, 290, 140, 140);
  timbre.setBounds(kLx + 80, 456, 84, 84);
  morph.setBounds(kLx + kLw - 164, 456, 84, 84);
  Knob* envs[3] = {&fmEnv, &timbreEnv, &morphEnv};
  for (int i = 0; i < 3; ++i) envs[i]->setBounds(kLx + 70 + i * 130, 574, 48, 48);
  lpgDecay.setBounds(kLx + 20, 650, 40, 40);
  lpgColor.setBounds(kLx + 250, 650, 40, 40);

  // expansion panel
  scope.setBounds(kRx, 96, kRw, 150);
  ampMode.setBounds(kRx, 342, 100, 26);
  Knob* adsr[4] = {&attack, &decay, &sustain, &release};
  for (int i = 0; i < 4; ++i) adsr[i]->setBounds(kRx + i * 84 + 20, 378, 44, 44);
  voices.setBounds(kRx + 400, 342, 188, 26);
  glide.setBounds(kRx + 360 + 20, 378, 44, 44);
  bend.setBounds(kRx + 470 + 20, 378, 44, 44);

  lfo1Rate.setBounds(kRx, 496, 48, 48);
  lfo1Shape.setBounds(kRx + 56, 532, 60, 20);
  lfo2Rate.setBounds(kRx + 200, 496, 48, 48);
  lfo2Shape.setBounds(kRx + 256, 532, 60, 20);
  menvAttack.setBounds(kRx + 410 + 18, 496, 44, 44);
  menvDecay.setBounds(kRx + 500 + 18, 496, 44, 44);

  for (int i = 0; i < orgu::kNumSlots; ++i) {
    const int y = 594 + i * 24;
    modSrc[(size_t)i]->setBounds(kRx, y, 104, 20);
    modDst[(size_t)i]->setBounds(kRx + 128, y, 104, 20);
    modAmt[(size_t)i]->setBounds(kRx + 244, y, 128, 20);
  }
  outAux.setBounds(kRx + 430, 596, 48, 48);
  level.setBounds(kRx + 500, 596, 48, 48);
  stereoButton.setBounds(kRx + 556, 604, 32, 20);

  // header
  saveButton.setBounds(786, 32, 70, 28);
  loadButton.setBounds(862, 32, 58, 28);
  prevPreset.setBounds(940, 32, 28, 28);
  presetButton.setBounds(972, 32, 152, 28);
  nextPreset.setBounds(1128, 32, 32, 28);

  timerCallback();
  startTimerHz(30);
}

OrguEditor::~OrguEditor() {
  stopTimer();
  setLookAndFeel(nullptr);
}

int OrguEditor::engine() const {
  return juce::roundToInt(proc.apvts.getRawParameterValue(ids::engine)->load());
}

void OrguEditor::selectEngine(int e) {
  if (auto* prm = proc.apvts.getParameter(ids::engine)) {
    prm->beginChangeGesture();
    prm->setValueNotifyingHost(prm->convertTo0to1((float)juce::jlimit(0, orgu::kNumEngines - 1, e)));
    prm->endChangeGesture();
  }
  content.repaint();
}

void OrguEditor::showValue(const juce::String& id) {
  if (auto* prm = proc.apvts.getParameter(id)) {
    readout = prm->getName(64).toUpperCase() + "   " + prm->getCurrentValueAsText();
    readoutTime = juce::Time::getMillisecondCounter();
    content.repaint();
  }
}

void OrguEditor::paint(juce::Graphics& g) { g.fillAll(col::panel); }

void OrguEditor::resized() {
  content.setBounds(0, 0, kW, kH);
  content.setTransform(juce::AffineTransform::scale((float)getWidth() / (float)kW));
}

void OrguEditor::timerCallback() {
  scope.update();
  shownLevel = juce::jmax(scope.peak, shownLevel * 0.85f);

  if (presetButton.text != proc.presetName) {
    presetButton.text = proc.presetName;
    presetButton.repaint();
  }
  const bool st = proc.apvts.getRawParameterValue(ids::stereo)->load() > 0.5f;
  if (stereoButton.lit != st) {
    stereoButton.lit = st;
    stereoButton.repaint();
  }
  const int amp = juce::roundToInt(proc.apvts.getRawParameterValue(ids::ampMode)->load());
  if (amp != shownAmp) {
    shownAmp = amp;
    // in LPG mode the AMP envelope only shapes the Chiptune engine
    for (auto* k : {&attack, &decay, &sustain, &release}) k->setAlpha(amp == orgu::AmpAdsr ? 1.0f : 0.45f);
  }
  content.repaint();
}

void OrguEditor::stepPreset(int delta) {
  const int n = proc.getNumPrograms();
  const int cur = proc.currentPreset < 0 ? 0 : proc.currentPreset;
  proc.loadFactoryPreset((cur + delta + n) % n);
  timerCallback();
}

void OrguEditor::showPresetMenu() {
  juce::PopupMenu m;
  m.addSectionHeader(U8("FABRİKA"));
  for (int i = 0; i < proc.getNumPrograms(); ++i)
    m.addItem(i + 1, proc.getProgramName(i), true, i == proc.currentPreset);
  juce::Array<juce::File> user = OrguProcessor::presetFolder().findChildFiles(juce::File::findFiles, false, "*.orgu");
  user.sort();
  if (!user.isEmpty()) {
    m.addSectionHeader(U8("KULLANICI"));
    for (int i = 0; i < user.size(); ++i) m.addItem(1000 + i, user[i].getFileNameWithoutExtension());
  }
  m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetButton), [this, user](int r) {
    if (r >= 1000) proc.loadPresetFile(user[r - 1000]);
    else if (r > 0) proc.loadFactoryPreset(r - 1);
    timerCallback();
  });
}

void OrguEditor::savePreset() {
  chooser = std::make_unique<juce::FileChooser>(U8("Preset kaydet"),
                                                OrguProcessor::presetFolder().getChildFile(proc.presetName + ".orgu"),
                                                "*.orgu");
  chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
                           juce::FileBrowserComponent::warnAboutOverwriting,
                       [this](const juce::FileChooser& fc) {
                         auto f = fc.getResult();
                         if (f != juce::File()) proc.savePresetFile(f.withFileExtension("orgu"));
                       });
}

void OrguEditor::loadPreset() {
  chooser = std::make_unique<juce::FileChooser>(U8("Preset aç"), OrguProcessor::presetFolder(), "*.orgu");
  chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                       [this](const juce::FileChooser& fc) {
                         auto f = fc.getResult();
                         if (f.existsAsFile()) proc.loadPresetFile(f);
                       });
}

void OrguEditor::showEngineMenu() {
  juce::PopupMenu m;
  const int cur = engine();
  for (int b = 0; b < 3; ++b) {
    m.addSectionHeader(juce::String::fromUTF8(kBankNames[b]));
    for (int k = 0; k < 8; ++k) {
      const int e = b * 8 + k;
      m.addItem(e + 1, juce::String(e + 1) + "  " + orgu::engineName(e), true, e == cur);
    }
  }
  m.showMenuAsync(juce::PopupMenu::Options(), [this](int r) {
    if (r > 0) selectEngine(r - 1);
  });
}

// ------------------------------------------------------------------ content painting

void OrguEditor::Content::paint(juce::Graphics& g) {
  auto& p = ed.proc;
  const auto value = [&](const juce::String& id) {
    if (auto* prm = p.apvts.getParameter(id)) return prm->getCurrentValueAsText();
    return juce::String();
  };
  const auto text = [&](const juce::String& t, int x, int y, int w, int h, float size, bool bold,
                        juce::Colour c, juce::Justification j = juce::Justification::centred) {
    g.setColour(c);
    g.setFont(panelFont(size, bold));
    g.drawText(t, x, y, w, h, j);
  };
  const auto mono = [&](const juce::String& t, int x, int y, int w, int h, float size, juce::Colour c,
                        juce::Justification j = juce::Justification::centred) {
    g.setColour(c);
    g.setFont(monoFont(size));
    g.drawText(t, x, y, w, h, j);
  };
  const auto L = juce::Justification::centredLeft;

  g.fillAll(col::panel);
  g.setColour(col::border);
  g.drawRect(getLocalBounds(), 1);

  // --- header
  text(U("ÖRGÜ"), kLx, 26, 100, 40, 30.0f, true, col::ink, L);
  text(U("MAKRO OSİLATÖR"), kLx + 104, 38, 300, 20, 12.0f, false, col::inkSoft, L);

  // --- left panel: engine section
  const int e = ed.engine();
  const int bank = e / 8;
  text("MOTOR", kLx, 96, kLw, 18, 13.0f, true, col::ink);
  for (int k = 0; k < 8; ++k) {
    const int idx = bank * 8 + k;
    const bool on = idx == e;
    const int y = kLedY + k * kLedStep;
    g.setColour(on ? bankColour(bank) : col::ledOff);
    g.fillEllipse((float)kLedX, (float)y, (float)kLedSize, (float)kLedSize);
    g.setColour(on ? col::ink : col::inkMid);
    g.setFont(on ? monoFont(11.0f).boldened() : monoFont(10.5f));
    g.drawText(shortEngine(idx), kLedX + 18, y - 3, 60, 16, L);
  }
  text(U("◀ ÖNCEKİ"), kLx + 40, 198, 112, 14, 10.0f, false, col::inkSoft);
  text(U("SONRAKİ ▶"), kLx + kLw - 152, 198, 112, 14, 10.0f, false, col::inkSoft);
  for (int b = 0; b < 3; ++b) {
    const int x = kBankX0 + b * kBankStep;
    g.setColour(bankColour(b));
    g.fillEllipse((float)x, (float)kBankY + 6.0f, 8.0f, 8.0f);
    text(juce::String::fromUTF8(kBankNames[b]), x + 12, kBankY, 70, 20, 9.5f, b == bank,
         b == bank ? col::ink : col::inkSoft, L);
  }

  // macro labels / values
  text("FREQUENCY", kLx + 20, 268, 200, 18, 13.0f, true, col::ink);
  text("HARMONICS", kLx + kLw - 220, 268, 200, 18, 13.0f, true, col::ink);
  mono(value(ids::frequency), kLx + 20, 432, 200, 16, 11.0f, col::inkMid);
  mono(value(ids::harmonics), kLx + kLw - 220, 432, 200, 16, 11.0f, col::inkMid);
  text("TIMBRE  " + value(ids::timbre), kLx + 40, 540, 164, 16, 12.0f, true, col::ink);
  text("MORPH  " + value(ids::morph), kLx + kLw - 204, 540, 164, 16, 12.0f, true, col::ink);
  const char* envLabels[3] = {"FM ENV", "TIMBRE ENV", "MORPH ENV"};
  for (int i = 0; i < 3; ++i) text(envLabels[i], kLx + 44 + i * 130, 624, 100, 14, 10.0f, false, col::inkSoft);
  text("LPG DECAY", kLx + 66, 652, 120, 16, 11.0f, true, col::ink, L);
  mono(value(ids::lpgDecay), kLx + 66, 668, 120, 16, 10.0f, col::inkMid, L);
  text("LPG COLOR", kLx + 296, 652, 120, 16, 11.0f, true, col::ink, L);
  mono(value(ids::lpgColor), kLx + 296, 668, 120, 16, 10.0f, col::inkMid, L);

  g.setColour(col::divider);
  g.drawVerticalLine(kDivX, 96.0f, 690.0f);

  // --- expansion panel
  text(orgu::engineName(e), kRx, 254, 340, 26, 17.0f, true, col::ink, L);
  g.setColour(col::inkSoft);
  g.setFont(panelFont(10.0f, false));
  g.drawText(U(orgu::engineHint(e)), kRx, 280, kRw, 16, L);

  // readout (last touched control)
  const auto age = juce::Time::getMillisecondCounter() - ed.readoutTime;
  if (ed.readout.isNotEmpty() && age < 2500) {
    const float alpha = age < 1800 ? 1.0f : 1.0f - (float)(age - 1800) / 700.0f;
    mono(ed.readout, kRx + 300, 254, kRw - 300, 26, 13.0f, col::ink.withAlpha(alpha),
         juce::Justification::centredRight);
  }

  const auto section = [&](int x, int y, int w, const juce::String& t) {
    text(t, x, y, w, 16, 11.0f, true, col::ink, L);
    g.setColour(col::divider);
    g.drawHorizontalLine(y + 20, (float)x, (float)(x + w));
  };
  const auto under = [&](int x, int y, int w, const juce::String& label, const juce::String& id) {
    text(label, x, y, w, 14, 10.0f, true, col::ink);
    mono(value(id), x, y + 14, w, 14, 10.0f, col::inkMid);
  };

  section(kRx, 312, 330, "AMP ZARFI");
  const char* adsrL[4] = {"ATTACK", "DECAY", "SUSTAIN", "RELEASE"};
  const juce::String adsrId[4] = {ids::attack, ids::decay, ids::sustain, ids::release};
  for (int i = 0; i < 4; ++i) under(kRx + i * 84, 424, 84, adsrL[i], adsrId[i]);
  if (ed.shownAmp == orgu::AmpLpg)
    text(U("LPG modunda yalnız Chiptune'u şekillendirir"), kRx + 110, 342, 220, 26, 9.5f, false, col::inkSoft, L);

  section(kRx + 360, 312, 228, U("ÇALMA"));
  mono("SES", kRx + 360, 342, 40, 26, 11.0f, col::inkMid, L);
  under(kRx + 360, 424, 84, "GLIDE", ids::glide);
  under(kRx + 470, 424, 84, "BEND", ids::bendRange);

  section(kRx, 468, kRw, U("LFO · MOD ZARFI"));
  text("LFO 1", kRx + 56, 498, 80, 16, 11.0f, true, col::ink, L);
  mono(value(ids::lfo1Rate), kRx + 56, 514, 100, 14, 10.0f, col::inkMid, L);
  text("LFO 2", kRx + 256, 498, 80, 16, 11.0f, true, col::ink, L);
  mono(value(ids::lfo2Rate), kRx + 256, 514, 100, 14, 10.0f, col::inkMid, L);
  text("M.ATTACK", kRx + 410, 540, 80, 12, 9.0f, true, col::ink);
  mono(value(ids::menvAttack), kRx + 410, 552, 80, 12, 9.5f, col::inkMid);
  text("M.DECAY", kRx + 500, 540, 80, 12, 9.0f, true, col::ink);
  mono(value(ids::menvDecay), kRx + 500, 552, 80, 12, 9.5f, col::inkMid);

  section(kRx, 566, 400, U("MOD MATRİS"));
  for (int i = 0; i < orgu::kNumSlots; ++i) {
    const int y = 594 + i * 24;
    text(U("→"), kRx + 106, y, 20, 20, 11.0f, false, col::inkSoft);
    mono(value(ids::slotAmt(i)), kRx + 376, y, 50, 20, 10.0f, col::inkMid, L);
  }

  section(kRx + 430, 566, 158, U("ÇIKIŞ"));
  text("OUT/AUX", kRx + 418, 646, 72, 14, 9.0f, false, col::inkSoft);
  text("LEVEL", kRx + 488, 646, 72, 14, 9.0f, false, col::inkSoft);
  mono(value(ids::level), kRx + 488, 658, 72, 12, 9.5f, col::inkMid);
  const float thresholds[6] = {0.02f, 0.06f, 0.15f, 0.35f, 0.7f, 0.98f};
  for (int i = 0; i < 6; ++i) {
    juce::Colour c = col::ledOff;
    if (ed.shownLevel >= thresholds[i]) c = i < 3 ? col::green : (i < 5 ? col::amber : col::red);
    g.setColour(c);
    g.fillEllipse((float)(kRx + 430 + i * 14), 676.0f, 10.0f, 10.0f);
  }
}

void OrguEditor::Content::mouseDown(const juce::MouseEvent& ev) {
  const auto pos = ev.getPosition();
  const int e = ed.engine();
  const int bank = e / 8;
  // LED rows (LED + short name)
  if (pos.x >= kLedX - 4 && pos.x < kLedX + 80 && pos.y >= kLedY - 3 && pos.y < kLedY + 8 * kLedStep) {
    const int k = juce::jlimit(0, 7, (pos.y - kLedY + 3) / kLedStep);
    ed.selectEngine(bank * 8 + k);
    return;
  }
  // bank labels
  if (pos.y >= kBankY && pos.y < kBankY + 20 && pos.x >= kBankX0 && pos.x < kBankX0 + 3 * kBankStep) {
    const int b = juce::jlimit(0, 2, (pos.x - kBankX0) / kBankStep);
    ed.selectEngine(b * 8 + e % 8);
    return;
  }
  // engine name -> full list
  if (pos.y >= 254 && pos.y < 280 && pos.x >= kRx && pos.x < kRx + 300) ed.showEngineMenu();
}
