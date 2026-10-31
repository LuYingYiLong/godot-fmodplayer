#include "audio/fmod_audio_stream.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/project_settings.hpp>

#include <limits>
#include <vector>

namespace godot {
	void FmodAudioStream::_bind_methods() {
		ClassDB::bind_method(D_METHOD("set_resampler_method", "method"), &FmodAudioStream::set_resampler_method);
		ClassDB::bind_method(D_METHOD("get_resampler_method"), &FmodAudioStream::get_resampler_method);
		ADD_PROPERTY(PropertyInfo(Variant::INT, "resampler_method", PROPERTY_HINT_ENUM, "Default,No Interpolation,Linear,Cubic,Spline"), "set_resampler_method", "get_resampler_method");
		BIND_ENUM_CONSTANT(MODE_DEFAULT);
		BIND_ENUM_CONSTANT(MODE_STREAM);
		BIND_ENUM_CONSTANT(MODE_SAMPLE);
		BIND_ENUM_CONSTANT(MODE_LOOP);
		BIND_ENUM_CONSTANT(MODE_LOOP_BIDI);
		BIND_ENUM_CONSTANT(MODE_2D);
		BIND_ENUM_CONSTANT(MODE_3D);

		ClassDB::bind_method(D_METHOD("set_audio_data", "data"), &FmodAudioStream::set_audio_data);
		ClassDB::bind_method(D_METHOD("get_audio_data"), &FmodAudioStream::get_audio_data);

		ClassDB::bind_method(D_METHOD("set_mode_flags", "flags"), &FmodAudioStream::set_mode_flags);
		ClassDB::bind_method(D_METHOD("get_mode_flags"), &FmodAudioStream::get_mode_flags);

		ClassDB::bind_method(D_METHOD("set_length_override", "length"), &FmodAudioStream::set_length_override);
		ClassDB::bind_method(D_METHOD("get_length_override"), &FmodAudioStream::get_length_override);
		ClassDB::bind_method(D_METHOD("set_file_offset", "offset"), &FmodAudioStream::set_file_offset);
		ClassDB::bind_method(D_METHOD("get_file_offset"), &FmodAudioStream::get_file_offset);
		ClassDB::bind_method(D_METHOD("set_decode_buffer_size", "size"), &FmodAudioStream::set_decode_buffer_size);
		ClassDB::bind_method(D_METHOD("get_decode_buffer_size"), &FmodAudioStream::get_decode_buffer_size);
		ClassDB::bind_method(D_METHOD("set_initial_subsound", "subsound"), &FmodAudioStream::set_initial_subsound);
		ClassDB::bind_method(D_METHOD("get_initial_subsound"), &FmodAudioStream::get_initial_subsound);
		ClassDB::bind_method(D_METHOD("set_num_subsounds", "count"), &FmodAudioStream::set_num_subsounds);
		ClassDB::bind_method(D_METHOD("get_num_subsounds"), &FmodAudioStream::get_num_subsounds);
		ClassDB::bind_method(D_METHOD("set_inclusion_list", "list"), &FmodAudioStream::set_inclusion_list);
		ClassDB::bind_method(D_METHOD("get_inclusion_list"), &FmodAudioStream::get_inclusion_list);
		ClassDB::bind_method(D_METHOD("set_suggested_sound_type", "type"), &FmodAudioStream::set_suggested_sound_type);
		ClassDB::bind_method(D_METHOD("get_suggested_sound_type"), &FmodAudioStream::get_suggested_sound_type);
		ClassDB::bind_method(D_METHOD("set_default_frequency", "frequency"), &FmodAudioStream::set_default_frequency);
		ClassDB::bind_method(D_METHOD("get_default_frequency"), &FmodAudioStream::get_default_frequency);
		ClassDB::bind_method(D_METHOD("set_num_channels", "channels"), &FmodAudioStream::set_num_channels);
		ClassDB::bind_method(D_METHOD("get_num_channels"), &FmodAudioStream::get_num_channels);
		ClassDB::bind_method(D_METHOD("set_format", "format"), &FmodAudioStream::set_format);
		ClassDB::bind_method(D_METHOD("get_format"), &FmodAudioStream::get_format);
		ClassDB::bind_method(D_METHOD("set_dls_name", "name"), &FmodAudioStream::set_dls_name);
		ClassDB::bind_method(D_METHOD("get_dls_name"), &FmodAudioStream::get_dls_name);
		ClassDB::bind_method(D_METHOD("set_encryption_key", "key"), &FmodAudioStream::set_encryption_key);
		ClassDB::bind_method(D_METHOD("get_encryption_key"), &FmodAudioStream::get_encryption_key);
		ClassDB::bind_method(D_METHOD("set_max_polyphony", "polyphony"), &FmodAudioStream::set_max_polyphony);
		ClassDB::bind_method(D_METHOD("get_max_polyphony"), &FmodAudioStream::get_max_polyphony);
		ClassDB::bind_method(D_METHOD("set_initial_seek_position", "position"), &FmodAudioStream::set_initial_seek_position);
		ClassDB::bind_method(D_METHOD("get_initial_seek_position"), &FmodAudioStream::get_initial_seek_position);
		ClassDB::bind_method(D_METHOD("set_initial_seek_postype", "postype"), &FmodAudioStream::set_initial_seek_postype);
		ClassDB::bind_method(D_METHOD("get_initial_seek_postype"), &FmodAudioStream::get_initial_seek_postype);
		ClassDB::bind_method(D_METHOD("set_ignore_set_file_system", "ignore"), &FmodAudioStream::set_ignore_set_file_system);
		ClassDB::bind_method(D_METHOD("is_ignore_set_file_system"), &FmodAudioStream::is_ignore_set_file_system);

		ClassDB::bind_method(D_METHOD("get_sound"), &FmodAudioStream::get_sound);
		ClassDB::bind_method(D_METHOD("preload"), &FmodAudioStream::preload);
		ClassDB::bind_method(D_METHOD("is_preloaded"), &FmodAudioStream::is_preloaded);
		ClassDB::bind_method(D_METHOD("get_length"), &FmodAudioStream::get_length);
		ClassDB::bind_method(D_METHOD("is_data_loaded"), &FmodAudioStream::is_data_loaded);
		ClassDB::bind_method(D_METHOD("clear"), &FmodAudioStream::clear);

		ClassDB::bind_static_method("FmodAudioStream", D_METHOD("load_from_file", "path", "flags"), &FmodAudioStream::load_from_file, DEFVAL(MODE_STREAM));

		ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "audio_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_audio_data", "get_audio_data");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "mode_flags", PROPERTY_HINT_FLAGS, "Stream,Sample,Loop,Loop Bidi,2D,3D"), "set_mode_flags", "get_mode_flags");

		ADD_GROUP("Create Info", "");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "length_override", PROPERTY_HINT_RANGE, "0,2147483647,1,or_greater,suffix:bytes"), "set_length_override", "get_length_override");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "file_offset", PROPERTY_HINT_RANGE, "0,2147483647,1,or_greater,suffix:bytes"), "set_file_offset", "get_file_offset");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "decode_buffer_size", PROPERTY_HINT_RANGE, "0,1000000,1,or_greater,suffix:samples"), "set_decode_buffer_size", "get_decode_buffer_size");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "initial_subsound", PROPERTY_HINT_RANGE, "-1,65535,1"), "set_initial_subsound", "get_initial_subsound");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "num_subsounds", PROPERTY_HINT_RANGE, "0,65535,1"), "set_num_subsounds", "get_num_subsounds");
		ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "inclusion_list"), "set_inclusion_list", "get_inclusion_list");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "suggested_sound_type", PROPERTY_HINT_ENUM, "Unknown,AIFF,ASF,DLS,FLAC,FSB,IT,MIDI,MOD,MPEG,OGGVORBIS,PLAYLIST,RAW,S3M,USER,WAV,XM,XMA,AUDIOQUEUE,AT9,VORBIS,MEDIACODEC,FADPCM,OPUS,MAX"), "set_suggested_sound_type", "get_suggested_sound_type");

		ADD_GROUP("Raw PCM", "");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "default_frequency", PROPERTY_HINT_RANGE, "0,384000,1,or_greater,suffix:Hz"), "set_default_frequency", "get_default_frequency");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "num_channels", PROPERTY_HINT_RANGE, "0,32,1"), "set_num_channels", "get_num_channels");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "format", PROPERTY_HINT_ENUM, "None,PCM8,PCM16,PCM24,PCM32,PCMFloat,Bitstream,Max"), "set_format", "get_format");

		ADD_GROUP("Advanced", "");
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "dls_name", PROPERTY_HINT_FILE), "set_dls_name", "get_dls_name");
		ADD_PROPERTY(PropertyInfo(Variant::STRING, "encryption_key"), "set_encryption_key", "get_encryption_key");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "max_polyphony", PROPERTY_HINT_RANGE, "0,4095,1,or_greater"), "set_max_polyphony", "get_max_polyphony");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "initial_seek_position", PROPERTY_HINT_RANGE, "0,2147483647,1,or_greater"), "set_initial_seek_position", "get_initial_seek_position");
		ADD_PROPERTY(PropertyInfo(Variant::INT, "initial_seek_postype", PROPERTY_HINT_FLAGS, "MS,PCM,PCM Bytes,Raw Bytes,PCM Fraction,,,,MOD Order,MOD Row,MOD Pattern"), "set_initial_seek_postype", "get_initial_seek_postype");
		ADD_PROPERTY(PropertyInfo(Variant::BOOL, "ignore_set_file_system"), "set_ignore_set_file_system", "is_ignore_set_file_system");
	}

	FmodAudioStream::FmodAudioStream() {
		create_mode_flags = MODE_STREAM;
	}

	FmodAudioStream::~FmodAudioStream() {
		sound.unref();
	}

	void FmodAudioStream::set_audio_data(const PackedByteArray& p_data) {
		audio_data = p_data;
		data_loaded = !audio_data.is_empty();
		invalidate_sound();
	}

	PackedByteArray FmodAudioStream::get_audio_data() const {
		return audio_data;
	}

	void FmodAudioStream::set_mode_flags(int p_flags) {
		create_mode_flags = p_flags;
		invalidate_sound(false);
	}

	int FmodAudioStream::get_mode_flags() const {
		return create_mode_flags;
	}

	void FmodAudioStream::add_mode_flag(CreateMode p_flag) {
		unsigned int new_flags = create_mode_flags | p_flag;
		if (new_flags != create_mode_flags) {
			create_mode_flags = new_flags;
			invalidate_sound(false);
		}
	}

	void FmodAudioStream::remove_mode_flag(CreateMode p_flag) {
		unsigned int new_flags = create_mode_flags & ~p_flag;
		if (new_flags != create_mode_flags) {
			create_mode_flags = new_flags;
			invalidate_sound(false);
		}
	}

	bool FmodAudioStream::has_mode_flag(CreateMode p_flag) const {
		return (create_mode_flags & p_flag) != 0;
	}

	unsigned int FmodAudioStream::_get_fmod_mode_flags() const {
		unsigned int fmod_mode = FmodSystem::FMOD_MODE_OPENMEMORY;

		if (has_mode_flag(MODE_STREAM)) {
			fmod_mode |= FmodSystem::FMOD_MODE_CREATESTREAM;
		}
		else {
			fmod_mode |= FmodSystem::FMOD_MODE_CREATESAMPLE | FmodSystem::FMOD_MODE_LOOP_OFF;
		}

		if (has_mode_flag(MODE_LOOP)) {
			fmod_mode |= FmodSystem::FMOD_MODE_LOOP_NORMAL;
		}
		if (has_mode_flag(MODE_LOOP_BIDI)) {
			fmod_mode |= FmodSystem::FMOD_MODE_LOOP_BIDI;
		}

		if (has_mode_flag(MODE_3D)) {
			fmod_mode |= FmodSystem::FMOD_MODE_3D;
		}
		else {
			fmod_mode |= FmodSystem::FMOD_MODE_2D;
		}

		return fmod_mode;
	}

	Ref<FmodSound> FmodAudioStream::_create_sound() {
		if (!data_loaded || audio_data.is_empty()) {
			return Ref<FmodSound>();
		}
		ERR_FAIL_COND_V_MSG(audio_data.size() > static_cast<int64_t>(std::numeric_limits<unsigned int>::max()), Ref<FmodSound>(), "Sound data is too large for FMOD OPENMEMORY.");

		Ref<FmodSystem> system = FmodServer::get_main_system();
		if (system.is_null() || system->system_is_null()) {
			return Ref<FmodSound>();
		}

		Ref<FmodSound> fmod_sound;
		fmod_sound.instantiate();
		fmod_sound->data = audio_data;

		FMOD_CREATESOUNDEXINFO exinfo = {};
		exinfo.cbsize = sizeof(exinfo);
		exinfo.resamplermethod = static_cast<FMOD_DSP_RESAMPLER>(resampler_method);
		exinfo.length = length_override > 0 ? static_cast<unsigned int>(MIN(length_override, audio_data.size())) : static_cast<unsigned int>(audio_data.size());

		if (file_offset > 0) {
			exinfo.fileoffset = static_cast<unsigned int>(file_offset);
		}
		if (decode_buffer_size > 0) {
			exinfo.decodebuffersize = static_cast<unsigned int>(decode_buffer_size);
		}
		if (initial_subsound >= 0) {
			exinfo.initialsubsound = initial_subsound;
		}
		if (num_subsounds > 0) {
			exinfo.numsubsounds = num_subsounds;
		}

		std::vector<int> inclusion_storage;
		if (!inclusion_list.is_empty()) {
			inclusion_storage.reserve(inclusion_list.size());
			for (int i = 0; i < inclusion_list.size(); i++) {
				inclusion_storage.push_back(inclusion_list[i]);
			}
			exinfo.inclusionlist = inclusion_storage.data();
			exinfo.inclusionlistnum = static_cast<int>(inclusion_storage.size());
		}

		if (suggested_sound_type != FmodSound::FMOD_SOUND_TYPE_UNKNOWN) {
			exinfo.suggestedsoundtype = static_cast<FMOD_SOUND_TYPE>(suggested_sound_type);
		}
		if (default_frequency > 0) {
			exinfo.defaultfrequency = default_frequency;
		}
		if (num_channels > 0) {
			exinfo.numchannels = num_channels;
		}
		if (format != FmodSound::FMOD_SOUND_FORMAT_NONE) {
			exinfo.format = static_cast<FMOD_SOUND_FORMAT>(format);
		}

		CharString dls_name_utf8;
		if (!dls_name.is_empty()) {
			String resolved_dls_name = dls_name;
			if (dls_name.begins_with("res://") || dls_name.begins_with("user://")) {
				ProjectSettings* project_settings = ProjectSettings::get_singleton();
				ERR_FAIL_NULL_V(project_settings, Ref<FmodSound>());
				resolved_dls_name = project_settings->globalize_path(dls_name);
			}
			dls_name_utf8 = resolved_dls_name.utf8();
			exinfo.dlsname = dls_name_utf8.get_data();
		}

		CharString encryption_key_utf8;
		if (!encryption_key.is_empty()) {
			encryption_key_utf8 = encryption_key.utf8();
			exinfo.encryptionkey = encryption_key_utf8.get_data();
		}

		if (max_polyphony > 0) {
			exinfo.maxpolyphony = max_polyphony;
		}
		if (initial_seek_position > 0) {
			exinfo.initialseekposition = static_cast<unsigned int>(initial_seek_position);
			exinfo.initialseekpostype = static_cast<FMOD_TIMEUNIT>(initial_seek_postype);
		}
		exinfo.ignoresetfilesystem = ignore_set_file_system ? 1 : 0;

		FMOD::Sound* sound_ptr = nullptr;
		FMOD_ERR_CHECK_V(system->get_system()->createSound(
			reinterpret_cast<const char*>(fmod_sound->data.ptr()),
			_get_fmod_mode_flags(),
			&exinfo,
			&sound_ptr
		), Ref<FmodSound>());

		ERR_FAIL_NULL_V(sound_ptr, Ref<FmodSound>());
		fmod_sound->setup(sound_ptr, true, system->sound_lifetime);
		return fmod_sound;
	}

	void FmodAudioStream::set_length_override(int p_length) {
		length_override = MAX(p_length, 0);
		invalidate_sound();
	}

	int FmodAudioStream::get_length_override() const {
		return length_override;
	}

	void FmodAudioStream::set_file_offset(int p_offset) {
		file_offset = MAX(p_offset, 0);
		invalidate_sound();
	}

	int FmodAudioStream::get_file_offset() const {
		return file_offset;
	}

	void FmodAudioStream::set_decode_buffer_size(int p_size) {
		decode_buffer_size = MAX(p_size, 0);
		invalidate_sound();
	}

	int FmodAudioStream::get_decode_buffer_size() const {
		return decode_buffer_size;
	}

	void FmodAudioStream::set_initial_subsound(int p_subsound) {
		initial_subsound = MAX(p_subsound, -1);
		invalidate_sound();
	}

	int FmodAudioStream::get_initial_subsound() const {
		return initial_subsound;
	}

	void FmodAudioStream::set_num_subsounds(int p_count) {
		num_subsounds = MAX(p_count, 0);
		invalidate_sound();
	}

	int FmodAudioStream::get_num_subsounds() const {
		return num_subsounds;
	}

	void FmodAudioStream::set_inclusion_list(const PackedInt32Array& p_list) {
		inclusion_list = p_list;
		invalidate_sound();
	}

	PackedInt32Array FmodAudioStream::get_inclusion_list() const {
		return inclusion_list;
	}

	void FmodAudioStream::set_resampler_method(FmodSystem::FmodResamplerMethod p_method) {
		ERR_FAIL_COND(p_method < FmodSystem::FMOD_RESAMPLER_DEFAULT || p_method >= FmodSystem::FMOD_RESAMPLER_MAX);
		if (resampler_method == p_method) return;
		resampler_method = p_method;
		invalidate_sound();
	}

	FmodSystem::FmodResamplerMethod FmodAudioStream::get_resampler_method() const {
		return resampler_method;
	}

	void FmodAudioStream::set_suggested_sound_type(FmodSound::FmodSoundType p_type) {
		suggested_sound_type = p_type;
		invalidate_sound();
	}

	FmodSound::FmodSoundType FmodAudioStream::get_suggested_sound_type() const {
		return suggested_sound_type;
	}

	void FmodAudioStream::set_default_frequency(int p_frequency) {
		default_frequency = MAX(p_frequency, 0);
		invalidate_sound();
	}

	int FmodAudioStream::get_default_frequency() const {
		return default_frequency;
	}

	void FmodAudioStream::set_num_channels(int p_channels) {
		num_channels = CLAMP(p_channels, 0, 32);
		invalidate_sound();
	}

	int FmodAudioStream::get_num_channels() const {
		return num_channels;
	}

	void FmodAudioStream::set_format(FmodSound::FmodSoundFormat p_format) {
		format = p_format;
		invalidate_sound();
	}

	FmodSound::FmodSoundFormat FmodAudioStream::get_format() const {
		return format;
	}

	void FmodAudioStream::set_dls_name(const String& p_name) {
		dls_name = p_name;
		invalidate_sound();
	}

	String FmodAudioStream::get_dls_name() const {
		return dls_name;
	}

	void FmodAudioStream::set_encryption_key(const String& p_key) {
		encryption_key = p_key;
		invalidate_sound();
	}

	String FmodAudioStream::get_encryption_key() const {
		return encryption_key;
	}

	void FmodAudioStream::set_max_polyphony(int p_polyphony) {
		max_polyphony = MAX(p_polyphony, 0);
		invalidate_sound();
	}

	int FmodAudioStream::get_max_polyphony() const {
		return max_polyphony;
	}

	void FmodAudioStream::set_initial_seek_position(int p_position) {
		initial_seek_position = MAX(p_position, 0);
		invalidate_sound();
	}

	int FmodAudioStream::get_initial_seek_position() const {
		return initial_seek_position;
	}

	void FmodAudioStream::set_initial_seek_postype(FmodSystem::FmodTimeUnit p_postype) {
		initial_seek_postype = p_postype;
		invalidate_sound();
	}

	FmodSystem::FmodTimeUnit FmodAudioStream::get_initial_seek_postype() const {
		return initial_seek_postype;
	}

	void FmodAudioStream::set_ignore_set_file_system(bool p_ignore) {
		ignore_set_file_system = p_ignore;
		invalidate_sound();
	}

	bool FmodAudioStream::is_ignore_set_file_system() const {
		return ignore_set_file_system;
	}

	Ref<FmodSound> FmodAudioStream::get_sound() {
		if (sound.is_valid()) {
			return sound;
		}

		if (data_loaded && !audio_data.is_empty()) {
			sound = _create_sound();
		}

		return sound;
	}

	bool FmodAudioStream::preload() {
		return get_sound().is_valid();
	}

	bool FmodAudioStream::is_preloaded() const {
		return sound.is_valid();
	}

	double FmodAudioStream::get_length() const {
		if (sound.is_valid()) {
			return sound->get_length();
		}
		return 0.0;
	}

	bool FmodAudioStream::is_data_loaded() const {
		return data_loaded;
	}

	Ref<FmodAudioStream> FmodAudioStream::load_from_file(const String& p_path, int p_flags) {
		Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
		ERR_FAIL_COND_V_MSG(file.is_null(), Ref<FmodAudioStream>(), "Cannot open audio file: " + p_path);

		PackedByteArray data = file->get_buffer(file->get_length());
		file->close();
		ERR_FAIL_COND_V_MSG(data.is_empty(), Ref<FmodAudioStream>(), "Failed to read audio file: " + p_path);

		Ref<FmodAudioStream> stream;
		stream.instantiate();
		stream->set_audio_data(data);
		stream->set_mode_flags(p_flags);

		return stream;
	}

	void FmodAudioStream::clear() {
		audio_data.clear();
		data_loaded = false;
		invalidate_sound();
	}

	void FmodAudioStream::invalidate_sound(bool p_waveform_changed) {
		if (sound.is_valid()) {
			sound.unref();
		}
		if (p_waveform_changed) waveform_revision++;
		emit_changed();
	}
}
