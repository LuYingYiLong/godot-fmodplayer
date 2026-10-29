#ifndef FMOD_AUDIO_STREAM_PLAYER_INTERNAL_H
#define FMOD_AUDIO_STREAM_PLAYER_INTERNAL_H

#include "audio/fmod_audio_stream.h"
#include "playback/fmod_channel.h"
#include "playback/fmod_channel_group.h"

#include <godot_cpp/classes/object.hpp>

namespace godot {
	class FmodAudioStreamPlayerInternal {
	public:
		enum MixTarget {
			MIX_TARGET_STEREO,
			MIX_TARGET_SURROUND,
			MIX_TARGET_CENTER,
		};

	private:
		Ref<FmodAudioStream> stream;
		Ref<FmodChannel> channel;
		Ref<FmodChannelGroup> channel_group;
		Callable channel_ended_callback;

		FmodAudioStream::CreateMode playback_mode = FmodAudioStream::MODE_2D;
		bool playing = false;
		bool stream_paused = false;
		bool process_paused = false;
		bool autoplay = false;
		bool preload_on_set_stream = false;
		bool last_channel_created = false;
		float volume_db = 0.0f;
		float pitch_scale = 1.0f;
		MixTarget mix_target = MIX_TARGET_STEREO;
		StringName bus;

		void _apply_stream_mode();
		void _apply_mix_target();
		Ref<FmodChannelGroup> _get_bus_channel_group() const;
		void _connect_channel();
		void _disconnect_channel();
		void _clear_channel(bool p_stop);
		void _apply_paused_state();

	public:
		FmodAudioStreamPlayerInternal();
		~FmodAudioStreamPlayerInternal();

		void set_channel_ended_callback(const Callable& p_callback);
		bool handle_channel_ended();
		void set_playback_mode(FmodAudioStream::CreateMode p_mode);

		void set_stream(Ref<FmodAudioStream> p_stream);
		Ref<FmodAudioStream> get_stream() const;

		bool preload_stream();
		bool prepare_play(float p_from_position);
		void start_prepared_playback();
		void stop();
		void seek(float p_position);

		void set_playing(bool p_playing);
		bool is_playing() const;

		void set_stream_paused(bool p_paused);
		bool get_stream_paused() const;
		void set_process_paused(bool p_paused);

		float get_playback_position() const;

		void set_volume_db(float p_volume_db);
		float get_volume_db() const;

		void set_pitch_scale(float p_pitch_scale);
		float get_pitch_scale() const;

		void set_mix_target(MixTarget p_mix_target);
		MixTarget get_mix_target() const;

		void set_autoplay(bool p_enable);
		bool is_autoplay_enabled() const;

		void set_preload_on_set_stream(bool p_enable);
		bool is_preload_on_set_stream_enabled() const;

		void set_bus(const StringName& p_bus);
		StringName get_bus() const;

		bool has_channel() const;
		bool was_channel_created() const;
		Ref<FmodChannel> get_channel() const;
	};
}

#endif // !FMOD_AUDIO_STREAM_PLAYER_INTERNAL_H
