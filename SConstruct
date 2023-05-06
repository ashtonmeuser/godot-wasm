#!python
import os
from utils import download_wasmer, VERSION_DEFAULT

# Initial options inheriting from CLI args
opts = Variables([], ARGUMENTS)

# Define options
opts.Add(EnumVariable("target", "Compilation target", "release", ["debug", "release"], {"d": "debug"}))
opts.Add(EnumVariable("platform", "Platform", "", ["", "windows", "linux", "osx"], {"x11": "linux", "macos": "osx"}))
opts.Add(BoolVariable("use_llvm", "Use LLVM/Clang compiler", "no"))
opts.Add(BoolVariable("download_wasmer", "Download Wasmer library", "no"))
opts.Add("wasmer_version", "Wasmer library version", VERSION_DEFAULT)

# Standard flags CC, CCX, etc. with options
env = DefaultEnvironment(variables=opts)

# Process some arguments
if env["platform"] == "":
    exit("Invalid platform selected")

if env["use_llvm"]:
    env["CC"] = "clang"
    env["CXX"] = "clang++"

# Download Wasmer if required
download_wasmer(env, env["download_wasmer"], env["wasmer_version"])

# Check platform specifics
if env["platform"] in ["osx", "macos"]:
    env.Append(CCFLAGS=["-arch", "x86_64", "-Wall", "-g", "-O3"])
    env.Append(CXXFLAGS=["-std=c++17"])
    env.Append(LINKFLAGS=["-arch", "x86_64", "-framework", "Security"])

elif env["platform"] == "linux":
    env.Append(CCFLAGS=["-fPIC", "-g", "-O3"])
    env.Append(CXXFLAGS=["-std=c++17"])

elif env["platform"] == "windows":
    env["LIBPREFIX"] = ""
    env["LIBSUFFIX"] = ".lib"
    env["LIBWASMERSUFFIX"] = ".a" if env.get("use_mingw") else ".lib"
    env.Append(ENV=os.environ)  # Keep session env variables to support VS 2017 prompt
    env.Append(CPPDEFINES=["WIN32", "_WIN32", "_WINDOWS", "_CRT_SECURE_NO_WARNINGS", "NDEBUG"])
    env.Append(CCFLAGS=["-W3", "-GR", "-O2", "-EHsc", "-MD"])
    env.Append(CCFLAGS="/std:c++20")
    env.Append(LIBS=["bcrypt", "userenv", "ws2_32", "advapi32"])

# Defines for GDNative specific API
env.Append(CPPDEFINES=["GDNATIVE"])

# Explicit static libraries
cpp_lib = env.File("godot-cpp/bin/libgodot-cpp.{}.{}.64{}".format(env["platform"], env["target"], env["LIBSUFFIX"]))
wasmer_lib = env.File("wasmer/lib/{}wasmer{}".format(env["LIBPREFIX"], env.get("LIBWASMERSUFFIX", env["LIBSUFFIX"])))

# CPP includes and libraries
env.Append(
    CPPPATH=[
        ".",
        "godot-cpp/godot-headers",
        "godot-cpp/include",
        "wasmer/include",
        "godot-cpp/include/core",
        "godot-cpp/include/gen",
    ]
)
env.Append(LIBS=[cpp_lib, wasmer_lib])

# Godot Wasm sources
source = [env.Glob("src/*.cpp")]

# Builders
library = env.SharedLibrary(target="addons/godot-wasm/bin/{}/godot-wasm".format(env["platform"]), source=source)
env.Help(opts.GenerateHelpText(env))
Default(library)
