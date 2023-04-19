#ifndef STREAM_PEER_WASM_H
#define STREAM_PEER_WASM_H

#include <Godot.hpp>
#include <StreamPeerGDNative.hpp>
#include "wasmer.h"
#include "defs.h"

namespace godot {
  class StreamPeerWasm : public StreamPeerGDNative {
    GODOT_CLASS(StreamPeerWasm, StreamPeerGDNative)

    private:
      godot_net_stream_peer interface;
      uint32_t pointer;

    public:
      static void _register_methods();
      StreamPeerWasm();
      ~StreamPeerWasm();
      void _init();
      wasm_memory_t* memory;
      Ref<StreamPeerWasm> seek(int p_pos);
      uint32_t get_position();
      godot_error get_data(uint8_t *p_buffer, int p_bytes);
      godot_error get_partial_data(uint8_t *p_buffer, int p_bytes, int *r_received);
      godot_error put_data(const uint8_t *p_data, int p_bytes);
      godot_error put_partial_data(const uint8_t *p_data, int p_bytes, int *r_sent);
      int get_available_bytes() const;
  };
}

#endif
