#!/usr/bin/env python
import os
import sys

from SCons.Errors import UserError

build_options = Variables("custom.py", ARGUMENTS)
build_options.Add(
    "fmod_android_sdk",
    "FMOD Android Core SDK directory containing inc/ and lib/ (or the full SDK root)",
    os.environ.get("FMOD_ANDROID_SDK", "src/thirdparty/fmod/android"),
)
build_config = Environment(tools=[])
build_options.Update(build_config)
Help(build_options.GenerateHelpText(build_config))
# This option belongs to the extension, rather than godot-cpp.
ARGUMENTS.pop("fmod_android_sdk", None)
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
    env.Append(CCFLAGS=["/wd4828"])

    env.Append(LIBPATH=["src/thirdparty/fmod/lib/x64"])
    env.Append(LIBS=["fmod_vc"])
    library = env.SharedLibrary(
        "addons/fmod_player/bin/fmod_player{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
