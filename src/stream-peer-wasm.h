#ifndef STREAM_PEER_WASM_H
#define STREAM_PEER_WASM_H

#include "defs.h"

#ifdef GODOT_MODULE
  #define SUPER_CLASS StreamPeer
  #define INTERFACE_DECLARE
#else
  #define SUPER_CLASS StreamPeerGDNative
  #define INTERFACE_DECLARE godot_net_stream_peer interface
#endif

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
      uint32_t get_position() const;
      godot_error get_data(uint8_t* buffer, int bytes);
      godot_error get_partial_data(uint8_t* buffer, int bytes, int& received);
      godot_error put_data(const uint8_t* buffer, int bytes);
      godot_error put_partial_data(const uint8_t* buffer, int bytes, int& sent);
      int get_available_bytes() const;
  };
}

#endif
