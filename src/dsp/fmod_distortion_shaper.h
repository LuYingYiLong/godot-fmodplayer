#ifndef FMOD_DISTORTION_SHAPER_H
#define FMOD_DISTORTION_SHAPER_H

#include <algorithm>
#include <cmath>

namespace fmod_distortion {
	// 音频处理与编辑器曲线共用整形公式，模式值对应 FmodAudioEffectDistortion::Mode
	inline float process_sample(float sample, int mode, float drive, float atan_mult, float atan_div, float lofi_mult) {
		switch (mode) {
		case 0: {
			const float sign = sample < 0.0f ? -1.0f : 1.0f;
			sample = std::pow(std::abs(sample), 1.0001f - drive) * sign;
			return std::clamp(sample, -1.0f, 1.0f);
		}
		case 1:
			return std::atan(sample * atan_mult) * atan_div;
		case 2:
		case 5:
			return std::floor(sample * lofi_mult + 0.5f) / lofi_mult;
		case 3: {
			const double x = static_cast<double>(sample) * 0.686306;
			const double z = 1.0 + std::exp(std::sqrt(std::abs(x)) * -0.75);
			return static_cast<float>((std::exp(x) - std::exp(-x * z)) / (std::exp(x) + std::exp(-x)));
		}
		case 4: {
			const float k = 2.0f * drive / (1.00001f - drive);
			return (1.0f + k) * sample / (1.0f + k * std::abs(sample));
		}
		}
		return sample;
	}
}

#endif // FMOD_DISTORTION_SHAPER_H
