<p align="center">
  <img alt="fmod_player_banner" src="https://github.com/user-attachments/assets/1d2655f5-d440-4bdb-978a-b095cb275e9c" />
</p>

<h1 align="center">Godot-FmodPlayer</h1>

<p align="center">
  <b>FMOD Core API playback, mixing, DSP, and 3D audio for Godot 4.</b>
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

## Features

- Playback nodes: `FmodAudioStreamPlayer`, `FmodAudioStreamPlayer2D`, and `FmodAudioStreamPlayer3D`.
- Audio import and inspector preview, with streaming and sample loading from files, memory, and Godot PCK resources.
- Bus synchronization with Godot's `AudioServer`, named bus routing, and channel group controls.
- DSP effects: gain, filter, EQ6/10/21, chorus, delay, distortion, phaser, pitch shift, reverb, compressor, limiter, stereo enhancement, and panning.
- Spectrum analysis, sample capture, and bus recording with waveform snapshots.
- 3D audio with camera-based listeners, distance attenuation, geometry occlusion, and reverb zones.
- Editor gizmos, geometry tools, project settings, performance monitors, and Android export setup.

## Quick Start

1. Install from the [Godot Asset Library](https://godotengine.org/asset-library/asset/4905) or copy `addons/fmod_player` from a [GitHub release](https://github.com/LuYingYiLong/Godot-FmodPlayer/releases) into your project.
2. On Windows, copy `api/core/lib/x64/fmod.dll` from the matching FMOD 2.04 [Core SDK](https://www.fmod.com/download) into `addons/fmod_player/bin`.
3. Enable **FMOD Player** under **Project > Project Settings > Plugins**.
4. Import audio as an FMOD audio resource, assign it to a player node, choose a bus, and enable `auto_play` or call `play()`.

## Platform Support

| Platform | Status |
|----------|--------|
| Windows x86_64 | Supported |
| Linux x86_64 | Tested on Ubuntu 26.04 |
| Android | arm64 builds; other ABIs available through `build_android.bat` |
| macOS / iOS | Experimental; requires local SDK and linking setup |

## Building From Source

Requires Godot 4.1+, a C++17 compiler, SCons, and the FMOD Core SDK 2.04. Windows builds use MSVC; Android builds use the Android NDK.

Initialize the bindings:

```sh
git submodule update --init --recursive
```

### Windows

Copy the Windows SDK's `api/core/inc` folder to `src/thirdparty/fmod/inc` and its `api/core/lib/x64` files to `src/thirdparty/fmod/lib/x64`. Use headers and libraries from the same SDK release.

```bat
scons platform=windows target=template_debug arch=x86_64 -j16
scons platform=windows target=template_release arch=x86_64 -j16
```

Place your SDK's `fmod.dll` in the project's `addons/fmod_player/bin` folder.

### Linux

Use the FMOD **Linux** Core SDK 2.04 with GCC or Clang and SCons:

```sh
FMOD_LINUX_SDK=/path/to/fmodstudioapi20400linux bash build_linux.sh debug_symbols=no
```

The script builds x86_64 Debug and Release extensions under `addons/fmod_player/bin`, with logs in `tests/logs`. Set `JOBS` to change the default of two compile jobs. `fmod_linux_sdk` also accepts an `api/core` folder through `custom.py` or the SCons command line; its default is `src/thirdparty/fmod/linux`.

Place `libfmodL.so.15` (Debug) and `libfmod.so.15` (Release) beside the extension. Copy the SDK's symlink targets as regular files with these names, or keep the versioned files and symlinks together. The extension uses `$ORIGIN` to find its runtime libraries.

### Android

Install the Android SDK and NDK, then extract the FMOD Android SDK. Create `custom.py` in the repository root with your local paths and installed NDK version:

```python
ANDROID_HOME = r"C:\path\to\Android\Sdk"
ndk_version = "your-installed-version"
fmod_android_sdk = r"C:\path\to\FMOD Android SDK"
```

`fmod_android_sdk` accepts the full SDK root or its `api/core` folder. The default location is `src/thirdparty/fmod/android`, containing `inc/` and `lib/<ABI>/`.

```bat
build_android.bat
```

This builds arm64 Debug and Release libraries. Pass `arch=x86_64`, `arch=x86_32`, or `arch=arm32` for another architecture, and add its library entries to `fmod_player.gdextension`. `build_all_platform.bat` builds Windows and Android.

For APK export, install Godot's Android build template and run **Configure FMOD Android Export** with the same FMOD Android SDK to include the Java and native runtime files.

Local configuration stays in `custom.py`, which is ignored by Git. SCons command-line arguments override it; `FMOD_ANDROID_SDK` supplies an alternative default SDK path.

## FMOD 2.04 API

- Use `set_input_channel_format()` / `get_input_channel_format()` for DSP input formats.
- Custom DSPs use `set_process_callback()`: `(samples, frames, input_channels, output_channels) -> Dictionary`, returning `{"output": samples}`. Offline NRT outputs run script callbacks on the main thread.
- `create_dsp()` accepts filter/generator mode, cooldown, and channel counts. `create_dsp_connection()` and `add_input_preallocated()` support connections configured before attachment.
- Sound creation and `FmodAudioStream.resampler_method` accept per-sound resampling settings.
- `read` / `shouldiprocess` callbacks, `MIX_FROM_UPDATE`, and `MEDIA_FOUNDATION` were removed. Output and sound type enums follow FMOD 2.04 numbering; update saved numeric values when migrating.

See the [FMOD 2.04 changes](https://www.fmod.com/docs/2.04/api/welcome-whats-new-204.html).

## Tests

From the repository root:

```powershell
.\build_debug_to_test.bat
.\tools\run_tests.ps1 -Suite regression
```

The build script copies the complete addon into `tests/addons/fmod_player`. Test runs go to `tests/logs/runs`, build logs to `tests/logs/builds`, and comparison reports to `tests/logs/comparisons`.

Open `tests/project.godot` and press F5 for the manual test menu. Automated tests use a separate offline process. See [tests/README.md](tests/README.md) for individual suites, editor and native checks, and performance comparisons.

## Documentation

[Setup guides, API reference, and examples](https://godot-fmodplayerdocs.readthedocs.io/en/latest/)

## License

Godot-FmodPlayer uses the [MIT License](LICENSE.txt). FMOD is licensed separately by [Firelight Technologies](https://www.fmod.com); obtain an FMOD license for your project.
