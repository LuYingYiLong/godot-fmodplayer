// Meter presentation follows Godot's editor_audio_buses.cpp (MIT license).
// Copyright (c) Godot Engine contributors. See LICENSE.txt for the MIT terms.
#include "fmod_audio_buses_editor.h"
#include "core/fmod_server.h"
#include "mixer/fmod_audio_bus.h"
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <algorithm>
#include <cmath>
#include <iterator>

namespace godot {

	static float meter_scale() {
		return EditorInterface::get_singleton()->get_editor_scale();
	}

	void FmodAudioBusMeter::_bind_methods() {
	}

	void FmodAudioBusMeter::setup(int64_t id) {
		bus_id = id;
		set_mouse_filter(MOUSE_FILTER_PASS);
		active_gradient.instantiate();
		inactive_gradient.instantiate();
		PackedFloat32Array offsets;
		offsets.push_back(0.0f);
		offsets.push_back(0.72f);
		offsets.push_back(0.74f);
		active_gradient->set_offsets(offsets);
		inactive_gradient->set_offsets(offsets);
		active_texture.instantiate();
		inactive_texture.instantiate();
		active_texture->set_gradient(active_gradient);
		inactive_texture->set_gradient(inactive_gradient);
		for (const auto& texture : {active_texture, inactive_texture}) {
			texture->set_fill_from(Vector2(0.0f, 1.0f));
			texture->set_fill_to(Vector2(0.0f, 0.0f));
		}
		_resize_channels(2);
		_update_colors();
	}

	FmodAudioBusMeter::~FmodAudioBusMeter() {
		_release_meter();
	}

	void FmodAudioBusMeter::_release_meter() {
		if (metering && FmodServer::get_main_system().is_valid() && !FmodServer::get_main_system()->system_is_null()) {
			bool input = false, output = false;
			if (metering->getMeteringEnabled(&input, &output) == FMOD_OK) {
				// Input metering is owned by other consumers; only restore our output flag.
				metering->setMeteringEnabled(input, previous_output);
			}
		}
		metering = nullptr;
	}

	void FmodAudioBusMeter::_resize_channels(int count) {
		if (int(channels.size()) == count) return;
		while (int(channels.size()) > count) {
			auto bar = channels.back().bar;
			remove_child(bar);
			bar->queue_free();
			channels.pop_back();
		}
		while (int(channels.size()) < count) {
			Channel channel;
			channel.bar = memnew(TextureProgressBar);
			channel.bar->set_name("Channel" + String::num_int64(channels.size()));
			channel.bar->set_fill_mode(TextureProgressBar::FILL_BOTTOM_TO_TOP);
			channel.bar->set_custom_minimum_size(Vector2(16.0f, 128.0f) * meter_scale());
			channel.bar->set_progress_texture(active_texture);
			channel.bar->set_under_texture(active_texture);
			channel.bar->set_over_texture(inactive_texture);
			channel.bar->set_min(0.0);
			channel.bar->set_max(1.0);
			channel.bar->set_step(0.0001);
			add_child(channel.bar);
			channel.peak = memnew(Panel);
			channel.peak->set_name("Peak");
			channel.peak->set_custom_minimum_size(Vector2(16.0f * meter_scale(), 2.0f));
			channel.peak_style.instantiate();
			channel.peak->add_theme_stylebox_override("panel", channel.peak_style);
			channel.bar->add_child(channel.peak);
			channel.peak->set_position(Vector2(0.0f, 128.0f * meter_scale() - 2.0f));
			channel.peak->hide();
			channel.hold = memnew(Timer);
			channel.hold->set_name("PeakHold");
			channel.hold->set_wait_time(1.5);
			channel.hold->set_one_shot(true);
			channel.hold->set_paused(true);
			channel.peak->add_child(channel.hold);
			channel.hold->connect("timeout", callable_mp(this, &FmodAudioBusMeter::_enable_indicator_fall));
			channels.push_back(channel);
		}
		_update_colors();
	}

	void FmodAudioBusMeter::_update_colors() {
		if (active_gradient.is_null()) return;
		auto settings = EditorInterface::get_singleton()->get_editor_settings();
		PackedColorArray active, inactive;
		for (const char* suffix : {"min_db_color", "normalized_db_color", "max_db_color"}) {
			active.push_back(settings->get_setting(String("editors/audio_buses/active_") + suffix));
			inactive.push_back(settings->get_setting(String("editors/audio_buses/inactive_") + suffix));
		}
		active_gradient->set_colors(active);
		inactive_gradient->set_colors(inactive);
		for (const auto& texture : {active_texture, inactive_texture}) {
			texture->set_width(int(16.0f * meter_scale()));
			texture->set_height(int(128.0f * meter_scale()));
		}
		for (auto& channel : channels) {
			channel.bar->set_custom_minimum_size(Vector2(16.0f, 128.0f) * meter_scale());
			channel.bar->set_tint_under(settings->get_setting("editors/audio_buses/tint_under_color"));
			channel.bar->set_tint_over(settings->get_setting("editors/audio_buses/tint_over_color"));
			channel.peak->set_custom_minimum_size(Vector2(16.0f * meter_scale(), 2.0f));
		}
	}

	void FmodAudioBusMeter::_reset_levels() {
		pending_valid = pending_active = last_active = false;
		last_valid_ticks = 0;
		std::fill(std::begin(pending_peaks), std::end(pending_peaks), 0.0f);
		std::fill(std::begin(last_peaks), std::end(last_peaks), 0.0f);
		for (auto& channel : channels) {
			channel.level_db = -100.0f;
			channel.bar->set_value(0.0);
			channel.bar->set_over_texture(inactive_texture);
			channel.peak->set_position(Vector2(0.0f, 128.0f * meter_scale() - 2.0f));
			channel.peak->hide();
			channel.hold->stop();
			channel.hold->set_paused(true);
			channel.falling = false;
		}
	}

	void FmodAudioBusMeter::_notification(int what) {
		if (what == NOTIFICATION_READY) {
			auto settings = EditorInterface::get_singleton()->get_editor_settings();
			const auto changed = callable_mp(this, &FmodAudioBusMeter::_update_colors);
			if (!settings->is_connected("settings_changed", changed)) settings->connect("settings_changed", changed);
			_update_colors();
		}
		if (what == NOTIFICATION_THEME_CHANGED) _update_colors();
		if (what == NOTIFICATION_READY || what == NOTIFICATION_VISIBILITY_CHANGED) {
			const bool visible = is_visible_in_tree();
			set_process(visible);
			if (!visible) _release_meter();
			_reset_levels();
		}
		if (what == NOTIFICATION_EXIT_TREE) {
			set_process(false);
			_release_meter();
		}
		if (what == NOTIFICATION_PROCESS) _collect_metering();
	}

	void FmodAudioBusMeter::_collect_metering() {
		auto runtime = FmodServer::get_audio_bus_by_id(bus_id);
		FMOD::DSP* head = nullptr;
		if (runtime.is_valid() && runtime->get_bus().is_valid() && runtime->get_bus()->channel_group) {
			runtime->get_bus()->channel_group->getDSP(FMOD_CHANNELCONTROL_DSP_HEAD, &head);
		}
		if (head != metering) {
			_release_meter();
			if (head && head->getMeteringEnabled(&previous_input, &previous_output) == FMOD_OK) metering = head;
		}
		if (!metering) return;
		bool input = false, output = false;
		if (metering->getMeteringEnabled(&input, &output) != FMOD_OK) return;
		if (!output && metering->setMeteringEnabled(input, true) != FMOD_OK) return;
		bool idle = false;
		const bool inactive = metering->getIdle(&idle) == FMOD_OK && idle;
		FMOD_DSP_METERING_INFO info{};
		const FMOD_RESULT result = metering->getMeteringInfo(nullptr, &info);
		// New, replaced or temporarily unavailable DSPs must not reset channels or
		// turn a failed read into a silent sample.
		if (!inactive && (result != FMOD_OK || info.numsamples <= 0 || info.numchannels <= 0)) return;
		if (result == FMOD_OK && info.numchannels > 0) _resize_channels(MAX(2, MIN(8, int(info.numchannels))));
		for (int i = 0; i < int(channels.size()); i++) {
			const float peak = !inactive && i < info.numchannels && std::isfinite(info.peaklevel[i]) ? MAX(0.0f, info.peaklevel[i]) : 0.0f;
			pending_peaks[i] = MAX(pending_peaks[i], peak);
			last_peaks[i] = peak;
		}
		pending_valid = true;
		pending_active |= !inactive;
		last_active = !inactive;
		last_valid_ticks = Time::get_singleton()->get_ticks_usec();
	}

	void FmodAudioBusMeter::_enable_indicator_fall() {
		for (auto& channel : channels) {
			if (channel.hold->get_time_left() <= 0.0 && !channel.falling) channel.falling = true;
		}
	}

	void FmodAudioBusMeter::sample(float delta) {
		if (!is_visible_in_tree()) return;
		const uint64_t now = Time::get_singleton()->get_ticks_usec();
		const bool recent = last_valid_ticks && now - last_valid_ticks <= 100000;
		const bool active = pending_valid ? pending_active : recent && last_active;
		const float range = 128.0f * meter_scale() - 2.0f;
		for (int i = 0; i < int(channels.size()); i++) {
			auto& channel = channels[i];
			const float peak = pending_valid ? pending_peaks[i] : recent ? last_peaks[i] : 0.0f;
			const float db = peak > 0.00001f ? 20.0f * std::log10(peak) : -100.0f;
			if (db >= channel.level_db) channel.level_db = db;
			else channel.level_db -= delta * 60.0f;
			channel.bar->set_value(fmod_bus_db_to_position(channel.level_db));
			float y = channel.peak->get_position().y;
			if (!channel.falling) y = MIN(y, range - float(channel.bar->get_value()) * range);
			else y = MIN(range, y + delta * 60.0f);
			channel.peak->set_position(Vector2(0.0f, y));
			const float normalized = (y - range) / -range;
			if (channel.bar->get_value() > normalized || std::abs(float(channel.bar->get_value()) - normalized) <= 0.01f) {
				channel.hold->set_paused(true);
				channel.falling = false;
			} else if (channel.hold->is_paused()) {
				channel.hold->set_paused(false);
				channel.hold->start();
			}
			channel.peak_style->set_bg_color(active_gradient->sample(normalized));
			channel.bar->set_over_texture(active ? Ref<Texture2D>() : inactive_texture);
			channel.peak->set_visible(active || y <= range - 2.0f);
		}
		std::fill(std::begin(pending_peaks), std::end(pending_peaks), 0.0f);
		pending_valid = pending_active = false;
	}
}
