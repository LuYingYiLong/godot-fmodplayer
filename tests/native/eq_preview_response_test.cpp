#include "editor/audio_effects/fmod_eq_preview_response.h"
#include "editor/audio_effects/fmod_filter_preview_response.h"
#include <cassert>
#include <iostream>

using namespace fmod_eq_preview;

int main() {
	for (double rate : { 44100.0, 48000.0, 96000.0 }) {
		for (int slope = 0; slope < 4; slope++) {
			for (double cutoff : { 1.0, 2000.0, 20500.0 }) {
				for (double resonance : { 0.0, 0.1, 0.5, 1.0 }) {
					const Band response = fmod_filter_preview::make_response(slope, cutoff, resonance, rate);
					for (double frequency : { 1.0, 20.0, 2000.0, 22000.0 }) {
						assert(std::isfinite(response.response_db(frequency, rate)));
					}
				}
			}
		}
		const Band lowpass6 = fmod_filter_preview::make_response(0, 2000.0, 0.5, rate);
		assert(std::abs(lowpass6.response_db(2000.0, rate) + 3.01029995664) < 0.000001);
		for (int slope : { 1, 2, 3 }) {
			const Band response = fmod_filter_preview::make_response(slope, 2000.0, 0.5, rate);
			assert(std::abs(response.response_db(2000.0, rate) - 20.0 * std::log10(0.5)) < 0.000001);
		}
		for (double frequency : { 1.0, 20.0, 1000.0, 2000.0, 20000.0 }) {
			assert(fmod_filter_preview::make_response(2, 2000.0, 0.5, rate).response_db(frequency, rate) == fmod_filter_preview::make_response(3, 2000.0, 0.5, rate).response_db(frequency, rate));
			assert(fmod_filter_preview::make_response(1, 1.0, 0.0, rate).response_db(frequency, rate) == fmod_filter_preview::make_response(1, 20.0, 0.1, rate).response_db(frequency, rate));
			assert(lowpass6.response_db(frequency, rate) == fmod_filter_preview::make_response(0, 2000.0, 1.0, rate).response_db(frequency, rate));
		}
		for (double gain : { -24.0, -6.0, 0.0, 6.0, 24.0 }) {
			const Band peak = make_band(Filter::PEAKING, 1000.0, gain, 0.707, rate);
			assert(std::abs(peak.response_db(1000.0, rate) - gain) < 0.000001);
			const Band inverse = make_band(Filter::PEAKING, 1000.0, -gain, 0.707, rate);
			for (double frequency : { 10.0, 100.0, 1000.0, 5000.0, 20000.0 }) {
				assert(std::abs(peak.response_db(frequency, rate) + inverse.response_db(frequency, rate)) < 0.000001);
			}
		}
		for (Filter type : { Filter::LOWPASS_6DB, Filter::LOWPASS_12DB, Filter::LOWPASS_24DB, Filter::LOWPASS_48DB, Filter::HIGHPASS_6DB, Filter::HIGHPASS_12DB, Filter::HIGHPASS_24DB, Filter::HIGHPASS_48DB }) {
			const Band band = make_band(type, 1000.0, 0.0, std::sqrt(0.5), rate);
			assert(std::abs(band.response_db(1000.0, rate) + 3.01029995664) < 0.000001);
		}
		assert(make_band(Filter::NOTCH, 1000.0, 0.0, 1.0, rate).response_db(1000.0, rate) < -100.0);
		assert(std::abs(make_band(Filter::BANDPASS, 1000.0, 0.0, 1.0, rate).response_db(1000.0, rate)) < 0.000001);
		for (Filter type : { Filter::LOWSHELF, Filter::HIGHSHELF }) {
			const Band shelf = make_band(type, 1000.0, 6.0, 0.707, rate);
			assert(std::abs(shelf.response_db(1000.0, rate) - 3.0) < 0.000001);
			assert(std::abs(shelf.response_db(type == Filter::LOWSHELF ? 1.0 : rate * 0.499, rate) - 6.0) < 0.001);
		}
		for (int filter = 0; filter <= int(Filter::HIGHPASS_6DB); filter++) {
			for (double cutoff : { 10.0, 1000.0, 22000.0 }) {
				for (double q : { 0.01, 0.707, 10.0 }) {
					const Band band = make_band(Filter(filter), cutoff, 24.0, q, rate);
					for (int i = 0; i < 384; i++) {
						const double frequency = 10.0 * std::pow(2200.0, double(i) / 383.0);
						const double db = band.response_db(frequency, rate);
						assert(std::isfinite(db));
						if (filter == int(Filter::DISABLED) || filter == int(Filter::ALLPASS)) {
							assert(std::abs(db) < 0.000001);
						}
					}
				}
			}
		}
	}
	std::cout << "EQ/Filter preview response: peaking, inverse, cutoff, shelf, notch, all-pass, slope mapping and boundary checks passed\n";
}
