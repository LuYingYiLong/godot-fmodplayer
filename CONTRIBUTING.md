# Contributing to Godot-FmodPlayer

Contributions can include bug fixes, audio effects, platform support, tests, and documentation. For larger changes, open an issue describing the use case and proposed approach before starting.

## Set up the repository

Initialize the pinned Godot C++ bindings:

```sh
git submodule update --init --recursive
```

You need a compatible Godot 4 executable, a C++17 compiler, Python, SCons, and the FMOD Core SDK 2.04 for your target platform. Windows builds use MSVC; Android builds also need the Android SDK and NDK.

Follow the [build instructions](README.md#building-from-source) to place the SDK headers and libraries. Keep headers, import libraries, and runtime libraries from the same FMOD release. Store local SDK paths in the ignored `custom.py` file or pass them to SCons.

Keep FMOD SDK files and runtime libraries, including `fmod.dll` and `libfmod*.so`, out of patches. Tests use a local copy of the runtime.

## Build and test

Run commands from the repository root. On Windows, set the paths for your machine:

```powershell
$godot = "C:\path\to\Godot.exe"
$runtime = "C:\path\to\FMOD\api\core\lib\x64\fmod.dll"

.\build_debug_to_test.bat -GodotPath $godot -FmodRuntime $runtime
.\tools\run_tests.ps1 -Suite regression -GodotPath $godot -FmodRuntime $runtime
```

The build script creates an optimized `template_debug` extension with matching PDB symbols, copies the complete addon to `tests/addons/fmod_player`, and refreshes Godot imports. Staged extension binaries belong in `tests/addons/fmod_player/bin`; test logs and reports belong in `tests/logs`.

Run the suite relevant to your change: `smoke`, `capture`, `spectrum`, `reverb`, or `profile`. The `regression` suite runs correctness checks; `all -Quick` keeps the combined regression and benchmark workflow. Use `-Suite editor` for property translations and `python tools/run_native_tests.py` for the C++ ring-buffer tests. For interactive checks, open `tests/project.godot` and press F5 for the test menu.

For Linux, build with the Linux SDK and run the test script:

```sh
export FMOD_LINUX_SDK=/path/to/fmodstudioapi20400linux
export GODOT_BIN=/path/to/Godot
bash build_linux.sh
bash tools/run_tests_linux.sh --suite=regression
```

See [tests/README.md](tests/README.md) for individual suites, Release testing, log locations, and performance comparisons. Android build and export instructions are in the [README](README.md#android).

### Performance changes

Measure a baseline before editing the implementation. Use the same compiler options, test scripts, audio configuration, and workload for both versions. Run the full measurements without `-Quick` and retain the build hashes and reports in `tests/logs`.

Use `tools/compare_capture.py` or `tools/compare_reverb.py` for the corresponding effect. Include timing results and audio correctness checks in the PR. Describe any intended change to audio output, latency, or memory use.

## Code style

Follow the surrounding C++ style and keep diffs focused. Use the pinned `godot-cpp` bindings and the FMOD 2.04 API. For exposed API changes, update registration, XML signatures, and affected tests together.

For DSP changes, make thread ownership and buffer layout explicit. Keep allocation and lock contention in audio callbacks low, and follow the existing main-thread dispatch rules for Godot objects and script callbacks. State whether buffer sizes count bytes, samples, or complete audio frames.

For GDScript:

- Use LF line endings, tabs, and explicit type hints.
- Add trailing commas to multiline arrays, dictionaries, and enums.
- Separate function and class definitions with two blank lines.
- Declare signals and properties before methods, and public members before private members. Place initialization first, then runtime callbacks, then public interfaces.
- Use past-tense English signal names and resource UIDs for hardcoded Godot resources when available.
- Write complete floating-point literals, such as `0.0` and `1.0`, and omit redundant default initialization values.
- Omit the final Chinese full stop in Chinese comments.

## Documentation

Maintain English API descriptions in `doc_classes/*.xml`. These files supply both Godot's built-in help and the generated website class reference. Keep signatures, defaults, enums, return values, units, and threading behavior aligned with the implementation.

Maintain Chinese API descriptions in `addons/fmod_player/translations/documentation/zh_CN.po`. Each message matches an entire XML description after Godot removes indentation and surrounding whitespace. Preserve documentation tags such as `[param frames]` and `[method get_buffer]`. Class names, signatures, and reference targets remain unchanged. The plugin registers this catalog in `godot.documentation`; both editor help and the website use it.

Install the catalog tools and refresh messages after editing XML:

```sh
python -m pip install -r tools/requirements-docs.txt
python tools/documentation_translations.py extract
python tools/documentation_translations.py update
# Review fuzzy entries and translate new messages in the PO.
python tools/documentation_translations.py check --strict
python tools/test_documentation_translations.py
```

Empty or fuzzy translations fall back to English. `check --strict` requires complete translations and checks documentation references and markup. Translation-only edits need addon deployment; they do not require rebuilding the extension. See `tests/README.md` for the isolated offline-help regression command.

Website guides and generation tools live in the separate [documentation repository](https://github.com/LuYingYiLong/godot-fmodplayer-docs). Its `classref/translations` directory contains a checked snapshot of the plugin PO. Generated class RST files, translated XML, and snapshot files are script outputs.

For API changes, rebuild and deploy the extension to the test project, then follow the documentation repository's README to export ClassDB metadata, synchronize the XML snapshot, and refresh translations. From that repository, build both languages with:

```sh
python scripts/build_docs.py
```

Use canonical `class_*` and `enum_*` reference labels. Include a link to the companion documentation PR when a change spans both repositories.

## Issues and pull requests

For a bug report, include the Godot and FMOD versions, platform and architecture, reproduction steps, expected behavior, and relevant log excerpts. A small reproduction project is useful for playback, import, or spatial audio issues.

Keep each PR centered on one change. Explain the problem, resulting behavior, and validation performed. List tested platforms and any remaining gaps. Performance PRs should include before-and-after measurements.

Keep local configuration, SDK files, test addon copies, caches, generated documentation, and profiling output out of commits. Contributions should be compatible with the project's [MIT License](LICENSE.txt); FMOD is licensed separately.
