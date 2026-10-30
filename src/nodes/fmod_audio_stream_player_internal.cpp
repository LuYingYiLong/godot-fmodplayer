#include "nodes/fmod_audio_stream_player_internal.h"

#include "core/fmod_server.h"
#include "mixer/fmod_audio_bus_layout.h"
#include "mixer/fmod_audio_bus.h"
#include <godot_cpp/classes/audio_server.hpp>

namespace godot {
	FmodAudioStreamPlayerInternal::FmodAudioStreamPlayerInternal() {
		if (FmodServer::get_singleton()) {
			Ref<FmodSystem> system = FmodServer::get_main_system();
			if (system.is_valid()) {
				channel_group = system->get_master_channel_group();
			}
		}
	}

	FmodAudioStreamPlayerInternal::~FmodAudioStreamPlayerInternal() {
		_clear_channel(true);
		channel_group.unref();
	}

	void FmodAudioStreamPlayerInternal::_apply_stream_mode() {
		if (stream.is_null()) {
			return;
		}

		int flags = stream->get_mode_flags();
		int new_flags = flags;
		if (playback_mode == FmodAudioStream::MODE_3D) {
			new_flags |= FmodAudioStream::MODE_3D;
			new_flags &= ~FmodAudioStream::MODE_2D;
		} else {
			new_flags |= FmodAudioStream::MODE_2D;
			new_flags &= ~FmodAudioStream::MODE_3D;
		}

		if (new_flags != flags) {
			stream->set_mode_flags(new_flags);
		}
	}

	void FmodAudioStreamPlayerInternal::_apply_mix_target() {
		if (channel.is_null() || !channel->channel_control_is_valid()) {
			return;
		}

		AudioServer* audio_server = AudioServer::get_singleton();
		if (audio_server && audio_server->get_speaker_mode() == AudioServer::SPEAKER_MODE_STEREO) {
			channel->set_mix_levels_output(1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
			return;
		}

		switch (mix_target) {
			case MIX_TARGET_SURROUND:
				channel->set_mix_levels_output(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
				break;
			case MIX_TARGET_CENTER:
				channel->set_mix_levels_output(0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
				break;
			case MIX_TARGET_STEREO:
			default:
				channel->set_mix_levels_output(1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
				break;
		}
	}

	Ref<FmodChannelGroup> FmodAudioStreamPlayerInternal::_get_bus_channel_group() const {
		Ref<FmodAudioBusLayout> layout = FmodServer::get_audio_bus_layout();
		if (layout.is_valid()) {
			Ref<FmodAudioBus> audio_bus = FmodServer::get_audio_bus(get_bus());
			if (audio_bus.is_valid()) {
				Ref<FmodChannelGroup> bus_group = audio_bus->get_bus();
				if (bus_group.is_valid()) {
					return bus_group;
				}
			}
		}

		return FmodServer::get_master_channel_group();
	}

	void FmodAudioStreamPlayerInternal::_connect_channel() {
		if (channel.is_valid() && channel_ended_callback.is_valid()) {
			if (!channel->is_connected("ended", channel_ended_callback)) {
				channel->connect("ended", channel_ended_callback, Object::CONNECT_DEFERRED);
			}
		}
	}

	void FmodAudioStreamPlayerInternal::_disconnect_channel() {
		if (channel.is_valid() && channel_ended_callback.is_valid()) {
			if (channel->is_connected("ended", channel_ended_callback)) {
				channel->disconnect("ended", channel_ended_callback);
			}
		}
	}

	void FmodAudioStreamPlayerInternal::_clear_channel(bool p_stop) {
		if (channel.is_valid()) {
			_disconnect_channel();
			if (p_stop && channel->channel_control_is_valid()) {
				channel->stop();
			}
			channel.unref();
		}
	}

	void FmodAudioStreamPlayerInternal::_apply_paused_state() {
		if (channel.is_valid() && channel->channel_control_is_valid()) {
			channel->set_paused(stream_paused || process_paused);
		}
	}

	bool FmodAudioStreamPlayerInternal::handle_channel_ended() {
		_disconnect_channel();
		channel.unref();

		if (playing) {
			playing = false;
			stream_paused = false;
			return true;
		}

		return false;
	}

	void FmodAudioStreamPlayerInternal::set_channel_ended_callback(const Callable &p_callback) {
		channel_ended_callback = p_callback;
	}

	void FmodAudioStreamPlayerInternal::set_playback_mode(FmodAudioStream::CreateMode p_mode) {
		playback_mode = p_mode;
		if (stream.is_valid()) {
			_apply_stream_mode();
		}
	}

	void FmodAudioStreamPlayerInternal::set_stream(Ref<FmodAudioStream> p_stream) {
		stop();
		stream = p_stream;

		if (preload_on_set_stream) {
			preload_stream();
		}
	}

	Ref<FmodAudioStream> FmodAudioStreamPlayerInternal::get_stream() const {
		return stream;
	}

	bool FmodAudioStreamPlayerInternal::preload_stream() {
		if (stream.is_null()) {
			return false;
		}

		_apply_stream_mode();
		return stream->preload();
	}

	bool FmodAudioStreamPlayerInternal::prepare_play(float p_from_position) {
		last_channel_created = false;

		channel_group = _get_bus_channel_group();
		if (channel_group.is_null()) {
			ERR_PRINT(vformat("Cannot get channel group for bus: %s", get_bus()));
			return false;
		}

		if (channel.is_valid() && channel->is_playing()) {
			channel->set_paused(true);
			channel->set_position(int(p_from_position * 1000.0f));
			playing = true;
			stream_paused = false;
			_apply_paused_state();
			return true;
		}

		_clear_channel(false);

		if (stream.is_null()) {
			return false;
		}

		_apply_stream_mode();

		Ref<FmodSound> sound = stream->get_sound();
		if (sound.is_null()) {
			ERR_PRINT("Failed to get sound from stream");
			return false;
		}

		Ref<FmodSystem> system = FmodServer::get_main_system();
		if (system.is_null()) {
			ERR_PRINT("FMOD system not available");
			return false;
		}

		channel = system->play_sound(sound, channel_group, true);
		if (channel.is_null()) {
			ERR_PRINT("Failed to create FMOD channel");
			return false;
		}

		last_channel_created = true;
		_connect_channel();

		channel->set_volume_db(volume_db);
		channel->set_pitch(pitch_scale);
		_apply_mix_target();
		if (p_from_position > 0.0f) {
			channel->set_position(int(p_from_position * 1000.0f));
		}

		playing = true;
		stream_paused = false;
		return true;
	}

	void FmodAudioStreamPlayerInternal::start_prepared_playback() {
		_apply_paused_state();
	}

	void FmodAudioStreamPlayerInternal::stop() {
		playing = false;
		stream_paused = false;
		_clear_channel(true);
	}

	void FmodAudioStreamPlayerInternal::seek(float p_position) {
		if (channel.is_valid() && channel->channel_control_is_valid()) {
			channel->set_position(int(p_position * 1000.0f));
		}
	}

	void FmodAudioStreamPlayerInternal::set_playing(bool p_playing) {
		if (p_playing) {
			prepare_play(0.0f);
			start_prepared_playback();
		} else {
			stop();
		}
	}

	bool FmodAudioStreamPlayerInternal::is_playing() const {
		if (channel.is_null()) {
			return false;
		}
		return playing && channel->is_playing();
	}

	void FmodAudioStreamPlayerInternal::set_stream_paused(bool p_paused) {
		stream_paused = p_paused;
		_apply_paused_state();
	}

	bool FmodAudioStreamPlayerInternal::get_stream_paused() const {
		return stream_paused;
	}

	void FmodAudioStreamPlayerInternal::set_process_paused(bool p_paused) {
		process_paused = p_paused;
		_apply_paused_state();
	}

	float FmodAudioStreamPlayerInternal::get_playback_position() const {
		if (channel.is_null() || !channel->channel_control_is_valid()) {
			return 0.0f;
		}
		return channel->get_position() / 1000.0f;
	}

	void FmodAudioStreamPlayerInternal::set_volume_db(float p_volume_db) {
		volume_db = p_volume_db;
		if (channel.is_valid() && channel->channel_control_is_valid()) {
			channel->set_volume_db(volume_db);
		}
	}

	float FmodAudioStreamPlayerInternal::get_volume_db() const {
		return volume_db;
	}

	void FmodAudioStreamPlayerInternal::set_pitch_scale(float p_pitch_scale) {
		pitch_scale = p_pitch_scale;
		if (channel.is_valid() && channel->channel_control_is_valid()) {
			channel->set_pitch(pitch_scale);
		}
	}

	float FmodAudioStreamPlayerInternal::get_pitch_scale() const {
		return pitch_scale;
	}

	void FmodAudioStreamPlayerInternal::set_mix_target(MixTarget p_mix_target) {
		ERR_FAIL_COND(p_mix_target < MIX_TARGET_STEREO || p_mix_target > MIX_TARGET_CENTER);
		mix_target = p_mix_target;
		_apply_mix_target();
	}

	FmodAudioStreamPlayerInternal::MixTarget FmodAudioStreamPlayerInternal::get_mix_target() const {
		return mix_target;
	}

	void FmodAudioStreamPlayerInternal::set_autoplay(bool p_enable) {
		autoplay = p_enable;
	}

	bool FmodAudioStreamPlayerInternal::is_autoplay_enabled() const {
		return autoplay;
	}

	void FmodAudioStreamPlayerInternal::set_preload_on_set_stream(bool p_enable) {
		preload_on_set_stream = p_enable;
		if (preload_on_set_stream) {
			preload_stream();
		}
	}

	bool FmodAudioStreamPlayerInternal::is_preload_on_set_stream_enabled() const {
		return preload_on_set_stream;
	}

	void FmodAudioStreamPlayerInternal::set_bus(const StringName &p_bus) {
		bus = p_bus;
		if (channel.is_valid() && channel->channel_control_is_valid()) channel->set_channel_group(_get_bus_channel_group());
	}

	StringName FmodAudioStreamPlayerInternal::get_bus() const {
		if (bus == StringName()) {
			return StringName("Master");
		}
		return bus;
	}

	bool FmodAudioStreamPlayerInternal::has_channel() const {
		return channel.is_valid();
	}

	bool FmodAudioStreamPlayerInternal::was_channel_created() const {
		return last_channel_created;
	}

	Ref<FmodChannel> FmodAudioStreamPlayerInternal::get_channel() const {
		return channel;
	}
}
