#include "nnsys/g3d.h"

// NitroSystem's G3D accessors for animation resources (res_struct_accessor_anm.c): the animations of a file, and the
// names and frames of a texture pattern animation (BTP0). The functions have NitroSystem's names, and g3d.h notes
// swan's. GetTexPatAnmData is static because only this file calls it

static const NNSG3dResTexPatAnmData *GetTexPatAnmData(const NNSG3dResTexPatAnm *anm, u32 idx);

void *NNS_G3dGetAnmByIdx(const NNSG3dResFileHeader *header, u32 idx) {
    if (header != NULL) {
        const u32 *blockOfs = (const u32 *)((const u8 *)header + header->headerSize);
        const NNSG3dResAnmSet *set = (const NNSG3dResAnmSet *)((const u8 *)header + blockOfs[0]);
        const u32 *ofs = NNS_G3dGetResDataByIdx(&set->dict, idx);

        if (ofs != NULL) {
            return (u8 *)set + *ofs;
        }
    }
    return NULL;
}

// The name of a texture of the animation, or of a palette
const NNSG3dResName *NNS_G3dGetTexPatAnmTexNameByIdx(const NNSG3dResTexPatAnm *anm, u32 idx) {
    if (anm != NULL && idx < anm->numTex) {
        return (const NNSG3dResName *)((const u8 *)anm + anm->ofsTexName) + idx;
    }
    return NULL;
}

const NNSG3dResName *NNS_G3dGetTexPatAnmPlttNameByIdx(const NNSG3dResTexPatAnm *anm, u32 idx) {
    if (anm != NULL && idx < anm->numPltt) {
        return (const NNSG3dResName *)((const u8 *)anm + anm->ofsPlttName) + idx;
    }
    return NULL;
}

// The texture and palette that the material animation at idx shows at a frame: the last pair of its frame values that
// starts at or before the frame, found from where the frame is in the animation's length
const NNSG3dResTexPatAnmFV *NNSi_G3dGetTexPatAnmFV(const NNSG3dResTexPatAnm *anm, u32 idx, u32 frame) {
    const NNSG3dResTexPatAnmData *data = GetTexPatAnmData(anm, idx);
    const NNSG3dResTexPatAnmFV *fv = (const NNSG3dResTexPatAnmFV *)((const u8 *)anm + data->offset);
    u32 i = data->ratioDataFrame * frame >> FX32_SHIFT;

    while (i != 0 && fv[i].idxFrame >= frame) {
        i--;
    }
    while (i + 1 < data->numFV && fv[i + 1].idxFrame <= frame) {
        i++;
    }
    return &fv[i];
}

// The data of the material animation at idx
static const NNSG3dResTexPatAnmData *GetTexPatAnmData(const NNSG3dResTexPatAnm *anm, u32 idx) {
    return NNS_G3dGetResDataByIdx(&anm->dict, idx);
}
