#!python
from utils import download_wasmer, download_wasmtime, WASMER_VER_DEFAULT, WASMTIME_VER_DEFAULT

# Initial options inheriting from CLI args
opts = Variables([], ARGUMENTS)

# Define options
opts.Add(EnumVariable("wasm_runtime", "Wasm runtime used", "wasmtime", ["wasmer", "wasmtime"]))
opts.Add(BoolVariable("download_runtime", "(Re)download runtime library", "no"))
opts.Add("runtime_version", "Runtime library version", None)

# SConstruct environment from Godot CPP
env = SConscript("godot-cpp/SConstruct")
opts.Update(env)

# Download runtime if required
if env["wasm_runtime"] == "wasmer":
    download_wasmer(env, env["download_runtime"], env.get("runtime_version", WASMER_VER_DEFAULT))
elif env["wasm_runtime"] == "wasmtime":
    download_wasmtime(env, env["download_runtime"], env.get("runtime_version", WASMTIME_VER_DEFAULT))

# Check platform specifics
if env["platform"] == "windows":
    # Additional libraries required by Wasmer
    env.Append(LIBS=["ole32.lib", "runtimeobject.lib"])
    if env.get("use_mingw"):  # MinGW
        env["LIBRUNTIMESUFFIX"] = ".a"
        env.Append(LIBS=["userenv"])
    else:  # MSVC
        env["LIBRUNTIMESUFFIX"] = ".lib"
        env.Append(CCFLAGS=["-MD"])  # Dynamic CRT used by Wasmer >= v3.2.0
        if "/MT" in env["CCFLAGS"]: env["CCFLAGS"].remove("/MT")  # Silence MT/MD override warning
        # Force Windows SDK library suffix (see https://github.com/godotengine/godot/issues/23687)
        env.Append(LINKFLAGS=["bcrypt.lib", "userenv.lib", "ws2_32.lib", "advapi32.lib", "ntdll.lib"])
        # Additional libraries to build wasmtime for Windows
        if env["wasm_runtime"] == "wasmtime":
            env.Append(LINKFLAGS=["shell32.lib", "ole32.lib"])
            env.Append(LINKFLAGS=["/WX:NO"])  # Temporarily disable warnings as errors to fix LIBCMT conflict warning

# Defines for GDExtension specific API
env.Append(CPPDEFINES=["GDEXTENSION", "LIBWASM_STATIC"])

# Explicit static libraries
runtime_lib = env.File(
    "{runtime}/lib/{prefix}{runtime}{suffix}".format(
        runtime=env["wasm_runtime"],
        prefix=env["LIBPREFIX"],
        suffix=env.get("LIBRUNTIMESUFFIX", env["LIBSUFFIX"]),
    )
)

# CPP includes and libraries
env.Append(CPPPATH=[".", "{}/include".format(env["wasm_runtime"])])
env.Append(LIBS=[runtime_lib])

# Godot Wasm sources
source = ["register_types.cpp", env.Glob("src/*.cpp"), env.Glob("src/extensions/*.cpp")]

# Builders
library = env.SharedLibrary(target="addons/godot-wasm/bin/{}/godot-wasm".format(env["platform"]), source=source)
env.Help(opts.GenerateHelpText(env))
Default(library)
