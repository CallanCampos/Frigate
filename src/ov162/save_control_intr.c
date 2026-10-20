#include "types.h"
#include "gfl/backup_system.h"
#include "gfl/heap.h"
#include "gfl/ui.h"
#include "save/save_control.h"
#include "save/save_control_intr.h"

// Saving a new game in the background. A soft reset cancels it, and it does nothing if save data is already present

enum {
    STATE_IDLE,
    STATE_SAVING,
    STATE_PAUSED,
    STATE_DONE,
};

struct SaveControlIntr {
    SaveControl *save;
    u8 state;
    // The state to go back to after a pause
    u8 resumeState;
    u8 pauseRequested;
    u8 disabled;
    // The save control's flags, cleared while saving and put back afterwards
    u32 flag4;
    u32 flag5;
    u32 flag6;
    u32 flag7;
};

static void SaveControlIntr_OnSoftReset(void *work);

SaveControlIntr *SaveControlIntr_Create(HeapID heapId, SaveControl *save) {
    SaveControlIntr *intr = GFL_HeapAllocate(heapId, sizeof(SaveControlIntr), TRUE, "save_control_intr.c", 71);

    intr->save = save;
    if (SaveControl_IsDataAlreadyPresent(save) == TRUE) {
        intr->disabled = TRUE;
    } else {
        GCTX_HIDSetSoftResetCallback(SaveControlIntr_OnSoftReset, intr);
    }
    return intr;
}

void SaveControlIntr_Free(SaveControlIntr *intr) {
    GCTX_HIDSetSoftResetCallback(NULL, NULL);
    GFL_HeapFree(intr);
}

void SaveControlIntr_Start(SaveControlIntr *intr) {
    if (intr->disabled == TRUE) {
        return;
    }
    SaveControl_StartSave(intr->save);
    if (intr->disabled == FALSE) {
        func_02008e04(intr->save);
        intr->flag4 = SaveControl_GetGiftsLoaded(intr->save);
        intr->flag5 = SaveControl_GetGiftsBroken(intr->save);
        intr->flag6 = SaveControl_GetKeysLoaded(intr->save);
        intr->flag7 = SaveControl_GetKeysBroken(intr->save);
        SaveControl_SetGiftsLoaded(intr->save, 0);
        SaveControl_SetGiftsBroken(intr->save, 0);
        SaveControl_SetKeysLoaded(intr->save, 0);
        SaveControl_SetKeysBroken(intr->save, 0);
    }
    intr->state = STATE_SAVING;
}

u32 SaveControlIntr_Update(SaveControlIntr *intr) {
    u32 result;

    if (intr->disabled == TRUE) {
        return 2;
    }
    switch (intr->state) {
    case STATE_SAVING:
        if (intr->pauseRequested == TRUE && getLockIDStatus_inline_stub() == 0) {
            intr->resumeState = intr->state;
            intr->state = STATE_PAUSED;
            return 0;
        }
        result = SaveControl_ContinueSave(intr->save);
        if (result - 2 <= 1) {
            intr->state = STATE_DONE;
            SaveControl_SetGiftsLoaded(intr->save, intr->flag4);
            SaveControl_SetGiftsBroken(intr->save, intr->flag5);
            SaveControl_SetKeysLoaded(intr->save, intr->flag6);
            SaveControl_SetKeysBroken(intr->save, intr->flag7);
        }
        return result;
    case STATE_PAUSED:
        if (intr->pauseRequested == FALSE) {
            intr->state = intr->resumeState;
        }
        return 0;
    case STATE_DONE:
        return 2;
    }
    return 0;
}

void SaveControlIntr_Pause(SaveControlIntr *intr) {
    if (intr->state != STATE_DONE) {
        intr->pauseRequested = TRUE;
    }
}

void SaveControlIntr_Resume(SaveControlIntr *intr) {
    if (intr->state != STATE_DONE) {
        intr->pauseRequested = FALSE;
    }
}

BOOL SaveControlIntr_IsPausedOrDone(SaveControlIntr *intr) {
    if (intr->disabled == TRUE || intr->state == STATE_DONE) {
        return TRUE;
    }
    if (intr->state == STATE_PAUSED) {
        return TRUE;
    }
    return FALSE;
}

void SaveControlIntr_Nop(SaveControlIntr *intr) {
}

BOOL SaveControlIntr_IsDone(SaveControlIntr *intr) {
    if (intr->disabled == TRUE) {
        return TRUE;
    }
    if (intr->state == STATE_DONE) {
        return TRUE;
    }
    return FALSE;
}

static void SaveControlIntr_OnSoftReset(void *work) {
    SaveControlIntr *intr = work;

    if (intr->disabled == FALSE && intr->state != STATE_DONE) {
        SaveControl_CancelSave(intr->save);
        intr->disabled = TRUE;
    }
}
