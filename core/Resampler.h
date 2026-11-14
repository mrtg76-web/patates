// Stereo windowed-sinc resampler: pulls audio rendered at 48 kHz and delivers
// it at the host sample rate. At 48 kHz host rate it is an exact pass-through.
#pragma once

#include <cmath>
#include <cstring>
#include <vector>

namespace orgu {

constexpr double kPi = 3.14159265358979323846;

class Resampler {
 public:
  static constexpr int kTaps = 24;        // even
  static constexpr int kPhases = 512;
  static constexpr int kHalf = kTaps / 2;

  void init(double inputRate, double outputRate) {
    ratio_ = inputRate / outputRate;
    const double cutoff = ratio_ <= 1.0 ? 1.0 : 0.96 / ratio_;
    table_.assign(static_cast<size_t>(kPhases + 1) * kTaps, 0.0f);
    for (int p = 0; p <= kPhases; ++p) {
      const double frac = static_cast<double>(p) / kPhases;
      double sum = 0.0;
      for (int t = 0; t < kTaps; ++t) {
        const double x = (t - kHalf + 1) - frac;      // tap offset relative to the read point
        double s = (std::fabs(x) < 1e-9) ? 1.0 : std::sin(kPi * cutoff * x) / (kPi * cutoff * x);
        const double w = window(x / (kHalf + 0.5));
        const double v = cutoff * s * w;
        table_[p * kTaps + t] = static_cast<float>(v);
        sum += v;
      }
      // normalise DC gain of every phase to exactly 1
      for (int t = 0; t < kTaps; ++t) table_[p * kTaps + t] = static_cast<float>(table_[p * kTaps + t] / sum);
    }
    passThrough_ = std::fabs(ratio_ - 1.0) < 1e-9;
    reset();
  }

  void reset() {
    bufL_.assign(kHalf, 0.0f);
    bufR_.assign(kHalf, 0.0f);
    pos_ = kHalf - 1;
    bufL_.reserve(4096);
    bufR_.reserve(4096);
  }

  // gen(float* l, float* r) must append exactly `chunk` samples at the input rate.
  template <typename Gen>
  void process(float* outL, float* outR, int n, int chunk, Gen&& gen) {
    float tmpL[64], tmpR[64];
    for (int i = 0; i < n; ++i) {
      const int i0 = static_cast<int>(pos_);
      while (static_cast<int>(bufL_.size()) <= i0 + kHalf) {
        gen(tmpL, tmpR);
        bufL_.insert(bufL_.end(), tmpL, tmpL + chunk);
        bufR_.insert(bufR_.end(), tmpR, tmpR + chunk);
      }
      if (passThrough_) {
        outL[i] = bufL_[i0];
        outR[i] = bufR_[i0];
      } else {
        const double frac = pos_ - i0;
        const double fp = frac * kPhases;
        const int p = static_cast<int>(fp);
        const float a = static_cast<float>(fp - p);
        const float* k0 = &table_[p * kTaps];
        const float* k1 = k0 + kTaps;
        const float* l = &bufL_[i0 - kHalf + 1];
        const float* r = &bufR_[i0 - kHalf + 1];
        float sl = 0.0f, sr = 0.0f;
        for (int t = 0; t < kTaps; ++t) {
          const float k = k0[t] + a * (k1[t] - k0[t]);
          sl += l[t] * k;
          sr += r[t] * k;
        }
        outL[i] = sl;
        outR[i] = sr;
      }
      pos_ += ratio_;
    }
    // discard consumed history, keep kHalf samples before the read point
    const int drop = static_cast<int>(pos_) - kHalf;
    if (drop > 256) {
      bufL_.erase(bufL_.begin(), bufL_.begin() + drop);
      bufR_.erase(bufR_.begin(), bufR_.begin() + drop);
      pos_ -= drop;
    }
  }

 private:
  static double window(double x) {        // Blackman-Harris on [-1, 1]
    if (x <= -1.0 || x >= 1.0) return 0.0;
    const double t = (x + 1.0) * 0.5;
    return 0.35875 - 0.48829 * std::cos(2 * kPi * t) + 0.14128 * std::cos(4 * kPi * t)
           - 0.01168 * std::cos(6 * kPi * t);
  }

  double ratio_ = 1.0;
  double pos_ = 0.0;
  bool passThrough_ = true;
  std::vector<float> table_;
  std::vector<float> bufL_, bufR_;
};

}  // namespace orgu
