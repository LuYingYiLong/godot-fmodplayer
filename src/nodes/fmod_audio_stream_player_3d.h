#ifndef FMOD_AUDIO_STREAM_PLAYER_3D_H
#define FMOD_AUDIO_STREAM_PLAYER_3D_H

#include "audio/fmod_audio_stream.h"
#include "nodes/fmod_audio_stream_player_internal.h"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/audio_listener3d.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {
	class FmodAudioStreamPlayer3D : public Node3D {
		GDCLASS(FmodAudioStreamPlayer3D, Node3D)

	public:
		enum AttenuationModel {
			ATTENUATION_INVERSE_DISTANCE,
			ATTENUATION_INVERSE_SQUARE_DISTANCE,
			ATTENUATION_LOGARITHMIC,
			ATTENUATION_DISABLED
		};

		enum DopplerTracking {
			DOPPLER_TRACKING_DISABLED,
			DOPPLER_TRACKING_IDLE_STEP,
			DOPPLER_TRACKING_PHYSICS_STEP
		};

	private:
		FmodAudioStreamPlayerInternal* internal_player = nullptr;

		Vector3 last_position;
		Vector3 current_velocity;
		uint64_t last_process_time = 0;

		float max_distance = 10.0f;
		float unit_size = 1.0f;
		AttenuationModel attenuation_model = ATTENUATION_LOGARITHMIC;

		float attenuation_filter_cutoff_hz = 5000.0f;
		float attenuation_filter_db = 0.0f;

		bool emission_angle_enabled = false;
		float emission_angle = 45.0f;
		float emission_angle_filter_attenuation_db = -12.0f;

		DopplerTracking doppler_tracking = DOPPLER_TRACKING_DISABLED;

		uint32_t area_mask = 1;

		void _on_internal_player_finished();
		void _update_3d_attributes();
		void _update_velocity(double delta);
		void _apply_attenuation_settings();
		void _apply_distance_filter();
		void _sync_process_pause_state();

	protected:
		static void _bind_methods();
		void _notification(int p_what);
		void _validate_property(PropertyInfo& p_property) const;

	public:
		FmodAudioStreamPlayer3D();
		~FmodAudioStreamPlayer3D();

		void set_stream(Ref<FmodAudioStream> new_stream);
		Ref<FmodAudioStream> get_stream() const;

		bool preload_stream();

		void play(const float from_position = 0.0f);
		void seek(const float to_position);
		void stop();

		void set_playing(const bool playing);
		bool is_playing() const;

		void set_stream_paused(const bool paused);
		bool get_stream_paused() const;

		float get_playback_position() const;

		void set_volume_db(const float new_volume_db);
		float get_volume_db() const;

		void set_pitch_scale(const float new_pitch);
		float get_pitch_scale() const;

		void set_auto_play(const bool enable);
		bool is_autoplay_enabled() const;

		void set_preload_on_set_stream(const bool enable);
		bool is_preload_on_set_stream_enabled() const;

		void set_bus(const StringName& p_bus);
		StringName get_bus() const;
		
		void set_max_distance(const float p_distance);
		float get_max_distance() const;

		void set_unit_size(const float p_size);
		float get_unit_size() const;

		void set_attenuation_model(const AttenuationModel p_model);
		AttenuationModel get_attenuation_model() const;

		void set_emission_angle_enabled(const bool p_enabled);
		bool is_emission_angle_enabled() const;

		void set_emission_angle(const float p_angle);
		float get_emission_angle() const;

		void set_emission_angle_filter_attenuation_db(const float p_db);
		float get_emission_angle_filter_attenuation_db() const;

		void set_attenuation_filter_cutoff_hz(const float p_freq);
		float get_attenuation_filter_cutoff_hz() const;

		void set_attenuation_filter_db(const float p_db);
		float get_attenuation_filter_db() const;

		void set_doppler_tracking(const DopplerTracking p_tracking);
		DopplerTracking get_doppler_tracking() const;

		void set_area_mask(const uint32_t p_mask);
		uint32_t get_area_mask() const;
	};
}

VARIANT_ENUM_CAST(FmodAudioStreamPlayer3D::AttenuationModel)
VARIANT_ENUM_CAST(FmodAudioStreamPlayer3D::DopplerTracking)

#endif // !FMOD_AUDIO_STREAM_PLAYER_3D_H
