#!python
import os
import tarfile
import urllib.request

opts = Variables([], ARGUMENTS)

# Standard flags CC, CCX, etc.
env = DefaultEnvironment()

# Define options
opts.Add(EnumVariable('target', 'Compilation target', 'debug', ['d', 'debug', 'r', 'release']))
opts.Add(EnumVariable('platform', 'Compilation platform', '', ['', 'windows', 'x11', 'linux', 'osx']))
opts.Add(EnumVariable('p', 'Compilation target, alias for platform', '', ['', 'windows', 'x11', 'linux', 'osx']))
opts.Add(BoolVariable('use_llvm', 'Use the LLVM / Clang compiler', 'no'))
opts.Add(BoolVariable('download_wasmer', 'Auto download the wasmer library', 'no'))

# Local dependency paths, adapt them to your setup
wasmer_path = 'wasmer/'
wasmer_library = 'wasmer'
# Replace the platform with %s in the url. Pltform is derived from scons platform opt.
wasmer_url = "https://github.com/wasmerio/wasmer/releases/download/v3.1.1/wasmer-%s-amd64.tar.gz"
godot_headers_path = 'godot-cpp/godot-headers/'
cpp_bindings_path = 'godot-cpp/'
cpp_library = 'libgodot-cpp'
target_path = 'addons/godot-wasm/bin/'
target_name = 'godot-wasm'
bits = 64

# Updates the environment with the option variables.
opts.Update(env)

def download_wasmer(): 
    if os.path.exists(wasmer_path):
        print('Wasmer folder exists, skipping download!')
        return
    
    platform = ""
    if env['platform']=='x11' or env['platform']=='linux':
        platform="linux"
    elif env['platform']=='osx':
        platform='darwin'
    else:
        platform='windows'
    tarball = urllib.request.urlopen(wasmer_url % platform)
    tarfile.open(fileobj=tarball, mode='r:gz').extractall(wasmer_path)

# Process some arguments
if env['use_llvm']:
    env['CC'] = 'clang'
    env['CXX'] = 'clang++'

if env['p'] != '':
    env['platform'] = env['p']

if env['platform'] == '':
    print('No valid target platform selected.')
    quit();

if env['download_wasmer']:
    download_wasmer()

# Fix needed on OSX
def rpath_fix(target, source, env):
    os.system('install_name_tool -change @rpath/libwasmer.dylib @loader_path/libwasmer.dylib {0}'.format(target[0]))

# Check platform specifics
if env['platform'] == 'osx':
    target_path += 'osx/'
    cpp_library += '.osx'
    env.Append(CCFLAGS=['-arch', 'x86_64'])
    env.Append(CXXFLAGS=['-std=c++17'])
    env.Append(LINKFLAGS=['-arch', 'x86_64'])
    if env['target'] in ('debug', 'd'):
        env.Append(CCFLAGS=['-g', '-O2'])
    else:
        env.Append(CCFLAGS=['-g', '-O3'])

elif env['platform'] in ('x11', 'linux'):
    target_path += 'linux/'
    cpp_library += '.linux'
    env.Append(CCFLAGS=['-fPIC'])
    env.Append(CXXFLAGS=['-std=c++17'])
    if env['target'] in ('debug', 'd'):
        env.Append(CCFLAGS=['-g3', '-Og'])
    else:
        env.Append(CCFLAGS=['-g', '-O3'])

elif env['platform'] == 'windows':
    target_path += 'windows/'
    cpp_library += '.windows'
    wasmer_library += '.dll'
    # This makes sure to keep the session environment variables on windows,
    # that way you can run scons in a vs 2017 prompt and it will find all the required tools
    env.Append(ENV=os.environ)

    env.Append(CPPDEFINES=['WIN32', '_WIN32', '_WINDOWS', '_CRT_SECURE_NO_WARNINGS'])
    env.Append(CCFLAGS=['-W3', '-GR'])
    env.Append(CCFLAGS='/std:c++20')
    env.Append(LIBS=['bcrypt', 'userenv', 'ws2_32', 'advapi32'])
    if env['target'] in ('debug', 'd'):
        env.Append(CPPDEFINES=['_DEBUG'])
        env.Append(CCFLAGS=['-EHsc', '-MDd', '-ZI'])
        env.Append(LINKFLAGS=['-DEBUG'])
    else:
        env.Append(CPPDEFINES=['NDEBUG'])
        env.Append(CCFLAGS=['-O2', '-EHsc', '-MD'])

if env['target'] in ('debug', 'd'):
    cpp_library += '.debug'
else:
    cpp_library += '.release'

cpp_library += '.' + str(bits)

env.Append(CPPPATH=['.', godot_headers_path, cpp_bindings_path + 'include/', wasmer_path + 'include/', cpp_bindings_path + 'include/core/', cpp_bindings_path + 'include/gen/'])
env.Append(LIBPATH=[cpp_bindings_path + 'bin/', wasmer_path + 'lib/'])
env.Append(LIBS=[cpp_library, wasmer_library])

sources = Glob('*.cpp')

library = env.SharedLibrary(target=target_path + target_name, source=sources)

if env['platform'] == 'osx':
    env.AddPostAction(library, rpath_fix)

Default(library)

Help(opts.GenerateHelpText(env))

