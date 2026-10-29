#include "register_types.h"

#include "audio/fmod_audio_stream.h"
#include "audio/fmod_sound.h"
#include "audio/fmod_sound_lock.h"
#include "core/fmod_server.h"
#include "core/fmod_system.h"
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
#include "dsp/fmod_dsp.h"
#include "dsp/fmod_dsp_connection.h"
#include "editor/fmod_audio_importer.h"
#include "editor/fmod_audio_preview_inspector.h"
#include "editor/fmod_audio_preview_property.h"
#include "editor/fmod_audio_stream_player_3d_gizmo_plugin.h"
#include "editor/fmod_geometry_mesh_tools_plugin.h"
#include "editor/fmod_reverb_zone_3d_gizmo_plugin.h"
#include "editor/modify_android_build_template_plugin.h"
#ifdef TOOLS_ENABLED
#include "editor/audio_effects/fmod_audio_effect_inspector.h"
#include "editor/audio_effects/fmod_audio_effect_panner_preview.h"
#include "editor/audio_effects/fmod_audio_effect_eq_preview.h"
#include "editor/audio_effects/fmod_audio_effect_filter_preview.h"
#include "editor/audio_effects/fmod_audio_effect_compressor_preview.h"
#include "editor/audio_effects/fmod_audio_effect_capture_preview.h"
#include "editor/audio_effects/fmod_audio_effect_amplify_preview.h"
#include "editor/audio_effects/fmod_audio_effect_chorus_preview.h"
#include "editor/audio_effects/fmod_audio_effect_reverb_preview.h"
#include "editor/audio_effects/fmod_audio_effect_pitch_shift_preview.h"
#include "editor/audio_effects/fmod_audio_effect_phaser_preview.h"
#include "editor/audio_effects/fmod_audio_effect_stereo_enhance_preview.h"
#include "editor/audio_effects/fmod_audio_effect_delay_preview.h"
#include "editor/audio_effects/fmod_audio_effect_distortion_preview.h"
#include "editor/audio_effects/fmod_audio_effect_hard_limiter_preview.h"
#include "editor/audio_effects/fmod_audio_effect_spectrum_analyzer_preview.h"
#endif
#include "geometry/fmod_geometry.h"
#include "mixer/fmod_audio_bus.h"
#include "mixer/fmod_audio_bus_layout.h"
#include "nodes/fmod_audio_stream_player.h"
#include "nodes/fmod_audio_stream_player_2d.h"
#include "nodes/fmod_audio_stream_player_3d.h"
#include "nodes/fmod_geometry_instance_3d.h"
#include "nodes/fmod_reverb_zone_3d.h"
#include "playback/fmod_channel.h"
#include "playback/fmod_channel_control.h"
#include "playback/fmod_channel_group.h"
#include "playback/fmod_sound_group.h"
#include "spatial/fmod_reverb_3d.h"

#include <gdextension_interface.h>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

static FmodServer* fmod_server_instance = nullptr;

static void add_fmod_project_setting(
	const String& p_name,
	const Variant& p_default_value,
	Variant::Type p_type,
	bool p_as_basic,
	bool p_restart_if_changed,
	int p_hint = PROPERTY_HINT_NONE,
	const String& p_hint_string = String()
) {
	ProjectSettings* settings = ProjectSettings::get_singleton();
	ERR_FAIL_COND(!settings);

	if (!settings->has_setting(p_name)) {
		settings->set_setting(p_name, p_default_value);
	}
	settings->set_initial_value(p_name, p_default_value);
	settings->set_as_basic(p_name, p_as_basic);
	settings->set_restart_if_changed(p_name, p_restart_if_changed);

	Dictionary property_info;
	property_info["name"] = p_name;
	property_info["type"] = p_type;
	if (p_hint != PROPERTY_HINT_NONE) {
		property_info["hint"] = p_hint;
		property_info["hint_string"] = p_hint_string;
	}
	settings->add_property_info(property_info);
}

void initialize_fmod_player_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		add_fmod_project_setting("audio/fmod/show_startup_banner", true, Variant::BOOL, false, false);
		add_fmod_project_setting("audio/fmod/max_channels", 32, Variant::INT, true, true, PROPERTY_HINT_RANGE, "1,4095,1");
		add_fmod_project_setting("audio/fmod/software_channels", 0, Variant::INT, false, true, PROPERTY_HINT_RANGE, "0,4095,1,or_greater");
		add_fmod_project_setting("audio/fmod/output_type", FmodSystem::FMOD_OUTPUT_TYPE_AUTODETECT, Variant::INT, false, true, PROPERTY_HINT_ENUM, "Autodetect,Unknown,No Sound,WAV Writer,No Sound NRT,WAV Writer NRT,WASAPI,ASIO,PulseAudio,ALSA,CoreAudio,AudioTrack,OpenSL,AudioOut,Audio3D,WebAudio,NNAudio,WinSonic,AAudio,AudioWorklet,AudioWorkletP,PHASE,OHAudio");
		add_fmod_project_setting("audio/fmod/driver", -1, Variant::INT, false, true, PROPERTY_HINT_RANGE, "-1,128,1");
		add_fmod_project_setting("audio/fmod/software_sample_rate", 0, Variant::INT, false, true, PROPERTY_HINT_RANGE, "0,192000,1,or_greater");
		add_fmod_project_setting("audio/fmod/speaker_mode", FmodSystem::FMOD_SPEAKER_MODE_DEFAULT, Variant::INT, false, true, PROPERTY_HINT_ENUM, "Default,Raw,Mono,Stereo,Quad,Surround,5.1,7.1,7.1.4");
		add_fmod_project_setting("audio/fmod/num_raw_speakers", 0, Variant::INT, false, true, PROPERTY_HINT_RANGE, "0,32,1");
		add_fmod_project_setting("audio/fmod/dsp_buffer_size", 1024, Variant::INT, false, true, PROPERTY_HINT_RANGE, "64,8192,1,or_greater");
		add_fmod_project_setting("audio/fmod/dsp_buffer_count", 4, Variant::INT, false, true, PROPERTY_HINT_RANGE, "2,16,1,or_greater");
		add_fmod_project_setting("audio/fmod/stream_buffer_size", 16384, Variant::INT, false, true, PROPERTY_HINT_RANGE, "0,10485760,1,or_greater");
		add_fmod_project_setting("audio/fmod/init_flags/stream_from_update", false, Variant::BOOL, false, true);
		add_fmod_project_setting("audio/fmod/init_flags/3d_right_handed", false, Variant::BOOL, false, true);
		add_fmod_project_setting("audio/fmod/init_flags/clip_output", false, Variant::BOOL, false, true);
		add_fmod_project_setting("audio/fmod/init_flags/channel_lowpass", false, Variant::BOOL, false, true);
		add_fmod_project_setting("audio/fmod/init_flags/channel_distance_filter", false, Variant::BOOL, false, true);
		add_fmod_project_setting("audio/fmod/init_flags/vol0_becomes_virtual", false, Variant::BOOL, false, true);
		add_fmod_project_setting("audio/fmod/init_flags/geometry_use_closest", false, Variant::BOOL, false, true);
		add_fmod_project_setting("audio/fmod/init_flags/prefer_dolby_downmix", false, Variant::BOOL, false, true);
		add_fmod_project_setting("audio/fmod/init_flags/thread_unsafe", false, Variant::BOOL, false, true);
		add_fmod_project_setting("audio/fmod/init_flags/profile_meter_all", false, Variant::BOOL, false, true);
		add_fmod_project_setting("audio/fmod/init_flags/memory_tracking", false, Variant::BOOL, false, true);
		add_fmod_project_setting("audio/fmod/enable_profile", true, Variant::BOOL, true, true);
		add_fmod_project_setting("audio/fmod/profile_port", 0, Variant::INT, false, true, PROPERTY_HINT_RANGE, "0,65535,1");
		add_fmod_project_setting("audio/fmod/network_proxy", "", Variant::STRING, true, true);
		add_fmod_project_setting("audio/fmod/network_timeout", 0, Variant::INT, false, true, PROPERTY_HINT_RANGE, "0,600000,1,or_greater");
		add_fmod_project_setting("audio/fmod/3d/doppler_scale", 1.0, Variant::FLOAT, false, true, PROPERTY_HINT_RANGE, "0,10,0.01,or_greater");
		add_fmod_project_setting("audio/fmod/3d/distance_factor", 1.0, Variant::FLOAT, false, true, PROPERTY_HINT_RANGE, "0.001,1000,0.001,or_greater");
		add_fmod_project_setting("audio/fmod/3d/rolloff_scale", 1.0, Variant::FLOAT, false, true, PROPERTY_HINT_RANGE, "0,10,0.01,or_greater");
		add_fmod_project_setting("audio/fmod/3d/num_listeners", 1, Variant::INT, false, true, PROPERTY_HINT_RANGE, "1,8,1");
		add_fmod_project_setting("audio/fmod/3d/geometry_max_fade_time", 0, Variant::INT, false, true, PROPERTY_HINT_RANGE, "0,10000,1,or_greater");
		add_fmod_project_setting("audio/fmod/3d/distance_filter_center_freq", 0.0, Variant::FLOAT, false, true, PROPERTY_HINT_RANGE, "0,22000,1,or_greater");
		add_fmod_project_setting("audio/fmod/advanced/default_decode_buffer_size", 0, Variant::INT, false, true, PROPERTY_HINT_RANGE, "0,10000,1,or_greater");
		add_fmod_project_setting("audio/fmod/advanced/dsp_buffer_pool_size", 0, Variant::INT, false, true, PROPERTY_HINT_RANGE, "0,4096,1,or_greater");
		add_fmod_project_setting("audio/fmod/advanced/resampler_method", FmodSystem::FMOD_RESAMPLER_DEFAULT, Variant::INT, false, true, PROPERTY_HINT_ENUM, "Default,No Interp,Linear,Cubic,Spline");
		add_fmod_project_setting("audio/fmod/advanced/random_seed", 0, Variant::INT, false, true, PROPERTY_HINT_RANGE, "0,2147483647,1,or_greater");
		add_fmod_project_setting("audio/fmod/advanced/max_convolution_threads", 0, Variant::INT, false, true, PROPERTY_HINT_RANGE, "0,256,1,or_greater");
		add_fmod_project_setting("audio/fmod/advanced/max_spatial_objects", 0, Variant::INT, false, true, PROPERTY_HINT_RANGE, "0,1024,1,or_greater");
		add_fmod_project_setting("audio/fmod/advanced/vol0_virtual_vol", 0.0, Variant::FLOAT, false, true, PROPERTY_HINT_RANGE, "0,1,0.001");
		add_fmod_project_setting("audio/fmod/advanced/max_mpeg_codecs", 0, Variant::INT, false, true, PROPERTY_HINT_RANGE, "0,4096,1,or_greater");
		add_fmod_project_setting("audio/fmod/advanced/max_adpcm_codecs", 0, Variant::INT, false, true, PROPERTY_HINT_RANGE, "0,4096,1,or_greater");
		add_fmod_project_setting("audio/fmod/advanced/max_xma_codecs", 0, Variant::INT, false, true, PROPERTY_HINT_RANGE, "0,4096,1,or_greater");
		add_fmod_project_setting("audio/fmod/advanced/max_vorbis_codecs", 0, Variant::INT, false, true, PROPERTY_HINT_RANGE, "0,4096,1,or_greater");
		add_fmod_project_setting("audio/fmod/advanced/max_at9_codecs", 0, Variant::INT, false, true, PROPERTY_HINT_RANGE, "0,4096,1,or_greater");
		add_fmod_project_setting("audio/fmod/advanced/max_fadpcm_codecs", 0, Variant::INT, false, true, PROPERTY_HINT_RANGE, "0,4096,1,or_greater");
		add_fmod_project_setting("audio/fmod/advanced/max_opus_codecs", 0, Variant::INT, false, true, PROPERTY_HINT_RANGE, "0,4096,1,or_greater");

		GDREGISTER_CLASS(AudioImporterFmod);
		GDREGISTER_CLASS(FmodAudioPreviewInspector);
		GDREGISTER_CLASS(FmodAudioPreviewProperty);
		GDREGISTER_CLASS(FmodAudioStreamPlayer3DGizmoPlugin);
		GDREGISTER_CLASS(FmodGeometryMeshToolsPlugin);
		GDREGISTER_CLASS(FmodReverbZone3DGizmoPlugin);
		GDREGISTER_CLASS(ModifyAndroidBuildTemplatePlugin);
#ifdef TOOLS_ENABLED
		GDREGISTER_INTERNAL_CLASS(FmodAudioEffectPannerPreview);
		GDREGISTER_INTERNAL_CLASS(FmodAudioEffectEQPreview);
		GDREGISTER_INTERNAL_CLASS(FmodAudioEffectFilterPreview);
		GDREGISTER_INTERNAL_CLASS(FmodAudioEffectCompressorPreview);
		GDREGISTER_INTERNAL_CLASS(FmodAudioEffectCapturePreview);
		GDREGISTER_INTERNAL_CLASS(FmodAudioEffectAmplifyPreview);
		GDREGISTER_INTERNAL_CLASS(FmodAudioEffectChorusPreview);
		GDREGISTER_INTERNAL_CLASS(FmodAudioEffectReverbPreview);
		GDREGISTER_INTERNAL_CLASS(FmodAudioEffectPitchShiftPreview);
		GDREGISTER_INTERNAL_CLASS(FmodAudioEffectPhaserPreview);
		GDREGISTER_INTERNAL_CLASS(FmodAudioEffectStereoEnhancePreview);
		GDREGISTER_INTERNAL_CLASS(FmodAudioEffectDelayPreview);
		GDREGISTER_INTERNAL_CLASS(FmodAudioEffectDistortionPreview);
		GDREGISTER_INTERNAL_CLASS(FmodAudioEffectHardLimiterPreview);
		GDREGISTER_INTERNAL_CLASS(FmodAudioEffectSpectrumAnalyzerPreview);
		GDREGISTER_INTERNAL_CLASS(FmodAudioEffectInspector);
		GDREGISTER_INTERNAL_CLASS(FmodAudioEffectEditorPlugin);
		EditorPlugins::add_by_type<FmodAudioEffectEditorPlugin>();
#endif
		return;
	}

	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	GDREGISTER_CLASS(FmodChannelControl);
	GDREGISTER_CLASS(FmodChannel);
	GDREGISTER_CLASS(FmodChannelGroup);
	GDREGISTER_CLASS(FmodSoundGroup);
	GDREGISTER_CLASS(FmodDSPConnection);
	GDREGISTER_CLASS(FmodDSP);
	GDREGISTER_CLASS(FmodSound);
	GDREGISTER_CLASS(FmodSoundLock);
	GDREGISTER_CLASS(FmodSystem);

	GDREGISTER_ABSTRACT_CLASS(FmodAudioEffect);
	GDREGISTER_CLASS(FmodAudioEffectAmplify);
	GDREGISTER_CLASS(FmodAudioEffectFilter);
	GDREGISTER_CLASS(FmodAudioEffectCapture);
	GDREGISTER_CLASS(FmodAudioEffectChorus);
	GDREGISTER_CLASS(FmodAudioEffectCompressor);
	GDREGISTER_CLASS(FmodAudioEffectDelay);
	GDREGISTER_CLASS(FmodAudioEffectDistortion);
	GDREGISTER_CLASS(FmodAudioEffectEQ);
	GDREGISTER_CLASS(FmodAudioEffectEQ6);
	GDREGISTER_CLASS(FmodAudioEffectEQ10);
	GDREGISTER_CLASS(FmodAudioEffectEQ21);
	GDREGISTER_CLASS(FmodAudioEffectHardLimiter);
	GDREGISTER_CLASS(FmodAudioEffectPanner);
	GDREGISTER_CLASS(FmodAudioEffectPhaser);
	GDREGISTER_CLASS(FmodAudioEffectPitchShift);
	GDREGISTER_CLASS(FmodAudioEffectRecord);
	GDREGISTER_CLASS(FmodAudioEffectReverb);
	GDREGISTER_CLASS(FmodAudioEffectSpectrumAnalyzer);
	GDREGISTER_CLASS(FmodAudioEffectStereoEnhance);

	GDREGISTER_CLASS(FmodAudioBus);
	GDREGISTER_CLASS(FmodAudioBusLayout);

	GDREGISTER_CLASS(FmodAudioStream);

	GDREGISTER_CLASS(FmodReverb3D);
	GDREGISTER_CLASS(FmodReverbZone3D);

	GDREGISTER_CLASS(FmodGeometry);

	GDREGISTER_CLASS(FmodAudioStreamPlayer);
	GDREGISTER_CLASS(FmodAudioStreamPlayer2D);
	GDREGISTER_CLASS(FmodAudioStreamPlayer3D);
	GDREGISTER_CLASS(FmodGeometryInstance3D);

	GDREGISTER_CLASS(FmodServer);
	fmod_server_instance = memnew(FmodServer);
	Engine::get_singleton()->register_singleton("FmodServer", FmodServer::get_singleton());
}

void uninitialize_fmod_player_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		if (fmod_server_instance) {
			Engine::get_singleton()->unregister_singleton("FmodServer");
			memdelete(fmod_server_instance);
			fmod_server_instance = nullptr;
		}
	}
}

extern "C" {
	GDExtensionBool GDE_EXPORT fmod_player_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization* r_initialization) {
		godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

		init_obj.register_initializer(initialize_fmod_player_module);
		init_obj.register_terminator(uninitialize_fmod_player_module);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_EDITOR);

		return init_obj.init();
	}
}
