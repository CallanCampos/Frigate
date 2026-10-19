#include "types.h"
#include "gfl/button_man.h"
#include "gfl/heap.h"
#include "gfl/touchpanel.h"

enum {
    BUTTON_RELEASED,
    BUTTON_PRESSED,
};

typedef struct {
    u8 state;
    // Whether the button is being touched
    u8 touched;
    // The frames the button has been held
    u16 holdFrames;
} Button;

struct ButtonMan {
    const TouchRect *rects;
    u32 count;
    ButtonManCallback callback;
    void *work;
    Button *buttons;
};

// Steps a button of a state, and returns its event or BMN_EVENT_NONE
typedef s32 (*ButtonStateFunc)(Button *button, BOOL touching, BOOL touchedNow);

static void ButtonInit(Button *button);
static s32 ButtonReleased(Button *button, BOOL touching, BOOL touchedNow);
static s32 ButtonPressed(Button *button, BOOL touching, BOOL touchedNow);
static void ButtonSetState(Button *button, u8 state);

static const ButtonStateFunc sButtonStateFuncs[] = {
    ButtonReleased,
    ButtonPressed,
};

ButtonMan *GFL_BMN_Create(const TouchRect *rects, ButtonManCallback callback, void *work, u32 heapId) {
    ButtonMan *mgr;
    int count;
    u32 i;

    for (count = 0; count < 0xff; count++) {
        if (rects[count].top == TOUCH_RECT_END) {
            break;
        }
    }
    mgr = GFL_HeapAllocate(heapId, sizeof(ButtonMan), FALSE, "button_man.c", 107);
    if (mgr != NULL) {
        mgr->rects = rects;
        mgr->count = count;
        mgr->callback = callback;
        mgr->work = work;
        mgr->buttons = GFL_HeapAllocate(heapId, count * sizeof(Button), FALSE, "button_man.c", 115);
        if (mgr->buttons != NULL) {
            for (i = 0; i < count; i++) {
                ButtonInit(&mgr->buttons[i]);
            }
        } else {
            GFL_HeapFree(mgr);
            mgr = NULL;
        }
    }
    return mgr;
}

static void ButtonInit(Button *button) {
    button->state = BUTTON_RELEASED;
    button->touched = FALSE;
    button->holdFrames = 0;
}

void GFL_BMN_Delete(ButtonMan *mgr) {
    GFL_HeapFree(mgr->buttons);
    GFL_HeapFree(mgr);
}

BOOL GFL_BMN_Main(ButtonMan *mgr) {
    BOOL touching;
    BOOL touchedButton = FALSE;
    BOOL touchedNow;
    u32 i;

    touching = GFL_HIDIsTouchHeld();
    if (touching) {
        s32 held = GFL_HIDGetTouchHeldRect(mgr->rects);
        s32 touched = GFL_HIDGetTouchPressedRect(mgr->rects);

        touchedNow = GFL_HIDIsTouchPressed();
        for (i = 0; i < mgr->count; i++) {
            Button *button = &mgr->buttons[i];

            if (button->touched) {
                button->touched = held == i;
            } else {
                button->touched = touched == i;
                if (mgr->buttons[i].touched) {
                    touchedButton = TRUE;
                }
            }
        }
    } else {
        touchedNow = FALSE;
        for (i = 0; i < mgr->count; i++) {
            mgr->buttons[i].touched = FALSE;
        }
    }
    for (i = 0; i < mgr->count; i++) {
        Button *button = &mgr->buttons[i];
        s32 event = sButtonStateFuncs[button->state](button, touching, touchedNow);

        if (event != BMN_EVENT_NONE) {
            mgr->callback(i, event, mgr->work);
        }
    }
    return touchedButton;
}

static s32 ButtonReleased(Button *button, BOOL touching, BOOL touchedNow) {
    if (button->touched && touchedNow) {
        ButtonSetState(button, BUTTON_PRESSED);
        return BMN_EVENT_TOUCH;
    }
    return BMN_EVENT_NONE;
}

static s32 ButtonPressed(Button *button, BOOL touching, BOOL touchedNow) {
    if (button->touched) {
        if (button->holdFrames < 0xffff) {
            button->holdFrames++;
        }
        return BMN_EVENT_HOLD;
    }
    if (touching) {
        ButtonSetState(button, BUTTON_RELEASED);
        return BMN_EVENT_SLIDEOUT;
    }
    ButtonSetState(button, BUTTON_RELEASED);
    return BMN_EVENT_RELEASE;
}

static void ButtonSetState(Button *button, u8 state) {
    button->state = state;
    button->holdFrames = 0;
}
