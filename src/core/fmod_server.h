#ifndef FMOD_SERVER_H
#define FMOD_SERVER_H

#include "fmod_system.h"

namespace godot {
	class FmodAudioBus;
	class FmodAudioBusLayout;
	class FmodAudioEffect;
	class FmodAudioBusRuntime;

	class FmodServer : public Object {
		GDCLASS(FmodServer, Object)
			static FmodServer* singleton;

	private:
		bool update_connected = false;
		bool audio_server_connected = false;
		int connect_retry_count = 0;

		void _build_bus_layout();
		void _register_performance_monitors();

		double _get_dsp() const;
		double _get_stream() const;
		double _get_geometry() const;
		double _get_update() const;
		double _get_convolution1() const;
		double _get_convolution2() const;
		int64_t _get_sample_bytes_read() const;
		int64_t _get_stream_bytes_read() const;
		int64_t _get_other_bytes_read() const;

		void _connect_update();
		void _update_fmod();

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		FmodServer();
		~FmodServer();

		static Ref<FmodSystem> main_system;
		static Ref<FmodAudioBusLayout> audio_bus_layout;
		static Ref<FmodAudioBusRuntime> bus_runtime;
		static bool sync_godot_buses;
		static Vector2 camera_2d_pos;
		static float camera_2d_rot;

		static FmodServer* get_singleton();

		// 获取
		static Ref<FmodSystem> get_main_system();
		static Ref<FmodChannelGroup> get_master_channel_group();
		static Ref<FmodAudioBusLayout> get_audio_bus_layout();
		static Error set_audio_bus_layout(const Ref<FmodAudioBusLayout>& layout);
		static Ref<FmodAudioBus> get_audio_bus(const StringName& name);
		static Ref<FmodAudioBus> get_audio_bus_by_id(int64_t id);
		static Ref<FmodAudioEffect> get_bus_effect_instance(int64_t bus_id, int64_t slot_id);
		static PackedStringArray get_bus_names();
		static bool is_syncing_godot_buses();
		static void flush_bus_updates();
		static Ref<FmodAudioBusLayout> create_layout_from_godot();

		static void generate_bus_layout();								// 构建总线布局
		static void reset_main_system(Ref<FmodSystem> p_system);		// 重新设置 FMOD System
	};
}

#endif // !FMOD_SERVER_H
