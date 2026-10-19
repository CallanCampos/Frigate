#include "types.h"
#include "app/ui/frame_list.h"
#include "app/zknlist_main.h"
#include "app/zknlist_ui.h"
#include "gfl/key.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "save/pokedex.h"

// The keys and touches of the Pokédex's list, besides those of the list itself. The ROM doesn't name this file;
// zknlist_ui.c is a guess after the search screen's zknsearch_ui.c

// The picture, the icons of the rows, the buttons of the touch bar and the buttons of the Pokédexes, with the
// SELECT button
static const TouchRect sZknListTouchHabitat[] = {
    { 0, 95, 0, 95 },       { 0, 23, 96, 127 },          { 24, 47, 96, 127 },
    { 48, 71, 96, 127 },    { 72, 95, 96, 127 },         { 96, 119, 96, 127 },
    { 120, 143, 96, 127 },  { 144, 167, 96, 127 },       { 168, 191, 72, 135 },
    { 168, 191, 0, 71 },    { 168, 191, 184, 207 },      { 168, 191, 208, 231 },
    { 168, 191, 232, 255 }, { 98, 122, 0, 64 },          { 121, 145, 0, 64 },
    { 144, 168, 0, 64 },    { TOUCH_RECT_END, 0, 0, 0 },
};

// The same before the habitat list, when the START button is where the SELECT button goes
static const TouchRect sZknListTouchNoHabitat[] = {
    { 0, 95, 0, 95 },       { 0, 23, 96, 127 },          { 24, 47, 96, 127 },
    { 48, 71, 96, 127 },    { 72, 95, 96, 127 },         { 96, 119, 96, 127 },
    { 120, 143, 96, 127 },  { 144, 167, 96, 127 },       { 168, 191, 40, 103 },
    { 168, 191, 0, 71 },    { 168, 191, 184, 207 },      { 168, 191, 208, 231 },
    { 168, 191, 232, 255 }, { 98, 122, 0, 64 },          { 121, 145, 0, 64 },
    { 144, 168, 0, 64 },    { TOUCH_RECT_END, 0, 0, 0 },
};

int ZknListUI_Main(ZknListWork *wk) {
    int ret;

    if (PokeDex_IsHabitatListEnabled(wk->param->pokedex) == TRUE) {
        ret = func_0203da0c(sZknListTouchHabitat);
    } else {
        ret = func_0203da0c(sZknListTouchNoHabitat);
    }
    if (ret != -1) {
        if (ret == ZKNLIST_UI_START) {
            if (func_0200d1dc(wk->param->pokedex) == 0) {
                return -1;
            }
        }
        if (ret == ZKNLIST_UI_SELECT) {
            if (PokeDex_IsHabitatListEnabled(wk->param->pokedex) == FALSE) {
                return -1;
            }
            if (wk->param->searched == TRUE) {
                return -1;
            }
        }
        if (ret == ZKNLIST_UI_SHORTCUT) {
            if (wk->param->searched == TRUE) {
                return -1;
            }
            if (func_0203d554() == FALSE) {
                func_0203d564(TRUE);
                FrameList_RedrawCursor(wk->frameList);
            }
        }
        if (ret == ZKNLIST_UI_UNOVA) {
            if (PokeDex_IsNationalObtained(wk->param->pokedex) != TRUE ||
                PokeDex_IsHabitatListEnabled(wk->param->pokedex) != TRUE) {
                return -1;
            }
            if (wk->param->searched == TRUE) {
                return -1;
            }
            if (PokeDex_GetMode(wk->param->pokedex) != 1) {
                return -1;
            }
            func_0203d564(TRUE);
        }
        if (ret == ZKNLIST_UI_NATIONAL) {
            if (PokeDex_IsNationalObtained(wk->param->pokedex) != TRUE ||
                PokeDex_IsHabitatListEnabled(wk->param->pokedex) != TRUE) {
                return -1;
            }
            if (wk->param->searched == TRUE) {
                return -1;
            }
            // The button of the Pokédex that the list is of does nothing
            if (PokeDex_IsNationalObtained(wk->param->pokedex) == TRUE) {
                if (PokeDex_GetMode(wk->param->pokedex) == 1) {
                    return -1;
                }
            } else {
                if (PokeDex_GetMode(wk->param->pokedex) == 1) {
                    return -1;
                }
            }
            func_0203d564(TRUE);
        }
        if (ret == ZKNLIST_UI_HABITAT) {
            if (PokeDex_IsHabitatListEnabled(wk->param->pokedex) == FALSE) {
                return -1;
            }
            if (wk->param->searched == TRUE) {
                return -1;
            }
            func_0203d564(TRUE);
        }
        func_0203d564(TRUE);
        return ret;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        func_0203d564(FALSE);
        return ZKNLIST_UI_RETURN;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
        func_0203d564(FALSE);
        return ZKNLIST_UI_EXIT;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) {
        if (func_0200d1dc(wk->param->pokedex) == 1) {
            func_0203d564(FALSE);
            return ZKNLIST_UI_START;
        }
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        if (PokeDex_IsHabitatListEnabled(wk->param->pokedex) == FALSE) {
            return -1;
        }
        if (wk->param->searched == TRUE) {
            return -1;
        }
        func_0203d564(FALSE);
        return ZKNLIST_UI_SELECT;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_Y) {
        if (wk->param->searched == TRUE) {
            return -1;
        }
        if (func_0203d554() == TRUE) {
            func_0203d564(FALSE);
            FrameList_ShowCursor(wk->frameList);
        } else {
            func_0203d564(FALSE);
        }
        return ZKNLIST_UI_SHORTCUT;
    }
    return ret;
}
