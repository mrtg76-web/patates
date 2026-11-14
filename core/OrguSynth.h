// Örgü — polyphonic host for the Plaits macro-oscillator DSP.
// Framework independent: the JUCE plugin and the headless tests both use it.
#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include "Resampler.h"

namespace plaits { class Voice; }
namespace stmlib { class BufferAllocator; }

namespace orgu {

constexpr int kNumEngines = 24;
constexpr int kMaxVoices = 8;
constexpr int kNumSlots = 4;
constexpr int kBlock = 12;              // Plaits block size (samples @ 48 kHz)
constexpr double kInternalRate = 48000.0;
constexpr int kScopeSize = 4096;

enum ModSource { SrcOff, SrcLfo1, SrcLfo2, SrcModEnv, SrcVelocity, SrcModWheel,
                 SrcAftertouch, SrcKeyTrack, SrcRandom, kNumSources };
enum ModDest { DstOff, DstFrequency, DstHarmonics, DstTimbre, DstMorph, DstDecay,
               DstColor, DstLevel, DstEngine, kNumDests };
enum AmpMode { AmpLpg, AmpAdsr, kNumAmpModes };
enum LfoShape { LfoSine, LfoTriangle, LfoSaw, LfoSquare, LfoSampleHold, kNumLfoShapes };

struct ModSlot {
  int source = SrcOff;
  int dest = DstOff;
  float amount = 0.0f;      // -1 .. 1
};

struct LfoParams {
  float rateHz = 1.0f;
  int shape = LfoSine;
};

struct Params {
  int engine = 13;            // 0..23, same order as the hardware
  float frequency = 0.0f;     // semitones, -36 .. +36
  float harmonics = 0.5f;
  float timbre = 0.5f;
  float morph = 0.5f;
  float fmEnv = 0.0f;         // attenuverters: internal decay envelope amount, -1 .. 1
  float timbreEnv = 0.0f;
  float morphEnv = 0.0f;
  float lpgDecay = 0.5f;
  float lpgColor = 0.5f;
  float outAux = 0.0f;        // 0 = OUT only, 1 = AUX only (mix mode)
  bool stereo = false;        // true: OUT left, AUX right
  float levelDb = -6.0f;
  int ampMode = AmpLpg;
  float attack = 0.005f, decay = 0.4f, sustain = 0.8f, release = 0.4f;   // seconds / 0..1
  int voices = 8;
  float glide = 0.0f;         // seconds
  float bendRange = 2.0f;     // semitones
  LfoParams lfo[2];
  float modEnvAttack = 0.01f, modEnvDecay = 0.5f;
  ModSlot slots[kNumSlots];
};

const char* engineName(int index);
const char* engineShortName(int index);
const char* engineGroupName(int index);   // "NEW", "PITCHED", "NOISE / PERCUSSION"
const char* engineHint(int index);        // what Harmonics / Timbre / Morph do
const char* modSourceName(int s);
const char* modDestName(int d);
const char* lfoShapeName(int s);

class Synth {
 public:
  Synth();
  ~Synth();

  void setSampleRate(double hostRate);
  void reset();
  Params params;

  // MIDI
  void noteOn(int note, float velocity);
  void noteOff(int note);
  void pitchBend(float v);       // -1 .. 1
  void modWheel(float v);        // 0 .. 1
  void aftertouch(float v);      // 0 .. 1
  void sustainPedal(bool down);
  void allNotesOff();

  // Renders n host-rate samples (adds nothing; overwrites).
  void process(float* left, float* right, int n);

  // Oscilloscope tap (48 kHz, post level). Safe to read from another thread.
  void copyScope(float* out, float* aux, int n) const;
  int activeVoiceCount() const { return activeVoices_.load(); }
  float lfoValue(int i) const { return lfoOut_[i]; }

 private:
  struct Adsr {
    enum Stage { Idle, Attack, Decay, Sustain, Release } stage = Idle;
    float value = 0.0f;
  };
  struct AdEnv {
    bool attacking = false;
    float value = 0.0f;
  };
  struct VoiceSlot {
    std::unique_ptr<plaits::Voice> dsp;
    std::unique_ptr<char[]> ram;
    std::unique_ptr<stmlib::BufferAllocator> allocator;
    int note = -1;
    float velocity = 0.0f;
    bool gate = false;
    bool sustained = false;     // note-off arrived while the pedal was down
    uint64_t age = 0;
    float noteCurrent = 60.0f;
    float random = 0.0f;
    int retriggerLow = 0;
    int blocksSinceOn = 0;
    Adsr amp;
    AdEnv modEnv;
    float follower = 0.0f;
    float lastGain = 0.0f;
    bool active = false;
  };

  void renderBlock(float* out, float* aux);
  void startVoice(VoiceSlot& v, int note, float velocity, bool retrigger);
  void releaseVoice(VoiceSlot& v);
  int allocateVoice(int note);
  void syncVoiceCount();
  float nextRandom();

  std::array<VoiceSlot, kMaxVoices> voices_;
  std::vector<int> monoStack_;
  uint64_t ageCounter_ = 0;
  float lastNote_ = 60.0f;
  float bend_ = 0.0f, modWheel_ = 0.0f, aftertouch_ = 0.0f;
  bool sustain_ = false;
  float lfoPhase_[2] = {0.0f, 0.0f};
  float lfoHold_[2] = {0.0f, 0.0f};
  float lfoOut_[2] = {0.0f, 0.0f};
  uint32_t rng_ = 0x12345678u;
  int lastVoiceCount_ = 8;

  Resampler resampler_;
  float scopeOut_[kScopeSize] = {};
  float scopeAux_[kScopeSize] = {};
  std::atomic<int> scopeWrite_{0};
  std::atomic<int> activeVoices_{0};
};

}  // namespace orgu
