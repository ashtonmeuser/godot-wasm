#ifndef WASI_PREVIEW_1_EXTENSION_H
#define WASI_PREVIEW_1_EXTENSION_H

#include "extension.h"

namespace godot {
  namespace godot_wasm {
    class WasiPreview1Extension: public Extension {
      public:
        WasiPreview1Extension(Wasm* wasm);
    };
  }
}

#endif
