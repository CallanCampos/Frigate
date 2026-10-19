#include "types.h"
#include "constants/sound.h"
#include "gfl/key.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "system/app_printsys_common.h"
#include "system/printsys.h"

// The menus' handling of a message printed a character at a time. The file's name is a guess: the ROM has no string
// for it

static BOOL AppPrintsysCommon_IsHeld(u32 flags);
static BOOL AppPrintsysCommon_IsReleased(u32 flags);

void AppPrintsysCommon_Init(AppPrintsysCommon *work, u32 flags) {
    MI_CpuFill8(work, 0, sizeof(AppPrintsysCommon));
    work->flags = flags;
}

BOOL AppPrintsysCommon_Update(AppPrintsysCommon *work, PrintStream *stream) {
    if (stream != NULL) {
        switch (PrintStream_GetState(stream)) {
        case PRINT_STREAM_DONE:
            return TRUE;
        case PRINT_STREAM_PAUSED:
            if (AppPrintsysCommon_IsPressed(work->flags)) {
                GFL_SndSEPlay(SEQ_SE_MESSAGE);
                PrintStream_Continue(stream);
            }
            break;
        case PRINT_STREAM_RUNNING:
            if ((work->flags & APP_PRINTSYS_COMMON_NO_SKIP) == 0) {
                // Only a press after everything was released speeds the message up
                if (work->released == FALSE) {
                    if (AppPrintsysCommon_IsReleased(work->flags)) {
                        work->released = TRUE;
                    }
                } else if (AppPrintsysCommon_IsHeld(work->flags)) {
                    PrintStream_SkipWait(stream, 0);
                }
            }
            break;
        }
        return FALSE;
    }
    return TRUE;
}

BOOL AppPrintsysCommon_IsPressed(u32 flags) {
    BOOL pressed = FALSE;

    if (flags == 0) {
        pressed = TRUE;
    }
    if (flags & APP_PRINTSYS_COMMON_KEYS) {
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            GCTX_HIDSetUsingTouch(FALSE);
            pressed = TRUE;
        }
    }
    if (flags & APP_PRINTSYS_COMMON_TOUCH) {
        if (GFL_HIDIsTouchPressed()) {
            pressed = TRUE;
            GCTX_HIDSetUsingTouch(TRUE);
        }
    }
    return pressed;
}

static BOOL AppPrintsysCommon_IsHeld(u32 flags) {
    BOOL held = FALSE;

    if (flags == 0) {
        held = TRUE;
    }
    if (flags & APP_PRINTSYS_COMMON_KEYS) {
        if (GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            held = TRUE;
        }
    }
    if (flags & APP_PRINTSYS_COMMON_TOUCH) {
        if (GFL_HIDIsTouchHeld()) {
            held = TRUE;
        }
    }
    return held;
}

static BOOL AppPrintsysCommon_IsReleased(u32 flags) {
    BOOL released = FALSE;

    if (flags == 0) {
        released = TRUE;
    }
    if (flags & APP_PRINTSYS_COMMON_KEYS) {
        if (GCTX_HIDGetHeldKeys() == 0) {
            released = TRUE;
        }
    }
    if (flags & APP_PRINTSYS_COMMON_TOUCH) {
        if (GFL_HIDIsTouchHeld() == FALSE) {
            released = TRUE;
        }
    }
    return released;
}
