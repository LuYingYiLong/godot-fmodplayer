#include "fmod_audio_stream_property.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace godot {
	void FmodAudioStreamPickerPreview::_bind_methods() {}

	FmodAudioStreamPickerPreview::FmodAudioStreamPickerPreview() {
		set_picker_presentation(true);
		set_mouse_filter(MOUSE_FILTER_IGNORE);
		set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		set_offset(SIDE_TOP, 1.0f);
		set_offset(SIDE_BOTTOM, -1.0f);
		set_offset(SIDE_RIGHT, -1.0f);
	}

	void FmodAudioStreamPickerPreview::setup(EditorResourcePicker* p_picker, Button* p_button) {
		picker = p_picker;
		assign_button = p_button;
		// 原生按钮负责点击、右键菜单与拖放，预览层不接收输入
		connect("waveform_updated", callable_mp(this, &FmodAudioStreamPickerPreview::_sync_native_button));
		assign_button->connect("minimum_size_changed", callable_mp(this, &FmodAudioStreamPickerPreview::_sync_native_button), CONNECT_DEFERRED);
	}

	void FmodAudioStreamPickerPreview::_sync_native_button() {
		if (synchronizing || !is_inside_tree() || !picker || !assign_button) return;
		synchronizing = true;
		const Ref<FmodAudioStream> stream = picker->get_edited_resource();
		if (assign_button->get_button_icon().is_valid()) resource_icon = assign_button->get_button_icon();
		set_stream(stream);
		if (stream.is_valid()) {
			if (!assign_button->get_text().is_empty()) assign_button->set_text("");
			if (assign_button->get_button_icon().is_valid()) assign_button->set_button_icon(Ref<Texture2D>());
		}
		else resource_icon.unref();
		const Ref<Font> font = get_theme_font("font", "Label");
		const int font_size = get_theme_font_size("font_size", "Label");
		const Ref<FmodAudioWaveform> waveform = get_waveform();
		const bool has_waveform = stream.is_valid() && waveform.is_valid() && waveform->get_length() > 0.0;
		const Vector2 minimum(1.0f, float(int(font->get_height(font_size) * (has_waveform ? 3.0f : 1.5f))));
		picker->set_custom_minimum_size(minimum);
		assign_button->set_custom_minimum_size(minimum);
		synchronizing = false;
	}

	void FmodAudioStreamPickerPreview::_notification(int p_what) {
		if (p_what == NOTIFICATION_READY || p_what == NOTIFICATION_THEME_CHANGED) _sync_native_button();
	}

	void FmodAudioStreamPickerPreview::_draw() {
		FmodAudioWaveformView::_draw();
		const Ref<FmodAudioStream> stream = get_stream();
		if (stream.is_null()) return;
		const Vector2i size = Vector2i(get_size());
		Rect2 rect{ Vector2(), Vector2(size) };
		const Ref<FmodAudioWaveform> waveform = get_waveform();
		if (waveform.is_valid() && waveform->get_length() > 0.0 && size.x > 0) {
			rect.size.y *= 0.5f;
			rect.position.y += rect.size.y;
		}
		const float scale = EditorInterface::get_singleton()->get_editor_scale();
		const Ref<Font> font = get_theme_font("font", "Label");
		const int font_size = get_theme_font_size("font_size", "Label");
		const float icon_width = resource_icon.is_valid() ? float(resource_icon->get_width()) : 0.0f;
		if (resource_icon.is_valid()) {
			draw_texture(resource_icon, Vector2(Vector2i(int(scale * 4.0f), int(rect.position.y + (rect.size.y - resource_icon->get_height()) * 0.5f))));
		}
		String text = stream->get_name();
		const String path = stream->get_path();
		if (text.is_empty()) text = !path.is_empty() && !path.contains("::") ? path.get_file() : stream->get_class();
		const float text_width = size.x - 4.0f * scale - icon_width;
		if (text_width > 0.0f) {
			const Vector2 position = Vector2(Vector2i(int(size.x - text_width), int(rect.position.y + font->get_ascent(font_size) + (rect.size.y - font->get_height(font_size)) * 0.5f)));
			draw_string(font, position, text, HORIZONTAL_ALIGNMENT_CENTER, text_width, font_size, get_theme_color("font_color", "Editor"));
		}
	}

	void decorate_fmod_audio_stream_property(EditorProperty* p_editor) {
		const TypedArray<Node> pickers = p_editor->find_children("*", "EditorResourcePicker", true, false);
		if (pickers.is_empty()) return;
		EditorResourcePicker* picker = Object::cast_to<EditorResourcePicker>(pickers[0]);
		for (int i = 0; i < picker->get_child_count(); i++) {
			Button* button = Object::cast_to<Button>(picker->get_child(i));
			if (!button || !button->get_h_size_flags().has_flag(Control::SIZE_EXPAND)) continue;
			// GDExtension 未公开 get_assign_button，按原生构造器的伸展标记查找
			// 保留普通 picker 的内部 TextureRect 引用，隐藏它以采用音频布局
			for (int child = 0; child < button->get_child_count(); child++) {
				TextureRect* texture = Object::cast_to<TextureRect>(button->get_child(child));
				if (texture) { texture->hide(); texture->set_mouse_filter(Control::MOUSE_FILTER_IGNORE); }
			}
			FmodAudioStreamPickerPreview* preview = memnew(FmodAudioStreamPickerPreview);
			preview->setup(picker, button);
			button->add_child(preview);
			button->move_child(preview, 0);
			return;
		}
	}

}
