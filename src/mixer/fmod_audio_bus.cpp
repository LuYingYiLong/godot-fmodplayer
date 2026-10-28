#include "mixer/fmod_audio_bus.h"
#include "playback/fmod_channel_group.h"
#include "dsp/fmod_audio_effect.h"

namespace godot {
	void FmodAudioBus::_bind_methods() {
		ClassDB::bind_method(D_METHOD("init_bus", "name", "parent"), &FmodAudioBus::init_bus, DEFVAL(Ref<FmodChannelGroup>()));
		ClassDB::bind_method(D_METHOD("set_parent", "parent"), &FmodAudioBus::set_parent);
		ClassDB::bind_method(D_METHOD("get_bus"), &FmodAudioBus::get_bus);
		ClassDB::bind_method(D_METHOD("get_parent"), &FmodAudioBus::get_parent);
		ClassDB::bind_method(D_METHOD("get_bus_name"), &FmodAudioBus::get_bus_name);
		ClassDB::bind_method(D_METHOD("release"), &FmodAudioBus::release);

		ClassDB::bind_method(D_METHOD("add_effect", "effect", "index"), &FmodAudioBus::add_effect, DEFVAL(0));
		ClassDB::bind_method(D_METHOD("remove_effect", "index"), &FmodAudioBus::remove_effect);
		ClassDB::bind_method(D_METHOD("get_effect_count"), &FmodAudioBus::get_effect_count);
		ClassDB::bind_method(D_METHOD("get_effect", "index"), &FmodAudioBus::get_effect);

		ClassDB::bind_method(D_METHOD("set_volume_db", "volume_db"), &FmodAudioBus::set_volume_db);
		ClassDB::bind_method(D_METHOD("get_volume_db"), &FmodAudioBus::get_volume_db);

		ClassDB::bind_method(D_METHOD("set_mute", "mute"), &FmodAudioBus::set_mute);
		ClassDB::bind_method(D_METHOD("is_mute"), &FmodAudioBus::is_mute);
		ClassDB::bind_method(D_METHOD("apply_mute", "mute"), &FmodAudioBus::apply_mute);

		ClassDB::bind_method(D_METHOD("set_solo", "solo"), &FmodAudioBus::set_solo);
		ClassDB::bind_method(D_METHOD("is_solo"), &FmodAudioBus::is_solo);

		ClassDB::bind_method(D_METHOD("set_bypass", "bypass"), &FmodAudioBus::set_bypass);
		ClassDB::bind_method(D_METHOD("is_bypass"), &FmodAudioBus::is_bypass);
		ClassDB::bind_method(D_METHOD("sync_bypass"), &FmodAudioBus::sync_bypass);
		
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volume"), "set_volume_db", "get_volume_db");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "solo"), "set_solo", "is_solo");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "mute"), "set_mute", "is_mute");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "bypass"), "set_bypass", "is_bypass");
	}

	FmodAudioBus::FmodAudioBus() {

	}

	FmodAudioBus::~FmodAudioBus() {
		if (bus.is_valid()) {
			bus.unref();
		}
	}

	void FmodAudioBus::init_bus(const String& p_name, Ref<FmodChannelGroup> p_parent) {
		Ref<FmodSystem> system = FmodServer::get_main_system();
		ERR_FAIL_COND_MSG(system.is_null(), "Failed to get FMOD System");
		bus_name = p_name;
		
		// 如果是 Master 总线，直接使用 FMOD 的 Master ChannelGroup
		if (p_name == "Master") {
			bus = FmodServer::get_master_channel_group();
			// Master 总线不需要设置父级，它本身就是根
			parent = Ref<FmodChannelGroup>();
		} else {
			bus = system->create_channel_group(p_name);
			ERR_FAIL_COND_MSG(bus.is_null(), vformat("Failed to create bus: %s", p_name));
			set_parent(p_parent);
		}
	}

	void FmodAudioBus::set_parent(Ref<FmodChannelGroup> p_parent) {
		parent = p_parent;
		if (bus.is_null()) return;
		
		// 重新连接到新的父总线
		if (parent.is_valid()) {
			parent->add_group(bus);
		} else {
			FmodServer::get_master_channel_group()->add_group(bus);
		}
	}

	Ref<FmodChannelGroup> FmodAudioBus::get_bus() const {
		return bus;
	}

	Ref<FmodChannelGroup> FmodAudioBus::get_parent() const {
		return parent;
	}

	String FmodAudioBus::get_bus_name() const {
		return bus_name;
	}

	void FmodAudioBus::release() {
		clear_effects();

		if (bus.is_valid() && bus_name != "Master") {
			bus->release();
		}

		bus.unref();
		parent.unref();
	}

	void FmodAudioBus::add_effect(Ref<FmodAudioEffect> effect, int index) {
		ERR_FAIL_COND(bus.is_null());
		ERR_FAIL_COND_MSG(effect.is_null(), "Effect is null");

		index = CLAMP(index, 0, audio_effects.size());
		audio_effects.insert(index, effect);
		effect->apply_to(bus);

		// 如果当前总线为 bypass，立即禁用新添加的效果器
		if (bypass) {
			sync_bypass();
		}
	}

	void FmodAudioBus::remove_effect(int index) {
		ERR_FAIL_COND(bus.is_null());
		ERR_FAIL_COND_MSG(index < 0 || index >= audio_effects.size(), "Index out of bounds");
		Ref<FmodAudioEffect> effect = audio_effects[index];
		if (effect.is_valid()) {
			effect->remove_from_bus(bus);
		}
		audio_effects.remove_at(index);
	}

	void FmodAudioBus::clear_effects() {
		if (bus.is_valid()) {
			for (int i = 0; i < audio_effects.size(); i++) {
				Ref<FmodAudioEffect> effect = audio_effects[i];
				if (effect.is_valid()) {
					effect->remove_from_bus(bus);
				}
			}
		}
		audio_effects.clear();
	}

	int FmodAudioBus::get_effect_count() const {
		return audio_effects.size();
	}

	Ref<FmodAudioEffect> FmodAudioBus::get_effect(int index) const {
		ERR_FAIL_COND_V(bus.is_null(), Ref<FmodAudioEffect>());
		ERR_FAIL_COND_V_MSG(index < 0 || index >= audio_effects.size(), Ref<FmodAudioEffect>(), "Index out of bounds");
		return audio_effects.get(index);
	}

	void FmodAudioBus::set_volume_db(const float volume_db) {
		ERR_FAIL_COND(bus.is_null());
		bus->set_volume_db(volume_db);
	}

	float FmodAudioBus::get_volume_db() const {
		if (bus.is_null()) return 0.0f;
		return bus->get_volume_db();
	}

	void FmodAudioBus::set_solo(bool p_solo) {
		solo = p_solo;
	}

	bool FmodAudioBus::is_solo() const {
		return solo;
	}

	void FmodAudioBus::set_mute(bool p_mute) {
		user_mute = p_mute;
	}

	bool FmodAudioBus::is_mute() const {
		return user_mute;
	}

	void FmodAudioBus::apply_mute(bool p_any_solo) {
		ERR_FAIL_COND(bus.is_null());
		if (bus.is_null()) return;
		bool final_mute = user_mute || (p_any_solo && !solo);
		bus->set_mute(final_mute);
	}

	void FmodAudioBus::set_bypass(bool p_bypass) {
		bypass = p_bypass;
	}

	bool FmodAudioBus::is_bypass() const {
		return bypass;
	}

	void FmodAudioBus::sync_bypass() {
		ERR_FAIL_COND(bus.is_null());
		int dsp_count = bus->get_num_dsps();
		for (int i = 0; i < dsp_count; ++i) {
			Ref<FmodDSP> dsp = bus->get_dsp(i);
			if (dsp.is_valid()) {
				dsp->set_bypass(bypass);
			}
		}
	}
}
