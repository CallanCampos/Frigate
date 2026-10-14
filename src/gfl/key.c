#include "types.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/std.h"
#include "gfl/ui.h"
#include "nitro/os.h"

#define reg_PAD_KEYINPUT (*(vu16 *)0x04000130)
// The buttons and keys that PAD_Read reports, X and Y among them
#define PAD_ALL_MASK 0x2fff

// What a remapping does to its keys
enum {
    KEY_REMAP_END,
    // Also reports from as to
    KEY_REMAP_ADD,
    // Swaps from and to
    KEY_REMAP_SWAP,
    // No longer reports from
    KEY_REMAP_REMOVE,
};

typedef struct {
    u32 from;
    u32 to;
    u8 mode;
} KeyRemap;

struct KeypadManager {
    const KeyRemap *remaps;
    u32 unk4;
    u32 remapIndex;
    u32 prevHeld;
    u32 pressedRaw;
    u32 typedRaw;
    u32 held;
    u32 pressed;
    u32 typed;
    // The keys of two frames, at 30 frames per second, and of the frame before
    u32 held30;
    u32 pressed30;
    u32 typed30;
    u32 heldNext;
    u32 pressedNext;
    u32 typedNext;
    u32 repeatCounter;
    // The frames between repeats of a held key, and before the first
    u32 repeatWait;
    u32 repeatStart;
    u32 unk48;
};

static void GFL_HIDUpdateKeypad(SystemUI *ui);
static void keypressInterpreter(KeypadManager *keypad);
static void setFramecounts(SystemUI *ui, u32 repeatWait, u32 repeatStart);
static void setTempSpaceToKeyBlock(SystemUI *ui, const void *remaps);
static u32 GFL_HIDGetPressedKeys(SystemUI *ui);
static u32 GFL_HIDGetHeldKeys(SystemUI *ui);
static u32 GFL_HIDGetTypedKeys(SystemUI *ui);
static void GFL_HIDGetRepeat(SystemUI *ui, u32 *repeatWait, u32 *repeatStart);

static inline u16 PAD_Read(void) {
    return (u16)(((reg_PAD_KEYINPUT | *(vu16 *)HW_BUTTON_XY_BUF) ^ PAD_ALL_MASK) & PAD_ALL_MASK);
}

KeypadManager *GFL_HIDCreateKeypadManager(HeapID heapId) {
    KeypadManager *keypad = GFL_HeapAllocate(heapId, sizeof(KeypadManager), FALSE, "key.c", 68);

    sys_memset(keypad, 0, sizeof(KeypadManager));
    keypad->repeatWait = 8;
    keypad->repeatStart = 15;
    return keypad;
}

static void GFL_HIDUpdateKeypad(SystemUI *ui) {
    KeypadManager *keypad = GFL_HIDGetKeypadManager(ui);
    u16 keys;

    if (PAD_DetectFold()) {
        keypad->pressed = 0;
        keypad->held = 0;
        keypad->typed = 0;
        keypad->pressed30 = 0;
        keypad->held30 = 0;
        keypad->typed30 = 0;
        return;
    }
    keys = PAD_Read();
    // Up cancels down, and left cancels right
    keys = keys & ~((keys & PAD_KEY_UP) << 1) & ~((keys & PAD_KEY_LEFT) >> 1);
    keypad->pressedRaw = keys & (keys ^ keypad->prevHeld);
    keypad->typedRaw = keys & (keys ^ keypad->prevHeld);
    if (keys != 0 && keypad->prevHeld == keys) {
        if (--keypad->repeatCounter == 0) {
            keypad->typedRaw = keys;
            keypad->repeatCounter = keypad->repeatWait;
        }
    } else {
        keypad->repeatCounter = keypad->repeatStart;
    }
    keypad->prevHeld = keys;
    keypad->pressed = keypad->pressedRaw;
    keypad->held = keys;
    keypad->typed = keypad->typedRaw;
    keypressInterpreter(keypad);
    keypad->pressedNext |= keypad->pressed;
    keypad->heldNext |= keypad->held;
    keypad->typedNext |= keypad->typed;
    if (ui->frameCount % 2 == 0) {
        keypad->pressed30 = keypad->pressedNext;
        keypad->held30 = keypad->heldNext;
        keypad->typed30 = keypad->typedNext;
        keypad->pressedNext = 0;
        keypad->heldNext = 0;
        keypad->typedNext = 0;
    }
}

void GCTX_HIDUpdateKeypad(void) {
    GFL_HIDUpdateKeypad(GCTX_HIDGetInstance());
}

// The keys a swap reports: to for from, and from for to
static inline u32 SwapRemappedKeys(const u32 *keys, const KeyRemap *remap) {
    u32 swapped = 0;

    if (*keys & remap->from) {
        swapped |= remap->to;
    }
    if (*keys & remap->to) {
        swapped |= remap->from;
    }
    return swapped;
}

static void keypressInterpreter(KeypadManager *keypad) {
    u32 *typed;
    u32 *held;
    u32 *pressed;
    const KeyRemap *remap;

    if (keypad->remaps == NULL) {
        return;
    }
    remap = &keypad->remaps[keypad->remapIndex];
    pressed = &keypad->pressed;
    held = &keypad->held;
    typed = &keypad->typed;
    for (;; remap++) {
        switch (remap->mode) {
        case KEY_REMAP_END:
            return;
        case KEY_REMAP_ADD:
            if (keypad->pressed & remap->from) {
                *pressed |= remap->to;
            }
            if (keypad->held & remap->from) {
                *held |= remap->to;
            }
            if (keypad->typed & remap->from) {
                *typed |= remap->to;
            }
            break;
        case KEY_REMAP_SWAP:
            *pressed = (*pressed & ((remap->from | remap->to) ^ 0xffff)) | SwapRemappedKeys(&keypad->pressed, remap);
            *held = (*held & ((remap->from | remap->to) ^ 0xffff)) | SwapRemappedKeys(&keypad->held, remap);
            *typed = (*typed & ((remap->from | remap->to) ^ 0xffff)) | SwapRemappedKeys(&keypad->typed, remap);
            break;
        case KEY_REMAP_REMOVE:
            *pressed &= remap->from ^ 0xffff;
            *held &= remap->from ^ 0xffff;
            *typed &= remap->from ^ 0xffff;
            break;
        }
    }
}

static void setFramecounts(SystemUI *ui, u32 repeatWait, u32 repeatStart) {
    KeypadManager *keypad = GFL_HIDGetKeypadManager(ui);

    keypad->repeatWait = repeatWait;
    keypad->repeatStart = repeatStart;
}

void setKeypressFramecounts(u32 repeatWait, u32 repeatStart) {
    setFramecounts(GCTX_HIDGetInstance(), repeatWait, repeatStart);
}

static void setTempSpaceToKeyBlock(SystemUI *ui, const void *remaps) {
    KeypadManager *keypad = GFL_HIDGetKeypadManager(ui);

    keypad->remaps = remaps;
}

void setKeyBlkStarted(const void *remaps) {
    setTempSpaceToKeyBlock(GCTX_HIDGetInstance(), remaps);
}

static u32 GFL_HIDGetPressedKeys(SystemUI *ui) {
    KeypadManager *keypad = GFL_HIDGetKeypadManager(ui);

    if (ui->updateRate == 30) {
        return keypad->pressed30;
    }
    return keypad->pressed;
}

u32 GCTX_HIDGetPressedKeys(void) {
    return GFL_HIDGetPressedKeys(GCTX_HIDGetInstance());
}

static u32 GFL_HIDGetHeldKeys(SystemUI *ui) {
    KeypadManager *keypad = GFL_HIDGetKeypadManager(ui);

    if (ui->updateRate == 30) {
        return keypad->held30;
    }
    return keypad->held;
}

u32 GCTX_HIDGetHeldKeys(void) {
    return GFL_HIDGetHeldKeys(GCTX_HIDGetInstance());
}

static u32 GFL_HIDGetTypedKeys(SystemUI *ui) {
    KeypadManager *keypad = GFL_HIDGetKeypadManager(ui);

    if (ui->updateRate == 30) {
        return keypad->typed30;
    }
    return keypad->typed;
}

u32 GCTX_HIDGetTypedKeys(void) {
    return GFL_HIDGetTypedKeys(GCTX_HIDGetInstance());
}

static void GFL_HIDGetRepeat(SystemUI *ui, u32 *repeatWait, u32 *repeatStart) {
    KeypadManager *keypad = GFL_HIDGetKeypadManager(ui);

    *repeatWait = keypad->repeatWait;
    *repeatStart = keypad->repeatStart;
}

void GCTX_HIDGetRepeat(u32 *repeatWait, u32 *repeatStart) {
    GFL_HIDGetRepeat(GCTX_HIDGetInstance(), repeatWait, repeatStart);
}

void GFL_HIDClearKeypadState(SystemUI *ui) {
    KeypadManager *keypad = GFL_HIDGetKeypadManager(ui);

    keypad->held30 = 0;
    keypad->pressed30 = 0;
    keypad->typed30 = 0;
    keypad->heldNext = 0;
    keypad->pressedNext = 0;
    keypad->typedNext = 0;
}

void func_0203df90(SystemUI *ui, u32 pressed, u32 pressed30) {
    KeypadManager *keypad = GFL_HIDGetKeypadManager(ui);

    keypad->pressed30 = pressed30;
    keypad->pressed = pressed;
}

void func_0203dfa0(SystemUI *ui, u32 held, u32 held30) {
    KeypadManager *keypad = GFL_HIDGetKeypadManager(ui);

    keypad->held30 = held30;
    keypad->held = held;
}

void func_0203dfb0(SystemUI *ui, u32 typed, u32 typed30) {
    KeypadManager *keypad = GFL_HIDGetKeypadManager(ui);

    keypad->typed30 = typed30;
    keypad->typed = typed;
}
