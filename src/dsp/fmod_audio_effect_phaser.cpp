#include "dsp/fmod_audio_effect_phaser.h"
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
	static constexpr uint32_t PHASER_DSP_USERDATA_MAGIC = 0x50485352;
	static constexpr float PHASER_TAU = 6.28318530717958647692f;

	struct FmodAudioEffectPhaserSharedState {
		std::atomic<float> depth{ 1.0f };
		std::atomic<float> feedback{ 0.7f };
		std::atomic<float> range_max_hz{ 1600.0f };
		std::atomic<float> range_min_hz{ 440.0f };
		std::atomic<float> rate_hz{ 0.5f };
	};

	struct PhaserAllpassDelay {
		float a = 0.0f;
		float h = 0.0f;

		void delay(float d) {
			a = (1.0f - d) / (1.0f + d);
		}

		float update(float s) {
			float y = s * -a + h;
			h = y * a + s;
			return y;
		}
	};

	struct PhaserDSPRuntime {
		std::vector<PhaserAllpassDelay> allpass;
		std::vector<float> feedback_history;
		float phase = 0.0f;
		int channels = 0;
	};

	struct PhaserDSPUserData {
		uint32_t magic = PHASER_DSP_USERDATA_MAGIC;
		std::shared_ptr<FmodAudioEffectPhaserSharedState> phaser_state;
		PhaserDSPRuntime runtime;
	};

	static void _phaser_ensure_runtime(PhaserDSPRuntime& runtime, int channels) {
		channels = MAX(channels, 1);
		if (runtime.channels == channels) {
			return;
		}
		runtime.channels = channels;
		runtime.allpass.assign(static_cast<size_t>(channels) * 6, PhaserAllpassDelay());
		runtime.feedback_history.assign(static_cast<size_t>(channels), 0.0f);
		runtime.phase = 0.0f;
	}

	static PhaserDSPUserData* _get_phaser_userdata(FMOD_DSP_STATE* dsp_state) {
		if (!dsp_state || !dsp_state->plugindata) {
			return nullptr;
		}
		PhaserDSPUserData* userdata = static_cast<PhaserDSPUserData*>(dsp_state->plugindata);
		return userdata->magic == PHASER_DSP_USERDATA_MAGIC ? userdata : nullptr;
	}

	static FMOD_RESULT F_CALL phaser_dsp_create_callback(FMOD_DSP_STATE* dsp_state) {
		if (!dsp_state) {
			return FMOD_ERR_INVALID_PARAM;
		}
		void* userdata = nullptr;
		if (dsp_state->functions && dsp_state->functions->getuserdata) {
			dsp_state->functions->getuserdata(dsp_state, &userdata);
		}
		PhaserDSPUserData* phaser_userdata = static_cast<PhaserDSPUserData*>(userdata);
		dsp_state->plugindata = (phaser_userdata && phaser_userdata->magic == PHASER_DSP_USERDATA_MAGIC) ? phaser_userdata : nullptr;
		return FMOD_OK;
	}

	static FMOD_RESULT F_CALL phaser_dsp_release_callback(FMOD_DSP_STATE* dsp_state) {
		if (dsp_state) {
			PhaserDSPUserData* userdata = static_cast<PhaserDSPUserData*>(dsp_state->plugindata);
			if (userdata && userdata->magic == PHASER_DSP_USERDATA_MAGIC) {
				userdata->magic = 0;
				delete userdata;
			}
			dsp_state->plugindata = nullptr;
		}
		return FMOD_OK;
	}

	static FMOD_RESULT F_CALL phaser_dsp_process_callback(FMOD_DSP_STATE* dsp_state,
		unsigned int length,
		const FMOD_DSP_BUFFER* inbufferarray,
		const FMOD_DSP_BUFFER* outbufferarray) {

		if (!inbufferarray || !outbufferarray) {
			return FMOD_ERR_INVALID_PARAM;
		}

		const int input_channels = inbufferarray->numchannels;
		const int output_channels = outbufferarray->numchannels;
		const float* in_buffer = static_cast<const float*>(inbufferarray->buffer);
		float* out_buffer = static_cast<float*>(outbufferarray->buffer);
		if (!in_buffer || !out_buffer || input_channels <= 0 || output_channels <= 0) {
			return FMOD_ERR_INVALID_PARAM;
		}

		PhaserDSPUserData* userdata = _get_phaser_userdata(dsp_state);
		FmodAudioEffectPhaserSharedState* state = userdata ? userdata->phaser_state.get() : nullptr;
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

		PhaserDSPRuntime& runtime = userdata->runtime;
		_phaser_ensure_runtime(runtime, output_channels);

		const float range_min = CLAMP(state->range_min_hz.load(), 10.0f, 10000.0f);
		const float range_max = CLAMP(state->range_max_hz.load(), range_min, 10000.0f);
		const float dmin = range_min / (static_cast<float>(sample_rate) * 0.5f);
		const float dmax = range_max / (static_cast<float>(sample_rate) * 0.5f);
		const float increment = PHASER_TAU * (state->rate_hz.load() / static_cast<float>(sample_rate));
		const float feedback = state->feedback.load();
		const float depth = state->depth.load();

		for (unsigned int frame = 0; frame < length; frame++) {
			runtime.phase += increment;
			while (runtime.phase >= PHASER_TAU) {
				runtime.phase -= PHASER_TAU;
			}

			const float d = dmin + (dmax - dmin) * ((std::sin(runtime.phase) + 1.0f) * 0.5f);

			for (int channel = 0; channel < output_channels; channel++) {
				const int input_channel = MIN(channel, input_channels - 1);
				const float source = in_buffer[frame * input_channels + input_channel];

				float y = source + runtime.feedback_history[channel] * feedback;
				for (int stage = 5; stage >= 0; stage--) {
					PhaserAllpassDelay& allpass = runtime.allpass[static_cast<size_t>(channel) * 6 + stage];
					allpass.delay(d);
					y = allpass.update(y);
				}
				runtime.feedback_history[channel] = y;
				out_buffer[frame * output_channels + channel] = source + y * depth;
			}
		}

		return FMOD_OK;
	}

	void FmodAudioEffectPhaser::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_depth", "depth"), &FmodAudioEffectPhaser::set_depth);
		ClassDB::bind_method(D_METHOD("get_depth"), &FmodAudioEffectPhaser::get_depth);
		ClassDB::bind_method(D_METHOD("set_feedback", "feedback"), &FmodAudioEffectPhaser::set_feedback);
		ClassDB::bind_method(D_METHOD("get_feedback"), &FmodAudioEffectPhaser::get_feedback);
		ClassDB::bind_method(D_METHOD("set_range_max_hz", "range_max_hz"), &FmodAudioEffectPhaser::set_range_max_hz);
		ClassDB::bind_method(D_METHOD("get_range_max_hz"), &FmodAudioEffectPhaser::get_range_max_hz);
		ClassDB::bind_method(D_METHOD("set_range_min_hz", "range_min_hz"), &FmodAudioEffectPhaser::set_range_min_hz);
		ClassDB::bind_method(D_METHOD("get_range_min_hz"), &FmodAudioEffectPhaser::get_range_min_hz);
		ClassDB::bind_method(D_METHOD("set_rate_hz", "rate_hz"), &FmodAudioEffectPhaser::set_rate_hz);
		ClassDB::bind_method(D_METHOD("get_rate_hz"), &FmodAudioEffectPhaser::get_rate_hz);

		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "depth", PROPERTY_HINT_RANGE, "0.1,4.0,0.1"), "set_depth", "get_depth");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "feedback", PROPERTY_HINT_RANGE, "0.1,0.9,0.1"), "set_feedback", "get_feedback");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "range_max_hz", PROPERTY_HINT_RANGE, "10,10000,suffix:Hz"), "set_range_max_hz", "get_range_max_hz");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "range_min_hz", PROPERTY_HINT_RANGE, "10,10000,suffix:Hz"), "set_range_min_hz", "get_range_min_hz");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "rate_hz", PROPERTY_HINT_RANGE, "0.01,20,suffix:Hz"), "set_rate_hz", "get_rate_hz");
	}

	FmodAudioEffectPhaser::FmodAudioEffectPhaser() {
		phaser_state = std::make_shared<FmodAudioEffectPhaserSharedState>();
		_sync_shared_state();
	}

	FmodAudioEffectPhaser::~FmodAudioEffectPhaser() {
		remove_from_bus(bus);
		phaser_state.reset();
	}

	void FmodAudioEffectPhaser::_sync_shared_state() {
		if (!phaser_state) {
			return;
		}
		phaser_state->depth.store(depth);
		phaser_state->feedback.store(feedback);
		phaser_state->range_max_hz.store(range_max_hz);
		phaser_state->range_min_hz.store(range_min_hz);
		phaser_state->rate_hz.store(rate_hz);
	}

	void FmodAudioEffectPhaser::apply_to(Ref<FmodChannelGroup> p_bus) {
		ERR_FAIL_COND_MSG(p_bus.is_null(), "Invalid bus");
		if (bus.is_valid()) {
			remove_from_bus(bus);
		}
		bus = p_bus;
		Ref<FmodSystem> system = FmodServer::get_singleton()->get_main_system();
		ERR_FAIL_COND_MSG(system.is_null(), "FMOD system not initialized");
		Ref<FmodDSP> dsp = create_custom_dsp(system);
		if (!dsp.is_valid()) {
			return;
		}
		bus->add_dsp(-1, dsp);
		dsp_chain.push_back(dsp);
	}

	Ref<FmodDSP> FmodAudioEffectPhaser::create_custom_dsp(Ref<FmodSystem> system) {
		ERR_FAIL_COND_V(system.is_null(), Ref<FmodDSP>());
		FMOD::System* fmod_system = system->get_system();
		ERR_FAIL_COND_V(!fmod_system, Ref<FmodDSP>());

		FMOD_DSP_DESCRIPTION desc;
		memcpy(&desc, get_dsp_description(), sizeof(FMOD_DSP_DESCRIPTION));
		PhaserDSPUserData* dsp_userdata = new PhaserDSPUserData();
		dsp_userdata->phaser_state = phaser_state;
		desc.userdata = dsp_userdata;

		Ref<FmodDSP> dsp = system->create_dsp_from_description(desc);
		if (dsp.is_null()) {
			delete dsp_userdata;
			return Ref<FmodDSP>();
		}
		return dsp;
	}

	FMOD_DSP_DESCRIPTION* FmodAudioEffectPhaser::get_dsp_description() {
		static FMOD_DSP_DESCRIPTION desc = {};
		static bool initialized = false;
		if (!initialized) {
			memset(&desc, 0, sizeof(desc));
			desc.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
			strncpy(desc.name, "GodotPhaserDSP", sizeof(desc.name) - 1);
			desc.version = 1;
			fmod_configure_filter_description(desc);
			desc.create = phaser_dsp_create_callback;
			desc.release = phaser_dsp_release_callback;
			desc.process = phaser_dsp_process_callback;
			initialized = true;
		}
		return &desc;
	}

	void FmodAudioEffectPhaser::set_depth(float p_depth) {
		p_depth = CLAMP(p_depth, 0.1f, 4.0f);
		if (depth == p_depth) return;
		depth = p_depth;
		_sync_shared_state();
		emit_changed();
	}

	float FmodAudioEffectPhaser::get_depth() const {
		return depth;
	}

	void FmodAudioEffectPhaser::set_feedback(float p_feedback) {
		p_feedback = CLAMP(p_feedback, 0.1f, 0.9f);
		if (feedback == p_feedback) return;
		feedback = p_feedback;
		_sync_shared_state();
		emit_changed();
	}

	float FmodAudioEffectPhaser::get_feedback() const {
		return feedback;
	}

	void FmodAudioEffectPhaser::set_range_max_hz(float p_range_max_hz) {
		p_range_max_hz = CLAMP(p_range_max_hz, range_min_hz, 10000.0f);
		if (range_max_hz == p_range_max_hz) return;
		range_max_hz = p_range_max_hz;
		_sync_shared_state();
		emit_changed();
	}

	float FmodAudioEffectPhaser::get_range_max_hz() const {
		return range_max_hz;
	}

	void FmodAudioEffectPhaser::set_range_min_hz(float p_range_min_hz) {
		p_range_min_hz = CLAMP(p_range_min_hz, 10.0f, range_max_hz);
		if (range_min_hz == p_range_min_hz) return;
		range_min_hz = p_range_min_hz;
		_sync_shared_state();
		emit_changed();
	}

	float FmodAudioEffectPhaser::get_range_min_hz() const {
		return range_min_hz;
	}

	void FmodAudioEffectPhaser::set_rate_hz(float p_rate_hz) {
		p_rate_hz = CLAMP(p_rate_hz, 0.01f, 20.0f);
		if (rate_hz == p_rate_hz) return;
		rate_hz = p_rate_hz;
		_sync_shared_state();
		emit_changed();
	}

	float FmodAudioEffectPhaser::get_rate_hz() const {
		return rate_hz;
	}
} // namespace godot
