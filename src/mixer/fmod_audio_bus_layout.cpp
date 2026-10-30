#include "fmod_audio_bus_layout.h"
#include <algorithm>
#include <cmath>
#include <godot_cpp/variant/dictionary.hpp>
#include <set>

namespace godot {
void FmodAudioBusLayout::_bind_methods() {
#define B(method, ...) ClassDB::bind_method(D_METHOD(#method, __VA_ARGS__), &FmodAudioBusLayout::method)
	ClassDB::bind_method(D_METHOD("get_bus_count"), &FmodAudioBusLayout::get_bus_count);
	ClassDB::bind_method(D_METHOD("get_bus_ids"), &FmodAudioBusLayout::get_bus_ids);
	B(find_bus_id, "name");
	B(get_bus_name, "id");
	ClassDB::bind_method(D_METHOD("add_bus", "name", "send_id"), &FmodAudioBusLayout::add_bus, DEFVAL(0));
	B(remove_bus, "id");
	B(move_bus, "id", "index");
	B(rename_bus, "id", "name");
	B(can_send_to, "id", "target");
	B(set_bus_send, "id", "target");
	B(get_bus_send, "id");
	B(set_bus_volume_db, "id", "volume_db");
	B(get_bus_volume_db, "id");
	B(set_bus_mute, "id", "mute");
	B(is_bus_mute, "id");
	B(set_bus_solo, "id", "solo");
	B(is_bus_solo, "id");
	B(set_bus_bypass, "id", "bypass");
	B(is_bus_bypass, "id");
	ClassDB::bind_method(D_METHOD("add_bus_effect", "bus_id", "effect", "index"), &FmodAudioBusLayout::add_bus_effect, DEFVAL(-1));
	B(remove_bus_effect, "bus_id", "slot_id");
	B(move_bus_effect, "bus_id", "slot_id", "target_bus", "index");
	B(set_bus_effect_enabled, "bus_id", "slot_id", "enabled");
	B(is_bus_effect_enabled, "bus_id", "slot_id");
	B(get_bus_effect, "bus_id", "slot_id");
	B(get_bus_effect_ids, "bus_id");
	B(set_state, "state");
	ClassDB::bind_method(D_METHOD("get_state"), &FmodAudioBusLayout::get_state);
	ClassDB::bind_method(D_METHOD("validate"), &FmodAudioBusLayout::validate);
#undef B
}
FmodAudioBusLayout::FmodAudioBusLayout() {
	Bus b;
	b.name = "Master";
	buses.push_back(b);
}
void FmodAudioBusLayout::_touch() {
	notify_property_list_changed();
	emit_changed();
}
const FmodAudioBusLayout::Bus *FmodAudioBusLayout::find_bus(int64_t id) const {
	for (const auto &b : buses) {
		if (b.id == id) {
			return &b;
		}
	}
	return nullptr;
}
PackedInt64Array FmodAudioBusLayout::get_bus_ids() const {
	PackedInt64Array ids;
	for (const auto &b : buses) {
		ids.push_back(b.id);
	}
	return ids;
}
int64_t FmodAudioBusLayout::find_bus_id(const StringName &name) const {
	for (const auto &b : buses) {
		if (b.name == name) {
			return b.id;
		}
	}
	return -1;
}
StringName FmodAudioBusLayout::get_bus_name(int64_t id) const {
	const Bus *b = find_bus(id);
	return b ? b->name : StringName();
}
int64_t FmodAudioBusLayout::add_bus(const StringName &name, int64_t send) {
	if (buses.size() >= 4096 || next_bus_id == INT64_MAX || String(name).strip_edges().is_empty() || String(name).contains(",") || find_bus_id(name) >= 0 || !find_bus(send)) {
		return -1;
	}
	Bus b;
	b.id = next_bus_id++;
	b.name = name;
	b.send_id = send;
	buses.push_back(b);
	_touch();
	return b.id;
}
Error FmodAudioBusLayout::remove_bus(int64_t id) {
	const Bus *b = find_bus(id);
	if (!b || id == 0) {
		return ERR_INVALID_PARAMETER;
	}
	const int64_t target = b->send_id;
	for (auto &item : buses) {
		if (item.send_id == id) {
			item.send_id = target;
		}
	}
	buses.erase(std::remove_if(buses.begin(), buses.end(), [id](const Bus &b) { return b.id == id; }), buses.end());
	_touch();
	return OK;
}
Error FmodAudioBusLayout::move_bus(int64_t id, int index) {
	if (id == 0 || !find_bus(id) || index < 1 || index >= int(buses.size())) {
		return ERR_INVALID_PARAMETER;
	}
	auto it = std::find_if(buses.begin(), buses.end(), [id](const Bus &b) { return b.id == id; });
	Bus b = *it;
	buses.erase(it);
	buses.insert(buses.begin() + index, b);
	_touch();
	return OK;
}
Error FmodAudioBusLayout::rename_bus(int64_t id, const StringName &name) {
	Bus *b = const_cast<Bus *>(find_bus(id));
	if (!b || id == 0 || String(name).strip_edges().is_empty() || String(name).contains(",") || (find_bus_id(name) >= 0 && b->name != name)) {
		return ERR_INVALID_PARAMETER;
	}
	if (b->name != name) {
		b->name = name;
		_touch();
	}
	return OK;
}
bool FmodAudioBusLayout::can_send_to(int64_t id, int64_t target) const {
	if (id == 0 || !find_bus(id) || !find_bus(target)) {
		return false;
	}
	for (size_t n = 0; n <= buses.size(); n++) {
		if (target == id) {
			return false;
		}
		if (target == 0) {
			return true;
		}
		const Bus *b = find_bus(target);
		if (!b) {
			return false;
		}
		target = b->send_id;
	}
	return false;
}
Error FmodAudioBusLayout::set_bus_send(int64_t id, int64_t target) {
	if (!can_send_to(id, target)) {
		return ERR_INVALID_PARAMETER;
	}
	Bus *b = const_cast<Bus *>(find_bus(id));
	if (b->send_id != target) {
		b->send_id = target;
		_touch();
	}
	return OK;
}
int64_t FmodAudioBusLayout::get_bus_send(int64_t id) const {
	const Bus *b = find_bus(id);
	return b ? b->send_id : -1;
}
#define STATE(type, setter, getter, field, fallback)      \
	void FmodAudioBusLayout::setter(int64_t id, type v) { \
		Bus *b = const_cast<Bus *>(find_bus(id));         \
		if (b && b->field != v) {                         \
			b->field = v;                                 \
			emit_changed();                               \
		}                                                 \
	}                                                     \
	type FmodAudioBusLayout::getter(int64_t id) const {   \
		const Bus *b = find_bus(id);                      \
		return b ? b->field : fallback;                   \
	}
STATE(bool, set_bus_mute, is_bus_mute, mute, false)
STATE(bool, set_bus_solo, is_bus_solo, solo, false)
STATE(bool, set_bus_bypass, is_bus_bypass, bypass, false)
#undef STATE
void FmodAudioBusLayout::set_bus_volume_db(int64_t id, float v) {
	if (!std::isfinite(v)) {
		return;
	}
	Bus *b = const_cast<Bus *>(find_bus(id));
	if (b && b->volume_db != v) {
		b->volume_db = v;
		emit_changed();
	}
}
float FmodAudioBusLayout::get_bus_volume_db(int64_t id) const {
	const Bus *b = find_bus(id);
	return b ? b->volume_db : 0.0f;
}
int64_t FmodAudioBusLayout::add_bus_effect(int64_t id, const Ref<FmodAudioEffect> &effect, int index) {
	Bus *b = const_cast<Bus *>(find_bus(id));
	if (!b || b->effects.size() >= 512 || next_slot_id == INT64_MAX || effect.is_null()) {
		return -1;
	}
	if (index < 0) {
		index = int(b->effects.size());
	}
	if (index > int(b->effects.size())) {
		return -1;
	}
	EffectSlot s;
	s.id = next_slot_id++;
	s.effect = effect;
	b->effects.insert(b->effects.begin() + index, s);
	_touch();
	return s.id;
}
Error FmodAudioBusLayout::remove_bus_effect(int64_t id, int64_t slot) {
	Bus *b = const_cast<Bus *>(find_bus(id));
	if (!b) {
		return ERR_INVALID_PARAMETER;
	}
	auto it = std::find_if(b->effects.begin(), b->effects.end(), [slot](const EffectSlot &s) { return s.id == slot; });
	if (it == b->effects.end()) {
		return ERR_INVALID_PARAMETER;
	}
	b->effects.erase(it);
	_touch();
	return OK;
}
Error FmodAudioBusLayout::move_bus_effect(int64_t id, int64_t slot, int64_t target, int index) {
	Bus *from = const_cast<Bus *>(find_bus(id));
	Bus *to = const_cast<Bus *>(find_bus(target));
	if (!from || !to || index < 0 || index > int(to->effects.size())) {
		return ERR_INVALID_PARAMETER;
	}
	auto it = std::find_if(from->effects.begin(), from->effects.end(), [slot](const EffectSlot &s) { return s.id == slot; });
	if (it == from->effects.end()) {
		return ERR_INVALID_PARAMETER;
	}
	EffectSlot saved = *it;
	const int old = int(it - from->effects.begin());
	from->effects.erase(it);
	if (from == to && index > old) {
		index--;
	}
	to->effects.insert(to->effects.begin() + index, saved);
	_touch();
	return OK;
}
void FmodAudioBusLayout::set_bus_effect_enabled(int64_t id, int64_t slot, bool enabled) {
	Bus *b = const_cast<Bus *>(find_bus(id));
	if (b) {
		for (auto &s : b->effects) {
			if (s.id == slot && s.enabled != enabled) {
				s.enabled = enabled;
				emit_changed();
				return;
			}
		}
	}
}
bool FmodAudioBusLayout::is_bus_effect_enabled(int64_t id, int64_t slot) const {
	const Bus *b = find_bus(id);
	if (b) {
		for (const auto &s : b->effects) {
			if (s.id == slot) {
				return s.enabled;
			}
		}
	}
	return false;
}
Ref<FmodAudioEffect> FmodAudioBusLayout::get_bus_effect(int64_t id, int64_t slot) const {
	const Bus *b = find_bus(id);
	if (b) {
		for (const auto &s : b->effects) {
			if (s.id == slot) {
				return s.effect;
			}
		}
	}
	return {};
}
PackedInt64Array FmodAudioBusLayout::get_bus_effect_ids(int64_t id) const {
	PackedInt64Array a;
	const Bus *b = find_bus(id);
	if (b) {
		for (const auto &s : b->effects) {
			a.push_back(s.id);
		}
	}
	return a;
}
Array FmodAudioBusLayout::get_state() const {
	Array a;
	for (const auto &b : buses) {
		Dictionary d;
		d["id"] = b.id;
		d["name"] = b.name;
		d["send_id"] = b.send_id;
		d["volume_db"] = b.volume_db;
		d["solo"] = b.solo;
		d["mute"] = b.mute;
		d["bypass_fx"] = b.bypass;
		Array effects;
		for (const auto &s : b.effects) {
			Dictionary e;
			e["id"] = s.id;
			e["effect"] = s.effect;
			e["enabled"] = s.enabled;
			effects.push_back(e);
		}
		d["effects"] = effects;
		a.push_back(d);
	}
	return a;
}
Error FmodAudioBusLayout::set_state(const Array &state) {
	if (state.is_empty() || state.size() > 4096) {
		return ERR_INVALID_DATA;
	}
	auto previous = buses;
	buses.clear();
	for (int i = 0; i < state.size(); i++) {
		if (state[i].get_type() != Variant::DICTIONARY) {
			buses = previous;
			return ERR_INVALID_DATA;
		}
		Dictionary d = state[i];
		Bus b;
		b.id = d.get("id", -1);
		b.name = d.get("name", "");
		b.send_id = d.get("send_id", 0);
		b.volume_db = d.get("volume_db", 0.0);
		b.solo = d.get("solo", false);
		b.mute = d.get("mute", false);
		b.bypass = d.get("bypass_fx", false);
		Array effects = d.get("effects", Array());
		if (effects.size() > 512) {
			buses = previous;
			return ERR_INVALID_DATA;
		}
		for (int j = 0; j < effects.size(); j++) {
			if (effects[j].get_type() != Variant::DICTIONARY) {
				buses = previous;
				return ERR_INVALID_DATA;
			}
			Dictionary e = effects[j];
			EffectSlot s;
			s.id = e.get("id", -1);
			s.effect = e.get("effect", Variant());
			s.enabled = e.get("enabled", true);
			b.effects.push_back(s);
		}
		buses.push_back(b);
	}
	if (validate() != OK) {
		buses = previous;
		return ERR_INVALID_DATA;
	}
	for (const auto &b : buses) {
		next_bus_id = MAX(next_bus_id, b.id + 1);
		for (const auto &s : b.effects) {
			next_slot_id = MAX(next_slot_id, s.id + 1);
		}
	}
	_touch();
	return OK;
}
Error FmodAudioBusLayout::validate() const {
	if (layout_version != 1 || buses.empty() || buses[0].id != 0 || buses[0].name != StringName("Master") || buses[0].send_id != 0) {
		return ERR_INVALID_DATA;
	}
	std::set<int64_t> ids, slots;
	std::set<String> names;
	for (const auto &b : buses) {
		if (b.id < 0 || b.id == INT64_MAX || !ids.insert(b.id).second || !names.insert(String(b.name)).second || String(b.name).strip_edges().is_empty() || String(b.name).contains(",") || !std::isfinite(b.volume_db)) {
			return ERR_INVALID_DATA;
		}
		if (b.id && !can_send_to(b.id, b.send_id)) {
			return ERR_INVALID_DATA;
		}
		for (const auto &s : b.effects) {
			if (s.id <= 0 || s.id == INT64_MAX || s.effect.is_null() || !slots.insert(s.id).second) {
				return ERR_INVALID_DATA;
			}
		}
	}
	return OK;
}
bool FmodAudioBusLayout::_set(const StringName &name, const Variant &v) {
	String k = name;
	if (k == "layout_version") {
		layout_version = v;
		return true;
	}
	if (k == "next_bus_id") {
		next_bus_id = MAX(int64_t(v), int64_t(1));
		return true;
	}
	if (k == "next_slot_id") {
		next_slot_id = MAX(int64_t(v), int64_t(1));
		return true;
	}
	if (!k.begins_with("bus/") || !k.get_slice("/", 1).is_valid_int()) {
		return false;
	}
	int i = k.get_slice("/", 1).to_int();
	if (i < 0 || i >= 4096) {
		return false;
	}
	if (i >= int(buses.size())) {
		buses.resize(i + 1);
	}
	Bus &b = buses[i];
	String f = k.get_slice("/", 2);
	if (f == "id") {
		b.id = v;
		if (b.id >= 0 && b.id < INT64_MAX) {
			next_bus_id = MAX(next_bus_id, b.id + 1);
		}
	} else if (f == "name") {
		b.name = v;
	} else if (f == "send_id") {
		b.send_id = v;
	} else if (f == "volume_db") {
		b.volume_db = v;
	} else if (f == "solo") {
		b.solo = v;
	} else if (f == "mute") {
		b.mute = v;
	} else if (f == "bypass_fx") {
		b.bypass = v;
	} else if (f == "effect") {
		if (!k.get_slice("/", 3).is_valid_int()) {
			return false;
		}
		int j = k.get_slice("/", 3).to_int();
		if (j < 0 || j >= 512) {
			return false;
		}
		if (j >= int(b.effects.size())) {
			b.effects.resize(j + 1);
		}
		EffectSlot &s = b.effects[j];
		String e = k.get_slice("/", 4);
		if (e == "id") {
			s.id = v;
			if (s.id > 0 && s.id < INT64_MAX) {
				next_slot_id = MAX(next_slot_id, s.id + 1);
			}
		} else if (e == "effect") {
			s.effect = v;
		} else if (e == "enabled") {
			s.enabled = v;
		} else {
			return false;
		}
	} else {
		return false;
	}
	return true;
}
bool FmodAudioBusLayout::_get(const StringName &name, Variant &out) const {
	String k = name;
	if (k == "layout_version") {
		out = layout_version;
		return true;
	}
	if (k == "next_bus_id") {
		out = next_bus_id;
		return true;
	}
	if (k == "next_slot_id") {
		out = next_slot_id;
		return true;
	}
	if (!k.begins_with("bus/")) {
		return false;
	}
	int i = k.get_slice("/", 1).to_int();
	if (i < 0 || i >= int(buses.size())) {
		return false;
	}
	const auto &b = buses[i];
	String f = k.get_slice("/", 2);
	if (f == "id") {
		out = b.id;
	} else if (f == "name") {
		out = b.name;
	} else if (f == "send_id") {
		out = b.send_id;
	} else if (f == "volume_db") {
		out = b.volume_db;
	} else if (f == "solo") {
		out = b.solo;
	} else if (f == "mute") {
		out = b.mute;
	} else if (f == "bypass_fx") {
		out = b.bypass;
	} else if (f == "effect") {
		int j = k.get_slice("/", 3).to_int();
		if (j < 0 || j >= int(b.effects.size())) {
			return false;
		}
		const auto &s = b.effects[j];
		String e = k.get_slice("/", 4);
		if (e == "id") {
			out = s.id;
		} else if (e == "effect") {
			out = s.effect;
		} else if (e == "enabled") {
			out = s.enabled;
		} else {
			return false;
		}
	} else {
		return false;
	}
	return true;
}
void FmodAudioBusLayout::_get_property_list(List<PropertyInfo> *list) const {
	const auto usage = PROPERTY_USAGE_STORAGE | PROPERTY_USAGE_INTERNAL;
	for (const char *k : { "layout_version", "next_bus_id", "next_slot_id" }) {
		list->push_back(PropertyInfo(Variant::INT, k, PROPERTY_HINT_NONE, "", usage));
	}
	for (int i = 0; i < int(buses.size()); i++) {
		String prefix = "bus/" + String::num_int64(i) + "/";
		for (const char *f : { "id", "name", "send_id", "volume_db", "solo", "mute", "bypass_fx" }) {
			Variant v;
			_get(prefix + f, v);
			list->push_back(PropertyInfo(v.get_type(), prefix + f, PROPERTY_HINT_NONE, "", usage));
		}
		for (int j = 0; j < int(buses[i].effects.size()); j++) {
			for (const char *f : { "id", "effect", "enabled" }) {
				String k = prefix + "effect/" + String::num_int64(j) + "/" + f;
				Variant v;
				_get(k, v);
				list->push_back(PropertyInfo(v.get_type(), k, PROPERTY_HINT_NONE, "", usage));
			}
		}
	}
}
} //namespace godot
