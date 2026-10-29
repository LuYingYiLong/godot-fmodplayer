#ifndef FMOD_FILTER_PREVIEW_RESPONSE_H
#define FMOD_FILTER_PREVIEW_RESPONSE_H

#include "fmod_eq_preview_response.h"

namespace fmod_filter_preview {
	inline fmod_eq_preview::Band make_response(int p_slope, double p_cutoff, double p_resonance, double p_sample_rate) {
		using Filter = fmod_eq_preview::Filter;
		Filter type = Filter::LOWPASS_6DB;
		if (p_slope == 1) {
			type = Filter::LOWPASS_12DB;
		}
		else if (p_slope == 2 || p_slope == 3) {
			// 与运行时一致，18 dB 选项使用 24 dB 低通
			type = Filter::LOWPASS_24DB;
		}
		// MULTIBAND_EQ 的低通不使用 gain，频率和 Q 按 FMOD 有效范围绘制
		return fmod_eq_preview::make_band(type, std::clamp(p_cutoff, 20.0, 22000.0), 0.0, std::clamp(p_resonance, 0.1, 10.0), p_sample_rate);
	}
}

#endif // FMOD_FILTER_PREVIEW_RESPONSE_H
