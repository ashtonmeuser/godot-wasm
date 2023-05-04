#!python
import os
from utils import download_wasmer, VERSION_DEFAULT

# Initial options inheriting from CLI args
opts = Variables([], ARGUMENTS)

# Define options
opts.Add(BoolVariable('download_wasmer', 'Download Wasmer library', 'no'))
opts.Add('wasmer_version', 'Wasmer library version', VERSION_DEFAULT)

# SConstruct environment from Godot CPP
env = SConscript("godot-cpp/SConstruct")
opts.Update(env)

# Download Wasmer if required
download_wasmer(env, env['download_wasmer'], env['wasmer_version'])


if env['platform'] == 'windows':
    env['LIBWASMERSUFFIX'] = '.a' if env.get('use_mingw') else '.dll.lib'
    env.Append(LIBS=['bcrypt', 'userenv', 'ws2_32', 'advapi32'])

# Defines for GDExtension specific API
env.Append(CPPDEFINES=["GDEXTENSION"])

# Explicit static libraries
wasmer_library = env.File('wasmer/lib/{}wasmer{}'.format(env['LIBPREFIX'], env.get('LIBWASMERSUFFIX', env['LIBSUFFIX'])))

# CPP includes and libraries
env.Append(CPPPATH=['.', 'wasmer/include'])
env.Append(LIBS=[wasmer_library])

# Godot Wasm sources
source = ['register_types.cpp', env.Glob('src/*.cpp')]

# Builders
library = env.SharedLibrary(target='addons/godot-wasm/bin/{}/godot-wasm'.format(env['platform']), source=source)
env.Help(opts.GenerateHelpText(env))
Default(library)
