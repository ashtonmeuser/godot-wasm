#!python
import os
from utils import download_wasmer, download_wasmtime, WASMER_VER_DEFAULT, WASMTIME_VER_DEFAULT

# Initial options inheriting from CLI args
opts = Variables([], ARGUMENTS)

# Define options
opts.Add(EnumVariable("target", "Compilation target", "release", ["debug", "release"], {"d": "debug"}))
opts.Add(EnumVariable("platform", "Platform", "", ["", "windows", "linux", "osx"], {"x11": "linux", "macos": "osx"}))
opts.Add(BoolVariable("use_llvm", "Use LLVM/Clang compiler", "no"))
opts.Add(EnumVariable("wasm_runtime", "Wasm runtime used", "wasmer", ["wasmer", "wasmtime"]))
opts.Add(BoolVariable("download_runtime", "(Re)download runtime library", "no"))
opts.Add("runtime_version", "Runtime library version", None)

# Standard flags CC, CCX, etc. with options
env = DefaultEnvironment(variables=opts)

# Process some arguments
if env["platform"] == "":
    exit("Invalid platform selected")

if env["use_llvm"]:
    env["CC"] = "clang"
    env["CXX"] = "clang++"

# Download runtime if required
if env["wasm_runtime"] == "wasmer":
    download_wasmer(env, env["download_runtime"], env.get("runtime_version", WASMER_VER_DEFAULT))
elif env["wasm_runtime"] == "wasmtime":
    download_wasmtime(env, env["download_runtime"], env.get("runtime_version", WASMTIME_VER_DEFAULT))

# Check platform specifics
if env["platform"] in ["osx", "macos"]:
    env.Prepend(CFLAGS=["-std=gnu11"])
    env.Prepend(CXXFLAGS=["-std=gnu++14"])
    env.Append(CCFLAGS=["-arch", "x86_64", "-Wall", "-g", "-O3"])
    env.Append(LINKFLAGS=["-arch", "x86_64", "-framework", "Security"])
elif env["platform"] == "linux":
    env.Prepend(CFLAGS=["-std=gnu11"])
    env.Prepend(CXXFLAGS=["-std=gnu++14"])
    env.Append(CCFLAGS=["-fPIC", "-g", "-O3"])
elif env["platform"] == "windows":
    env.Prepend(CCFLAGS=["/std:c++14", "-W3", "-GR", "-O2", "-EHsc"])
    env.Append(ENV=os.environ)  # Keep session env variables to support VS 2017 prompt
    env.Append(CPPDEFINES=["WIN32", "_WIN32", "_WINDOWS", "_CRT_SECURE_NO_WARNINGS", "NDEBUG"])
    if env.get("use_mingw"):  # MinGW
        env["LIBRUNTIMESUFFIX"] = ".a"
        env.Append(LIBS=["userenv"])
    else:  # MSVC
        env["LIBRUNTIMESUFFIX"] = ".lib"
        env.Append(CCFLAGS=["-MD"]) # Dynamic CRT used by Wasmer >= v3.2.0
        # Force Windows SDK library suffix (see https://github.com/godotengine/godot/issues/23687)
        env.Append(LINKFLAGS=["bcrypt.lib", "userenv.lib", "ws2_32.lib", "advapi32.lib", "ntdll.lib"])

# Defines for GDNative specific API
env.Append(CPPDEFINES=["GDNATIVE", "LIBWASM_STATIC"])

# Explicit static libraries
cpp_lib = env.File("godot-cpp/bin/libgodot-cpp.{}.{}.64{}".format(env["platform"], env["target"], env["LIBSUFFIX"]))
runtime_lib = env.File(
    "{runtime}/lib/{prefix}{runtime}{suffix}".format(
        runtime=env["wasm_runtime"],
        prefix=env["LIBPREFIX"],
        suffix=env.get("LIBRUNTIMESUFFIX", env["LIBSUFFIX"]),
    )
)

# CPP includes and libraries
env.Append(
    CPPPATH=[
        ".",
        "godot-cpp/godot-headers",
        "godot-cpp/include",
        "{}/include".format(env["wasm_runtime"]),
        "godot-cpp/include/core",
        "godot-cpp/include/gen",
    ]
)
env.Append(LIBS=[cpp_lib, runtime_lib])

# Godot Wasm sources
source = [env.Glob("src/*.cpp")]

# Builders
library = env.SharedLibrary(target="addons/godot-wasm/bin/{}/godot-wasm".format(env["platform"]), source=source)
env.Help(opts.GenerateHelpText(env))
Default(library)
