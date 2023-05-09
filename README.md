# Godot Wasm

<p align="center">
  <a href="https://wasmer.io" target="_blank" rel="noopener noreferrer">
    <img width="200" src="https://raw.githubusercontent.com/ashtonmeuser/godot-wasm/master/media/Icon.png" alt="Wasmer logo">
  </a>
</p>
<p align="center">
  <a href="https://github.com/ashtonmeuser/godot-wasm/actions/workflows/build-addon.yml">
    <img src="https://github.com/ashtonmeuser/godot-wasm/actions/workflows/build-addon.yml/badge.svg" alt="Build Status">
  </a>
  <a href="https://github.com/ashtonmeuser/godot-wasm/blob/master/LICENSE">
    <img src="https://img.shields.io/github/license/ashtonmeuser/godot-wasm" alt="License">
  </a>
  <a href="https://github.com/ashtonmeuser/godot-wasm/releases/latest">
    <img src="https://img.shields.io/github/v/release/ashtonmeuser/godot-wasm" alt="Version">
  </a>
</p>

> **Warning**
> This project is still in its infancy. Interfaces are liable to change with each release until v1.0.

A Godot addon allowing for loading and interacting with [WebAssembly (Wasm)](https://webassembly.org) modules from GDScript. Note that this project is still in development.

Godot Wasm can be used either as a [GDNative](https://docs.godotengine.org/en/stable/tutorials/scripting/gdnative/what_is_gdnative.html) addon or Godot module. It uses [Wasmer](https://wasmer.io) as the WebAssembly runtime.

## Installation

Godot Wasm supports installation via addon or module. Installing as an addon is far faster and simpler and requires merely including the asset in your Godot project while installing as a module requires recompilation of the Godot engine.

### Addon

Installation in a Godot project involves simply downloading and installing a zip file from Godot's UI. Recompilation of the engine is *not* required.

1. Download the Godot Wasm addon zip file from the [releases page](https://github.com/ashtonmeuser/godot-wasm/releases).
1. In Godot's Asset Library tab, click Import and select the addon zip file. Follow prompts to complete installation of the addon.

Alternatively, you can use the Asset Library tab within the Godot editor, search for "Wasm", and follow the prompts to install. Yet another alternative is downloading directly from the [asset page](https://godotengine.org/asset-library/asset/1798) and following the installation instructions above. Note that the Asset Library has an approval process that can take several days and may therefore be a version or two behind.

### Godot Module

Installation as a Godot module requires recompilation of the Godot engine.

1. Clone or download the [Godot engine](https://github.com/godotengine/godot) following [this guide](https://docs.godotengine.org/en/3.5/development/compiling/getting_source.html).
1. Download the Godot Wasm source via the [releases page](https://github.com/ashtonmeuser/godot-wasm/releases) or Code → Download ZIP on GitHub.
1. Include the entire Godot Wasm directory within the *godot/modules* directory.
1. Rename the Godot Wasm directory to *wasm*. All project files e.g. *SCsub* should now be in *godot/modules/wasm*.

Recompile the Godot engine following [this guide](https://docs.godotengine.org/en/3.5/development/compiling/index.html#toc-devel-compiling). More information on custom Godot modules can be found in [this guide](https://docs.godotengine.org/en/3.5/development/cpp/custom_modules_in_cpp.html).

Compiling the web/HTML5 export template is not yet supported (see [#18](https://github.com/ashtonmeuser/godot-wasm/issues/18)).

## Usage

Once installed as an addon in a Godot project, the Godot Wasm addon class can be accessed via `Wasm`.

1. Create a Wasm module or use the [example module](https://github.com/ashtonmeuser/godot-wasm/blob/master/examples/wasm-consume/example.wasm).
1. Add the Wasm module to your Godot project.
1. In GDScript, instantiate the `Wasm` class via `var wasm = Wasm.new()`.
1. Load your Wasm module bytecode as follows replacing `YOUR_WASM_MODULE_PATH` with the path to your Wasm module e.g. *example.wasm*. The `Wasm.load()` method accepts a [PoolByteArray](https://docs.godotengine.org/en/stable/classes/class_poolbytearray.html) and a dictionary defining Wasm module imports. All imports should be satisfied and may differ with each Wasm module.
    ```
    var file = File.new()
    file.open("res://YOUR_WASM_MODULE_PATH", File.READ)
    var buffer = file.get_buffer(file.get_len())
    var imports = { "functions": { "index.callback": [self, "callback"] } } # Set imports according to Wasm module
    wasm.load(buffer, imports)
    file.close()
    ```
1. Access global constants and mutables exported by the Wasm module via `wasm.global("YOUR_GLOBAL_NAME")` replacing `YOUR_GLOBAL_NAME` with the name of an exported Wasm module global.
1. Define an array containing the arguments to be supplied to your exported Wasm module function via `var args = [1, 2]`. Ensure the number of arguments and argument types match those expected by the exported Wasm module function.
1. Call a function exported by your Wasm module via `wasm.function("YOUR_FUNCTION_NAME", args)` replacing `YOUR_FUNCTION_NAME` with the name of the exported Wasm module function.

### Exporting Godot Project

> **Note**
> Exporting to web/HTML5 is not supported. See [#15](https://github.com/ashtonmeuser/godot-wasm/issues/15) and [#18](https://github.com/ashtonmeuser/godot-wasm/issues/18).

Exporting to Windows, macOS, and Linux is officially supported. Exporting from Godot may require the following additional steps. See the export configuration of the [example Godot project](https://github.com/ashtonmeuser/godot-wasm/tree/master/examples) for a practical illustration.

1. For macOS exports, disable library validation in Project → Export → Options.
1. If your project contains Wasm files, they'll need to be marked for export. Add `*.wasm` in Project → Export → Resources.

### Writing to Wasm Module Memory

Writing data to exported Wasm memory is supported via a familiar [StreamPeer](https://docs.godotengine.org/en/3.5/classes/class_streampeer.html) interface. This StreamPeer is available under the `stream` property of the `Wasm` object.

The internal StreamPeerWasm class mirrors the `seek()` and `get_position()` methods of [StreamPeerBuffer](https://docs.godotengine.org/en/3.5/classes/class_streampeerbuffer.html#class-streampeerbuffer) with the addition of `seek()` returning a reference to the StreamPeer allowing chaining i.e. `wasm.stream.seek(0).get_u64()`.

Note that, as mentioned in Godot's  [StreamPeer documentation](https://docs.godotengine.org/en/stable/classes/class_streampeer.html#class-streampeer-method-put-string), writing strings via `put_string()` and `put_utf8_string()` will prepend four bytes containing the length of the string.

## Examples

[Examples](https://github.com/ashtonmeuser/godot-wasm/tree/master/examples) are provided for both creating and consuming/using a Wasm module.

### Wasm Consume (Godot)

A simple example of loading a Wasm module, displaying the structure of its imports and exports, calling its exported functions, and providing GDScript callbacks via import functions. Some computationally-expensive benchmarks e.g. [prime sieve](https://en.wikipedia.org/wiki/Sieve_of_Atkin) in GDScript and Wasm can be compared. The Wasm module used is that generated by the [Wasm Create example](#wasm-create-assemblyscript). All logic for this Godot project exists in `Main.gd`.

### Wasm Create (AssemblyScript)

An example [AssemblyScript](https://www.assemblyscript.org) project which creates a simple Wasm module with the following exported entities.

Entity | Type | Description
--|--|--
`global_const` | Global `i64` | Global constant
`global_var` | Global `f64` | Global mutable
`memory_value` | Global `i64` | Used to store first eight bytes of memory to demonstrate memory manipulation
`update_memory` | Function `() → void` | Updates the value of `memory_value`
`add` | Function `(i64, i64) → i64` | Adds two integers
`fibonacci` | Function `(i64) → i64` | Return Fibonacci number at the position provided
`sieve` | Function `(i64) → i32` | Return largest prime number up to the limit provided

From the example directory (*examples/wasm-create*), install or update Node modules `npm i`. Run `npm run build` to build the Wasm module. This will create and populate a *build* directory containing the Wasm module, *example.wasm*, amongst other build artifacts. If you have the [Wasmer CLI](https://docs.wasmer.io/ecosystem/wasmer/getting-started) installed, run `wasmer inspect build/example.wasm` to view the Wasm module imports and exports.

Note that WebAssembly is a large topic and thoroughly documenting the creation of Wasm modules is beyond the scope of this project. AssemblyScript is just one of [many ways](https://github.com/appcypher/awesome-wasm-langs#awesome-webassembly-languages-) to create a Wasm module.

### Wasm Visualizations

An example of displaying graphics generated by Wasm modules. Graphics are read directly from the Wasm module memory using the [StreamPeer](https://docs.godotengine.org/en/3.5/classes/class_streampeer.html) interface.

## Benchmarks

Comparison of GDScript, GDNative, and Wasm (n=1000, p95). The following benchmarks were run on macOS 12.6.3, 16GB RAM, 2.8 GHz i7. The project was exported to avoid GDScript slowdown likely caused by performance monitoring. Speedup figures for both GDNative and Wasm are relative to GDScript. The benchmarks used are 1) a [DP Nth Fibonacci number](https://www.geeksforgeeks.org/program-for-nth-fibonacci-number/) and 2) a [Sieve of Atkin](https://www.geeksforgeeks.org/sieve-of-atkin/).

Benchmark | GDScript | GDNative | WRT GDScript | Wasm | WRT GDScript
--|--|--|--|--|--
Fibonacci (index=20) | 6.0 µs | 1.0 µs | 6.0x | 4.0 µs | 1.5x
Fibonacci (index=50) | 13.0 µs | 1.0 µs | 13.0x | 4.0 µs | 3.3x
Sieve (limit=1000) | 704.1 µs | 4.0 µs | 176.0x | 30.0 µs | 23.5x
Sieve (limit=10000) | 6331.8 µs | 52.0 µs | 121.8x | 134.1 µs | 47.2x
Sieve (limit=100000) | 62454.1 µs | 341.0 µs | 183.1x | 1053.1 µs | 59.3x

## Developing

This section is to aid in developing the Godot Wasm addon; not to use the addon in a Godot project.

These instructions are tailored to UNIX machines.

1. Clone repo and submodules via `git clone --recurse-submodules https://github.com/ashtonmeuser/godot-wasm.git`.
1. Ensure the correct Godot submodule commit is checked out. Refer to relevant branch of the [godot-cpp project](https://github.com/godotengine/godot-cpp/tree/3.x) e.g. `3.x` to verify submodule hash. At the time of this writing, the hash for the godot-cpp submodule was `1049927a596402cd2e8215cd6624868929f5f18d`.
1. Download the Wasmer C library from [Wasmer releases](https://github.com/wasmerio/wasmer/releases) and add them to a *wasmer* directory at the root of this project. There should be *include* and *lib* subdirectories within the *wasmer* directory.
1. Install [SConstruct](https://scons.org/) via `pip install SCons`. SConstruct is what Godot uses to build Godot and generate C++ bindings. For convenience, we'll use the same tool to build the Godot TF Inference addon.
1. Compile the Godot C++ bindings. From within the *godot-cpp* directory, run `scons target=release platform=PLATFORM generate_bindings=yes` replacing `PLATFORM` with your relevant platform type e.g. `osx`, `linux`, `windows`, etc.
1. Compile the Godot Wasm addon. From the repository root directory, run `scons platform=PLATFORM` once again replacing `PLATFORM` with your platform. This will create the *addons/godot-wasm/bin/PLATFORM* directory where `PLATFORM` is your platform. You should see a dynamic library (*.dylib*, *.so*, *.dll*, etc.) created within this directory.
1. Copy the Wasmer dynamic libraries to the appropriate platform directory via `cp -RP wasmer/lib/. addons/godot-wasm/bin/PLATFORM/` replacing `PLATFORM` with your platform.
1. Zip the addons directory via `zip -FSr addons.zip addons`. This allows the addon to be conveniently distributed and imported into Godot. This zip file can be imported directly into Godot (see [Installation](https://github.com/ashtonmeuser/godot-wasm#installation)).

If frequently iterating on the addon using a Godot project, it may help to create a symlink from the Godot project to the compiled addon via `ln -s RELATIVE_PATH_TO_ADDONS addons` from the root directory of your Godot project.

## Known Issues

1. No [WASI](https://wasmbyexample.dev/examples/wasi-introduction/wasi-introduction.all.en-us.html) bindings are provided to the Wasm module. This means that the guest Wasm module has no access to the host machines filesystem, etc. Pros for this are simplicity and increased security. Cons include not being able to generate truly random numbers (without a workaround) or run Wasm modules created in ways that require WASI bindings e.g. [TinyGo](https://tinygo.org/docs/guides/webassembly/) (see relevant [issue](https://github.com/tinygo-org/tinygo/issues/3068)).
1. Only `int` and `float` return values are supported. While workarounds could be used, this limitation is because the only [concrete types supported by Wasm](https://webassembly.github.io/spec/core/syntax/types.html#number-types) are integers and floating point.
1. A default empty `args` parameter for `function(name, args)` can not be supplied. Default `Array` parameters in GDNative seem to retain values between calls. Calling methods of this addon without expected arguments produces undefined behaviour. This is reliant on [godotengine/godot-cpp#209](https://github.com/godotengine/godot-cpp/issues/209).
1. Web/HTML5 export is not supported (see [#15](https://github.com/ashtonmeuser/godot-wasm/issues/15) and [#18](https://github.com/ashtonmeuser/godot-wasm/issues/18)).

## Relevant Discussion

There have been numerous discussions around modding/sandboxing support for Godot. Some of those are included below.

- [Proposal](https://github.com/godotengine/godot-proposals/issues/5010): Implement a sandbox mode
- [Issue](https://github.com/godotengine/godot/issues/28303) Add support for WebAssembly plugins and scripts
- [Proposal](https://github.com/godotengine/godot-proposals/issues/147) Add WASM (WASI) host support (including, but not limited to, the HTML5 target)
- [Proposal](https://github.com/godotengine/godot-proposals/issues/4642): Add a method to disallow using all global classes in a particular GDScript
- [Pull Request](https://github.com/godotengine/godot/pull/61831): Added globals disabled feature to GDScript class

## Roadmap

Please feel free submit a PR or an [issue](https://github.com/ashtonmeuser/godot-wasm/issues).

- [x] Load module
- [x] Export functions
- [x] Export global constants
- [x] Export global mutables
- [ ] Export tables
- [x] Export memories
- [x] Import functions
- [ ] Import globals
- [x] Map export names to indices (access function/global by name)
- [x] Inspect module exports
- [ ] Automatically provide [AssemblyScript special imports](https://www.assemblyscript.org/concepts.html#special-imports)
- [ ] Automatically cast to 32-bit values
- [x] Inspect function signatures
- [ ] Set export globals
