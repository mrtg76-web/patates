// Headless checks for the Örgü core (no JUCE needed).
#include <cmath>
#include <cstdio>
#include <vector>

#include "OrguSynth.h"

static int failures = 0;
#define CHECK(cond, ...)                         \
  do {                                           \
    if (!(cond)) {                               \
      std::printf("FAIL %s:%d ", __FILE__, __LINE__); \
      std::printf(__VA_ARGS__);                  \
      std::printf("\n");                         \
      ++failures;                                \
    }                                            \
  } while (0)

struct Stats {
  double rms = 0.0;
  float peak = 0.0f;
  bool finite = true;
};

static Stats render(orgu::Synth& s, int samples, int block = 256) {
  std::vector<float> l(block), r(block);
  double sum = 0.0;
  Stats st;
  for (int done = 0; done < samples; done += block) {
    const int n = std::min(block, samples - done);
    s.process(l.data(), r.data(), n);
    for (int i = 0; i < n; ++i) {
      if (!std::isfinite(l[i]) || !std::isfinite(r[i])) st.finite = false;
      sum += l[i] * l[i] + r[i] * r[i];
      st.peak = std::max(st.peak, std::max(std::fabs(l[i]), std::fabs(r[i])));
    }
  }
  st.rms = std::sqrt(sum / (2.0 * samples));
  return st;
}

static void testEngines(double rate) {
  for (int mode = 0; mode < orgu::kNumAmpModes; ++mode) {
    for (int e = 0; e < orgu::kNumEngines; ++e) {
      orgu::Synth s;
      s.setSampleRate(rate);
      s.params.engine = e;
      s.params.ampMode = mode;
      s.noteOn(48, 1.0f);
      Stats on = render(s, static_cast<int>(rate * 0.4));
      CHECK(on.finite, "engine %d mode %d produced NaN/inf @%.0f", e, mode, rate);
      CHECK(on.rms > 1e-3, "engine %d mode %d silent @%.0f (rms %g)", e, mode, rate, on.rms);
      CHECK(on.peak <= 1.0f, "engine %d over full scale", e);
      s.noteOff(48);
      render(s, static_cast<int>(rate * 4.0));   // let the release / LPG close
      Stats off = render(s, static_cast<int>(rate * 0.2));
      // in LPG mode the DX7-style engines keep their own (long) release
      const double limit = (mode == orgu::AmpLpg && e >= 2 && e <= 4) ? 1e-2 : 1e-3;
      CHECK(off.rms < limit, "engine %d mode %d does not stop after note-off (rms %g)", e, mode, off.rms);
    }
  }
}

static void testPolyphony() {
  orgu::Synth s;
  s.params.ampMode = orgu::AmpAdsr;
  s.params.voices = 4;
  for (int n = 0; n < 4; ++n) s.noteOn(48 + n * 4, 0.8f);
  render(s, 4800);
  CHECK(s.activeVoiceCount() == 4, "expected 4 active voices, got %d", s.activeVoiceCount());
  s.noteOn(72, 0.8f);   // steals one
  render(s, 4800);
  CHECK(s.activeVoiceCount() == 4, "stealing should keep 4 voices, got %d", s.activeVoiceCount());
  s.allNotesOff();
  render(s, 48000 * 5);
  CHECK(s.activeVoiceCount() == 0, "voices still active after all notes off: %d", s.activeVoiceCount());
}

static void testMonoAndSustain() {
  orgu::Synth s;
  s.params.ampMode = orgu::AmpAdsr;
  s.params.voices = 1;
  s.noteOn(60, 1.0f);
  s.noteOn(64, 1.0f);
  s.noteOff(64);                    // back to 60, still gated
  render(s, 4800);
  CHECK(render(s, 4800).rms > 1e-3, "mono fallback note should keep sounding");
  s.noteOff(60);
  render(s, 48000 * 5);
  CHECK(render(s, 4800).rms < 1e-3, "mono voice should stop");

  s.params.voices = 8;
  render(s, 480);
  s.sustainPedal(true);
  s.noteOn(60, 1.0f);
  s.noteOff(60);
  render(s, 48000);
  CHECK(render(s, 4800).rms > 1e-3, "sustain pedal should hold the note");
  s.sustainPedal(false);
  render(s, 48000 * 5);
  CHECK(render(s, 4800).rms < 1e-3, "releasing the pedal should stop the note");
}

static void testResampler() {
  // 48k → 48k is an exact copy; 48k → 44.1k keeps a 1 kHz sine's level.
  for (double out : {48000.0, 44100.0, 96000.0}) {
    orgu::Resampler r;
    r.init(48000.0, out);
    double ph = 0.0;
    std::vector<float> l(1024), rr(1024);
    double sum = 0.0;
    int count = 0;
    for (int b = 0; b < 200; ++b) {
      r.process(l.data(), rr.data(), 1024, 12, [&](float* a, float* c) {
        for (int i = 0; i < 12; ++i) {
          a[i] = c[i] = static_cast<float>(std::sin(ph));
          ph += 2.0 * 3.14159265358979 * 1000.0 / 48000.0;
        }
      });
      if (b > 10)
        for (float v : l) { sum += v * v; ++count; }
    }
    const double rms = std::sqrt(sum / count);
    CHECK(std::fabs(rms - 0.7071) < 0.01, "resampler @%.0f changes level: rms %f", out, rms);
  }
}

int main() {
  testResampler();
  testEngines(48000.0);
  testEngines(44100.0);
  testPolyphony();
  testMonoAndSustain();
  if (failures == 0) std::printf("all core tests passed\n");
  return failures == 0 ? 0 : 1;
}
