#include "stream-peer-wasm.h"

#ifdef GDNATIVE
  #define INTERFACE_DEFINE interface = { { 3, 5 }, this, &_get_data, &_get_partial_data, &_put_data, &_put_partial_data, &_get_available_bytes, NULL }
  #define INTERFACE_INIT net_api->godot_net_bind_stream_peer(_owner, &interface)
  namespace {
    godot_error _get_data(void* user, uint8_t* buffer, int bytes) { return ((godot::StreamPeerWasm*)user)->get_data(buffer, bytes); }
    godot_error _get_partial_data(void* user, uint8_t* buffer, int bytes, int* received) { return ((godot::StreamPeerWasm*)user)->get_partial_data(buffer, bytes, *received); }
    godot_error _put_data(void* user, const uint8_t* buffer, int bytes) { return ((godot::StreamPeerWasm*)user)->put_data(buffer, bytes); }
    godot_error _put_partial_data(void* user, const uint8_t* buffer, int bytes, int* sent) { return ((godot::StreamPeerWasm*)user)->put_partial_data(buffer, bytes, *sent); }
    int _get_available_bytes(const void* user) { return ((godot::StreamPeerWasm*)user)->get_available_bytes(); }
  }
#else
  #define INTERFACE_DEFINE
  #define INTERFACE_INIT
#endif

namespace godot {
  void StreamPeerWasm::REGISTRATION_METHOD() {
    #ifdef GDNATIVE
      register_method("seek", &StreamPeerWasm::seek);
      register_method("get_position", &StreamPeerWasm::get_position);
    #else
      ClassDB::bind_method(D_METHOD("seek", "p_pos"), &StreamPeerWasm::seek);
      ClassDB::bind_method(D_METHOD("get_position"), &StreamPeerWasm::get_position);
    #endif
  }

  StreamPeerWasm::StreamPeerWasm() {
    INTERFACE_DEFINE;
    pointer = 0;
    memory = NULL;
  }

  StreamPeerWasm::~StreamPeerWasm() { }

  void StreamPeerWasm::_init() {
    INTERFACE_INIT;
  }

  Ref<StreamPeerWasm> StreamPeerWasm::seek(int p_pos) {
    Ref<StreamPeerWasm> ref = Ref<StreamPeerWasm>(this);
    FAIL_IF(p_pos < 0, "Invalid stream peer position", ref);
    pointer = p_pos;
    return ref;
  }

  uint32_t StreamPeerWasm::get_position() {
    return pointer;
  }

  godot_error StreamPeerWasm::INTERFACE_GET_DATA {
    FAIL_IF(memory == NULL, "Invalid stream peer memory", ERR_INVALID_DATA);
    byte_t* data = wasm_memory_data(memory) + pointer;
    memcpy(buffer, data, bytes);
    pointer += bytes;
    #ifndef GODOT_MODULE
      *received = bytes;
    #endif
    return OK;
  }

  godot_error StreamPeerWasm::INTERFACE_GET_PARTIAL_DATA {
    #ifdef GODOT_MODULE
      received = bytes;
      return get_data(buffer, bytes);
    #else
      return _get_data(buffer, bytes, received);
    #endif
  }

  godot_error StreamPeerWasm::INTERFACE_PUT_DATA {
    FAIL_IF(memory == NULL, "Invalid stream peer memory", ERR_INVALID_DATA);
    if (bytes <= 0) return OK;
    byte_t* data = wasm_memory_data(memory) + pointer;
    memcpy(data, buffer, bytes);
    pointer += bytes;
    #ifndef GODOT_MODULE
      *sent = bytes;
    #endif
    return OK;
  }

  godot_error StreamPeerWasm::INTERFACE_PUT_PARTIAL_DATA {
    #ifdef GODOT_MODULE
      sent = bytes;
      return put_data(buffer, bytes);
    #else
      return _put_data(buffer, bytes, sent);
    #endif
  }

  int32_t StreamPeerWasm::INTERFACE_GET_AVAILABLE_BYTES {
    return 0; // Not relevant
  }
}
