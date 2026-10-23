#include "types.h"
#include "field/el_scoreboard.h"
#include "field/field.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "struct_decls.h"

struct ElScoreboard {
    u32 unk0;
    u16 unk4;
    u16 unk6;
    u32 unk8;
    u32 unkC;
    u32 unk10;
    u32 unk14;
    u32 frame;
};

struct ElScoreboardPaletteTarget {
    u32 palette;
    u32 vramOffset;
};

// Declared in reverse, as the compiler emits them in reverse order
static const u16 sScoreboardPalette3[4] = { 0x0000, 0x18c6, 0x0d73, 0x021f };
static const u16 sScoreboardPalette2[4] = { 0x0000, 0x2108, 0x0d73, 0x021f };
static const u16 sScoreboardPalette1[4] = { 0x0000, 0x18c6, 0x0df3, 0x031f };
static const u16 sScoreboardPalette0[4] = { 0x0000, 0x2108, 0x0df3, 0x031f };

static const u16 *sScoreboardPalettes[4] = { sScoreboardPalette3, sScoreboardPalette2, sScoreboardPalette1,
                                             sScoreboardPalette0 };

void ElScoreboard_UploadPalette(ElScoreboardPaletteTarget *target, s32 frame);

ElScoreboard *ElScoreboard_Create(void *texture, const char *texName, const char *plName, const StrBuf *text, u16 a4,
                                  u16 a5, HeapID heapId) {
    G3DTextDrawResource resource;
    ElScoreboard *board = GFL_HeapAllocate(heapId, sizeof(ElScoreboard), TRUE, "el_scoreboard.c", 412);

    board->unk0 = 0;
    board->unk4 = G3DTextDraw_GetTexDimCount() - 1;
    board->unk6 = G3DTextDraw_GetTexDimCount() - 1;
    board->unk10 = -1;
    board->unk14 = -1;
    if (G3DTextDraw_CreateResource(texture, texName, 0, plName, text, a4, a5, 0xc40, heapId, &resource)) {
        board->unk8 = resource.unk4;
        board->unkC = resource.unk8;
        board->unk10 = resource.unkC;
        board->unk4 = resource.unk0;
        board->unk6 = resource.unk2;
        board->unk14 = resource.unk10;
        board->frame = 0;
    }
    return board;
}

void ElScoreboard_Free(ElScoreboard *board) {
    GFL_HeapFree(board);
}

void ElScoreboard_Update(ElScoreboard *board) {
    ElScoreboardPaletteTarget target;

    board->frame++;
    if (board->unk14 != -1) {
        target.palette = board->unkC;
        target.vramOffset = board->unk14;
        ElScoreboard_UploadPalette(&target, board->frame);
    }
}

void ElScoreboard_UploadPalette(ElScoreboardPaletteTarget *target, s32 frame) {
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_3D_TEX_PLTT, target->vramOffset + (u16)target->palette * 8,
                                       sScoreboardPalettes[(frame & 0x1f) / 8], 8);
}
