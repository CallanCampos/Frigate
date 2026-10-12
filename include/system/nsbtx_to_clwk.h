#ifndef POKEBW2_SYSTEM_NSBTX_TO_CLWK_H
#define POKEBW2_SYSTEM_NSBTX_TO_CLWK_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// Loads a texture of a model file, width by height tiles, into the actor's characters and palette
void NsbtxToClwk_LoadTexture(ClActor *actor, u32 arcId, u32 fileId, u8 texture, u16 width, u16 height, u32 a6, u32 a7,
                   HeapID heapId);
// Loads a texture of a model file, width by height tiles, into the actor's characters at offset and its palette.
// With a palette fade, the colors go to its OBJ buffers at the given palette instead of to VRAM
void NsbtxToClwk_LoadTextureEx(ClActor *actor, u32 arcId, u32 fileId, u8 texture, u16 width, u16 height, u32 offset, u32 vramType,
                   PaletteFade *fade, u8 palette, HeapID heapId);

#endif // POKEBW2_SYSTEM_NSBTX_TO_CLWK_H
