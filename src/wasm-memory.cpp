#include "wasmer.h"
#include "wasm-memory.h"
#include "store.h"

#ifdef GDNATIVE
  #define INTERFACE_DEFINE interface = { { 3, 1 }, this, &_get_data, &_get_partial_data, &_put_data, &_put_partial_data, &_get_available_bytes, NULL }
  #define INTERFACE_INIT net_api->godot_net_bind_stream_peer(_owner, &interface)
  namespace {
    godot_error _get_data(void* user, uint8_t* buffer, int bytes) { return ((godot::WasmMemory*)user)->get_data(buffer, bytes); }
    godot_error _get_partial_data(void* user, uint8_t* buffer, int bytes, int* received) { return ((godot::WasmMemory*)user)->get_partial_data(buffer, bytes, *received); }
    godot_error _put_data(void* user, const uint8_t* buffer, int bytes) { return ((godot::WasmMemory*)user)->put_data(buffer, bytes); }
    godot_error _put_partial_data(void* user, const uint8_t* buffer, int bytes, int* sent) { return ((godot::WasmMemory*)user)->put_partial_data(buffer, bytes, *sent); }
    int _get_available_bytes(const void* user) { return ((godot::WasmMemory*)user)->get_available_bytes(); }
  }
#else
  #define INTERFACE_DEFINE
  #define INTERFACE_INIT
#endif

namespace godot {
  void WasmMemory::REGISTRATION_METHOD() {
    #ifdef GDNATIVE
      register_method("inspect", &WasmMemory::inspect);
      register_method("grow", &WasmMemory::grow);
      register_method("seek", &WasmMemory::seek);
      register_method("get_position", &WasmMemory::get_position);
    #else
      ClassDB::bind_method(D_METHOD("inspect"), &WasmMemory::inspect);
      ClassDB::bind_method(D_METHOD("grow", "pages"), &WasmMemory::grow);
      ClassDB::bind_method(D_METHOD("seek", "p_pos"), &WasmMemory::seek);
      ClassDB::bind_method(D_METHOD("get_position"), &WasmMemory::get_position);
    #endif
  }

  WasmMemory::WasmMemory() {
    INTERFACE_DEFINE;
    memory = NULL;
    pointer = 0;
  }

  WasmMemory::~WasmMemory() {
    set_memory(NULL);
  }

  void WasmMemory::_init() {
    INTERFACE_INIT;
  }

  void WasmMemory::set_memory(const wasm_memory_t* memory) {
    if (this->memory != NULL) wasm_memory_delete(this->memory);
    this->memory = (wasm_memory_t*)memory;
  }

  wasm_memory_t* WasmMemory::get_memory() const {
    return memory;
  }

  Dictionary WasmMemory::inspect() const {
    if (memory == NULL) return Dictionary();
    auto limits = wasm_memorytype_limits(wasm_memory_type(memory));
    Dictionary dict;
    dict["min"] = limits->min * PAGE_SIZE;
    dict["max"] = limits->max * PAGE_SIZE;
    dict["current"] = wasm_memory_size(memory) * PAGE_SIZE;
    return dict;
  }

  godot_error WasmMemory::grow(uint32_t pages) {
    if (!memory) { // Create new memory
      const wasm_limits_t limits = { pages, wasm_limits_max_default };
      memory = wasm_memory_new(STORE, wasm_memorytype_new(&limits));
      return memory ? OK : FAILED;
    }
    FAIL_IF(memory == NULL, "Invalid memory", ERR_INVALID_DATA);
    return wasm_memory_grow(memory, pages) ? OK : FAILED;
  }

  Ref<WasmMemory> WasmMemory::seek(int p_pos) {
    Ref<WasmMemory> ref = Ref<WasmMemory>(this);
    FAIL_IF(p_pos < 0, "Invalid memory position", ref);
    pointer = p_pos;
    return ref;
  }

  uint32_t WasmMemory::get_position() const {
    return pointer;
  }

  godot_error WasmMemory::INTERFACE_GET_DATA {
    FAIL_IF(memory == NULL, "Invalid memory", ERR_INVALID_DATA);
    byte_t* data = wasm_memory_data(memory) + pointer;
    memcpy(buffer, data, bytes);
    pointer += bytes;
    #ifndef GODOT_MODULE
      *received = bytes;
    #endif
    return OK;
  }

  godot_error WasmMemory::INTERFACE_GET_PARTIAL_DATA {
    #ifdef GODOT_MODULE
      received = bytes;
      return get_data(buffer, bytes);
    #else
      return _get_data(buffer, bytes, received);
    #endif
  }

  godot_error WasmMemory::INTERFACE_PUT_DATA {
    FAIL_IF(memory == NULL, "Invalid memory", ERR_INVALID_DATA);
    if (bytes <= 0) return OK;
    byte_t* data = wasm_memory_data(memory) + pointer;
    memcpy(data, buffer, bytes);
    pointer += bytes;
    #ifndef GODOT_MODULE
      *sent = bytes;
    #endif
    return OK;
  }

  godot_error WasmMemory::INTERFACE_PUT_PARTIAL_DATA {
    #ifdef GODOT_MODULE
      sent = bytes;
      return put_data(buffer, bytes);
    #else
      return _put_data(buffer, bytes, sent);
    #endif
  }

  int32_t WasmMemory::INTERFACE_GET_AVAILABLE_BYTES {
    return 0; // Not relevant
  }
}
