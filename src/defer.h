#ifndef GODOT_WASM_DEFER_H
#define GODOT_WASM_DEFER_H

#define GODOT_WASM_CONCAT_INNER(x, y) x##y
#define GODOT_WASM_CONCAT(x, y) GODOT_WASM_CONCAT_INNER(x, y)
#define GODOT_WASM_DEFER(code) auto GODOT_WASM_CONCAT(_defer_, __LINE__) = ::godot_wasm::_defer_func([&](){ code; })

namespace godot_wasm {
  template <typename T> struct _deferrer {
    T f;
    _deferrer(T f): f(f) {}
    ~_deferrer() { f(); }
  };
  template <typename T> _deferrer<T> _defer_func(T f) {
    return _deferrer<T>(f);
  }
}

#endif
