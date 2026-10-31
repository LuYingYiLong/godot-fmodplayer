#include "editor/fmod_editor_translation.h"
#include "fmod_audio_effect_panner_preview.h"

#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace godot {
	namespace {
		float get_preview_scale() {
			EditorInterface* editor = EditorInterface::get_singleton();
			return editor ? editor->get_editor_scale() : 1.0f;
		}
	}

	void FmodAudioEffectPannerPreview::_bind_methods() {
	}

	FmodAudioEffectPannerPreview::FmodAudioEffectPannerPreview() {
		set_mouse_filter(MOUSE_FILTER_PASS);
		set_default_cursor_shape(CURSOR_HSIZE);
		set_draw_label(false);
		set_draw_background(false);
		set_clip_contents(true);
		set_custom_minimum_size(Vector2(0.0f, 160.0f * get_preview_scale()));
		fmod_set_editor_tooltip(this, "Click or drag horizontally to adjust pan: left (-1), center (0), right (1).");
	}

	void FmodAudioEffectPannerPreview::set_effect(const Ref<FmodAudioEffectPanner>& p_effect) {
		if (effect == p_effect) {
			return;
		}
		dragging = false;
		_disconnect_effect();
		effect = p_effect;
		if (is_inside_tree()) {
			_connect_effect();
		}
		queue_redraw();
	}

	void FmodAudioEffectPannerPreview::_connect_effect() {
		if (effect.is_valid() && !effect->is_connected("changed", callable_mp(this, &FmodAudioEffectPannerPreview::_on_effect_changed))) {
			effect->connect("changed", callable_mp(this, &FmodAudioEffectPannerPreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectPannerPreview::_disconnect_effect() {
		if (effect.is_valid() && effect->is_connected("changed", callable_mp(this, &FmodAudioEffectPannerPreview::_on_effect_changed))) {
			effect->disconnect("changed", callable_mp(this, &FmodAudioEffectPannerPreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectPannerPreview::_on_effect_changed() {
		queue_redraw();
	}

	void FmodAudioEffectPannerPreview::_notification(int p_what) {
		switch (p_what) {
		case NOTIFICATION_TRANSLATION_CHANGED:
			fmod_refresh_editor_control(this);
			_on_effect_changed();
			break;
		case NOTIFICATION_ENTER_TREE:
			_connect_effect();
			break;
		case NOTIFICATION_EXIT_TREE:
			dragging = false;
			_disconnect_effect();
			break;
		case NOTIFICATION_THEME_CHANGED:
			queue_redraw();
			break;
		case NOTIFICATION_RESIZED:
		case NOTIFICATION_VISIBILITY_CHANGED:
			queue_redraw();
			break;
		}
	}

	float FmodAudioEffectPannerPreview::_get_plot_inset() const {
		return 14.0f * get_preview_scale(); // 绘图区边距与指示点半径之和
	}

	void FmodAudioEffectPannerPreview::_edit_pan_at(float p_x, bool p_changing) {
		const float inset = _get_plot_inset();
		const float width = get_size().x - 2.0f * inset;
		if (effect.is_null() || width <= 0.0f || is_read_only()) {
			return;
		}
		const float pan = CLAMP((p_x - inset) / width, 0.0f, 1.0f) * 2.0f - 1.0f;
		if (pan != effect->get_pan() || (!p_changing && gesture_changed)) {
			gesture_changed = true;
			// 由检查器处理属性同步、资源修改标记和连续编辑的撤销合并
			emit_changed("pan", pan, StringName(), p_changing);
		}
	}

	void FmodAudioEffectPannerPreview::_gui_input(const Ref<InputEvent>& p_event) {
		if (effect.is_null() || is_read_only()) {
			return;
		}
		Ref<InputEventMouseButton> button = p_event;
		if (button.is_valid() && button->get_button_index() == MOUSE_BUTTON_LEFT) {
			if (button->is_pressed()) {
				dragging = true;
				gesture_changed = false;
				_edit_pan_at(button->get_position().x, true);
			} else if (dragging) {
				dragging = false;
				if (!button->is_canceled()) {
					_edit_pan_at(button->get_position().x, false);
				}
			}
			accept_event();
			return;
		}
		Ref<InputEventMouseMotion> motion = p_event;
		if (motion.is_valid() && dragging) {
			if (motion->get_button_mask().has_flag(MOUSE_BUTTON_MASK_LEFT)) {
				_edit_pan_at(motion->get_position().x, true);
			} else {
				dragging = false;
			}
			accept_event();
		}
	}

	void FmodAudioEffectPannerPreview::_update_property() {
		queue_redraw();
	}

	void FmodAudioEffectPannerPreview::_set_read_only(bool p_read_only) {
		dragging = false;
		set_default_cursor_shape(p_read_only ? CURSOR_ARROW : CURSOR_HSIZE);
		queue_redraw();
	}

	void FmodAudioEffectPannerPreview::_draw() {
		const Vector2 size = get_size();
		if (size.x <= 0.0f || size.y <= 0.0f) {
			return;
		}
		const Color background = get_theme_color("dark_color_2", "Editor");
		const Color accent = get_theme_color("accent_color", "Editor");
		const Color text = get_theme_color("font_color", "Editor");
		Color fill = get_theme_color("accent_color", "Editor");
		fill.a = 0.25f;
		const Ref<Font> font = get_theme_default_font();
		const int font_size = get_theme_default_font_size();
		const float scale = get_preview_scale();
		const float padding = 8.0f * scale;
		const float radius = 4.0f * scale;
		const float font_height = font->get_height(font_size);
		const float ascent = font->get_ascent(font_size);
		draw_rect(Rect2(Vector2(), size), background);
		if (effect.is_null() || size.x <= 2.0f * _get_plot_inset()) {
			return;
		}

		const float left = _get_plot_inset();
		const float right = size.x - _get_plot_inset();
		const float top = padding + font_height + 8.0f * scale;
		const float bottom = size.y - padding - font_height - 8.0f * scale;
		const float center_x = (left + right) * 0.5f;
		const float center_y = (top + bottom) * 0.5f;
		const float pan = effect->get_pan();
		const float indicator_x = Math::lerp(left, right, (pan + 1.0f) * 0.5f);

		const float fill_height = MAX(bottom - top, 0.0f);
		if (pan > 0.0f) {
			draw_rect(Rect2(Point2(center_x, top), Size2(right - center_x, fill_height)), fill);
		}
		else if (pan < 0.0f) {
			draw_rect(Rect2(Point2(left, top), Size2(center_x - left, fill_height)), fill);
		}
		draw_circle(Vector2(indicator_x, center_y), radius, accent, true, -1.0f, true);
		draw_string(font, Vector2(padding, padding + ascent), "L", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, text);
		draw_string(font, Vector2(size.x - padding - font->get_string_size("R", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size).x, padding + ascent), "R", HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size, text);
		const String value = String::num(pan, 2);
		draw_string(font, Vector2(left, size.y - padding - font_height + ascent), value, HORIZONTAL_ALIGNMENT_CENTER, right - left, font_size, text);
	}
}
