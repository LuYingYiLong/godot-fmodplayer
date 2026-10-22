#include "fmod_audio_effect_chorus.h"
#include "core/fmod_server.h"
#include "core/fmod_system.h"
#include "playback/fmod_channel_group.h"

#include <fmod_errors.h>
#include <godot_cpp/core/class_db.hpp>
#include <atomic>
#include <cmath>
#include <cstring>
#include <vector>

namespace godot {
	static constexpr uint32_t CHORUS_DSP_USERDATA_MAGIC = 0x43484F52;
	static constexpr float CHORUS_TAU = 6.28318530717958647692f;

	struct FmodAudioEffectChorusSharedState {
		struct VoiceState {
			std::atomic<float> delay{ 12.0f };
			std::atomic<float> rate{ 1.0f };
			std::atomic<float> depth{ 0.0f };
			std::atomic<float> level{ 0.0f };
			std::atomic<float> cutoff{ static_cast<float>(FmodAudioEffectChorus::MS_CUTOFF_MAX) };
			std::atomic<float> pan{ 0.0f };
		};

		std::atomic<int> voice_count{ 2 };
		std::atomic<float> wet{ 0.5f };
		std::atomic<float> dry{ 1.0f };
		VoiceState voice[FmodAudioEffectChorus::MAX_VOICES];
	};

	struct ChorusDSPRuntime {
		std::vector<float> audio_buffer;
		std::vector<float> filter_history;
		uint64_t cycles[FmodAudioEffectChorus::MAX_VOICES] = {};
		unsigned int buffer_pos = 0;
		unsigned int buffer_mask = 0;
		int channels = 0;
		int sample_rate = 48000;
	};

	struct ChorusDSPUserData {
		uint32_t magic = CHORUS_DSP_USERDATA_MAGIC;
		std::shared_ptr<FmodAudioEffectChorusSharedState> chorus_state;
		ChorusDSPRuntime runtime;
	};

	static unsigned int _next_power_of_two(unsigned int p_value) {
		if (p_value <= 1) {
			return 1;
		}

		p_value--;
		p_value |= p_value >> 1;
		p_value |= p_value >> 2;
		p_value |= p_value >> 4;
		p_value |= p_value >> 8;
		p_value |= p_value >> 16;
		return p_value + 1;
	}

	static void _chorus_ensure_runtime(ChorusDSPRuntime& runtime, int channels, int sample_rate) {
		channels = MAX(channels, 1);
		sample_rate = MAX(sample_rate, 1);

		const float max_ms = static_cast<float>(FmodAudioEffectChorus::MAX_DELAY_MS + FmodAudioEffectChorus::MAX_DEPTH_MS + FmodAudioEffectChorus::MAX_WIDTH_MS) * 2.0f;
		const unsigned int desired_frames = _next_power_of_two(static_cast<unsigned int>((max_ms / 1000.0f) * static_cast<float>(sample_rate)) + 1);
		if (runtime.channels == channels && runtime.sample_rate == sample_rate && runtime.buffer_mask + 1 == desired_frames) {
			return;
		}

		runtime.channels = channels;
		runtime.sample_rate = sample_rate;
		runtime.buffer_mask = desired_frames - 1;
		runtime.buffer_pos = 0;
		memset(runtime.cycles, 0, sizeof(runtime.cycles));
		runtime.audio_buffer.assign(static_cast<size_t>(desired_frames) * static_cast<size_t>(channels), 0.0f);
		runtime.filter_history.assign(static_cast<size_t>(FmodAudioEffectChorus::MAX_VOICES) * static_cast<size_t>(channels), 0.0f);
	}

	static float _chorus_pan_scale(float p_pan, int p_channel) {
		if (p_channel == 0) {
			return CLAMP(1.0f - p_pan, 0.0f, 1.0f);
		}

		if (p_channel == 1) {
			return CLAMP(1.0f + p_pan, 0.0f, 1.0f);
		}

		return 1.0f;
	}

	static float _chorus_db_to_linear(float p_db) {
		return static_cast<float>(FmodUtils::db_to_linear(p_db));
	}

	static ChorusDSPUserData* _get_chorus_userdata(FMOD_DSP_STATE* dsp_state) {
		if (!dsp_state || !dsp_state->plugindata) {
			return nullptr;
		}

		ChorusDSPUserData* userdata = static_cast<ChorusDSPUserData*>(dsp_state->plugindata);
		return userdata->magic == CHORUS_DSP_USERDATA_MAGIC ? userdata : nullptr;
	}

	static FMOD_RESULT F_CALL chorus_dsp_create_callback(FMOD_DSP_STATE* dsp_state) {
		if (!dsp_state) {
			return FMOD_ERR_INVALID_PARAM;
		}

		void* userdata = nullptr;
		if (dsp_state->functions && dsp_state->functions->getuserdata) {
			dsp_state->functions->getuserdata(dsp_state, &userdata);
		}

		ChorusDSPUserData* chorus_userdata = static_cast<ChorusDSPUserData*>(userdata);
		if (!chorus_userdata || chorus_userdata->magic != CHORUS_DSP_USERDATA_MAGIC) {
			dsp_state->plugindata = nullptr;
			return FMOD_OK;
		}

		dsp_state->plugindata = chorus_userdata;
		return FMOD_OK;
	}

	static FMOD_RESULT F_CALL chorus_dsp_release_callback(FMOD_DSP_STATE* dsp_state) {
		if (dsp_state) {
			ChorusDSPUserData* userdata = static_cast<ChorusDSPUserData*>(dsp_state->plugindata);
			if (userdata && userdata->magic == CHORUS_DSP_USERDATA_MAGIC) {
				userdata->magic = 0;
				delete userdata;
			}
			dsp_state->plugindata = nullptr;
		}
		return FMOD_OK;
	}

	static FMOD_RESULT F_CALL chorus_dsp_process_callback(FMOD_DSP_STATE* dsp_state,
		unsigned int length,
		const FmodDSPBufferArray* inbufferarray,
		FmodDSPBufferArray* outbufferarray,
		FMOD_BOOL inputsidle,
		FmodDSPProcessOperation op) {

		static_cast<void>(inputsidle);

		if (!inbufferarray || !outbufferarray ||
			inbufferarray->numbuffers <= 0 || outbufferarray->numbuffers <= 0 ||
			!inbufferarray->buffernumchannels || !outbufferarray->buffernumchannels ||
			!inbufferarray->bufferchannelmask || !outbufferarray->bufferchannelmask) {
			return FMOD_ERR_INVALID_PARAM;
		}

		if (op == FMOD_PLAYER_DSP_PROCESS_QUERY) {
			outbufferarray->buffernumchannels[0] = inbufferarray->buffernumchannels[0];
			outbufferarray->bufferchannelmask[0] = inbufferarray->bufferchannelmask[0];
			outbufferarray->speakermode = inbufferarray->speakermode;
			return FMOD_OK;
		}

		if (op != FMOD_PLAYER_DSP_PROCESS_PERFORM) {
			return FMOD_OK;
		}

		if (!inbufferarray->buffers || !outbufferarray->buffers) {
			return FMOD_ERR_INVALID_PARAM;
		}

		const int input_channels = inbufferarray->buffernumchannels[0];
		const int output_channels = outbufferarray->buffernumchannels[0];
		const float* in_buffer = static_cast<const float*>(inbufferarray->buffers[0]);
		float* out_buffer = static_cast<float*>(outbufferarray->buffers[0]);
		if (!out_buffer || output_channels <= 0) {
			return FMOD_ERR_INVALID_PARAM;
		}

		if (!in_buffer || input_channels <= 0) {
			memset(out_buffer, 0, static_cast<size_t>(length) * static_cast<size_t>(output_channels) * sizeof(float));
			return FMOD_OK;
		}

		ChorusDSPUserData* userdata = _get_chorus_userdata(dsp_state);
		FmodAudioEffectChorusSharedState* state = userdata ? userdata->chorus_state.get() : nullptr;
		if (!state) {
			for (unsigned int frame = 0; frame < length; frame++) {
				for (int channel = 0; channel < output_channels; channel++) {
					const int input_channel = MIN(channel, input_channels - 1);
					out_buffer[frame * output_channels + channel] = in_buffer[frame * input_channels + input_channel];
				}
			}
			return FMOD_OK;
		}

		int sample_rate = 48000;
		if (dsp_state && dsp_state->functions && dsp_state->functions->getsamplerate) {
			int current_rate = 0;
			if (dsp_state->functions->getsamplerate(dsp_state, &current_rate) == FMOD_OK && current_rate > 0) {
				sample_rate = current_rate;
			}
		}

		ChorusDSPRuntime& runtime = userdata->runtime;
		_chorus_ensure_runtime(runtime, output_channels, sample_rate);

		const int channels = runtime.channels;
		const float dry_level = state->dry.load();

		for (unsigned int frame = 0; frame < length; frame++) {
			const unsigned int write_frame = (runtime.buffer_pos + frame) & runtime.buffer_mask;
			for (int channel = 0; channel < channels; channel++) {
				const int input_channel = MIN(channel, input_channels - 1);
				const float input = in_buffer[frame * input_channels + input_channel];
				runtime.audio_buffer[static_cast<size_t>(write_frame) * channels + channel] = input;
				out_buffer[frame * output_channels + channel] = input * dry_level;
			}
		}

		const int voice_count = CLAMP(state->voice_count.load(), 1, FmodAudioEffectChorus::MAX_VOICES);
		const float wet_level = state->wet.load();
		for (int voice_index = 0; voice_index < voice_count; voice_index++) {
			const FmodAudioEffectChorusSharedState::VoiceState& voice = state->voice[voice_index];
			const float cutoff = voice.cutoff.load();
			if (cutoff == 0.0f) {
				continue;
			}

			const double time_to_mix = static_cast<double>(length) / static_cast<double>(sample_rate);
			const double cycles_to_mix = time_to_mix * static_cast<double>(voice.rate.load());
			const uint64_t increment = length > 0 ? static_cast<uint64_t>(std::llround((cycles_to_mix / static_cast<double>(length)) * static_cast<double>(1 << FmodAudioEffectChorus::CYCLES_FRAC))) : 0;

			unsigned int delay_frames = static_cast<unsigned int>((voice.delay.load() / 1000.0f) * static_cast<float>(sample_rate));
			const float max_depth_frames = (voice.depth.load() / 1000.0f) * static_cast<float>(sample_rate);
			if ((static_cast<unsigned int>(max_depth_frames) + 10u) > delay_frames) {
				delay_frames += static_cast<int>(max_depth_frames) - static_cast<int>(delay_frames);
				delay_frames += 10;
			}
			delay_frames = MIN(delay_frames, runtime.buffer_mask);

			float c1 = 1.0f;
			float c2 = 0.0f;
			if (cutoff < static_cast<float>(FmodAudioEffectChorus::MS_CUTOFF_MAX)) {
				const float auxlp = std::exp(-CHORUS_TAU * cutoff / static_cast<float>(sample_rate));
				c1 = 1.0f - auxlp;
				c2 = auxlp;
			}

			const float level_linear = _chorus_db_to_linear(voice.level.load());
			const float pan = voice.pan.load();
			uint64_t local_cycles = runtime.cycles[voice_index];
			unsigned int local_rb_pos = runtime.buffer_pos;

			for (unsigned int frame = 0; frame < length; frame++) {
				const float phase = static_cast<float>(local_cycles & FmodAudioEffectChorus::CYCLES_MASK) / static_cast<float>(1 << FmodAudioEffectChorus::CYCLES_FRAC);
				const float wave_delay = std::sin(phase * CHORUS_TAU) * max_depth_frames;
				const int wave_delay_frames = static_cast<int>(std::llround(std::floor(wave_delay)));
				const float wave_delay_frac = wave_delay - static_cast<float>(wave_delay_frames);
				unsigned int rb_source = local_rb_pos;
				rb_source -= delay_frames;
				rb_source -= wave_delay_frames;

				for (int channel = 0; channel < channels; channel++) {
					float value = runtime.audio_buffer[static_cast<size_t>(rb_source & runtime.buffer_mask) * channels + channel];
					const float next_value = runtime.audio_buffer[static_cast<size_t>((rb_source - 1) & runtime.buffer_mask) * channels + channel];
					value += (next_value - value) * wave_delay_frac;

					float& history = runtime.filter_history[static_cast<size_t>(voice_index) * channels + channel];
					value = value * c1 + history * c2;
					history = value;

					const float voice_gain = wet_level * level_linear * _chorus_pan_scale(pan, channel);
					out_buffer[frame * output_channels + channel] += value * voice_gain;
				}

				local_cycles += increment;
				local_rb_pos++;
			}

			runtime.cycles[voice_index] += static_cast<uint64_t>(cycles_to_mix * static_cast<double>(1 << FmodAudioEffectChorus::CYCLES_FRAC));
		}

		runtime.buffer_pos += length;
		return FMOD_OK;
	}

	void FmodAudioEffectChorus::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_voice_count", "voices"), &FmodAudioEffectChorus::set_voice_count);
		ClassDB::bind_method(D_METHOD("get_voice_count"), &FmodAudioEffectChorus::get_voice_count);
		ADD_PROPERTY(PropertyInfo(Variant::INT, "voice_count", PROPERTY_HINT_RANGE, "1,4,1", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED), "set_voice_count", "get_voice_count");

		ClassDB::bind_method(D_METHOD("set_wet", "amount"), &FmodAudioEffectChorus::set_wet);
		ClassDB::bind_method(D_METHOD("get_wet"), &FmodAudioEffectChorus::get_wet);
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wet", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_wet", "get_wet");

		ClassDB::bind_method(D_METHOD("set_dry", "amount"), &FmodAudioEffectChorus::set_dry);
		ClassDB::bind_method(D_METHOD("get_dry"), &FmodAudioEffectChorus::get_dry);
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dry", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_dry", "get_dry");

		ClassDB::bind_method(D_METHOD("set_voice_delay_ms", "voice_idx", "delay_ms"), &FmodAudioEffectChorus::set_voice_delay_ms);
		ClassDB::bind_method(D_METHOD("get_voice_delay_ms", "voice_idx"), &FmodAudioEffectChorus::get_voice_delay_ms);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/1/delay_ms", PROPERTY_HINT_RANGE, "0,50,0.01,suffix:ms"), "set_voice_delay_ms", "get_voice_delay_ms", 0);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/2/delay_ms", PROPERTY_HINT_RANGE, "0,50,0.01,suffix:ms"), "set_voice_delay_ms", "get_voice_delay_ms", 1);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/3/delay_ms", PROPERTY_HINT_RANGE, "0,50,0.01,suffix:ms"), "set_voice_delay_ms", "get_voice_delay_ms", 2);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/4/delay_ms", PROPERTY_HINT_RANGE, "0,50,0.01,suffix:ms"), "set_voice_delay_ms", "get_voice_delay_ms", 3);

		ClassDB::bind_method(D_METHOD("set_voice_rate_hz", "voice_idx", "rate_hz"), &FmodAudioEffectChorus::set_voice_rate_hz);
		ClassDB::bind_method(D_METHOD("get_voice_rate_hz", "voice_idx"), &FmodAudioEffectChorus::get_voice_rate_hz);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/1/rate_hz", PROPERTY_HINT_RANGE, "0.1,20,0.1,suffix:Hz"), "set_voice_rate_hz", "get_voice_rate_hz", 0);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/2/rate_hz", PROPERTY_HINT_RANGE, "0.1,20,0.1,suffix:Hz"), "set_voice_rate_hz", "get_voice_rate_hz", 1);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/3/rate_hz", PROPERTY_HINT_RANGE, "0.1,20,0.1,suffix:Hz"), "set_voice_rate_hz", "get_voice_rate_hz", 2);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/4/rate_hz", PROPERTY_HINT_RANGE, "0.1,20,0.1,suffix:Hz"), "set_voice_rate_hz", "get_voice_rate_hz", 3);

		ClassDB::bind_method(D_METHOD("set_voice_depth_ms", "voice_idx", "depth_ms"), &FmodAudioEffectChorus::set_voice_depth_ms);
		ClassDB::bind_method(D_METHOD("get_voice_depth_ms", "voice_idx"), &FmodAudioEffectChorus::get_voice_depth_ms);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/1/depth_ms", PROPERTY_HINT_RANGE, "0,20,0.01,suffix:ms"), "set_voice_depth_ms", "get_voice_depth_ms", 0);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/2/depth_ms", PROPERTY_HINT_RANGE, "0,20,0.01,suffix:ms"), "set_voice_depth_ms", "get_voice_depth_ms", 1);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/3/depth_ms", PROPERTY_HINT_RANGE, "0,20,0.01,suffix:ms"), "set_voice_depth_ms", "get_voice_depth_ms", 2);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/4/depth_ms", PROPERTY_HINT_RANGE, "0,20,0.01,suffix:ms"), "set_voice_depth_ms", "get_voice_depth_ms", 3);

		ClassDB::bind_method(D_METHOD("set_voice_level_db", "voice_idx", "level_db"), &FmodAudioEffectChorus::set_voice_level_db);
		ClassDB::bind_method(D_METHOD("get_voice_level_db", "voice_idx"), &FmodAudioEffectChorus::get_voice_level_db);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/1/level_db", PROPERTY_HINT_RANGE, "-60,24,0.1,suffix:dB"), "set_voice_level_db", "get_voice_level_db", 0);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/2/level_db", PROPERTY_HINT_RANGE, "-60,24,0.1,suffix:dB"), "set_voice_level_db", "get_voice_level_db", 1);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/3/level_db", PROPERTY_HINT_RANGE, "-60,24,0.1,suffix:dB"), "set_voice_level_db", "get_voice_level_db", 2);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/4/level_db", PROPERTY_HINT_RANGE, "-60,24,0.1,suffix:dB"), "set_voice_level_db", "get_voice_level_db", 3);

		ClassDB::bind_method(D_METHOD("set_voice_cutoff_hz", "voice_idx", "cutoff_hz"), &FmodAudioEffectChorus::set_voice_cutoff_hz);
		ClassDB::bind_method(D_METHOD("get_voice_cutoff_hz", "voice_idx"), &FmodAudioEffectChorus::get_voice_cutoff_hz);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/1/cutoff_hz", PROPERTY_HINT_RANGE, "1,16000,1,suffix:Hz"), "set_voice_cutoff_hz", "get_voice_cutoff_hz", 0);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/2/cutoff_hz", PROPERTY_HINT_RANGE, "1,16000,1,suffix:Hz"), "set_voice_cutoff_hz", "get_voice_cutoff_hz", 1);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/3/cutoff_hz", PROPERTY_HINT_RANGE, "1,16000,1,suffix:Hz"), "set_voice_cutoff_hz", "get_voice_cutoff_hz", 2);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/4/cutoff_hz", PROPERTY_HINT_RANGE, "1,16000,1,suffix:Hz"), "set_voice_cutoff_hz", "get_voice_cutoff_hz", 3);

		ClassDB::bind_method(D_METHOD("set_voice_pan", "voice_idx", "pan"), &FmodAudioEffectChorus::set_voice_pan);
		ClassDB::bind_method(D_METHOD("get_voice_pan", "voice_idx"), &FmodAudioEffectChorus::get_voice_pan);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/1/pan", PROPERTY_HINT_RANGE, "-1,1,0.01"), "set_voice_pan", "get_voice_pan", 0);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/2/pan", PROPERTY_HINT_RANGE, "-1,1,0.01"), "set_voice_pan", "get_voice_pan", 1);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/3/pan", PROPERTY_HINT_RANGE, "-1,1,0.01"), "set_voice_pan", "get_voice_pan", 2);
		ADD_PROPERTYI(PropertyInfo(Variant::FLOAT, "voice/4/pan", PROPERTY_HINT_RANGE, "-1,1,0.01"), "set_voice_pan", "get_voice_pan", 3);
	}

	void FmodAudioEffectChorus::_validate_property(PropertyInfo& p_property) const {
		if (p_property.name.begins_with("voice/")) {
			int voice_idx = p_property.name.get_slicec('/', 1).to_int();
			if (voice_idx > voice_count) {
				p_property.usage = PROPERTY_USAGE_NONE;
			}
		}
	}

	FmodAudioEffectChorus::FmodAudioEffectChorus() {
		voice_count = 2;
		voice[0].delay = 15.0f;
		voice[1].delay = 20.0f;
		voice[0].rate = 0.8f;
		voice[1].rate = 1.2f;
		voice[0].depth = 2.0f;
		voice[1].depth = 3.0f;
		voice[0].cutoff = 8000.0f;
		voice[1].cutoff = 8000.0f;
		voice[0].pan = -0.5f;
		voice[1].pan = 0.5f;
		wet = 0.5f;
		dry = 1.0f;

		chorus_state = std::make_shared<FmodAudioEffectChorusSharedState>();
		_sync_shared_state();
	}

	FmodAudioEffectChorus::~FmodAudioEffectChorus() {
		remove_from_bus(bus);
		chorus_state.reset();
	}

	void FmodAudioEffectChorus::_sync_shared_state() {
		if (!chorus_state) {
			return;
		}

		chorus_state->voice_count.store(voice_count);
		chorus_state->wet.store(wet);
		chorus_state->dry.store(dry);
		for (int i = 0; i < MAX_VOICES; i++) {
			chorus_state->voice[i].delay.store(voice[i].delay);
			chorus_state->voice[i].rate.store(voice[i].rate);
			chorus_state->voice[i].depth.store(voice[i].depth);
			chorus_state->voice[i].level.store(voice[i].level);
			chorus_state->voice[i].cutoff.store(voice[i].cutoff);
			chorus_state->voice[i].pan.store(voice[i].pan);
		}
	}

	void FmodAudioEffectChorus::apply_to(Ref<FmodChannelGroup> p_bus) {
		ERR_FAIL_COND_MSG(p_bus.is_null(), "Invalid bus");

		if (bus.is_valid()) {
			remove_from_bus(bus);
		}

		bus = p_bus;

		Ref<FmodSystem> system = FmodServer::get_singleton()->get_main_system();
		ERR_FAIL_COND_MSG(system.is_null(), "FMOD system not initialized");

		Ref<FmodDSP> chorus_dsp = create_custom_dsp(system);
		if (!chorus_dsp.is_valid()) {
			ERR_PRINT("FmodAudioEffectChorus: Failed to create custom DSP");
			return;
		}

		bus->add_dsp(-1, chorus_dsp);
		dsp_chain.push_back(chorus_dsp);
	}

	Ref<FmodDSP> FmodAudioEffectChorus::create_custom_dsp(Ref<FmodSystem> system) {
		ERR_FAIL_COND_V(system.is_null(), Ref<FmodDSP>());

		FMOD::System* fmod_system = system->get_system();
		ERR_FAIL_COND_V(!fmod_system, Ref<FmodDSP>());

		FMOD_DSP_DESCRIPTION* base_desc = get_dsp_description();
		ERR_FAIL_COND_V(!base_desc, Ref<FmodDSP>());

		FMOD_DSP_DESCRIPTION desc;
		memcpy(&desc, base_desc, sizeof(FMOD_DSP_DESCRIPTION));
		if (!chorus_state) {
			chorus_state = std::make_shared<FmodAudioEffectChorusSharedState>();
			_sync_shared_state();
		}

		ChorusDSPUserData* dsp_userdata = new ChorusDSPUserData();
		dsp_userdata->chorus_state = chorus_state;
		desc.userdata = dsp_userdata;

		Ref<FmodDSP> dsp = system->create_dsp_from_description(desc);
		if (dsp.is_null()) {
			delete dsp_userdata;
			return Ref<FmodDSP>();
		}
		return dsp;
	}

	FMOD_DSP_DESCRIPTION* FmodAudioEffectChorus::get_dsp_description() {
		static FMOD_DSP_DESCRIPTION desc = {};
		static bool initialized = false;

		if (!initialized) {
			memset(&desc, 0, sizeof(desc));
			desc.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
			strncpy(desc.name, "GodotChorusDSP", sizeof(desc.name) - 1);
			desc.version = 1;
			fmod_configure_filter_description(desc);
			desc.create = chorus_dsp_create_callback;
			desc.release = chorus_dsp_release_callback;
			desc.process = fmod_dsp_process_callback<chorus_dsp_process_callback>();
			initialized = true;
		}

		return &desc;
	}

	void FmodAudioEffectChorus::set_voice_count(int p_voices) {
		ERR_FAIL_COND(p_voices < 1 || p_voices > MAX_VOICES);
		voice_count = p_voices;
		_sync_shared_state();
		notify_property_list_changed();
	}

	int FmodAudioEffectChorus::get_voice_count() const {
		return voice_count;
	}

	void FmodAudioEffectChorus::set_wet(float amount) {
		wet = CLAMP(amount, 0.0f, 1.0f);
		_sync_shared_state();
	}

	float FmodAudioEffectChorus::get_wet() const {
		return wet;
	}

	void FmodAudioEffectChorus::set_dry(float amount) {
		dry = CLAMP(amount, 0.0f, 1.0f);
		_sync_shared_state();
	}

	float FmodAudioEffectChorus::get_dry() const {
		return dry;
	}

	void FmodAudioEffectChorus::set_voice_delay_ms(int p_voice, float p_delay_ms) {
		ERR_FAIL_INDEX(p_voice, MAX_VOICES);
		voice[p_voice].delay = CLAMP(p_delay_ms, 0.0f, static_cast<float>(MAX_DELAY_MS));
		_sync_shared_state();
	}

	float FmodAudioEffectChorus::get_voice_delay_ms(int p_voice) const {
		ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);
		return voice[p_voice].delay;
	}

	void FmodAudioEffectChorus::set_voice_rate_hz(int p_voice, float p_rate_hz) {
		ERR_FAIL_INDEX(p_voice, MAX_VOICES);
		voice[p_voice].rate = CLAMP(p_rate_hz, 0.1f, 20.0f);
		_sync_shared_state();
	}

	float FmodAudioEffectChorus::get_voice_rate_hz(int p_voice) const {
		ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);
		return voice[p_voice].rate;
	}

	void FmodAudioEffectChorus::set_voice_depth_ms(int p_voice, float p_depth_ms) {
		ERR_FAIL_INDEX(p_voice, MAX_VOICES);
		voice[p_voice].depth = CLAMP(p_depth_ms, 0.0f, static_cast<float>(MAX_DEPTH_MS));
		_sync_shared_state();
	}

	float FmodAudioEffectChorus::get_voice_depth_ms(int p_voice) const {
		ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);
		return voice[p_voice].depth;
	}

	void FmodAudioEffectChorus::set_voice_level_db(int p_voice, float p_level_db) {
		ERR_FAIL_INDEX(p_voice, MAX_VOICES);
		voice[p_voice].level = CLAMP(p_level_db, -60.0f, 24.0f);
		_sync_shared_state();
	}

	float FmodAudioEffectChorus::get_voice_level_db(int p_voice) const {
		ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);
		return voice[p_voice].level;
	}

	void FmodAudioEffectChorus::set_voice_cutoff_hz(int p_voice, float p_cutoff_hz) {
		ERR_FAIL_INDEX(p_voice, MAX_VOICES);
		voice[p_voice].cutoff = CLAMP(p_cutoff_hz, 1.0f, static_cast<float>(MS_CUTOFF_MAX));
		_sync_shared_state();
	}

	float FmodAudioEffectChorus::get_voice_cutoff_hz(int p_voice) const {
		ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);
		return voice[p_voice].cutoff;
	}

	void FmodAudioEffectChorus::set_voice_pan(int p_voice, float p_pan) {
		ERR_FAIL_INDEX(p_voice, MAX_VOICES);
		voice[p_voice].pan = CLAMP(p_pan, -1.0f, 1.0f);
		_sync_shared_state();
	}

	float FmodAudioEffectChorus::get_voice_pan(int p_voice) const {
		ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);
		return voice[p_voice].pan;
	}
}
