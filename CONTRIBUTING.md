# Contributing to Godot-FmodPlayer

Contributions can include bug fixes, audio effects, platform support, tests, and documentation. For larger changes, open an issue describing the use case and proposed approach before starting.

## Set up the repository

Initialize the pinned Godot C++ bindings:

```sh
git submodule update --init --recursive
```

You need a compatible Godot 4 executable, a C++17 compiler, Python, SCons, and the FMOD Core SDK 2.04 for your target platform. Windows builds use MSVC; Android builds also need the Android SDK and NDK.

Follow the [online documentation](https://godot-fmodplayerdocs.readthedocs.io/en/latest/) to place the SDK headers and libraries. Keep headers, import libraries, and runtime libraries from the same FMOD release. Store local SDK paths in the ignored `custom.py` file or pass them to SCons.

Keep FMOD SDK files and runtime libraries, including `fmod.dll` and `libfmod*.so`, out of patches. Tests use a local copy of the runtime.

## Build and test

Build the extension and stage the complete addon in the test project. From the repository root on Windows:

```sh
scons platform=windows target=template_debug copy_to_tests=yes
```

On Linux, configure the FMOD Linux SDK and use `platform=linux`. The build copies the local FMOD runtime as well; `fmod_runtime=/path/to/runtime` overrides its source. Close the test project before replacing a native library, then reopen `tests/project.godot` in Godot.

Press **F5** for the test menu. Use the playback comparison and 3D audio scenes to listen and interact, or click **Run numerical regression** (运行数值回归) for the eight retained correctness checks. The menu displays individual results and opens the latest report folder. Inspector previews, bus editing, localization, and gizmos are checked directly in the editor.

See [tests/README.md](tests/README.md) for the controls and editor checklist. You do not need a separate command-line test runner.

### Performance changes

Use the same compiler options, audio configuration, and workload when measuring before and after a change. Include measurements and a repeatable reproduction in the PR, along with the numerical regression results. Describe any intended changes to audio output, latency, or memory use.

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

## Editor localization

Open **Project > Tools > FMOD Translations** to edit the plugin catalogs. The panel manages property names (`godot.properties`), editor UI (`godot.editor`), and API help (`godot.documentation`). Property and UI CSV files are loaded directly; saving refreshes translations without generating GDScript data. Keep Simplified Chinese, Traditional Chinese, Japanese, and Russian UI translations complete.

The panel supports search, untranslated/review filters, message context, multiline text, source locations, and PO review flags. PO editing and source extraction use Python with Babel; configure its executable and the source repository in the panel. Property extraction uses the loaded ClassDB. Settings are stored per project in `user://fmod_localization.cfg`. CSV source text remains exact, including quoted multiline fields. Keep `%s` and other format placeholders in translations.

`fmod_check.gd` starts the shared GDScript translation service before checking the FMOD runtime. C++ editor text uses `src/editor/fmod_editor_translation.h`. Use `fmod_editor_translate()` for dynamic UI text and `FMOD_EDITOR_TEXT()` for literal menu keys that Godot translates automatically. Preserve runtime identifiers, resource names, file paths, and enum values.

Check language switching, placeholder preservation, and saved catalog changes directly in the Godot editor.

## Documentation

Maintain English API descriptions in `doc_classes/*.xml`. These files supply both Godot's built-in help and the generated website class reference. Keep signatures, defaults, enums, return values, units, and threading behavior aligned with the implementation.

Maintain Chinese API descriptions in `addons/fmod_player/translations/documentation/zh_CN.po`. Each message matches an entire XML description after Godot removes indentation and surrounding whitespace. Preserve documentation tags such as `[param frames]` and `[method get_buffer]`. Class names, signatures, and reference targets remain unchanged. The plugin registers this catalog in `godot.documentation`; both editor help and the website use it.

Use **Project > Tools > FMOD Translations** to extract and review messages after editing XML. Empty or fuzzy translations fall back to English. Translation-only edits need addon deployment; they do not require rebuilding the extension. Verify the result in Godot's built-in class help.

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
