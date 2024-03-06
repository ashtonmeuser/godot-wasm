#ifndef GODOT_WASM_MEMORY_H
#define GODOT_WASM_MEMORY_H

#include "defs.h"

#ifdef GODOT_MODULE
  #define GODOT_WASM_SUPER_CLASS StreamPeer
  #define GODOT_WASM_INTERFACE_DECLARE
  #define GODOT_WASM_INTERFACE_GET_DATA get_data(uint8_t *buffer, int32_t bytes)
  #define GODOT_WASM_INTERFACE_GET_PARTIAL_DATA get_partial_data(uint8_t *buffer, int32_t bytes, int32_t &received)
  #define GODOT_WASM_INTERFACE_PUT_DATA put_data(const uint8_t *buffer, int32_t bytes)
  #define GODOT_WASM_INTERFACE_PUT_PARTIAL_DATA put_partial_data(const uint8_t *buffer, int bytes, int32_t &sent)
  #define GODOT_WASM_INTERFACE_GET_AVAILABLE_BYTES get_available_bytes() const
#else
  #define GODOT_WASM_SUPER_CLASS StreamPeerExtension
  #define GODOT_WASM_INTERFACE_DECLARE
  #define GODOT_WASM_INTERFACE_GET_DATA _get_data(uint8_t *buffer, int32_t bytes, int32_t *received)
  #define GODOT_WASM_INTERFACE_GET_PARTIAL_DATA _get_partial_data(uint8_t *buffer, int32_t bytes, int32_t *received)
  #define GODOT_WASM_INTERFACE_PUT_DATA _put_data(const uint8_t *buffer, int32_t bytes, int32_t *sent)
  #define GODOT_WASM_INTERFACE_PUT_PARTIAL_DATA _put_partial_data(const uint8_t *buffer, int32_t bytes, int32_t *sent)
  #define GODOT_WASM_INTERFACE_GET_AVAILABLE_BYTES _get_available_bytes() const
#endif

namespace godot {
  class WasmMemory : public GODOT_WASM_SUPER_CLASS {
    GDCLASS(WasmMemory, GODOT_WASM_SUPER_CLASS);

    private:
      GODOT_WASM_INTERFACE_DECLARE;
      wasm_memory_t* memory;
      uint32_t pointer;

    public:
      static void GODOT_WASM_REGISTRATION_METHOD();
      WasmMemory();
      ~WasmMemory();
      void _init();
      void set_memory(const wasm_memory_t* memory);
      wasm_memory_t* get_memory() const;
      Dictionary inspect() const;
      GODOT_WASM_ERROR grow(uint32_t pages);
      Ref<WasmMemory> seek(int p_pos);
      uint32_t get_position() const;
      GODOT_WASM_ERROR GODOT_WASM_INTERFACE_GET_DATA override;
      GODOT_WASM_ERROR GODOT_WASM_INTERFACE_GET_PARTIAL_DATA override;
      GODOT_WASM_ERROR GODOT_WASM_INTERFACE_PUT_DATA override;
      GODOT_WASM_ERROR GODOT_WASM_INTERFACE_PUT_PARTIAL_DATA override;
      int32_t GODOT_WASM_INTERFACE_GET_AVAILABLE_BYTES override;
  };
}

#endif
