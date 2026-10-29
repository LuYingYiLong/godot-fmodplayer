#include "fmod_sound_group.h"
#include "audio/fmod_sound.h"

namespace godot {
	void FmodSoundGroup::_bind_methods() {
		BIND_ENUM_CONSTANT(BEHAVIOR_FAIL);
		BIND_ENUM_CONSTANT(BEHAVIOR_MUTE);
		BIND_ENUM_CONSTANT(BEHAVIOR_STEAL_LOWEST);
		BIND_ENUM_CONSTANT(BEHAVIOR_MAX);

		ClassDB::bind_method(D_METHOD("sound_group_is_valid"), &FmodSoundGroup::sound_group_is_valid);
		ClassDB::bind_method(D_METHOD("sound_group_is_null"), &FmodSoundGroup::sound_group_is_null);

		ClassDB::bind_method(D_METHOD("set_max_audible", "max_audible"), &FmodSoundGroup::set_max_audible);
		ClassDB::bind_method(D_METHOD("get_max_audible"), &FmodSoundGroup::get_max_audible);

		ClassDB::bind_method(D_METHOD("set_max_audible_behavior", "behavior"), &FmodSoundGroup::set_max_audible_behavior);
		ClassDB::bind_method(D_METHOD("get_max_audible_behavior"), &FmodSoundGroup::get_max_audible_behavior);
		
		ClassDB::bind_method(D_METHOD("set_mute_fade_speed", "speed"), &FmodSoundGroup::set_mute_fade_speed);
		ClassDB::bind_method(D_METHOD("get_mute_fade_speed"), &FmodSoundGroup::get_mute_fade_speed);

		ClassDB::bind_method(D_METHOD("set_volume_db", "volume_db"), &FmodSoundGroup::set_volume_db);
		ClassDB::bind_method(D_METHOD("get_volume_db"), &FmodSoundGroup::get_volume_db);
	
		ClassDB::bind_method(D_METHOD("get_num_sounds"), &FmodSoundGroup::get_num_sounds);
		ClassDB::bind_method(D_METHOD("get_sound", "index"), &FmodSoundGroup::get_sound);
		ClassDB::bind_method(D_METHOD("get_num_playing"), &FmodSoundGroup::get_num_playing);
		ClassDB::bind_method(D_METHOD("stop"), &FmodSoundGroup::stop);

		ClassDB::bind_method(D_METHOD("get_name"), &FmodSoundGroup::get_name);
		ClassDB::bind_method(D_METHOD("release"), &FmodSoundGroup::release);

		ADD_PROPERTY(PropertyInfo(Variant::INT, "max_audible"), "set_max_audible", "get_max_audible");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "behavior", PROPERTY_HINT_ENUM, "Fail,Mute,StealLowest,Max"), "set_max_audible_behavior", "get_max_audible_behavior");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mute_fade_speed"), "set_mute_fade_speed", "get_mute_fade_speed");
		ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volume_db", PROPERTY_HINT_RANGE, "-80,24,0.1,suffix:dB"), "set_volume_db", "get_volume_db");
	}

	FmodSoundGroup::FmodSoundGroup() {
	}

	FmodSoundGroup::~FmodSoundGroup() {
		if (sound_group) {
			sound_group->setUserData(nullptr);
			sound_group = nullptr;
		}
	}

	bool FmodSoundGroup::sound_group_is_valid() const {
		return sound_group != nullptr;
	}

	bool FmodSoundGroup::sound_group_is_null() const {
		return sound_group == nullptr;
	}

	void FmodSoundGroup::setup(FMOD::SoundGroup* p_sound_group) {
		ERR_FAIL_COND_MSG(!p_sound_group, "SoundGroup pointer is null");

		if (sound_group) {
			sound_group->setUserData(nullptr);
		}

		sound_group = p_sound_group;
		sound_group->setUserData(this);
	}

	void FmodSoundGroup::set_max_audible(const int max_audible) {
		ERR_FAIL_COND(!sound_group);
		ERR_FAIL_COND_MSG(max_audible < -1, "max_audible must be >= -1 (-1 means unlimited).");
		FMOD_ERR_CHECK(sound_group->setMaxAudible(max_audible));
	}

	int FmodSoundGroup::get_max_audible() const {
		if (!sound_group) return 0;
		int max_audible = 0;
		FMOD_ERR_CHECK_V(sound_group->getMaxAudible(&max_audible), 0);
		return max_audible;
	}

	void FmodSoundGroup::set_max_audible_behavior(const Behavior behavior) {
		ERR_FAIL_COND(!sound_group);
		ERR_FAIL_COND_MSG(behavior < BEHAVIOR_FAIL || behavior >= BEHAVIOR_MAX, "Invalid SoundGroup behavior.");
		FMOD_ERR_CHECK(sound_group->setMaxAudibleBehavior(static_cast<FMOD_SOUNDGROUP_BEHAVIOR>(behavior)));
	}

	FmodSoundGroup::Behavior FmodSoundGroup::get_max_audible_behavior() const {
		if (!sound_group) return BEHAVIOR_FAIL;
		FMOD_SOUNDGROUP_BEHAVIOR behavior;
		FMOD_ERR_CHECK_V(sound_group->getMaxAudibleBehavior(&behavior), BEHAVIOR_FAIL);
		return static_cast<Behavior>(behavior);
	}

	void FmodSoundGroup::set_mute_fade_speed(const float speed) {
		ERR_FAIL_COND(!sound_group);
		ERR_FAIL_COND_MSG(speed < 0.0f, "Mute fade speed must be >= 0.");
		FMOD_ERR_CHECK(sound_group->setMuteFadeSpeed(speed));
	}

	float FmodSoundGroup::get_mute_fade_speed() const {
		if (!sound_group) return 0.0f;
		float speed = 0.0f;
		FMOD_ERR_CHECK_V(sound_group->getMuteFadeSpeed(&speed), 0.0f);
		return speed;
	}

	void FmodSoundGroup::set_volume_db(const float volume_db) {
		ERR_FAIL_COND(!sound_group);
		FMOD_ERR_CHECK(sound_group->setVolume(FmodUtils::db_to_linear(volume_db)));
	}

	float FmodSoundGroup::get_volume_db() const {
		if (!sound_group) return 0.0f;
		float volume = 0.0f;
		FMOD_ERR_CHECK_V(sound_group->getVolume(&volume), 0.0f);
		return FmodUtils::linear_to_db(volume);
	}

	int FmodSoundGroup::get_num_sounds() const {
		ERR_FAIL_COND_V(!sound_group, 0);
		int numsounds = 0;
		FMOD_ERR_CHECK_V(sound_group->getNumSounds(&numsounds), 0);
		return numsounds;
	}

	Ref<FmodSound> FmodSoundGroup::get_sound(const int index) const {
		ERR_FAIL_COND_V(!sound_group, Ref<FmodSound>());
		ERR_FAIL_COND_V_MSG(index < 0, Ref<FmodSound>(), "Sound index must be >= 0.");
		FMOD::Sound* sound_ptr = nullptr;
		FMOD_ERR_CHECK_V(sound_group->getSound(index, &sound_ptr), Ref<FmodSound>());
		ERR_FAIL_NULL_V(sound_ptr, Ref<FmodSound>());
		Ref<FmodSound> sound;
		sound.instantiate();
		sound->setup(sound_ptr, false);
		return sound;
	}

	int FmodSoundGroup::get_num_playing() const {
		ERR_FAIL_COND_V(!sound_group, 0);
		int numplaying = 0;
		FMOD_ERR_CHECK_V(sound_group->getNumPlaying(&numplaying), 0);
		return numplaying;
	}

	void FmodSoundGroup::stop() {
		ERR_FAIL_COND(!sound_group);
		FMOD_ERR_CHECK(sound_group->stop());
	}

	String FmodSoundGroup::get_name() const {
		ERR_FAIL_COND_V(!sound_group, String());
		char name[256] = { 0 };
		FMOD_ERR_CHECK_V(sound_group->getName(name, 256), String());
		return String::utf8(name);
	}

	void FmodSoundGroup::release() {
		ERR_FAIL_COND(!sound_group);
		sound_group->setUserData(nullptr);
		FMOD_RESULT result = sound_group->release();
		if (result != FMOD_OK) {
			ERR_PRINT(vformat("Failed to release FMOD SoundGroup: %s", FMOD_ErrorString(result)));
			return;
		}
		sound_group = nullptr;
	}
}
