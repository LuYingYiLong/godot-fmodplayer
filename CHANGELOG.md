# Changelog

## 2026-11-01 — Audio Bus Editing and Inspector Previews

Changes from `a800d06` to `2ef9a0e` (35 commits). [Full comparison](https://github.com/LuYingYiLong/Godot-FmodPlayer/compare/a800d06c23894fe15699b12b2f9ffa2050e8fb32...2ef9a0e105f89bf5d0d9919d1e9ae392e6154696)

Requires **Godot 4.7.2+** and **FMOD Core SDK 2.04**. Current Linux prebuilts require **glibc 2.43**; older distributions need a compatible rebuild.

### Highlights

- Added the **FMOD Audio Dock** with bus routing, drag-and-drop effects, meters, undo/redo, and automatic layout saving. Supports Godot bus synchronization and independent FMOD layouts.
- Added **15 effect preview types**, including interactive EQ, filter, compressor, and delay controls, plus live Capture and SpectrumAnalyzer views.
- Expanded waveform previews with inline playback, seeking, drag-and-drop, and editor scaling support.
- Added a unified translation editor and Simplified Chinese API help inside Godot.
- Added an F5 test menu, playback comparison and Doppler scenes, and SCons `copy_to_tests=yes` for addon deployment.

### Important Fixes

- FMOD listener 0 now follows the main Viewport's current `AudioListener3D`, with `Camera3D` fallback. 2D camera tracking updates independently.
- Corrected 3D attenuation-filter strength, disabled-distance rolloff, and emission-angle units and ranges.
- Fixed HardLimiter retaining attenuation after release and cached sounds accessing invalid handles after FMOD System release or reset.
- Hardened DSP mix-matrix dimension, stride, and buffer validation to prevent out-of-bounds access and integer overflow.
- Updated Android template handling for newer Gradle/Java structures, correct Debug/Release runtime loading, repeatable updates, and copy/write error reporting.

### API Migration

`FmodAudioBusLayout` now stores configuration only. Bus and effect-slot IDs are stable and separate from array indices; Master has bus ID `0`. Bus setters now take IDs instead of names.

| Previous API | Replacement |
|--------------|-------------|
| `layout.sync_from_audio_server()` / `sync_from_audio_server_if_changed()` | Automatic in Godot sync mode; explicitly refresh with `FmodServer.generate_bus_layout()`. |
| `layout.get_audio_bus(name)` | `FmodServer.get_audio_bus(name)` or `get_audio_bus_by_id(bus_id)`. |
| `layout.has_audio_bus(name)` | `layout.find_bus_id(name) != -1`. |
| `create_audio_bus(name)` / `remove_audio_bus(name)` | `add_bus(name)` / `remove_bus(bus_id)`. |
| `set_bus_volume()` / `get_bus_volume()` | `set_bus_volume_db()` / `get_bus_volume_db()`, using bus IDs. |
| `bus_is_mute()` / `bus_is_solo()` / `bus_is_bypass()` | `is_bus_mute()` / `is_bus_solo()` / `is_bus_bypass()`, using bus IDs. |
| `layout.get_bus_effect_count(name)` | `layout.get_bus_effect_ids(bus_id).size()`. |
| `layout.add_bus_effect(name, effect, index)` | Pass `bus_id`; the method returns a stable `slot_id` and appends by default. |
| `layout.get_bus_effect(name, index)` / `remove_bus_effect(name, index)` | Use `(bus_id, slot_id)`. `get_bus_effect()` returns configuration. |

Read live Capture, SpectrumAnalyzer, and Record data through `FmodServer.get_bus_effect_instance(bus_id, slot_id)`.

Godot bus synchronization is enabled by default. For independent layout editing, disable `audio/fmod/sync_godot_audio_buses` and reload the project. Configuration changes apply on the next server update; `FmodServer.flush_bus_updates()` applies them immediately without polling Godot AudioServer.

Reopen the Godot project after replacing native libraries (`reloadable=false`).
