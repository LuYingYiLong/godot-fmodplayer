#ifndef FMOD_COMPRESSOR_PREVIEW_RESPONSE_H
#define FMOD_COMPRESSOR_PREVIEW_RESPONSE_H

#include <algorithm>
#include <cmath>

// 静态硬拐点近似，不参与音频处理，也不模拟包络跟随
namespace fmod_compressor_preview {
	inline double output_db(double p_input, double p_threshold, double p_ratio, double p_gain, double p_mix) {
		p_threshold = std::clamp(p_threshold, -60.0, 0.0);
		p_ratio = std::clamp(p_ratio, 1.0, 48.0);
		p_gain = std::clamp(p_gain, -20.0, 20.0);
		p_mix = std::clamp(p_mix, 0.0, 1.0);
		const double compressed = p_input <= p_threshold ? p_input : p_threshold + (p_input - p_threshold) / p_ratio;
		// Dry/Wet 在线性振幅域相加，补偿增益只作用于湿声
		const double amplitude = (1.0 - p_mix) * std::pow(10.0, p_input / 20.0) + p_mix * std::pow(10.0, (compressed + p_gain) / 20.0);
		return 20.0 * std::log10(std::max(amplitude, 1e-12));
	}

	inline double ratio_for_output(double p_input, double p_output, double p_threshold, double p_gain, double p_mix) {
		p_threshold = std::clamp(p_threshold, -60.0, 0.0);
		p_gain = std::clamp(p_gain, -20.0, 20.0);
		p_mix = std::clamp(p_mix, 0.000001, 1.0);
		const double dry = (1.0 - p_mix) * std::pow(10.0, p_input / 20.0);
		const double wet = (std::pow(10.0, p_output / 20.0) - dry) / p_mix;
		if (wet <= 0.0) {
			return 48.0;
		}
		const double above_threshold = 20.0 * std::log10(wet) - p_gain - p_threshold;
		return above_threshold <= 0.0 ? 48.0 : std::clamp((p_input - p_threshold) / above_threshold, 1.0, 48.0);
	}
}

#endif // FMOD_COMPRESSOR_PREVIEW_RESPONSE_H
