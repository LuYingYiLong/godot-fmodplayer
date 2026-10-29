#ifndef FMOD_CAPTURE_PREVIEW_ENVELOPE_H
#define FMOD_CAPTURE_PREVIEW_ENVELOPE_H

#include <godot_cpp/variant/vector2.hpp>
#include <algorithm>
#include <cmath>
#include <vector>

namespace fmod_capture_preview {
	struct PeakRange {
		double minimum = 0.0;
		double maximum = 0.0;
		bool has_samples = false;
	};

	inline void build_envelope(const godot::Vector2* p_frames, size_t p_count, size_t p_capacity, std::vector<PeakRange>& r_ranges) {
		std::fill(r_ranges.begin(), r_ranges.end(), PeakRange{});
		if (!p_frames || p_capacity == 0 || r_ranges.empty()) {
			return;
		}
		for (size_t i = 0; i < std::min(p_count, p_capacity); i++) {
			PeakRange& range = r_ranges[std::min(i * r_ranges.size() / p_capacity, r_ranges.size() - 1)];
			for (double value : { double(p_frames[i].x), double(p_frames[i].y) }) {
				if (!std::isfinite(value)) continue;
				if (!range.has_samples) {
					range.minimum = range.maximum = value;
					range.has_samples = true;
				}
				else {
					range.minimum = std::min(range.minimum, value);
					range.maximum = std::max(range.maximum, value);
				}
			}
		}
	}

	inline double time_step(double p_duration, int p_max_intervals) {
		const double target = std::max(p_duration, 0.000001) / std::max(p_max_intervals, 1);
		const double magnitude = std::pow(10.0, std::floor(std::log10(target)));
		const double normalized = target / magnitude;
		return (normalized <= 1.000001 ? 1.0 : normalized <= 2.000001 ? 2.0 : normalized <= 5.000001 ? 5.0 : 10.0) * magnitude;
	}
}

#endif // FMOD_CAPTURE_PREVIEW_ENVELOPE_H
