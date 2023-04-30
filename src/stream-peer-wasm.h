#ifndef STREAM_PEER_WASM_H
#define STREAM_PEER_WASM_H

#include "wasmer.h"
#include "defs.h"

#ifdef GODOT_MODULE
  #define SUPER_CLASS StreamPeer
  #define INTERFACE_DECLARE
#else
  #define SUPER_CLASS StreamPeerExtension
  #define INTERFACE_DECLARE
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
      uint32_t get_position();
      godot_error _get_data(uint8_t *buffer, int32_t bytes, int32_t *received) override;
      godot_error _get_partial_data(uint8_t *buffer, int32_t bytes, int32_t *received) override;
      godot_error _put_data(const uint8_t *buffer, int32_t bytes, int32_t *sent) override;
      godot_error _put_partial_data(const uint8_t *buffer, int32_t bytes, int32_t *sent) override;
      int32_t _get_available_bytes() const override;
  };
}

#endif
