#include "fmod_audio_effect_inspector.h"
#include "fmod_audio_effect_panner_preview.h"
#include "fmod_audio_effect_eq_preview.h"
#include "fmod_audio_effect_filter_preview.h"
#include "fmod_audio_effect_compressor_preview.h"
#include "fmod_audio_effect_capture_preview.h"
#include "fmod_audio_effect_amplify_preview.h"
#include "fmod_audio_effect_chorus_preview.h"
#include "fmod_audio_effect_reverb_preview.h"

namespace godot {
	void FmodAudioEffectInspector::_bind_methods() {
	}

	bool FmodAudioEffectInspector::_can_handle(Object* p_object) const {
		return Object::cast_to<FmodAudioEffectPanner>(p_object) != nullptr ||
			Object::cast_to<FmodAudioEffectEQ>(p_object) != nullptr ||
			Object::cast_to<FmodAudioEffectFilter>(p_object) != nullptr ||
			Object::cast_to<FmodAudioEffectCompressor>(p_object) != nullptr ||
			Object::cast_to<FmodAudioEffectCapture>(p_object) != nullptr ||
			Object::cast_to<FmodAudioEffectAmplify>(p_object) != nullptr ||
			Object::cast_to<FmodAudioEffectChorus>(p_object) != nullptr ||
			Object::cast_to<FmodAudioEffectReverb>(p_object) != nullptr;
	}

	void FmodAudioEffectInspector::_parse_begin(Object* p_object) {
		FmodAudioEffectReverb* reverb = Object::cast_to<FmodAudioEffectReverb>(p_object);
		if (reverb) {
			FmodAudioEffectReverbPreview* preview = memnew(FmodAudioEffectReverbPreview);
			preview->set_effect(Ref<FmodAudioEffectReverb>(reverb));
			add_custom_control(preview);
			return;
		}
		FmodAudioEffectChorus* chorus = Object::cast_to<FmodAudioEffectChorus>(p_object);
		if (chorus) {
			FmodAudioEffectChorusPreview* preview = memnew(FmodAudioEffectChorusPreview);
			preview->set_effect(Ref<FmodAudioEffectChorus>(chorus));
			add_custom_control(preview);
			return;
		}
		FmodAudioEffectAmplify* amplify = Object::cast_to<FmodAudioEffectAmplify>(p_object);
		if (amplify) {
			FmodAudioEffectAmplifyPreview* preview = memnew(FmodAudioEffectAmplifyPreview);
			preview->set_effect(Ref<FmodAudioEffectAmplify>(amplify));
			add_custom_control(preview);
			return;
		}
		FmodAudioEffectCapture* capture = Object::cast_to<FmodAudioEffectCapture>(p_object);
		if (capture) {
			FmodAudioEffectCapturePreview* preview = memnew(FmodAudioEffectCapturePreview);
			preview->set_effect(Ref<FmodAudioEffectCapture>(capture));
			add_custom_control(preview);
			return;
		}
		FmodAudioEffectCompressor* compressor = Object::cast_to<FmodAudioEffectCompressor>(p_object);
		if (compressor) {
			FmodAudioEffectCompressorPreview* preview = memnew(FmodAudioEffectCompressorPreview);
			preview->set_effect(Ref<FmodAudioEffectCompressor>(compressor));
			add_custom_control(preview);
			return;
		}
		FmodAudioEffectFilter* filter = Object::cast_to<FmodAudioEffectFilter>(p_object);
		if (filter) {
			FmodAudioEffectFilterPreview* preview = memnew(FmodAudioEffectFilterPreview);
			preview->set_effect(Ref<FmodAudioEffectFilter>(filter));
			add_custom_control(preview);
			return;
		}
		FmodAudioEffectEQ* eq = Object::cast_to<FmodAudioEffectEQ>(p_object);
		if (eq) {
			FmodAudioEffectEQPreview* preview = memnew(FmodAudioEffectEQPreview);
			preview->set_effect(Ref<FmodAudioEffectEQ>(eq));
			add_custom_control(preview);
			return;
		}
		FmodAudioEffectPanner* panner = Object::cast_to<FmodAudioEffectPanner>(p_object);
		if (!panner) {
			return;
		}
		FmodAudioEffectPannerPreview* preview = memnew(FmodAudioEffectPannerPreview);
		preview->set_effect(Ref<FmodAudioEffectPanner>(panner));
		// 保留 EditorProperty 的编辑信号接入，不关联单一属性，避免生成重复的重置按钮
		add_custom_control(preview);
	}

	void FmodAudioEffectEditorPlugin::_bind_methods() {
	}

	void FmodAudioEffectEditorPlugin::_notification(int p_what) {
		if (p_what == NOTIFICATION_ENTER_TREE) {
			inspector.instantiate();
			add_inspector_plugin(inspector);
		}
		else if (p_what == NOTIFICATION_EXIT_TREE && inspector.is_valid()) {
			remove_inspector_plugin(inspector);
			inspector.unref();
		}
	}
}
