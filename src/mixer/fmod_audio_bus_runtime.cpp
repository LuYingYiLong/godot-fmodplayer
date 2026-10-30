#include "fmod_audio_bus_runtime.h"
#include "core/fmod_server.h"
#include "fmod_audio_bus_converter.h"
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <set>

namespace godot {
void copy_fmod_effect_parameters(const Ref<FmodAudioEffect> &source, const Ref<FmodAudioEffect> &target) {
	if (source.is_null() || target.is_null()) {
		return;
	}
	const TypedArray<Dictionary> properties = source->get_property_list();
	for (int i = 0; i < properties.size(); i++) {
		Dictionary p = properties[i];
		const StringName name = p["name"];
		if (!(int64_t(p["usage"]) & PROPERTY_USAGE_STORAGE) || String(name).begins_with("resource_") || name == StringName("script")) {
			continue;
		}
		Variant value = source->get(name);
		if (target->get(name) != value) {
			target->set(name, value);
		}
	}
}
void FmodAudioBusRuntime::_bind_methods() {}
void FmodAudioBusRuntime::_changed() {
	dirty = true;
}
void FmodAudioBusRuntime::_effect_changed(int64_t id) {
	for (auto &pair : slots) {
		if (pair.second.source.is_valid() && pair.second.source->get_instance_id() == uint64_t(id)) {
			pair.second.dirty = true;
		}
	}
}
void FmodAudioBusRuntime::_native_changed() {
	source_dirty = true;
}
FmodAudioBusRuntime::~FmodAudioBusRuntime() {
	clear();
}
Ref<FmodAudioBus> FmodAudioBusRuntime::get_bus(int64_t id) const {
	auto it = buses.find(id);
	return it == buses.end() ? Ref<FmodAudioBus>() : it->second;
}
Ref<FmodAudioEffect> FmodAudioBusRuntime::get_effect(int64_t bus, int64_t id) const {
	auto it = slots.find(id);
	return it != slots.end() && it->second.bus_id == bus ? it->second.instance : Ref<FmodAudioEffect>();
}
void FmodAudioBusRuntime::_clear_observers() {
	for (auto &pair : effect_observers) {
		if (pair.second.first->is_connected("changed", pair.second.second)) {
			pair.second.first->disconnect("changed", pair.second.second);
		}
	}
	effect_observers.clear();
}
void FmodAudioBusRuntime::_clear_native_observers() {
	for (auto &item : native_effects) {
		if (item.source.is_valid() && item.source->is_connected("changed", item.observer)) {
			item.source->disconnect("changed", item.observer);
		}
	}
	native_effects.clear();
}
void FmodAudioBusRuntime::clear() {
	_clear_native_observers();
	_clear_observers();
	if (layout.is_valid() && layout->is_connected("changed", callable_mp(this, &FmodAudioBusRuntime::_changed))) {
		layout->disconnect("changed", callable_mp(this, &FmodAudioBusRuntime::_changed));
	}
	for (auto &pair : slots) {
		if (pair.second.instance.is_valid()) {
			pair.second.instance->remove_from_bus(pair.second.instance->get_bus());
		}
	}
	slots.clear();
	for (auto &pair : buses) {
		pair.second->set_effect_instances({});
		pair.second->release();
	}
	buses.clear();
	applied_state.clear();
	applied_sends.clear();
	layout.unref();
	native_state.clear();
	dirty = true;
}
Error FmodAudioBusRuntime::initialize(const Ref<FmodAudioBusLayout> &data) {
	if (data.is_null() || data->validate() != OK) {
		return ERR_INVALID_DATA;
	}
	layout = data;
	layout->connect("changed", callable_mp(this, &FmodAudioBusRuntime::_changed));
	const Error result = _apply();
	if (result != OK) {
		clear();
	}
	return result;
}
void FmodAudioBusRuntime::move_channels_to(FmodAudioBusRuntime *replacement) {
	for (auto &pair : buses) {
		Ref<FmodAudioBus> target;
		if (replacement && replacement->layout.is_valid()) {
			target = replacement->get_bus(replacement->layout->find_bus_id(pair.second->get_bus_name()));
		}
		Ref<FmodChannelGroup> group = target.is_valid() ? target->get_bus() : FmodServer::get_master_channel_group();
		auto from = pair.second->get_bus();
		if (from.is_null() || from == group) {
			continue;
		}
		for (int i = from->get_num_channels() - 1; i >= 0; i--) {
			auto channel = from->get_channel(i);
			if (channel.is_valid()) {
				channel->set_channel_group(group);
			}
		}
	}
}
Error FmodAudioBusRuntime::_apply() {
	if (layout.is_null() || layout->validate() != OK) {
		return ERR_INVALID_DATA;
	}
	const auto system = FmodServer::get_main_system();
	if (system.is_null() || system->system_is_null()) {
		return ERR_UNCONFIGURED;
	}
	// Prepare all new groups and effect objects before changing the existing graph.
	std::map<int64_t, Ref<FmodAudioBus>> created;
	std::map<int64_t, Ref<FmodAudioEffect>> effects;
	for (const auto &b : layout->get_buses()) {
		if (!buses.count(b.id)) {
			Ref<FmodAudioBus> instance;
			instance.instantiate();
			instance->init_bus(b.name);
			if (instance->get_bus().is_null()) {
				for (auto &p : created) {
					p.second->release();
				}
				return ERR_CANT_CREATE;
			}
			if (b.id) {
				instance->get_bus()->set_mute(true);
			}
			created[b.id] = instance;
		}
		for (const auto &s : b.effects) {
			if (!slots.count(s.id) || slots[s.id].instance->get_class() != s.effect->get_class()) {
				Ref<FmodAudioEffect> instance = s.effect->duplicate(true);
				if (instance.is_null()) {
					for (auto &p : created) {
						p.second->release();
					}
					return ERR_CANT_CREATE;
				}
				effects[s.id] = instance;
			}
		}
	}
	// Build new DSP chains on a muted staging group so allocation failures cannot
	// tear down a working slot or partially replace the active layout.
	Ref<FmodChannelGroup> staging;
	if (!effects.empty()) {
		staging = system->create_channel_group("FMOD layout staging");
		bool ready = staging.is_valid();
		if (ready) {
			staging->set_mute(true);
		}
		for (auto &pair : effects) {
			if (!ready) {
				break;
			}
			pair.second->apply_to(staging);
			const auto chain = pair.second->get_dsp_chain();
			if (chain.is_empty() && pair.second->get_class() != StringName("FmodAudioEffect")) {
				ready = false;
			}
			for (const auto &dsp : chain) {
				int index = -1;
				if (dsp.is_null() || !dsp->dsp || staging->channel_group->getDSPIndex(dsp->dsp, &index) != FMOD_OK) {
					ready = false;
				}
			}
		}
		if (!ready) {
			for (auto &pair : effects) {
				pair.second->remove_from_bus(staging);
			}
			if (staging.is_valid()) {
				staging->release();
			}
			for (auto &pair : created) {
				pair.second->release();
			}
			return ERR_CANT_CREATE;
		}
	}
	system->lock_dsp();
	for (auto &p : created) {
		buses[p.first] = p.second;
	}
	std::set<int64_t> wanted;
	for (const auto &b : layout->get_buses()) {
		for (const auto &s : b.effects) {
			wanted.insert(s.id);
		}
	}
	for (auto it = slots.begin(); it != slots.end();) {
		if (!wanted.count(it->first) || effects.count(it->first)) {
			auto &s = it->second;
			s.instance->remove_from_bus(s.instance->get_bus());
			it = slots.erase(it);
		} else {
			++it;
		}
	}
	std::map<int64_t, Dictionary> previous;
	for (int i = 0; i < applied_state.size(); i++) {
		Dictionary d = applied_state[i];
		previous[int64_t(d["id"])] = d;
	}
	std::set<int64_t> audible;
	bool any_solo = false;
	for (const auto &b : layout->get_buses()) {
		if (b.solo) {
			any_solo = true;
			int64_t id = b.id;
			while (audible.insert(id).second && id != 0) {
				id = layout->get_bus_send(id);
			}
		}
	}
	for (const auto &b : layout->get_buses()) {
		auto runtime = buses[b.id];
		auto group = runtime->get_bus();
		runtime->set_runtime_name(b.name);
		if (b.id && (!applied_sends.count(b.id) || applied_sends[b.id] != b.send_id || created.count(b.id))) {
			runtime->set_parent(buses[b.send_id]->get_bus());
		}
		applied_sends[b.id] = b.send_id;
		const auto prev = previous.find(b.id);
		if (prev == previous.end() || float(prev->second["volume_db"]) != b.volume_db) {
			runtime->set_volume_db(b.volume_db);
		}
		runtime->set_solo(b.solo);
		runtime->set_mute(b.mute);
		runtime->set_bypass(b.bypass);
		group->set_mute(any_solo ? !audible.count(b.id) : b.mute);
		Vector<Ref<FmodAudioEffect>> ordered;
		std::vector<Ref<FmodDSP>> chain;
		for (const auto &s : b.effects) {
			if (!slots.count(s.id)) {
				Slot value;
				value.bus_id = b.id;
				value.instance = effects[s.id];
				slots[s.id] = value;
			}
			auto &live = slots[s.id];
			if (live.source != s.effect) {
				live.source = s.effect;
				live.dirty = true;
			}
			if (live.instance->get_bus() != group) {
				// Reconnect the same DSPs: moving a slot preserves its delay/reverb state.
				for (const auto &dsp : live.instance->get_dsp_chain()) {
					if (live.instance->bus.is_valid()) {
						live.instance->bus->remove_dsp(dsp);
					}
					group->add_dsp(-1, dsp);
				}
				live.instance->bus = group;
			}
			live.bus_id = b.id;
			if (live.dirty) {
				copy_fmod_effect_parameters(live.source, live.instance);
				live.dirty = false;
			}
			for (const auto &dsp : live.instance->get_dsp_chain()) {
				dsp->set_bypass(b.bypass || !s.enabled);
				chain.push_back(dsp);
			}
			ordered.push_back(live.instance);
		}
		// FMOD executes the chain from tail to head; slot zero is processed first.
		int index = 0;
		for (auto it = chain.rbegin(); it != chain.rend(); ++it, ++index) {
			if (group->get_dsp_index(*it) != index) {
				group->set_dsp_index(*it, index);
			}
		}
		runtime->set_effect_instances(ordered);
	}
	for (auto it = buses.begin(); it != buses.end();) {
		if (!layout->find_bus(it->first)) {
			int64_t parent = applied_sends.count(it->first) ? applied_sends[it->first] : 0;
			std::set<int64_t> visited;
			while (!layout->find_bus(parent) && visited.insert(parent).second) {
				parent = applied_sends.count(parent) ? applied_sends[parent] : 0;
			}
			if (!layout->find_bus(parent)) {
				parent = 0;
			}
			auto from = it->second->get_bus();
			for (int i = from->get_num_channels() - 1; i >= 0; i--) {
				auto channel = from->get_channel(i);
				if (channel.is_valid()) {
					channel->set_channel_group(buses[parent]->get_bus());
				}
			}
			it->second->set_effect_instances({});
			it->second->release();
			applied_sends.erase(it->first);
			it = buses.erase(it);
		} else {
			++it;
		}
	}
	std::set<uint64_t> sources;
	for (const auto &pair : slots) {
		sources.insert(pair.second.source->get_instance_id());
	}
	for (auto it = effect_observers.begin(); it != effect_observers.end();) {
		if (!sources.count(it->first)) {
			if (it->second.first->is_connected("changed", it->second.second)) {
				it->second.first->disconnect("changed", it->second.second);
			}
			it = effect_observers.erase(it);
		} else {
			++it;
		}
	}
	for (const auto &pair : slots) {
		const auto source = pair.second.source;
		const uint64_t id = source->get_instance_id();
		if (!effect_observers.count(id)) {
			Callable callback = callable_mp(this, &FmodAudioBusRuntime::_effect_changed).bind(int64_t(id));
			source->connect("changed", callback);
			effect_observers[id] = { source, callback };
		}
	}
	system->unlock_dsp();
	if (staging.is_valid()) {
		staging->release();
	}
	PackedStringArray previous_names, names;
	for (int i = 0; i < applied_state.size(); i++) {
		Dictionary d = applied_state[i];
		previous_names.push_back(d["name"]);
	}
	for (const auto &b : layout->get_buses()) {
		names.push_back(b.name);
	}
	const bool structure_changed = previous_names != names;
	applied_state = layout->get_state();
	dirty = false;
	if (structure_changed && FmodServer::get_singleton()) {
		FmodServer::get_singleton()->emit_signal("bus_layout_changed");
	}
	return OK;
}
bool FmodAudioBusRuntime::godot_changed() {
	AudioServer *server = AudioServer::get_singleton();
	if (!server) {
		return false;
	}
	if (source_dirty || server->get_bus_count() != native_state.size()) {
		return true;
	}
	for (int n = 0; n < MIN(4, server->get_bus_count()); n++) {
		if (bus_cursor >= server->get_bus_count()) {
			bus_cursor = 0;
		}
		const int i = bus_cursor++;
		Dictionary d = native_state[i];
		if (StringName(d["name"]) != server->get_bus_name(i) || StringName(d["send"]) != server->get_bus_send(i) || float(d["volume_db"]) != server->get_bus_volume_db(i) || bool(d["mute"]) != server->is_bus_mute(i) || bool(d["solo"]) != server->is_bus_solo(i) || bool(d["bypass"]) != server->is_bus_bypassing_effects(i) || int(d["effects"]) != server->get_bus_effect_count(i)) {
			return true;
		}
	}
	for (int n = 0; n < MIN(4, int(native_effects.size())); n++) {
		if (effect_cursor >= int(native_effects.size())) {
			effect_cursor = 0;
		}
		const auto &s = native_effects[effect_cursor++];
		int i = server->get_bus_index(layout->get_bus_name(s.bus_id));
		if (i < 0 || s.index >= server->get_bus_effect_count(i) || server->get_bus_effect(i, s.index) != s.source || server->is_bus_effect_enabled(i, s.index) != s.enabled) {
			return true;
		}
	}
	return false;
}
Ref<FmodAudioBusLayout> FmodAudioBusRuntime::import_godot(PackedStringArray *skipped, bool observe) {
	Ref<FmodAudioBusLayout> result;
	result.instantiate();
	AudioServer *server = AudioServer::get_singleton();
	if (!server) {
		return result;
	}
	const auto old_effects = native_effects;
	const Array old_state = native_state;
	if (observe) {
		_clear_native_observers();
		native_state = Array();
	}
	std::map<int64_t, int64_t> stable_ids, stable_slots;
	std::set<int64_t> used_slots;
	int64_t next_bus = 1, next_slot = 1;
	if (layout.is_valid()) {
		next_bus = layout->get("next_bus_id");
		next_slot = layout->get("next_slot_id");
	}
	if (layout.is_valid()) {
		for (const auto &b : layout->get_buses()) {
			next_bus = MAX(next_bus, b.id + 1);
			for (const auto &s : b.effects) {
				next_slot = MAX(next_slot, s.id + 1);
			}
		}
	}
	std::map<StringName, int64_t> ids;
	for (int i = 0; i < server->get_bus_count(); i++) {
		StringName name = i == 0 ? StringName("Master") : server->get_bus_name(i);
		int64_t id = i == 0 ? 0 : result->add_bus(name);
		ids[name] = id;
		int64_t stable = observe && layout.is_valid() ? layout->find_bus_id(name) : id;
		if (stable < 0 && i < old_state.size()) {
			Dictionary old = old_state[i];
			if (server->get_bus_index(old["name"]) < 0) {
				stable = old.get("id", -1);
			}
		}
		if (stable < 0) {
			stable = next_bus++;
		}
		stable_ids[id] = i == 0 ? 0 : stable;
	}
	for (int i = 0; i < server->get_bus_count(); i++) {
		StringName name = i == 0 ? StringName("Master") : server->get_bus_name(i);
		const int64_t id = ids[name];
		const StringName send = server->get_bus_send(i);
		if (i && ids.count(send) && result->can_send_to(id, ids[send])) {
			result->set_bus_send(id, ids[send]);
		}
		result->set_bus_volume_db(id, server->get_bus_volume_db(i));
		result->set_bus_mute(id, server->is_bus_mute(i));
		result->set_bus_solo(id, server->is_bus_solo(i));
		result->set_bus_bypass(id, server->is_bus_bypassing_effects(i));
		for (int j = 0; j < server->get_bus_effect_count(i); j++) {
			Ref<AudioEffect> source = server->get_bus_effect(i, j);
			auto converted = convert_godot_audio_effect(source);
			int64_t slot_id = 0;
			if (converted.is_valid()) {
				int64_t old_slot = 0;
				for (const auto &old : old_effects) {
					if (old.source == source && !used_slots.count(old.slot_id) && old.slot_id && layout.is_valid()) {
						auto existing = layout->get_bus_effect(old.bus_id, old.slot_id);
						if (existing.is_valid() && existing->get_class() == converted->get_class()) {
							copy_fmod_effect_parameters(converted, existing);
							converted = existing;
							old_slot = old.slot_id;
							used_slots.insert(old_slot);
							break;
						}
					}
				}
				slot_id = result->add_bus_effect(id, converted);
				result->set_bus_effect_enabled(id, slot_id, server->is_bus_effect_enabled(i, j));
				stable_slots[slot_id] = observe ? (old_slot ? old_slot : next_slot++) : slot_id;
			} else if (skipped && source.is_valid()) {
				skipped->push_back(String(name) + ": " + source->get_class());
			}
			if (observe && source.is_valid()) {
				NativeEffect s;
				s.source = source;
				s.bus_id = id;
				s.slot_id = slot_id;
				s.index = j;
				s.enabled = server->is_bus_effect_enabled(i, j);
				s.observer = callable_mp(this, &FmodAudioBusRuntime::_native_changed);
				if (!source->is_connected("changed", s.observer)) {
					source->connect("changed", s.observer);
				}
				native_effects.push_back(s);
			}
		}
		if (observe) {
			Dictionary d;
			d["id"] = stable_ids[id];
			d["name"] = name;
			d["send"] = send;
			d["volume_db"] = server->get_bus_volume_db(i);
			d["mute"] = server->is_bus_mute(i);
			d["solo"] = server->is_bus_solo(i);
			d["bypass"] = server->is_bus_bypassing_effects(i);
			d["effects"] = server->get_bus_effect_count(i);
			native_state.push_back(d);
		}
	}
	if (observe) {
		Array state = result->get_state();
		for (int i = 0; i < state.size(); i++) {
			Dictionary d = state[i];
			d["id"] = stable_ids[int64_t(d["id"])];
			d["send_id"] = stable_ids[int64_t(d["send_id"])];
			Array effects = d["effects"];
			for (int j = 0; j < effects.size(); j++) {
				Dictionary e = effects[j];
				e["id"] = stable_slots[int64_t(e["id"])];
			}
		}
		result->set_state(state);
		for (auto &native : native_effects) {
			native.bus_id = stable_ids[native.bus_id];
			if (native.slot_id) {
				native.slot_id = stable_slots[native.slot_id];
			}
		}
		source_dirty = false;
		bus_cursor = 0;
		effect_cursor = 0;
	}
	return result;
}
void FmodAudioBusRuntime::process(bool sync) {
	if (sync && godot_changed()) {
		auto mirror = import_godot(nullptr, true);
		if (layout.is_valid()) {
			layout->set_state(mirror->get_state());
		}
	}
	if (dirty) {
		if (_apply() != OK) {
			dirty = false;
		}
	}
	for (auto &pair : slots) {
		if (pair.second.dirty) {
			copy_fmod_effect_parameters(pair.second.source, pair.second.instance);
			pair.second.dirty = false;
		}
	}
}
} //namespace godot
