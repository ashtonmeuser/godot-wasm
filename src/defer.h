#ifndef GODOT_WASM_DEFER_H
#define GODOT_WASM_DEFER_H

/*
Using the Wasm C API, there are often cases where resources should be deallocated after use
This emulates Go's defer functionality to schedule clearing resources after declaration
*/

#define CONCAT_INNER(x, y) x##y
#define CONCAT(x, y) CONCAT_INNER(x, y)
#define DEFER(code) auto CONCAT(_defer_, __LINE__) = _defer_func([&](){ code; })

template <typename T> struct _deferrer {
  T f;
  _deferrer(T f): f(f) {}
  ~_deferrer() { f(); }
};
template <typename T> _deferrer<T> _defer_func(T f) {
  return _deferrer<T>(f);
}

#endif
