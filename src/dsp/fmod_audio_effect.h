#ifndef FMOD_AUDIO_EFFECT_H
#define FMOD_AUDIO_EFFECT_H

#include "playback/fmod_channel_group.h"
#include "dsp/fmod_dsp.h"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
	class FmodAudioEffect;
	
	// 自定义 DSP 状态数据 - 存储在 FMOD_DSP_STATE->plugindata 中
	struct CustomDSPState {
		FmodAudioEffect* effect;										// 指向效果对象（非引用计数，仅用于 C++ 虚函数调用）
		void* user_data;												// 子类自定义数据
		
		CustomDSPState() : effect(nullptr), user_data(nullptr) {}
	};
	
	class FmodAudioEffect : public Resource {
		GDCLASS(FmodAudioEffect, Resource)

	public:
		Ref<FmodChannelGroup> bus;
		Vector<Ref<FmodDSP>> dsp_chain;

	protected:
		static void _bind_methods();
		
	public:
		// 供子类重写的虚函数 - 在音频线程中调用，不能做阻塞操作
		// 注意：必须是 public，因为 C 回调需要访问
		virtual void _on_dsp_create(FMOD_DSP_STATE* dsp_state);
		virtual void _on_dsp_process(FMOD_DSP_STATE* dsp_state,
			unsigned int length,
			const FMOD_DSP_BUFFER* inbufferarray,
			const FMOD_DSP_BUFFER* outbufferarray);
		virtual void _on_dsp_release(FMOD_DSP_STATE* dsp_state);
		
	protected:

	public:
		FmodAudioEffect();
		virtual ~FmodAudioEffect();

		virtual void apply_to(Ref<FmodChannelGroup> p_bus);
		virtual void remove_from_bus(Ref<FmodChannelGroup> p_bus);
		Ref<FmodChannelGroup> get_bus() const;
		Vector<Ref<FmodDSP>> get_dsp_chain() const;
		
		// 创建自定义 DSP - 子类可以重写以提供自己的 DSP 描述
		virtual Ref<FmodDSP> create_custom_dsp(Ref<FmodSystem> system);
		// 获取 DSP 描述 - 子类重写以自定义 DSP 行为
		virtual FMOD_DSP_DESCRIPTION* get_dsp_description();
	};
}

extern "C" {
	FMOD_RESULT F_CALL fmod_custom_dsp_create_callback(FMOD_DSP_STATE* dsp_state);
	FMOD_RESULT F_CALL fmod_custom_dsp_process_callback(
		FMOD_DSP_STATE* dsp_state,
		unsigned int length,
		const FMOD_DSP_BUFFER* inbufferarray,
		const FMOD_DSP_BUFFER* outbufferarray
	);
	FMOD_RESULT F_CALL fmod_custom_dsp_release_callback(FMOD_DSP_STATE* dsp_state);
}

#endif // !FMOD_AUDIO_EFFECT_H
