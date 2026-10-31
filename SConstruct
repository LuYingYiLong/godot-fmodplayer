#!/usr/bin/env python
import filecmp
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys

from SCons.Errors import UserError
from SCons.Variables import BoolVariable

build_options = Variables("custom.py", ARGUMENTS)
build_options.Add(
    "fmod_android_sdk",
    "FMOD Android Core SDK directory containing inc/ and lib/ (or the full SDK root)",
    os.environ.get("FMOD_ANDROID_SDK", "src/thirdparty/fmod/android"),
)
build_options.Add(
    "fmod_linux_sdk",
    "FMOD Linux Core SDK directory containing inc/ and lib/ (or the full SDK root)",
    os.environ.get("FMOD_LINUX_SDK", "src/thirdparty/fmod/linux"),
)
build_options.Add(BoolVariable(
    "copy_to_tests",
    "Copy the complete addon and local FMOD runtime to tests after a successful build",
    False,
))
build_options.Add(
    "fmod_runtime",
    "Override the FMOD runtime copied to tests (defaults to the Windows/Linux SDK runtime)",
    "",
)
build_config = Environment(tools=[])
build_options.Update(build_config)
Help(build_options.GenerateHelpText(build_config))
# These options belong to the extension, rather than godot-cpp.
for option in ("fmod_android_sdk", "fmod_linux_sdk", "copy_to_tests", "fmod_runtime"):
    ARGUMENTS.pop(option, None)
# godot-cpp resolves custom.py relative to its own directory. Forward the
# remaining root configuration options without forwarding our FMOD option.
for key, value in build_options.UnknownVariables().items():
    ARGUMENTS.setdefault(key, value)

if ARGUMENTS.get("platform") == "android":
    android_home = ARGUMENTS.get("ANDROID_HOME") or os.environ.get("ANDROID_HOME") or os.environ.get("ANDROID_SDK_ROOT")
    if not android_home and not os.environ.get("ANDROID_NDK_ROOT"):
        raise UserError("Set ANDROID_HOME in custom.py to your Android SDK directory and ndk_version to an installed NDK version.")
    # godot-cpp accesses this key even when a standalone NDK is configured.
    ARGUMENTS.setdefault("ANDROID_HOME", android_home or "")

ARGUMENTS.setdefault("custom_api_file", File("#api/godot_4_7_2.json").abspath)
env = SConscript("godot-cpp/SConstruct")

env.Append(CPPPATH=["src/"])

if env["platform"] == "android":
    fmod_core_path = Dir(build_config["fmod_android_sdk"]).abspath
    if os.path.isdir(os.path.join(fmod_core_path, "api", "core")):
        fmod_core_path = os.path.join(fmod_core_path, "api", "core")
    fmod_arch_dirs = {
        "arm64": "arm64-v8a",
        "arm32": "armeabi-v7a",
        "x86_64": "x86_64",
        "x86_32": "x86",
    }
    fmod_lib_path = os.path.join(fmod_core_path, "lib", fmod_arch_dirs[env["arch"]])
    fmod_library_name = "fmod" if env["target"] == "template_release" else "fmodL"
    for required_file in [
        os.path.join(fmod_core_path, "inc", "fmod.hpp"),
        os.path.join(fmod_lib_path, "lib" + fmod_library_name + ".so"),
    ]:
        if not os.path.isfile(required_file):
            raise UserError(
                "Missing FMOD Android SDK file: " + required_file
                + "\nExtract the Android SDK and set fmod_android_sdk in custom.py "
                "or on the SCons command line. Windows FMOD libraries cannot be used for Android."
            )
    env.Append(CPPPATH=[os.path.join(fmod_core_path, "inc")])

elif env["platform"] == "linux":
    fmod_core_path = Dir(build_config["fmod_linux_sdk"]).abspath
    if os.path.isdir(os.path.join(fmod_core_path, "api", "core")):
        fmod_core_path = os.path.join(fmod_core_path, "api", "core")
    fmod_arch_dirs = {"x86_64": "x86_64", "x86_32": "x86", "arm64": "arm64", "arm32": "arm"}
    if env["arch"] not in fmod_arch_dirs:
        raise UserError("The FMOD Linux SDK does not support architecture: " + env["arch"])
    fmod_lib_path = os.path.join(fmod_core_path, "lib", fmod_arch_dirs[env["arch"]])
    fmod_library_name = "fmod" if env["target"] == "template_release" else "fmodL"
    for required_file in [
        os.path.join(fmod_core_path, "inc", "fmod.hpp"),
        os.path.join(fmod_lib_path, "lib" + fmod_library_name + ".so"),
    ]:
        if not os.path.isfile(required_file):
            raise UserError(
                "Missing FMOD Linux SDK file: " + required_file
                + "\nExtract the Linux SDK and set fmod_linux_sdk in custom.py or on the SCons command line."
            )
    env.Append(CPPPATH=[os.path.join(fmod_core_path, "inc")])
else:
    env.Append(CPPPATH=["src/thirdparty/fmod/inc"])

# godot-cpp only defines TOOLS_ENABLED for target=editor. This extension's
# .gdextension file uses the template_debug binary inside the Godot editor, so
# keep editor-only integration code enabled for template_debug builds as well.
if env["target"] == "template_debug":
    env.Append(CPPDEFINES=["TOOLS_ENABLED"])

env.Tool("compilation_db")
env.CompilationDatabase()
sources = (Glob("src/core/*.cpp") + Glob("src/audio/*.cpp") + Glob("src/playback/*.cpp") + 
           Glob("src/mixer/*.cpp") + Glob("src/dsp/*.cpp") + Glob("src/nodes/*.cpp") + 
           Glob("src/editor/*.cpp")) + Glob("src/geometry/*.cpp") + Glob("src/spatial/*.cpp")

if env["target"] in ["editor", "template_debug"]:
    sources += Glob("src/editor/audio_effects/*.cpp")
    sources += Glob("src/editor/audio_buses/*.cpp")
    doc_data = env.GodotCPPDocData("src/gen/doc_data.gen.cpp", source=Glob("doc_classes/*.xml"))
    sources.append(doc_data)

if env["platform"] == "android":
    if sys.platform == "win32":
        # Avoid godot-cpp's long-command fallback launching llvm-ar once per
        # object file. LLVM accepts a response file for the full archive.
        env["ARCOM"] = '${TEMPFILE("$AR $ARFLAGS $TARGET $SOURCES", "$ARCOMSTR")}'
        env["MAXLINELENGTH"] = 32000
        env["TEMPFILEPREFIX"] = "@"
        env["TEMPFILESUFFIX"] = ".rsp"
    windows_libs_blacklist = [
        "mingw32", "gcc", "gcc_s", "moldname", "mingwex", "msvcrt", 
        "advapi32", "shell32", "user32", "kernel32", "stdc++"
    ]
    if "LIBS" in env:
        clean_libs = [lib for lib in env["LIBS"] if str(lib) not in windows_libs_blacklist]
        env.Replace(LIBS=clean_libs)

if env["platform"] == "android":
    env.Append(CXXFLAGS=['-std=c++17'])
    print(f"FMOD Android library path: {fmod_lib_path}")
    env.Append(LIBPATH=[fmod_lib_path])
    env.Append(LIBS=[fmod_library_name])

    if env["target"] == "template_release":
        env.Append(CXXFLAGS=['-O3'])
    else:
        env.Append(CXXFLAGS=['-g', '-O0'])
    
    library = env.SharedLibrary(
        "addons/fmod_player/bin/libfmod_player{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

elif env["platform"] == "linux":
    env.Append(LIBPATH=[fmod_lib_path])
    env.Append(LIBS=[fmod_library_name])
    # Fail the build for unresolved extension symbols instead of failing at load time.
    env.Append(LINKFLAGS=["-Wl,--no-undefined"])
    library = env.SharedLibrary(
        "addons/fmod_player/bin/libfmod_player{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

elif env["platform"] == "macos":
    library = env.SharedLibrary(
        "addons/fmod_player/bin/fmod_player.{}.{}.framework/fmod_player.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )

elif env["platform"] == "ios":
    if env["ios_simulator"]:
        library = env.StaticLibrary(
            "addons/fmod_player/bin/fmod_player.{}.{}.simulator.a".format(env["platform"], env["target"]),
            source=sources,
        )
    else:
        library = env.StaticLibrary(
            "addons/fmod_player/bin/fmod_player.{}.{}.a".format(env["platform"], env["target"]),
            source=sources,
        )

else:
    if env["platform"] == "windows" and env.get("is_msvc", False) and env["debug_symbols"]:
        # Embed compiler symbols in each object to avoid the shared compiler
        # PDB server. /DEBUG:FULL still produces the final DLL's matching PDB.
        env["CCFLAGS"] = [flag for flag in env["CCFLAGS"] if flag not in ["/Zi", "/FS"]]
        env.Append(CCFLAGS=["/Z7"])
    env.Append(CCFLAGS=["/utf-8"])
    env.Append(CXXFLAGS=["/utf-8"])

    env.Append(LIBPATH=["src/thirdparty/fmod/lib/x64"])
    env.Append(LIBS=["fmod_vc"])
    library = env.SharedLibrary(
        "addons/fmod_player/bin/fmod_player{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

if build_config["copy_to_tests"]:
    def copy_test_addon(target, source, env):
        root = Path(Dir("#").abspath)
        addon = root / "addons/fmod_player"
        destination = root / "tests/addons/fmod_player"
        if not destination.resolve().is_relative_to(root / "tests"):
            raise UserError("Test addon destination must stay inside this repository's tests directory.")
        binary = Path(source[0].abspath)
        symbols = binary.with_suffix(".pdb")
        has_pdb = env["platform"] == "windows" and env.get("is_msvc", False) and env["debug_symbols"]
        if has_pdb and not symbols.is_file():
            raise UserError("Matching PDB missing: " + str(symbols) + ". Rebuild the extension before copying it.")

        runtime = None
        runtime_name = ""
        if env["platform"] == "windows":
            runtime = root / "src/thirdparty/fmod/lib/x64/fmod.dll"
            runtime_name = "fmod.dll"
        elif env["platform"] == "linux":
            runtime_name = "lib" + fmod_library_name + ".so.15"
            runtime = Path(fmod_lib_path) / runtime_name
            if not runtime.is_file():
                runtime = Path(fmod_lib_path) / ("lib" + fmod_library_name + ".so")
        if build_config["fmod_runtime"]:
            runtime = Path(Dir("#").abspath, build_config["fmod_runtime"])
            runtime_name = runtime_name or runtime.name
        if runtime is not None and not runtime.is_file():
            raise UserError("FMOD runtime missing: " + str(runtime) + ". Set fmod_runtime to your local SDK runtime file.")

        copied = []
        skipped_pdb = symbols.relative_to(addon) if env["platform"] == "windows" and not has_pdb else None

        def copy_file(original, relative):
            staged = destination / relative
            if not staged.resolve().is_relative_to(destination.resolve()):
                raise UserError("Test addon file escaped its destination: " + str(staged))
            if staged.is_file() and filecmp.cmp(original, staged, shallow=False):
                return
            staged.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(original, staged)
            copied.append(relative.as_posix())

        for directory, directories, filenames in os.walk(addon):
            directories[:] = [name for name in directories if name != "__pycache__"]
            for filename in filenames:
                original = Path(directory) / filename
                relative = original.relative_to(addon)
                if original.suffix in (".pyc", ".pyo", ".tmp", ".lib", ".exp", ".ilk") or relative == skipped_pdb:
                    continue
                copy_file(original, relative)
        if skipped_pdb is not None:
            (destination / skipped_pdb).unlink(missing_ok=True)
        if runtime is not None:
            copy_file(runtime, Path("bin") / runtime_name)

        def digest(path):
            return hashlib.sha256(path.read_bytes()).hexdigest()

        report = {
            "platform": env["platform"], "target": env["target"], "arch": env["arch"],
            "binary": binary.name, "binary_sha256": digest(binary),
            "pdb_sha256": digest(symbols) if has_pdb else None,
            "runtime": runtime_name or None,
            "runtime_sha256": digest(runtime) if runtime is not None else None,
            "copied": copied,
        }
        log_directory = root / "tests/logs/builds"
        log_directory.mkdir(parents=True, exist_ok=True)
        report_path = log_directory / ("copy_to_tests.{platform}.{target}.{arch}.json".format(**report))
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")
        print("Test addon synchronized: tests/addons/fmod_player (" + str(len(copied)) + " files updated)")
        return 0

    # A link post-action would miss script-only edits and up-to-date builds.
    deployment = env.Alias("copy_to_tests", library, Action(copy_test_addon, "Synchronizing FMOD addon to tests ..."))
    env.AlwaysBuild(deployment)
    Default(deployment)
else:
    Default(library)
