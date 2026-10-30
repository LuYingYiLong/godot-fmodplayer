#ifndef FMOD_AUDIO_BUS_H
#define FMOD_AUDIO_BUS_H

#include "playback/fmod_channel_group.h"
#include <godot_cpp/classes/ref_counted.hpp>

namespace godot {
	class FmodAudioEffect;

	class FmodAudioBus : public RefCounted {
		GDCLASS(FmodAudioBus, RefCounted)

	private:
		String bus_name;
		Ref<FmodChannelGroup> parent;
		Ref<FmodChannelGroup> bus;
		Vector<Ref<FmodAudioEffect>> audio_effects;

		bool solo = false;
		bool user_mute = false;												// 用户设置的静音 (独立于 solo)
		bool bypass = false;
	protected:
		static void _bind_methods();

	public:
		FmodAudioBus();
		~FmodAudioBus();

		// 总线管理
		void init_bus(const String& p_name, Ref<FmodChannelGroup> p_parent = Ref<FmodChannelGroup>());
		void set_parent(Ref<FmodChannelGroup> p_parent);
		Ref<FmodChannelGroup> get_bus() const;
		Ref<FmodChannelGroup> get_parent() const;
		String get_bus_name() const;
		void release();

		// 效果器管理
		void add_effect(Ref<FmodAudioEffect> effect, int index = 0);
		void remove_effect(int index);
		void clear_effects();
		int get_effect_count() const;
		Ref<FmodAudioEffect> get_effect(int index) const;
		void set_effect_instances(const Vector<Ref<FmodAudioEffect>>& effects);
		void set_runtime_name(const String& name) { bus_name = name; }

		void set_volume_db(const float volume_db);
		float get_volume_db() const;

		void set_solo(bool p_solo);
		bool is_solo() const;

		void set_mute(bool p_mute);
		bool is_mute() const;
		void apply_mute(bool p_any_solo);

		void set_bypass(bool p_bypass);
		bool is_bypass() const;
		void sync_bypass();
	};
}

#endif // !FMOD_AUDIO_BUS_H
