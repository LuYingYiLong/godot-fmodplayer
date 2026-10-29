#ifndef FMOD_EQ_PREVIEW_RESPONSE_H
#define FMOD_EQ_PREVIEW_RESPONSE_H

#include <algorithm>
#include <array>
#include <cmath>

// 编辑器使用的参数响应模型，不参与音频处理
namespace fmod_eq_preview {

enum class Filter {
	DISABLED, LOWPASS_12DB, LOWPASS_24DB, LOWPASS_48DB,
	HIGHPASS_12DB, HIGHPASS_24DB, HIGHPASS_48DB,
	LOWSHELF, HIGHSHELF, PEAKING, BANDPASS, NOTCH, ALLPASS,
	LOWPASS_6DB, HIGHPASS_6DB,
};

struct Biquad {
	double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;

	double response_db(double p_frequency, double p_sample_rate) const {
		const double w = 6.283185307179586 * std::clamp(p_frequency / p_sample_rate, 0.0, 0.499999);
		const double c1 = std::cos(w), c2 = std::cos(2.0 * w);
		const double s1 = std::sin(w), s2 = std::sin(2.0 * w);
		const double nr = b0 + b1 * c1 + b2 * c2, ni = b1 * s1 + b2 * s2;
		const double dr = 1.0 + a1 * c1 + a2 * c2, di = a1 * s1 + a2 * s2;
		return 10.0 * std::log10(std::max((nr * nr + ni * ni) / std::max(dr * dr + di * di, 1e-24), 1e-24));
	}
};

struct Band {
	std::array<Biquad, 4> sections;
	int count = 0;

	double response_db(double p_frequency, double p_sample_rate) const {
		double result = 0.0;
		for (int i = 0; i < count; i++) {
			result += sections[i].response_db(p_frequency, p_sample_rate);
		}
		return result;
	}
};

inline Band make_band(Filter p_type, double p_frequency, double p_gain, double p_q, double p_sample_rate) {
	Band result;
	if (p_type == Filter::DISABLED) {
		return result;
	}
	p_sample_rate = std::max(p_sample_rate, 100.0);
	p_frequency = std::clamp(p_frequency, 1.0, p_sample_rate * 0.499);
	p_q = std::clamp(p_q, 0.01, 10.0);
	const double w = 6.283185307179586 * p_frequency / p_sample_rate;
	const double c = std::cos(w), s = std::sin(w);
	const double a = std::pow(10.0, std::clamp(p_gain, -60.0, 24.0) / 40.0);
	const bool lowpass = p_type >= Filter::LOWPASS_12DB && p_type <= Filter::LOWPASS_48DB;
	const bool highpass = p_type >= Filter::HIGHPASS_12DB && p_type <= Filter::HIGHPASS_48DB;
	if (lowpass || highpass) {
		const int offset = int(p_type) - int(lowpass ? Filter::LOWPASS_12DB : Filter::HIGHPASS_12DB);
		result.count = 1 << offset;
		for (int i = 0; i < result.count; i++) {
			// Butterworth 级联作为高阶滤波的近似，Q 控制总截止频率增益
			double q = 1.0 / (2.0 * std::cos(3.141592653589793 * (2.0 * i + 1.0) / (4.0 * result.count)));
			if (i == 0) {
				q *= p_q / 0.7071067811865476;
			}
			const double alpha = s / (2.0 * q), norm = 1.0 + alpha;
			const double b = (lowpass ? 1.0 - c : 1.0 + c) * 0.5 / norm;
			result.sections[i] = { b, lowpass ? 2.0 * b : -2.0 * b, b, -2.0 * c / norm, (1.0 - alpha) / norm };
		}
		return result;
	}
	result.count = 1;
	Biquad& b = result.sections[0];
	if (p_type == Filter::LOWPASS_6DB || p_type == Filter::HIGHPASS_6DB) {
		const double k = std::tan(w * 0.5), norm = 1.0 / (1.0 + k);
		b = p_type == Filter::LOWPASS_6DB ? Biquad{ k * norm, k * norm, 0.0, (k - 1.0) * norm, 0.0 }
			: Biquad{ norm, -norm, 0.0, (k - 1.0) * norm, 0.0 };
		return result;
	}
	const double alpha = s / (2.0 * p_q);
	double b0 = 1.0, b1 = -2.0 * c, b2 = 1.0, a0 = 1.0 + alpha, a1 = -2.0 * c, a2 = 1.0 - alpha;
	switch (p_type) {
		case Filter::PEAKING:
			b0 = 1.0 + alpha * a;
			b2 = 1.0 - alpha * a;
			a0 = 1.0 + alpha / a;
			a2 = 1.0 - alpha / a;
			break;
		case Filter::BANDPASS:
			b0 = alpha; b1 = 0.0; b2 = -alpha;
			break;
		case Filter::NOTCH:
			break;
		case Filter::ALLPASS:
			b0 = 1.0 - alpha; b2 = 1.0 + alpha;
			break;
		case Filter::LOWSHELF:
		case Filter::HIGHSHELF: {
			// Shelf 的过渡斜率固定，Q 不参与计算
			const double v = 2.0 * std::sqrt(a) * s / std::sqrt(2.0);
			if (p_type == Filter::LOWSHELF) {
				b0 = a * ((a + 1.0) - (a - 1.0) * c + v);
				b1 = 2.0 * a * ((a - 1.0) - (a + 1.0) * c);
				b2 = a * ((a + 1.0) - (a - 1.0) * c - v);
				a0 = (a + 1.0) + (a - 1.0) * c + v;
				a1 = -2.0 * ((a - 1.0) + (a + 1.0) * c);
				a2 = (a + 1.0) + (a - 1.0) * c - v;
			} else {
				b0 = a * ((a + 1.0) + (a - 1.0) * c + v);
				b1 = -2.0 * a * ((a - 1.0) + (a + 1.0) * c);
				b2 = a * ((a + 1.0) + (a - 1.0) * c - v);
				a0 = (a + 1.0) - (a - 1.0) * c + v;
				a1 = 2.0 * ((a - 1.0) - (a + 1.0) * c);
				a2 = (a + 1.0) - (a - 1.0) * c - v;
			}
			break;
		}
		default:
			return Band();
	}
	b = { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
	return result;
}

}

#endif // FMOD_EQ_PREVIEW_RESPONSE_H
