#ifndef WASM_MEMORY_H
#define WASM_MEMORY_H

#include "defs.h"

#ifdef GODOT_MODULE
  #define SUPER_CLASS StreamPeer
  #define INTERFACE_DECLARE
#else
  #define SUPER_CLASS StreamPeerGDNative
  #define INTERFACE_DECLARE godot_net_stream_peer interface
#endif

namespace godot {
  class WasmMemory : public SUPER_CLASS {
    GDCLASS(WasmMemory, SUPER_CLASS);

    private:
      INTERFACE_DECLARE;
      wasm_memory_t* memory;
      uint32_t pointer;

    public:
      static void REGISTRATION_METHOD();
      WasmMemory();
      ~WasmMemory();
      void _init();
      void set_memory(const wasm_memory_t* memory);
      wasm_memory_t* get_memory() const;
      Dictionary inspect() const;
      godot_error grow(uint32_t pages);
      Ref<WasmMemory> seek(int p_pos);
      uint32_t get_position() const;
      godot_error get_data(uint8_t* buffer, int bytes);
      godot_error get_partial_data(uint8_t* buffer, int bytes, int& received);
      godot_error put_data(const uint8_t* buffer, int bytes);
      godot_error put_partial_data(const uint8_t* buffer, int bytes, int& sent);
      int get_available_bytes() const;
  };
}

#endif
