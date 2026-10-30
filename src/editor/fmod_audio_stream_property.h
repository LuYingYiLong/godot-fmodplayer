#ifndef FMOD_AUDIO_STREAM_PROPERTY_H
#define FMOD_AUDIO_STREAM_PROPERTY_H

#include "editor/fmod_audio_waveform.h"
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_property.hpp>
#include <godot_cpp/classes/editor_resource_picker.hpp>

namespace godot {
	// 保留原生资源选择器及按钮，只在资源按钮内绘制音频预览
	class FmodAudioStreamPickerPreview : public FmodAudioWaveformView {
		GDCLASS(FmodAudioStreamPickerPreview, FmodAudioWaveformView)
		EditorResourcePicker* picker = nullptr;
		Button* assign_button = nullptr;
		Ref<Texture2D> resource_icon;
		bool synchronizing = false;
		void _sync_native_button();
	protected:
		static void _bind_methods();
		void _notification(int p_what);
	public:
		FmodAudioStreamPickerPreview();
		void setup(EditorResourcePicker* p_picker, Button* p_button);
		void _draw() override;
	};

	void decorate_fmod_audio_stream_property(EditorProperty* p_editor);

}

#endif // FMOD_AUDIO_STREAM_PROPERTY_H
