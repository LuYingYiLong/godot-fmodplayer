#include "fmod_system.h"

#include <godot_cpp/classes/os.hpp>
#include "audio/fmod_sound.h"
#include "playback/fmod_channel.h"
#include "playback/fmod_channel_group.h"
#include "playback/fmod_sound_group.h"
#include "geometry/fmod_geometry.h"
#include "dsp/fmod_dsp.h"
#include "spatial/fmod_reverb_3d.h"
#include "dsp/fmod_dsp_connection.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>

#include <limits>

namespace godot {
	static bool _is_main_thread() {
		OS* os = OS::get_singleton();
		return os && os->get_thread_caller_id() == os->get_main_thread_id();
	}

	void FmodSystem::_bind_methods() {
		BIND_ENUM_CONSTANT(FMOD_INIT_FLAG_NORMAL);
		BIND_ENUM_CONSTANT(FMOD_INIT_FLAG_STREAM_FROM_UPDATE);
		BIND_ENUM_CONSTANT(FMOD_INIT_FLAG_3D_RIGHTHANDED);
		BIND_ENUM_CONSTANT(FMOD_INIT_FLAG_CLIP_OUTPUT);
		BIND_ENUM_CONSTANT(FMOD_INIT_FLAG_CHANNEL_LOWPASS);
		BIND_ENUM_CONSTANT(FMOD_INIT_FLAG_CHANNEL_DISTANCEFILTER);
		BIND_ENUM_CONSTANT(FMOD_INIT_FLAG_PROFILE_ENABLE);
		BIND_ENUM_CONSTANT(FMOD_INIT_FLAG_VOL0_BECOMES_VIRTUAL);
		BIND_ENUM_CONSTANT(FMOD_INIT_FLAG_GEOMETRY_USECLOSEST);
		BIND_ENUM_CONSTANT(FMOD_INIT_FLAG_PREFER_DOLBY_DOWNMIX);
		BIND_ENUM_CONSTANT(FMOD_INIT_FLAG_THREAD_UNSAFE);
		BIND_ENUM_CONSTANT(FMOD_INIT_FLAG_PROFILE_METER_ALL);
		BIND_ENUM_CONSTANT(FMOD_INIT_FLAG_MEMORY_TRACKING);

		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_AUTODETECT);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_UNKNOWN);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_NOSOUND);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_WAVWRITER);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_NOSOUND_NRT);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_WAVWRITER_NRT);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_WASAPI);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_ASIO);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_PULSEAUDIO);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_ALSA);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_COREAUDIO);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_AUDIOTRACK);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_OPENSL);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_AUDIOOUT);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_AUDIO3D);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_WEBAUDIO);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_NNAUDIO);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_WINSONIC);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_AAUDIO);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_AUDIOWORKLET);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_AUDIOWORKLETP);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_PHASE);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_OHAUDIO);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_MAX);
		BIND_ENUM_CONSTANT(FMOD_OUTPUT_TYPE_FORCEINT);

		BIND_ENUM_CONSTANT(FMOD_SPEAKER_NONE);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_FRONT_LEFT);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_FRONT_RIGHT);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_FRONT_CENTER);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_LOW_FREQUENCY);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_SURROUND_LEFT);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_SURROUND_RIGHT);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_BACK_LEFT);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_BACK_RIGHT);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_TOP_FRONT_LEFT);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_TOP_FRONT_RIGHT);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_TOP_BACK_LEFT);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_TOP_BACK_RIGHT);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_MAX);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_FORCEINT);

		BIND_ENUM_CONSTANT(FMOD_SPEAKER_MODE_DEFAULT);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_MODE_RAW);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_MODE_MONO);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_MODE_STEREO);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_MODE_QUAD);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_MODE_SURROUND);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_MODE_5POINT1);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_MODE_7POINT1);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_MODE_7POINT1POINT4);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_MODE_MAX);
		BIND_ENUM_CONSTANT(FMOD_SPEAKER_MODE_FORCEINT);

		BIND_ENUM_CONSTANT(FMOD_MODE_DEFAULT);
		BIND_ENUM_CONSTANT(FMOD_MODE_LOOP_OFF);
		BIND_ENUM_CONSTANT(FMOD_MODE_LOOP_NORMAL);
		BIND_ENUM_CONSTANT(FMOD_MODE_LOOP_BIDI);
		BIND_ENUM_CONSTANT(FMOD_MODE_2D);
		BIND_ENUM_CONSTANT(FMOD_MODE_3D);
		BIND_ENUM_CONSTANT(FMOD_MODE_CREATESTREAM);
		BIND_ENUM_CONSTANT(FMOD_MODE_CREATESAMPLE);
		BIND_ENUM_CONSTANT(FMOD_MODE_CREATECOMPRESSEDSAMPLE);
		BIND_ENUM_CONSTANT(FMOD_MODE_OPENUSER);
		BIND_ENUM_CONSTANT(FMOD_MODE_OPENMEMORY);
		BIND_ENUM_CONSTANT(FMOD_MODE_OPENMEMORY_POINT);
		BIND_ENUM_CONSTANT(FMOD_MODE_OPENRAW);
		BIND_ENUM_CONSTANT(FMOD_MODE_OPENONLY);
		BIND_ENUM_CONSTANT(FMOD_MODE_ACCURATETIME);
		BIND_ENUM_CONSTANT(FMOD_MODE_MPEGSEARCH);
		BIND_ENUM_CONSTANT(FMOD_MODE_NONBLOCKING);
		BIND_ENUM_CONSTANT(FMOD_MODE_UNIQUE);
		BIND_ENUM_CONSTANT(FMOD_MODE_3D_HEADRELATIVE);
		BIND_ENUM_CONSTANT(FMOD_MODE_3D_WORLDRELATIVE);
		BIND_ENUM_CONSTANT(FMOD_MODE_3D_INVERSEROLLOFF);
		BIND_ENUM_CONSTANT(FMOD_MODE_3D_LINEARROLLOFF);
		BIND_ENUM_CONSTANT(FMOD_MODE_3D_LINEARSQUAREROLLOFF);
		BIND_ENUM_CONSTANT(FMOD_MODE_3D_INVERSETAPEREDROLLOFF);
		BIND_ENUM_CONSTANT(FMOD_MODE_3D_CUSTOMROLLOFF);
		BIND_ENUM_CONSTANT(FMOD_MODE_3D_IGNOREGEOMETRY);
		BIND_ENUM_CONSTANT(FMOD_MODE_IGNORETAGS);
		BIND_ENUM_CONSTANT(FMOD_MODE_LOWMEM);
		BIND_ENUM_CONSTANT(FMOD_MODE_VIRTUAL_PLAYFROMSTART);

		BIND_ENUM_CONSTANT(FMOD_TIME_UNIT_MS);
		BIND_ENUM_CONSTANT(FMOD_TIME_UNIT_PCM);
		BIND_ENUM_CONSTANT(FMOD_TIME_UNIT_PCMBYTES);
		BIND_ENUM_CONSTANT(FMOD_TIME_UNIT_RAWBYTES);
		BIND_ENUM_CONSTANT(FMOD_TIME_UNIT_PCMFRACTION);
		BIND_ENUM_CONSTANT(FMOD_TIME_UNIT_MODORDER);
		BIND_ENUM_CONSTANT(FMOD_TIME_UNIT_MODROW);
		BIND_ENUM_CONSTANT(FMOD_TIME_UNIT_MODPATTERN);

		BIND_ENUM_CONSTANT(FMOD_RESAMPLER_DEFAULT);
		BIND_ENUM_CONSTANT(FMOD_RESAMPLER_NOINTERP);
		BIND_ENUM_CONSTANT(FMOD_RESAMPLER_LINEAR);
		BIND_ENUM_CONSTANT(FMOD_RESAMPLER_CUBIC);
		BIND_ENUM_CONSTANT(FMOD_RESAMPLER_SPLINE);
		BIND_ENUM_CONSTANT(FMOD_RESAMPLER_MAX);
		BIND_ENUM_CONSTANT(FMOD_RESAMPLER_FORCEINT);

		BIND_ENUM_CONSTANT(FMOD_PORT_TYPE_MUSIC);
		BIND_ENUM_CONSTANT(FMOD_PORT_TYPE_COPYRIGHT_MUSIC);
		BIND_ENUM_CONSTANT(FMOD_PORT_TYPE_VOICE);
		BIND_ENUM_CONSTANT(FMOD_PORT_TYPE_CONTROLLER);
		BIND_ENUM_CONSTANT(FMOD_PORT_TYPE_PERSONAL);
		BIND_ENUM_CONSTANT(FMOD_PORT_TYPE_VIBRATION);
		BIND_ENUM_CONSTANT(FMOD_PORT_TYPE_AUX);
		BIND_ENUM_CONSTANT(FMOD_PORT_TYPE_PASSTHROUGH);
		BIND_ENUM_CONSTANT(FMOD_PORT_TYPE_VR_VIBRATION);
		BIND_ENUM_CONSTANT(FMOD_PORT_TYPE_MAX);

		ClassDB::bind_method(D_METHOD("system_is_valid"), &FmodSystem::system_is_valid);
		ClassDB::bind_method(D_METHOD("system_is_null"), &FmodSystem::system_is_null);

		ClassDB::bind_static_method("FmodSystem", D_METHOD("create_system"), &FmodSystem::create_system);
		ClassDB::bind_method(D_METHOD("init", "max_channels", "flags"), &FmodSystem::init, DEFVAL(32), DEFVAL(FMOD_INIT_FLAG_NORMAL));
		ClassDB::bind_method(D_METHOD("close"), &FmodSystem::close);
		ClassDB::bind_method(D_METHOD("release"), &FmodSystem::release);
		ClassDB::bind_method(D_METHOD("update"), &FmodSystem::update);
		ClassDB::bind_method(D_METHOD("mixer_suspend"), &FmodSystem::mixer_suspend);
		ClassDB::bind_method(D_METHOD("mixer_resume"), &FmodSystem::mixer_resume);

		ClassDB::bind_method(D_METHOD("set_output", "output_type"), &FmodSystem::set_output);
		ClassDB::bind_method(D_METHOD("get_output"), &FmodSystem::get_output);
		ClassDB::bind_method(D_METHOD("get_num_drivers"), &FmodSystem::get_num_drivers);
		ClassDB::bind_method(D_METHOD("get_driver_info", "id"), &FmodSystem::get_driver_info);
		ClassDB::bind_method(D_METHOD("set_driver", "driver"), &FmodSystem::set_driver);
		ClassDB::bind_method(D_METHOD("get_driver"), &FmodSystem::get_driver);

		ClassDB::bind_method(D_METHOD("set_software_channels", "num_software_channels"), &FmodSystem::set_software_channels);
		ClassDB::bind_method(D_METHOD("get_software_channels"), &FmodSystem::get_software_channels);

		ClassDB::bind_method(D_METHOD("set_software_format", "sample_rate", "speaker_mode", "num_raw_speakers"), &FmodSystem::set_software_format);
		ClassDB::bind_method(D_METHOD("get_software_format"), &FmodSystem::get_software_format);
		ClassDB::bind_method(D_METHOD("set_dsp_buffer_size", "buffer_length", "num_buffers"), &FmodSystem::set_dsp_buffer_size);
		ClassDB::bind_method(D_METHOD("get_dsp_buffer_size"), &FmodSystem::get_dsp_buffer_size);
		ClassDB::bind_method(D_METHOD("set_stream_buffer_size", "file_buffer_size", "file_buffer_size_type"), &FmodSystem::set_stream_buffer_size, DEFVAL(16384), DEFVAL(FMOD_TIME_UNIT_RAWBYTES));
		ClassDB::bind_method(D_METHOD("get_stream_buffer_size"), &FmodSystem::get_stream_buffer_size);
		ClassDB::bind_method(D_METHOD("set_speaker_position", "speaker", "x", "y", "active"), &FmodSystem::set_speaker_position);
		ClassDB::bind_method(D_METHOD("get_speaker_position", "speaker"), &FmodSystem::get_speaker_position);

		ClassDB::bind_method(D_METHOD("set_doppler_scale", "doppler_scale"), &FmodSystem::set_doppler_scale);
		ClassDB::bind_method(D_METHOD("get_doppler_scale"), &FmodSystem::get_doppler_scale);

		ClassDB::bind_method(D_METHOD("set_distance_factor", "distance_factor"), &FmodSystem::set_distance_factor);
		ClassDB::bind_method(D_METHOD("get_distance_factor"), &FmodSystem::get_distance_factor);

		ClassDB::bind_method(D_METHOD("set_rolloff_scale", "rolloff_scale"), &FmodSystem::set_rolloff_scale);
		ClassDB::bind_method(D_METHOD("get_rolloff_scale"), &FmodSystem::get_rolloff_scale);

		ClassDB::bind_method(D_METHOD("set_3d_num_listeners", "num_listeners"), &FmodSystem::set_3d_num_listeners);
		ClassDB::bind_method(D_METHOD("get_3d_num_listeners"), &FmodSystem::get_3d_num_listeners);

		ClassDB::bind_method(D_METHOD("set_max_mpeg_codecs", "max_codecs"), &FmodSystem::set_max_mpeg_codecs);
		ClassDB::bind_method(D_METHOD("get_max_mpeg_codecs"), &FmodSystem::get_max_mpeg_codecs);

		ClassDB::bind_method(D_METHOD("set_max_adpcm_codecs", "max_codecs"), &FmodSystem::set_max_adpcm_codecs);
		ClassDB::bind_method(D_METHOD("get_max_adpcm_codecs"), &FmodSystem::get_max_adpcm_codecs);

		ClassDB::bind_method(D_METHOD("set_max_xma_codecs", "max_codecs"), &FmodSystem::set_max_xma_codecs);
		ClassDB::bind_method(D_METHOD("get_max_xma_codecs"), &FmodSystem::get_max_xma_codecs);

		ClassDB::bind_method(D_METHOD("set_max_vorbis_codecs", "max_codecs"), &FmodSystem::set_max_vorbis_codecs);
		ClassDB::bind_method(D_METHOD("get_max_vorbis_codecs"), &FmodSystem::get_max_vorbis_codecs);

		ClassDB::bind_method(D_METHOD("set_max_at9_codecs", "max_codecs"), &FmodSystem::set_max_at9_codecs);
		ClassDB::bind_method(D_METHOD("get_max_at9_codecs"), &FmodSystem::get_max_at9_codecs);

		ClassDB::bind_method(D_METHOD("set_max_fadpcm_codecs", "max_codecs"), &FmodSystem::set_max_fadpcm_codecs);
		ClassDB::bind_method(D_METHOD("get_max_fadpcm_codecs"), &FmodSystem::get_max_fadpcm_codecs);

		ClassDB::bind_method(D_METHOD("set_max_opus_codecs", "max_codecs"), &FmodSystem::set_max_opus_codecs);
		ClassDB::bind_method(D_METHOD("get_max_opus_codecs"), &FmodSystem::get_max_opus_codecs);

		ClassDB::bind_method(D_METHOD("set_asio_num_channels", "num_channels"), &FmodSystem::set_asio_num_channels);
		ClassDB::bind_method(D_METHOD("get_asio_num_channels"), &FmodSystem::get_asio_num_channels);

		ClassDB::bind_method(D_METHOD("set_vol0_virtual_vol", "vol"), &FmodSystem::set_vol0_virtual_vol);
		ClassDB::bind_method(D_METHOD("get_vol0_virtual_vol"), &FmodSystem::get_vol0_virtual_vol);

		ClassDB::bind_method(D_METHOD("set_default_decode_buffer_size", "size"), &FmodSystem::set_default_decode_buffer_size);
		ClassDB::bind_method(D_METHOD("get_default_decode_buffer_size"), &FmodSystem::get_default_decode_buffer_size);

		ClassDB::bind_method(D_METHOD("set_profile_port", "port"), &FmodSystem::set_profile_port);
		ClassDB::bind_method(D_METHOD("get_profile_port"), &FmodSystem::get_profile_port);

		ClassDB::bind_method(D_METHOD("set_geometry_max_fade_time", "time"), &FmodSystem::set_geometry_max_fade_time);
		ClassDB::bind_method(D_METHOD("get_geometry_max_fade_time"), &FmodSystem::get_geometry_max_fade_time);

		ClassDB::bind_method(D_METHOD("set_distance_filter_center_freq", "freq"), &FmodSystem::set_distance_filter_center_freq);
		ClassDB::bind_method(D_METHOD("get_distance_filter_center_freq"), &FmodSystem::get_distance_filter_center_freq);

		ClassDB::bind_method(D_METHOD("set_reverb_3d_instance", "instance"), &FmodSystem::set_reverb_3d_instance);
		ClassDB::bind_method(D_METHOD("get_reverb_3d_instance"), &FmodSystem::get_reverb_3d_instance);

		ClassDB::bind_method(D_METHOD("set_dsp_buffer_pool_size", "size"), &FmodSystem::set_dsp_buffer_pool_size);
		ClassDB::bind_method(D_METHOD("get_dsp_buffer_pool_size"), &FmodSystem::get_dsp_buffer_pool_size);

		ClassDB::bind_method(D_METHOD("set_resampler_method", "method"), &FmodSystem::set_resampler_method);
		ClassDB::bind_method(D_METHOD("get_resampler_method"), &FmodSystem::get_resampler_method);

		ClassDB::bind_method(D_METHOD("set_random_seed", "seed"), &FmodSystem::set_random_seed);
		ClassDB::bind_method(D_METHOD("get_random_seed"), &FmodSystem::get_random_seed);

		ClassDB::bind_method(D_METHOD("set_max_convolution_threads", "max_threads"), &FmodSystem::set_max_convolution_threads);
		ClassDB::bind_method(D_METHOD("get_max_convolution_threads"), &FmodSystem::get_max_convolution_threads);

		ClassDB::bind_method(D_METHOD("set_max_spatial_objects", "max_objects"), &FmodSystem::set_max_spatial_objects);
		ClassDB::bind_method(D_METHOD("get_max_spatial_objects"), &FmodSystem::get_max_spatial_objects);

		ClassDB::bind_method(D_METHOD("set_3d_rolloff_callback", "callback"), &FmodSystem::set_3d_rolloff_callback);
		ClassDB::bind_method(D_METHOD("get_3d_rolloff_callback"), &FmodSystem::get_3d_rolloff_callback);

		ClassDB::bind_method(D_METHOD("set_network_proxy", "proxy"), &FmodSystem::set_network_proxy);
		ClassDB::bind_method(D_METHOD("get_network_proxy"), &FmodSystem::get_network_proxy);
		ClassDB::bind_method(D_METHOD("set_network_timeout", "timeout"), &FmodSystem::set_network_timeout);
		ClassDB::bind_method(D_METHOD("get_network_timeout"), &FmodSystem::get_network_timeout);

		ClassDB::bind_method(D_METHOD("get_version"), &FmodSystem::get_version);
		ClassDB::bind_method(D_METHOD("get_output_handle"), &FmodSystem::get_output_handle);
		ClassDB::bind_method(D_METHOD("get_channels_playing"), &FmodSystem::get_channels_playing);
		ClassDB::bind_method(D_METHOD("get_cpu_usage"), &FmodSystem::get_cpu_usage);
		ClassDB::bind_method(D_METHOD("get_file_usage"), &FmodSystem::get_file_usage);
		ClassDB::bind_method(D_METHOD("get_default_mix_matrix", "source_speaker_mode", "target_speaker_mode", "array_length", "hop"), &FmodSystem::get_default_mix_matrix);
		ClassDB::bind_method(D_METHOD("get_speaker_mode_channels", "mode"), &FmodSystem::get_speaker_mode_channels);

		ClassDB::bind_method(D_METHOD("create_sound_from_file", "path", "mode", "resampler_method"), &FmodSystem::create_sound_from_file, DEFVAL(FMOD_MODE_DEFAULT), DEFVAL(FMOD_RESAMPLER_DEFAULT));
		ClassDB::bind_method(D_METHOD("create_sound_from_memory", "data", "mode", "resampler_method"), &FmodSystem::create_sound_from_memory, DEFVAL(FMOD_MODE_DEFAULT), DEFVAL(FMOD_RESAMPLER_DEFAULT));
		ClassDB::bind_method(D_METHOD("create_sound_from_res", "path", "mode", "resampler_method"), &FmodSystem::create_sound_from_res, DEFVAL(FMOD_MODE_DEFAULT), DEFVAL(FMOD_RESAMPLER_DEFAULT));
		ClassDB::bind_method(D_METHOD("create_stream_from_file", "path", "mode", "resampler_method"), &FmodSystem::create_stream_from_file, DEFVAL(FMOD_MODE_DEFAULT), DEFVAL(FMOD_RESAMPLER_DEFAULT));
		ClassDB::bind_method(D_METHOD("create_dsp", "name", "mode", "default_cooldown", "input_channels", "output_channels"), &FmodSystem::create_dsp, DEFVAL(FMOD_DSP_MODE_FILTER), DEFVAL(int64_t(FMOD_DSP_COOLDOWN_INFINITE)), DEFVAL(0), DEFVAL(0));
		ClassDB::bind_method(D_METHOD("create_dsp_connection", "type"), &FmodSystem::create_dsp_connection, DEFVAL(FMOD_DSPCONNECTION_TYPE_STANDARD));
		ClassDB::bind_method(D_METHOD("create_dsp_by_type", "type"), &FmodSystem::create_dsp_by_type);
		ClassDB::bind_method(D_METHOD("create_channel_group", "name"), &FmodSystem::create_channel_group);
		ClassDB::bind_method(D_METHOD("create_sound_group", "name"), &FmodSystem::create_sound_group);
		ClassDB::bind_method(D_METHOD("create_reverb_3d"), &FmodSystem::create_reverb_3d);
		ClassDB::bind_method(D_METHOD("play_sound", "sound", "channel_group", "paused"), &FmodSystem::play_sound, DEFVAL(false));
		ClassDB::bind_method(D_METHOD("play_dsp", "dsp", "channel_group", "paused"), &FmodSystem::play_dsp, DEFVAL(false));
		ClassDB::bind_method(D_METHOD("get_channel", "id"), &FmodSystem::get_channel);
		ClassDB::bind_method(D_METHOD("get_dsp_info_by_type", "type"), &FmodSystem::get_dsp_info_by_type);
		ClassDB::bind_method(D_METHOD("get_master_channel_group"), &FmodSystem::get_master_channel_group);
		ClassDB::bind_method(D_METHOD("get_master_sound_group"), &FmodSystem::get_master_sound_group);

		ClassDB::bind_method(D_METHOD("set_3d_listener_attributes", "listener_index", "position", "velocity", "forward", "up"), &FmodSystem::set_3d_listener_attributes);
		ClassDB::bind_method(D_METHOD("get_3d_listener_attributes", "listener_index"), &FmodSystem::get_3d_listener_attributes);
		ClassDB::bind_method(D_METHOD("set_reverb_properties",
			"instance",
			"decay_time",
			"early_time",
			"late_delay",
			"hf_reference",
			"hf_decay_ratio",
			"diffusion",
			"dennsity",
			"low_shelf_frequency",
			"low_shelf_gain",
			"high_cut",
			"early_late_mix",
			"wet_level"
		), &FmodSystem::set_reverb_properties);
		ClassDB::bind_method(D_METHOD("get_reverb_properties", "instance"), &FmodSystem::get_reverb_properties);
		ClassDB::bind_method(D_METHOD("attach_channel_group_to_port", "channel_group", "prot_type", "port_index", "pass_thru"), &FmodSystem::attach_channel_group_to_port, DEFVAL(FMOD_PORT_INDEX_NONE), DEFVAL(false));
		ClassDB::bind_method(D_METHOD("detach_channel_group_from_port", "channel_group"), &FmodSystem::detach_channel_group_from_port);

		ClassDB::bind_method(D_METHOD("get_record_num_drivers"), &FmodSystem::get_record_num_drivers);
		ClassDB::bind_method(D_METHOD("get_record_driver_info", "id"), &FmodSystem::get_record_driver_info);
		ClassDB::bind_method(D_METHOD("get_record_position", "id"), &FmodSystem::get_record_position);
		ClassDB::bind_method(D_METHOD("record_start", "id", "sound", "loop"), &FmodSystem::record_start);
		ClassDB::bind_method(D_METHOD("record_stop", "id"), &FmodSystem::record_stop);
		ClassDB::bind_method(D_METHOD("is_recording", "id"), &FmodSystem::is_recording);

		ClassDB::bind_method(D_METHOD("create_geometry", "max_polygons", "max_vertices"), &FmodSystem::create_geometry, DEFVAL(9999), DEFVAL(9999));

		ClassDB::bind_method(D_METHOD("set_3d_max_world_size", "max_world_size"), &FmodSystem::set_3d_max_world_size);
		ClassDB::bind_method(D_METHOD("get_3d_max_world_size"), &FmodSystem::get_3d_max_world_size);

		ClassDB::bind_method(D_METHOD("load_geometry", "data"), &FmodSystem::load_geometry);
		ClassDB::bind_method(D_METHOD("get_geometry_occlusion", "listener", "source"), &FmodSystem::get_geometry_occlusion);

		ClassDB::bind_method(D_METHOD("lock_dsp"), &FmodSystem::lock_dsp);
		ClassDB::bind_method(D_METHOD("unlock_dsp"), &FmodSystem::unlock_dsp);

		ADD_PROPERTY(PropertyInfo(Variant::INT, "max_software_channels"), "set_software_channels", "get_software_channels");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "doppler_scale"), "set_doppler_scale", "get_doppler_scale");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "distance_factor"), "set_distance_factor", "get_distance_factor");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "rolloff_scale"), "set_rolloff_scale", "get_rolloff_scale");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "3d_num_listeners"), "set_3d_num_listeners", "get_3d_num_listeners");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "max_mpeg_codecs"), "set_max_mpeg_codecs", "get_max_mpeg_codecs");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "max_adpcm_codecs"), "set_max_adpcm_codecs", "get_max_adpcm_codecs");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "max_xma_codecs"), "set_max_xma_codecs", "get_max_xma_codecs");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "max_vorbis_codecs"), "set_max_vorbis_codecs", "get_max_vorbis_codecs");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "max_at9_codecs"), "set_max_at9_codecs", "get_max_at9_codecs");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "max_fadpcm_codecs"), "set_max_fadpcm_codecs", "get_max_fadpcm_codecs");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "max_opus_codecs"), "set_max_opus_codecs", "get_max_opus_codecs");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "asio_num_channels"), "set_asio_num_channels", "get_asio_num_channels");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "vol0_virtual_vol"), "set_vol0_virtual_vol", "get_vol0_virtual_vol");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "default_decode_buffer_size"), "set_default_decode_buffer_size", "get_default_decode_buffer_size");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "profile_port"), "set_profile_port", "get_profile_port");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "geometry_max_fade_time"), "set_geometry_max_fade_time", "get_geometry_max_fade_time");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "distance_filter_center_freq"), "set_distance_filter_center_freq", "get_distance_filter_center_freq");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "reverb_3d_instance"), "set_reverb_3d_instance", "get_reverb_3d_instance");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "dsp_buffer_pool_size"), "set_dsp_buffer_pool_size", "get_dsp_buffer_pool_size");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "resampler_method", PROPERTY_HINT_ENUM, "Default,No Interp,Linear,Cubic,Spline"), "set_resampler_method", "get_resampler_method");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "random_seed"), "set_random_seed", "get_random_seed");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "max_convolution_threads"), "set_max_convolution_threads", "get_max_convolution_threads");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "max_spatial_objects"), "set_max_spatial_objects", "get_max_spatial_objects");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "3d_max_world_size", PROPERTY_HINT_RANGE, "0,999999,1"), "set_3d_max_world_size", "get_3d_max_world_size");
	}

	FmodSystem::FmodSystem() {
		system = nullptr;
	}

	FmodSystem::~FmodSystem() {
		if (system) {
			if (current_callback_system == this) {
				current_callback_system = nullptr;
				system->set3DRolloffCallback(nullptr);
			}

			void* userdata = nullptr;
			if (owns_system && system->getUserData(&userdata) == FMOD_OK && userdata == this) {
				system->setUserData(nullptr);
			}
		}
		else {
			_release_dsp_descriptions();
		}
	}

	void FmodSystem::_apply_advanced_settings() {
		ERR_FAIL_COND(!system);
		settings.cbSize = sizeof(FMOD_ADVANCEDSETTINGS);
		FMOD_ERR_CHECK(system->setAdvancedSettings(&settings));
	}

	void FmodSystem::_apply_3d_settings() {
		ERR_FAIL_COND(!system);
		FMOD_ERR_CHECK(system->set3DSettings(doppler_scale, distance_factor, rolloff_scale));
	}

	void FmodSystem::_release_dsp_descriptions() const {
		for (FMOD_DSP_DESCRIPTION* description : dsp_descriptions) {
			memdelete(description);
		}
		dsp_descriptions.clear();
	}

	Ref<FmodDSP> FmodSystem::_create_dsp_from_description(FMOD_DSP_DESCRIPTION* p_description) const {
		ERR_FAIL_COND_V(!system || !p_description, Ref<FmodDSP>());

		FMOD::DSP* dsp_ptr = nullptr;
		const FMOD_RESULT result = system->createDSP(p_description, &dsp_ptr);
		if (result != FMOD_OK || !dsp_ptr) {
			memdelete(p_description);
			if (result != FMOD_OK) {
				FMOD_ERR_CHECK(result);
			}
			return Ref<FmodDSP>();
		}

		dsp_descriptions.push_back(p_description);

		Ref<FmodDSP> dsp;
		dsp.instantiate();
		dsp->setup(dsp_ptr);
		return dsp;
	}

	Ref<FmodDSP> FmodSystem::create_dsp_from_description(const FMOD_DSP_DESCRIPTION& p_description) const {
		FMOD_DSP_DESCRIPTION* description = memnew(FMOD_DSP_DESCRIPTION);
		memcpy(description, &p_description, sizeof(FMOD_DSP_DESCRIPTION));
		return _create_dsp_from_description(description);
	}

	bool FmodSystem::system_is_valid() const {
		return system != nullptr;
	}

	bool FmodSystem::system_is_null() const {
		return system == nullptr;
	}

	FMOD::System* FmodSystem::get_system() const {
		return system;
	}

	void FmodSystem::setup(FMOD::System* p_system) {
		ERR_FAIL_COND_MSG(!p_system, "System pointer is null");
		ERR_FAIL_COND_MSG(system != nullptr, "FmodSystem is already bound to an FMOD System");

		system = p_system;
		sound_lifetime = std::make_shared<std::atomic_bool>(true);
		owns_system = true;
		system->setUserData(this);
	}

	void FmodSystem::setup_borrowed(FMOD::System* p_system) {
		ERR_FAIL_COND_MSG(!p_system, "System pointer is null");
		ERR_FAIL_COND_MSG(system != nullptr, "FmodSystem is already bound to an FMOD System");

		system = p_system;
		owns_system = false;
	}

	Ref<FmodSystem> FmodSystem::create_system() {
		Ref<FmodSystem> new_system;
		new_system.instantiate();
		FMOD::System* system_ptr = nullptr;
		FMOD_ERR_CHECK_V(FMOD::System_Create(&system_ptr), Ref<FmodSystem>());
		new_system->setup(system_ptr);
		return new_system;
	}

	void FmodSystem::init(const int max_channels, FmodInitFlags flags) {
		ERR_FAIL_COND(!system);
		ERR_FAIL_COND_MSG(max_channels <= 0, "max_channels must be greater than 0.");
		auto initflags = static_cast<FMOD_INITFLAGS>(flags);
		FMOD_ERR_CHECK(system->init(max_channels, initflags, nullptr));
	}

	void FmodSystem::close() {
		ERR_FAIL_COND(!system);
		ERR_FAIL_COND_MSG(!owns_system, "Cannot close a borrowed FMOD System");
		if (current_callback_system == this) {
			current_callback_system = nullptr;
			_3d_rolloff_callback = Callable();
			FMOD_ERR_CHECK(system->set3DRolloffCallback(nullptr));
		}
		FMOD_ERR_CHECK(system->close());
	}

	void FmodSystem::release() {
		ERR_FAIL_COND(!system);
		ERR_FAIL_COND_MSG(!owns_system, "Cannot release a borrowed FMOD System");
		const bool had_3d_rolloff_callback = current_callback_system == this;
		if (had_3d_rolloff_callback) {
			current_callback_system = nullptr;
			FMOD_ERR_CHECK(system->set3DRolloffCallback(nullptr));
		}
		void* userdata = nullptr;
		const bool had_userdata = system->getUserData(&userdata) == FMOD_OK && userdata == this;
		if (had_userdata) {
			system->setUserData(nullptr);
		}
		const bool sounds_were_alive = sound_lifetime->exchange(false);
		FMOD_RESULT result = system->release();
		if (result != FMOD_OK) {
			sound_lifetime->store(sounds_were_alive);
			ERR_PRINT(vformat("Failed to release FMOD System: %s", FMOD_ErrorString(result)));
			if (had_userdata) {
				FMOD_ERR_CHECK(system->setUserData(this));
			}
			if (had_3d_rolloff_callback) {
				current_callback_system = this;
				FMOD_ERR_CHECK(system->set3DRolloffCallback(GD_3D_ROLLOFF_CALLBACK));
			}
			return;
		}
		system = nullptr;
		owns_system = false;
		_3d_rolloff_callback = Callable();
		_release_dsp_descriptions();
	}

	void FmodSystem::update() {
		ERR_FAIL_COND(!system);
		FMOD_RESULT result = system->update();
		if (result != FMOD_OK)
			ERR_PRINT(vformat("FMOD System update failed: %s", FMOD_ErrorString(result)));
	}

	void FmodSystem::mixer_suspend() {
		ERR_FAIL_COND(!system);
		FMOD_ERR_CHECK(system->mixerSuspend());
	}

	void FmodSystem::mixer_resume() {
		ERR_FAIL_COND(!system);
		FMOD_ERR_CHECK(system->mixerResume());
	}

	void FmodSystem::set_output(FmodOutputType output_type) {
		ERR_FAIL_COND(!system);
		FMOD_ERR_CHECK(system->setOutput(static_cast<FMOD_OUTPUTTYPE>(output_type)));
	}

	FmodSystem::FmodOutputType FmodSystem::get_output() const {
		ERR_FAIL_COND_V(!system, FMOD_OUTPUT_TYPE_UNKNOWN);
		FMOD_OUTPUTTYPE output = FMOD_OUTPUTTYPE_UNKNOWN;
		FMOD_ERR_CHECK_V(system->getOutput(&output), FMOD_OUTPUT_TYPE_UNKNOWN);
		return static_cast<FmodOutputType>(output);
	}

	int FmodSystem::get_num_drivers() const {
		ERR_FAIL_COND_V(!system, 0);
		int num = 0;
		FMOD_ERR_CHECK_V(system->getNumDrivers(&num), 0);
		return num;
	}

	Dictionary FmodSystem::get_driver_info(const int id) const {
		ERR_FAIL_COND_V(!system, Dictionary());
		ERR_FAIL_COND_V_MSG(id < 0, Dictionary(), "Driver id must be >= 0.");
		char name[256] = { 0 };
		FMOD_GUID guid;
		int system_rate = 0;
		FMOD_SPEAKERMODE speaker_mode = FMOD_SPEAKERMODE_DEFAULT;
		int speaker_mode_channels = 0;
		FMOD_ERR_CHECK_V(system->getDriverInfo(
			id, name, 256, &guid, &system_rate, &speaker_mode, &speaker_mode_channels),
			Dictionary()
		);
		Dictionary result;
		result["name"] = String::utf8(name);
		result["guid"] = FmodUtils::guid_to_string(guid);
		result["system_rate"] = system_rate;
		result["speaker_mode"] = static_cast<FmodSpeakerMode>(speaker_mode);
		result["speaker_mode_channels"] = speaker_mode_channels;
		return result;
	}

	void FmodSystem::set_driver(const int driver) {
		ERR_FAIL_COND(!system);
		ERR_FAIL_COND_MSG(driver < 0, "Driver id must be >= 0.");
		FMOD_ERR_CHECK(system->setDriver(driver));
	}

	int FmodSystem::get_driver() const {
		ERR_FAIL_COND_V(!system, 0);
		int driver = -1;
		FMOD_ERR_CHECK_V(system->getDriver(&driver), -1);
		return driver;
	}

	void FmodSystem::set_software_channels(const int num_software_channels) {
		ERR_FAIL_COND(!system);
		ERR_FAIL_COND_MSG(num_software_channels <= 0, "num_software_channels must be greater than 0.");
		FMOD_ERR_CHECK(system->setSoftwareChannels(num_software_channels));
	}

	int FmodSystem::get_software_channels() const {
		if (!system) return 0;
		int numsoftwarechannels = 0;
		FMOD_ERR_CHECK_V(system->getSoftwareChannels(&numsoftwarechannels), 0);
		return numsoftwarechannels;
	}

	void FmodSystem::set_software_format(
		const int sample_rate,
		FmodSpeakerMode speaker_mode,
		const int num_raw_speakers
	) {
		ERR_FAIL_COND(!system);
		ERR_FAIL_COND_MSG(sample_rate <= 0, "sample_rate must be greater than 0.");
		ERR_FAIL_COND_MSG(speaker_mode == FMOD_SPEAKER_MODE_RAW && num_raw_speakers <= 0, "num_raw_speakers must be greater than 0 when speaker_mode is RAW.");
		FMOD_SPEAKERMODE fmod_speaker_mode = static_cast<FMOD_SPEAKERMODE>(speaker_mode);
		FMOD_ERR_CHECK(system->setSoftwareFormat(sample_rate, fmod_speaker_mode, num_raw_speakers));
	}

	Dictionary FmodSystem::get_software_format() const {
		ERR_FAIL_COND_V(!system, Dictionary());
		int samplerate = 0, numrawspeakers = 0;
		FMOD_SPEAKERMODE fmod_speaker_mode = FMOD_SPEAKERMODE_DEFAULT;
		FMOD_ERR_CHECK_V(system->getSoftwareFormat(&samplerate, &fmod_speaker_mode, &numrawspeakers), Dictionary());
		Dictionary result;
		result["sample_rate"] = samplerate;
		result["speaker_mode"] = static_cast<FmodSpeakerMode>(fmod_speaker_mode);
		result["num_raw_speakers"] = numrawspeakers;
		return result;
	}

	void FmodSystem::set_dsp_buffer_size(const unsigned int buffer_length, const int num_buffers) {
		ERR_FAIL_COND(!system);
		ERR_FAIL_COND_MSG(buffer_length == 0, "buffer_length must be greater than 0.");
		ERR_FAIL_COND_MSG(num_buffers <= 0, "num_buffers must be greater than 0.");
		FMOD_ERR_CHECK(system->setDSPBufferSize(buffer_length, num_buffers));
	}

	Dictionary FmodSystem::get_dsp_buffer_size() const {
		ERR_FAIL_COND_V(!system, Dictionary());
		unsigned int bufferlength;
		int numbuffers;
		FMOD_ERR_CHECK_V(system->getDSPBufferSize(&bufferlength, &numbuffers), Dictionary());
		Dictionary result;
		result["buffer_length"] = static_cast<uint32_t>(bufferlength);
		result["num_buffers"] = numbuffers;
		return result;
	}

	void FmodSystem::set_stream_buffer_size(const unsigned int file_buffer_size, FmodTimeUnit file_buffer_size_type) {
		ERR_FAIL_COND(!system);
		ERR_FAIL_COND_MSG(file_buffer_size == 0, "file_buffer_size must be greater than 0.");
		const bool valid_time_unit =
			file_buffer_size_type == FMOD_TIME_UNIT_MS ||
			file_buffer_size_type == FMOD_TIME_UNIT_PCM ||
			file_buffer_size_type == FMOD_TIME_UNIT_PCMBYTES ||
			file_buffer_size_type == FMOD_TIME_UNIT_RAWBYTES;
		ERR_FAIL_COND_MSG(!valid_time_unit, "file_buffer_size_type is invalid.");
		FMOD_TIMEUNIT fmod_file_buffer_size_type = static_cast<FMOD_TIMEUNIT>(file_buffer_size_type);
		FMOD_ERR_CHECK(system->setStreamBufferSize(file_buffer_size, fmod_file_buffer_size_type));
	}

	Dictionary FmodSystem::get_stream_buffer_size() const {
		ERR_FAIL_COND_V(!system, Dictionary());
		unsigned int filebuffersize = 0;
		FMOD_TIMEUNIT filebuffersizetype = FMOD_TIMEUNIT_RAWBYTES;
		FMOD_ERR_CHECK_V(system->getStreamBufferSize(&filebuffersize, &filebuffersizetype), Dictionary());
		Dictionary result;
		result["file_buffer_size"] = static_cast<uint64_t>(filebuffersize);
		result["file_buffer_size_type"] = static_cast<FmodTimeUnit>(filebuffersizetype);
		return result;
	}

	void FmodSystem::set_speaker_position(FmodSpeaker speaker, const float x, const float y, const bool active) {
		ERR_FAIL_COND(!system);
		FMOD_SPEAKER fmod_speaker = static_cast<FMOD_SPEAKER>(speaker);
		float clamped_x = CLAMP(x, -1.0f, 1.0f);
		float clamped_y = CLAMP(y, -1.0f, 1.0f);
		FMOD_ERR_CHECK(system->setSpeakerPosition(fmod_speaker, clamped_x, clamped_y, active));
	}

	Dictionary FmodSystem::get_speaker_position(FmodSpeaker speaker) const {
		ERR_FAIL_COND_V(!system, Dictionary());
		FMOD_SPEAKER fmod_speaker = static_cast<FMOD_SPEAKER>(speaker);
		float x = 0.0f, y = 0.0f;
		bool active = false;
		FMOD_ERR_CHECK_V(system->getSpeakerPosition(fmod_speaker, &x, &y, &active), Dictionary());
		Dictionary result;
		result["x"] = x;
		result["y"] = y;
		result["active"] = active;
		return result;
	}

	void FmodSystem::set_doppler_scale(const float _doppler_scale) {
		ERR_FAIL_COND(!system);
		doppler_scale = _doppler_scale;
		_apply_3d_settings();
	}

	float FmodSystem::get_doppler_scale() const {
		return doppler_scale;
	}

	void FmodSystem::set_distance_factor(const float _distance_factor) {
		ERR_FAIL_COND(!system);
		distance_factor = _distance_factor;
		_apply_3d_settings();
	}

	float FmodSystem::get_distance_factor() const {
		return distance_factor;
	}

	void FmodSystem::set_rolloff_scale(const float _rolloff_scale) {
		ERR_FAIL_COND(!system);
		rolloff_scale = _rolloff_scale;
		_apply_3d_settings();
	}

	float FmodSystem::get_rolloff_scale() const {
		return rolloff_scale;
	}

	void FmodSystem::set_3d_num_listeners(const int num_listeners) {
		ERR_FAIL_COND(!system);
		ERR_FAIL_COND_MSG(num_listeners < 1 || num_listeners > 8, "FMOD supports 1 to 8 listeners.");
		FMOD_ERR_CHECK(system->set3DNumListeners(num_listeners));
	}

	int FmodSystem::get_3d_num_listeners() const {
		if (!system) return 1;
		int numlisteners = 1;
		FMOD_ERR_CHECK_V(system->get3DNumListeners(&numlisteners), 1);
		return numlisteners;
	}

	void FmodSystem::set_max_mpeg_codecs(const int max_codecs) {
		settings.maxMPEGCodecs = max_codecs;
		_apply_advanced_settings();
	}

	int FmodSystem::get_max_mpeg_codecs() const {
		return settings.maxMPEGCodecs;
	}

	void FmodSystem::set_max_adpcm_codecs(const int max_codecs) {
		settings.maxADPCMCodecs = max_codecs;
		_apply_advanced_settings();
	}

	int FmodSystem::get_max_adpcm_codecs() const {
		return settings.maxADPCMCodecs;
	}

	void FmodSystem::set_max_xma_codecs(const int max_codecs) {
		settings.maxXMACodecs = max_codecs;
		_apply_advanced_settings();
	}

	int FmodSystem::get_max_xma_codecs() const {
		return settings.maxXMACodecs;
	}

	void FmodSystem::set_max_vorbis_codecs(const int max_codecs) {
		settings.maxVorbisCodecs = max_codecs;
		_apply_advanced_settings();
	}

	int FmodSystem::get_max_vorbis_codecs() const {
		return settings.maxVorbisCodecs;
	}

	void FmodSystem::set_max_at9_codecs(const int max_codecs) {
		settings.maxAT9Codecs = max_codecs;
		_apply_advanced_settings();
	}

	int FmodSystem::get_max_at9_codecs() const {
		return settings.maxAT9Codecs;
	}

	void FmodSystem::set_max_fadpcm_codecs(const int max_codecs) {
		settings.maxFADPCMCodecs = max_codecs;
		_apply_advanced_settings();
	}

	int FmodSystem::get_max_fadpcm_codecs() const {
		return settings.maxFADPCMCodecs;
	}

	void FmodSystem::set_max_opus_codecs(const int max_codecs) {
		settings.maxOpusCodecs = max_codecs;
		_apply_advanced_settings();
	}

	int FmodSystem::get_max_opus_codecs() const {
		return settings.maxOpusCodecs;
	}

	void FmodSystem::set_asio_num_channels(const int num_channels) {
		settings.ASIONumChannels = num_channels;
		_apply_advanced_settings();
	}

	int FmodSystem::get_asio_num_channels() const {
		return settings.ASIONumChannels;
	}

	void FmodSystem::set_vol0_virtual_vol(const float vol) {
		settings.vol0virtualvol = vol;
		_apply_advanced_settings();
	}

	float FmodSystem::get_vol0_virtual_vol() const {
		return settings.vol0virtualvol;
	}

	void FmodSystem::set_default_decode_buffer_size(const unsigned int size) {
		settings.defaultDecodeBufferSize = size;
		_apply_advanced_settings();
	}

	unsigned int FmodSystem::get_default_decode_buffer_size() const {
		return settings.defaultDecodeBufferSize;
	}

	void FmodSystem::set_profile_port(const unsigned short port) {
		settings.profilePort = port;
		_apply_advanced_settings();
	}

	unsigned short FmodSystem::get_profile_port() const {
		return settings.profilePort;
	}

	void FmodSystem::set_geometry_max_fade_time(const unsigned int time) {
		settings.geometryMaxFadeTime = time;
		_apply_advanced_settings();
	}

	unsigned int FmodSystem::get_geometry_max_fade_time() const {
		return settings.geometryMaxFadeTime;
	}

	void FmodSystem::set_distance_filter_center_freq(const float freq) {
		settings.distanceFilterCenterFreq = freq;
		_apply_advanced_settings();
	}

	float FmodSystem::get_distance_filter_center_freq() const {
		return settings.distanceFilterCenterFreq;
	}

	void FmodSystem::set_reverb_3d_instance(const int instance) {
		settings.reverb3Dinstance = instance;
		_apply_advanced_settings();
	}

	int FmodSystem::get_reverb_3d_instance() const {
		return settings.reverb3Dinstance;
	}

	void FmodSystem::set_dsp_buffer_pool_size(const int size) {
		settings.DSPBufferPoolSize = size;
		_apply_advanced_settings();
	}

	int FmodSystem::get_dsp_buffer_pool_size() const {
		return settings.DSPBufferPoolSize;
	}

	void FmodSystem::set_resampler_method(FmodResamplerMethod method) {
		settings.resamplerMethod = static_cast<FMOD_DSP_RESAMPLER>(method);
		_apply_advanced_settings();
	}

	FmodSystem::FmodResamplerMethod FmodSystem::get_resampler_method() const {
		return static_cast<FmodResamplerMethod>(settings.resamplerMethod);
	}

	void FmodSystem::set_random_seed(const unsigned int seed) {
		settings.randomSeed = seed;
		_apply_advanced_settings();
	}

	unsigned int FmodSystem::get_random_seed() const {
		return settings.randomSeed;
	}

	void FmodSystem::set_max_convolution_threads(const int max_threads) {
		settings.maxConvolutionThreads = max_threads;
		_apply_advanced_settings();
	}

	int FmodSystem::get_max_convolution_threads() const {
		return settings.maxConvolutionThreads;
	}

	void FmodSystem::set_max_spatial_objects(const int max_objects) {
		settings.maxSpatialObjects = max_objects;
		_apply_advanced_settings();
	}

	int FmodSystem::get_max_spatial_objects() const {
		return settings.maxSpatialObjects;
	}

	void FmodSystem::set_network_proxy(const String& p_proxy) {
		ERR_FAIL_COND(!system);
		CharString proxy_utf8 = p_proxy.utf8();
		FMOD_ERR_CHECK(system->setNetworkProxy(proxy_utf8.get_data()));
	}

	String FmodSystem::get_network_proxy() const {
		ERR_FAIL_COND_V(!system, String());
		char proxy[256] = { 0 };
		FMOD_ERR_CHECK_V(system->getNetworkProxy(proxy, 256), String());
		return String::utf8(proxy);
	}

	void FmodSystem::set_network_timeout(const int timeout) {
		ERR_FAIL_COND(!system);
		ERR_FAIL_COND_MSG(timeout < 0, "timeout must be >= 0.");
		FMOD_ERR_CHECK(system->setNetworkTimeout(timeout));
	}

	int FmodSystem::get_network_timeout() const {
		ERR_FAIL_COND_V(!system, 0);
		int timeout = -1;
		FMOD_ERR_CHECK_V(system->getNetworkTimeout(&timeout), -1);
		return timeout;
	}

	Dictionary FmodSystem::get_version() const {
		ERR_FAIL_COND_V(!system, Dictionary());
		unsigned int version = 0;
		unsigned int buildnumber = 0;
		FMOD_ERR_CHECK_V(system->getVersion(&version, &buildnumber), Dictionary());
		Dictionary result;
		unsigned int product = 0, major = 0, minor = 0;
		product = (version >> 16) & 0xFFFF;									// 产品版本号 (高 16 位)
		major = (version >> 8) & 0xFF;										// 主要版本号 (中间 8 位)
		minor = version & 0xFF;												// 次要版本号 (低 8 位)
		String version_str = vformat("%x.%x.%x", product, major, minor);
		result["version"] = version_str;
		product = (buildnumber >> 16) & 0xFFFF;								// 产品版本号 (高 16 位)
		major = (buildnumber >> 8) & 0xFF;									// 主要版本号 (中间 8 位)
		minor = buildnumber & 0xFF;											// 次要版本号 (低 8 位)
		String buildnumber_str = vformat("%x.%x.%x", product, major, minor);
		result["build_number"] = buildnumber_str;
		return result;
	}

	uint64_t FmodSystem::get_output_handle() const {
		ERR_FAIL_COND_V(!system, 0);
		void* handle = nullptr;
		FMOD_ERR_CHECK_V(system->getOutputHandle(&handle), 0);
		return reinterpret_cast<uint64_t>(handle);
	}

	Dictionary FmodSystem::get_channels_playing() const {
		ERR_FAIL_COND_V(!system, Dictionary());
		int channels = 0;
		int real_channels = 0;
		FMOD_ERR_CHECK_V(system->getChannelsPlaying(&channels, &real_channels), Dictionary());
		Dictionary result;
		result["channels"] = channels;
		result["real_channels"] = real_channels;
		return result;
	}

	Dictionary FmodSystem::get_cpu_usage() const {
		ERR_FAIL_COND_V(!system, Dictionary());
		Dictionary result;
		FMOD_CPU_USAGE cpu_usage;
		FMOD_ERR_CHECK_V(system->getCPUUsage(&cpu_usage), Dictionary());
		result["dsp"] = cpu_usage.dsp;
		result["stream"] = cpu_usage.stream;
		result["geometry"] = cpu_usage.geometry;
		result["update"] = cpu_usage.update;
		result["convolution1"] = cpu_usage.convolution1;
		result["convolution2"] = cpu_usage.convolution2;
		return result;
	}

	Dictionary FmodSystem::get_file_usage() const {
		ERR_FAIL_COND_V(!system, Dictionary());
		long long sample_bytes_read = 0, stream_bytes_read = 0, other_bytes_read = 0;
		FMOD_ERR_CHECK_V(system->getFileUsage(&sample_bytes_read, &stream_bytes_read, &other_bytes_read), Dictionary());
		Dictionary result;
		result["sample_bytes_read"] = static_cast<int64_t>(sample_bytes_read);
		result["stream_bytes_read"] = static_cast<int64_t>(stream_bytes_read);
		result["other_bytes_read"] = static_cast<int64_t>(other_bytes_read);
		return result;
	}

	PackedFloat32Array FmodSystem::get_default_mix_matrix(
		const FmodSpeakerMode source_speaker_mode,
		const FmodSpeakerMode target_speaker_mode,
		const int array_length,
		const int hop) const {
		ERR_FAIL_COND_V(!system, PackedFloat32Array());

		FMOD_SPEAKERMODE fmod_source = static_cast<FMOD_SPEAKERMODE>(source_speaker_mode);
		FMOD_SPEAKERMODE fmod_target = static_cast<FMOD_SPEAKERMODE>(target_speaker_mode);
		ERR_FAIL_COND_V_MSG(fmod_source == FMOD_SPEAKERMODE_RAW || fmod_target == FMOD_SPEAKERMODE_RAW, PackedFloat32Array(), "RAW speaker mode is not supported by getDefaultMixMatrix.");

		int source_channels = 0;
		int target_channels = 0;
		FMOD_ERR_CHECK_V(system->getSpeakerModeChannels(fmod_source, &source_channels), PackedFloat32Array());
		FMOD_ERR_CHECK_V(system->getSpeakerModeChannels(fmod_target, &target_channels), PackedFloat32Array());
		ERR_FAIL_COND_V_MSG(source_channels <= 0 || target_channels <= 0, PackedFloat32Array(), "Invalid source or target speaker channel count.");

		const int matrix_hop = hop <= 0 ? source_channels : hop;
		ERR_FAIL_COND_V_MSG(matrix_hop < source_channels, PackedFloat32Array(), "hop must be 0 or greater than or equal to the source channel count.");
		ERR_FAIL_COND_V_MSG(matrix_hop > FMOD_MAX_CHANNEL_WIDTH, PackedFloat32Array(), "hop cannot exceed FMOD_MAX_CHANNEL_WIDTH.");

		const int compact_size = source_channels * target_channels;
		const int required_size = target_channels * matrix_hop;
		ERR_FAIL_COND_V_MSG(array_length > 0 && array_length < required_size, PackedFloat32Array(), "array_length is too small for the requested mix matrix.");

		PackedFloat32Array matrix_storage;
		matrix_storage.resize(array_length > 0 ? array_length : required_size);
		float* matrix = matrix_storage.ptrw();
		for (int i = 0; i < matrix_storage.size(); i++) {
			matrix[i] = 0.0f;
		}

		FMOD_RESULT result = system->getDefaultMixMatrix(
			fmod_source,
			fmod_target,
			matrix,
			matrix_hop
		);

		if (result != FMOD_OK) {
			FMOD_ERR_CHECK_V(result, PackedFloat32Array());
			return PackedFloat32Array();
		}

		PackedFloat32Array mix_matrix;
		mix_matrix.resize(compact_size);
		float* compact_matrix = mix_matrix.ptrw();

		for (int target = 0; target < target_channels; target++) {
			for (int source = 0; source < source_channels; source++) {
				compact_matrix[target * source_channels + source] = matrix[target * matrix_hop + source];
			}
		}

		return mix_matrix;
	}

	int FmodSystem::get_speaker_mode_channels(FmodSpeakerMode mode) const {
		ERR_FAIL_COND_V(!system, -1);
		FMOD_SPEAKERMODE fmod_speaker_mode = static_cast<FMOD_SPEAKERMODE>(mode);
		int channels = 0;
		FMOD_ERR_CHECK_V(system->getSpeakerModeChannels(fmod_speaker_mode, &channels), -1);
		return channels;
	}

	Ref<FmodSound> FmodSystem::create_sound_from_file(const String p_path, unsigned int mode, FmodResamplerMethod resampler_method) const {
		ERR_FAIL_COND_V(!system, Ref<FmodSound>());
		ERR_FAIL_COND_V_MSG(p_path.is_empty(), Ref<FmodSound>(), "Sound path cannot be empty.");

		// 检查是否为资源文件路径，如果是则使用资源文件加载 FmodSound
		if (p_path.begins_with("res://")) {
			return create_sound_from_res(p_path, mode, resampler_method);
		}

		// 转换字符串为对象，防止野指针
		ProjectSettings* project_settings = ProjectSettings::get_singleton();
		ERR_FAIL_NULL_V(project_settings, Ref<FmodSound>());
		CharString path_utf8 = project_settings->globalize_path(p_path).utf8();
		const char* path_cstr = path_utf8.get_data();
		FMOD::Sound* sound_ptr = nullptr;

		FMOD_CREATESOUNDEXINFO exinfo = {};
		exinfo.cbsize = sizeof(exinfo);
		exinfo.resamplermethod = static_cast<FMOD_DSP_RESAMPLER>(resampler_method);
		FMOD_ERR_CHECK_V(system->createSound(
			path_cstr,
			mode,
			&exinfo,
			&sound_ptr
		), Ref<FmodSound>());

		Ref<FmodSound> fmod_sound;
		fmod_sound.instantiate();
		ERR_FAIL_NULL_V(sound_ptr, Ref<FmodSound>());
		fmod_sound->setup(sound_ptr, true, sound_lifetime);
		return fmod_sound;
	}

	Ref<FmodSound> FmodSystem::create_sound_from_memory(const PackedByteArray& data, unsigned int mode, FmodResamplerMethod resampler_method) const {
		ERR_FAIL_COND_V(!system || data.is_empty(), Ref<FmodSound>());
		ERR_FAIL_COND_V_MSG(data.size() > static_cast<int64_t>(std::numeric_limits<unsigned int>::max()), Ref<FmodSound>(), "Sound data is too large for FMOD OPENMEMORY.");

		FMOD::Sound* sound_ptr = nullptr;
		Ref<FmodSound> sound;
		sound.instantiate();
		sound->data = data;
		FMOD_CREATESOUNDEXINFO exinfo = {};
		exinfo.cbsize = sizeof(exinfo);
		exinfo.resamplermethod = static_cast<FMOD_DSP_RESAMPLER>(resampler_method);
		exinfo.length = static_cast<unsigned int>(sound->data.size());

		if (!(mode & FMOD_MODE_OPENMEMORY)) {
			mode |= FMOD_MODE_OPENMEMORY;
		}

		FMOD_ERR_CHECK_V(system->createSound(
			reinterpret_cast<const char*>(sound->data.ptr()),
			mode,
			&exinfo,
			&sound_ptr
		), Ref<FmodSound>());

		ERR_FAIL_NULL_V(sound_ptr, Ref<FmodSound>());
		sound->setup(sound_ptr, true, sound_lifetime);
		return sound;
	}

	Ref<FmodSound> FmodSystem::create_sound_from_res(const String p_path, unsigned int mode, FmodResamplerMethod resampler_method) const {
		ERR_FAIL_COND_V(!system, Ref<FmodSound>());
		ERR_FAIL_COND_V_MSG(p_path.is_empty(), Ref<FmodSound>(), "Resource sound path cannot be empty.");

		// 从文件加载
		Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
		ERR_FAIL_COND_V(file.is_null(), Ref<FmodSound>());
		ERR_FAIL_COND_V_MSG(file->get_length() > static_cast<uint64_t>(std::numeric_limits<unsigned int>::max()), Ref<FmodSound>(), "Resource sound is too large for FMOD OPENMEMORY.");

		// 读取文件数据到内存
		PackedByteArray data = file->get_buffer(file->get_length());
		ERR_FAIL_COND_V(data.is_empty(), Ref<FmodSound>());

		// 以内存模式创建 FMOD Sound（添加 OPENMEMORY 标志）
		return create_sound_from_memory(data, mode, resampler_method);
	}

	Ref<FmodSound> FmodSystem::create_stream_from_file(const String p_path, unsigned int mode, FmodResamplerMethod resampler_method) const {
		ERR_FAIL_COND_V(!system, Ref<FmodSound>());
		ERR_FAIL_COND_V_MSG(p_path.is_empty(), Ref<FmodSound>(), "Stream path cannot be empty.");

		unsigned int stream_mode = mode | FMOD_MODE_CREATESTREAM;
		stream_mode &= ~FMOD_MODE_CREATESAMPLE;
		stream_mode &= ~FMOD_MODE_CREATECOMPRESSEDSAMPLE;

		if (p_path.begins_with("res://")) {
			Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
			ERR_FAIL_COND_V(file.is_null(), Ref<FmodSound>());
			ERR_FAIL_COND_V_MSG(file->get_length() > static_cast<uint64_t>(std::numeric_limits<unsigned int>::max()), Ref<FmodSound>(), "Resource stream is too large for FMOD OPENMEMORY.");

			PackedByteArray data = file->get_buffer(file->get_length());
			ERR_FAIL_COND_V(data.is_empty(), Ref<FmodSound>());

			stream_mode |= FMOD_MODE_OPENMEMORY;

			Ref<FmodSound> sound;
			sound.instantiate();
			sound->data = data;
			sound->file_path = p_path;

			FMOD_CREATESOUNDEXINFO exinfo = {};
			exinfo.cbsize = sizeof(exinfo);
			exinfo.resamplermethod = static_cast<FMOD_DSP_RESAMPLER>(resampler_method);
			exinfo.length = static_cast<unsigned int>(sound->data.size());

			FMOD::Sound* sound_ptr = nullptr;
			FMOD_ERR_CHECK_V(system->createStream(
				reinterpret_cast<const char*>(sound->data.ptr()),
				stream_mode,
				&exinfo,
				&sound_ptr
			), Ref<FmodSound>());

			ERR_FAIL_NULL_V(sound_ptr, Ref<FmodSound>());
			sound->setup(sound_ptr, true, sound_lifetime);
			return sound;
		}

		// 转换字符串为对象，防止野指针
		ProjectSettings* project_settings = ProjectSettings::get_singleton();
		ERR_FAIL_NULL_V(project_settings, Ref<FmodSound>());
		CharString path_utf8 = project_settings->globalize_path(p_path).utf8();
		const char* path_cstr = path_utf8.get_data();

		FMOD::Sound* sound_ptr = nullptr;
		FMOD_CREATESOUNDEXINFO exinfo = {};
		exinfo.cbsize = sizeof(exinfo);
		exinfo.resamplermethod = static_cast<FMOD_DSP_RESAMPLER>(resampler_method);
		FMOD_ERR_CHECK_V(system->createStream(
			path_cstr,
			stream_mode,
			&exinfo,
			&sound_ptr
		), Ref<FmodSound>());

		Ref<FmodSound> sound;
		sound.instantiate();
		ERR_FAIL_NULL_V(sound_ptr, Ref<FmodSound>());
		sound->setup(sound_ptr, true, sound_lifetime);
		sound->file_path = p_path;
		return sound;
	}

	Ref<FmodDSP> FmodSystem::create_dsp(const String& name, unsigned int mode, unsigned int default_cooldown, int input_channels, int output_channels) const {
		ERR_FAIL_COND_V(mode > FMOD_DSP_MODE_GENERATOR || input_channels < 0 || output_channels < 0 || input_channels > FMOD_MAX_CHANNEL_WIDTH || output_channels > FMOD_MAX_CHANNEL_WIDTH, Ref<FmodDSP>());
		ERR_FAIL_COND_V(!system, Ref<FmodDSP>());

		// FMOD 直接引用描述符，描述符由 FmodSystem 持有到 System::release
		FMOD_DSP_DESCRIPTION* desc = memnew(FMOD_DSP_DESCRIPTION);
		memset(desc, 0, sizeof(FMOD_DSP_DESCRIPTION));

		desc->pluginsdkversion = FMOD_PLUGIN_SDK_VERSION;
		const CharString name_utf8 = name.utf8();
		strncpy(desc->name, name_utf8.get_data(), sizeof(desc->name) - 1);
		desc->mode = static_cast<FMOD_DSP_MODE>(mode);
		desc->defaultcooldown = default_cooldown;
		desc->inchannels = input_channels;
		desc->outchannels = output_channels;

		// 使用 fmod_dsp.cpp 中定义的回调函数
		// 这些回调会通过 userdata 找到 FmodDSP 实例并调用 GDScript Callable
		desc->create = GD_FMOD_DSP_CREATE_CALLBACK;
		desc->release = GD_FMOD_DSP_RELEASE_CALLBACK;
		desc->reset = GD_FMOD_DSP_RESET_CALLBACK;
		desc->process = GD_FMOD_DSP_PROCESS_CALLBACK;
		desc->setposition = GD_FMOD_DSP_SETPOSITION_CALLBACK;
		desc->setparameterfloat = GD_FMOD_DSP_SETPARAM_FLOAT_CALLBACK;
		desc->setparameterint = GD_FMOD_DSP_SETPARAM_INT_CALLBACK;
		desc->setparameterbool = GD_FMOD_DSP_SETPARAM_BOOL_CALLBACK;
		desc->setparameterdata = GD_FMOD_DSP_SETPARAM_DATA_CALLBACK;
		desc->getparameterfloat = GD_FMOD_DSP_GETPARAM_FLOAT_CALLBACK;
		desc->getparameterint = GD_FMOD_DSP_GETPARAM_INT_CALLBACK;
		desc->getparameterbool = GD_FMOD_DSP_GETPARAM_BOOL_CALLBACK;
		desc->getparameterdata = GD_FMOD_DSP_GETPARAM_DATA_CALLBACK;

		return _create_dsp_from_description(desc);
	}

	Ref<FmodDSP> FmodSystem::create_dsp_by_type(unsigned int type) const {
		ERR_FAIL_COND_V(!system, Ref<FmodDSP>());
		ERR_FAIL_COND_V_MSG(type >= FMOD_DSP_TYPE_MAX, Ref<FmodDSP>(), "Invalid built-in DSP type.");
		FMOD::DSP* dsp_ptr = nullptr;
		FMOD_ERR_CHECK_V(system->createDSPByType(static_cast<FMOD_DSP_TYPE>(type), &dsp_ptr), Ref<FmodDSP>());
		Ref<FmodDSP> dsp;
		dsp.instantiate();
		ERR_FAIL_NULL_V(dsp_ptr, Ref<FmodDSP>());
		dsp->setup(dsp_ptr);
		return dsp;
	}

	Ref<FmodChannelGroup> FmodSystem::create_channel_group(const String& p_name) const {
		ERR_FAIL_COND_V(!system, Ref<FmodChannelGroup>());
		FMOD::ChannelGroup* channel_group_ptr = nullptr;
		FMOD_ERR_CHECK_V(
			system->createChannelGroup(p_name.utf8().get_data(), &channel_group_ptr),
			Ref<FmodChannelGroup>()
		);
		Ref<FmodChannelGroup> channel_group;
		channel_group.instantiate();
		ERR_FAIL_NULL_V(channel_group_ptr, Ref<FmodChannelGroup>());
		channel_group->setup(channel_group_ptr);
		return channel_group;
	}

	Ref<FmodSoundGroup> FmodSystem::create_sound_group(const String& p_name) const {
		ERR_FAIL_COND_V(!system, Ref<FmodSoundGroup>());
		FMOD::SoundGroup* sound_group_ptr = nullptr;
		FMOD_ERR_CHECK_V(
			system->createSoundGroup(p_name.utf8().get_data(), &sound_group_ptr),
			Ref<FmodSoundGroup>()
		);
		Ref<FmodSoundGroup> sound_group;
		sound_group.instantiate();
		ERR_FAIL_NULL_V(sound_group_ptr, Ref<FmodSoundGroup>());
		sound_group->setup(sound_group_ptr);
		return sound_group;
	}

	Ref<FmodReverb3D> FmodSystem::create_reverb_3d() const {
		ERR_FAIL_COND_V(!system, Ref<FmodReverb3D>());
		FMOD::Reverb3D* reverb_ptr = nullptr;
		FMOD_ERR_CHECK_V(system->createReverb3D(&reverb_ptr), Ref<FmodReverb3D>());
		Ref<FmodReverb3D> reverb;
		reverb.instantiate();
		ERR_FAIL_NULL_V(reverb_ptr, Ref<FmodReverb3D>());
		reverb->setup(reverb_ptr);
		return reverb;
	}

	Ref<FmodChannel> FmodSystem::play_sound(Ref<FmodSound> sound, Ref<FmodChannelGroup> channel_group, const bool paused) const {
		ERR_FAIL_COND_V(
			!system || sound.is_null() || !sound->sound || channel_group.is_null() || !channel_group->channel_group,
			Ref<FmodChannel>()
		);

		FMOD::Channel* fmod_channel = nullptr;
		FMOD_ERR_CHECK_V(system->playSound(
			sound->sound,
			channel_group->channel_group,
			paused,
			&fmod_channel
		), nullptr);

		if (!fmod_channel) {
			ERR_PRINT("playSound returned null channel");
			return nullptr;
		}

		Ref<FmodChannel> channel;
		channel.instantiate();
		channel->setup(fmod_channel);
		return channel;
	}

	Ref<FmodChannel> FmodSystem::play_dsp(Ref<FmodDSP> dsp, Ref<FmodChannelGroup> channel_group, const bool paused) const {
		ERR_FAIL_COND_V(
			!system || dsp.is_null() || !dsp->dsp || channel_group.is_null() || !channel_group->channel_group,
			Ref<FmodChannel>()
		);

		FMOD::Channel* fmod_channel = nullptr;
		FMOD_ERR_CHECK_V(system->playDSP(
			dsp->dsp,
			channel_group->channel_group,
			paused,
			&fmod_channel
		), Ref<FmodChannel>());

		if (!fmod_channel) {
			ERR_PRINT("playSound returned null channel");
			return Ref<FmodChannel>();
		}

		Ref<FmodChannel> channel;
		channel.instantiate();
		channel->setup(fmod_channel);
		return channel;
	}

	Ref<FmodChannel> FmodSystem::get_channel(const int id) const {
		ERR_FAIL_COND_V(!system, Ref<FmodChannel>());
		ERR_FAIL_COND_V_MSG(id < 0, Ref<FmodChannel>(), "Channel id must be >= 0.");
		FMOD::Channel* fmod_channel = nullptr;
		FMOD_ERR_CHECK_V(system->getChannel(id, &fmod_channel), Ref<FmodChannel>());
		ERR_FAIL_NULL_V(fmod_channel, Ref<FmodChannel>());
		Ref<FmodChannel> channel;
		channel.instantiate();
		channel->setup(fmod_channel);
		return channel;
	}

	Dictionary FmodSystem::get_dsp_info_by_type(unsigned int type) const {
		ERR_FAIL_COND_V(!system, Dictionary());
		ERR_FAIL_COND_V_MSG(type >= FMOD_DSP_TYPE_MAX, Dictionary(), "Invalid built-in DSP type.");

		FMOD_DSP_TYPE fmod_type = static_cast<FMOD_DSP_TYPE>(type);
		const FMOD_DSP_DESCRIPTION* desc = nullptr;
		FMOD_ERR_CHECK_V(system->getDSPInfoByType(fmod_type, &desc), Dictionary());
		ERR_FAIL_NULL_V(desc, Dictionary());

		Dictionary info;
		info["name"] = String::utf8(desc->name);
		info["version"] = desc->version;
		info["plugin_sdk_version"] = desc->pluginsdkversion;
		info["mode"] = static_cast<int>(desc->mode);
		info["input_channels"] = desc->inchannels;
		info["output_channels"] = desc->outchannels;
		info["default_cooldown"] = desc->defaultcooldown;
		info["has_create"] = (desc->create != nullptr);
		info["has_release"] = (desc->release != nullptr);
		info["has_reset"] = (desc->reset != nullptr);
		info["has_process"] = (desc->process != nullptr);
		info["has_setposition"] = (desc->setposition != nullptr);
		return info;
	}

	Ref<FmodChannelGroup> FmodSystem::get_master_channel_group() const {
		ERR_FAIL_COND_V(!system, Ref<FmodChannelGroup>());
		FMOD::ChannelGroup* channel_group_ptr = nullptr;
		FMOD_ERR_CHECK_V(system->getMasterChannelGroup(&channel_group_ptr), Ref<FmodChannelGroup>());
		ERR_FAIL_NULL_V(channel_group_ptr, Ref<FmodChannelGroup>());
		Ref<FmodChannelGroup> channel_group;
		channel_group.instantiate();
		channel_group->setup(channel_group_ptr);
		return channel_group;
	}

	Ref<FmodSoundGroup> FmodSystem::get_master_sound_group() const {
		ERR_FAIL_COND_V(!system, Ref<FmodSoundGroup>());
		FMOD::SoundGroup* sound_group_ptr = nullptr;
		FMOD_ERR_CHECK_V(system->getMasterSoundGroup(&sound_group_ptr), Ref<FmodSoundGroup>());
		ERR_FAIL_NULL_V(sound_group_ptr, Ref<FmodSoundGroup>());
		Ref<FmodSoundGroup> sound_group;
		sound_group.instantiate();
		sound_group->setup(sound_group_ptr);
		return sound_group;
	}

	Ref<FmodDSPConnection> FmodSystem::create_dsp_connection(unsigned int type) const {
		ERR_FAIL_COND_V(!system || type >= FMOD_DSPCONNECTION_TYPE_PREALLOCATED, Ref<FmodDSPConnection>());
		FMOD::DSPConnection* native = nullptr;
		FMOD_ERR_CHECK_V(system->createDSPConnection(static_cast<FMOD_DSPCONNECTION_TYPE>(type), &native), Ref<FmodDSPConnection>());
		Ref<FmodDSPConnection> connection;
		connection.instantiate();
		connection->setup(native);
		return connection;
	}

	void FmodSystem::set_3d_listener_attributes(
		const int listener,
		const Vector3 position,
		const Vector3 velocity,
		const Vector3 forward,
		const Vector3 up
	) {
		ERR_FAIL_COND(!system);
		ERR_FAIL_COND(listener < 0 || listener >= 8);										// FMOD 支持最多 8 个监听者
		FMOD_VECTOR fmod_pos = { position.x, position.y, position.z };
		FMOD_VECTOR fmod_vel = { velocity.x, velocity.y, velocity.z };
		FMOD_VECTOR fmod_fwd = { forward.x, forward.y, forward.z };
		FMOD_VECTOR fmod_up = { up.x, up.y, up.z };
		FMOD_ERR_CHECK(system->set3DListenerAttributes(listener, &fmod_pos, &fmod_vel, &fmod_fwd, &fmod_up));
	}

	Dictionary FmodSystem::get_3d_listener_attributes(const int listener) const {
		ERR_FAIL_COND_V(!system || listener < 0 || listener >= 8, Dictionary());
		FMOD_VECTOR pos, vel, fwd, up;
		FMOD_ERR_CHECK_V(system->get3DListenerAttributes(listener, &pos, &vel, &fwd, &up), Dictionary());
		Dictionary result;
		result["position"] = Vector3(pos.x, pos.y, pos.z);
		result["velocity"] = Vector3(vel.x, vel.y, vel.z);
		result["forward"] = Vector3(fwd.x, fwd.y, fwd.z);
		result["up"] = Vector3(up.x, up.y, up.z);
		return result;
	}

	void FmodSystem::set_reverb_properties(
		const int instance,
		const float decay_time,
		const float early_delay,
		const float late_delay,
		const float hf_reference,
		const float hf_decay_ratio,
		const float diffusion,
		const float density,
		const float low_shelf_frequency,
		const float low_shelf_gain,
		const float high_cut,
		const float early_late_mix,
		const float wet_level
	) {
		ERR_FAIL_COND(!system);
		const FMOD_REVERB_PROPERTIES prop = {
			decay_time,
			early_delay,
			late_delay,
			hf_reference,
			hf_decay_ratio,
			diffusion,
			density,
			low_shelf_frequency,
			low_shelf_gain,
			high_cut,
			early_late_mix,
			wet_level
		};
		FMOD_ERR_CHECK(system->setReverbProperties(instance, &prop));
	}

	Dictionary FmodSystem::get_reverb_properties(const int instance) const {
		ERR_FAIL_COND_V(!system, Dictionary());
		FMOD_REVERB_PROPERTIES prop;
		FMOD_ERR_CHECK_V(system->getReverbProperties(instance, &prop), Dictionary());
		Dictionary result;
		result["decay_time"] = prop.DecayTime;
		result["early_delay"] = prop.EarlyDelay;
		result["late_delay"] = prop.LateDelay;
		result["hf_reference"] = prop.HFReference;
		result["hf_decay_ratio"] = prop.HFDecayRatio;
		result["diffusion"] = prop.Diffusion;
		result["density"] = prop.Density;
		result["low_shelf_frequency"] = prop.LowShelfFrequency;
		result["low_shelf_gain"] = prop.LowShelfGain;
		result["high_cut"] = prop.HighCut;
		result["early_late_mix"] = prop.EarlyLateMix;
		result["wet_level"] = prop.WetLevel;
		return result;
	}

	void FmodSystem::attach_channel_group_to_port(
		Ref<FmodChannelGroup> channel_group,
		const FmodPortType prot_type,
		const uint64_t port_index,
		const bool pass_thru
	) {
		ERR_FAIL_COND(!system || channel_group.is_null() || channel_group->channel_control_is_null());
		FMOD_PORT_TYPE fmod_port_type = static_cast<FMOD_PORT_TYPE>(prot_type);
		FMOD_ERR_CHECK(system->attachChannelGroupToPort(fmod_port_type, static_cast<FMOD_PORT_INDEX>(port_index), channel_group->channel_group, pass_thru));
	}

	void FmodSystem::detach_channel_group_from_port(Ref<FmodChannelGroup> channel_group) {
		ERR_FAIL_COND(!system || channel_group.is_null() || channel_group->channel_control_is_null());
		FMOD_ERR_CHECK(system->detachChannelGroupFromPort(channel_group->channel_group));
	}

	Dictionary FmodSystem::get_record_num_drivers() const {
		ERR_FAIL_COND_V(!system, Dictionary());
		int numdrivers = 0, numconnected = 0;
		FMOD_ERR_CHECK_V(system->getRecordNumDrivers(&numdrivers, &numconnected), Dictionary());
		Dictionary result;
		result["num_drivers"] = numdrivers;
		result["num_connected"] = numconnected;
		return result;
	}

	Dictionary FmodSystem::get_record_driver_info(const int id) const {
		ERR_FAIL_COND_V(!system, Dictionary());
		ERR_FAIL_COND_V_MSG(id < 0, Dictionary(), "Record driver id must be >= 0.");

		char name[512] = { 0 };
		FMOD_GUID guid;
		int systemrate = 0;
		FMOD_SPEAKERMODE speakermode = FMOD_SPEAKERMODE_DEFAULT;
		int speakermodechannels = 0;
		FMOD_DRIVER_STATE state;
		FMOD_ERR_CHECK_V(system->getRecordDriverInfo(
			id,
			name,
			512,
			&guid,
			&systemrate,
			&speakermode,
			&speakermodechannels,
			&state
		), Dictionary());

		Dictionary info;
		info["name"] = String::utf8(name);
		info["guid"] = FmodUtils::guid_to_string(guid);
		info["system_rate"] = systemrate;
		info["speaker_mode"] = static_cast<int>(speakermode);
		info["speaker_mode_channels"] = speakermodechannels;
		info["state"] = static_cast<int>(state);
		return info;
	}

	int FmodSystem::get_record_position(const int id) const {
		ERR_FAIL_COND_V(!system, 0);
		ERR_FAIL_COND_V_MSG(id < 0, 0, "Record driver id must be >= 0.");
		unsigned int position = 0;
		FMOD_ERR_CHECK_V(system->getRecordPosition(id, &position), 0);
		return position;
	}

	void FmodSystem::record_start(const int id, Ref<FmodSound> sound, const bool loop) {
		ERR_FAIL_COND(!system || sound.is_null() || !sound->sound);
		ERR_FAIL_COND_MSG(id < 0, "Record driver id must be >= 0.");
		FMOD_ERR_CHECK(system->recordStart(id, sound->sound, loop));
	}

	void FmodSystem::record_stop(const int id) {
		ERR_FAIL_COND(!system);
		ERR_FAIL_COND_MSG(id < 0, "Record driver id must be >= 0.");
		FMOD_ERR_CHECK(system->recordStop(id));
	}

	bool FmodSystem::is_recording(const int id) const {
		ERR_FAIL_COND_V(!system, false);
		ERR_FAIL_COND_V_MSG(id < 0, false, "Record driver id must be >= 0.");
		bool recording = false;
		FMOD_ERR_CHECK_V(system->isRecording(id, &recording), false);
		return recording;
	}

	Ref<FmodGeometry> FmodSystem::create_geometry(const int max_polygons, const int max_vertices) const {
		ERR_FAIL_COND_V(!system, Ref<FmodGeometry>());
		ERR_FAIL_COND_V_MSG(max_polygons <= 0, Ref<FmodGeometry>(), "max_polygons must be greater than 0.");
		ERR_FAIL_COND_V_MSG(max_vertices <= 0, Ref<FmodGeometry>(), "max_vertices must be greater than 0.");
		FMOD::Geometry* geometry_ptr = nullptr;
		FMOD_ERR_CHECK_V(system->createGeometry(max_polygons, max_vertices, &geometry_ptr), Ref<FmodGeometry>());
		ERR_FAIL_NULL_V(geometry_ptr, Ref<FmodGeometry>());
		Ref<FmodGeometry> geometry;
		geometry.instantiate();
		geometry->setup(geometry_ptr);
		return geometry;
	}

	void FmodSystem::set_3d_max_world_size(const float max_world_size) {
		ERR_FAIL_COND(!system);
		ERR_FAIL_COND_MSG(max_world_size <= 0.0f, "max_world_size must be greater than 0.");
		FMOD_ERR_CHECK(system->setGeometrySettings(max_world_size));
	}

	float FmodSystem::get_3d_max_world_size() const {
		if (!system) return 1000.0f;
		float max_world_size = 1000.0f;
		FMOD_ERR_CHECK_V(system->getGeometrySettings(&max_world_size), 1000.0f);
		return max_world_size;
	}

	Ref<FmodGeometry> FmodSystem::load_geometry(const PackedByteArray& data) const {
		ERR_FAIL_COND_V(!system, Ref<FmodGeometry>());
		ERR_FAIL_COND_V_MSG(data.is_empty(), Ref<FmodGeometry>(), "Geometry data cannot be empty.");
		ERR_FAIL_COND_V_MSG(data.size() > static_cast<int64_t>(std::numeric_limits<int>::max()), Ref<FmodGeometry>(), "Geometry data is too large for FMOD loadGeometry.");
		FMOD::Geometry* geometry_ptr = nullptr;
		FMOD_ERR_CHECK_V(system->loadGeometry(data.ptr(), static_cast<int>(data.size()), &geometry_ptr), Ref<FmodGeometry>());
		ERR_FAIL_NULL_V(geometry_ptr, Ref<FmodGeometry>());
		Ref<FmodGeometry> geometry;
		geometry.instantiate();
		geometry->setup(geometry_ptr);
		return geometry;
	}

	Dictionary FmodSystem::get_geometry_occlusion(const Vector3 listener, const Vector3 source) const {
		ERR_FAIL_COND_V(!system, Dictionary());
		const FMOD_VECTOR fmod_listener = { listener.x, listener.y, listener.z };
		const FMOD_VECTOR fmod_source = { source.x, source.y, source.z };
		float direct = 0.0f, reverb = 0.0f;
		FMOD_ERR_CHECK_V(system->getGeometryOcclusion(&fmod_listener, &fmod_source, &direct, &reverb), Dictionary());
		Dictionary result;
		result["direct"] = direct;
		result["reverb"] = reverb;
		return result;
	}

	void FmodSystem::lock_dsp() {
		ERR_FAIL_COND(!system);
		FMOD_ERR_CHECK(system->lockDSP());
	}

	void FmodSystem::unlock_dsp() {
		ERR_FAIL_COND(!system);
		FMOD_ERR_CHECK(system->unlockDSP());
	}

	// 静态成员变量定义
	FmodSystem* FmodSystem::current_callback_system = nullptr;

	void FmodSystem::set_3d_rolloff_callback(const Callable& p_callback) {
		ERR_FAIL_COND(!system);
		_3d_rolloff_callback = p_callback;
		if (p_callback.is_valid()) {
			// 设置当前实例为回调目标
			current_callback_system = this;
			FMOD_ERR_CHECK(system->set3DRolloffCallback(GD_3D_ROLLOFF_CALLBACK));
		}
		else {
			// 如果当前实例是回调目标，则移除回调
			if (current_callback_system == this) {
				current_callback_system = nullptr;
				FMOD_ERR_CHECK(system->set3DRolloffCallback(nullptr));
			}
		}
	}

	Callable FmodSystem::get_3d_rolloff_callback() const {
		return _3d_rolloff_callback;
	}

	float FmodSystem::_handle_3d_rolloff_callback(float distance) const {
		if (_3d_rolloff_callback.is_valid() && _is_main_thread()) {
			const Variant ret = _3d_rolloff_callback.call(distance);
			if (ret.get_type() == Variant::FLOAT) {
				return ret;
			}
		}
		return 1.0f;
	}
}

float F_CALL GD_3D_ROLLOFF_CALLBACK(FMOD_CHANNELCONTROL* channel_control, float distance) {
	if (!godot::FmodSystem::current_callback_system) return 1.0f;
	return godot::FmodSystem::current_callback_system->_handle_3d_rolloff_callback(distance);
}
