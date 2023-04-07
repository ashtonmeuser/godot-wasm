#!python
import os
import shutil
import re
from urllib import request
import tarfile

opts = Variables([], ARGUMENTS)

# Define options
opts.Add(EnumVariable('target', 'Compilation target', 'debug', ['debug', 'release'], {'d': 'debug'}))
opts.Add(EnumVariable('platform', 'Compilation platform', '', ['', 'windows', 'linux', 'osx'], {'x11': 'linux'}))
opts.Add(BoolVariable('use_llvm', 'Use LLVM/Clang compiler', 'no'))
opts.Add(BoolVariable('download_wasmer', 'Download Wasmer library', 'no'))
opts.Add('wasmer_version', 'Wasmer library version', 'v3.1.1')

# Standard flags CC, CCX, etc. with options
env = DefaultEnvironment(variables=opts)

# Wasmer download
def download_wasmer(env):
    def download_tarfile(url, dest, rename={}):
        filename = 'tmp.tar.gz'
        os.makedirs(dest, exist_ok=True)
        request.urlretrieve(url, filename)
        file = tarfile.open(filename)
        file.extractall(dest)
        file.close()
        for k, v in rename.items(): os.rename(k, v)
        os.remove(filename)
    base_url = 'https://github.com/wasmerio/wasmer/releases/download/{}/wasmer-{}.tar.gz'
    if env['platform'] == 'osx':
        # For macOS, we need to universalize the AMD and ARM libraries
        download_tarfile(base_url.format(env['wasmer_version'], 'darwin-amd64'), 'wasmer', {'wasmer/lib/libwasmer.a': 'wasmer/lib/libwasmer.amd64.a'})
        download_tarfile(base_url.format(env['wasmer_version'], 'darwin-arm64'), 'wasmer', {'wasmer/lib/libwasmer.a': 'wasmer/lib/libwasmer.arm64.a'})
        os.system('lipo wasmer/lib/libwasmer.*64.a -output wasmer/lib/libwasmer.a -create')
    elif env['platform'] == 'linux':
        download_tarfile(base_url.format(env['wasmer_version'], 'linux-amd64'), 'wasmer')
    elif env['platform'] == 'windows':
        download_tarfile(base_url.format(env['wasmer_version'], 'windows-amd64'), 'wasmer')

# Process some arguments
if env['platform'] == '':
    exit('Invalid platform selected')

if not re.fullmatch(r'v\d+\.\d+\.\d+(-.+)?', env['wasmer_version']):
    exit('Invalid Wasmer version')

if env['use_llvm']:
    env['CC'] = 'clang'
    env['CXX'] = 'clang++'

if env['download_wasmer'] or not os.path.isdir('wasmer'):
    shutil.rmtree('wasmer', True)
    download_wasmer(env)

# Check platform specifics
if env['platform'] == 'osx':
    env.Append(CCFLAGS=['-arch', 'x86_64'])
    env.Append(CXXFLAGS=['-std=c++17'])
    env.Append(LINKFLAGS=['-arch', 'x86_64', '-framework', 'Security'])
    if env['target'] in ('debug', 'd'):
        env.Append(CCFLAGS=['-g', '-O2'])
    else:
        env.Append(CCFLAGS=['-g', '-O3'])

elif env['platform'] == 'linux':
    env.Append(CCFLAGS=['-fPIC'])
    env.Append(CXXFLAGS=['-std=c++17'])
    if env['target'] in ('debug', 'd'):
        env.Append(CCFLAGS=['-g3', '-Og'])
    else:
        env.Append(CCFLAGS=['-g', '-O3'])

elif env['platform'] == 'windows':
    env['LIBPREFIX'] = ''
    env['LIBSUFFIX'] = '.lib'
    env['LIBWASMERSUFFIX'] = '.dll.lib' # Requires special suffix
    env.Append(ENV=os.environ) # Keep session env variables to support VS 2017 prompt
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

# Explicit static libraries
cpp_library = env.File('godot-cpp/bin/libgodot-cpp.{}.{}.64{}'.format(env['platform'], env['target'], env['LIBSUFFIX']))
wasmer_library = env.File('wasmer/lib/{}wasmer{}'.format(env['LIBPREFIX'], env.get('LIBWASMERSUFFIX', env['LIBSUFFIX'])))

# CPP includes and libraries
env.Append(CPPPATH=['.', 'godot-cpp/godot-headers', 'godot-cpp/include', 'wasmer/include', 'godot-cpp/include/core', 'godot-cpp/include/gen'])
env.Append(LIBS=[cpp_library, wasmer_library])

# Builders
env.SharedLibrary(target='addons/godot-wasm/bin/{}/godot-wasm'.format(env['platform']), source=env.Glob('src/*.cpp'))
env.Help(opts.GenerateHelpText(env))

