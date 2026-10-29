#ifndef FMOD_PHASER_PREVIEW_RESPONSE_H
#define FMOD_PHASER_PREVIEW_RESPONSE_H

#include <algorithm>
#include <cmath>
#include <complex>

namespace fmod_phaser_preview {
	constexpr double PI = 3.14159265358979323846;

	inline std::complex<double> filtered_response(double p_frequency, double p_min_hz, double p_max_hz, double p_feedback, double p_sample_rate) {
		const double minimum = std::clamp(p_min_hz, 10.0, 10000.0);
		const double maximum = std::clamp(p_max_hz, minimum, 10000.0);
		// 冻结在 LFO 的中点，与运行时的 d 和六级全通系数一致
		const double delay = (minimum + maximum) / p_sample_rate;
		const double a = (1.0 - delay) / (1.0 + delay);
		const std::complex<double> z = std::polar(1.0, -2.0 * PI * p_frequency / p_sample_rate);
		const std::complex<double> stage = (z - a) / (1.0 - a * z);
		const std::complex<double> chain = std::pow(stage, 6);
		// 运行时反馈读取上一采样的滤波输出
		return chain / (1.0 - std::clamp(p_feedback, 0.1, 0.9) * z * chain);
	}

	inline double phase_degrees(double p_frequency, double p_min_hz, double p_max_hz, double p_feedback, double p_sample_rate) {
		return std::arg(filtered_response(p_frequency, p_min_hz, p_max_hz, p_feedback, p_sample_rate)) * 180.0 / PI;
	}

	inline std::complex<double> mix_with_dry(const std::complex<double>& p_filtered, double p_depth) {
		// 对应运行时 source + filtered * depth，保留原声参与产生的峰谷
		return 1.0 + std::clamp(p_depth, 0.1, 4.0) * p_filtered;
	}

	inline double gain_db(const std::complex<double>& p_response) {
		// 完全相消时使用有限的显示下限
		return 20.0 * std::log10(std::max(std::abs(p_response), 1e-6));
	}
}

#endif // FMOD_PHASER_PREVIEW_RESPONSE_H
