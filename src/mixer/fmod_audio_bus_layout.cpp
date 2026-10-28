#include "dsp/fmod_audio_effect.h"
#include "dsp/fmod_audio_effect_amplify.h"
#include "dsp/fmod_audio_effect_filter.h"
#include "dsp/fmod_audio_effect_capture.h"
#include "dsp/fmod_audio_effect_chorus.h"
#include "dsp/fmod_audio_effect_compressor.h"
#include "dsp/fmod_audio_effect_delay.h"
#include "dsp/fmod_audio_effect_distortion.h"
#include "dsp/fmod_audio_effect_eq.h"
#include "dsp/fmod_audio_effect_hard_limiter.h"
#include "dsp/fmod_audio_effect_panner.h"
#include "dsp/fmod_audio_effect_phaser.h"
#include "dsp/fmod_audio_effect_pitch_shift.h"
#include "dsp/fmod_audio_effect_record.h"
#include "dsp/fmod_audio_effect_reverb.h"
#include "dsp/fmod_audio_effect_spectrum_analyzer.h"
#include "dsp/fmod_audio_effect_stereo_enhance.h"
#include "fmod_audio_bus_layout.h"
#include "mixer/fmod_audio_bus.h"
#include "playback/fmod_channel_group.h"
#include <godot_cpp/classes/audio_effect.hpp>
#include <godot_cpp/classes/audio_effect_amplify.hpp>
#include <godot_cpp/classes/audio_effect_filter.hpp>
#include <godot_cpp/classes/audio_effect_capture.hpp>
#include <godot_cpp/classes/audio_effect_chorus.hpp>
#include <godot_cpp/classes/audio_effect_compressor.hpp>
#include <godot_cpp/classes/audio_effect_delay.hpp>
#include <godot_cpp/classes/audio_effect_distortion.hpp>
#include <godot_cpp/classes/audio_effect_eq.hpp>
#include <godot_cpp/classes/audio_effect_eq6.hpp>
#include <godot_cpp/classes/audio_effect_eq10.hpp>
#include <godot_cpp/classes/audio_effect_eq21.hpp>
#include <godot_cpp/classes/audio_effect_hard_limiter.hpp>
#include <godot_cpp/classes/audio_effect_panner.hpp>
#include <godot_cpp/classes/audio_effect_phaser.hpp>
#include <godot_cpp/classes/audio_effect_pitch_shift.hpp>
#include <godot_cpp/classes/audio_effect_record.hpp>
#include <godot_cpp/classes/audio_effect_reverb.hpp>
#include <godot_cpp/classes/audio_effect_spectrum_analyzer.hpp>
#include <godot_cpp/classes/audio_effect_stereo_enhance.hpp>
#include <godot_cpp/classes/audio_server.hpp>

namespace godot {
	namespace {
		constexpr int MAX_INCREMENTAL_BUS_CHECKS_PER_FRAME = 4;
		constexpr int MAX_INCREMENTAL_EFFECT_CHECKS_PER_FRAME = 4;

		class FmodDSPGraphLock {
			Ref<FmodSystem> system;
			bool locked = false;

		public:
			FmodDSPGraphLock(Ref<FmodSystem> p_system) {
				system = p_system;
				if (system.is_valid() && !system->system_is_null()) {
					system->lock_dsp();
					locked = true;
				}
			}

			~FmodDSPGraphLock() {
				if (locked && system.is_valid() && !system->system_is_null()) {
					system->unlock_dsp();
				}
			}
		};
	}

	void FmodAudioBusLayout::_bind_methods() {
		ClassDB::bind_method(D_METHOD("create_audio_bus", "name", "parent"), &FmodAudioBusLayout::create_audio_bus, DEFVAL(Ref<FmodAudioBus>()));
		ClassDB::bind_method(D_METHOD("get_audio_bus", "name"), &FmodAudioBusLayout::get_audio_bus);
		ClassDB::bind_method(D_METHOD("has_audio_bus", "name"), &FmodAudioBusLayout::has_audio_bus);
		ClassDB::bind_method(D_METHOD("remove_audio_bus", "name"), &FmodAudioBusLayout::remove_audio_bus);

		ClassDB::bind_method(D_METHOD("set_bus_volume", "name", "volume_db"), &FmodAudioBusLayout::set_bus_volume);
		ClassDB::bind_method(D_METHOD("get_bus_volume", "name"), &FmodAudioBusLayout::get_bus_volume);

		ClassDB::bind_method(D_METHOD("set_bus_solo", "name", "mute"), &FmodAudioBusLayout::set_bus_solo);
		ClassDB::bind_method(D_METHOD("bus_is_solo", "name"), &FmodAudioBusLayout::bus_is_solo);

		ClassDB::bind_method(D_METHOD("set_bus_mute", "name", "mute"), &FmodAudioBusLayout::set_bus_mute);
		ClassDB::bind_method(D_METHOD("bus_is_mute", "name"), &FmodAudioBusLayout::bus_is_mute);

		ClassDB::bind_method(D_METHOD("set_bus_bypass", "name", "mute"), &FmodAudioBusLayout::set_bus_bypass);
		ClassDB::bind_method(D_METHOD("bus_is_bypass", "name"), &FmodAudioBusLayout::bus_is_bypass);

		ClassDB::bind_method(D_METHOD("add_bus_effect", "bus_name", "effect", "index"), &FmodAudioBusLayout::add_bus_effect, DEFVAL(0));
		ClassDB::bind_method(D_METHOD("remove_bus_effect", "bus_name", "index"), &FmodAudioBusLayout::remove_bus_effect);
		ClassDB::bind_method(D_METHOD("get_bus_effect_count", "bus_name"), &FmodAudioBusLayout::get_bus_effect_count);
		ClassDB::bind_method(D_METHOD("get_bus_effect", "bus_name", "index"), &FmodAudioBusLayout::get_bus_effect);
		
		ClassDB::bind_method(D_METHOD("sync_from_audio_server"), &FmodAudioBusLayout::sync_from_audio_server);
		ClassDB::bind_method(D_METHOD("sync_from_audio_server_if_changed"), &FmodAudioBusLayout::sync_from_audio_server_if_changed);
	}

	void FmodAudioBusLayout::_clear_buses_except_master() {
		AudioServer* audio_server = AudioServer::get_singleton();
		ERR_FAIL_COND(!audio_server);

		Vector<String> buses_to_remove;
		for (const auto& pair : audio_buses_map) {
			if (pair.key == "Master") {
				continue;
			}

			bool bus_exists_in_audio_server = false;
			for (int i = 0; i < audio_server->get_bus_count(); i++) {
				if (audio_server->get_bus_name(i) == pair.key) {
					bus_exists_in_audio_server = true;
					break;
				}
			}

			if (!bus_exists_in_audio_server) {
				buses_to_remove.append(pair.key);
			}
		}
		for (const auto& bus_name : buses_to_remove) {
			Ref<FmodAudioBus> bus = audio_buses_map[bus_name];
			if (bus.is_valid()) {
				bus->release();
			}
			audio_buses_map.erase(bus_name);
		}
	}

	Ref<FmodAudioBus> FmodAudioBusLayout::_ensure_master_bus() {
		Ref<FmodAudioBus> master_bus;
		if (audio_buses_map.has("Master")) {
			master_bus = audio_buses_map["Master"];
		} else {
			master_bus.instantiate();
			master_bus->init_bus("Master");
			audio_buses_map["Master"] = master_bus;
		}
		return master_bus;
	}

	void FmodAudioBusLayout::_sync_bus_effects(Ref<FmodAudioBus> bus, int audio_server_bus_index) {
		ERR_FAIL_COND(bus.is_null());
		ERR_FAIL_COND(bus->get_bus().is_null());
		AudioServer* audio_server = AudioServer::get_singleton();
		ERR_FAIL_COND(!audio_server);

		bus->clear_effects();

		const int32_t effect_count = audio_server->get_bus_effect_count(audio_server_bus_index);
		for (int32_t i = 0; i < effect_count; i++) {
			if (!audio_server->is_bus_effect_enabled(audio_server_bus_index, i)) continue;
			Ref<AudioEffect> godot_effect = audio_server->get_bus_effect(audio_server_bus_index, i);

			// Amplify 效果器 - 处理 AudioEffectAmplify
			Ref<AudioEffectAmplify> godot_amplify = godot_effect;

			if (godot_amplify.is_valid()) {
				Ref<FmodAudioEffectAmplify> fmod_amplify;
				fmod_amplify.instantiate();
				fmod_amplify->set_volume_db(godot_amplify->get_volume_db());
				bus->add_effect(fmod_amplify, i);
				continue;
			}

			// Filter 效果器 - 处理 AudioEffectFilter
			Ref<AudioEffectFilter> godot_filter = godot_effect;
			if (godot_filter.is_valid()) {
				Ref<FmodAudioEffectFilter> fmod_filter;
				fmod_filter.instantiate();
				fmod_filter->set_cutoff_hz(godot_filter->get_cutoff());
				fmod_filter->set_db(static_cast<FmodAudioEffectFilter::FilterDB>(godot_filter->get_db()));
				fmod_filter->set_gain(godot_filter->get_gain());
				fmod_filter->set_resonance(godot_filter->get_resonance());
				bus->add_effect(fmod_filter, i);
				continue;
			}

			// Capture 效果器 - 处理 AudioEffectCapture
			Ref<AudioEffectCapture> godot_capture = godot_effect;
			if (godot_capture.is_valid()) {
				Ref<FmodAudioEffectCapture> fmod_capture;
				fmod_capture.instantiate();
				if (fmod_capture.is_null()) {
					ERR_PRINT("Failed to instantiate FmodAudioEffectCapture");
					continue;
				}
				// 获取 Godot 的 buffer_length，确保它是有效的
				double buffer_length = godot_capture->get_buffer_length();
				if (buffer_length <= 0.0) {
					buffer_length = 1.0; // 使用默认值
				}
				fmod_capture->set_buffer_length(buffer_length);
				bus->add_effect(fmod_capture, i);
				continue;
			}

			// Chorus 效果器 - 处理 AudioEffectChorus
			Ref<AudioEffectChorus> godot_chorus = godot_effect;
			if (godot_chorus.is_valid()) {
				Ref<FmodAudioEffectChorus> fmod_chorus;
				fmod_chorus.instantiate();
				fmod_chorus->set_voice_count(godot_chorus->get_voice_count());
				for (int voice_idx = 0; voice_idx < godot_chorus->get_voice_count(); voice_idx++) {
					fmod_chorus->set_voice_delay_ms(voice_idx, godot_chorus->get_voice_delay_ms(voice_idx));
					fmod_chorus->set_voice_rate_hz(voice_idx, godot_chorus->get_voice_rate_hz(voice_idx));
					fmod_chorus->set_voice_depth_ms(voice_idx, godot_chorus->get_voice_depth_ms(voice_idx));
					fmod_chorus->set_voice_level_db(voice_idx, godot_chorus->get_voice_level_db(voice_idx));
					fmod_chorus->set_voice_cutoff_hz(voice_idx, godot_chorus->get_voice_cutoff_hz(voice_idx));
					fmod_chorus->set_voice_pan(voice_idx, godot_chorus->get_voice_pan(voice_idx));
				}
				fmod_chorus->set_wet(godot_chorus->get_wet());
				fmod_chorus->set_dry(godot_chorus->get_dry());
				bus->add_effect(fmod_chorus, i);
				continue;
			}

			// Compressor 效果器 - 处理 AudioEffectCompressor
			Ref<AudioEffectCompressor> godot_compressor = godot_effect;
			if (godot_compressor.is_valid()) {
				Ref<FmodAudioEffectCompressor> fmod_compressor;
				fmod_compressor.instantiate();
				fmod_compressor->set_threshold(godot_compressor->get_threshold());
				fmod_compressor->set_ratio(godot_compressor->get_ratio());
				fmod_compressor->set_gain(godot_compressor->get_gain());
				fmod_compressor->set_attack_us(godot_compressor->get_attack_us());
				fmod_compressor->set_release_ms(godot_compressor->get_release_ms());
				fmod_compressor->set_mix(godot_compressor->get_mix());
				fmod_compressor->set_sidechain(godot_compressor->get_sidechain());
				bus->add_effect(fmod_compressor, i);
				continue;
			}

			// Delay 效果器 - 处理 AudioEffectDelay
			Ref<AudioEffectDelay> godot_delay = godot_effect;
			if (godot_delay.is_valid()) {
				Ref<FmodAudioEffectDelay> fmod_delay;
				fmod_delay.instantiate();
				fmod_delay->set_dry(godot_delay->get_dry());
				fmod_delay->set_tap1_active(godot_delay->is_tap1_active());
				fmod_delay->set_tap1_delay_ms(godot_delay->get_tap1_delay_ms());
				fmod_delay->set_tap1_level_db(godot_delay->get_tap1_level_db());
				fmod_delay->set_tap1_pan(godot_delay->get_tap1_pan());
				fmod_delay->set_tap2_active(godot_delay->is_tap2_active());
				fmod_delay->set_tap2_delay_ms(godot_delay->get_tap2_delay_ms());
				fmod_delay->set_tap2_level_db(godot_delay->get_tap2_level_db());
				fmod_delay->set_tap2_pan(godot_delay->get_tap2_pan());
				fmod_delay->set_feedback_active(godot_delay->is_feedback_active());
				fmod_delay->set_feedback_delay_ms(godot_delay->get_feedback_delay_ms());
				fmod_delay->set_feedback_level_db(godot_delay->get_feedback_level_db());
				fmod_delay->set_feedback_lowpass(godot_delay->get_feedback_lowpass());
				bus->add_effect(fmod_delay, i);
				continue;
			}

			// Distortion 效果器 - 处理 AudioEffectDistortion
			Ref<AudioEffectDistortion> godot_distortion = godot_effect;
			if (godot_distortion.is_valid()) {
				Ref<FmodAudioEffectDistortion> fmod_distortion;
				fmod_distortion.instantiate();
				fmod_distortion->set_mode(static_cast<FmodAudioEffectDistortion::Mode>(godot_distortion->get_mode()));
				fmod_distortion->set_pre_gain(godot_distortion->get_pre_gain());
				fmod_distortion->set_drive(godot_distortion->get_drive());
				fmod_distortion->set_post_gain(godot_distortion->get_post_gain());
				fmod_distortion->set_keep_hf_hz(godot_distortion->get_keep_hf_hz());
				bus->add_effect(fmod_distortion, i);
				continue;
			}

			// EQ 效果器 - 处理 AudioEffectEQ6/10/21 以及基类 AudioEffectEQ
			Ref<AudioEffectEQ> godot_eq = godot_effect;
			if (godot_eq.is_valid()) {
				// 根据频段数量确定使用哪个 FMOD EQ 类
				int band_count = godot_eq->get_band_count();
				Ref<FmodAudioEffectEQ> fmod_eq;

				// 根据频段数量选择对应的预设
				if (band_count == 6) {
					fmod_eq = Ref<FmodAudioEffectEQ>(memnew(FmodAudioEffectEQ6));
				} else if (band_count == 10) {
					fmod_eq = Ref<FmodAudioEffectEQ>(memnew(FmodAudioEffectEQ10));
				} else if (band_count == 21) {
					fmod_eq = Ref<FmodAudioEffectEQ>(memnew(FmodAudioEffectEQ21));
				} else {
					// 其他频段数量，使用基础类并设置对应预设
					fmod_eq.instantiate();
					if (band_count <= 6) {
						fmod_eq->set_preset(FmodAudioEffectEQ::PRESET_6_BANDS);
					} else if (band_count <= 10) {
						fmod_eq->set_preset(FmodAudioEffectEQ::PRESET_10_BANDS);
					} else {
						fmod_eq->set_preset(FmodAudioEffectEQ::PRESET_21_BANDS);
					}
				}

				// 同步每个频段的增益
				for (int band_idx = 0; band_idx < band_count; band_idx++) {
					float gain_db = godot_eq->get_band_gain_db(band_idx);
					fmod_eq->set_band_gain_db(band_idx, gain_db);
				}

				bus->add_effect(fmod_eq, i);
				continue;
			}

			// HardLimiter 效果器 - 处理 AudioEffectHardLimiter
			Ref<AudioEffectHardLimiter> godot_hard_limiter = godot_effect;
			if (godot_hard_limiter.is_valid()) {
				Ref<FmodAudioEffectHardLimiter> fmod_hard_limiter;
				fmod_hard_limiter.instantiate();
				fmod_hard_limiter->set_ceiling_db(godot_hard_limiter->get_ceiling_db());
				fmod_hard_limiter->set_pre_gain_db(godot_hard_limiter->get_pre_gain_db());
				fmod_hard_limiter->set_release(godot_hard_limiter->get_release());
				bus->add_effect(fmod_hard_limiter, i);
				continue;
			}

			// Panner 效果器 - 处理 AudioEffectPanner
			Ref<AudioEffectPanner> godot_panner = godot_effect;
			if (godot_panner.is_valid()) {
				Ref<FmodAudioEffectPanner> fmod_panner;
				fmod_panner.instantiate();
				fmod_panner->set_pan(godot_panner->get_pan());
				bus->add_effect(fmod_panner, i);
				continue;
			}

			// Phaser 效果器 - 处理 AudioEffectPhaser
			Ref<AudioEffectPhaser> godot_phaser = godot_effect;
			if (godot_phaser.is_valid()) {
				Ref<FmodAudioEffectPhaser> fmod_phaser;
				fmod_phaser.instantiate();
				fmod_phaser->set_depth(godot_phaser->get_depth());
				fmod_phaser->set_feedback(godot_phaser->get_feedback());
				fmod_phaser->set_range_max_hz(godot_phaser->get_range_max_hz());
				fmod_phaser->set_range_min_hz(godot_phaser->get_range_min_hz());
				fmod_phaser->set_rate_hz(godot_phaser->get_rate_hz());
				bus->add_effect(fmod_phaser, i);
				continue;
			}

			// PitchShift 效果器 - 处理 AudioEffectPitchShift
			Ref<AudioEffectPitchShift> godot_pitch = godot_effect;
			if (godot_pitch.is_valid()) {
				Ref<FmodAudioEffectPitchShift> fmod_pitch;
				fmod_pitch.instantiate();
				fmod_pitch->set_pitch_scale(godot_pitch->get_pitch_scale());
				fmod_pitch->set_fft_size(static_cast<FmodAudioEffectPitchShift::FFTSize>(godot_pitch->get_fft_size()));
				fmod_pitch->set_oversampling(godot_pitch->get_oversampling());
				bus->add_effect(fmod_pitch, i);
				continue;
			}

			// Record 效果器 - 处理 AudioEffectRecord
			Ref<AudioEffectRecord> godot_record = godot_effect;
			if (godot_record.is_valid()) {
				Ref<FmodAudioEffectRecord> fmod_record;
				fmod_record.instantiate();
				fmod_record->set_format(static_cast<FmodAudioEffectRecord::Format>(godot_record->get_format()));
				bus->add_effect(fmod_record, i);
				continue;
			}

			// Reverb 效果器 - 处理 AudioEffectReverb
			Ref<AudioEffectReverb> godot_reverb = godot_effect;
			if (godot_reverb.is_valid()) {
				Ref<FmodAudioEffectReverb> fmod_reverb;
				fmod_reverb.instantiate();
				fmod_reverb->set_damping(godot_reverb->get_damping());
				fmod_reverb->set_dry(godot_reverb->get_dry());
				fmod_reverb->set_hpf(godot_reverb->get_hpf());
				fmod_reverb->set_predelay_feedback(godot_reverb->get_predelay_feedback());
				fmod_reverb->set_predelay_msec(godot_reverb->get_predelay_msec());
				fmod_reverb->set_room_size(godot_reverb->get_room_size());
				fmod_reverb->set_spread(godot_reverb->get_spread());
				fmod_reverb->set_wet(godot_reverb->get_wet());
				bus->add_effect(fmod_reverb, i);
				continue;
			}

			// SpectrumAnalyzer 效果器 - 处理 AudioEffectSpectrumAnalyzer
			Ref<AudioEffectSpectrumAnalyzer> godot_spectrum = godot_effect;
			if (godot_spectrum.is_valid()) {
				Ref<FmodAudioEffectSpectrumAnalyzer> fmod_spectrum;
				fmod_spectrum.instantiate();
				fmod_spectrum->set_fft_size(static_cast<FmodAudioEffectSpectrumAnalyzer::FFTSize>(godot_spectrum->get_fft_size()));
				fmod_spectrum->set_buffer_length(godot_spectrum->get_buffer_length());
				bus->add_effect(fmod_spectrum, i);
				continue;
			}

			// StereoEnhance 效果器 - 处理 AudioEffectStereoEnhance
			Ref<AudioEffectStereoEnhance> godot_stereo = godot_effect;
			if (godot_stereo.is_valid()) {
				Ref<FmodAudioEffectStereoEnhance> fmod_stereo;
				fmod_stereo.instantiate();
				fmod_stereo->set_pan_pullout(godot_stereo->get_pan_pullout());
				fmod_stereo->set_time_pullout(godot_stereo->get_time_pullout());
				fmod_stereo->set_surround(godot_stereo->get_surround());
				bus->add_effect(fmod_stereo, i);
				continue;
			}

			// 检查是否是不支持的效果器类型
			String effect_class = godot_effect->get_class();
			if (effect_class != "AudioEffect") {
				WARN_PRINT(vformat("FmodAudioBusLayout: Audio effect '%s' on bus index %d is not supported by FMOD. Effect will be skipped.", effect_class, audio_server_bus_index));
			}
		}
	}

	void FmodAudioBusLayout::_update_solo_mute() {
		bool any_solo = false;
		for (const auto& pair : audio_buses_map) {
			if (pair.value->is_solo()) {
				any_solo = true;
				break;
			}
		}

		for (const auto& pair : audio_buses_map) {
			pair.value->apply_mute(any_solo);
		}
	}

	void FmodAudioBusLayout::create_audio_bus(const String& name, Ref<FmodAudioBus> parent) {
		Ref<FmodSystem> system = FmodServer::get_main_system();
		ERR_FAIL_COND_MSG(system.is_null(), "FMOD System is null");
		ERR_FAIL_COND_MSG(audio_buses_map.has(name), "Name is already taken");
		Ref<FmodChannelGroup> parent_group;
		if (parent.is_valid()) {
			parent_group = parent->get_bus();
			ERR_FAIL_COND_MSG(parent_group.is_null() || parent_group->channel_control_is_null(), "Parent bus is not initialized");
		}

		Ref<FmodAudioBus> new_bus;
		new_bus.instantiate();
		new_bus->init_bus(name, parent_group);
		ERR_FAIL_COND_MSG(new_bus->get_bus().is_null(), "Failed to initialize audio bus");
		audio_buses_map[name] = new_bus;
	}

	Ref<FmodAudioBus> FmodAudioBusLayout::get_audio_bus(const String& name) const {
		if (audio_buses_map.has(name)) {
			return audio_buses_map[name];
		}
		return Ref<FmodAudioBus>();
	}

	bool FmodAudioBusLayout::has_audio_bus(const String& name) const {
		return audio_buses_map.has(name);
	}

	void FmodAudioBusLayout::remove_audio_bus(const String& name) {
		ERR_FAIL_COND_MSG(name == "Master", "Cannot remove Master bus");
		audio_buses_map.erase(name);
	}

	void FmodAudioBusLayout::set_bus_volume(const String& name, const float volume_db) {
		ERR_FAIL_COND_MSG(!audio_buses_map.has(name), vformat("Bus not found: %s", name));
		Ref<FmodAudioBus> bus = audio_buses_map.get(name);
		ERR_FAIL_COND_MSG(bus.is_null(), "Bus is invalid");
		bus->set_volume_db(volume_db);
	}

	float FmodAudioBusLayout::get_bus_volume(const String& name) const {
		ERR_FAIL_COND_V_MSG(!audio_buses_map.has(name), 0.0f, vformat("Bus not found: %s", name));
		Ref<FmodAudioBus> bus = audio_buses_map.get(name);
		ERR_FAIL_COND_V_MSG(bus.is_null(), 0.0f, "Bus is invalid");
		return bus->get_volume_db();
	}

	void FmodAudioBusLayout::set_bus_solo(const String& name, const bool solo) {
		ERR_FAIL_COND_MSG(!audio_buses_map.has(name), vformat("Bus not found: %s", name));
		Ref<FmodAudioBus> bus = audio_buses_map.get(name);
		ERR_FAIL_COND_MSG(bus.is_null(), "Bus is invalid");
		bus->set_solo(solo);
		_update_solo_mute();
	}

	bool FmodAudioBusLayout::bus_is_solo(const String& name) const {
		ERR_FAIL_COND_V_MSG(!audio_buses_map.has(name), false, vformat("Bus not found: %s", name));
		Ref<FmodAudioBus> bus = audio_buses_map.get(name);
		ERR_FAIL_COND_V_MSG(bus.is_null(), false, "Bus is invalid");
		return bus->is_solo();
	}

	void FmodAudioBusLayout::set_bus_mute(const String& name, const bool mute) {
		ERR_FAIL_COND_MSG(!audio_buses_map.has(name), vformat("Bus not found: %s", name));
		Ref<FmodAudioBus> bus = audio_buses_map.get(name);
		ERR_FAIL_COND_MSG(bus.is_null(), "Bus is invalid");
		bus->set_mute(mute);
		_update_solo_mute();
	}

	bool FmodAudioBusLayout::bus_is_mute(const String& name) const {
		ERR_FAIL_COND_V_MSG(!audio_buses_map.has(name), false, vformat("Bus not found: %s", name));
		Ref<FmodAudioBus> bus = audio_buses_map.get(name);
		ERR_FAIL_COND_V_MSG(bus.is_null(), false, "Bus is invalid");
		return bus->is_mute();
	}

	void FmodAudioBusLayout::set_bus_bypass(const String& name, const bool bypass) {
		ERR_FAIL_COND_MSG(!audio_buses_map.has(name), vformat("Bus not found: %s", name));
		Ref<FmodAudioBus> bus = audio_buses_map.get(name);
		ERR_FAIL_COND_MSG(bus.is_null(), "Bus is invalid");
		bus->set_bypass(bypass);
		bus->sync_bypass();
	}

	bool FmodAudioBusLayout::bus_is_bypass(const String& name) const {
		ERR_FAIL_COND_V_MSG(!audio_buses_map.has(name), false, vformat("Bus not found: %s", name));
		Ref<FmodAudioBus> bus = audio_buses_map.get(name);
		ERR_FAIL_COND_V_MSG(bus.is_null(), false, "Bus is invalid");
		return bus->is_bypass();
	}

	void FmodAudioBusLayout::add_bus_effect(const String& bus_name, Ref<FmodAudioEffect> effect, const int index) {
		ERR_FAIL_COND_MSG(effect.is_null(), "Effect is null");
		ERR_FAIL_COND_MSG(!audio_buses_map.has(bus_name), vformat("Bus not found: %s", bus_name));
		Ref<FmodAudioBus> bus = get_audio_bus(bus_name);
		ERR_FAIL_COND_MSG(bus.is_null(), "Bus is invalid");
		bus->add_effect(effect, index);
	}

	void FmodAudioBusLayout::remove_bus_effect(const String& bus_name, const int index) {
		ERR_FAIL_COND_MSG(!audio_buses_map.has(bus_name), vformat("Bus not found: %s", bus_name));
		Ref<FmodAudioBus> bus = get_audio_bus(bus_name);
		ERR_FAIL_COND_MSG(bus.is_null(), "Bus is invalid");
		bus->remove_effect(index);
	}

	int FmodAudioBusLayout::get_bus_effect_count(const String& bus_name) const {
		ERR_FAIL_COND_V_MSG(!audio_buses_map.has(bus_name), 0, vformat("Bus not found: %s", bus_name));
		Ref<FmodAudioBus> bus = get_audio_bus(bus_name);
		ERR_FAIL_COND_V_MSG(bus.is_null(), 0, "Bus is invalid");
		return bus->get_effect_count();
	}

	Ref<FmodAudioEffect> FmodAudioBusLayout::get_bus_effect(const String& bus_name, const int index) const {
		ERR_FAIL_COND_V_MSG(!audio_buses_map.has(bus_name), Ref<FmodAudioEffect>(), vformat("Bus not found: %s", bus_name));
		Ref<FmodAudioBus> bus = get_audio_bus(bus_name);
		ERR_FAIL_COND_V_MSG(bus.is_null(), Ref<FmodAudioEffect>(), "Bus is invalid");
		return bus->get_effect(index);
	}

	void FmodAudioBusLayout::_clear_audio_effect_observers() {
		for (AudioEffectSlot& slot : audio_effect_slots) {
			if (slot.effect.is_valid() && slot.effect->is_connected("changed", slot.changed_callable)) {
				slot.effect->disconnect("changed", slot.changed_callable);
			}
		}
		audio_effect_slots.clear();
	}

	void FmodAudioBusLayout::_cache_audio_server_state() {
		AudioServer* audio_server = AudioServer::get_singleton();
		if (!audio_server) {
			audio_server_bus_states.clear();
			_clear_audio_effect_observers();
			return;
		}

		_clear_audio_effect_observers();
		const int bus_count = audio_server->get_bus_count();
		audio_server_bus_states.resize(bus_count);

		for (int bus_index = 0; bus_index < bus_count; bus_index++) {
			AudioServerBusState& state = audio_server_bus_states.write[bus_index];
			state.name = audio_server->get_bus_name(bus_index);
			state.send = audio_server->get_bus_send(bus_index);
			state.volume_db = audio_server->get_bus_volume_db(bus_index);
			state.mute = audio_server->is_bus_mute(bus_index);
			state.solo = audio_server->is_bus_solo(bus_index);
			state.bypass = audio_server->is_bus_bypassing_effects(bus_index);
			state.effect_count = audio_server->get_bus_effect_count(bus_index);

			for (int effect_index = 0; effect_index < state.effect_count; effect_index++) {
				Ref<AudioEffect> effect = audio_server->get_bus_effect(bus_index, effect_index);
				if (effect.is_null()) {
					continue;
				}

				AudioEffectSlot slot;
				slot.bus_index = bus_index;
				slot.effect_index = effect_index;
				slot.instance_id = static_cast<uint64_t>(effect->get_instance_id());
				slot.enabled = audio_server->is_bus_effect_enabled(bus_index, effect_index);
				slot.effect = effect;
				slot.changed_callable = callable_mp(this, &FmodAudioBusLayout::_on_audio_effect_changed);
				effect->connect("changed", slot.changed_callable, CONNECT_DEFERRED);
				audio_effect_slots.push_back(slot);
			}
		}

		next_audio_server_bus_index = 0;
		next_audio_effect_slot_index = 0;
		audio_effects_dirty = false;
	}

	void FmodAudioBusLayout::_on_audio_effect_changed() {
		audio_effects_dirty = true;
	}

	bool FmodAudioBusLayout::_has_audio_server_bus_layout_changed(const int p_bus_index) const {
		AudioServer* audio_server = AudioServer::get_singleton();
		if (!audio_server || p_bus_index < 0 || p_bus_index >= audio_server_bus_states.size()) {
			return true;
		}

		const AudioServerBusState& state = audio_server_bus_states[p_bus_index];
		return state.name != audio_server->get_bus_name(p_bus_index)
			|| state.send != audio_server->get_bus_send(p_bus_index)
			|| state.effect_count != audio_server->get_bus_effect_count(p_bus_index);
	}

	bool FmodAudioBusLayout::_has_audio_server_bus_state_changed(const int p_bus_index) const {
		AudioServer* audio_server = AudioServer::get_singleton();
		if (!audio_server || p_bus_index < 0 || p_bus_index >= audio_server_bus_states.size()) {
			return false;
		}

		const AudioServerBusState& state = audio_server_bus_states[p_bus_index];
		return state.volume_db != audio_server->get_bus_volume_db(p_bus_index)
			|| state.mute != audio_server->is_bus_mute(p_bus_index)
			|| state.solo != audio_server->is_bus_solo(p_bus_index)
			|| state.bypass != audio_server->is_bus_bypassing_effects(p_bus_index);
	}

	bool FmodAudioBusLayout::_has_audio_effect_instance_changed() {
		if (audio_effect_slots.is_empty()) {
			return false;
		}

		AudioServer* audio_server = AudioServer::get_singleton();
		if (!audio_server) {
			return true;
		}

		const int checks = MIN(MAX_INCREMENTAL_EFFECT_CHECKS_PER_FRAME, audio_effect_slots.size());
		for (int i = 0; i < checks; i++) {
			if (next_audio_effect_slot_index >= audio_effect_slots.size()) {
				next_audio_effect_slot_index = 0;
			}

			const AudioEffectSlot& slot = audio_effect_slots[next_audio_effect_slot_index++];
			if (slot.bus_index < 0 || slot.bus_index >= audio_server->get_bus_count()
					|| slot.effect_index < 0 || slot.effect_index >= audio_server->get_bus_effect_count(slot.bus_index)) {
				return true;
			}

			Ref<AudioEffect> effect = audio_server->get_bus_effect(slot.bus_index, slot.effect_index);
			if (effect.is_null()
					|| static_cast<uint64_t>(effect->get_instance_id()) != slot.instance_id
					|| audio_server->is_bus_effect_enabled(slot.bus_index, slot.effect_index) != slot.enabled) {
				return true;
			}
		}

		return false;
	}

	void FmodAudioBusLayout::sync_from_audio_server() {
		Ref<FmodSystem> system = FmodServer::get_main_system();
		ERR_FAIL_COND_MSG(system.is_null(), "FMOD System is not initialized.");

		AudioServer* audio_server = AudioServer::get_singleton();
		if (!audio_server) return;

		// 清空旧的总线映射（保留 Master）
		FmodDSPGraphLock dsp_lock(system);

		_clear_buses_except_master();

		// 确保 Master 总线存在
		Ref<FmodAudioBus> master_bus = _ensure_master_bus();
		ERR_FAIL_COND_MSG(master_bus.is_null(), "Failed to create Master bus.");
		ERR_FAIL_COND_MSG(master_bus->get_bus().is_null(), "Master bus ChannelGroup is null.");

		// 临时存储所有总线，先创建再连接（避免依赖顺序问题）
		HashMap<String, Ref<FmodAudioBus>> temp_buses;
		temp_buses["Master"] = master_bus;

		// 创建所有总线
		for (int i = 0; i < audio_server->get_bus_count(); i++) {
			String bus_name = audio_server->get_bus_name(i);
			if (bus_name == "Master") continue;

			Ref<FmodAudioBus> new_bus;
			if (audio_buses_map.has(bus_name)) {
				new_bus = audio_buses_map[bus_name];
			}

			if (new_bus.is_null() || new_bus->get_bus().is_null()) {
				new_bus.instantiate();
				new_bus->init_bus(bus_name);
			}
			
			// 检查 bus 是否成功创建
			if (new_bus.is_null() || new_bus->get_bus().is_null()) {
				ERR_PRINT(vformat("FmodAudioBusLayout: Failed to create bus '%s', skipping.", bus_name));
				continue;
			}
			
			temp_buses[bus_name] = new_bus;
			audio_buses_map[bus_name] = new_bus;
		}

		// 建立父子关系和应用设置
		for (int i = 0; i < audio_server->get_bus_count(); i++) {
			String bus_name = audio_server->get_bus_name(i);
			if (bus_name == "Master") continue;

			Ref<FmodAudioBus> bus = temp_buses[bus_name];
			if (bus.is_null()) continue;

			// 设置父子关系
			String parent_send = audio_server->get_bus_send(i);
			Ref<FmodChannelGroup> parent_group;
			if (temp_buses.has(parent_send)) {
				parent_group = temp_buses[parent_send]->get_bus();
				if (parent_group.is_valid()) {
					Ref<FmodChannelGroup> current_parent = bus->get_bus()->get_parent_group();
					if (current_parent.is_null() || current_parent->get_name() != parent_group->get_name()) {
						parent_group->add_group(bus->get_bus());
					}
				}
			} else if (master_bus.is_valid() && master_bus->get_bus().is_valid()) {
				// 默认添加到 Master
				parent_group = master_bus->get_bus();
				Ref<FmodChannelGroup> current_parent = bus->get_bus()->get_parent_group();
				if (current_parent.is_null() || current_parent->get_name() != parent_group->get_name()) {
					parent_group->add_group(bus->get_bus());
				}
			}
		}

		for (int i = 0; i < audio_server->get_bus_count(); i++) {
			String bus_name = audio_server->get_bus_name(i);
			if (!temp_buses.has(bus_name)) {
				continue;
			}
			Ref<FmodAudioBus> bus = temp_buses[bus_name];
			if (bus.is_null()) {
				continue;
			}
			sync_bus_state(bus_name, i);
			_sync_bus_effects(bus, i);
		}
		// 应用 solo 静音
		// bypass 已在每个 bus 的 set_bypass 中通过 sync_bypass 应用，无需额外操作
		_update_solo_mute();
		_cache_audio_server_state();
	}

	bool FmodAudioBusLayout::sync_from_audio_server_if_changed() {
		AudioServer* audio_server = AudioServer::get_singleton();
		if (!audio_server) {
			return false;
		}

		const int bus_count = audio_server->get_bus_count();
		if (bus_count != audio_server_bus_states.size() || audio_effects_dirty || _has_audio_effect_instance_changed()) {
			sync_from_audio_server();
			return true;
		}

		if (bus_count == 0) {
			return false;
		}

		bool bus_state_changed = false;
		const int checks = MIN(MAX_INCREMENTAL_BUS_CHECKS_PER_FRAME, bus_count);
		for (int i = 0; i < checks; i++) {
			if (next_audio_server_bus_index >= bus_count) {
				next_audio_server_bus_index = 0;
			}

			const int bus_index = next_audio_server_bus_index++;
			if (_has_audio_server_bus_layout_changed(bus_index)) {
				sync_from_audio_server();
				return true;
			}

			if (_has_audio_server_bus_state_changed(bus_index)) {
				const AudioServerBusState& state = audio_server_bus_states[bus_index];
				sync_bus_state(state.name, bus_index);
				audio_server_bus_states.write[bus_index].volume_db = audio_server->get_bus_volume_db(bus_index);
				audio_server_bus_states.write[bus_index].mute = audio_server->is_bus_mute(bus_index);
				audio_server_bus_states.write[bus_index].solo = audio_server->is_bus_solo(bus_index);
				audio_server_bus_states.write[bus_index].bypass = audio_server->is_bus_bypassing_effects(bus_index);
				bus_state_changed = true;
			}
		}

		if (bus_state_changed) {
			_update_solo_mute();
			return true;
		}

		return false;
	}

	void FmodAudioBusLayout::sync_bus_state(const String& bus_name, int audio_server_bus_index) const {
		AudioServer* audio_server = AudioServer::get_singleton();
		if (!audio_server) return;

		Ref<FmodAudioBus> bus = get_audio_bus(bus_name);
		if (bus.is_null()) return;

		bus->set_volume_db(audio_server->get_bus_volume_db(audio_server_bus_index));
		bus->set_mute(audio_server->is_bus_mute(audio_server_bus_index));
		bus->set_solo(audio_server->is_bus_solo(audio_server_bus_index));
		bus->set_bypass(audio_server->is_bus_bypassing_effects(audio_server_bus_index));
	}

	void FmodAudioBusLayout::clear() {
		for (const auto& pair : audio_buses_map) {
			if (pair.value.is_valid()) {
				pair.value->release();
			}
		}
		audio_buses_map.clear();
		audio_server_bus_states.clear();
		_clear_audio_effect_observers();
		next_audio_server_bus_index = 0;
		next_audio_effect_slot_index = 0;
		audio_effects_dirty = false;
	}
}
