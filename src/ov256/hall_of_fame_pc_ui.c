#include "types.h"
#include "app/hall_of_fame_pc_main.h"
#include "app/hall_of_fame_pc_obj.h"
#include "app/hall_of_fame_pc_ui.h"
#include "gfl/key.h"
#include "gfl/touchpanel.h"

// The Hall of Fame PC's input: the buttons, the keys and the Pokémon touched on the ring. The ROM doesn't name this
// file; the name is a guess, after box2_ui.c

static u32 HallOfFamePCUi_HitPokemon(HallOfFamePCWork *wk);

// The buttons, in the order of HALL_OF_FAME_PC_UI_*
static const TouchRect sHallOfFamePCUiTouchRects[] = {
    { 168, 191, 8, 31 },    { 168, 191, 88, 111 },       { 168, 191, 200, 223 },
    { 168, 191, 232, 255 }, { TOUCH_RECT_END, 0, 0, 0 },
};

u32 HallOfFamePCUi_PageMain(HallOfFamePCWork *wk) {
    u32 ret = GFL_HIDGetTouchPressedRect(sHallOfFamePCUiTouchRects);

    if (ret == TOUCH_RECT_NONE) {
        ret = HallOfFamePCUi_HitPokemon(wk);
        if (ret == HALL_OF_FAME_PC_UI_NONE) {
            if (GCTX_HIDGetTypedKeys() & (PAD_KEY_LEFT | PAD_BUTTON_L)) {
                return HALL_OF_FAME_PC_UI_LEFT;
            }
            if (GCTX_HIDGetTypedKeys() & (PAD_KEY_RIGHT | PAD_BUTTON_R)) {
                return HALL_OF_FAME_PC_UI_RIGHT;
            }
            if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
                return HALL_OF_FAME_PC_UI_DECIDE;
            }
            if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
                return HALL_OF_FAME_PC_UI_RETURN;
            }
            if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
                return HALL_OF_FAME_PC_UI_CLOSE;
            }
            return HALL_OF_FAME_PC_UI_NONE;
        }
    }
    return ret;
}

u32 HallOfFamePCUi_PokeMain(HallOfFamePCWork *wk) {
    u32 ret = GFL_HIDGetTouchPressedRect(sHallOfFamePCUiTouchRects);

    if (ret != HALL_OF_FAME_PC_UI_RETURN) {
        ret = HallOfFamePCUi_HitPokemon(wk);
        if (ret == HALL_OF_FAME_PC_UI_NONE) {
            if (GCTX_HIDGetTypedKeys() & (PAD_KEY_LEFT | PAD_BUTTON_L)) {
                return HALL_OF_FAME_PC_UI_LEFT;
            }
            if (GCTX_HIDGetTypedKeys() & (PAD_KEY_RIGHT | PAD_BUTTON_R)) {
                return HALL_OF_FAME_PC_UI_RIGHT;
            }
            if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
                return HALL_OF_FAME_PC_UI_RETURN;
            }
            return HALL_OF_FAME_PC_UI_NONE;
        }
    }
    return ret;
}

// The Pokémon touched this frame as a HALL_OF_FAME_PC_UI_POKE, the one in front where several overlap
static u32 HallOfFamePCUi_HitPokemon(HallOfFamePCWork *wk) {
    u32 tpX, tpY;
    u32 i;
    s16 base;
    s16 hit;
    s16 hitY;
    s16 x, y;
    s16 left, right, top, bottom;

    if (GFL_HIDGetTouchPressedPoint(&tpX, &tpY) == FALSE) {
        return HALL_OF_FAME_PC_UI_NONE;
    }

    base = HallOfFamePCObj_GetPokeBase(wk);
    hit = -1;
    for (i = base; i < base + HALL_OF_FAME_TEAM_SIZE; i++) {
        if (wk->actors[i] == NULL) {
            continue;
        }
        HallOfFamePCObj_GetPos(wk, i, &x, &y);
        left = x - 48;
        right = x + 48;
        top = y - 48;
        bottom = y + 48;
        if (left < 0) {
            left = 0;
        }
        if (right > 255) {
            right = 255;
        }
        if (top < 0) {
            top = 0;
        }
        if (bottom > 255) {
            bottom = 255;
        }
        if (tpX >= left && tpX < right && tpY >= top && tpY < bottom) {
            if (hit == -1) {
                hitY = y;
                hit = i - base;
            } else if (y > hitY) {
                hitY = y;
                hit = i - base;
            }
        }
    }

    if (hit != -1) {
        return HALL_OF_FAME_PC_UI_POKE + hit % HALL_OF_FAME_TEAM_SIZE;
    }
    return HALL_OF_FAME_PC_UI_NONE;
}
