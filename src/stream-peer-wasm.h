#ifndef STREAM_PEER_WASM_H
#define STREAM_PEER_WASM_H

#include "wasmer.h"
#include "defs.h"

#ifdef GODOT_MODULE
  #define SUPER_CLASS StreamPeer
  #define INTERFACE_DECLARE
  #define INTERFACE_GET_DATA get_data(uint8_t *buffer, int32_t bytes)
  #define INTERFACE_GET_PARTIAL_DATA get_partial_data(uint8_t *buffer, int32_t bytes, int32_t &received)
  #define INTERFACE_PUT_DATA put_data(const uint8_t *buffer, int32_t bytes)
  #define INTERFACE_PUT_PARTIAL_DATA put_partial_data(const uint8_t *buffer, int bytes, int32_t &sent)
  #define INTERFACE_GET_AVAILABLE_BYTES get_available_bytes() const
#else
  #define SUPER_CLASS StreamPeerExtension
  #define INTERFACE_DECLARE
  #define INTERFACE_GET_DATA _get_data(uint8_t *buffer, int32_t bytes, int32_t *received)
  #define INTERFACE_GET_PARTIAL_DATA _get_partial_data(uint8_t *buffer, int32_t bytes, int32_t *received)
  #define INTERFACE_PUT_DATA _put_data(const uint8_t *buffer, int32_t bytes, int32_t *sent)
  #define INTERFACE_PUT_PARTIAL_DATA _put_partial_data(const uint8_t *buffer, int32_t bytes, int32_t *sent)
  #define INTERFACE_GET_AVAILABLE_BYTES _get_available_bytes() const
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
      godot_error INTERFACE_GET_DATA override;
      godot_error INTERFACE_GET_PARTIAL_DATA override;
      godot_error INTERFACE_PUT_DATA override;
      godot_error INTERFACE_PUT_PARTIAL_DATA override;
      int32_t INTERFACE_GET_AVAILABLE_BYTES override;
  };
}

#endif
