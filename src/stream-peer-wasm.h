#ifndef STREAM_PEER_WASM_H
#define STREAM_PEER_WASM_H

#include "wasmer.h"
#include "defs.h"

namespace {
  #ifdef GODOT_MODULE
    #define SUPER_CLASS StreamPeer
    #define INTERFACE_DECLARE
  #else
    #define SUPER_CLASS StreamPeerGDNative
    #define INTERFACE_DECLARE godot_net_stream_peer interface
  #endif
}

namespace godot {
  class StreamPeerWasm : public SUPER_CLASS {
    GDCLASS(StreamPeerWasm, SUPER_CLASS);

    private:
      INTERFACE_DECLARE;
      uint32_t pointer;

    public:
      StreamPeerWasm* i;
      static void REGISTRATION_METHOD();
      StreamPeerWasm();
      ~StreamPeerWasm();
      void _init();
      wasm_memory_t* memory;
      Ref<StreamPeerWasm> seek(int p_pos);
      uint32_t get_position();
      godot_error put_data(const uint8_t* p_data, int p_bytes);
      godot_error put_partial_data(const uint8_t* p_data, int p_bytes, int& r_sent);
      godot_error get_data(uint8_t* p_buffer, int p_bytes);
      godot_error get_partial_data(uint8_t* p_buffer, int p_bytes, int& r_received);
      int get_available_bytes() const;
  };
}

#endif
