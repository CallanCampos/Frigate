// The pointing finger that the catching demonstration and the battle bag show tapping the touch screen. The name is
// the ROM's string, from GFL_HeapAllocate's assert. BtlvFingerCursor_Create is swan's name; the other names are ours

#include "battle/btlv_finger_cursor.h"
#include "types.h"
#include "battle/btl_pokeparam.h"
#include "battle/btlv_effect.h"
#include "gfl/arc.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "system/palanm.h"

// The finger's work, 0x30 bytes
struct BtlvFingerCursor {
    ClActUnit *unit; // 0x00
    ClActor *actor;  // 0x04
    TCB *task;       // 0x08
    u32 chars;       // 0x0c
    u32 palette;     // 0x10
    u32 cellAnims;   // 0x14
    s32 seq;         // 0x18  0 waits, 1 taps, 2 hides, 3 done
    u32 wait;        // 0x1c  frames before the tap
    s32 tapFrame;    // 0x20  the frame of the tap animation that touches the screen
    u32 hideWait;    // 0x24  frames the finger stays on the screen after touching it
    BOOL touched;    // 0x28
    u16 heapId;      // 0x2c
};

static void BtlvFingerCursor_AnimCallback(uptr param, fx32 frame);
static void BtlvFingerCursor_Task(TCB *tcb, void *data);

static const ClActorSetupEx data_ov168_021f3ef4 = { { 0, 0, 0, 0, 0 }, { 0, 0 }, FX32_ONE, FX32_ONE, 0, 2 };

BtlvFingerCursor *BtlvFingerCursor_Create(PaletteFade *fade, u32 paletteRow, HeapID heapId) {
    BtlvFingerCursor *cursor = GFL_HeapAllocate(heapId, sizeof(BtlvFingerCursor), TRUE, "btlv_finger_cursor.c", 0x52);
    ArcTool *arc = GFL_ArcSysCreateFileHandle(11, HEAPID_TAIL(heapId));

    cursor->heapId = heapId;
    cursor->chars = func_0204b81c(arc, 0x1e7, FALSE, CLACT_VRAM_SUB, heapId);
    cursor->cellAnims = func_0204bde0(arc, 0x1e9, 0x1ea, heapId);
    cursor->palette = func_0204bbb8(arc, 0x1e8, CLACT_VRAM_SUB, paletteRow * 32, 0, 1, heapId);
    PaletteFade_LoadFromVRAM(fade, PALFADE_VRAM_SUB_OBJ, func_0204bdc0(cursor->palette, TRUE) / 2, 0x20);
    cursor->unit = func_0204bf1c(1, 0, heapId);
    GFL_ArcToolFree(arc);
    cursor->task = GFL_TCBMgrAddTask(BtlvEffect_GetTCBManager(), BtlvFingerCursor_Task, cursor, 0);
    return cursor;
}

void BtlvFingerCursor_Delete(BtlvFingerCursor *cursor) {
    GFL_TCBRemove(cursor->task);
    BtlvFingerCursor_RemoveActor(cursor);
    func_0204b98c(cursor->chars);
    func_0204be64(cursor->cellAnims);
    func_0204bcd0(cursor->palette);
    func_0204bf98(cursor->unit);
    GFL_HeapFree(cursor);
}

BOOL BtlvFingerCursor_Start(BtlvFingerCursor *cursor, s32 x, s32 y, u32 wait, s32 tapFrame, u32 hideWait) {
    ClActorSetupEx setup = data_ov168_021f3ef4;
    ClActorCallback callback;

    if (cursor->touched) {
        return FALSE;
    }
    if (cursor->actor != NULL) {
        BtlvFingerCursor_RemoveActor(cursor);
        cursor->actor = NULL;
    }
    setup.base.x = x;
    setup.base.y = y;
    cursor->actor =
        func_0204c0a4(cursor->unit, cursor->chars, cursor->palette, cursor->cellAnims, &setup, 1, cursor->heapId);
    func_0204c488(cursor->actor, 0);
    func_0204c4d4(cursor->actor, 0);
    func_0204c520(cursor->actor, TRUE);
    func_0204c124(cursor->actor, TRUE);
    if (wait != 0) {
        cursor->wait = wait;
    } else {
        cursor->wait = 1;
    }
    if (hideWait != 0) {
        cursor->hideWait = hideWait;
    } else {
        cursor->hideWait = 1;
    }
    cursor->seq = 0;
    cursor->tapFrame = tapFrame;
    cursor->touched = FALSE;
    callback.param = (uptr)cursor;
    callback.type = CLACT_CALLBACK_LAST_FRAME;
    callback.frame = 0;
    callback.func = BtlvFingerCursor_AnimCallback;
    func_0204c5b0(cursor->actor, &callback);
    return TRUE;
}

void BtlvFingerCursor_RemoveActor(BtlvFingerCursor *cursor) {
    if (cursor->actor != NULL) {
        func_0204c108(cursor->actor);
        cursor->actor = NULL;
    }
}

BOOL BtlvFingerCursor_IsTouched(BtlvFingerCursor *cursor) {
    return cursor->touched;
}

// The actor's animation callback, at the end of each animation and on the tap's frame
static void BtlvFingerCursor_AnimCallback(uptr param, fx32 frame) {
    BtlvFingerCursor *cursor = (BtlvFingerCursor *)param;

    switch (cursor->seq) {
    case 0:
        if (--cursor->wait == 0) {
            ClActorCallback callback;

            func_0204c488(cursor->actor, 1);
            cursor->seq++;
            callback.param = (uptr)cursor;
            callback.type = CLACT_CALLBACK_FRAME;
            callback.frame = cursor->tapFrame;
            callback.func = BtlvFingerCursor_AnimCallback;
            func_0204c5b0(cursor->actor, &callback);
        }
        break;
    case 1: {
        ClActorCallback callback;

        cursor->touched = TRUE;
        cursor->seq++;
        callback.param = (uptr)cursor;
        callback.type = CLACT_CALLBACK_EVERY_FRAME;
        callback.frame = 0;
        callback.func = BtlvFingerCursor_AnimCallback;
        func_0204c5b0(cursor->actor, &callback);
        break;
    }
    case 2:
        func_0204c124(cursor->actor, FALSE);
        cursor->touched = FALSE;
        cursor->seq++;
        break;
    case 3:
        break;
    }
}

static void BtlvFingerCursor_Task(TCB *tcb, void *data) {
    BtlvFingerCursor *cursor = data;

    if (cursor->actor != NULL && cursor->touched && --cursor->hideWait == 0) {
        func_0204c124(cursor->actor, FALSE);
        cursor->touched = FALSE;
    }
}
