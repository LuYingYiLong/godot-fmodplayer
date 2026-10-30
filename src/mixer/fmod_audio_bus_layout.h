#ifndef FMOD_AUDIO_BUS_LAYOUT_H
#define FMOD_AUDIO_BUS_LAYOUT_H
#include "dsp/fmod_audio_effect.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <vector>

namespace godot {
// Authoring data only: loading this resource never creates FMOD objects.
class FmodAudioBusLayout : public Resource {
	GDCLASS(FmodAudioBusLayout, Resource)
public:
	struct EffectSlot {
		int64_t id = 0;
		Ref<FmodAudioEffect> effect;
		bool enabled = true;
	};
	struct Bus {
		int64_t id = 0;
		StringName name;
		int64_t send_id = 0;
		float volume_db = 0.0f;
		bool solo = false, mute = false, bypass = false;
		std::vector<EffectSlot> effects;
	};

private:
	std::vector<Bus> buses;
	int layout_version = 1;
	int64_t next_bus_id = 1, next_slot_id = 1;
	void _touch();

protected:
	static void _bind_methods();
	bool _set(const StringName &, const Variant &);
	bool _get(const StringName &, Variant &) const;
	void _get_property_list(List<PropertyInfo> *) const;

public:
	FmodAudioBusLayout();
	const std::vector<Bus> &get_buses() const { return buses; }
	const Bus *find_bus(int64_t id) const;
	int get_bus_count() const { return int(buses.size()); }
	PackedInt64Array get_bus_ids() const;
	int64_t find_bus_id(const StringName &name) const;
	StringName get_bus_name(int64_t id) const;
	int64_t add_bus(const StringName &name, int64_t send_id = 0);
	Error remove_bus(int64_t id);
	Error move_bus(int64_t id, int index);
	Error rename_bus(int64_t id, const StringName &name);
	bool can_send_to(int64_t id, int64_t target) const;
	Error set_bus_send(int64_t id, int64_t target);
	int64_t get_bus_send(int64_t id) const;
	void set_bus_volume_db(int64_t id, float volume);
	float get_bus_volume_db(int64_t id) const;
	void set_bus_mute(int64_t id, bool value);
	bool is_bus_mute(int64_t id) const;
	void set_bus_solo(int64_t id, bool value);
	bool is_bus_solo(int64_t id) const;
	void set_bus_bypass(int64_t id, bool value);
	bool is_bus_bypass(int64_t id) const;
	int64_t add_bus_effect(int64_t bus_id, const Ref<FmodAudioEffect> &effect, int index = -1);
	Error remove_bus_effect(int64_t bus_id, int64_t slot_id);
	Error move_bus_effect(int64_t bus_id, int64_t slot_id, int64_t target_bus, int index);
	void set_bus_effect_enabled(int64_t bus_id, int64_t slot_id, bool enabled);
	bool is_bus_effect_enabled(int64_t bus_id, int64_t slot_id) const;
	Ref<FmodAudioEffect> get_bus_effect(int64_t bus_id, int64_t slot_id) const;
	PackedInt64Array get_bus_effect_ids(int64_t bus_id) const;
	Array get_state() const;
	Error set_state(const Array &state);
	Error validate() const;
};
} //namespace godot
#endif
