#include "fmod_audio_waveform.h"

#include <fmod.hpp>
#include <fmod_errors.h>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <limits>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace godot {
	namespace {
		constexpr size_t MAX_ENTRIES = 64;
		constexpr uint64_t MAX_BINS = 4096;
		struct DecodeJob {
			uint64_t id = 0;
			PackedByteArray data;
			FMOD_CREATESOUNDEXINFO info{};
			std::vector<int> inclusion;
			std::string dls, encryption;
			std::atomic<bool> canceled{ false };
			std::vector<Vector2> envelope;
			double length = 0.0;
			FMOD_RESULT error = FMOD_OK;
		};

		float sample_value(const uint8_t* p_data, FMOD_SOUND_FORMAT p_format) {
			switch (p_format) {
			case FMOD_SOUND_FORMAT_PCM8:
				return float(*p_data) / 128.0f - 1.0f;
			case FMOD_SOUND_FORMAT_PCM16: {
				int16_t value;
				std::memcpy(&value, p_data, sizeof(value));
				return float(value) / 32768.0f;
			}
			case FMOD_SOUND_FORMAT_PCM24: {
				uint32_t raw = uint32_t(p_data[0]) | (uint32_t(p_data[1]) << 8) | (uint32_t(p_data[2]) << 16);
				if (raw & 0x800000U) raw |= 0xff000000U;
				int32_t value;
				std::memcpy(&value, &raw, sizeof(value));
				return float(value) / 8388608.0f;
			}
			case FMOD_SOUND_FORMAT_PCM32: {
				int32_t value;
				std::memcpy(&value, p_data, sizeof(value));
				return float(double(value) / 2147483648.0);
			}
			case FMOD_SOUND_FORMAT_PCMFLOAT: {
				float value;
				std::memcpy(&value, p_data, sizeof(value));
				return std::isfinite(value) ? std::clamp(value, -1.0f, 1.0f) : 0.0f;
			}
			default: return 0.0f;
			}
		}

		void decode(FMOD::System* p_system, DecodeJob& p_job) {
			FMOD::Sound* owner = nullptr;
			p_job.info.inclusionlist = p_job.inclusion.empty() ? nullptr : p_job.inclusion.data();
			p_job.info.dlsname = p_job.dls.empty() ? nullptr : p_job.dls.c_str();
			p_job.info.encryptionkey = p_job.encryption.empty() ? nullptr : p_job.encryption.c_str();
			FMOD_MODE mode = FMOD_OPENMEMORY | FMOD_OPENONLY | FMOD_CREATESTREAM | FMOD_LOOP_OFF | FMOD_2D;
			if (p_job.info.suggestedsoundtype == FMOD_SOUND_TYPE_RAW) mode |= FMOD_OPENRAW;
			p_job.error = p_system->createSound(reinterpret_cast<const char*>(p_job.data.ptr()), mode, &p_job.info, &owner);
			if (p_job.error != FMOD_OK) return;
			struct ReleaseSound { FMOD::Sound* sound; ~ReleaseSound() { sound->release(); } } release{ owner };
			FMOD::Sound* sound = owner;
			int subsounds = 0;
			owner->getNumSubSounds(&subsounds);
			if (subsounds > 0) {
				p_job.error = owner->getSubSound(std::clamp(p_job.info.initialsubsound, 0, subsounds - 1), &sound);
				if (p_job.error != FMOD_OK) return;
			}
			FMOD_SOUND_FORMAT format;
			int channels = 0, bits = 0;
			float frequency = 0.0f;
			unsigned int frame_count = 0;
			p_job.error = sound->getFormat(nullptr, &format, &channels, &bits);
			if (p_job.error != FMOD_OK) return;
			if (format < FMOD_SOUND_FORMAT_PCM8 || format > FMOD_SOUND_FORMAT_PCMFLOAT || channels < 1 || channels > 32 || bits < 8 || bits > 32) {
				p_job.error = FMOD_ERR_FORMAT;
				return;
			}
			sound->getDefaults(&frequency, nullptr);
			sound->getLength(&frame_count, FMOD_TIMEUNIT_PCM);
			if (frequency <= 0.0f || frame_count == 0) { p_job.error = FMOD_ERR_FORMAT; return; }
			const size_t sample_bytes = size_t(bits / 8);
			const size_t frame_bytes = sample_bytes * size_t(channels);
			const size_t chunk_bytes = frame_bytes * 4096;
			std::vector<uint8_t> buffer(chunk_bytes + frame_bytes);
			p_job.envelope.assign(size_t(std::min(uint64_t(frame_count), MAX_BINS)), Vector2(1.0f, -1.0f));
			uint64_t decoded = 0;
			size_t carry = 0;
			while (!p_job.canceled.load()) {
				unsigned int bytes_read = 0;
				const FMOD_RESULT result = sound->readData(buffer.data() + carry, static_cast<unsigned int>(chunk_bytes), &bytes_read);
				if (result != FMOD_OK && result != FMOD_ERR_FILE_EOF) { p_job.error = result; return; }
				const size_t frames = (carry + bytes_read) / frame_bytes;
				for (size_t frame = 0; frame < frames; frame++, decoded++) {
					const size_t bin = size_t(std::min(decoded * p_job.envelope.size() / frame_count, uint64_t(p_job.envelope.size() - 1)));
					Vector2& extrema = p_job.envelope[bin];
					for (int channel = 0; channel < channels; channel++) {
						const float sample = sample_value(buffer.data() + frame * frame_bytes + size_t(channel) * sample_bytes, format);
						extrema.x = std::min(extrema.x, sample);
						extrema.y = std::max(extrema.y, sample);
					}
				}
				carry += bytes_read;
				carry -= frames * frame_bytes;
				if (carry) std::memmove(buffer.data(), buffer.data() + frames * frame_bytes, carry);
				if (result == FMOD_ERR_FILE_EOF || bytes_read == 0) break;
			}
			for (Vector2& extrema : p_job.envelope) if (extrema.x > extrema.y) extrema = Vector2();
			p_job.length = double(decoded) / frequency;
		}
	}

	struct FmodAudioWaveformCache::State {
		struct Entry {
			uint64_t revision = 0, used = 0;
			Ref<FmodAudioWaveform> waveform;
			std::shared_ptr<DecodeJob> job;
		};
		std::unordered_map<uint64_t, Entry> entries;
		uint64_t clock = 0;
		std::mutex mutex;
		std::condition_variable available;
		std::deque<std::shared_ptr<DecodeJob>> pending, finished;
		bool stopping = false;
		std::thread worker;

		State() : worker([this]() {
			FMOD::System* system = nullptr;
			FMOD_RESULT system_error = FMOD::System_Create(&system);
			if (system_error == FMOD_OK) system_error = system->setOutput(FMOD_OUTPUTTYPE_NOSOUND_NRT);
			if (system_error == FMOD_OK) system_error = system->init(1, FMOD_INIT_NORMAL, nullptr);
			for (;;) {
				std::shared_ptr<DecodeJob> job;
				{
					std::unique_lock<std::mutex> lock(mutex);
					available.wait(lock, [this]() { return stopping || !pending.empty(); });
					if (stopping) break;
					job = pending.front();
					pending.pop_front();
				}
				if (!job->canceled.load()) {
					if (system_error == FMOD_OK) decode(system, *job);
					else job->error = system_error;
				}
				std::lock_guard<std::mutex> lock(mutex);
				finished.push_back(job);
			}
			if (system) system->release();
		}) {}

		~State() {
			for (auto& entry : entries) entry.second.job->canceled.store(true);
			{
				std::lock_guard<std::mutex> lock(mutex);
				stopping = true;
			}
			available.notify_one();
			worker.join();
		}
	};

	FmodAudioWaveformCache* FmodAudioWaveformCache::singleton = nullptr;
	FmodAudioWaveformCache::FmodAudioWaveformCache() {
		singleton = this;
		set_process(false);
		state = std::make_unique<State>();
	}
	FmodAudioWaveformCache::~FmodAudioWaveformCache() { state.reset(); if (singleton == this) singleton = nullptr; }
	void FmodAudioWaveformCache::_bind_methods() {}

	Ref<FmodAudioWaveform> FmodAudioWaveformCache::request(const Ref<FmodAudioStream>& p_stream) {
		if (p_stream.is_null() || !state) return Ref<FmodAudioWaveform>();
		const uint64_t id = p_stream->get_instance_id();
		auto found = state->entries.find(id);
		if (found != state->entries.end()) {
			if (found->second.revision == p_stream->get_waveform_revision()) {
				found->second.used = ++state->clock;
				return found->second.waveform;
			}
			found->second.job->canceled.store(true);
			state->entries.erase(found);
		}
		while (state->entries.size() >= MAX_ENTRIES) {
			auto oldest = state->entries.end();
			for (auto it = state->entries.begin(); it != state->entries.end(); ++it) {
				if (it->second.waveform->ready && (oldest == state->entries.end() || it->second.used < oldest->second.used)) oldest = it;
			}
			// 正在被解码的请求会正常完成，避免让使用中的视图一直等待
			if (oldest == state->entries.end()) break;
			oldest->second.job->canceled.store(true);
			state->entries.erase(oldest);
		}
		auto job = std::make_shared<DecodeJob>();
		job->id = id;
		job->data = p_stream->get_audio_data();
		job->info.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
		job->info.length = static_cast<unsigned int>(std::min(uint64_t(job->data.size()), uint64_t(std::numeric_limits<unsigned int>::max())));
		if (p_stream->get_length_override() > 0) job->info.length = std::min(job->info.length, static_cast<unsigned int>(p_stream->get_length_override()));
		job->info.fileoffset = p_stream->get_file_offset();
		job->info.decodebuffersize = p_stream->get_decode_buffer_size();
		job->info.initialsubsound = std::max(p_stream->get_initial_subsound(), 0);
		job->info.numsubsounds = p_stream->get_num_subsounds();
		const PackedInt32Array inclusion = p_stream->get_inclusion_list();
		if (!inclusion.is_empty()) job->inclusion.assign(inclusion.ptr(), inclusion.ptr() + inclusion.size());
		job->info.inclusionlistnum = int(job->inclusion.size());
		job->info.suggestedsoundtype = static_cast<FMOD_SOUND_TYPE>(p_stream->get_suggested_sound_type());
		job->info.defaultfrequency = p_stream->get_default_frequency();
		job->info.numchannels = p_stream->get_num_channels();
		job->info.format = static_cast<FMOD_SOUND_FORMAT>(p_stream->get_format());
		job->info.resamplermethod = static_cast<FMOD_DSP_RESAMPLER>(p_stream->get_resampler_method());
		job->info.ignoresetfilesystem = p_stream->is_ignore_set_file_system();
		String dls = p_stream->get_dls_name();
		if (dls.begins_with("res://") || dls.begins_with("user://")) dls = ProjectSettings::get_singleton()->globalize_path(dls);
		job->dls = dls.utf8().get_data();
		job->encryption = p_stream->get_encryption_key().utf8().get_data();
		State::Entry entry;
		entry.revision = p_stream->get_waveform_revision();
		entry.used = ++state->clock;
		entry.job = job;
		entry.waveform.instantiate();
		const Ref<FmodAudioWaveform> result = entry.waveform;
		state->entries.emplace(id, entry);
		if (job->data.is_empty() || uint64_t(job->data.size()) > std::numeric_limits<unsigned int>::max()) {
			result->ready = true;
			result->error = "No finite audio data to preview.";
			job->data = PackedByteArray();
			return result;
		}
		{
			std::lock_guard<std::mutex> lock(state->mutex);
			state->pending.push_back(job);
		}
		state->available.notify_one();
		set_process(true);
		return result;
	}

	void FmodAudioWaveformCache::_notification(int p_what) {
		if (p_what == NOTIFICATION_EXIT_TREE) { state.reset(); set_process(false); return; }
		if (p_what != NOTIFICATION_PROCESS || !state) return;
		std::deque<std::shared_ptr<DecodeJob>> finished;
		{
			std::lock_guard<std::mutex> lock(state->mutex);
			finished.swap(state->finished);
		}
		for (const auto& job : finished) {
			auto found = state->entries.find(job->id);
			if (found == state->entries.end() || found->second.job != job || job->canceled.load()) continue;
			const Ref<FmodAudioWaveform> waveform = found->second.waveform;
			waveform->length = job->length;
			if (job->error == FMOD_OK) {
				waveform->envelope.resize(int64_t(job->envelope.size()));
				for (int64_t i = 0; i < waveform->envelope.size(); i++) waveform->envelope.set(i, job->envelope[size_t(i)]);
			}
			else waveform->error = FMOD_ErrorString(job->error);
			waveform->ready = true;
			// 已完成任务不再持有压缩数据，缓存仅保留振幅包络
			job->data = PackedByteArray();
			std::vector<Vector2>().swap(job->envelope);
			waveform->emit_signal("updated");
		}
		bool active = false;
		for (const auto& entry : state->entries) active |= !entry.second.waveform->ready;
		set_process(active);
	}

	void FmodAudioWaveform::_bind_methods() {
		ClassDB::bind_method(D_METHOD("is_ready"), &FmodAudioWaveform::is_ready);
		ClassDB::bind_method(D_METHOD("get_error"), &FmodAudioWaveform::get_error);
		ClassDB::bind_method(D_METHOD("get_length"), &FmodAudioWaveform::get_length);
		ClassDB::bind_method(D_METHOD("get_envelope"), &FmodAudioWaveform::get_envelope);
		ADD_SIGNAL(MethodInfo("updated"));
	}

	PackedVector2Array FmodAudioWaveform::get_points(int p_width, float p_height) const {
		PackedVector2Array result;
		if (p_width <= 0 || p_height <= 0.0f || envelope.is_empty()) return result;
		result.resize(int64_t(p_width) * 2);
		for (int x = 0; x < p_width; x++) {
			const int64_t begin = int64_t(x) * envelope.size() / p_width;
			const int64_t end = MIN(MAX(begin + 1, int64_t(x + 1) * envelope.size() / p_width), envelope.size());
			float low = 1.0f, high = -1.0f;
			for (int64_t i = begin; i < end; i++) { low = MIN(low, envelope[i].x); high = MAX(high, envelope[i].y); }
			result.set(x * 2, Vector2(float(x) + 0.5f, (1.0f - high) * p_height * 0.5f));
			result.set(x * 2 + 1, Vector2(float(x) + 0.5f, (1.0f - low) * p_height * 0.5f));
		}
		return result;
	}

	void FmodAudioWaveformView::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_stream", "stream"), &FmodAudioWaveformView::set_stream);
		ClassDB::bind_method(D_METHOD("get_waveform"), &FmodAudioWaveformView::get_waveform);
		ADD_SIGNAL(MethodInfo("waveform_updated"));
	}
	FmodAudioWaveformView::FmodAudioWaveformView() { set_mouse_filter(MOUSE_FILTER_IGNORE); }
	FmodAudioWaveformView::~FmodAudioWaveformView() {
		if (stream.is_valid()) stream->disconnect("changed", callable_mp(this, &FmodAudioWaveformView::_request_waveform));
		if (waveform.is_valid()) waveform->disconnect("updated", callable_mp(this, &FmodAudioWaveformView::_waveform_updated));
	}
	void FmodAudioWaveformView::set_stream(const Ref<FmodAudioStream>& p_stream) {
		if (stream == p_stream) return;
		if (stream.is_valid()) stream->disconnect("changed", callable_mp(this, &FmodAudioWaveformView::_request_waveform));
		stream = p_stream;
		if (stream.is_valid()) stream->connect("changed", callable_mp(this, &FmodAudioWaveformView::_request_waveform));
		_request_waveform();
	}
	void FmodAudioWaveformView::_request_waveform() {
		set_visible(picker_presentation || (stream.is_valid() && stream->is_data_loaded()));
		if (waveform.is_valid()) waveform->disconnect("updated", callable_mp(this, &FmodAudioWaveformView::_waveform_updated));
		FmodAudioWaveformCache* cache = FmodAudioWaveformCache::get_singleton();
		waveform = cache ? cache->request(stream) : Ref<FmodAudioWaveform>();
		if (waveform.is_valid()) waveform->connect("updated", callable_mp(this, &FmodAudioWaveformView::_waveform_updated));
		_waveform_updated();
	}
	void FmodAudioWaveformView::_waveform_updated() {
		const float height = picker_presentation ? float(int(get_size().y)) * 0.5f : get_size().y;
		points = waveform.is_valid() ? waveform->get_points(int(get_size().x), height) : PackedVector2Array();
		if (picker_presentation) {
			for (int64_t i = 0; i < points.size(); i++) points.set(i, points[i] + Vector2(0.5f, 0.0f));
		}
		queue_redraw();
		emit_signal("waveform_updated");
	}
	void FmodAudioWaveformView::_notification(int p_what) {
		if (p_what == NOTIFICATION_ENTER_TREE) _request_waveform();
		else if (p_what == NOTIFICATION_RESIZED) _waveform_updated();
		else if (p_what == NOTIFICATION_THEME_CHANGED) queue_redraw();
	}
	void FmodAudioWaveformView::_draw() {
		if (!picker_presentation) draw_rect(Rect2(Vector2(), get_size()), get_theme_color("dark_color_2", "Editor"));
		if (!points.is_empty()) draw_multiline(points, get_theme_color("contrast_color_2", "Editor"));
	}
	void FmodAudioStreamEditorPlugin::_bind_methods() {}
	void FmodAudioStreamEditorPlugin::_notification(int p_what) {
		if (p_what == NOTIFICATION_ENTER_TREE) { cache = memnew(FmodAudioWaveformCache); add_child(cache); }
		else if (p_what == NOTIFICATION_EXIT_TREE && cache) { remove_child(cache); memdelete(cache); cache = nullptr; }
	}
}
