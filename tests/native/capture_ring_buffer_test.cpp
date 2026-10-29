#include "dsp/fmod_capture_ring_buffer.h"
#include "editor/audio_effects/fmod_capture_preview_envelope.h"

#ifdef NDEBUG
#error "Native test assertions must remain enabled"
#endif

#include <cassert>
#include <cstdio>
#include <deque>
#include <limits>
#include <random>
#include <utility>
#include <vector>

int main() {
	godot::AudioRingBuffer ring;
	ring.resize(3);
	const float stereo[] = {1, -1, 2, -2, 3, -3, 4, -4, 5, -5};
	assert(ring.write(stereo, 5, 2) == 3);
	assert(ring.space_left() == 0 && ring.data_left() == 3);
	assert(ring.write(stereo, 1, 2) == 0);
	godot::Vector2 output[8];
	assert(ring.peek(output, 8) == 3);
	assert(ring.data_left() == 3 && ring.space_left() == 0);
	for (int i = 0; i < 3; i++) assert(output[i].x == i + 1 && output[i].y == -(i + 1));
	assert(ring.read(output, 2) == 2);
	assert(output[0].x == 1 && output[0].y == -1);
	assert(output[1].x == 2 && output[1].y == -2);
	assert(ring.write(stereo + 6, 2, 2) == 2);
	assert(ring.peek(output, 8) == 3);
	assert(ring.data_left() == 3);
	for (int i = 0; i < 3; i++) assert(output[i].x == i + 3 && output[i].y == -(i + 3));
	assert(ring.read(output, 8) == 3);
	for (int i = 0; i < 3; i++) {
		assert(output[i].x == i + 3 && output[i].y == -(i + 3));
	}
	const float mono[] = {7, 8, 9, 10};
	assert(ring.write(mono, 4, 1) == 3);
	assert(ring.read(output, 8) == 3);
	for (int i = 0; i < 3; i++) {
		assert(output[i].x == i + 7 && output[i].y == i + 7);
	}
	const float quad[] = {1, 2, 99, 98, 3, 4, 97, 96};
	assert(ring.write(quad, 2, 4) == 2);
	assert(ring.read(output, 8) == 2);
	assert(output[0].x == 1 && output[0].y == 2);
	assert(output[1].x == 3 && output[1].y == 4);
	assert(ring.write(nullptr, 3, 2) == 0);
	assert(ring.write(stereo, 3, 0) == 0);
	assert(ring.read(nullptr, 3) == 0);
	assert(ring.peek(nullptr, 3) == 0);
	assert(ring.peek(output, 0) == 0);

	std::vector<fmod_capture_preview::PeakRange> ranges(2);
	const godot::Vector2 peak_samples[] = { { 0.25, 0.75 }, { 0.5, 0.125 }, { -0.25, -0.75 }, { -0.5, -0.125 } };
	fmod_capture_preview::build_envelope(peak_samples, 4, 4, ranges);
	assert(ranges[0].minimum == 0.125 && ranges[0].maximum == 0.75);
	assert(ranges[1].minimum == -0.75 && ranges[1].maximum == -0.125);
	fmod_capture_preview::build_envelope(peak_samples, 2, 4, ranges);
	assert(ranges[0].has_samples && !ranges[1].has_samples);
	const godot::Vector2 invalid[] = { { std::numeric_limits<godot::real_t>::quiet_NaN(), 0.5 } };
	fmod_capture_preview::build_envelope(invalid, 1, 4, ranges);
	assert(ranges[0].minimum == 0.5 && ranges[0].maximum == 0.5);
	fmod_capture_preview::build_envelope(nullptr, 0, 4, ranges);
	assert(!ranges[0].has_samples && !ranges[1].has_samples);
	assert(std::abs(fmod_capture_preview::time_step(10.0, 10) - 1.0) < 1e-9);
	assert(std::abs(fmod_capture_preview::time_step(10.0, 5) - 2.0) < 1e-9);
	assert(std::abs(fmod_capture_preview::time_step(0.1, 5) - 0.02) < 1e-9);

	// Compare arbitrary reads, overflow, format conversion and resize against a queue.
	std::mt19937 random(0x43415054);
	std::deque<std::pair<float, float>> expected;
	size_t capacity = 3;
	for (int trial = 0; trial < 20000; trial++) {
		const unsigned int action = random() % 16;
		if (action == 0) {
			capacity = 1 + random() % 257;
			ring.resize(capacity);
			expected.clear();
		} else if (action == 1) {
			ring.clear();
			expected.clear();
		} else if (action < 9) {
			const size_t frames = random() % 513;
			const int channels = 1 + random() % 6;
			std::vector<float> input(frames * channels);
			for (size_t i = 0; i < input.size(); i++) {
				input[i] = static_cast<float>(static_cast<int>(random() % 65536) - 32768) / 32768.0f;
			}
			const size_t accepted = std::min(frames, capacity - expected.size());
			assert(ring.write(input.data(), frames, channels) == accepted);
			for (size_t i = 0; i < accepted; i++) {
				expected.emplace_back(input[i * channels], input[i * channels + (channels > 1 ? 1 : 0)]);
			}
		} else if (action < 13) {
			const size_t frames = random() % 513;
			std::vector<godot::Vector2> actual(frames);
			const size_t count = std::min(frames, expected.size());
			assert(ring.read(actual.data(), frames) == count);
			for (size_t i = 0; i < count; i++) {
				assert(actual[i].x == expected.front().first && actual[i].y == expected.front().second);
				expected.pop_front();
			}
		} else {
			const size_t frames = random() % 513;
			std::vector<godot::Vector2> actual(frames);
			const size_t count = std::min(frames, expected.size());
			assert(ring.peek(actual.data(), frames) == count);
			for (size_t i = 0; i < count; i++) {
				assert(actual[i].x == expected[i].first && actual[i].y == expected[i].second);
			}
		}
		assert(ring.data_left() == expected.size());
		assert(ring.space_left() + ring.data_left() == capacity);
	}
	std::printf("Capture ring/preview tests passed (real_t: %zu bytes, 20000 queue operations, non-consuming snapshots and peak ranges)\n", sizeof(godot::real_t));
}
