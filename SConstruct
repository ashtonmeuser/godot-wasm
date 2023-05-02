#!python
import os
import shutil
import re
from urllib import request
import tarfile

# Initial options inheriting from CLI args
opts = Variables([], ARGUMENTS)

# Define options
opts.Add(BoolVariable('download_wasmer', 'Download Wasmer library', 'no'))
opts.Add('wasmer_version', 'Wasmer library version', 'v3.1.1')

# SConstruct environment from Godot CPP
env = SConscript("godot-cpp/SConstruct")
opts.Update(env)

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
    if env['platform'] in ['osx', 'macos']:
        # For macOS, we need to universalize the AMD and ARM libraries
        download_tarfile(base_url.format(env['wasmer_version'], 'darwin-amd64'), 'wasmer', {'wasmer/lib/libwasmer.a': 'wasmer/lib/libwasmer.amd64.a'})
        download_tarfile(base_url.format(env['wasmer_version'], 'darwin-arm64'), 'wasmer', {'wasmer/lib/libwasmer.a': 'wasmer/lib/libwasmer.arm64.a'})
        os.system('lipo wasmer/lib/libwasmer.*64.a -output wasmer/lib/libwasmer.a -create')
    elif env['platform'] == 'linux':
        download_tarfile(base_url.format(env['wasmer_version'], 'linux-amd64'), 'wasmer')
    elif env['platform'] == 'windows':
        download_tarfile(base_url.format(env['wasmer_version'], 'windows-amd64'), 'wasmer')

# Process some arguments
if not re.fullmatch(r'v\d+\.\d+\.\d+(-.+)?', env['wasmer_version']):
    exit('Invalid Wasmer version')

if env['download_wasmer'] or not os.path.isdir('wasmer'):
    print('Downloading Wasmer {}'.format(env['wasmer_version']))
    shutil.rmtree('wasmer', True)
    download_wasmer(env)

if env['platform'] == 'windows':
    env['LIBWASMERSUFFIX'] = '.dll.lib' # Requires special suffix
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
