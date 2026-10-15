#include "types.h"
#include "app/ui/touchbar.h"
#include "app/pokemon_trade_local.h"
#include "constants/sound.h"
#include "nitro/mi.h"

// The trade's touch bar. The ROM names no file between pokemontrade_3d.c and pokemontrade_2d.c, so the name is
// descriptive. It is a file of its own because its function's 0x80-byte initializer comes first in .rodata, where
// MWCC, which sorts a file's data by size, would put it after pokemontrade_2d.c's smaller objects; at the end of
// pokemontrade_3d.c it would reorder two pairs of that file's equal-size objects

void func_ov194_021c2a24(PokemonTradeWork *wk) {
    TouchBarItem items[] = {
        { 1, { 232, 168 } },
        { TOUCHBAR_ICON_CUSTOM, { 28, 168 } },
        { TOUCHBAR_ICON_CUSTOM + 1, { 204, 168 } },
        { TOUCHBAR_ICON_CUSTOM + 2, { 48, 168 } },
    };
    TouchBarSetup setup;

    sys_memset(&setup, 0, sizeof(setup));
    setup.items = items;
    setup.count = NELEMS(items);
    setup.unit = wk->clactUnit;
    setup.unk1C = TRUE;
    setup.bgPalette = 7;
    setup.objPalette = 0;
    setup.vramType = 2;
    setup.bg = 4;

    items[1].charRes = wk->objRes[TRADE_OBJRES_CHAR_SUB];
    items[1].plttRes = wk->objRes[TRADE_OBJRES_PLTT_SUB];
    items[1].cellRes = wk->objRes[TRADE_OBJRES_CELL_SUB];
    items[1].anims[0] = 6;
    items[1].anims[1] = 5;
    items[1].anims[2] = 4;
    items[1].key = 0;
    items[1].se = SEQ_SE_DECIDE1;

    items[2].charRes = wk->objRes[TRADE_OBJRES_CHAR_SUB];
    items[2].plttRes = wk->objRes[TRADE_OBJRES_PLTT_SUB];
    items[2].cellRes = wk->objRes[TRADE_OBJRES_CELL_SUB];
    items[2].anims[0] = 9;
    items[2].anims[1] = 8;
    items[2].anims[2] = 7;
    items[2].key = PAD_BUTTON_START;
    items[2].se = SEQ_SE_DECIDE1;

    items[3].charRes = wk->objRes[TRADE_OBJRES_CHAR_SUB];
    items[3].plttRes = wk->objRes[TRADE_OBJRES_PLTT_SUB];
    items[3].cellRes = wk->objRes[TRADE_OBJRES_CELL_SUB];
    items[3].anims[0] = 22;
    items[3].anims[1] = 21;
    items[3].anims[2] = 20;
    items[3].key = PAD_KEY_RIGHT | PAD_KEY_LEFT;
    items[3].se = SEQ_SE_DECIDE1;

    wk->touchBar = TouchBar_Create(&setup, wk->heapId);
    TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 2, FALSE);
    TouchBar_SetIconVisible(wk->touchBar, TOUCHBAR_ICON_CUSTOM + 1, FALSE);
    TouchBar_SetPriority(wk->touchBar, 2);
}
