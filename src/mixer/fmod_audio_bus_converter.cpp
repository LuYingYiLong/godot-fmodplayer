#include "fmod_audio_bus_converter.h"
#include "dsp/fmod_audio_effect.h"
#include "dsp/fmod_audio_effect_amplify.h"
#include "dsp/fmod_audio_effect_capture.h"
#include "dsp/fmod_audio_effect_chorus.h"
#include "dsp/fmod_audio_effect_compressor.h"
#include "dsp/fmod_audio_effect_delay.h"
#include "dsp/fmod_audio_effect_distortion.h"
#include "dsp/fmod_audio_effect_eq.h"
#include "dsp/fmod_audio_effect_filter.h"
#include "dsp/fmod_audio_effect_hard_limiter.h"
#include "dsp/fmod_audio_effect_panner.h"
#include "dsp/fmod_audio_effect_phaser.h"
#include "dsp/fmod_audio_effect_pitch_shift.h"
#include "dsp/fmod_audio_effect_record.h"
#include "dsp/fmod_audio_effect_reverb.h"
#include "dsp/fmod_audio_effect_spectrum_analyzer.h"
#include "dsp/fmod_audio_effect_stereo_enhance.h"
#include "mixer/fmod_audio_bus.h"
#include "playback/fmod_channel_group.h"
#include <godot_cpp/classes/audio_effect.hpp>
#include <godot_cpp/classes/audio_effect_amplify.hpp>
#include <godot_cpp/classes/audio_effect_capture.hpp>
#include <godot_cpp/classes/audio_effect_chorus.hpp>
#include <godot_cpp/classes/audio_effect_compressor.hpp>
#include <godot_cpp/classes/audio_effect_delay.hpp>
#include <godot_cpp/classes/audio_effect_distortion.hpp>
#include <godot_cpp/classes/audio_effect_eq.hpp>
#include <godot_cpp/classes/audio_effect_eq10.hpp>
#include <godot_cpp/classes/audio_effect_eq21.hpp>
#include <godot_cpp/classes/audio_effect_eq6.hpp>
#include <godot_cpp/classes/audio_effect_filter.hpp>
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
Ref<FmodAudioEffect> convert_godot_audio_effect(const Ref<AudioEffect> &godot_effect) {
	// Amplify 效果器 - 处理 AudioEffectAmplify
	Ref<AudioEffectAmplify> godot_amplify = godot_effect;

	if (godot_amplify.is_valid()) {
		Ref<FmodAudioEffectAmplify> fmod_amplify;
		fmod_amplify.instantiate();
		fmod_amplify->set_volume_db(godot_amplify->get_volume_db());
		return fmod_amplify;
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
		return fmod_filter;
	}

	// Capture 效果器 - 处理 AudioEffectCapture
	Ref<AudioEffectCapture> godot_capture = godot_effect;
	if (godot_capture.is_valid()) {
		Ref<FmodAudioEffectCapture> fmod_capture;
		fmod_capture.instantiate();
		if (fmod_capture.is_null()) {
			ERR_PRINT("Failed to instantiate FmodAudioEffectCapture");
			return {};
		}
		// 获取 Godot 的 buffer_length，确保它是有效的
		double buffer_length = godot_capture->get_buffer_length();
		if (buffer_length <= 0.0) {
			buffer_length = 1.0; // 使用默认值
		}
		fmod_capture->set_buffer_length(buffer_length);
		return fmod_capture;
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
		return fmod_chorus;
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
		return fmod_compressor;
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
		return fmod_delay;
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
		return fmod_distortion;
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

		return fmod_eq;
	}

	// HardLimiter 效果器 - 处理 AudioEffectHardLimiter
	Ref<AudioEffectHardLimiter> godot_hard_limiter = godot_effect;
	if (godot_hard_limiter.is_valid()) {
		Ref<FmodAudioEffectHardLimiter> fmod_hard_limiter;
		fmod_hard_limiter.instantiate();
		fmod_hard_limiter->set_ceiling_db(godot_hard_limiter->get_ceiling_db());
		fmod_hard_limiter->set_pre_gain_db(godot_hard_limiter->get_pre_gain_db());
		fmod_hard_limiter->set_release(godot_hard_limiter->get_release());
		return fmod_hard_limiter;
	}

	// Panner 效果器 - 处理 AudioEffectPanner
	Ref<AudioEffectPanner> godot_panner = godot_effect;
	if (godot_panner.is_valid()) {
		Ref<FmodAudioEffectPanner> fmod_panner;
		fmod_panner.instantiate();
		fmod_panner->set_pan(godot_panner->get_pan());
		return fmod_panner;
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
		return fmod_phaser;
	}

	// PitchShift 效果器 - 处理 AudioEffectPitchShift
	Ref<AudioEffectPitchShift> godot_pitch = godot_effect;
	if (godot_pitch.is_valid()) {
		Ref<FmodAudioEffectPitchShift> fmod_pitch;
		fmod_pitch.instantiate();
		fmod_pitch->set_pitch_scale(godot_pitch->get_pitch_scale());
		fmod_pitch->set_fft_size(static_cast<FmodAudioEffectPitchShift::FFTSize>(godot_pitch->get_fft_size()));
		fmod_pitch->set_oversampling(godot_pitch->get_oversampling());
		return fmod_pitch;
	}

	// Record 效果器 - 处理 AudioEffectRecord
	Ref<AudioEffectRecord> godot_record = godot_effect;
	if (godot_record.is_valid()) {
		Ref<FmodAudioEffectRecord> fmod_record;
		fmod_record.instantiate();
		fmod_record->set_format(static_cast<FmodAudioEffectRecord::Format>(godot_record->get_format()));
		return fmod_record;
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
		return fmod_reverb;
	}

	// SpectrumAnalyzer 效果器 - 处理 AudioEffectSpectrumAnalyzer
	Ref<AudioEffectSpectrumAnalyzer> godot_spectrum = godot_effect;
	if (godot_spectrum.is_valid()) {
		Ref<FmodAudioEffectSpectrumAnalyzer> fmod_spectrum;
		fmod_spectrum.instantiate();
		fmod_spectrum->set_fft_size(static_cast<FmodAudioEffectSpectrumAnalyzer::FFTSize>(godot_spectrum->get_fft_size()));
		fmod_spectrum->set_buffer_length(godot_spectrum->get_buffer_length());
		return fmod_spectrum;
	}

	// StereoEnhance 效果器 - 处理 AudioEffectStereoEnhance
	Ref<AudioEffectStereoEnhance> godot_stereo = godot_effect;
	if (godot_stereo.is_valid()) {
		Ref<FmodAudioEffectStereoEnhance> fmod_stereo;
		fmod_stereo.instantiate();
		fmod_stereo->set_pan_pullout(godot_stereo->get_pan_pullout());
		fmod_stereo->set_time_pullout(godot_stereo->get_time_pullout());
		fmod_stereo->set_surround(godot_stereo->get_surround());
		return fmod_stereo;
	}

	return {};
}
} //namespace godot
