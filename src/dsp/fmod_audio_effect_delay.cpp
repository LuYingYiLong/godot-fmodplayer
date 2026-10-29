#include "fmod_audio_effect_delay.h"
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
	static constexpr uint32_t DELAY_DSP_USERDATA_MAGIC = 0x44454C59;
	static constexpr float DELAY_MAX_MS = 3000.0f;
	static constexpr float DELAY_TAU = 6.28318530717958647692f;

	struct FmodAudioEffectDelaySharedState {
		std::atomic<float> dry{ 1.0f };

		std::atomic<bool> tap_1_active{ true };
		std::atomic<float> tap_1_delay_ms{ 250.0f };
		std::atomic<float> tap_1_level_db{ -6.0f };
		std::atomic<float> tap_1_pan{ 0.2f };

		std::atomic<bool> tap_2_active{ true };
		std::atomic<float> tap_2_delay_ms{ 500.0f };
		std::atomic<float> tap_2_level_db{ -12.0f };
		std::atomic<float> tap_2_pan{ -0.4f };

		std::atomic<bool> feedback_active{ false };
		std::atomic<float> feedback_delay_ms{ 340.0f };
		std::atomic<float> feedback_level_db{ -6.0f };
		std::atomic<float> feedback_lowpass{ 16000.0f };
	};

	struct DelayDSPRuntime {
		std::vector<float> ring_buffer;
		std::vector<float> feedback_buffer;
		std::vector<float> lowpass_history;
		unsigned int ring_buffer_pos = 0;
		unsigned int feedback_buffer_pos = 0;
		unsigned int ring_buffer_mask = 0;
		int channels = 0;
		int sample_rate = 48000;
	};

	struct DelayDSPUserData {
		uint32_t magic = DELAY_DSP_USERDATA_MAGIC;
		std::shared_ptr<FmodAudioEffectDelaySharedState> delay_state;
		DelayDSPRuntime runtime;
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

	static void _delay_ensure_runtime(DelayDSPRuntime& runtime, int channels, int sample_rate) {
		channels = MAX(channels, 1);
		sample_rate = MAX(sample_rate, 1);

		const unsigned int desired_frames = _next_power_of_two(static_cast<unsigned int>(((DELAY_MAX_MS + 100.0f) / 1000.0f) * static_cast<float>(sample_rate)) + 1);
		if (runtime.channels == channels && runtime.sample_rate == sample_rate && runtime.ring_buffer_mask + 1 == desired_frames) {
			return;
		}

		runtime.channels = channels;
		runtime.sample_rate = sample_rate;
		runtime.ring_buffer_mask = desired_frames - 1;
		runtime.ring_buffer_pos = 0;
		runtime.feedback_buffer_pos = 0;
		runtime.ring_buffer.assign(static_cast<size_t>(desired_frames) * static_cast<size_t>(channels), 0.0f);
		runtime.feedback_buffer.assign(static_cast<size_t>(desired_frames) * static_cast<size_t>(channels), 0.0f);
		runtime.lowpass_history.assign(static_cast<size_t>(channels), 0.0f);
	}

	static float _delay_pan_scale(float p_pan, int p_channel) {
		if (p_channel == 0) {
			return CLAMP(1.0f - p_pan, 0.0f, 1.0f);
		}

		if (p_channel == 1) {
			return CLAMP(1.0f + p_pan, 0.0f, 1.0f);
		}

		return 1.0f;
	}

	static float _delay_db_to_linear(float p_db) {
		return static_cast<float>(FmodUtils::db_to_linear(p_db));
	}

	static DelayDSPUserData* _get_delay_userdata(FMOD_DSP_STATE* dsp_state) {
		if (!dsp_state || !dsp_state->plugindata) {
			return nullptr;
		}

		DelayDSPUserData* userdata = static_cast<DelayDSPUserData*>(dsp_state->plugindata);
		return userdata->magic == DELAY_DSP_USERDATA_MAGIC ? userdata : nullptr;
	}

	static FMOD_RESULT F_CALL delay_dsp_create_callback(FMOD_DSP_STATE* dsp_state) {
		if (!dsp_state) {
			return FMOD_ERR_INVALID_PARAM;
		}

		void* userdata = nullptr;
		if (dsp_state->functions && dsp_state->functions->getuserdata) {
			dsp_state->functions->getuserdata(dsp_state, &userdata);
		}

		DelayDSPUserData* delay_userdata = static_cast<DelayDSPUserData*>(userdata);
		if (!delay_userdata || delay_userdata->magic != DELAY_DSP_USERDATA_MAGIC) {
			dsp_state->plugindata = nullptr;
			return FMOD_OK;
		}

		dsp_state->plugindata = delay_userdata;
		return FMOD_OK;
	}

	static FMOD_RESULT F_CALL delay_dsp_release_callback(FMOD_DSP_STATE* dsp_state) {
		if (dsp_state) {
			DelayDSPUserData* userdata = static_cast<DelayDSPUserData*>(dsp_state->plugindata);
			if (userdata && userdata->magic == DELAY_DSP_USERDATA_MAGIC) {
				userdata->magic = 0;
				delete userdata;
			}
			dsp_state->plugindata = nullptr;
		}
		return FMOD_OK;
	}

	static FMOD_RESULT F_CALL delay_dsp_process_callback(FMOD_DSP_STATE* dsp_state,
		unsigned int length,
		const FMOD_DSP_BUFFER* inbufferarray,
		const FMOD_DSP_BUFFER* outbufferarray) {

		if (!inbufferarray || !outbufferarray) {
			return FMOD_ERR_INVALID_PARAM;
		}

		if (!inbufferarray->buffer || !outbufferarray->buffer) {
			return FMOD_ERR_INVALID_PARAM;
		}

		const int input_channels = inbufferarray->numchannels;
		const int output_channels = outbufferarray->numchannels;
		const float* in_buffer = static_cast<const float*>(inbufferarray->buffer);
		float* out_buffer = static_cast<float*>(outbufferarray->buffer);
		if (!out_buffer || output_channels <= 0) {
			return FMOD_ERR_INVALID_PARAM;
		}

		if (!in_buffer || input_channels <= 0) {
			memset(out_buffer, 0, static_cast<size_t>(length) * static_cast<size_t>(output_channels) * sizeof(float));
			return FMOD_OK;
		}

		DelayDSPUserData* userdata = _get_delay_userdata(dsp_state);
		FmodAudioEffectDelaySharedState* state = userdata ? userdata->delay_state.get() : nullptr;
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

		DelayDSPRuntime& runtime = userdata->runtime;
		_delay_ensure_runtime(runtime, output_channels, sample_rate);

		const float dry_level = state->dry.load();
		const bool tap_1_active = state->tap_1_active.load();
		const float tap_1_level = tap_1_active ? _delay_db_to_linear(state->tap_1_level_db.load()) : 0.0f;
		const unsigned int tap_1_delay_frames = MIN(static_cast<unsigned int>((state->tap_1_delay_ms.load() / 1000.0f) * sample_rate), runtime.ring_buffer_mask);
		const float tap_1_pan = state->tap_1_pan.load();

		const bool tap_2_active = state->tap_2_active.load();
		const float tap_2_level = tap_2_active ? _delay_db_to_linear(state->tap_2_level_db.load()) : 0.0f;
		const unsigned int tap_2_delay_frames = MIN(static_cast<unsigned int>((state->tap_2_delay_ms.load() / 1000.0f) * sample_rate), runtime.ring_buffer_mask);
		const float tap_2_pan = state->tap_2_pan.load();

		const bool feedback_active = state->feedback_active.load();
		const float feedback_level = feedback_active ? _delay_db_to_linear(state->feedback_level_db.load()) : 0.0f;
		const unsigned int feedback_delay_frames = MAX(1u, MIN(static_cast<unsigned int>((state->feedback_delay_ms.load() / 1000.0f) * sample_rate), runtime.ring_buffer_mask + 1));
		const float feedback_lowpass = state->feedback_lowpass.load();
		const float lpf_c = std::exp(-DELAY_TAU * feedback_lowpass / static_cast<float>(sample_rate));
		const float lpf_ic = 1.0f - lpf_c;

		const int channels = runtime.channels;
		for (unsigned int frame = 0; frame < length; frame++) {
			const unsigned int write_frame = runtime.ring_buffer_pos & runtime.ring_buffer_mask;
			const unsigned int tap_1_frame = (runtime.ring_buffer_pos - tap_1_delay_frames) & runtime.ring_buffer_mask;
			const unsigned int tap_2_frame = (runtime.ring_buffer_pos - tap_2_delay_frames) & runtime.ring_buffer_mask;

			for (int channel = 0; channel < channels; channel++) {
				const int input_channel = MIN(channel, input_channels - 1);
				const float input = in_buffer[frame * input_channels + input_channel];
				runtime.ring_buffer[static_cast<size_t>(write_frame) * channels + channel] = input;

				float output = input * dry_level;
				output += runtime.ring_buffer[static_cast<size_t>(tap_1_frame) * channels + channel] * tap_1_level * _delay_pan_scale(tap_1_pan, channel);
				output += runtime.ring_buffer[static_cast<size_t>(tap_2_frame) * channels + channel] * tap_2_level * _delay_pan_scale(tap_2_pan, channel);
				output += runtime.feedback_buffer[static_cast<size_t>(runtime.feedback_buffer_pos) * channels + channel];

				float feedback_in = output * feedback_level * lpf_ic + runtime.lowpass_history[channel] * lpf_c;
				if (std::abs(feedback_in) < 1e-20f) {
					feedback_in = 0.0f;
				}

				runtime.lowpass_history[channel] = feedback_in;
				runtime.feedback_buffer[static_cast<size_t>(runtime.feedback_buffer_pos) * channels + channel] = feedback_in;
				out_buffer[frame * output_channels + channel] = output;
			}

			runtime.ring_buffer_pos++;
			if ((++runtime.feedback_buffer_pos) >= feedback_delay_frames) {
				runtime.feedback_buffer_pos = 0;
			}
		}

		return FMOD_OK;
	}

	void FmodAudioEffectDelay::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_dry", "amount"), &FmodAudioEffectDelay::set_dry);
		ClassDB::bind_method(D_METHOD("get_dry"), &FmodAudioEffectDelay::get_dry);
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dry", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_dry", "get_dry");

		ClassDB::bind_method(D_METHOD("set_tap1_active", "active"), &FmodAudioEffectDelay::set_tap1_active);
		ClassDB::bind_method(D_METHOD("is_tap1_active"), &FmodAudioEffectDelay::is_tap1_active);

		ClassDB::bind_method(D_METHOD("set_tap1_delay_ms", "delay_ms"), &FmodAudioEffectDelay::set_tap1_delay_ms);
		ClassDB::bind_method(D_METHOD("get_tap1_delay_ms"), &FmodAudioEffectDelay::get_tap1_delay_ms);

		ClassDB::bind_method(D_METHOD("set_tap1_level_db", "level_db"), &FmodAudioEffectDelay::set_tap1_level_db);
		ClassDB::bind_method(D_METHOD("get_tap1_level_db"), &FmodAudioEffectDelay::get_tap1_level_db);

		ClassDB::bind_method(D_METHOD("set_tap1_pan", "pan"), &FmodAudioEffectDelay::set_tap1_pan);
		ClassDB::bind_method(D_METHOD("get_tap1_pan"), &FmodAudioEffectDelay::get_tap1_pan);

		ClassDB::bind_method(D_METHOD("set_tap2_active", "active"), &FmodAudioEffectDelay::set_tap2_active);
		ClassDB::bind_method(D_METHOD("is_tap2_active"), &FmodAudioEffectDelay::is_tap2_active);

		ClassDB::bind_method(D_METHOD("set_tap2_delay_ms", "delay_ms"), &FmodAudioEffectDelay::set_tap2_delay_ms);
		ClassDB::bind_method(D_METHOD("get_tap2_delay_ms"), &FmodAudioEffectDelay::get_tap2_delay_ms);

		ClassDB::bind_method(D_METHOD("set_tap2_level_db", "level_db"), &FmodAudioEffectDelay::set_tap2_level_db);
		ClassDB::bind_method(D_METHOD("get_tap2_level_db"), &FmodAudioEffectDelay::get_tap2_level_db);

		ClassDB::bind_method(D_METHOD("set_tap2_pan", "pan"), &FmodAudioEffectDelay::set_tap2_pan);
		ClassDB::bind_method(D_METHOD("get_tap2_pan"), &FmodAudioEffectDelay::get_tap2_pan);

		ClassDB::bind_method(D_METHOD("set_feedback_active", "active"), &FmodAudioEffectDelay::set_feedback_active);
		ClassDB::bind_method(D_METHOD("is_feedback_active"), &FmodAudioEffectDelay::is_feedback_active);

		ClassDB::bind_method(D_METHOD("set_feedback_delay_ms", "delay_ms"), &FmodAudioEffectDelay::set_feedback_delay_ms);
		ClassDB::bind_method(D_METHOD("get_feedback_delay_ms"), &FmodAudioEffectDelay::get_feedback_delay_ms);

		ClassDB::bind_method(D_METHOD("set_feedback_level_db", "level_db"), &FmodAudioEffectDelay::set_feedback_level_db);
		ClassDB::bind_method(D_METHOD("get_feedback_level_db"), &FmodAudioEffectDelay::get_feedback_level_db);

		ClassDB::bind_method(D_METHOD("set_feedback_lowpass", "lowpass"), &FmodAudioEffectDelay::set_feedback_lowpass);
		ClassDB::bind_method(D_METHOD("get_feedback_lowpass"), &FmodAudioEffectDelay::get_feedback_lowpass);

		ADD_GROUP("Tap 1", "tap1_");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "tap1_active"), "set_tap1_active", "is_tap1_active");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tap1_delay_ms", PROPERTY_HINT_RANGE, "0,3000,1,suffix:ms"), "set_tap1_delay_ms", "get_tap1_delay_ms");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tap1_level_db", PROPERTY_HINT_RANGE, "-60,0,0.1,suffix:dB"), "set_tap1_level_db", "get_tap1_level_db");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tap1_pan", PROPERTY_HINT_RANGE, "-1,1,0.01"), "set_tap1_pan", "get_tap1_pan");

		ADD_GROUP("Tap 2", "tap2_");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "tap2_active"), "set_tap2_active", "is_tap2_active");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tap2_delay_ms", PROPERTY_HINT_RANGE, "0,3000,1,suffix:ms"), "set_tap2_delay_ms", "get_tap2_delay_ms");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tap2_level_db", PROPERTY_HINT_RANGE, "-60,0,0.1,suffix:dB"), "set_tap2_level_db", "get_tap2_level_db");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tap2_pan", PROPERTY_HINT_RANGE, "-1,1,0.01"), "set_tap2_pan", "get_tap2_pan");

		ADD_GROUP("Feedback", "feedback_");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "feedback_active"), "set_feedback_active", "is_feedback_active");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "feedback_delay_ms", PROPERTY_HINT_RANGE, "0,3000,1,suffix:ms"), "set_feedback_delay_ms", "get_feedback_delay_ms");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "feedback_level_db", PROPERTY_HINT_RANGE, "-60,0,0.1,suffix:dB"), "set_feedback_level_db", "get_feedback_level_db");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "feedback_lowpass", PROPERTY_HINT_RANGE, "1,16000,1,suffix:Hz"), "set_feedback_lowpass", "get_feedback_lowpass");
	}

	FmodAudioEffectDelay::FmodAudioEffectDelay() {
		delay_state = std::make_shared<FmodAudioEffectDelaySharedState>();
		_sync_shared_state();
	}

	FmodAudioEffectDelay::~FmodAudioEffectDelay() {
		remove_from_bus(bus);
		delay_state.reset();
	}

	void FmodAudioEffectDelay::_sync_shared_state() {
		if (!delay_state) {
			return;
		}

		delay_state->dry.store(dry);
		delay_state->tap_1_active.store(tap_1_active);
		delay_state->tap_1_delay_ms.store(tap_1_delay_ms);
		delay_state->tap_1_level_db.store(tap_1_level);
		delay_state->tap_1_pan.store(tap_1_pan);
		delay_state->tap_2_active.store(tap_2_active);
		delay_state->tap_2_delay_ms.store(tap_2_delay_ms);
		delay_state->tap_2_level_db.store(tap_2_level);
		delay_state->tap_2_pan.store(tap_2_pan);
		delay_state->feedback_active.store(feedback_active);
		delay_state->feedback_delay_ms.store(feedback_delay_ms);
		delay_state->feedback_level_db.store(feedback_level);
		delay_state->feedback_lowpass.store(feedback_lowpass);
	}

	void FmodAudioEffectDelay::apply_to(Ref<FmodChannelGroup> p_bus) {
		ERR_FAIL_COND_MSG(p_bus.is_null(), "Invalid bus");

		if (bus.is_valid()) {
			remove_from_bus(bus);
		}

		bus = p_bus;

		Ref<FmodSystem> system = FmodServer::get_singleton()->get_main_system();
		ERR_FAIL_COND_MSG(system.is_null(), "FMOD system not initialized");

		Ref<FmodDSP> delay_dsp = create_custom_dsp(system);
		if (!delay_dsp.is_valid()) {
			ERR_PRINT("FmodAudioEffectDelay: Failed to create custom DSP");
			return;
		}

		bus->add_dsp(-1, delay_dsp);
		dsp_chain.push_back(delay_dsp);
	}

	Ref<FmodDSP> FmodAudioEffectDelay::create_custom_dsp(Ref<FmodSystem> system) {
		ERR_FAIL_COND_V(system.is_null(), Ref<FmodDSP>());

		FMOD::System* fmod_system = system->get_system();
		ERR_FAIL_COND_V(!fmod_system, Ref<FmodDSP>());

		FMOD_DSP_DESCRIPTION* base_desc = get_dsp_description();
		ERR_FAIL_COND_V(!base_desc, Ref<FmodDSP>());

		FMOD_DSP_DESCRIPTION desc;
		memcpy(&desc, base_desc, sizeof(FMOD_DSP_DESCRIPTION));
		if (!delay_state) {
			delay_state = std::make_shared<FmodAudioEffectDelaySharedState>();
			_sync_shared_state();
		}

		DelayDSPUserData* dsp_userdata = new DelayDSPUserData();
		dsp_userdata->delay_state = delay_state;
		desc.userdata = dsp_userdata;

		Ref<FmodDSP> dsp = system->create_dsp_from_description(desc);
		if (dsp.is_null()) {
			delete dsp_userdata;
			return Ref<FmodDSP>();
		}
		return dsp;
	}

	FMOD_DSP_DESCRIPTION* FmodAudioEffectDelay::get_dsp_description() {
		static FMOD_DSP_DESCRIPTION desc = {};
		static bool initialized = false;

		if (!initialized) {
			memset(&desc, 0, sizeof(desc));
			desc.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
			strncpy(desc.name, "GodotDelayDSP", sizeof(desc.name) - 1);
			desc.version = 1;
			fmod_configure_filter_description(desc);
			desc.create = delay_dsp_create_callback;
			desc.release = delay_dsp_release_callback;
			desc.process = delay_dsp_process_callback;
			initialized = true;
		}

		return &desc;
	}

	void FmodAudioEffectDelay::set_dry(float p_dry) {
		p_dry = CLAMP(p_dry, 0.0f, 1.0f);
		if (dry == p_dry) return;
		dry = p_dry;
		_sync_shared_state();
		emit_changed();
	}

	float FmodAudioEffectDelay::get_dry() {
		return dry;
	}

	void FmodAudioEffectDelay::set_tap1_active(bool p_active) {
		if (tap_1_active == p_active) return;
		tap_1_active = p_active;
		_sync_shared_state();
		emit_changed();
	}

	bool FmodAudioEffectDelay::is_tap1_active() const {
		return tap_1_active;
	}

	void FmodAudioEffectDelay::set_tap1_delay_ms(float p_delay_ms) {
		p_delay_ms = CLAMP(p_delay_ms, 0.0f, DELAY_MAX_MS);
		if (tap_1_delay_ms == p_delay_ms) return;
		tap_1_delay_ms = p_delay_ms;
		_sync_shared_state();
		emit_changed();
	}

	float FmodAudioEffectDelay::get_tap1_delay_ms() const {
		return tap_1_delay_ms;
	}

	void FmodAudioEffectDelay::set_tap1_level_db(float p_level_db) {
		p_level_db = CLAMP(p_level_db, -60.0f, 0.0f);
		if (tap_1_level == p_level_db) return;
		tap_1_level = p_level_db;
		_sync_shared_state();
		emit_changed();
	}

	float FmodAudioEffectDelay::get_tap1_level_db() const {
		return tap_1_level;
	}

	void FmodAudioEffectDelay::set_tap1_pan(float p_pan) {
		p_pan = CLAMP(p_pan, -1.0f, 1.0f);
		if (tap_1_pan == p_pan) return;
		tap_1_pan = p_pan;
		_sync_shared_state();
		emit_changed();
	}

	float FmodAudioEffectDelay::get_tap1_pan() const {
		return tap_1_pan;
	}

	void FmodAudioEffectDelay::set_tap2_active(bool p_active) {
		if (tap_2_active == p_active) return;
		tap_2_active = p_active;
		_sync_shared_state();
		emit_changed();
	}

	bool FmodAudioEffectDelay::is_tap2_active() const {
		return tap_2_active;
	}

	void FmodAudioEffectDelay::set_tap2_delay_ms(float p_delay_ms) {
		p_delay_ms = CLAMP(p_delay_ms, 0.0f, DELAY_MAX_MS);
		if (tap_2_delay_ms == p_delay_ms) return;
		tap_2_delay_ms = p_delay_ms;
		_sync_shared_state();
		emit_changed();
	}

	float FmodAudioEffectDelay::get_tap2_delay_ms() const {
		return tap_2_delay_ms;
	}

	void FmodAudioEffectDelay::set_tap2_level_db(float p_level_db) {
		p_level_db = CLAMP(p_level_db, -60.0f, 0.0f);
		if (tap_2_level == p_level_db) return;
		tap_2_level = p_level_db;
		_sync_shared_state();
		emit_changed();
	}

	float FmodAudioEffectDelay::get_tap2_level_db() const {
		return tap_2_level;
	}

	void FmodAudioEffectDelay::set_tap2_pan(float p_pan) {
		p_pan = CLAMP(p_pan, -1.0f, 1.0f);
		if (tap_2_pan == p_pan) return;
		tap_2_pan = p_pan;
		_sync_shared_state();
		emit_changed();
	}

	float FmodAudioEffectDelay::get_tap2_pan() const {
		return tap_2_pan;
	}

	void FmodAudioEffectDelay::set_feedback_active(bool p_active) {
		if (feedback_active == p_active) return;
		feedback_active = p_active;
		_sync_shared_state();
		emit_changed();
	}

	bool FmodAudioEffectDelay::is_feedback_active() const {
		return feedback_active;
	}

	void FmodAudioEffectDelay::set_feedback_delay_ms(float p_delay_ms) {
		p_delay_ms = CLAMP(p_delay_ms, 0.0f, DELAY_MAX_MS);
		if (feedback_delay_ms == p_delay_ms) return;
		feedback_delay_ms = p_delay_ms;
		_sync_shared_state();
		emit_changed();
	}

	float FmodAudioEffectDelay::get_feedback_delay_ms() const {
		return feedback_delay_ms;
	}

	void FmodAudioEffectDelay::set_feedback_level_db(float p_level_db) {
		p_level_db = CLAMP(p_level_db, -60.0f, 0.0f);
		if (feedback_level == p_level_db) return;
		feedback_level = p_level_db;
		_sync_shared_state();
		emit_changed();
	}

	float FmodAudioEffectDelay::get_feedback_level_db() const {
		return feedback_level;
	}

	void FmodAudioEffectDelay::set_feedback_lowpass(float p_lowpass) {
		p_lowpass = CLAMP(p_lowpass, 1.0f, 16000.0f);
		if (feedback_lowpass == p_lowpass) return;
		feedback_lowpass = p_lowpass;
		_sync_shared_state();
		emit_changed();
	}

	float FmodAudioEffectDelay::get_feedback_lowpass() const {
		return feedback_lowpass;
	}
}
