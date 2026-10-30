#ifndef FMOD_AUDIO_PREVIEW_PROPERTY_H
#define FMOD_AUDIO_PREVIEW_PROPERTY_H

#include "audio/fmod_audio_stream.h"
#include "nodes/fmod_audio_stream_player.h"
#include "editor/fmod_audio_waveform.h"
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
#include "core/fmod_utils.hpp"

namespace godot {
	class FmodAudioPreviewProperty : public ColorRect {
		GDCLASS(FmodAudioPreviewProperty, ColorRect)

	private:
		// 预览区域
		FmodAudioWaveformView* _preview = nullptr;
		Control* _indicator = nullptr;

		// 控制栏
		Button* _play_button = nullptr;
		Button* _stop_button = nullptr;
		Label* _current_label = nullptr;
		Label* _duration_label = nullptr;

		// 音频播放器
		FmodAudioStreamPlayer* _player = nullptr;

		// 当前音频流
		Ref<FmodAudioStream> stream;

		// 播放状态
		bool is_playing = false;
		bool is_pausing = false;
		bool is_dragging = false;
		float playback_position = 0.0f;
		float stream_duration = 0.0f;

		void _on_waveform_updated();
		void _draw_indicator();
		void _on_indicator_gui_input(const Ref<InputEvent>& p_event);
		void _update_playback_position(float p_position);
		void _update_time_labels();
		void _queue_preview_redraw();
		void _seek_to(float p_x);

		void _play();
		void _stop();
		void _on_finished();
		void _update_preview();
		void _process_playback();

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		void set_stream(const Ref<FmodAudioStream>& p_stream);
		Ref<FmodAudioStream> get_stream() const;

		FmodAudioPreviewProperty();
		~FmodAudioPreviewProperty();
	};
}

#endif // !FMOD_AUDIO_PREVIEW_PROPERTY_H
