#include "editor/audio_effects/fmod_compressor_preview_response.h"
#include <cassert>
#include <iostream>

using namespace fmod_compressor_preview;

int main() {
	assert(std::abs(output_db(0.0, -24.0, 4.0, 0.0, 1.0) + 18.0) < 1e-9);
	assert(std::abs(output_db(0.0, -24.0, 4.0, 6.0, 1.0) + 12.0) < 1e-9);
	assert(std::abs(std::pow(10.0, output_db(0.0, -24.0, 4.0, 0.0, 0.5) / 20.0) - (1.0 + std::pow(10.0, -18.0 / 20.0)) * 0.5) < 1e-9);
	for (double threshold : { -60.0, -24.0, 0.0 }) {
		for (double ratio : { 1.0, 4.0, 48.0 }) {
			for (double gain : { -20.0, 0.0, 20.0 }) {
				for (double mix : { 0.0, 0.01, 0.5, 1.0 }) {
					double previous = -1000.0;
					for (int input = -60; input <= 20; input++) {
						const double output = output_db(input, threshold, ratio, gain, mix);
						assert(std::isfinite(output) && output >= previous);
						assert(output >= -80.000001 && output <= 40.000001);
						previous = output;
						if (mix == 0.0) {
							assert(std::abs(output - input) < 1e-9);
						}
						if (mix == 1.0 && (ratio == 1.0 || input <= threshold)) {
							assert(std::abs(output - input - gain) < 1e-9);
						}
					}
					assert(std::abs(output_db(threshold - 1e-7, threshold, ratio, gain, mix) - output_db(threshold + 1e-7, threshold, ratio, gain, mix)) < 1e-6);
					if (mix > 0.0) {
						const double output = output_db(20.0, threshold, ratio, gain, mix);
						assert(std::abs(ratio_for_output(20.0, output, threshold, gain, mix) - ratio) < 1e-5);
					}
				}
			}
		}
	}
	assert(ratio_for_output(20.0, -100.0, -24.0, 0.0, 0.5) == 48.0);
	assert(ratio_for_output(20.0, 100.0, -24.0, 0.0, 1.0) == 1.0);
	std::cout << "Compressor preview response: threshold, ratio, makeup gain, dry/wet, continuity, inverse and boundary checks passed\n";
}
