#include "g2di_load.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"

// NitroSystem's g2d_NFT_load.c: finds the font information in a loaded NFTR file and turns the file's offsets into
// pointers in place. A version 1.0 file's glyphs have no rotation flags

// A file's header checks, which the code shows as inlines: the inner two build their result with an if and return it as
// a u32, and NNS_G2dIsBinFileValid tests the pointer once more before them
static inline u32 NNSi_G2dIsBinFileSignatureValid(const NNSG2dBinaryFileHeader *pHeader, u32 signature) {
    if (pHeader != NULL && pHeader->signature == signature) {
        return TRUE;
    }
    return FALSE;
}

static inline u32 NNSi_G2dIsBinFileVersionValid(const NNSG2dBinaryFileHeader *pHeader, u16 version) {
    if (pHeader != NULL && pHeader->version >= version) {
        return TRUE;
    }
    return FALSE;
}

static inline BOOL NNS_G2dIsBinFileValid(const NNSG2dBinaryFileHeader *pHeader, u32 signature, u16 version) {
    if (pHeader != NULL) {
        return NNSi_G2dIsBinFileSignatureValid(pHeader, signature) && NNSi_G2dIsBinFileVersionValid(pHeader, version);
    }
    return FALSE;
}

BOOL NNSi_G2dGetUnpackedFont(void *pNftrFile, NNSG2dFontInformation **ppFont) {
    NNSG2dBinaryBlockHeader *pBlk;
    BOOL bOldVer = FALSE;

    if (!NNS_G2dIsBinFileValid(pNftrFile, NNS_G2D_BINFILE_SIG_FONTDATA, 0x102) &&
        !NNS_G2dIsBinFileValid(pNftrFile, NNS_G2D_BINFILE_SIG_FONTDATA, 0x101)) {
        if (!NNS_G2dIsBinFileValid(pNftrFile, NNS_G2D_BINFILE_SIG_FONTDATA, 0x100)) {
            OS_Terminate();
        }
        bOldVer = TRUE;
    }

    NNSi_G2dUnpackNFT(pNftrFile);
    pBlk = NNS_G2dFindBinaryBlock(pNftrFile, NNS_G2D_BINBLK_SIG_FINFDATA);
    if (pBlk == NULL) {
        *ppFont = NULL;
        return FALSE;
    }
    *ppFont = (NNSG2dFontInformation *)(pBlk + 1);
    if (bOldVer) {
        (*ppFont)->pGlyph->flags = 0;
    }
    return TRUE;
}

void NNSi_G2dUnpackNFT(NNSG2dBinaryFileHeader *pHeader) {
    NNSG2dBinaryBlockHeader *pBlk = (NNSG2dBinaryBlockHeader *)((u8 *)pHeader + pHeader->headerSize);
    int i;

    for (i = 0; i < pHeader->dataBlocks; i++) {
        switch (pBlk->kind) {
        case NNS_G2D_BINBLK_SIG_FINFDATA: {
            NNSG2dFontInformation *pInfo = (NNSG2dFontInformation *)(pBlk + 1);

            NNS_G2D_UNPACK_OFFSET_PTR(pInfo->pGlyph, pHeader);
            if (pInfo->pWidth != NULL) {
                NNS_G2D_UNPACK_OFFSET_PTR(pInfo->pWidth, pHeader);
            }
            if (pInfo->pMap != NULL) {
                NNS_G2D_UNPACK_OFFSET_PTR(pInfo->pMap, pHeader);
            }
            break;
        }
        case NNS_G2D_BINBLK_SIG_CWDHDATA: {
            NNSG2dFontWidth *pWidth = (NNSG2dFontWidth *)(pBlk + 1);

            if (pWidth->pNext != NULL) {
                NNS_G2D_UNPACK_OFFSET_PTR(pWidth->pNext, pHeader);
            }
            break;
        }
        case NNS_G2D_BINBLK_SIG_CMAPDATA: {
            NNSG2dFontCodeMap *pMap = (NNSG2dFontCodeMap *)(pBlk + 1);

            if (pMap->pNext != NULL) {
                NNS_G2D_UNPACK_OFFSET_PTR(pMap->pNext, pHeader);
            }
            break;
        }
        case NNS_G2D_BINBLK_SIG_CGLPDATA:
            break;
        }
        pBlk = (NNSG2dBinaryBlockHeader *)((u8 *)pBlk + pBlk->size);
    }
}
