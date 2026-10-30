#include "nodes/fmod_audio_stream_player_2d.h"

#include "core/fmod_utils.hpp"

#include <godot_cpp/classes/audio_server.hpp>

namespace godot {
	void FmodAudioStreamPlayer2D::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_stream", "stream"), &FmodAudioStreamPlayer2D::set_stream);
		ClassDB::bind_method(D_METHOD("get_stream"), &FmodAudioStreamPlayer2D::get_stream);

		ClassDB::bind_method(D_METHOD("preload_stream"), &FmodAudioStreamPlayer2D::preload_stream);
		ClassDB::bind_method(D_METHOD("play", "from_position"), &FmodAudioStreamPlayer2D::play, DEFVAL(0.0));
		ClassDB::bind_method(D_METHOD("seek", "to_position"), &FmodAudioStreamPlayer2D::seek);
		ClassDB::bind_method(D_METHOD("stop"), &FmodAudioStreamPlayer2D::stop);

		ClassDB::bind_method(D_METHOD("set_playing", "playing"), &FmodAudioStreamPlayer2D::set_playing);
		ClassDB::bind_method(D_METHOD("is_playing"), &FmodAudioStreamPlayer2D::is_playing);

		ClassDB::bind_method(D_METHOD("set_stream_paused", "paused"), &FmodAudioStreamPlayer2D::set_stream_paused);
		ClassDB::bind_method(D_METHOD("get_stream_paused"), &FmodAudioStreamPlayer2D::get_stream_paused);

		ClassDB::bind_method(D_METHOD("get_playback_position"), &FmodAudioStreamPlayer2D::get_playback_position);

		ClassDB::bind_method(D_METHOD("set_volume_db", "volume_db"), &FmodAudioStreamPlayer2D::set_volume_db);
		ClassDB::bind_method(D_METHOD("get_volume_db"), &FmodAudioStreamPlayer2D::get_volume_db);

		ClassDB::bind_method(D_METHOD("set_volume_linear", "volume_linear"), &FmodAudioStreamPlayer2D::set_volume_linear);
		ClassDB::bind_method(D_METHOD("get_volume_linear"), &FmodAudioStreamPlayer2D::get_volume_linear);

		ClassDB::bind_method(D_METHOD("set_pitch_scale", "pitch_scale"), &FmodAudioStreamPlayer2D::set_pitch_scale);
		ClassDB::bind_method(D_METHOD("get_pitch_scale"), &FmodAudioStreamPlayer2D::get_pitch_scale);

		ClassDB::bind_method(D_METHOD("set_autoplay", "enable"), &FmodAudioStreamPlayer2D::set_autoplay);
		ClassDB::bind_method(D_METHOD("is_autoplay_enabled"), &FmodAudioStreamPlayer2D::is_autoplay_enabled);

		ClassDB::bind_method(D_METHOD("set_preload_on_set_stream", "enable"), &FmodAudioStreamPlayer2D::set_preload_on_set_stream);
		ClassDB::bind_method(D_METHOD("is_preload_on_set_stream_enabled"), &FmodAudioStreamPlayer2D::is_preload_on_set_stream_enabled);

		ClassDB::bind_method(D_METHOD("set_max_distance", "pixels"), &FmodAudioStreamPlayer2D::set_max_distance);
		ClassDB::bind_method(D_METHOD("get_max_distance"), &FmodAudioStreamPlayer2D::get_max_distance);

		ClassDB::bind_method(D_METHOD("set_attenuation", "curve"), &FmodAudioStreamPlayer2D::set_attenuation);
		ClassDB::bind_method(D_METHOD("get_attenuation"), &FmodAudioStreamPlayer2D::get_attenuation);

		ClassDB::bind_method(D_METHOD("set_panning_strength", "panning_strength"), &FmodAudioStreamPlayer2D::set_panning_strength);
		ClassDB::bind_method(D_METHOD("get_panning_strength"), &FmodAudioStreamPlayer2D::get_panning_strength);

		ClassDB::bind_method(D_METHOD("set_bus", "bus"), &FmodAudioStreamPlayer2D::set_bus);
		ClassDB::bind_method(D_METHOD("get_bus"), &FmodAudioStreamPlayer2D::get_bus);

		ClassDB::bind_method(D_METHOD("set_area_mask", "mask"), &FmodAudioStreamPlayer2D::set_area_mask);
		ClassDB::bind_method(D_METHOD("get_area_mask"), &FmodAudioStreamPlayer2D::get_area_mask);

		ClassDB::bind_method(D_METHOD("set_max_polyphony", "max_polyphony"), &FmodAudioStreamPlayer2D::set_max_polyphony);
		ClassDB::bind_method(D_METHOD("get_max_polyphony"), &FmodAudioStreamPlayer2D::get_max_polyphony);

		ClassDB::bind_method(D_METHOD("has_stream_playback"), &FmodAudioStreamPlayer2D::has_stream_playback);
		ClassDB::bind_method(D_METHOD("get_stream_playback"), &FmodAudioStreamPlayer2D::get_stream_playback);
		
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "stream", PROPERTY_HINT_RESOURCE_TYPE, "FmodAudioStream"), "set_stream", "get_stream");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "playing"), "set_playing", "is_playing");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "stream_paused"), "set_stream_paused", "get_stream_paused");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volume_db", PROPERTY_HINT_RANGE, "-80,24,suffix:dB"), "set_volume_db", "get_volume_db");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pitch_scale", PROPERTY_HINT_RANGE, "0.01,4,0.01,or_greater"), "set_pitch_scale", "get_pitch_scale");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoplay"), "set_autoplay", "is_autoplay_enabled");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "preload_on_set_stream"), "set_preload_on_set_stream", "is_preload_on_set_stream_enabled");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_distance", PROPERTY_HINT_RANGE, "1,4096,1,or_greater,exp,suffix:px"), "set_max_distance", "get_max_distance");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "attenuation", PROPERTY_HINT_EXP_EASING, "attenuation"), "set_attenuation", "get_attenuation");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "panning_strength", PROPERTY_HINT_RANGE, "0,3,0.01,or_greater"), "set_panning_strength", "get_panning_strength");
		ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "bus", PROPERTY_HINT_ENUM), "set_bus", "get_bus");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "area_mask", PROPERTY_HINT_LAYERS_2D_PHYSICS), "set_area_mask", "get_area_mask");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "max_polyphony"), "set_max_polyphony", "get_max_polyphony");

		ADD_SIGNAL(MethodInfo("finished"));
	}

	FmodAudioStreamPlayer2D::FmodAudioStreamPlayer2D() {
		internal_player = new FmodAudioStreamPlayerInternal;
		if (FmodServer::get_singleton()) FmodServer::get_singleton()->connect("bus_layout_changed", Callable(this, "notify_property_list_changed"));
		internal_player->set_channel_ended_callback(callable_mp(this, &FmodAudioStreamPlayer2D::_on_internal_player_finished));
		internal_player->set_playback_mode(FmodAudioStream::MODE_2D);
		set_physics_process(true);
	}

	FmodAudioStreamPlayer2D::~FmodAudioStreamPlayer2D() {
		if (internal_player) {
			internal_player->stop();
			delete internal_player;
			internal_player = nullptr;
		}
	}

	void FmodAudioStreamPlayer2D::_notification(int p_what) {
		switch (p_what) {
			case NOTIFICATION_READY: {
				_sync_process_pause_state();
				if (internal_player->is_autoplay_enabled() && internal_player->get_stream().is_valid() && !Engine::get_singleton()->is_editor_hint()) {
					call_deferred("play");
				}
				last_position = get_global_position();
			} break;

			case NOTIFICATION_PAUSED:
			case NOTIFICATION_UNPAUSED:
			case NOTIFICATION_DISABLED:
			case NOTIFICATION_ENABLED: {
				_sync_process_pause_state();
			} break;

			case NOTIFICATION_PHYSICS_PROCESS: {
				if (internal_player->is_playing()) {
					_update_panning();
				}
			} break;
		}
	}

	void FmodAudioStreamPlayer2D::_sync_process_pause_state() {
		if (internal_player) {
			internal_player->set_process_paused(is_inside_tree() && !can_process());
		}
	}

	void FmodAudioStreamPlayer2D::_validate_property(PropertyInfo& p_property) const {
		if (p_property.name == StringName("bus")) {
			p_property.hint = PROPERTY_HINT_ENUM;
			p_property.hint_string = String(",").join(FmodServer::get_bus_names());
		}
	}

	void FmodAudioStreamPlayer2D::_on_internal_player_finished() {
		if (internal_player && internal_player->handle_channel_ended()) {
			emit_signal("finished");
		}
	}

	void FmodAudioStreamPlayer2D::_update_panning() {
		Ref<FmodChannel> channel = internal_player->get_channel();
		if (channel.is_null() || !channel->channel_control_is_valid() || !is_inside_tree()) {
			return;
		}

		Vector2 global_pos = get_global_position();
		last_position = global_pos;

		Vector2 listener_pos = FmodServer::camera_2d_pos;
		float listener_rotation = FmodServer::camera_2d_rot;

		Viewport* viewport = get_viewport();
		Camera2D* camera = nullptr;
		if (viewport && listener_pos == Vector2() && listener_rotation == 0.0f) {
			camera = viewport->get_camera_2d();
			if (camera) {
				listener_pos = camera->get_global_position();
				listener_rotation = camera->get_global_rotation();
			}
		}

		float dist = global_pos.distance_to(listener_pos);
		if (dist > max_distance) {
			channel->set_volume_db(-80.0f);
			return;
		}

		float attenuation_mult = Math::pow(1.0f - dist / max_distance, attenuation);
		Vector2 relative_pos = (global_pos - listener_pos).rotated(-listener_rotation);

		float pan = 0.0f;
		if (viewport) {
			Size2 screen_size = viewport->get_visible_rect().size;
			pan = relative_pos.x / screen_size.x;
			pan = CLAMP(pan, -1.0f, 1.0f);
			pan *= panning_strength;
			pan = CLAMP(pan + 0.5f, 0.0f, 1.0f);
		}

		float total_volume_db = internal_player->get_volume_db();
		if (attenuation_mult > 0.0f) {
			total_volume_db += FmodUtils::linear_to_db(attenuation_mult);
		} else {
			total_volume_db = -80.0f;
		}
		channel->set_volume_db(total_volume_db);
		channel->set_pan(pan * 2.0f - 1.0f);
	}

	void FmodAudioStreamPlayer2D::set_stream(Ref<FmodAudioStream> new_stream) {
		internal_player->set_stream(new_stream);
	}

	Ref<FmodAudioStream> FmodAudioStreamPlayer2D::get_stream() const {
		return internal_player->get_stream();
	}

	bool FmodAudioStreamPlayer2D::preload_stream() {
		return internal_player->preload_stream();
	}

	void FmodAudioStreamPlayer2D::play(const float from_position) {
		_sync_process_pause_state();
		if (internal_player->prepare_play(from_position)) {
			Ref<FmodChannel> channel = internal_player->get_channel();
			if (channel.is_valid()) {
				channel->set_mode(FmodSystem::FMOD_MODE_2D);
			}
			_update_panning();
			internal_player->start_prepared_playback();
		}
	}

	void FmodAudioStreamPlayer2D::seek(const float to_position) {
		internal_player->seek(to_position);
	}

	void FmodAudioStreamPlayer2D::stop() {
		internal_player->stop();
	}

	void FmodAudioStreamPlayer2D::set_playing(const bool p_playing) {
		if (p_playing) {
			play();
		} else {
			stop();
		}
	}

	bool FmodAudioStreamPlayer2D::is_playing() const {
		return internal_player->is_playing();
	}

	void FmodAudioStreamPlayer2D::set_stream_paused(const bool paused) {
		internal_player->set_stream_paused(paused);
	}

	bool FmodAudioStreamPlayer2D::get_stream_paused() const {
		return internal_player->get_stream_paused();
	}

	float FmodAudioStreamPlayer2D::get_playback_position() const {
		return internal_player->get_playback_position();
	}

	void FmodAudioStreamPlayer2D::set_volume_db(const float new_volume_db) {
		internal_player->set_volume_db(new_volume_db);
	}

	float FmodAudioStreamPlayer2D::get_volume_db() const {
		return internal_player->get_volume_db();
	}

	void FmodAudioStreamPlayer2D::set_volume_linear(const float p_volume) {
		set_volume_db(FmodUtils::linear_to_db(p_volume));
	}

	float FmodAudioStreamPlayer2D::get_volume_linear() const {
		return FmodUtils::db_to_linear(get_volume_db());
	}

	void FmodAudioStreamPlayer2D::set_pitch_scale(const float new_pitch) {
		internal_player->set_pitch_scale(new_pitch);
	}

	float FmodAudioStreamPlayer2D::get_pitch_scale() const {
		return internal_player->get_pitch_scale();
	}

	void FmodAudioStreamPlayer2D::set_autoplay(const bool enable) {
		internal_player->set_autoplay(enable);
	}

	bool FmodAudioStreamPlayer2D::is_autoplay_enabled() const {
		return internal_player->is_autoplay_enabled();
	}

	void FmodAudioStreamPlayer2D::set_preload_on_set_stream(const bool enable) {
		internal_player->set_preload_on_set_stream(enable);
	}

	bool FmodAudioStreamPlayer2D::is_preload_on_set_stream_enabled() const {
		return internal_player->is_preload_on_set_stream_enabled();
	}

	void FmodAudioStreamPlayer2D::set_max_distance(float p_pixels) {
		max_distance = Math::max(p_pixels, 1.0f);
	}

	float FmodAudioStreamPlayer2D::get_max_distance() const {
		return max_distance;
	}

	void FmodAudioStreamPlayer2D::set_attenuation(float p_curve) {
		attenuation = p_curve;
	}

	float FmodAudioStreamPlayer2D::get_attenuation() const {
		return attenuation;
	}

	void FmodAudioStreamPlayer2D::set_panning_strength(float p_panning_strength) {
		panning_strength = Math::max(p_panning_strength, 0.0f);
	}

	float FmodAudioStreamPlayer2D::get_panning_strength() const {
		return panning_strength;
	}

	void FmodAudioStreamPlayer2D::set_bus(const StringName& p_bus) {
		internal_player->set_bus(p_bus);
	}

	StringName FmodAudioStreamPlayer2D::get_bus() const {
		const StringName name = internal_player->get_bus();
		return FmodServer::get_bus_names().has(name) ? name : StringName("Master");
	}

	void FmodAudioStreamPlayer2D::set_area_mask(uint32_t p_mask) {
		area_mask = p_mask;
	}

	uint32_t FmodAudioStreamPlayer2D::get_area_mask() const {
		return area_mask;
	}

	void FmodAudioStreamPlayer2D::set_max_polyphony(int p_max_polyphony) {
		max_polyphony = Math::max(p_max_polyphony, 1);
	}

	int FmodAudioStreamPlayer2D::get_max_polyphony() const {
		return max_polyphony;
	}

	bool FmodAudioStreamPlayer2D::has_stream_playback() const {
		return internal_player->has_channel();
	}

	Ref<FmodChannel> FmodAudioStreamPlayer2D::get_stream_playback() const {
		return internal_player->get_channel();
	}
}
