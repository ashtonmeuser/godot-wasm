#!python
import os
from utils import download_wasmer, VERSION_DEFAULT

# Initial options inheriting from CLI args
opts = Variables([], ARGUMENTS)

# Define options
opts.Add(BoolVariable("download_wasmer", "Download Wasmer library", "no"))
opts.Add("wasmer_version", "Wasmer library version", VERSION_DEFAULT)

# SConstruct environment from Godot CPP
env = SConscript("godot-cpp/SConstruct")
opts.Update(env)

# Download Wasmer if required
download_wasmer(env, env["download_wasmer"], env["wasmer_version"])

# Check platform specifics
if env["platform"] == "windows":
    if env.get("use_mingw"):  # MinGW
        env["LIBWASMERSUFFIX"] = ".a"
        env.Append(LIBS=["userenv"])
    else:  # MSVC
        env["LIBWASMERSUFFIX"] = ".lib"
        # Force Windows SDK library suffix (see https://github.com/godotengine/godot/issues/23687)
        env.Append(LINKFLAGS=["bcrypt.lib", "userenv.lib", "ws2_32.lib", "advapi32.lib"])

# Defines for GDExtension specific API
env.Append(CPPDEFINES=["GDEXTENSION"])

# Explicit static libraries
wasmer_lib = env.File("wasmer/lib/{}wasmer{}".format(env["LIBPREFIX"], env.get("LIBWASMERSUFFIX", env["LIBSUFFIX"])))

# CPP includes and libraries
env.Append(CPPPATH=[".", "wasmer/include"])
env.Append(LIBS=[wasmer_lib])

# Godot Wasm sources
source = ["register_types.cpp", env.Glob("src/*.cpp")]

# Builders
library = env.SharedLibrary(target="addons/godot-wasm/bin/{}/godot-wasm".format(env["platform"]), source=source)
env.Help(opts.GenerateHelpText(env))
Default(library)
