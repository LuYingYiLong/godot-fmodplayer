# Changelog

## 2026-11-01 — Audio Bus Editing and Inspector Previews

This entry covers 35 commits after `a800d06c23894fe15699b12b2f9ffa2050e8fb32`, up to `2ef9a0e105f89bf5d0d9919d1e9ae392e6154696`. [Full comparison](https://github.com/LuYingYiLong/Godot-FmodPlayer/compare/a800d06c23894fe15699b12b2f9ffa2050e8fb32...2ef9a0e105f89bf5d0d9919d1e9ae392e6154696)

### FMOD Audio Dock and Bus Layouts

- Added the **FMOD Audio Dock** for editing independent FMOD bus layouts inside Godot, with bus creation, duplication, deletion, renaming, drag-and-drop ordering, and send routing.
- Added effect insertion, reordering, and transfers between buses, alongside volume, mute, solo, bypass, and effect-slot controls. Layout editing supports undo/redo and automatic saving.
- Added layout loading, Save As, default-layout selection, conversion from Godot buses, and export dependency collection for the default layout.
- Separated serializable bus configuration from runtime channel groups and DSP instances. Buses and effect slots now use stable IDs that survive reordering. Routing allows any destination that does not create a cycle. Shared effect configurations receive a separate runtime instance in each slot.
- Added Godot bus synchronization and independent FMOD layout modes. Synchronization is enabled by default through `audio/fmod/sync_godot_audio_buses`. Independent mode uses `audio/fmod/default_bus_layout`. Changing modes requires reloading the project.
- Added bus meters with peak hold. Meters display audio played inside the editor and stop polling while the Dock is hidden.
- Player Bus selectors now follow the FMOD layout and refresh when its bus list changes. Changing a player's bus during playback updates its routing; missing buses fall back to Master.

### Effect and Audio Resource Previews

- Added **15 effect preview types**: Panner, EQ, Filter, Compressor, Amplify, Delay, PitchShift, Phaser, StereoEnhance, Distortion, HardLimiter, Chorus, Reverb, Capture, and SpectrumAnalyzer.
- Added direct parameter editing in supported preview graphs, including EQ bands and Q, filter cutoff, compressor threshold and ratio, and delay tap timing and pan. Edits stay synchronized with Inspector properties and support undo/redo.
- Added live Capture and SpectrumAnalyzer views using effect runtime data from editor playback.
- Expanded waveform previews and player `stream` property editing with inline playback, seeking, drag-and-drop, and resource actions. Waveform generation and caching are shared and refresh when audio resources change.
- Updated audio preview controls to respect editor scaling and improved their layout and interaction.
- Added resource change notifications across effect setters and reduced redundant refreshes when values remain unchanged, keeping previews, runtime instances, and saved layouts in sync.
- Shared distortion shaping logic between audio processing and its preview so the displayed curve follows the actual processing.

### Playback and Stability Fixes

- FMOD listener 0 now follows the main Viewport's current `AudioListener3D`, falling back to its current `Camera3D` when no explicit listener is active. Listener changes, movement, and removal are reflected automatically, and camera fallback uses the camera's actual transform.
- Updated 2D camera position and rotation independently of 3D camera availability.
- Corrected the conversion from 3D attenuation-filter decibels to FMOD filter strength; `0 dB` preserves the original signal. Disabled distance attenuation now uses a flat rolloff curve.
- Corrected 3D emission-angle units and ranges, and added attenuation-mode validation.
- Fixed HardLimiter retaining attenuation after its release phase completed.
- Hardened DSP connection mix-matrix channel, stride, and buffer-size validation. Corrected handling of actual matrix dimensions and output padding to prevent out-of-bounds access and integer overflow.
- Fixed cached sound destruction accessing native handles after their FMOD System had been released or reset. Improved audio-data updates, clearing, and sound-cache invalidation.
- Fixed `StringName` conversions in bus code and geometry/gizmo compilation errors caused by Godot 4.7 math constant names.

### Builds, Android, and Tests

- Builds now explicitly select `api_version=4.7`. The extension's declared minimum Godot version changed from `4.1` to **4.7.2**. The required FMOD Core SDK version is **2.04**.
- Added SCons `copy_to_tests=yes` to synchronize the complete addon into the test project, including script-only and translation-only changes. Windows and Linux builds also copy the local FMOD runtime. `fmod_runtime` can override its source, and synchronization writes a report with file hashes.
- Updated `ModifyAndroidBuildTemplatePlugin` for newer Gradle and Godot Java template structures. Debug and Release builds load `fmodL` and `fmod` respectively. Repeated updates avoid duplicate initialization blocks, and copy/write failures are reported.
- Updated Windows x86_64 and Android arm64 debug binaries, and rebuilt Linux x86_64 Debug and Release binaries.
- The current Linux prebuilt libraries were built on Ubuntu 26.04 and require **glibc 2.43**. Deployments targeting older distributions need libraries rebuilt in a compatible build environment.
- Reorganized testing around the Godot project's **F5 menu**, with spatial audio/reverb/geometry listening, FMOD versus Godot 2D playback comparison, 3D Doppler listening, and regression report access.
- Consolidated eight numerical regression suites behind the menu: spatial audio, mix matrices, buses, spectrum snapshots, SoundGroup, SDK integration, capture, and reverb. Inspector previews, bus editing, and gizmos are checked directly in the editor.
- Added a Doppler listening scene with adjustable speed and a Doppler toggle, plus effect preview resources. Reorganized audio assets and included [CC BY 4.0 attribution for the train sound](tests/assets/audio/LICENSE.txt).
- Removed obsolete build batch files, standalone command-line test entry points, diagnostic tools, and project files. See the [contribution guide](CONTRIBUTING.md) and [test project guide](tests/README.md) for the current workflow.

### Localization and Documentation

- Property and editor UI translations now load directly from CSV catalogs, replacing generated GDScript translation data. Catalogs cover Simplified Chinese, Traditional Chinese, Japanese, and Russian.
- Added a unified translation editor for properties, editor UI, and API help, with search, untranslated/review filters, context, multiline text, source locations, and PO review flags.
- Added Simplified Chinese API help inside Godot using a catalog shared with the documentation website. Revised XML API descriptions and updated the C# bus wrappers.
- Refreshed the README feature presentation, banner, node icons, contribution guide, and testing instructions. Removed outdated documentation links and assets.

### Bus API Migration

`FmodAudioBusLayout` now stores configuration only. Bus IDs and effect-slot IDs are separate from array indices; Master always has bus ID `0`. Existing code that addresses layout operations by bus name or effect index needs updating.

| Previous API | Current API |
|--------------|-------------|
| `layout.sync_from_audio_server()` / `sync_from_audio_server_if_changed()` | The server synchronizes automatically in Godot sync mode. Call `FmodServer.generate_bus_layout()` to request a refresh. |
| `layout.get_audio_bus(name)` | `FmodServer.get_audio_bus(name)`, or `get_audio_bus_by_id(bus_id)` for lookup by ID. |
| `layout.has_audio_bus(name)` | `layout.find_bus_id(name) != -1`. |
| `layout.create_audio_bus(name)` | `layout.add_bus(name)`, which returns a stable bus ID. |
| `layout.remove_audio_bus(name)` | `layout.remove_bus(bus_id)`. |
| `layout.set_bus_volume(name, db)` / `get_bus_volume(name)` | `layout.set_bus_volume_db(bus_id, db)` / `get_bus_volume_db(bus_id)`. |
| `layout.bus_is_mute(name)` / `bus_is_solo(name)` / `bus_is_bypass(name)` | `layout.is_bus_mute(bus_id)` / `is_bus_solo(bus_id)` / `is_bus_bypass(bus_id)`. |
| `layout.set_bus_mute(name, value)` / `set_bus_solo(...)` / `set_bus_bypass(...)` | Method names remain; the first argument is now a stable bus ID. |
| `layout.get_bus_effect_count(name)` | `layout.get_bus_effect_ids(bus_id).size()`. |
| `layout.add_bus_effect(name, effect, index)` | `layout.add_bus_effect(bus_id, effect, index)`, which returns a stable slot ID and appends by default. |
| `layout.get_bus_effect(name, index)` / `remove_bus_effect(name, index)` | Use `(bus_id, slot_id)`. `get_bus_effect()` returns the configuration resource. |
| Reading Capture, SpectrumAnalyzer, or Record data from the layout's effect resource | Read from the runtime instance returned by `FmodServer.get_bus_effect_instance(bus_id, slot_id)`. |

In independent mode, configuration changes are applied on the next server update. Call `FmodServer.flush_bus_updates()` when a new bus or effect slot must be available immediately. This method does not poll Godot AudioServer; use `generate_bus_layout()` for an explicit Godot-to-FMOD synchronization refresh.

Native libraries remain configured with `reloadable=false`. Reopen the Godot project after replacing a binary.
