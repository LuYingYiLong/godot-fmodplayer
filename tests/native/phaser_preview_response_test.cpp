#include "editor/audio_effects/fmod_phaser_preview_response.h"
#include <array>
#include <cassert>
#include <iostream>
#include <vector>

using namespace fmod_phaser_preview;

// 用运行时的时域递推生成脉冲，独立核对预览的复数频率响应
static std::vector<double> impulse_response(double minimum, double maximum, double feedback, double sample_rate) {
	const double d = (minimum + maximum) / sample_rate;
	const double a = (1.0 - d) / (1.0 + d);
	std::array<double, 6> history{};
	double previous = 0.0;
	std::vector<double> impulse(131072);
	for (size_t frame = 0; frame < impulse.size(); frame++) {
		double sample = (frame == 0 ? 1.0 : 0.0) + previous * feedback;
		for (int stage = 5; stage >= 0; stage--) {
			const double output = sample * -a + history[stage];
			history[stage] = output * a + sample;
			sample = output;
		}
		impulse[frame] = sample;
		previous = sample;
	}
	return impulse;
}

int main() {
	for (double sample_rate : { 44100.0, 48000.0 }) {
		for (const auto& parameters : { std::array<double, 3>{ 440.0, 1600.0, 0.7 }, { 10.0, 10000.0, 0.9 }, { 500.0, 6000.0, 0.1 } }) {
			const auto impulse = impulse_response(parameters[0], parameters[1], parameters[2], sample_rate);
			for (double frequency : { 20.0, 80.0, 500.0, 1000.0, 5000.0, 10000.0, 20000.0 }) {
				const std::complex<double> step = std::polar(1.0, -2.0 * PI * frequency / sample_rate);
				std::complex<double> z = 1.0;
				std::complex<double> measured = 0.0;
				const std::array<double, 4> depths{ 0.1, 1.0, 1.7, 4.0 };
				std::array<std::complex<double>, 4> measured_mixes{};
				for (size_t frame = 0; frame < impulse.size(); frame++) {
					const double value = impulse[frame];
					measured += value * z;
					for (size_t depth = 0; depth < depths.size(); depth++) {
						measured_mixes[depth] += ((frame == 0 ? 1.0 : 0.0) + value * depths[depth]) * z;
					}
					z *= step;
				}
				const auto expected = filtered_response(frequency, parameters[0], parameters[1], parameters[2], sample_rate);
				assert(std::abs(measured - expected) < 1e-7);
				for (size_t depth = 0; depth < depths.size(); depth++) {
					const auto mixed = mix_with_dry(expected, depths[depth]);
					assert(std::abs(measured_mixes[depth] - mixed) < 1e-7);
					assert(std::abs(gain_db(mixed) - 20.0 * std::log10(std::abs(measured_mixes[depth]))) < 1e-6);
				}
			}
		}
	}
	for (double sample_rate : { 8000.0, 44100.0, 48000.0, 96000.0 }) {
		for (const auto& range : { std::array<double, 2>{ 10.0, 10.0 }, { 10.0, 10000.0 }, { 10000.0, 10000.0 } }) {
			for (double feedback : { 0.1, 0.7, 0.9 }) {
				for (int point = 0; point <= 1000; point++) {
					const double frequency = 10.0 * std::pow(sample_rate * 0.049, double(point) / 1000.0);
					const double phase = phase_degrees(frequency, range[0], range[1], feedback, sample_rate);
					assert(std::isfinite(phase) && phase >= -180.0 && phase <= 180.0);
					const auto filtered = filtered_response(frequency, range[0], range[1], feedback, sample_rate);
					for (double depth : { 0.1, 1.0, 1.7, 4.0 }) {
						const double gain = gain_db(mix_with_dry(filtered, depth));
						assert(std::isfinite(gain) && gain >= -120.0 && gain <= 33.0);
					}
				}
			}
		}
	}
	double previous = 0.0;
	int wraps = 0;
	for (int point = 0; point < 768; point++) {
		const double frequency = 10.0 * std::pow(2000.0, double(point) / 767.0);
		const double phase = phase_degrees(frequency, 500.0, 6000.0, 0.7, 48000.0);
		if (point > 0 && std::abs(phase - previous) > 180.0) wraps++;
		previous = phase;
	}
	assert(wraps == 3);
	assert(std::abs(gain_db(std::complex<double>(0.0, 0.0)) + 120.0) < 1e-9);
	assert(std::abs(mix_with_dry(std::complex<double>(-1.0 / 1.7, 0.0), 1.7)) < 1e-12);
	std::cout << "Phaser preview response: six-stage impulse DFT, dry/wet output gain, depth, one-sample feedback, sample rates, boundaries, cancellation and phase wraps passed\n";
}
