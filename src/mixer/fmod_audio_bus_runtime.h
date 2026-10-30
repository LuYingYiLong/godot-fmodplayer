#ifndef FMOD_AUDIO_BUS_RUNTIME_H
#define FMOD_AUDIO_BUS_RUNTIME_H
#include "mixer/fmod_audio_bus.h"
#include "mixer/fmod_audio_bus_layout.h"
#include <godot_cpp/classes/audio_effect.hpp>
#include <map>

namespace godot {
class FmodAudioBusRuntime : public RefCounted {
	GDCLASS(FmodAudioBusRuntime, RefCounted)
	struct Slot {
		int64_t bus_id = 0;
		Ref<FmodAudioEffect> source, instance;
		bool dirty = false;
	};
	Ref<FmodAudioBusLayout> layout;
	std::map<int64_t, Ref<FmodAudioBus>> buses;
	std::map<int64_t, Slot> slots;
	std::map<uint64_t, std::pair<Ref<FmodAudioEffect>, Callable>> effect_observers;
	std::map<int64_t, int64_t> applied_sends;
	Array applied_state;
	bool dirty = true;
	bool source_dirty = true;
	int bus_cursor = 0, effect_cursor = 0;
	struct NativeEffect {
		Ref<AudioEffect> source;
		int64_t bus_id = 0, slot_id = 0;
		int index = 0;
		bool enabled = false;
		Callable observer;
	};
	std::vector<NativeEffect> native_effects;
	Array native_state;
	void _changed();
	void _effect_changed(int64_t slot);
	void _native_changed();
	void _clear_observers();
	void _clear_native_observers();
	Error _apply();

protected:
	static void _bind_methods();

public:
	~FmodAudioBusRuntime();
	Error initialize(const Ref<FmodAudioBusLayout> &data);
	void process(bool sync);
	void clear();
	Ref<FmodAudioBus> get_bus(int64_t id) const;
	Ref<FmodAudioEffect> get_effect(int64_t bus_id, int64_t slot_id) const;
	Ref<FmodAudioBusLayout> import_godot(PackedStringArray *skipped = nullptr, bool observe = false);
	bool godot_changed();
	void move_channels_to(FmodAudioBusRuntime *replacement);
};
} //namespace godot
#endif
