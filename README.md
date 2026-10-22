<p align="center">
  <img alt="fmod_player_banner" src="https://github.com/user-attachments/assets/1d2655f5-d440-4bdb-978a-b095cb275e9c" />
</p>

<h1 align="center">Godot-FmodPlayer</h1>

<p align="center">
  <b>FMOD Core API audio playback, mixing, DSP, and 3D spatial tools for Godot 4.</b>
</p>

<p align="center">
  <a href="https://github.com/LuYingYiLong/Godot-FmodPlayer/releases">
    <img src="https://img.shields.io/github/v/release/LuYingYiLong/Godot-FmodPlayer?include_prereleases&style=flat-square" alt="Release" />
  </a>
  <a href="#platform-support">
    <img src="https://img.shields.io/badge/platform-Windows%20%7C%20Android-blue?style=flat-square" alt="Platform" />
  </a>
  <a href="LICENSE.txt">
    <img src="https://img.shields.io/github/license/LuYingYiLong/Godot-FmodPlayer?style=flat-square" alt="License" />
  </a>
  <a href="https://godotengine.org">
    <img src="https://img.shields.io/badge/Godot-4.1%2B-478CBF?style=flat-square&logo=godot-engine" alt="Godot Version" />
  </a>
  <a href="https://godot-fmodplayerdocs.readthedocs.io/en/latest/">
    <img src="https://img.shields.io/badge/docs-online-2ea44f?style=flat-square" alt="Documentation" />
  </a>
</p>

<p align="center">
  A Godot 4 GDExtension plugin for developers who want low-level FMOD playback<br>
  without giving up Godot-style nodes, resources, buses, inspectors, and editor workflows.
</p>

---

## Why This Plugin?

Godot-FmodPlayer is built around the **FMOD Core API**, not FMOD Studio. It is aimed at projects that need direct control over sounds, channels, channel groups, DSP graphs, streamed audio, 3D spatial playback, and runtime mixing.

Use it when you want:

- FMOD's mature decoder, mixer, and DSP runtime inside Godot.
- Godot-friendly nodes such as `FmodAudioStreamPlayer`, `FmodAudioStreamPlayer2D`, and `FmodAudioStreamPlayer3D`.
- Editor tools for importing, previewing, inspecting, and testing FMOD audio resources.
- A bus layout that follows Godot's `AudioServer` while routing playback through FMOD.
- Advanced 3D audio features such as geometry occlusion and spatial reverb zones.

> Looking for FMOD Studio event/bank workflows? This project intentionally focuses on **FMOD Core API**. For Studio API integration, see [fmod-gdextension](https://github.com/utopia-rise/fmod-gdextension).

## Highlights

### Godot-Native Workflow

- Import audio as FMOD stream resources directly in the editor.
- Preview FMOD audio resources from the inspector.
- Use custom icons, 3D gizmos, and editor tools for FMOD nodes.
- Configure FMOD system options from `Project Settings`.
- Use a helper tool to prepare Android export templates for FMOD runtime files.

### Playback And Loading

- Play common FMOD-supported formats: MP3, WAV, OGG Vorbis, FLAC, AIFF, MIDI, MOD, XM, S3M, IT, and more.
- Load from `res://`, `user://`, absolute file paths, memory buffers, and Godot PCK resources.
- Choose streaming or sample loading modes depending on memory and latency needs.
- Use advanced stream settings for special formats, subsounds, tracks, and FMOD create flags.

### Mixing And Routing

- Synchronize FMOD buses with Godot's audio bus layout.
- Route players to named buses.
- Use channel groups and sound groups for grouped playback control.
- Adjust volume, pitch, pan, mute, solo, bypass, and DSP parameters at runtime.
- Monitor FMOD CPU and file usage in Godot's Performance panel.

### DSP Effects

Godot-FmodPlayer includes FMOD-backed effects designed to feel familiar to Godot users:

| Effect | Use Case |
|--------|----------|
| Amplify | Gain and volume adjustment |
| Filter | Low-pass, high-pass, band-pass filtering |
| EQ / EQ6 / EQ10 / EQ21 | Graphic and multi-band equalization |
| Chorus | Modulated stereo thickening |
| Delay | Echo, taps, and feedback delay |
| Distortion | Drive, clipping, and saturation |
| Phaser | Sweeping phase modulation |
| Pitch Shift | Real-time pitch manipulation |
| Reverb | Godot-like room reverb mapping |
| Compressor | Dynamic range compression |
| Hard Limiter | Peak control |
| Stereo Enhance | Stereo width shaping |
| Panner | Stereo positioning |
| Spectrum Analyzer | Real-time frequency visualization |
| Capture | Runtime sample capture |
| Record | Bus recording and waveform snapshots |

### 3D Audio

- `FmodAudioStreamPlayer3D` for spatial playback.
- Listener updates from the active Godot camera.
- Distance attenuation and rolloff controls.
- `FmodGeometryInstance3D` for FMOD geometry occlusion.
- MeshInstance3D editor tools for quickly creating FMOD geometry.
- `FmodReverbZone3D` for localized environmental reverb.
- Editor gizmos for spatial audio debugging.

## Quick Start

### 1. Install The Plugin

Recommended: install Godot-FmodPlayer from the Godot Asset Library:

**[Godot Asset Library - Godot-FmodPlayer](https://godotengine.org/asset-library/asset/4905)**

You can also download a release from GitHub or build the GDExtension from source if you need the latest development version.

### 2. Enable The Plugin

Open your Godot project and enable:

```text
Project > Project Settings > Plugins > FMOD Player
```

### 3. Play Audio

Add one of the player nodes:

- `FmodAudioStreamPlayer`
- `FmodAudioStreamPlayer2D`
- `FmodAudioStreamPlayer3D`

Assign an FMOD audio stream resource, choose a bus, and press play.

## Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| Windows x86_64 | Supported | Main development target |
| Android arm64 / arm32 / x86 / x86_64 | Supported | Requires FMOD Android runtime setup |
| macOS | Experimental | Build script support exists, FMOD linking may need local adjustment |
| iOS | Experimental | Static library workflow may need project-specific setup |

## Requirements

- **Godot:** 4.1 or later
- **Language:** C++17 or newer
- **Windows compiler:** Visual Studio 2022 / MSVC v145+
- **Android compiler:** Android NDK
- **Build system:** SCons
- **Audio runtime:** FMOD Core API SDK

## Building From Source

### Windows SDK Setup

Copy the FMOD Core Windows SDK's `inc` folder to `src/thirdparty/fmod/inc`
and its `lib/x64` files to `src/thirdparty/fmod/lib/x64`. The headers, import
library, and runtime DLL must come from the same SDK release. The build does
not bundle `fmod.dll`; users obtain it from their own FMOD SDK and place it in
their project's `addons/fmod_player/bin` folder for local use.

Run `scons platform=windows target=template_debug -j16` or use
`target=template_release` for Release. FMOD 2.04's DSP API is supported through
an adapter that preserves the existing effect processing and script callbacks.

### Building Android From Source On Windows

1. Install an Android SDK and NDK. Copy `custom.example.py` to `custom.py`,
   then set `ANDROID_HOME` to the SDK root and `ndk_version` to the version
   actually installed in its `ndk` folder. SCons reads this local configuration
   even when an existing terminal has not picked up new environment variables.
2. Extract the **FMOD Android SDK** and set `fmod_android_sdk` in `custom.py`
   to its root folder or `api/core` folder. Use the Android SDK's matching
   headers and native libraries; the Windows SDK alone is insufficient.
   The default layout is `src/thirdparty/fmod/android/inc` and
   `src/thirdparty/fmod/android/lib/<ABI>/libfmod.so` / `libfmodL.so`.
3. Run `build_android.bat` to build arm64 Debug and Release libraries.
   For another architecture, run e.g. `build_android.bat arch=x86_64` and add
   the corresponding library entries to `fmod_player.gdextension` before export.
   `build_all_platform.bat` builds Windows first and then Android, and stops
   immediately if a build fails.

For a single build, run `scons platform=android target=template_debug arch=arm64 -j16`.
Command-line options override `custom.py`, which is ignored by Git.
`FMOD_ANDROID_SDK` can also supply the default FMOD SDK path.

These scripts build the native GDExtension libraries. To export an APK, install
Godot's Android build template and run the plugin's **Configure FMOD Android
Export** tool using the same FMOD Android SDK so the Java runtime and FMOD
native libraries are included.

The adapter regression test can be built in a Visual Studio developer terminal:

```bat
cl /nologo /std:c++17 /EHsc /Isrc /Isrc/thirdparty/fmod/inc tests/fmod_dsp_process_compat_test.cpp /Fe:%TEMP%/fmod_dsp_process_compat_test.exe /Fo:%TEMP%/fmod_dsp_process_compat_test.obj
%TEMP%\fmod_dsp_process_compat_test.exe
```

For the Godot integration test, copy `tests/fmod_sdk_smoke_test.gd` into a test
project containing the built addon and your locally obtained FMOD runtime. Set
`audio/fmod/output_type` to `4` (NOSOUND_NRT) and
`audio/fmod/init_flags/mix_from_update` to `true`, then run the script headlessly.
It checks effect output, reverb tails, SDK metadata, and script callbacks. A
successful run prints `FMOD_SDK_SMOKE` results and exits with code zero.

## Documentation

Full documentation is available here:

**[godot-fmodplayerdocs.readthedocs.io](https://godot-fmodplayerdocs.readthedocs.io/en/latest/)**

The documentation includes setup guides, API references, usage examples, and platform-specific export notes.

## License

This repository is licensed under the MIT License. See [LICENSE.txt](LICENSE.txt).

FMOD is proprietary software by Firelight Technologies Pty Ltd. You must obtain the appropriate FMOD license for your project from [fmod.com](https://www.fmod.com).

## Acknowledgments

- [FMOD](https://www.fmod.com/) by Firelight Technologies Pty Ltd

<p align="center">
  <sub>FMOD is a trademark of Firelight Technologies Pty Ltd.<br>
  This plugin is not officially affiliated with or endorsed by Firelight Technologies.</sub>
</p>
