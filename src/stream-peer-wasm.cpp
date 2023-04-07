#include "stream-peer-wasm.h"

namespace {
  #define GDNATIVE_VERSION { 3, 5 } // No clue if this is correct

  // StreamPeerGDNative interface
  godot_error _get_data(void *user, uint8_t *p_buffer, int p_bytes) { return ((godot::StreamPeerWasm *)user)->get_data(p_buffer, p_bytes); }
  godot_error _get_partial_data(void *user, uint8_t *p_buffer, int p_bytes, int *r_received) { return ((godot::StreamPeerWasm *)user)->get_partial_data(p_buffer, p_bytes, r_received); }
  godot_error _put_data(void *user, const uint8_t *p_data, int p_bytes) { return ((godot::StreamPeerWasm *)user)->put_data(p_data, p_bytes); }
  godot_error _put_partial_data(void *user, const uint8_t *p_data, int p_bytes, int *r_sent) { return ((godot::StreamPeerWasm *)user)->put_partial_data(p_data, p_bytes, r_sent); }
  int _get_available_bytes(const void *user) { return ((godot::StreamPeerWasm *)user)->get_available_bytes(); }
}

namespace godot {
  void StreamPeerWasm::_register_methods() {
    register_property("pointer", &StreamPeerWasm::pointer, (uint32_t)0);
    register_method("seek", &StreamPeerWasm::seek);
  }

  StreamPeerWasm::StreamPeerWasm() {
    interface = {
      GDNATIVE_VERSION,
      this,
      &_get_data,
      &_get_partial_data,
      &_put_data,
      &_put_partial_data,
      &_get_available_bytes,
      NULL
    };
  }

  StreamPeerWasm::~StreamPeerWasm() { }

  void StreamPeerWasm::_init() {
    net_api->godot_net_bind_stream_peer(_owner, &interface);
  }

  Ref<StreamPeerWasm> StreamPeerWasm::seek(int p_pos) {
    Ref<StreamPeerWasm> ref = Ref<StreamPeerWasm>(this);
    FAIL_IF(p_pos < 0, "Invalid stream peer position", ref);
    pointer = p_pos;
    return ref;
  }

  godot_error StreamPeerWasm::get_data(uint8_t *p_buffer, int p_bytes) {
    FAIL_IF(memory == NULL, "Invalid stream peer memory", GDERROR(ERR_INVALID_DATA));
    byte_t* data = wasm_memory_data(memory) + pointer;
    memcpy(p_buffer, data, p_bytes);
    pointer += p_bytes;
    return GDERROR(OK);
  }

  godot_error StreamPeerWasm::get_partial_data(uint8_t *p_buffer, int p_bytes, int *r_received) {
    *r_received = p_bytes;
    return get_data(p_buffer, p_bytes);
  }

  godot_error StreamPeerWasm::put_data(const uint8_t *p_data, int p_bytes) {
    FAIL_IF(memory == NULL, "Invalid stream peer memory", GDERROR(ERR_INVALID_DATA));
    if (p_bytes <= 0) return GDERROR(OK);
    byte_t* data = wasm_memory_data(memory) + pointer;
    memcpy(data, p_data, p_bytes);
    pointer += p_bytes;
    return GDERROR(OK);
  }

  godot_error StreamPeerWasm::put_partial_data(const uint8_t *p_data, int p_bytes, int *r_sent) {
    *r_sent = p_bytes;
    return put_data(p_data, p_bytes);
  }

  int StreamPeerWasm::get_available_bytes() {
    return 0; // Not relevant
  }
}
