#include "dsp/fmod_distortion_shaper.h"
#include <cassert>
#include <cstring>
#include <iostream>

#define CLAMP(value, minimum, maximum) std::clamp(value, minimum, maximum)
struct FmodAudioEffectDistortion {
    enum Mode { MODE_CLIP, MODE_ATAN, MODE_LOFI, MODE_OVERDRIVE, MODE_WAVESHAPE, MODE_BITCRUSH };
};

	static float original_sample(float sample, int mode, float drive, float atan_mult, float atan_div, float lofi_mult) {
		switch (mode) {
			case FmodAudioEffectDistortion::MODE_CLIP: {
				float sign = sample < 0.0f ? -1.0f : 1.0f;
				sample = std::pow(std::abs(sample), 1.0001f - drive) * sign;
				return CLAMP(sample, -1.0f, 1.0f);
			}
			case FmodAudioEffectDistortion::MODE_ATAN:
				return std::atan(sample * atan_mult) * atan_div;
			case FmodAudioEffectDistortion::MODE_LOFI:
			case FmodAudioEffectDistortion::MODE_BITCRUSH:
				return std::floor(sample * lofi_mult + 0.5f) / lofi_mult;
			case FmodAudioEffectDistortion::MODE_OVERDRIVE: {
				double x = static_cast<double>(sample) * 0.686306;
				double z = 1.0 + std::exp(std::sqrt(std::abs(x)) * -0.75);
				return static_cast<float>((std::exp(x) - std::exp(-x * z)) / (std::exp(x) + std::exp(-x)));
			}
			case FmodAudioEffectDistortion::MODE_WAVESHAPE: {
				float k = 2.0f * drive / (1.00001f - drive);
				return (1.0f + k) * sample / (1.0f + k * std::abs(sample));
			}
		}

		return sample;
	}


int main() {
    int comparisons = 0;
    for (int mode = 0; mode < 6; mode++) {
        for (float drive : { 0.0f, 0.25f, 0.5f, 0.99f, 1.0f }) {
            const float mult = std::pow(10.0f, drive * drive * 3.0f) - 1.0f + 0.001f;
            const float div = 1.0f / (std::atan(mult) * (1.0f + drive * 8.0f));
            const float lofi = std::pow(2.0f, 2.0f + (1.0f - drive) * 14.0f);
            for (float gain : { 0.0f, 0.001f, 0.1f, 1.0f, 10.0f, 1000.0f }) {
                for (int point = -256; point <= 256; point++) {
                    const float sample = float(point) / 256.0f * gain;
                    const float actual = fmod_distortion::process_sample(sample, mode, drive, mult, div, lofi);
                    const float expected = original_sample(sample, mode, drive, mult, div, lofi);
                    assert(std::isfinite(actual));
                    assert(std::memcmp(&actual, &expected, sizeof(float)) == 0);
                    comparisons++;
                }
            }
        }
    }
    assert(fmod_distortion::process_sample(0.25f, 99, 0.0f, 0.0f, 0.0f, 0.0f) == 0.25f);
    std::cout << "Distortion shared shaper: " << comparisons << " bit-exact comparisons against original DSP across all six modes and gain/drive boundaries passed\n";
}
