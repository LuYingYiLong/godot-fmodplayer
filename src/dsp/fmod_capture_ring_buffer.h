#ifndef FMOD_CAPTURE_RING_BUFFER_H
#define FMOD_CAPTURE_RING_BUFFER_H

#include <godot_cpp/variant/vector2.hpp>
#include <algorithm>
#include <cstring>
#include <vector>

namespace godot {
	class AudioRingBuffer {
	private:
		// Positions and counts are stereo frames, never individual samples.
		std::vector<Vector2> buffer;
		size_t write_pos = 0;
		size_t read_pos = 0;
		size_t used_frames = 0;

		static void copy_input(Vector2* destination, const float* source, size_t frames, int channels) {
			if constexpr (sizeof(real_t) == sizeof(float) && sizeof(Vector2) == sizeof(float) * 2) {
				if (channels == 2) {
					std::memcpy(destination, source, frames * sizeof(Vector2));
					return;
				}
			}
			const int right_channel = channels > 1 ? 1 : 0;
			for (size_t i = 0; i < frames; i++) {
				destination[i] = Vector2(source[i * channels], source[i * channels + right_channel]);
			}
		}

	public:
		void resize(size_t frames) {
			buffer.resize(std::max(frames, size_t(1)));
			clear();
		}

		void clear() {
			write_pos = 0;
			read_pos = 0;
			used_frames = 0;
		}

		size_t space_left() const {
			return buffer.size() - used_frames;
		}

		size_t data_left() const {
			return used_frames;
		}

		size_t write(const float* data, size_t frames, int channels) {
			if (!data || channels <= 0) return 0;
			const size_t to_write = std::min(frames, space_left());
			if (to_write == 0) return 0;
			const size_t first = std::min(to_write, buffer.size() - write_pos);
			copy_input(buffer.data() + write_pos, data, first, channels);
			if (first < to_write) {
				copy_input(buffer.data(), data + first * channels, to_write - first, channels);
			}
			write_pos += to_write;
			if (write_pos >= buffer.size()) write_pos -= buffer.size();
			used_frames += to_write;
			return to_write;
		}

		size_t read(Vector2* data, size_t frames) {
			if (!data) return 0;
			const size_t to_read = std::min(frames, data_left());
			if (to_read == 0) return 0;
			const size_t first = std::min(to_read, buffer.size() - read_pos);
			std::memcpy(data, buffer.data() + read_pos, first * sizeof(Vector2));
			if (first < to_read) {
				std::memcpy(data + first, buffer.data(), (to_read - first) * sizeof(Vector2));
			}
			read_pos += to_read;
			if (read_pos >= buffer.size()) read_pos -= buffer.size();
			used_frames -= to_read;
			return to_read;
		}

		// Copy unread stereo frames without advancing the consumer.
		size_t peek(Vector2* data, size_t frames) const {
			if (!data) return 0;
			const size_t to_copy = std::min(frames, data_left());
			if (to_copy == 0) return 0;
			const size_t first = std::min(to_copy, buffer.size() - read_pos);
			std::memcpy(data, buffer.data() + read_pos, first * sizeof(Vector2));
			if (first < to_copy) {
				std::memcpy(data + first, buffer.data(), (to_copy - first) * sizeof(Vector2));
			}
			return to_copy;
		}
	};

}

#endif // FMOD_CAPTURE_RING_BUFFER_H
