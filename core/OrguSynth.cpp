#include "OrguSynth.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "plaits/dsp/voice.h"
#include "stmlib/utils/buffer_allocator.h"

namespace orgu {

namespace {

constexpr int kVoiceRam = 16384;
constexpr float kBlockDt = static_cast<float>(kBlock / kInternalRate);

struct EngineInfo {
  const char* name;
  const char* shortName;
  const char* hint;
};

const EngineInfo kEngines[kNumEngines] = {
  {"VA + VCF", "VCF", "Harmonics: filter resonance & character · Timbre: cutoff · Morph: waveform"},
  {"Phase Distortion", "PD", "Harmonics: distortion frequency · Timbre: amount · Morph: asymmetry"},
  {"6-Op FM I", "FM6", "Harmonics: patch (32) · Timbre: modulator level · Morph: envelope stretch"},
  {"6-Op FM II", "FM6", "Harmonics: patch (32) · Timbre: modulator level · Morph: envelope stretch"},
  {"6-Op FM III", "FM6", "Harmonics: patch (32) · Timbre: modulator level · Morph: envelope stretch"},
  {"Wave Terrain", "TER", "Harmonics: terrain · Timbre: path radius · Morph: path offset"},
  {"String Machine", "STM", "Harmonics: chord · Timbre: chorus / filter · Morph: registration"},
  {"Chiptune", "8BT", "Harmonics: chord · Timbre: arpeggio · Morph: pulse width / sweep"},
  {"Virtual Analog", "VA", "Harmonics: detune · Timbre: square shape · Morph: saw shape"},
  {"Waveshaping", "WSH", "Harmonics: shaper waveform · Timbre: fold amount · Morph: asymmetry"},
  {"2-Op FM", "FM2", "Harmonics: frequency ratio · Timbre: modulation index · Morph: feedback"},
  {"Formant", "FRM", "Harmonics: formant ratio · Timbre: formant frequency · Morph: formant width"},
  {"Harmonic", "HRM", "Harmonics: number of bumps · Timbre: prominent harmonic · Morph: bump shape"},
  {"Wavetable", "WTB", "Harmonics: bank · Timbre: row · Morph: column"},
  {"Chords", "CHD", "Harmonics: chord type · Timbre: inversion · Morph: waveform"},
  {"Speech", "SPC", "Harmonics: synthesis type · Timbre: species · Morph: phoneme / word"},
  {"Swarm", "SWM", "Harmonics: pitch spread · Timbre: grain density · Morph: grain duration"},
  {"Filtered Noise", "NOI", "Harmonics: LP → BP → HP · Timbre: clock frequency · Morph: resonance"},
  {"Particle", "PRT", "Harmonics: frequency spread · Timbre: density · Morph: filter type"},
  {"Inharmonic String", "STR", "Harmonics: inharmonicity · Timbre: brightness · Morph: decay"},
  {"Modal", "MOD", "Harmonics: material · Timbre: brightness · Morph: decay"},
  {"Bass Drum", "BD", "Harmonics: punch / drive · Timbre: brightness · Morph: decay"},
  {"Snare Drum", "SD", "Harmonics: tone / noise · Timbre: mode balance · Morph: decay"},
  {"Hi-Hat", "HH", "Harmonics: metal / noise · Timbre: high-pass · Morph: decay"},
};

// Engines that apply their own amplitude envelope (Plaits bypasses the LPG for them).
inline bool selfEnveloped(int e) {
  return (e >= 2 && e <= 4) || e >= 19;
}

inline float clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

inline float segmentCoef(float seconds) {
  // one-pole that reaches ~1.8% of the distance after `seconds`
  const float t = std::max(seconds, 0.0005f) * 0.25f;
  return std::exp(-kBlockDt / t);
}

}  // namespace

const char* engineName(int i) { return kEngines[std::clamp(i, 0, kNumEngines - 1)].name; }
const char* engineShortName(int i) { return kEngines[std::clamp(i, 0, kNumEngines - 1)].shortName; }
const char* engineHint(int i) { return kEngines[std::clamp(i, 0, kNumEngines - 1)].hint; }
const char* engineGroupName(int i) {
  return i < 8 ? "NEW" : (i < 16 ? "PITCHED" : "NOISE / PERCUSSION");
}
const char* modSourceName(int s) {
  static const char* n[kNumSources] = {"Off", "LFO 1", "LFO 2", "Mod Env", "Velocity",
                                       "Mod Wheel", "Aftertouch", "Key Track", "Random"};
  return n[std::clamp(s, 0, kNumSources - 1)];
}
const char* modDestName(int d) {
  static const char* n[kNumDests] = {"Off", "Frequency", "Harmonics", "Timbre", "Morph",
                                     "LPG Decay", "LPG Color", "Level", "Engine"};
  return n[std::clamp(d, 0, kNumDests - 1)];
}
const char* lfoShapeName(int s) {
  static const char* n[kNumLfoShapes] = {"Sine", "Triangle", "Saw", "Square", "S&H"};
  return n[std::clamp(s, 0, kNumLfoShapes - 1)];
}

Synth::Synth() {
  for (auto& v : voices_) {
    v.ram.reset(new char[kVoiceRam]);
    std::memset(v.ram.get(), 0, kVoiceRam);
    v.allocator.reset(new stmlib::BufferAllocator(v.ram.get(), kVoiceRam));
    v.dsp.reset(new plaits::Voice());
    v.dsp->Init(v.allocator.get());
  }
  setSampleRate(48000.0);
}

Synth::~Synth() = default;

void Synth::setSampleRate(double hostRate) {
  resampler_.init(kInternalRate, hostRate > 0 ? hostRate : 48000.0);
}

void Synth::reset() {
  allNotesOff();
  for (auto& v : voices_) {
    v.amp = Adsr();
    v.modEnv = AdEnv();
    v.follower = 0.0f;
    v.active = false;
    v.lastGain = 0.0f;
  }
  resampler_.reset();
}

float Synth::nextRandom() {
  rng_ ^= rng_ << 13;
  rng_ ^= rng_ >> 17;
  rng_ ^= rng_ << 5;
  return static_cast<float>(rng_ & 0xffffff) / 8388608.0f - 1.0f;   // -1 .. 1
}

// ------------------------------------------------------------------ MIDI

void Synth::startVoice(VoiceSlot& v, int note, float velocity, bool retrigger) {
  const bool wasSounding = v.gate || v.active;
  v.note = note;
  v.velocity = velocity;
  v.gate = true;
  v.sustained = false;
  v.age = ++ageCounter_;
  v.random = nextRandom();
  if (params.glide <= 0.0f || !wasSounding) {
    v.noteCurrent = (params.glide > 0.0f) ? lastNote_ : static_cast<float>(note);
  }
  if (retrigger) {
    v.retriggerLow = 2;
    v.amp.stage = Adsr::Attack;
    v.modEnv.attacking = true;
  } else if (!wasSounding) {
    v.amp.stage = Adsr::Attack;
    v.modEnv.attacking = true;
  }
  v.blocksSinceOn = 0;
  v.active = true;
  lastNote_ = static_cast<float>(note);
}

void Synth::releaseVoice(VoiceSlot& v) {
  v.gate = false;
  v.sustained = false;
  if (v.amp.stage != Adsr::Idle) v.amp.stage = Adsr::Release;
}

void Synth::syncVoiceCount() {
  // voices above the current count are released; switching mono <-> poly drops held notes
  const int voiceCount = std::clamp(params.voices, 1, kMaxVoices);
  if (voiceCount == lastVoiceCount_) return;
  if (voiceCount == 1 || lastVoiceCount_ == 1) {
    monoStack_.clear();
    for (auto& v : voices_) releaseVoice(v);
  } else {
    for (int i = voiceCount; i < kMaxVoices; ++i) releaseVoice(voices_[i]);
  }
  lastVoiceCount_ = voiceCount;
}

int Synth::allocateVoice(int note) {
  const int n = std::clamp(params.voices, 1, kMaxVoices);
  // same note still held → reuse it
  for (int i = 0; i < n; ++i)
    if (voices_[i].note == note && (voices_[i].gate || voices_[i].active)) return i;
  // free voice
  for (int i = 0; i < n; ++i)
    if (!voices_[i].active && !voices_[i].gate) return i;
  // oldest released voice
  int best = -1;
  for (int i = 0; i < n; ++i)
    if (!voices_[i].gate && (best < 0 || voices_[i].age < voices_[best].age)) best = i;
  if (best >= 0) return best;
  // oldest voice
  best = 0;
  for (int i = 1; i < n; ++i)
    if (voices_[i].age < voices_[best].age) best = i;
  return best;
}

void Synth::noteOn(int note, float velocity) {
  syncVoiceCount();
  if (velocity <= 0.0f) {
    noteOff(note);
    return;
  }
  if (params.voices <= 1) {
    monoStack_.erase(std::remove(monoStack_.begin(), monoStack_.end(), note), monoStack_.end());
    monoStack_.push_back(note);
    VoiceSlot& v = voices_[0];
    startVoice(v, note, velocity, v.gate || v.active);
    return;
  }
  VoiceSlot& v = voices_[allocateVoice(note)];
  startVoice(v, note, velocity, v.gate || v.active);
}

void Synth::noteOff(int note) {
  syncVoiceCount();
  if (params.voices <= 1) {
    const bool wasTop = !monoStack_.empty() && monoStack_.back() == note;
    monoStack_.erase(std::remove(monoStack_.begin(), monoStack_.end(), note), monoStack_.end());
    VoiceSlot& v = voices_[0];
    if (!wasTop) return;
    if (!monoStack_.empty()) {
      // fall back to the previous held note without retriggering
      v.note = monoStack_.back();
      lastNote_ = static_cast<float>(v.note);
      if (params.glide <= 0.0f) v.noteCurrent = lastNote_;
      return;
    }
    if (sustain_) v.sustained = true;
    else releaseVoice(v);
    return;
  }
  for (auto& v : voices_) {
    if (v.gate && v.note == note) {
      if (sustain_) v.sustained = true;
      else releaseVoice(v);
    }
  }
}

void Synth::pitchBend(float v) { bend_ = std::clamp(v, -1.0f, 1.0f); }
void Synth::modWheel(float v) { modWheel_ = clamp01(v); }
void Synth::aftertouch(float v) { aftertouch_ = clamp01(v); }

void Synth::sustainPedal(bool down) {
  sustain_ = down;
  if (!down) {
    for (auto& v : voices_)
      if (v.sustained) releaseVoice(v);
  }
}

void Synth::allNotesOff() {
  monoStack_.clear();
  sustain_ = false;
  for (auto& v : voices_) releaseVoice(v);
}

// ------------------------------------------------------------------ audio

void Synth::process(float* left, float* right, int n) {
  if (n <= 0) return;
  const float gain = std::pow(10.0f, params.levelDb / 20.0f);
  resampler_.process(left, right, n, kBlock, [this](float* l, float* r) {
    float out[kBlock], aux[kBlock];
    renderBlock(out, aux);
    const bool stereo = params.stereo;
    const float mix = clamp01(params.outAux);
    for (int i = 0; i < kBlock; ++i) {
      if (stereo) {
        l[i] = out[i];
        r[i] = aux[i];
      } else {
        l[i] = r[i] = out[i] + mix * (aux[i] - out[i]);
      }
    }
  });
  for (int i = 0; i < n; ++i) {
    for (float* s : {&left[i], &right[i]}) {
      float x = *s * gain;
      const float ax = std::fabs(x);
      if (ax > 0.9f) x = std::copysign(0.9f + 0.1f * std::tanh((ax - 0.9f) * 10.0f), x);
      *s = x;
    }
  }
}

void Synth::renderBlock(float* out, float* aux) {
  const Params& P = params;
  std::fill(out, out + kBlock, 0.0f);
  std::fill(aux, aux + kBlock, 0.0f);

  syncVoiceCount();

  // global LFOs
  for (int l = 0; l < 2; ++l) {
    float& ph = lfoPhase_[l];
    ph += std::clamp(P.lfo[l].rateHz, 0.001f, 100.0f) * kBlockDt;
    if (ph >= 1.0f) {
      ph -= std::floor(ph);
      lfoHold_[l] = nextRandom();
    }
    float v = 0.0f;
    switch (P.lfo[l].shape) {
      case LfoSine: v = std::sin(ph * 6.28318530718f); break;
      case LfoTriangle: v = ph < 0.5f ? ph * 4.0f - 1.0f : 3.0f - ph * 4.0f; break;
      case LfoSaw: v = ph * 2.0f - 1.0f; break;
      case LfoSquare: v = ph < 0.5f ? 1.0f : -1.0f; break;
      default: v = lfoHold_[l]; break;
    }
    lfoOut_[l] = v;
  }

  const float glideCoef = P.glide > 0.0f ? std::exp(-kBlockDt / std::max(P.glide * 0.3f, 1e-4f)) : 0.0f;
  const float aRate = kBlockDt / std::max(P.attack, 0.0005f);
  const float dCoef = segmentCoef(P.decay);
  const float rCoef = segmentCoef(P.release);
  const float meRate = kBlockDt / std::max(P.modEnvAttack, 0.0005f);
  const float meCoef = segmentCoef(P.modEnvDecay);
  const float bendSemis = bend_ * P.bendRange;
  const float sustainLevel = clamp01(P.sustain);

  int activeCount = 0;
  for (auto& v : voices_) {
    // envelopes advance even when idle so stages stay consistent
    switch (v.amp.stage) {
      case Adsr::Attack:
        v.amp.value += aRate;
        if (v.amp.value >= 1.0f) { v.amp.value = 1.0f; v.amp.stage = Adsr::Decay; }
        break;
      case Adsr::Decay:
        v.amp.value = sustainLevel + (v.amp.value - sustainLevel) * dCoef;
        if (std::fabs(v.amp.value - sustainLevel) < 1e-4f) v.amp.stage = Adsr::Sustain;
        break;
      case Adsr::Sustain: v.amp.value = sustainLevel; break;
      case Adsr::Release:
        v.amp.value *= rCoef;
        if (v.amp.value < 1e-5f) { v.amp.value = 0.0f; v.amp.stage = Adsr::Idle; }
        break;
      default: break;
    }
    if (v.modEnv.attacking) {
      v.modEnv.value += meRate;
      if (v.modEnv.value >= 1.0f) { v.modEnv.value = 1.0f; v.modEnv.attacking = false; }
    } else {
      v.modEnv.value *= meCoef;
    }

    if (!v.active) continue;
    ++activeCount;

    // glide
    const float target = static_cast<float>(v.note);
    v.noteCurrent = target + (v.noteCurrent - target) * glideCoef;

    // modulation matrix
    float d[kNumDests] = {};
    for (const ModSlot& s : P.slots) {
      if (s.source == SrcOff || s.dest == DstOff || s.amount == 0.0f) continue;
      float src = 0.0f;
      bool unipolar = true;
      switch (s.source) {
        case SrcLfo1: src = lfoOut_[0]; unipolar = false; break;
        case SrcLfo2: src = lfoOut_[1]; unipolar = false; break;
        case SrcModEnv: src = v.modEnv.value; break;
        case SrcVelocity: src = v.velocity; break;
        case SrcModWheel: src = modWheel_; break;
        case SrcAftertouch: src = aftertouch_; break;
        case SrcKeyTrack: src = (v.noteCurrent - 60.0f) / 48.0f; unipolar = false; break;
        case SrcRandom: src = v.random; unipolar = false; break;
        default: break;
      }
      // Level: a unipolar source at full scale leaves the level untouched,
      // at zero it pulls the level down by `amount` (velocity-style).
      if (s.dest == DstLevel && unipolar) src -= 1.0f;
      d[std::clamp(s.dest, 0, kNumDests - 1)] += s.amount * src;
    }

    plaits::Patch patch;
    patch.note = v.noteCurrent;
    patch.harmonics = clamp01(P.harmonics + d[DstHarmonics]);
    patch.timbre = clamp01(P.timbre + d[DstTimbre]);
    patch.morph = clamp01(P.morph + d[DstMorph]);
    patch.frequency_modulation_amount = std::clamp(P.fmEnv, -1.0f, 1.0f);
    patch.timbre_modulation_amount = std::clamp(P.timbreEnv, -1.0f, 1.0f);
    patch.morph_modulation_amount = std::clamp(P.morphEnv, -1.0f, 1.0f);
    const int engine = std::clamp(P.engine + static_cast<int>(std::lround(d[DstEngine] * 23.0f)), 0, kNumEngines - 1);
    patch.engine = engine;
    patch.decay = clamp01(P.lpgDecay + d[DstDecay]);
    patch.lpg_colour = clamp01(P.lpgColor + d[DstColor]);

    const float levelGain = std::clamp(1.0f + d[DstLevel], 0.0f, 2.0f);

    plaits::Modulations m;
    std::memset(&m, 0, sizeof(m));
    m.note = P.frequency + bendSemis + d[DstFrequency] * 24.0f;
    m.trigger_patched = true;
    if (v.retriggerLow > 0) {
      --v.retriggerLow;
      m.trigger = 0.0f;
    } else {
      m.trigger = v.gate ? 1.0f : 0.0f;
    }
    float postGain = levelGain;
    if (P.ampMode == AmpAdsr) {
      m.level_patched = true;
      m.level = v.amp.value * std::min(levelGain, 1.0f);
      postGain = levelGain > 1.0f ? levelGain : 1.0f;
      if (selfEnveloped(engine)) postGain *= v.amp.value;
    }
    // Chiptune treats TRIG as an arpeggiator clock and never closes on its
    // own, so it always follows the AMP envelope.
    if (engine == 7) postGain *= v.amp.value;

    plaits::Voice::Frame frames[kBlock];
    v.dsp->Render(patch, m, frames, kBlock);

    float peak = 0.0f;
    const float g0 = v.lastGain, step = (postGain - g0) / kBlock;
    for (int i = 0; i < kBlock; ++i) {
      const float g = (g0 + step * (i + 1)) * (1.0f / 32768.0f);
      const float o = frames[i].out * g;
      const float a = frames[i].aux * g;
      out[i] += o;
      aux[i] += a;
      peak = std::max(peak, std::max(std::fabs(o), std::fabs(a)));
    }
    v.lastGain = postGain;

    // voice activity tracking (for stealing and CPU)
    v.follower = std::max(peak, v.follower * 0.995f);
    ++v.blocksSinceOn;
    if (!v.gate && v.blocksSinceOn > 200 && v.follower < 1e-4f &&
        ((P.ampMode != AmpAdsr && engine != 7) || v.amp.stage == Adsr::Idle)) {
      v.active = false;
      v.follower = 0.0f;
      v.lastGain = 0.0f;
    }
  }
  activeVoices_.store(activeCount);

  // scope tap (post level)
  const float gain = std::pow(10.0f, P.levelDb / 20.0f);
  int w = scopeWrite_.load(std::memory_order_relaxed);
  for (int i = 0; i < kBlock; ++i) {
    scopeOut_[w] = out[i] * gain;
    scopeAux_[w] = aux[i] * gain;
    w = (w + 1) & (kScopeSize - 1);
  }
  scopeWrite_.store(w, std::memory_order_release);
}

void Synth::copyScope(float* outDst, float* auxDst, int n) const {
  n = std::min(n, kScopeSize);
  const int w = scopeWrite_.load(std::memory_order_acquire);
  for (int i = 0; i < n; ++i) {
    const int idx = (w - n + i + kScopeSize) & (kScopeSize - 1);
    outDst[i] = scopeOut_[idx];
    auxDst[i] = scopeAux_[idx];
  }
}

}  // namespace orgu
