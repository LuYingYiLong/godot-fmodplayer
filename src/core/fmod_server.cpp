#include "audio/fmod_sound.h"
#include "dsp/fmod_audio_effect.h"
#include "fmod_server.h"
#include "mixer/fmod_audio_bus.h"
#include "mixer/fmod_audio_bus_layout.h"
#include "playback/fmod_channel.h"
#include "playback/fmod_channel_group.h"

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/camera2d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_script_picker.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

// 用于获取编辑器视口的辅助函数
#ifdef TOOLS_ENABLED
static godot::Viewport* _get_editor_viewport_2d() {
	godot::EditorInterface* editor = godot::EditorInterface::get_singleton();
	if (!editor) return nullptr;
	// 获取当前编辑场景的根视口
	return editor->get_editor_viewport_2d();
}

static godot::SubViewport* _get_editor_viewport_3d(int p_index = 0) {
	godot::EditorInterface* editor = godot::EditorInterface::get_singleton();
	if (!editor) return nullptr;
	return editor->get_editor_viewport_3d(p_index);
}
#endif

namespace godot {
	static bool _get_fmod_setting_bool(ProjectSettings* p_settings, const String& p_name, bool p_default_value = false) {
		if (!p_settings || !p_settings->has_setting(p_name)) {
			return p_default_value;
		}
		return p_settings->get_setting(p_name);
	}

	static int _get_fmod_setting_int(ProjectSettings* p_settings, const String& p_name, int p_default_value = 0) {
		if (!p_settings || !p_settings->has_setting(p_name)) {
			return p_default_value;
		}
		return p_settings->get_setting(p_name);
	}

	static float _get_fmod_setting_float(ProjectSettings* p_settings, const String& p_name, float p_default_value = 0.0f) {
		if (!p_settings || !p_settings->has_setting(p_name)) {
			return p_default_value;
		}
		return p_settings->get_setting(p_name);
	}

	static String _get_fmod_setting_string(ProjectSettings* p_settings, const String& p_name, const String& p_default_value = String()) {
		if (!p_settings || !p_settings->has_setting(p_name)) {
			return p_default_value;
		}
		return p_settings->get_setting(p_name);
	}

	static void _append_init_flag(ProjectSettings* p_settings, const String& p_name, FmodSystem::FmodInitFlags p_flag, unsigned int& r_flags) {
		if (_get_fmod_setting_bool(p_settings, p_name)) {
			r_flags |= static_cast<unsigned int>(p_flag);
		}
	}

	static void _apply_main_system_settings_before_init(Ref<FmodSystem> p_system, ProjectSettings* p_settings) {
		ERR_FAIL_COND(p_system.is_null() || p_system->system_is_null());

		const int output_type = _get_fmod_setting_int(p_settings, "audio/fmod/output_type", FmodSystem::FMOD_OUTPUT_TYPE_AUTODETECT);
		if (output_type != FmodSystem::FMOD_OUTPUT_TYPE_AUTODETECT) {
			p_system->set_output(static_cast<FmodSystem::FmodOutputType>(output_type));
		}

		const int driver = _get_fmod_setting_int(p_settings, "audio/fmod/driver", -1);
		if (driver >= 0) {
			p_system->set_driver(driver);
		}

		const int software_channels = _get_fmod_setting_int(p_settings, "audio/fmod/software_channels", 0);
		if (software_channels > 0) {
			p_system->set_software_channels(software_channels);
		}

		const int sample_rate = _get_fmod_setting_int(p_settings, "audio/fmod/software_sample_rate", 0);
		const int speaker_mode = _get_fmod_setting_int(p_settings, "audio/fmod/speaker_mode", FmodSystem::FMOD_SPEAKER_MODE_DEFAULT);
		const int num_raw_speakers = _get_fmod_setting_int(p_settings, "audio/fmod/num_raw_speakers", 0);
		if (sample_rate > 0 || speaker_mode != FmodSystem::FMOD_SPEAKER_MODE_DEFAULT || num_raw_speakers > 0) {
			p_system->set_software_format(sample_rate, static_cast<FmodSystem::FmodSpeakerMode>(speaker_mode), num_raw_speakers);
		}

		const int dsp_buffer_size = _get_fmod_setting_int(p_settings, "audio/fmod/dsp_buffer_size", 1024);
		const int dsp_buffer_count = _get_fmod_setting_int(p_settings, "audio/fmod/dsp_buffer_count", 4);
		if (dsp_buffer_size > 0 && dsp_buffer_count > 0) {
			p_system->set_dsp_buffer_size(dsp_buffer_size, dsp_buffer_count);
		}

		const int profile_port = _get_fmod_setting_int(p_settings, "audio/fmod/profile_port", 0);
		if (profile_port > 0) {
			p_system->set_profile_port(profile_port);
		}

		const int default_decode_buffer_size = _get_fmod_setting_int(p_settings, "audio/fmod/advanced/default_decode_buffer_size", 0);
		if (default_decode_buffer_size > 0) {
			p_system->set_default_decode_buffer_size(default_decode_buffer_size);
		}

		const int dsp_buffer_pool_size = _get_fmod_setting_int(p_settings, "audio/fmod/advanced/dsp_buffer_pool_size", 0);
		if (dsp_buffer_pool_size > 0) {
			p_system->set_dsp_buffer_pool_size(dsp_buffer_pool_size);
		}

		const int resampler_method = _get_fmod_setting_int(p_settings, "audio/fmod/advanced/resampler_method", FmodSystem::FMOD_RESAMPLER_DEFAULT);
		if (resampler_method != FmodSystem::FMOD_RESAMPLER_DEFAULT) {
			p_system->set_resampler_method(static_cast<FmodSystem::FmodResamplerMethod>(resampler_method));
		}

		const int random_seed = _get_fmod_setting_int(p_settings, "audio/fmod/advanced/random_seed", 0);
		if (random_seed > 0) {
			p_system->set_random_seed(random_seed);
		}

		const int max_convolution_threads = _get_fmod_setting_int(p_settings, "audio/fmod/advanced/max_convolution_threads", 0);
		if (max_convolution_threads > 0) {
			p_system->set_max_convolution_threads(max_convolution_threads);
		}

		const int max_spatial_objects = _get_fmod_setting_int(p_settings, "audio/fmod/advanced/max_spatial_objects", 0);
		if (max_spatial_objects > 0) {
			p_system->set_max_spatial_objects(max_spatial_objects);
		}

		const float vol0_virtual_vol = _get_fmod_setting_float(p_settings, "audio/fmod/advanced/vol0_virtual_vol", 0.0f);
		if (vol0_virtual_vol > 0.0f) {
			p_system->set_vol0_virtual_vol(vol0_virtual_vol);
		}

		const int geometry_max_fade_time = _get_fmod_setting_int(p_settings, "audio/fmod/3d/geometry_max_fade_time", 0);
		if (geometry_max_fade_time > 0) {
			p_system->set_geometry_max_fade_time(geometry_max_fade_time);
		}

		const float distance_filter_center_freq = _get_fmod_setting_float(p_settings, "audio/fmod/3d/distance_filter_center_freq", 0.0f);
		if (distance_filter_center_freq > 0.0f) {
			p_system->set_distance_filter_center_freq(distance_filter_center_freq);
		}

		const int max_mpeg_codecs = _get_fmod_setting_int(p_settings, "audio/fmod/advanced/max_mpeg_codecs", 0);
		if (max_mpeg_codecs > 0) {
			p_system->set_max_mpeg_codecs(max_mpeg_codecs);
		}

		const int max_adpcm_codecs = _get_fmod_setting_int(p_settings, "audio/fmod/advanced/max_adpcm_codecs", 0);
		if (max_adpcm_codecs > 0) {
			p_system->set_max_adpcm_codecs(max_adpcm_codecs);
		}

		const int max_xma_codecs = _get_fmod_setting_int(p_settings, "audio/fmod/advanced/max_xma_codecs", 0);
		if (max_xma_codecs > 0) {
			p_system->set_max_xma_codecs(max_xma_codecs);
		}

		const int max_vorbis_codecs = _get_fmod_setting_int(p_settings, "audio/fmod/advanced/max_vorbis_codecs", 0);
		if (max_vorbis_codecs > 0) {
			p_system->set_max_vorbis_codecs(max_vorbis_codecs);
		}

		const int max_at9_codecs = _get_fmod_setting_int(p_settings, "audio/fmod/advanced/max_at9_codecs", 0);
		if (max_at9_codecs > 0) {
			p_system->set_max_at9_codecs(max_at9_codecs);
		}

		const int max_fadpcm_codecs = _get_fmod_setting_int(p_settings, "audio/fmod/advanced/max_fadpcm_codecs", 0);
		if (max_fadpcm_codecs > 0) {
			p_system->set_max_fadpcm_codecs(max_fadpcm_codecs);
		}

		const int max_opus_codecs = _get_fmod_setting_int(p_settings, "audio/fmod/advanced/max_opus_codecs", 0);
		if (max_opus_codecs > 0) {
			p_system->set_max_opus_codecs(max_opus_codecs);
		}
	}

	static void _apply_main_system_settings_after_init(Ref<FmodSystem> p_system, ProjectSettings* p_settings) {
		ERR_FAIL_COND(p_system.is_null() || p_system->system_is_null());

		const int stream_buffer_size = _get_fmod_setting_int(p_settings, "audio/fmod/stream_buffer_size", 16384);
		if (stream_buffer_size > 0) {
			p_system->set_stream_buffer_size(stream_buffer_size);
		}

		const String network_proxy = _get_fmod_setting_string(p_settings, "audio/fmod/network_proxy");
		if (!network_proxy.is_empty()) {
			p_system->set_network_proxy(network_proxy);
		}

		const int network_timeout = _get_fmod_setting_int(p_settings, "audio/fmod/network_timeout", 0);
		if (network_timeout > 0) {
			p_system->set_network_timeout(network_timeout);
		}

		p_system->set_doppler_scale(_get_fmod_setting_float(p_settings, "audio/fmod/3d/doppler_scale", 1.0f));
		p_system->set_distance_factor(_get_fmod_setting_float(p_settings, "audio/fmod/3d/distance_factor", 1.0f));
		p_system->set_rolloff_scale(_get_fmod_setting_float(p_settings, "audio/fmod/3d/rolloff_scale", 1.0f));
		p_system->set_3d_num_listeners(CLAMP(_get_fmod_setting_int(p_settings, "audio/fmod/3d/num_listeners", 1), 1, 8));
	}

	static void _add_custom_monitor_if_missing(Performance* p_perf, const StringName& p_name, const Callable& p_callable) {
		ERR_FAIL_NULL(p_perf);

		if (!p_perf->has_custom_monitor(p_name)) {
			p_perf->add_custom_monitor(p_name, p_callable);
		}
	}

	FmodServer* FmodServer::singleton = nullptr;
	Ref<FmodSystem> FmodServer::main_system;
	Ref<FmodAudioBusLayout> FmodServer::audio_bus_layout = Ref<FmodAudioBusLayout>();
	Vector2 FmodServer::camera_2d_pos;
	float FmodServer::camera_2d_rot = 0.0f;

	void FmodServer::_bind_methods() {
		ClassDB::bind_method(D_METHOD("_connect_update"), &FmodServer::_connect_update);
		ClassDB::bind_method(D_METHOD("_update_fmod"), &FmodServer::_update_fmod);
		ClassDB::bind_method(D_METHOD("_build_bus_layout"), &FmodServer::_build_bus_layout);
		ClassDB::bind_static_method("FmodServer", D_METHOD("get_main_system"), &FmodServer::get_main_system);
		ClassDB::bind_static_method("FmodServer", D_METHOD("get_master_channel_group"), &FmodServer::get_master_channel_group);
		ClassDB::bind_static_method("FmodServer", D_METHOD("get_audio_bus_layout"), &FmodServer::get_audio_bus_layout);
		ClassDB::bind_static_method("FmodServer", D_METHOD("generate_bus_layout"), &FmodServer::generate_bus_layout);
		ClassDB::bind_static_method("FmodServer", D_METHOD("reset_main_system", "system"), &FmodServer::reset_main_system);
	}

	FmodServer::FmodServer() {
		ERR_FAIL_COND(singleton != nullptr);
		singleton = this;

		ProjectSettings* settings = ProjectSettings::get_singleton();

		const int max_channels = CLAMP(_get_fmod_setting_int(settings, "audio/fmod/max_channels", 32), 1, 4095);

		bool enable_profile = false;
		unsigned int mode_flags = FmodSystem::FMOD_INIT_FLAG_NORMAL;
		_append_init_flag(settings, "audio/fmod/init_flags/stream_from_update", FmodSystem::FMOD_INIT_FLAG_STREAM_FROM_UPDATE, mode_flags);
		_append_init_flag(settings, "audio/fmod/init_flags/mix_from_update", FmodSystem::FMOD_INIT_FLAG_MIX_FROM_UPDATE, mode_flags);
		_append_init_flag(settings, "audio/fmod/init_flags/3d_right_handed", FmodSystem::FMOD_INIT_FLAG_3D_RIGHTHANDED, mode_flags);
		_append_init_flag(settings, "audio/fmod/init_flags/clip_output", FmodSystem::FMOD_INIT_FLAG_CLIP_OUTPUT, mode_flags);
		_append_init_flag(settings, "audio/fmod/init_flags/channel_lowpass", FmodSystem::FMOD_INIT_FLAG_CHANNEL_LOWPASS, mode_flags);
		_append_init_flag(settings, "audio/fmod/init_flags/channel_distance_filter", FmodSystem::FMOD_INIT_FLAG_CHANNEL_DISTANCEFILTER, mode_flags);
		_append_init_flag(settings, "audio/fmod/init_flags/vol0_becomes_virtual", FmodSystem::FMOD_INIT_FLAG_VOL0_BECOMES_VIRTUAL, mode_flags);
		_append_init_flag(settings, "audio/fmod/init_flags/geometry_use_closest", FmodSystem::FMOD_INIT_FLAG_GEOMETRY_USECLOSEST, mode_flags);
		_append_init_flag(settings, "audio/fmod/init_flags/prefer_dolby_downmix", FmodSystem::FMOD_INIT_FLAG_PREFER_DOLBY_DOWNMIX, mode_flags);
		_append_init_flag(settings, "audio/fmod/init_flags/thread_unsafe", FmodSystem::FMOD_INIT_FLAG_THREAD_UNSAFE, mode_flags);
		_append_init_flag(settings, "audio/fmod/init_flags/profile_meter_all", FmodSystem::FMOD_INIT_FLAG_PROFILE_METER_ALL, mode_flags);
		_append_init_flag(settings, "audio/fmod/init_flags/memory_tracking", FmodSystem::FMOD_INIT_FLAG_MEMORY_TRACKING, mode_flags);
		if (Engine::get_singleton()->is_editor_hint()) {
			enable_profile = _get_fmod_setting_bool(settings, "audio/fmod/enable_profile", true);
			if (enable_profile) {
				mode_flags |= FmodSystem::FMOD_INIT_FLAG_PROFILE_ENABLE;
			}
		}

		main_system = FmodSystem::create_system();
		if (main_system.is_null() || main_system->system_is_null()) {
			UtilityFunctions::print_rich("[b][color=WHITE][bgcolor=RED]Failed to init main system![/bgcolor][/color][/b]");
			return;
		}
		_apply_main_system_settings_before_init(main_system, settings);
		main_system->init(max_channels, static_cast<FmodSystem::FmodInitFlags>(mode_flags));
		_apply_main_system_settings_after_init(main_system, settings);

		const bool show_startup_banner = _get_fmod_setting_bool(settings, "audio/fmod/show_startup_banner", true);
		if (show_startup_banner) {
			UtilityFunctions::print("    _____                    _ ____  _                       ");
			UtilityFunctions::print("   |  ___| __ ___   ___   __| |  _ \\| | __ _ _   _  ___ _ __ ");
			UtilityFunctions::print("   | |_ | '_ ` _ \\ / _ \\ / _` | |_) | |/ _` | | | |/ _ \\ '__|");
			UtilityFunctions::print("   |  _|| | | | | | (_) | (_| |  __/| | (_| | |_| |  __/ |   ");
			UtilityFunctions::print("   |_|  |_| |_| |_|\\___/ \\__,_|_|   |_|\\__,_|\\__, |\\___|_|   ");
			UtilityFunctions::print("                                             |___/           ");
			Dictionary version = main_system->get_version();
			UtilityFunctions::print_rich(
				"[b][color=BLACK][bgcolor=WHITE]Fmod Completed.\tFmod Version: ",
				version.get("version", "Unknow"),
				"\tFmod Build Number: ",
				version.get("build_number", "Unknow"),
				"[/bgcolor][/color][/b]"
			);
		}

		// 初始化总线布局
		audio_bus_layout.instantiate();
		_register_performance_monitors();
	}

	FmodServer::~FmodServer() {
		ERR_FAIL_COND(singleton != this);

		SceneTree* tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
		Callable update_callable(this, "_update_fmod");
		if (tree && tree->is_connected("process_frame", update_callable)) {
			tree->disconnect("process_frame", update_callable);
		}

		AudioServer* audio_server = AudioServer::get_singleton();
		Callable bus_layout_callable(this, "_build_bus_layout");
		if (audio_server && audio_server->is_connected("bus_layout_changed", bus_layout_callable)) {
			audio_server->disconnect("bus_layout_changed", bus_layout_callable);
		}

		singleton = nullptr;

		// 释放 FMODSystem
		// Release DSPs and bus wrappers while the FMOD system is still alive.
		if (audio_bus_layout.is_valid()) {
			audio_bus_layout->clear();
			audio_bus_layout.unref();
		}

		if (main_system.is_valid() && !main_system->system_is_null()) {
			main_system->release();
		}
		main_system.unref();

		// 注销自定义监视器
		Performance* perf = Performance::get_singleton();
		if (perf) {
			perf->remove_custom_monitor("FmodCPUUsage/DSP");
			perf->remove_custom_monitor("FmodCPUUsage/Stream");
			perf->remove_custom_monitor("FmodCPUUsage/Geometry");
			perf->remove_custom_monitor("FmodCPUUsage/Update");
			perf->remove_custom_monitor("FmodCPUUsage/Convolution1");
			perf->remove_custom_monitor("FmodCPUUsage/Convolution2");
			perf->remove_custom_monitor("FmodFileUsage/SampleBytesRead");
			perf->remove_custom_monitor("FmodFileUsage/StreamBytesRead");
			perf->remove_custom_monitor("FmodFileUsage/OtherBytesRead");
		}
	}

	void FmodServer::_notification(int p_what) {
		switch (p_what) {
		case NOTIFICATION_POSTINITIALIZE: {
			call_deferred("_connect_update");
		} break;
		}
	}

	FmodServer* FmodServer::get_singleton() {
		return singleton;
	}

	void FmodServer::_register_performance_monitors() {
		Performance* perf = Performance::get_singleton();
		ERR_FAIL_NULL(perf);

		_add_custom_monitor_if_missing(perf, "FmodCPUUsage/DSP", callable_mp(this, &FmodServer::_get_dsp));
		_add_custom_monitor_if_missing(perf, "FmodCPUUsage/Stream", callable_mp(this, &FmodServer::_get_stream));
		_add_custom_monitor_if_missing(perf, "FmodCPUUsage/Geometry", callable_mp(this, &FmodServer::_get_geometry));
		_add_custom_monitor_if_missing(perf, "FmodCPUUsage/Update", callable_mp(this, &FmodServer::_get_update));
		_add_custom_monitor_if_missing(perf, "FmodCPUUsage/Convolution1", callable_mp(this, &FmodServer::_get_convolution1));
		_add_custom_monitor_if_missing(perf, "FmodCPUUsage/Convolution2", callable_mp(this, &FmodServer::_get_convolution2));
		_add_custom_monitor_if_missing(perf, "FmodFileUsage/SampleBytesRead", callable_mp(this, &FmodServer::_get_sample_bytes_read));
		_add_custom_monitor_if_missing(perf, "FmodFileUsage/StreamBytesRead", callable_mp(this, &FmodServer::_get_stream_bytes_read));
		_add_custom_monitor_if_missing(perf, "FmodFileUsage/OtherBytesRead", callable_mp(this, &FmodServer::_get_other_bytes_read));
	}

	void FmodServer::_connect_update() {
		bool retry_needed = false;

		Performance* perf = Performance::get_singleton();
		if (perf) {
			_register_performance_monitors();
		}
		else {
			retry_needed = true;
		}

		SceneTree* tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
		if (tree) {
			if (!update_connected) {
				Callable update_callable(this, "_update_fmod");
				if (update_callable.is_valid() && !tree->is_connected("process_frame", update_callable)) {
					Error err = tree->connect("process_frame", update_callable);
					update_connected = err == OK;
					if (err != OK) {
						WARN_PRINT("FmodServer: Failed to connect SceneTree::process_frame.");
						retry_needed = true;
					}
				}
				else {
					update_connected = update_callable.is_valid();
					retry_needed = !update_connected;
				}
			}
		}
		else {
			retry_needed = true;
		}

		AudioServer* audio_server = AudioServer::get_singleton();
		if (audio_server) {
			if (!audio_server_connected) {
				Callable bus_layout_callable(this, "_build_bus_layout");
				if (bus_layout_callable.is_valid() && !audio_server->is_connected("bus_layout_changed", bus_layout_callable)) {
					Error err = audio_server->connect("bus_layout_changed", bus_layout_callable, CONNECT_DEFERRED);
					audio_server_connected = err == OK;
					if (err == OK) {
						_build_bus_layout();
					}
					else {
						WARN_PRINT("FmodServer: Failed to connect AudioServer::bus_layout_changed.");
						retry_needed = true;
					}
				}
				else {
					audio_server_connected = bus_layout_callable.is_valid();
					if (audio_server_connected) {
						_build_bus_layout();
					}
					else {
						retry_needed = true;
					}
				}
			}
		}
		else {
			retry_needed = true;
		}

		if (retry_needed) {
			if (connect_retry_count < 300) {
				connect_retry_count++;
				call_deferred("_connect_update");
			}
			else {
				WARN_PRINT("FmodServer: Some editor/runtime services were not ready; Performance monitors that were available have still been registered.");
			}
		}
		else {
			connect_retry_count = 0;
		}
	}

	void FmodServer::_update_fmod() {
		if (!singleton || main_system.is_null()) return;

		if (audio_bus_layout.is_valid()) {
			audio_bus_layout->sync_from_audio_server_if_changed();
		}

		// 更新监听者位置
		SceneTree* tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
		if (tree) {
			Viewport* viewport = tree->get_root();

			if (Engine::get_singleton()->is_editor_hint()) {
				EditorInterface* editor = EditorInterface::get_singleton();
				if (editor) {
					SubViewport* editor_viewport_3d = editor->get_editor_viewport_3d(0);
					if (editor_viewport_3d) {
						Camera3D* camera_3d = editor_viewport_3d->get_camera_3d();
						if (camera_3d) {
							Vector3 position = camera_3d->get_global_position();
							Vector3 forward = -camera_3d->get_global_transform().get_basis().get_column(2).normalized();
							Vector3 up = camera_3d->get_global_transform().get_basis().get_column(1).normalized();
							main_system->set_3d_listener_attributes(
								0,
								position,
								Vector3(0, 0, 0),
								forward,
								up
							);
						}
					}

					Viewport* editor_viewport_2d = editor->get_editor_viewport_2d();
					if (editor_viewport_2d) {
						Transform2D canvas_transform = editor_viewport_2d->get_global_canvas_transform();
						Vector2 screen_center = editor_viewport_2d->get_visible_rect().size * 0.5f;
						camera_2d_pos = canvas_transform.affine_inverse().xform(screen_center);
						camera_2d_rot = canvas_transform.get_rotation();
					}

					main_system->update();
					return;
				}
			}

			// 运行时或编辑器中未找到编辑器相机，使用游戏视口
			if (viewport) {
				Camera3D* camera_3d = viewport->get_camera_3d();
				if (camera_3d) {
					Vector3 position = camera_3d->get_global_position();
					Vector3 forward = -camera_3d->get_global_transform().get_basis().get_column(2).normalized();
					Vector3 up = camera_3d->get_global_transform().get_basis().get_column(1).normalized();
					main_system->set_3d_listener_attributes(
						0,
						position,
						Vector3(0, 0, 0),
						forward,
						up
					);
				}
				else {
					Camera2D* camera_2d = viewport->get_camera_2d();
					if (camera_2d) {
						camera_2d_pos = camera_2d->get_global_position();
						camera_2d_rot = camera_2d->get_global_rotation();
					}
				}
			}
		}

		main_system->update();
	}

	Ref<FmodSystem> FmodServer::get_main_system() {
		return main_system;
	}

	Ref<FmodChannelGroup> FmodServer::get_master_channel_group() {
		if (main_system.is_null()) {
			return Ref<FmodChannelGroup>();
		}
		return main_system->get_master_channel_group();
	}

	Ref<FmodAudioBusLayout> FmodServer::get_audio_bus_layout() {
		return audio_bus_layout;
	}

	void FmodServer::generate_bus_layout() {
		audio_bus_layout->sync_from_audio_server();
	}

	void FmodServer::reset_main_system(Ref<FmodSystem> p_system) {
		ERR_FAIL_COND_MSG(p_system.is_null() || p_system->system_is_null(), "New system is null");
		if (p_system == main_system) {
			return;
		}
		// Bus groups and DSPs belong to the old system and must be released first.
		if (audio_bus_layout.is_valid()) {
			audio_bus_layout->clear();
		}
		if (main_system.is_valid() && !main_system->system_is_null()) {
			main_system->release();
		}
		main_system = p_system;
		if (audio_bus_layout.is_valid()) {
			audio_bus_layout->sync_from_audio_server();
		}
	}

	void FmodServer::_build_bus_layout() {
		if (audio_bus_layout.is_valid()) {
			audio_bus_layout->sync_from_audio_server();
		}
	}

	double FmodServer::_get_dsp() const {
		if (!singleton || main_system.is_null()) return 0.0;
		return main_system->get_cpu_usage().get("dsp", 0.0);
	}

	double FmodServer::_get_stream() const {
		if (!singleton || main_system.is_null()) return 0.0;
		return main_system->get_cpu_usage().get("stream", 0.0);
	}

	double FmodServer::_get_geometry() const {
		if (!singleton || main_system.is_null()) return 0.0;
		return main_system->get_cpu_usage().get("geometry", 0.0);
	}

	double FmodServer::_get_update() const {
		if (!singleton || main_system.is_null()) return 0.0;
		return main_system->get_cpu_usage().get("update", 0.0);
	}

	double FmodServer::_get_convolution1() const {
		if (!singleton || main_system.is_null()) return 0.0;
		return main_system->get_cpu_usage().get("convolution1", 0.0);
	}

	double FmodServer::_get_convolution2() const {
		if (!singleton || main_system.is_null()) return 0.0;
		return main_system->get_cpu_usage().get("convolution2", 0.0);
	}

	int64_t FmodServer::_get_sample_bytes_read() const {
		if (!singleton || main_system.is_null()) return 0;
		return main_system->get_file_usage().get("sample_bytes_read", 0);
	}

	int64_t FmodServer::_get_stream_bytes_read() const {
		if (!singleton || main_system.is_null()) return 0;
		return main_system->get_file_usage().get("stream_bytes_read", 0);
	}

	int64_t FmodServer::_get_other_bytes_read() const {
		if (!singleton || main_system.is_null()) return 0;
		return main_system->get_file_usage().get("other_bytes_read", 0);
	}
}
