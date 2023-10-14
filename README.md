# Godot Wasm

<p align="center">
  <img width="200" src="https://github.com/ashtonmeuser/godot-wasm/assets/7253863/201300b3-41bc-4c54-b649-01d325ff8b69" alt="Godot Wasm Logo">
</p>
<p align="center">
  <a href="https://github.com/ashtonmeuser/godot-wasm/actions/workflows/addon.yml">
    <img src="https://github.com/ashtonmeuser/godot-wasm/actions/workflows/addon.yml/badge.svg" alt="Addon Status">
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

A Godot extension allowing for loading and interacting with [WebAssembly (Wasm)](https://webassembly.org) modules from GDScript via the [Wasmer](https://wasmer.io) and [Wasmtime](https://wasmtime.dev) WebAssembly runtimes.

## Documentation

Please refer to the [Godot Wasm wiki](https://github.com/ashtonmeuser/godot-wasm/wiki) for guides, FAQs, class documentation, etc.

## Getting Started

Godot Wasm can be used as a [GDExtension/GDNative addon](https://docs.godotengine.org/en/4.0/) or [Godot module](https://docs.godotengine.org/en/4.0/contributing/development/core_and_modules/custom_modules_in_cpp.html). See the [Installation wiki page](https://github.com/ashtonmeuser/godot-wasm/wiki/Getting-Started#installation) for full instructions.

Using Godot Wasm involves the following.
1. Create a WebAssembly (Wasm) module using a language of your choice. See [FAQ](https://github.com/ashtonmeuser/godot-wasm/wiki/FAQs#how-do-i-build-a-wasm-module) for more information.
1. Create a new Godot Wasm instance, read your Wasm module bytecode, and instantiate the Godot Wasm module. The following assumes a Wasm module that requires no imports.
    ```
    var wasm = Wasm.new()
    var file = FileAccess.open("res://my_module.wasm", FileAccess.READ)
    var bytecode = file.get_buffer(file.get_length())
    wasm.load(bytecode, {})
    ```
1. The Wasm module is now instantiated and can be interacted with from GDScript. For example, an exported function may be invoked using via `wasm.function("my_function", [my_arg])`.

See the [Usage wiki page](https://github.com/ashtonmeuser/godot-wasm/wiki/Getting-Started#usage) for full instructions.

## Known Issues

1. A small subset of [WASI](https://wasmbyexample.dev/examples/wasi-introduction/wasi-introduction.all.en-us.html) bindings are provided to the Wasm module by default. These can be overridden by the imports supplied on module instantiation. The guest Wasm module has no access to the host machines filesystem, etc. Pros for this are simplicity and increased security. Cons include not being able to generate truly random numbers (without a workaround) or run Wasm modules created in ways that require a larger set of WASI bindings e.g. [TinyGo](https://tinygo.org/docs/guides/webassembly/) (see relevant [issue](https://github.com/tinygo-org/tinygo/issues/3068)).
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
