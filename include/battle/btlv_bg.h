#ifndef POKEBW2_BATTLE_BTLV_BG_H
#define POKEBW2_BATTLE_BTLV_BG_H

// Overlay 168's btlv_bg.c (named by its string), the effects' moves and waves of the battle's background, BG 3. The
// names are ours; swan has none for this file

#include "types.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "struct_decls.h"

BtlvBg *BtlvBg_Create(TCBManager *tcbMgr, HeapID heapId);
void BtlvBg_Delete(BtlvBg *work);
void BtlvBg_SetOffsetReq(BtlvBg *work, s32 x, s32 y);
// Moves BG 3 to x, y in the effect's movement mode, BTLV_EFFTOOL_MOVE_*: BTLV_EFFTOOL_MOVE_STEP relative to the current
// offset, BTLV_EFFTOOL_MOVE_SCROLL by x, y each frame with x mirrored for an odd pos
void BtlvBg_StartMove(BtlvBg *work, u32 pos, u32 mode, s32 x, s32 y, s32 frames, s32 wait, s32 count);
BOOL BtlvBg_IsMoving(BtlvBg *work);

// The waves of BtlvBg_StartWave: the lines shift horizontally (192 lines) or vertically (256)
enum {
    BTLV_BG_WAVE_HORIZONTAL,
    BTLV_BG_WAVE_VERTICAL,
};

// How a wave's amplitude fades over fadeFrames
enum {
    BTLV_BG_WAVE_FADE_NONE,
    BTLV_BG_WAVE_FADE_IN,
    BTLV_BG_WAVE_FADE_OUT,
    BTLV_BG_WAVE_FADE_IN_OUT,
};

void BtlvBg_StartWave(BtlvBg *work, u32 mode, fx32 amplitude, s32 step, s32 frames, u32 fadeMode, s32 fadeFrames);
BOOL BtlvBg_IsWaving(BtlvBg *work);

#endif // POKEBW2_BATTLE_BTLV_BG_H
