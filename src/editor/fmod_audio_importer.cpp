#include "editor/fmod_editor_translation.h"
#include "fmod_audio_importer.h"
#include "audio/fmod_audio_stream.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {
	static void _append_import_option(
		TypedArray<Dictionary>& r_options,
		const String& p_name,
		const Variant& p_default_value,
		PropertyHint p_hint = PROPERTY_HINT_NONE,
		const String& p_hint_string = String(),
		uint32_t p_usage = PROPERTY_USAGE_DEFAULT
	) {
		Dictionary option;
		option["name"] = p_name;
		option["default_value"] = p_default_value;
		option["property_hint"] = p_hint;
		String hint = p_hint_string;
		if (p_hint == PROPERTY_HINT_ENUM || p_hint == PROPERTY_HINT_FLAGS) {
			PackedStringArray choices = hint.split(",", true);
			for (int i = 0; i < choices.size(); ++i) {
				const int separator = choices[i].find(":");
				const String label = separator >= 0 ? choices[i].substr(0, separator) : choices[i];
				choices[i] = fmod_editor_translate(label) + (separator >= 0 ? choices[i].substr(separator) : String());
			}
			hint = String(",").join(choices);
		}
		option["hint_string"] = hint;
		option["usage"] = p_usage;
		r_options.append(option);
	}

	void AudioImporterFmod::_bind_methods() {
	}

	AudioImporterFmod::AudioImporterFmod() {
	}

	AudioImporterFmod::~AudioImporterFmod() {
	}

	String AudioImporterFmod::_get_importer_name() const {
		return "fmod_audio";
	}

	String AudioImporterFmod::_get_visible_name() const {
		return fmod_editor_translate("FMOD Audio");
	}

	PackedStringArray AudioImporterFmod::_get_recognized_extensions() const {
		PackedStringArray extensions;

		// Keep this list aligned with FMOD_SOUND_TYPE formats that can be
		// reasonably detected from normal files or memory-backed imports.
		static const char* recognized_extensions[] = {
			"aac",
			"adts",
			"aif",
			"aifc",
			"aiff",
			"asf",
			"at9",
			"caf",
			"flac",
			"fsb",
			"it",
			"m4a",
			"mid",
			"midi",
			"mod",
			"mp2",
			"mp3",
			"mp4",
			"oga",
			"ogg",
			"opus",
			"rmi",
			"s3m",
			"wav",
			"wma",
			"wmv",
			"xma",
			"xm",
			"3gp",
		};

		for (const char* extension : recognized_extensions) {
			extensions.append(extension);
		}

		return extensions;
	}

	String AudioImporterFmod::_get_save_extension() const {
		return "res";
	}

	String AudioImporterFmod::_get_resource_type() const {
		return "FmodAudioStream";
	}

	int32_t AudioImporterFmod::_get_preset_count() const {
		return 3;
	}

	String AudioImporterFmod::_get_preset_name(int32_t p_preset_index) const {
		switch (p_preset_index) {
			case 0:
				return fmod_editor_translate("Stream (Low memory)");
			case 1:
				return fmod_editor_translate("Sample (High quality)");
			case 2:
				return fmod_editor_translate("Loop Sample");
			default:
				return fmod_editor_translate("Default");
		}
	}

	TypedArray<Dictionary> AudioImporterFmod::_get_import_options(const String& p_path, int32_t p_preset_index) const {
		TypedArray<Dictionary> options;

		int default_flags = FmodAudioStream::MODE_STREAM;
		switch (p_preset_index) {
			case 0:
				default_flags = FmodAudioStream::MODE_STREAM;
				break;
			case 1:
				default_flags = FmodAudioStream::MODE_SAMPLE;
				break;
			case 2:
				default_flags = FmodAudioStream::MODE_SAMPLE | FmodAudioStream::MODE_LOOP;
				break;
		}

		_append_import_option(options, "flags", default_flags, PROPERTY_HINT_FLAGS, "Stream,Sample,Loop,Loop Bidi,2D,3D");

		_append_import_option(options, "advanced/resampler_method", FmodSystem::FMOD_RESAMPLER_DEFAULT, PROPERTY_HINT_ENUM, "Default,No Interpolation,Linear,Cubic,Spline");
		_append_import_option(options, "advanced/suggested_sound_type", FmodSound::FMOD_SOUND_TYPE_UNKNOWN, PROPERTY_HINT_ENUM, "Unknown,AIFF,ASF,DLS,FLAC,FSB,IT,MIDI,MOD,MPEG,OGGVORBIS,PLAYLIST,RAW,S3M,USER,WAV,XM,XMA,AUDIOQUEUE,AT9,VORBIS,MEDIACODEC,FADPCM,OPUS,MAX", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED);
		_append_import_option(options, "advanced/length_override", 0, PROPERTY_HINT_RANGE, "0,2147483647,1,or_greater,suffix:bytes");
		_append_import_option(options, "advanced/file_offset", 0, PROPERTY_HINT_RANGE, "0,2147483647,1,or_greater,suffix:bytes");
		_append_import_option(options, "advanced/decode_buffer_size", 0, PROPERTY_HINT_RANGE, "0,1000000,1,or_greater,suffix:samples");
		_append_import_option(options, "advanced/initial_subsound", -1, PROPERTY_HINT_RANGE, "-1,65535,1");
		_append_import_option(options, "advanced/num_subsounds", 0, PROPERTY_HINT_RANGE, "0,65535,1");

		PackedInt32Array default_inclusion_list;
		_append_import_option(options, "advanced/inclusion_list", default_inclusion_list);

		_append_import_option(options, "raw/default_frequency", 0, PROPERTY_HINT_RANGE, "0,384000,1,or_greater,suffix:Hz");
		_append_import_option(options, "raw/num_channels", 0, PROPERTY_HINT_RANGE, "0,32,1");
		_append_import_option(options, "raw/format", FmodSound::FMOD_SOUND_FORMAT_NONE, PROPERTY_HINT_ENUM, "None,PCM8,PCM16,PCM24,PCM32,PCMFloat,Bitstream,Max");

		_append_import_option(options, "advanced/dls_name", String(), PROPERTY_HINT_FILE);
		_append_import_option(options, "advanced/encryption_key", String());
		_append_import_option(options, "advanced/max_polyphony", 0, PROPERTY_HINT_RANGE, "0,4095,1,or_greater");
		_append_import_option(options, "advanced/initial_seek_position", 0, PROPERTY_HINT_RANGE, "0,2147483647,1,or_greater");
		_append_import_option(options, "advanced/initial_seek_postype", FmodSystem::FMOD_TIME_UNIT_MS, PROPERTY_HINT_FLAGS, "MS,PCM,PCM Bytes,Raw Bytes,PCM Fraction,,,,MOD Order,MOD Row,MOD Pattern");
		_append_import_option(options, "advanced/ignore_set_file_system", false);

		return options;
	}

	bool AudioImporterFmod::_get_option_visibility(const String& p_path, const StringName& p_option_name, const Dictionary& p_options) const {
		String option_name = String(p_option_name);
		if (option_name.begins_with("raw/")) {
			if (!p_options.has("advanced/suggested_sound_type")) {
				return false;
			}
			return static_cast<int>(p_options["advanced/suggested_sound_type"]) == FmodSound::FMOD_SOUND_TYPE_RAW;
		}

		if (option_name == "advanced/initial_seek_postype") {
			return p_options.has("advanced/initial_seek_position") && static_cast<int>(p_options["advanced/initial_seek_position"]) > 0;
		}

		return true;
	}

	Error AudioImporterFmod::_import(const String& p_source_file, const String& p_save_path, const Dictionary& p_options, const TypedArray<String>& p_platform_variants, const TypedArray<String>& p_gen_files) const {
		Ref<FileAccess> f = FileAccess::open(p_source_file, FileAccess::READ);
		if (f.is_null()) {
			ERR_PRINT("Cannot open audio file: " + p_source_file);
			return ERR_CANT_OPEN;
		}

		uint64_t file_size = f->get_length();
		PackedByteArray audio_data;
		audio_data.resize(file_size);
		f->get_buffer(audio_data.ptrw(), file_size);
		f->close();

		if (audio_data.is_empty()) {
			ERR_PRINT("Audio file is empty: " + p_source_file);
			return ERR_FILE_CORRUPT;
		}

		Ref<FmodAudioStream> stream;
		stream.instantiate();
		stream->set_audio_data(audio_data);

		if (p_options.has("flags")) {
			int flags = p_options["flags"];
			stream->set_mode_flags(flags);
		}
		else {
			stream->set_mode_flags(FmodAudioStream::MODE_STREAM);
		}

		if (p_options.has("advanced/length_override")) {
			stream->set_length_override(static_cast<int>(p_options["advanced/length_override"]));
		}
		if (p_options.has("advanced/file_offset")) {
			stream->set_file_offset(static_cast<int>(p_options["advanced/file_offset"]));
		}
		if (p_options.has("advanced/decode_buffer_size")) {
			stream->set_decode_buffer_size(static_cast<int>(p_options["advanced/decode_buffer_size"]));
		}
		if (p_options.has("advanced/initial_subsound")) {
			stream->set_initial_subsound(static_cast<int>(p_options["advanced/initial_subsound"]));
		}
		if (p_options.has("advanced/num_subsounds")) {
			stream->set_num_subsounds(static_cast<int>(p_options["advanced/num_subsounds"]));
		}
		if (p_options.has("advanced/inclusion_list")) {
			stream->set_inclusion_list(p_options["advanced/inclusion_list"]);
		}
		if (p_options.has("advanced/resampler_method")) {
			stream->set_resampler_method(static_cast<FmodSystem::FmodResamplerMethod>(static_cast<int>(p_options["advanced/resampler_method"])));
		}
		if (p_options.has("advanced/suggested_sound_type")) {
			stream->set_suggested_sound_type(static_cast<FmodSound::FmodSoundType>(static_cast<int>(p_options["advanced/suggested_sound_type"])));
		}
		if (p_options.has("raw/default_frequency")) {
			stream->set_default_frequency(static_cast<int>(p_options["raw/default_frequency"]));
		}
		if (p_options.has("raw/num_channels")) {
			stream->set_num_channels(static_cast<int>(p_options["raw/num_channels"]));
		}
		if (p_options.has("raw/format")) {
			stream->set_format(static_cast<FmodSound::FmodSoundFormat>(static_cast<int>(p_options["raw/format"])));
		}
		if (p_options.has("advanced/dls_name")) {
			stream->set_dls_name(p_options["advanced/dls_name"]);
		}
		if (p_options.has("advanced/encryption_key")) {
			stream->set_encryption_key(p_options["advanced/encryption_key"]);
		}
		if (p_options.has("advanced/max_polyphony")) {
			stream->set_max_polyphony(static_cast<int>(p_options["advanced/max_polyphony"]));
		}
		if (p_options.has("advanced/initial_seek_position")) {
			stream->set_initial_seek_position(static_cast<int>(p_options["advanced/initial_seek_position"]));
		}
		if (p_options.has("advanced/initial_seek_postype")) {
			stream->set_initial_seek_postype(static_cast<FmodSystem::FmodTimeUnit>(static_cast<int>(p_options["advanced/initial_seek_postype"])));
		}
		if (p_options.has("advanced/ignore_set_file_system")) {
			stream->set_ignore_set_file_system(p_options["advanced/ignore_set_file_system"]);
		}

		String save_path = p_save_path + String(".") + _get_save_extension();
		Error err = ResourceSaver::get_singleton()->save(stream, save_path);
		if (err != OK) {
			ERR_PRINT("Failed to save FMOD audio resource: " + save_path + ", error: " + String::num_int64(err));
			return err;
		}

		return OK;
	}
}
