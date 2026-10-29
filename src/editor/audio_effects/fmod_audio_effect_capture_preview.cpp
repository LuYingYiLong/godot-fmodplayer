#include "fmod_audio_effect_capture_preview.h"

#include "core/fmod_server.h"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace godot {
	void FmodAudioEffectCapturePreview::_bind_methods() {
	}

	FmodAudioEffectCapturePreview::FmodAudioEffectCapturePreview() {
		set_mouse_filter(MOUSE_FILTER_IGNORE);
		set_draw_label(false);
		set_draw_background(false);
		set_clip_contents(true);
		set_custom_minimum_size(Vector2(0.0f, 160.0f * _scale()));
		set_tooltip_text(tr("Peak ranges of unread stereo frames in this editor instance. Attach Capture to an FMOD bus to receive audio."));
		refresh_timer = memnew(Timer);
		refresh_timer->set_wait_time(0.1);
		refresh_timer->connect("timeout", callable_mp(this, &FmodAudioEffectCapturePreview::_refresh_snapshot));
		add_child(refresh_timer);
	}

	float FmodAudioEffectCapturePreview::_scale() const {
		EditorInterface* editor = EditorInterface::get_singleton();
		return editor ? editor->get_editor_scale() : 1.0f;
	}

	Rect2 FmodAudioEffectCapturePreview::_plot_rect() const {
		const float scale = _scale();
		return Rect2(Vector2(12.0f, 16.0f) * scale, Vector2(MAX(get_size().x - 24.0f * scale, 0.0f), MAX(get_size().y - 44.0f * scale, 0.0f)));
	}

	void FmodAudioEffectCapturePreview::set_effect(const Ref<FmodAudioEffectCapture>& p_effect) {
		if (effect == p_effect) return;
		_disconnect_effect();
		effect = p_effect;
		snapshot.clear();
		ranges.clear();
		last_capacity = last_frames = 0;
		last_written_frames = -1;
		if (is_inside_tree()) _connect_effect();
		_on_effect_changed();
		_update_refresh_timer();
	}

	void FmodAudioEffectCapturePreview::_connect_effect() {
		if (effect.is_valid() && !effect->is_connected("changed", callable_mp(this, &FmodAudioEffectCapturePreview::_on_effect_changed))) {
			effect->connect("changed", callable_mp(this, &FmodAudioEffectCapturePreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectCapturePreview::_disconnect_effect() {
		if (effect.is_valid() && effect->is_connected("changed", callable_mp(this, &FmodAudioEffectCapturePreview::_on_effect_changed))) {
			effect->disconnect("changed", callable_mp(this, &FmodAudioEffectCapturePreview::_on_effect_changed));
		}
	}

	void FmodAudioEffectCapturePreview::_on_effect_changed() {
		snapshot_dirty = true;
		_refresh_snapshot();
		queue_redraw();
	}

	void FmodAudioEffectCapturePreview::_update_refresh_timer() {
		if (!refresh_timer || !refresh_timer->is_inside_tree()) return;
		if (effect.is_valid() && is_visible_in_tree()) {
			if (refresh_timer->is_stopped()) refresh_timer->start();
			_refresh_snapshot();
		}
		else {
			refresh_timer->stop();
		}
	}

	void FmodAudioEffectCapturePreview::_notification(int p_what) {
		switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
			_connect_effect();
			// 检查器重挂接时不会再次触发 READY，等待 Timer 子节点入树后恢复刷新
			callable_mp(this, &FmodAudioEffectCapturePreview::_update_refresh_timer).call_deferred();
			break;
		case NOTIFICATION_READY:
		case NOTIFICATION_VISIBILITY_CHANGED:
			_update_refresh_timer();
			break;
		case NOTIFICATION_EXIT_TREE:
			if (refresh_timer) refresh_timer->stop();
			_disconnect_effect();
			break;
		case NOTIFICATION_THEME_CHANGED:
		case NOTIFICATION_RESIZED:
			_on_effect_changed();
			break;
		}
	}

	void FmodAudioEffectCapturePreview::_refresh_snapshot() {
		if (effect.is_null() || !is_inside_tree() || !is_visible_in_tree()) return;
		const size_t columns = size_t(CLAMP(int(_plot_rect().size.x / (16.0f * _scale())), 1, 10));
		const Ref<FmodSystem> system = FmodServer::get_main_system();
		const double sample_rate = system.is_valid() ? MAX(double(system->get_software_format().get("sample_rate", 48000)), 100.0) : 48000.0;
		size_t available;
		size_t capacity;
		int64_t written;
		{
			std::unique_lock<std::mutex> lock(effect->buffer_mutex, std::try_to_lock);
			if (!lock.owns_lock()) return;
			available = effect->ring_buffer.data_left();
			capacity = available + effect->ring_buffer.space_left();
			written = effect->pushed_frames.load() - effect->discarded_frames.load();
		}
		if (!snapshot_dirty && available == last_frames && capacity == last_capacity && written == last_written_frames && columns == ranges.size() && sample_rate == last_sample_rate) return;
		// 分配和峰值扫描都放在锁外，锁内只复制完整帧
		snapshot.resize(available);
		{
			std::unique_lock<std::mutex> lock(effect->buffer_mutex, std::try_to_lock);
			if (!lock.owns_lock()) return;
			available = effect->ring_buffer.peek(snapshot.data(), snapshot.size());
			capacity = effect->ring_buffer.data_left() + effect->ring_buffer.space_left();
			written = effect->pushed_frames.load() - effect->discarded_frames.load();
		}
		ranges.resize(columns);
		fmod_capture_preview::build_envelope(snapshot.data(), available, capacity, ranges);
		duration = double(capacity) / sample_rate;
		last_frames = available;
		last_capacity = capacity;
		last_written_frames = written;
		last_sample_rate = sample_rate;
		snapshot_dirty = false;
		queue_redraw();
	}

	void FmodAudioEffectCapturePreview::_update_property() {
		_on_effect_changed();
	}

	void FmodAudioEffectCapturePreview::_draw() {
		const Color background = get_theme_color("dark_color_2", "Editor");
		const Color accent = get_theme_color("accent_color", "Editor");
		const Color muted = get_theme_color("font_disabled_color", "Editor");
		draw_rect(Rect2(Vector2(), get_size()), background);
		const Rect2 plot = _plot_rect();
		const float scale = _scale();
		if (effect.is_null() || plot.size.x < 24.0f * scale || plot.size.y < 32.0f * scale || duration <= 0.0) return;
		const Ref<Font> font = get_theme_default_font();
		const int font_size = get_theme_default_font_size();
		if (last_frames == 0) {
			draw_string(font, Vector2(plot.position.x, plot.get_center().y + font->get_ascent(font_size) * 0.4f), tr("Waiting for captured audio"), HORIZONTAL_ALIGNMENT_CENTER, plot.size.x, font_size, muted);
			return;
		}
		const double step = fmod_capture_preview::time_step(duration, CLAMP(int(plot.size.x / (48.0f * scale)), 1, 10));
		const int decimals = CLAMP(int(std::ceil(-std::log10(step))), 0, 3);
		float label_end = -3.0f * scale;
		auto draw_tick = [&](double p_time) {
			const float x = plot.position.x + float(p_time / duration) * plot.size.x;
			draw_line(Vector2(x, plot.position.y), Vector2(x, plot.get_end().y), muted, scale);
			const String label = String::num(p_time, decimals) + "s";
			const float width = font->get_string_size(label, HORIZONTAL_ALIGNMENT_LEFT, -1.0f, font_size).x;
			const float start = CLAMP(x - width * 0.5f, 0.0f, MAX(get_size().x - width, 0.0f));
			if (start >= label_end + 3.0f * scale) {
				draw_string(font, Vector2(start, plot.get_end().y + 5.0f * scale + font->get_ascent(font_size)), label, HORIZONTAL_ALIGNMENT_LEFT, width, font_size, muted);
				label_end = start + width;
			}
			};
		const int ticks = int(std::floor(duration / step + 0.000001));
		for (int i = 0; i <= ticks; i++) draw_tick(MIN(double(i) * step, duration));
		if (duration - double(ticks) * step > 0.000001) draw_tick(duration);
		for (size_t i = 0; i < ranges.size(); i++) {
			const auto& range = ranges[i];
			if (!range.has_samples) continue;
			const float x = plot.position.x + float(i + 1) / float(ranges.size()) * plot.size.x;
			const float top = plot.position.y + (1.0f - float(CLAMP(range.maximum, -1.0, 1.0))) * 0.5f * plot.size.y;
			const float bottom = plot.position.y + (1.0f - float(CLAMP(range.minimum, -1.0, 1.0))) * 0.5f * plot.size.y;
			draw_line(Vector2(x, top), Vector2(x, bottom), accent, 2.0f * scale);
			draw_line(Vector2(x - 2.0f * scale, top), Vector2(x + 2.0f * scale, top), accent, 2.0f * scale);
			draw_line(Vector2(x - 2.0f * scale, bottom), Vector2(x + 2.0f * scale, bottom), accent, 2.0f * scale);
		}
	}
}
