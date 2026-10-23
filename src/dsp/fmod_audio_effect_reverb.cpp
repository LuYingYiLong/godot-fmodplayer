#include "dsp/fmod_audio_effect_reverb.h"
#include "core/fmod_server.h"
#include "core/fmod_system.h"
#include "playback/fmod_channel_group.h"
#include <godot_cpp/core/class_db.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace godot {
	struct FmodAudioEffectReverbSharedState {
		std::atomic<float> damping{ 0.5f };
		std::atomic<float> dry{ 1.0f };
		std::atomic<float> hipass{ 0.0f };
		std::atomic<float> predelay_feedback{ 0.4f };
		std::atomic<float> predelay_msec{ 150.0f };
		std::atomic<float> room_size{ 0.8f };
		std::atomic<float> spread{ 1.0f };
		std::atomic<float> wet{ 0.5f };
	};

	namespace {
		static constexpr uint32_t REVERB_DSP_USERDATA_MAGIC = 0x52455642;
		static constexpr int GODOT_REVERB_MAX_COMBS = 8;
		static constexpr int GODOT_REVERB_MAX_ALLPASS = 4;
		static constexpr int GODOT_REVERB_MAX_ECHO_MS = 500;
		static constexpr float GODOT_REVERB_TAU = 6.28318530717958647692f;
		static constexpr float GODOT_REVERB_WET_SCALE = 0.6f;

		static constexpr float godot_comb_tunings[GODOT_REVERB_MAX_COMBS] = {
			0.025306122448979593f,
			0.026938775510204082f,
			0.028956916099773241f,
			0.03074829931972789f,
			0.032244897959183672f,
			0.03380952380952381f,
			0.035306122448979592f,
			0.036666666666666667f
		};

		static constexpr float godot_allpass_tunings[GODOT_REVERB_MAX_ALLPASS] = {
			0.0051020408163265302f,
			0.007732426303854875f,
			0.01f,
			0.012607709750566893f
		};

		struct GodotReverbParameters {
			float predelay = 150.0f;
			float predelay_feedback = 0.4f;
			float highpass = 0.0f;
			float room_size = 0.8f;
			float damping = 0.5f;
			float spread = 1.0f;
			float dry = 1.0f;
			float wet = 0.5f;
		};

		struct GodotReverbComb {
			std::vector<float> buffer;
			int pos = 0;
			int size_limit = 0;
			int extra_spread_frames = 0;
			float feedback = 0.0f;
			float damp = 0.0f;
			float damp_h = 0.0f;
		};

		struct GodotReverbAllPass {
			std::vector<float> buffer;
			int pos = 0;
			int size_limit = 0;
			int extra_spread_frames = 0;
		};

		class GodotReverbCore {
			GodotReverbComb comb[GODOT_REVERB_MAX_COMBS];
			GodotReverbAllPass allpass[GODOT_REVERB_MAX_ALLPASS];
			std::vector<float> echo_buffer;
			std::vector<float> input_buffer;
			int echo_buffer_pos = 0;
			int mix_rate = 48000;
			float extra_spread_base = 0.0f;
			float hpf_h1 = 0.0f;
			float hpf_h2 = 0.0f;
			GodotReverbParameters cached_parameters;
			bool parameters_valid = false;
			int predelay_frames = 10;
			float hp_a1 = 0.0f;
			float hp_a2 = 0.0f;
			float hp_b1 = 0.0f;

			static float _undenormalize(float p_value) {
				return std::fabs(p_value) < 0.000000000000001f ? 0.0f : p_value;
			}

			void _configure_buffers() {
				parameters_valid = false;
				for (int i = 0; i < GODOT_REVERB_MAX_COMBS; i++) {
					GodotReverbComb& c = comb[i];
					c.extra_spread_frames = static_cast<int>(std::lround(extra_spread_base * mix_rate));
					int len = static_cast<int>(std::lround(godot_comb_tunings[i] * mix_rate)) + c.extra_spread_frames;
					len = std::max(len, 5);
					c.buffer.assign(static_cast<size_t>(len), 0.0f);
					c.pos = 0;
					c.damp_h = 0.0f;
				}

				for (int i = 0; i < GODOT_REVERB_MAX_ALLPASS; i++) {
					GodotReverbAllPass& a = allpass[i];
					a.extra_spread_frames = static_cast<int>(std::lround(extra_spread_base * mix_rate));
					int len = static_cast<int>(std::lround(godot_allpass_tunings[i] * mix_rate)) + a.extra_spread_frames;
					len = std::max(len, 5);
					a.buffer.assign(static_cast<size_t>(len), 0.0f);
					a.pos = 0;
				}

				const int echo_size = static_cast<int>((static_cast<float>(GODOT_REVERB_MAX_ECHO_MS) / 1000.0f) * mix_rate + 1.0f);
				echo_buffer.assign(static_cast<size_t>(std::max(echo_size, 11)), 0.0f);
				echo_buffer_pos = 0;
				hpf_h1 = 0.0f;
				hpf_h2 = 0.0f;
			}

			void _update_parameters(const GodotReverbParameters& p_params) {
				static constexpr float room_scale = 0.28f;
				static constexpr float room_offset = 0.7f;

				if (!parameters_valid || p_params.room_size != cached_parameters.room_size || p_params.damping != cached_parameters.damping) {
					const float feedback = CLAMP(room_offset + p_params.room_size * room_scale, room_offset, room_offset + room_scale);
					float auxdmp = p_params.damping / 2.0f + 0.5f;
					auxdmp *= auxdmp;
					const float damp = std::exp(-GODOT_REVERB_TAU * auxdmp * 10000.0f / static_cast<float>(mix_rate));
					for (GodotReverbComb& c : comb) {
						c.feedback = feedback;
						c.damp = damp;
					}
				}
				if (!parameters_valid || p_params.spread != cached_parameters.spread) {
					for (GodotReverbComb& c : comb) {
						c.size_limit = std::max(static_cast<int>(c.buffer.size()) - static_cast<int>(std::lround(static_cast<float>(c.extra_spread_frames) * (1.0f - p_params.spread))), 1);
					}
					for (GodotReverbAllPass& a : allpass) {
						a.size_limit = std::max(static_cast<int>(a.buffer.size()) - static_cast<int>(std::lround(static_cast<float>(a.extra_spread_frames) * (1.0f - p_params.spread))), 1);
					}
				}
				if (!parameters_valid || p_params.predelay != cached_parameters.predelay) {
					predelay_frames = static_cast<int>(std::lround((p_params.predelay / 1000.0f) * mix_rate));
					predelay_frames = std::min(std::max(predelay_frames, 10), static_cast<int>(echo_buffer.size()) - 1);
				}
				if ((!parameters_valid || p_params.highpass != cached_parameters.highpass) && p_params.highpass > 0.0f) {
					const float hpaux = std::exp(-GODOT_REVERB_TAU * p_params.highpass * 6000.0f / static_cast<float>(mix_rate));
					hp_a1 = (1.0f + hpaux) / 2.0f;
					hp_a2 = -(1.0f + hpaux) / 2.0f;
					hp_b1 = hpaux;
				}
				cached_parameters = p_params;
				parameters_valid = true;
			}

		public:
			void configure(int p_mix_rate, float p_extra_spread_base) {
				p_mix_rate = std::max(p_mix_rate, 1);
				if (p_mix_rate == mix_rate && p_extra_spread_base == extra_spread_base && !echo_buffer.empty()) {
					return;
				}

				mix_rate = p_mix_rate;
				extra_spread_base = p_extra_spread_base;
				_configure_buffers();
			}

			void process(const float* p_src, float* p_dst, int p_frames, const GodotReverbParameters& p_params) {
				if (p_frames <= 0 || !p_dst) {
					return;
				}

				if (input_buffer.size() < static_cast<size_t>(p_frames)) {
					input_buffer.resize(static_cast<size_t>(p_frames));
				}

				_update_parameters(p_params);

				const int echo_size = static_cast<int>(echo_buffer.size());
				float* echo = echo_buffer.data();
				float* input = input_buffer.data();
				int write_pos = echo_buffer_pos >= echo_size ? 0 : echo_buffer_pos;
				int read_pos = write_pos - predelay_frames;
				if (read_pos < 0) read_pos += echo_size;
				int offset = 0;
				while (offset < p_frames) {
					if (write_pos >= echo_size) write_pos = 0;
					if (read_pos >= echo_size) read_pos = 0;
					const int count = std::min(p_frames - offset, std::min(echo_size - write_pos, echo_size - read_pos));
					for (int j = 0; j < count; j++) {
						const float source = p_src ? p_src[offset + j] : 0.0f;
						const float in = _undenormalize(echo[read_pos + j] * p_params.predelay_feedback + source);
						echo[write_pos + j] = in;
						input[offset + j] = in;
						p_dst[offset + j] = 0.0f;
					}
					write_pos += count;
					read_pos += count;
					offset += count;
				}
				echo_buffer_pos = write_pos;

				if (p_params.highpass > 0.0f) {
					float h1 = hpf_h1;
					float h2 = hpf_h2;
					for (int i = 0; i < p_frames; i++) {
						const float in = input[i];
						input[i] = in * hp_a1 + h1 * hp_a2 + h2 * hp_b1;
						h2 = input[i];
						h1 = in;
					}
					hpf_h1 = h1;
					hpf_h2 = h2;
				}

				for (int i = 0; i < GODOT_REVERB_MAX_COMBS; i++) {
					GodotReverbComb& c = comb[i];
					const float feedback = c.feedback;
					const float damp = c.damp;
					const float undamped = 1.0f - damp;
					float history = c.damp_h;
					float* buffer = c.buffer.data();
					int pos = c.pos;
					int processed = 0;
					while (processed < p_frames) {
						if (pos >= c.size_limit) pos = 0;
						const int count = std::min(p_frames - processed, c.size_limit - pos);
						for (int j = 0; j < count; j++) {
							float out = _undenormalize(buffer[pos + j] * feedback);
							out = out * undamped + history * damp;
							history = out;
							buffer[pos + j] = input[processed + j] + out;
							p_dst[processed + j] += out;
						}
						pos += count;
						processed += count;
					}
					// Keep an end-of-block position equal to size_limit intact: a
					// spread change can enlarge the ring before the next block.
					c.pos = pos;
					c.damp_h = history;
				}

				static constexpr float allpass_feedback = 0.7f;
				for (int i = 0; i < GODOT_REVERB_MAX_ALLPASS; i++) {
					GodotReverbAllPass& a = allpass[i];
					float* buffer = a.buffer.data();
					int pos = a.pos;
					int processed = 0;
					while (processed < p_frames) {
						if (pos >= a.size_limit) pos = 0;
						const int count = std::min(p_frames - processed, a.size_limit - pos);
						for (int j = 0; j < count; j++) {
							const float aux = buffer[pos + j];
							const float value = _undenormalize(allpass_feedback * aux + p_dst[processed + j]);
							buffer[pos + j] = value;
							p_dst[processed + j] = aux - allpass_feedback * value;
						}
						pos += count;
						processed += count;
					}
					a.pos = pos;
				}

				for (int i = 0; i < p_frames; i++) {
					const float source = p_src ? p_src[i] : 0.0f;
					p_dst[i] = p_dst[i] * p_params.wet * GODOT_REVERB_WET_SCALE + source * p_params.dry;
				}
			}
		};

		struct FmodReverbDSPRuntime {
			std::vector<GodotReverbCore> reverb;
			std::vector<float> temp_src;
			std::vector<float> temp_dst;
			int configured_rate = 0;
			int configured_channels = 0;

			void configure(int p_mix_rate, int p_channels, unsigned int p_length) {
				p_mix_rate = std::max(p_mix_rate, 1);
				p_channels = std::max(p_channels, 1);
				if (configured_rate != p_mix_rate || configured_channels != p_channels || reverb.size() != static_cast<size_t>(p_channels)) {
					reverb.clear();
					reverb.resize(static_cast<size_t>(p_channels));
					for (int i = 0; i < p_channels; i++) {
						reverb[static_cast<size_t>(i)].configure(p_mix_rate, (i % 2) == 0 ? 0.0f : 0.000521f);
					}
					configured_rate = p_mix_rate;
					configured_channels = p_channels;
				}

				if (temp_src.size() < static_cast<size_t>(p_length)) {
					temp_src.resize(static_cast<size_t>(p_length));
					temp_dst.resize(static_cast<size_t>(p_length));
				}
			}
		};

		struct ReverbDSPUserData {
			uint32_t magic = REVERB_DSP_USERDATA_MAGIC;
			std::shared_ptr<FmodAudioEffectReverbSharedState> state;
			FmodReverbDSPRuntime runtime;
		};

		static ReverbDSPUserData* _get_reverb_userdata(FMOD_DSP_STATE* dsp_state) {
			if (!dsp_state || !dsp_state->plugindata) {
				return nullptr;
			}

			ReverbDSPUserData* userdata = static_cast<ReverbDSPUserData*>(dsp_state->plugindata);
			if (userdata->magic != REVERB_DSP_USERDATA_MAGIC) {
				return nullptr;
			}
			return userdata;
		}

		static FMOD_RESULT _process_reverb_dsp(ReverbDSPUserData* userdata,
			FMOD_DSP_STATE* dsp_state,
			unsigned int length,
			const FMOD_DSP_BUFFER* inbufferarray,
			const FMOD_DSP_BUFFER* outbufferarray) {

			if (!inbufferarray || !outbufferarray) {
				return FMOD_ERR_INVALID_PARAM;
			}

			if (!outbufferarray->buffer) {
				return FMOD_OK;
			}

			const int input_channels = inbufferarray->numchannels;
			const int output_channels = outbufferarray->numchannels;
			const float* in_buffer = (!fmod_dsp_input_is_idle(dsp_state) && inbufferarray->buffer) ? inbufferarray->buffer : nullptr;
			float* out_buffer = outbufferarray->buffer;

			if (!out_buffer || output_channels <= 0) {
				return FMOD_ERR_INVALID_PARAM;
			}

			if (!userdata || !userdata->state) {
				const size_t sample_count = static_cast<size_t>(length) * static_cast<size_t>(output_channels);
				if (in_buffer && input_channels == output_channels) {
					memcpy(out_buffer, in_buffer, sample_count * sizeof(float));
				} else {
					memset(out_buffer, 0, sample_count * sizeof(float));
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

			userdata->runtime.configure(sample_rate, output_channels, length);

			GodotReverbParameters parameters;
			parameters.predelay = CLAMP(userdata->state->predelay_msec.load(), 0.0f, 500.0f);
			parameters.predelay_feedback = CLAMP(userdata->state->predelay_feedback.load(), 0.0f, 0.98f);
			parameters.highpass = CLAMP(userdata->state->hipass.load(), 0.0f, 1.0f);
			parameters.room_size = CLAMP(userdata->state->room_size.load(), 0.0f, 1.0f);
			parameters.damping = CLAMP(userdata->state->damping.load(), 0.0f, 1.0f);
			parameters.spread = CLAMP(userdata->state->spread.load(), 0.0f, 1.0f);
			parameters.dry = CLAMP(userdata->state->dry.load(), 0.0f, 1.0f);
			parameters.wet = CLAMP(userdata->state->wet.load(), 0.0f, 1.0f);

			for (int channel = 0; channel < output_channels; channel++) {
				for (unsigned int frame = 0; frame < length; frame++) {
					float sample = 0.0f;
					if (in_buffer && input_channels > 0) {
						const int input_channel = MIN(channel, input_channels - 1);
						sample = in_buffer[static_cast<size_t>(frame) * static_cast<size_t>(input_channels) + static_cast<size_t>(input_channel)];
					}
					userdata->runtime.temp_src[static_cast<size_t>(frame)] = sample;
				}

				userdata->runtime.reverb[static_cast<size_t>(channel)].process(userdata->runtime.temp_src.data(), userdata->runtime.temp_dst.data(), static_cast<int>(length), parameters);

				for (unsigned int frame = 0; frame < length; frame++) {
					out_buffer[static_cast<size_t>(frame) * static_cast<size_t>(output_channels) + static_cast<size_t>(channel)] = userdata->runtime.temp_dst[static_cast<size_t>(frame)];
				}
			}

			return FMOD_OK;
		}

		static FMOD_RESULT F_CALL reverb_dsp_create_callback(FMOD_DSP_STATE* dsp_state) {
			void* userdata = nullptr;
			if (dsp_state && dsp_state->functions && dsp_state->functions->getuserdata) {
				dsp_state->functions->getuserdata(dsp_state, &userdata);
			}
			dsp_state->plugindata = userdata;
			return FMOD_OK;
		}

		static FMOD_RESULT F_CALL reverb_dsp_release_callback(FMOD_DSP_STATE* dsp_state) {
			ReverbDSPUserData* userdata = _get_reverb_userdata(dsp_state);
			if (userdata) {
				userdata->magic = 0;
				delete userdata;
			}
			if (dsp_state) {
				dsp_state->plugindata = nullptr;
			}
			return FMOD_OK;
		}

		static FMOD_RESULT F_CALL reverb_dsp_process_callback(FMOD_DSP_STATE* dsp_state,
			unsigned int length,
			const FMOD_DSP_BUFFER* inbufferarray,
			const FMOD_DSP_BUFFER* outbufferarray) {
			return _process_reverb_dsp(_get_reverb_userdata(dsp_state), dsp_state, length, inbufferarray, outbufferarray);
		}
	}

	void FmodAudioEffectReverb::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_damping", "damping"), &FmodAudioEffectReverb::set_damping);
		ClassDB::bind_method(D_METHOD("get_damping"), &FmodAudioEffectReverb::get_damping);
		ClassDB::bind_method(D_METHOD("set_dry", "dry"), &FmodAudioEffectReverb::set_dry);
		ClassDB::bind_method(D_METHOD("get_dry"), &FmodAudioEffectReverb::get_dry);
		ClassDB::bind_method(D_METHOD("set_hpf", "hpf"), &FmodAudioEffectReverb::set_hpf);
		ClassDB::bind_method(D_METHOD("get_hpf"), &FmodAudioEffectReverb::get_hpf);
		ClassDB::bind_method(D_METHOD("set_predelay_feedback", "predelay_feedback"), &FmodAudioEffectReverb::set_predelay_feedback);
		ClassDB::bind_method(D_METHOD("get_predelay_feedback"), &FmodAudioEffectReverb::get_predelay_feedback);
		ClassDB::bind_method(D_METHOD("set_predelay_msec", "predelay_msec"), &FmodAudioEffectReverb::set_predelay_msec);
		ClassDB::bind_method(D_METHOD("get_predelay_msec"), &FmodAudioEffectReverb::get_predelay_msec);
		ClassDB::bind_method(D_METHOD("set_room_size", "room_size"), &FmodAudioEffectReverb::set_room_size);
		ClassDB::bind_method(D_METHOD("get_room_size"), &FmodAudioEffectReverb::get_room_size);
		ClassDB::bind_method(D_METHOD("set_spread", "spread"), &FmodAudioEffectReverb::set_spread);
		ClassDB::bind_method(D_METHOD("get_spread"), &FmodAudioEffectReverb::get_spread);
		ClassDB::bind_method(D_METHOD("set_wet", "wet"), &FmodAudioEffectReverb::set_wet);
		ClassDB::bind_method(D_METHOD("get_wet"), &FmodAudioEffectReverb::get_wet);

		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "damping", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_damping", "get_damping");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dry", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_dry", "get_dry");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "hipass", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_hpf", "get_hpf");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "predelay_feedback", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_predelay_feedback", "get_predelay_feedback");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "predelay_msec", PROPERTY_HINT_RANGE, "0,500,0.1,suffix:ms"), "set_predelay_msec", "get_predelay_msec");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "room_size", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_room_size", "get_room_size");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "spread", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_spread", "get_spread");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wet", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_wet", "get_wet");
	}

	FmodAudioEffectReverb::FmodAudioEffectReverb() {
		reverb_state = std::make_shared<FmodAudioEffectReverbSharedState>();
	}

	FmodAudioEffectReverb::~FmodAudioEffectReverb() {
	}

	void FmodAudioEffectReverb::apply_to(Ref<FmodChannelGroup> p_bus) {
		ERR_FAIL_COND_MSG(p_bus.is_null(), "Invalid bus");

		if (bus == p_bus && !dsp_chain.is_empty()) {
			remove_from_bus(p_bus);
		}

		bus = p_bus;

		Ref<FmodSystem> system = FmodServer::get_singleton()->get_main_system();
		ERR_FAIL_COND_MSG(system.is_null(), "FMOD system not initialized");

		Ref<FmodDSP> reverb_dsp = create_custom_dsp(system);
		if (!reverb_dsp.is_valid()) return;

		bus->add_dsp(-1, reverb_dsp);
		dsp_chain.push_back(reverb_dsp);
	}

	Ref<FmodDSP> FmodAudioEffectReverb::create_custom_dsp(Ref<FmodSystem> system) {
		ERR_FAIL_COND_V(system.is_null(), Ref<FmodDSP>());

		FMOD::System* fmod_system = system->get_system();
		ERR_FAIL_COND_V(!fmod_system, Ref<FmodDSP>());

		FMOD_DSP_DESCRIPTION* base_desc = get_dsp_description();
		ERR_FAIL_COND_V(!base_desc, Ref<FmodDSP>());

		if (!reverb_state) {
			reverb_state = std::make_shared<FmodAudioEffectReverbSharedState>();
		}

		ReverbDSPUserData* userdata = new ReverbDSPUserData();
		userdata->state = reverb_state;

		FMOD_DSP_DESCRIPTION desc;
		memcpy(&desc, base_desc, sizeof(FMOD_DSP_DESCRIPTION));
		desc.userdata = userdata;

		Ref<FmodDSP> dsp = system->create_dsp_from_description(desc);
		if (dsp.is_null()) {
			delete userdata;
			return Ref<FmodDSP>();
		}
		return dsp;
	}

	FMOD_DSP_DESCRIPTION* FmodAudioEffectReverb::get_dsp_description() {
		static FMOD_DSP_DESCRIPTION desc = {};
		static bool initialized = false;

		if (!initialized) {
			memset(&desc, 0, sizeof(desc));
			desc.pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
			strncpy(desc.name, "GodotReverb", sizeof(desc.name) - 1);
			desc.version = 1;
			fmod_configure_filter_description(desc);
			desc.create = reverb_dsp_create_callback;
			desc.release = reverb_dsp_release_callback;
			desc.process = reverb_dsp_process_callback;
			initialized = true;
		}

		return &desc;
	}

	void FmodAudioEffectReverb::_on_dsp_create(FMOD_DSP_STATE* dsp_state) {
		CustomDSPState* state = new CustomDSPState();
		state->effect = this;
		state->user_data = new FmodReverbDSPRuntime();
		dsp_state->plugindata = state;
	}

	void FmodAudioEffectReverb::_on_dsp_process(FMOD_DSP_STATE* dsp_state,
		unsigned int length,
		const FMOD_DSP_BUFFER* inbufferarray,
		const FMOD_DSP_BUFFER* outbufferarray) {

		if (!inbufferarray || !outbufferarray) {
			return;
		}

		if (!outbufferarray->buffer) {
			return;
		}

		const int input_channels = inbufferarray->numchannels;
		const int output_channels = outbufferarray->numchannels;
		const float* in_buffer = (!fmod_dsp_input_is_idle(dsp_state) && inbufferarray->buffer) ? static_cast<const float*>(inbufferarray->buffer) : nullptr;
		float* out_buffer = static_cast<float*>(outbufferarray->buffer);

		if (!out_buffer || output_channels <= 0) {
			return;
		}

		CustomDSPState* state = static_cast<CustomDSPState*>(dsp_state ? dsp_state->plugindata : nullptr);
		FmodReverbDSPRuntime* runtime = state ? static_cast<FmodReverbDSPRuntime*>(state->user_data) : nullptr;
		if (!runtime) {
			const size_t sample_count = static_cast<size_t>(length) * static_cast<size_t>(output_channels);
			if (in_buffer && input_channels == output_channels) {
				memcpy(out_buffer, in_buffer, sample_count * sizeof(float));
			} else {
				memset(out_buffer, 0, sample_count * sizeof(float));
			}
			return;
		}

		int sample_rate = 48000;
		if (dsp_state && dsp_state->functions && dsp_state->functions->getsamplerate) {
			int current_rate = 0;
			if (dsp_state->functions->getsamplerate(dsp_state, &current_rate) == FMOD_OK && current_rate > 0) {
				sample_rate = current_rate;
			}
		}

		runtime->configure(sample_rate, output_channels, length);

		GodotReverbParameters parameters;
		parameters.predelay = CLAMP(predelay_msec.load(), 0.0f, 500.0f);
		parameters.predelay_feedback = CLAMP(predelay_feedback.load(), 0.0f, 0.98f);
		parameters.highpass = CLAMP(hipass.load(), 0.0f, 1.0f);
		parameters.room_size = CLAMP(room_size.load(), 0.0f, 1.0f);
		parameters.damping = CLAMP(damping.load(), 0.0f, 1.0f);
		parameters.spread = CLAMP(spread.load(), 0.0f, 1.0f);
		parameters.dry = CLAMP(dry.load(), 0.0f, 1.0f);
		parameters.wet = CLAMP(wet.load(), 0.0f, 1.0f);

		for (int channel = 0; channel < output_channels; channel++) {
			for (unsigned int frame = 0; frame < length; frame++) {
				float sample = 0.0f;
				if (in_buffer && input_channels > 0) {
					const int input_channel = MIN(channel, input_channels - 1);
					sample = in_buffer[static_cast<size_t>(frame) * static_cast<size_t>(input_channels) + static_cast<size_t>(input_channel)];
				}
				runtime->temp_src[static_cast<size_t>(frame)] = sample;
			}

			runtime->reverb[static_cast<size_t>(channel)].process(runtime->temp_src.data(), runtime->temp_dst.data(), static_cast<int>(length), parameters);

			for (unsigned int frame = 0; frame < length; frame++) {
				out_buffer[static_cast<size_t>(frame) * static_cast<size_t>(output_channels) + static_cast<size_t>(channel)] = runtime->temp_dst[static_cast<size_t>(frame)];
			}
		}
	}

	void FmodAudioEffectReverb::_on_dsp_release(FMOD_DSP_STATE* dsp_state) {
		if (!dsp_state || !dsp_state->plugindata) {
			return;
		}

		CustomDSPState* state = static_cast<CustomDSPState*>(dsp_state->plugindata);
		delete static_cast<FmodReverbDSPRuntime*>(state->user_data);
		state->user_data = nullptr;
		delete state;
		dsp_state->plugindata = nullptr;
	}

	void FmodAudioEffectReverb::set_damping(float p_damping) {
		float value = CLAMP(p_damping, 0.0f, 1.0f);
		damping.store(value);
		if (reverb_state) reverb_state->damping.store(value);
	}

	float FmodAudioEffectReverb::get_damping() const {
		return damping.load();
	}

	void FmodAudioEffectReverb::set_dry(float p_dry) {
		float value = CLAMP(p_dry, 0.0f, 1.0f);
		dry.store(value);
		if (reverb_state) reverb_state->dry.store(value);
	}

	float FmodAudioEffectReverb::get_dry() const {
		return dry.load();
	}

	void FmodAudioEffectReverb::set_hpf(float p_hpf) {
		float value = CLAMP(p_hpf, 0.0f, 1.0f);
		hipass.store(value);
		if (reverb_state) reverb_state->hipass.store(value);
	}

	float FmodAudioEffectReverb::get_hpf() const {
		return hipass.load();
	}

	void FmodAudioEffectReverb::set_predelay_feedback(float p_predelay_feedback) {
		float value = CLAMP(p_predelay_feedback, 0.0f, 0.98f);
		predelay_feedback.store(value);
		if (reverb_state) reverb_state->predelay_feedback.store(value);
	}

	float FmodAudioEffectReverb::get_predelay_feedback() const {
		return predelay_feedback.load();
	}

	void FmodAudioEffectReverb::set_predelay_msec(float p_predelay_msec) {
		float value = CLAMP(p_predelay_msec, 0.0f, 500.0f);
		predelay_msec.store(value);
		if (reverb_state) reverb_state->predelay_msec.store(value);
	}

	float FmodAudioEffectReverb::get_predelay_msec() const {
		return predelay_msec.load();
	}

	void FmodAudioEffectReverb::set_room_size(float p_room_size) {
		float value = CLAMP(p_room_size, 0.0f, 1.0f);
		room_size.store(value);
		if (reverb_state) reverb_state->room_size.store(value);
	}

	float FmodAudioEffectReverb::get_room_size() const {
		return room_size.load();
	}

	void FmodAudioEffectReverb::set_spread(float p_spread) {
		float value = CLAMP(p_spread, 0.0f, 1.0f);
		spread.store(value);
		if (reverb_state) reverb_state->spread.store(value);
	}

	float FmodAudioEffectReverb::get_spread() const {
		return spread.load();
	}

	void FmodAudioEffectReverb::set_wet(float p_wet) {
		float value = CLAMP(p_wet, 0.0f, 1.0f);
		wet.store(value);
		if (reverb_state) reverb_state->wet.store(value);
	}

	float FmodAudioEffectReverb::get_wet() const {
		return wet.load();
	}

}
