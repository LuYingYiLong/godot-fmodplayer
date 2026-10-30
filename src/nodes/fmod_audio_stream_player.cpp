#include "nodes/fmod_audio_stream_player.h"
#include "mixer/fmod_audio_bus.h"

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/engine.hpp>

namespace godot {
	void FmodAudioStreamPlayer::_bind_methods() {
		BIND_ENUM_CONSTANT(MIX_TARGET_STEREO);
		BIND_ENUM_CONSTANT(MIX_TARGET_SURROUND);
		BIND_ENUM_CONSTANT(MIX_TARGET_CENTER);

		ClassDB::bind_method(D_METHOD("set_stream", "stream"), &FmodAudioStreamPlayer::set_stream);
		ClassDB::bind_method(D_METHOD("get_stream"), &FmodAudioStreamPlayer::get_stream);

		ClassDB::bind_method(D_METHOD("preload_stream"), &FmodAudioStreamPlayer::preload_stream);
		ClassDB::bind_method(D_METHOD("play", "from_position"), &FmodAudioStreamPlayer::play, DEFVAL(0.0f));
		ClassDB::bind_method(D_METHOD("seek", "to_position"), &FmodAudioStreamPlayer::seek);
		ClassDB::bind_method(D_METHOD("stop"), &FmodAudioStreamPlayer::stop);

		ClassDB::bind_method(D_METHOD("set_playing", "playing"), &FmodAudioStreamPlayer::set_playing);
		ClassDB::bind_method(D_METHOD("is_playing"), &FmodAudioStreamPlayer::is_playing);

		ClassDB::bind_method(D_METHOD("set_stream_paused", "paused"), &FmodAudioStreamPlayer::set_stream_paused);
		ClassDB::bind_method(D_METHOD("get_stream_paused"), &FmodAudioStreamPlayer::get_stream_paused);

		ClassDB::bind_method(D_METHOD("get_playback_position"), &FmodAudioStreamPlayer::get_playback_position);

		ClassDB::bind_method(D_METHOD("set_volume_db", "volume_db"), &FmodAudioStreamPlayer::set_volume_db);
		ClassDB::bind_method(D_METHOD("get_volume_db"), &FmodAudioStreamPlayer::get_volume_db);

		ClassDB::bind_method(D_METHOD("set_pitch_scale", "pitch_scale"), &FmodAudioStreamPlayer::set_pitch_scale);
		ClassDB::bind_method(D_METHOD("get_pitch_scale"), &FmodAudioStreamPlayer::get_pitch_scale);

		ClassDB::bind_method(D_METHOD("set_mix_target", "mix_target"), &FmodAudioStreamPlayer::set_mix_target);
		ClassDB::bind_method(D_METHOD("get_mix_target"), &FmodAudioStreamPlayer::get_mix_target);

		ClassDB::bind_method(D_METHOD("set_auto_play", "enable"), &FmodAudioStreamPlayer::set_auto_play);
		ClassDB::bind_method(D_METHOD("is_autoplay_enabled"), &FmodAudioStreamPlayer::is_autoplay_enabled);

		ClassDB::bind_method(D_METHOD("set_preload_on_set_stream", "enable"), &FmodAudioStreamPlayer::set_preload_on_set_stream);
		ClassDB::bind_method(D_METHOD("is_preload_on_set_stream_enabled"), &FmodAudioStreamPlayer::is_preload_on_set_stream_enabled);

		ClassDB::bind_method(D_METHOD("set_bus", "bus"), &FmodAudioStreamPlayer::set_bus);
		ClassDB::bind_method(D_METHOD("get_bus"), &FmodAudioStreamPlayer::get_bus);
		
		ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "stream", PROPERTY_HINT_RESOURCE_TYPE, "FmodAudioStream"), "set_stream", "get_stream");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "playing"), "set_playing", "is_playing");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "stream_paused"), "set_stream_paused", "get_stream_paused");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volume_db", PROPERTY_HINT_RANGE, "-80,24,0.1,suffix:dB"), "set_volume_db", "get_volume_db");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "pitch_scale", PROPERTY_HINT_RANGE, "0.01,4,0.01,or_greater"), "set_pitch_scale", "get_pitch_scale");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "mix_target", PROPERTY_HINT_ENUM, "Stereo,Surround,Center"), "set_mix_target", "get_mix_target");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_play"), "set_auto_play", "is_autoplay_enabled");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "preload_on_set_stream"), "set_preload_on_set_stream", "is_preload_on_set_stream_enabled");
		ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "bus", PROPERTY_HINT_ENUM), "set_bus", "get_bus");

		ADD_SIGNAL(MethodInfo("finished"));
	}

	FmodAudioStreamPlayer::FmodAudioStreamPlayer() {
		internal_player = new FmodAudioStreamPlayerInternal;
		if (FmodServer::get_singleton()) FmodServer::get_singleton()->connect("bus_layout_changed", Callable(this, "notify_property_list_changed"));
		internal_player->set_channel_ended_callback(callable_mp(this, &FmodAudioStreamPlayer::_on_internal_player_finished));
		internal_player->set_playback_mode(FmodAudioStream::MODE_2D);
	}

	void FmodAudioStreamPlayer::_notification(int p_what) {
		switch (p_what) {
			case NOTIFICATION_READY: {
				_sync_process_pause_state();
				if (internal_player->is_autoplay_enabled() && internal_player->get_stream().is_valid() && !Engine::get_singleton()->is_editor_hint()) {
					call_deferred("play");
				}
			} break;

			case NOTIFICATION_PAUSED:
			case NOTIFICATION_UNPAUSED:
			case NOTIFICATION_DISABLED:
			case NOTIFICATION_ENABLED: {
				_sync_process_pause_state();
			} break;

			case NOTIFICATION_PREDELETE: {
				if (internal_player) {
					internal_player->stop();
					delete internal_player;
					internal_player = nullptr;
				}
			} break;
		}
	}

	void FmodAudioStreamPlayer::_sync_process_pause_state() {
		if (internal_player) {
			internal_player->set_process_paused(is_inside_tree() && !can_process());
		}
	}

	void FmodAudioStreamPlayer::_validate_property(PropertyInfo& p_property) const {
		if (p_property.name == StringName("bus")) {
			p_property.hint = PROPERTY_HINT_ENUM;
			p_property.hint_string = String(",").join(FmodServer::get_bus_names());
		}
	}

	void FmodAudioStreamPlayer::_on_internal_player_finished() {
		if (internal_player && internal_player->handle_channel_ended()) {
			emit_signal("finished");
		}
	}

	void FmodAudioStreamPlayer::set_stream(Ref<FmodAudioStream> new_stream) {
		internal_player->set_stream(new_stream);
	}

	Ref<FmodAudioStream> FmodAudioStreamPlayer::get_stream() const {
		return internal_player->get_stream();
	}

	bool FmodAudioStreamPlayer::preload_stream() {
		return internal_player->preload_stream();
	}

	void FmodAudioStreamPlayer::play(const float from_position) {
		_sync_process_pause_state();
		if (internal_player->prepare_play(from_position)) {
			internal_player->start_prepared_playback();
		}
	}

	void FmodAudioStreamPlayer::seek(const float to_position) {
		internal_player->seek(to_position);
	}

	void FmodAudioStreamPlayer::stop() {
		internal_player->stop();
	}

	void FmodAudioStreamPlayer::set_playing(const bool playing) {
		if (playing) {
			play();
		} else {
			stop();
		}
	}

	bool FmodAudioStreamPlayer::is_playing() const {
		return internal_player->is_playing();
	}

	void FmodAudioStreamPlayer::set_stream_paused(const bool paused) {
		internal_player->set_stream_paused(paused);
	}

	bool FmodAudioStreamPlayer::get_stream_paused() const {
		return internal_player->get_stream_paused();
	}

	float FmodAudioStreamPlayer::get_playback_position() const {
		return internal_player->get_playback_position();
	}

	void FmodAudioStreamPlayer::set_volume_db(const float new_volume_db) {
		internal_player->set_volume_db(new_volume_db);
	}

	float FmodAudioStreamPlayer::get_volume_db() const {
		return internal_player->get_volume_db();
	}

	void FmodAudioStreamPlayer::set_pitch_scale(const float new_pitch) {
		internal_player->set_pitch_scale(new_pitch);
	}

	float FmodAudioStreamPlayer::get_pitch_scale() const {
		return internal_player->get_pitch_scale();
	}

	void FmodAudioStreamPlayer::set_mix_target(MixTarget p_mix_target) {
		ERR_FAIL_COND(p_mix_target < MIX_TARGET_STEREO || p_mix_target > MIX_TARGET_CENTER);
		internal_player->set_mix_target(static_cast<FmodAudioStreamPlayerInternal::MixTarget>(p_mix_target));
	}

	FmodAudioStreamPlayer::MixTarget FmodAudioStreamPlayer::get_mix_target() const {
		return static_cast<MixTarget>(internal_player->get_mix_target());
	}

	void FmodAudioStreamPlayer::set_auto_play(const bool enable) {
		internal_player->set_autoplay(enable);
	}

	bool FmodAudioStreamPlayer::is_autoplay_enabled() const {
		return internal_player->is_autoplay_enabled();
	}

	void FmodAudioStreamPlayer::set_preload_on_set_stream(const bool enable) {
		internal_player->set_preload_on_set_stream(enable);
	}

	bool FmodAudioStreamPlayer::is_preload_on_set_stream_enabled() const {
		return internal_player->is_preload_on_set_stream_enabled();
	}

	void FmodAudioStreamPlayer::set_bus(const StringName& p_bus) {
		internal_player->set_bus(p_bus);
	}

	StringName FmodAudioStreamPlayer::get_bus() const {
		const StringName name = internal_player->get_bus();
		return FmodServer::get_audio_bus(name).is_valid() ? name : StringName("Master");
	}
}
