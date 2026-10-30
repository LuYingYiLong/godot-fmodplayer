#ifndef FMOD_AUDIO_WAVEFORM_H
#define FMOD_AUDIO_WAVEFORM_H

#include "audio/fmod_audio_stream.h"
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <memory>

namespace godot {
	class FmodAudioWaveform : public RefCounted {
		GDCLASS(FmodAudioWaveform, RefCounted)
		friend class FmodAudioWaveformCache;
		PackedVector2Array envelope;
		double length = 0.0;
		bool ready = false;
		String error;
	protected:
		static void _bind_methods();
	public:
		bool is_ready() const { return ready; }
		String get_error() const { return error; }
		double get_length() const { return length; }
		PackedVector2Array get_envelope() const { return envelope; }
		PackedVector2Array get_points(int p_width, float p_height) const;
	};

	class FmodAudioWaveformCache : public Node {
		GDCLASS(FmodAudioWaveformCache, Node)
		struct State;
		std::unique_ptr<State> state;
		static FmodAudioWaveformCache* singleton;
	protected:
		static void _bind_methods();
		void _notification(int p_what);
	public:
		static FmodAudioWaveformCache* get_singleton() { return singleton; }
		Ref<FmodAudioWaveform> request(const Ref<FmodAudioStream>& p_stream);
		FmodAudioWaveformCache();
		~FmodAudioWaveformCache();
	};

	class FmodAudioWaveformView : public Control {
		GDCLASS(FmodAudioWaveformView, Control)
		Ref<FmodAudioStream> stream;
		Ref<FmodAudioWaveform> waveform;
		PackedVector2Array points;
		bool picker_presentation = false;
		void _request_waveform();
		void _waveform_updated();
	protected:
		static void _bind_methods();
		void _notification(int p_what);
	public:
		FmodAudioWaveformView();
		~FmodAudioWaveformView();
		void set_stream(const Ref<FmodAudioStream>& p_stream);
		Ref<FmodAudioStream> get_stream() const { return stream; }
		void set_picker_presentation(bool p_enabled) { picker_presentation = p_enabled; }
		Ref<FmodAudioWaveform> get_waveform() const { return waveform; }
		void _draw() override;
	};

	class FmodAudioStreamEditorPlugin : public EditorPlugin {
		GDCLASS(FmodAudioStreamEditorPlugin, EditorPlugin)
		FmodAudioWaveformCache* cache = nullptr;
	protected:
		static void _bind_methods();
		void _notification(int p_what);
	};
}

#endif // FMOD_AUDIO_WAVEFORM_H
