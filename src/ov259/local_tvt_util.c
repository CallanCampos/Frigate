#include "types.h"
#include "app/local_tvt_local.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/net.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/rtc.h"
#include "nnsys/g2d.h"
#include "nnsys/gfd.h"
#include "pml/poke_party.h"
#include "save/player_info.h"
#include "system/app_menu_common.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/poke_icon.h"
#include "system/rtc.h"

// What the parts of the Xtransceiver's calls with the game's characters share: the backgrounds, the return button, the
// sequences, the menu of topics, the list of the party and the input of the lists, named after the ROM's
// "local_tvt_util.c". That the first function is this file's is a guess: nothing ties it to a file

// The BGs of the lower screen
#define BG_SUB_WINDOW 4
#define BG_SUB_PANEL 5
#define BG_SUB_BACK 6
#define BG_SUB_NOISE 7
// The noise over the faces of the upper screen
#define BG_MAIN_NOISE 1

// The palettes of the lower screen's BGs
#define PALETTE_BAR 6
#define PALETTE_BACK 11
#define PALETTE_NORMAL 12
#define PALETTE_SELECTED 13
#define PALETTE_EGG 14
#define PALETTE_TEXT 15
#define PALETTE_EMPTY 16

// The touch bar is the three rows at the bottom
#define BAR_Y 21
#define BAR_HEIGHT 3

// The animations of the return button
#define RETURN_SEQUENCE_ACTIVE 1
#define RETURN_SEQUENCE_PRESSED 9
#define RETURN_SEQUENCE_INACTIVE 15

// How fast the cursor's color pulses, and for how long the entry the player decided on flashes
#define PULSE_SPEED 0x400
#define FLASH_FRAMES 16

#define MENU_COLOR 0x3dc3

#define PARTY_MAX 6
#define PARTY_ROWS 3
#define PARTY_COLUMNS 2

// The return button of the touch bar
typedef struct {
    ClActor *actor;
    u32 palette;
    u32 chars;
    u32 cellAnims;
} LocalTvtReturnIcon;

struct LocalTvtBG {
    TCB *vblankTask;
};

struct LocalTvtReturnBar {
    LocalTvtReturnIcon icon;
    BOOL active;
    BOOL pressed;
    u32 chars;
};

struct LocalTvtSeq {
    LocalTvtSeqFunc func;
    int step;
    void *work;
    u32 unk0c;
};

struct LocalTvtMenu {
    LocalTvtMenuSetup setup;
    BmpWin *windows[LOCAL_TVT_TOPIC_MAX];
    int selected;
    int seq;
    BOOL end;
    u16 palette[16];
    u16 paletteSelected[16];
    // The cursor's palette, which pulses between the two
    u16 paletteAnim[16];
    u16 timer;
    u16 angle;
    LocalTvtReturnIcon icon;
    LocalTvtKeyTouch *input;
};

struct LocalTvtPokeSelect {
    LocalTvtPokeSelectSetup setup;
    // LOCAL_TVT_POKE_SELECT_*
    u32 result;
    LocalTvtReturnBar *returnBar;
    u32 chars;
    u16 palette[16];
    u16 paletteSelected[16];
    u16 paletteEgg[16];
    u16 paletteAnim[16];
    int seq;
    int cursor;
    u16 angle;
    u16 timer;
    BOOL active;
    // The player touched an egg, which shows the cursor on it for half a pulse
    BOOL touched;
    LocalTvtKeyTouch *input;
    BmpWin *windows[PARTY_MAX];
    ClActor *icons[PARTY_MAX];
    u16 iconCellAnims;
    u16 iconPalette;
    u16 iconChars[PARTY_MAX];
    u32 unkf0;
    TCB *vblankTask;
    // Shows the list at the next VBlank
    BOOL show;
};

struct LocalTvtKeyTouch {
    u32 unk00;
    BOOL touchMode;
    BOOL decided;
    LocalTvtKeyTouchSetup setup;
};

static void LocalTvtBG_VBlank(TCB *tcb, void *data);
static void LocalTvtReturnBar_SetVisible(LocalTvtReturnBar *bar, BOOL visible);
static void LocalTvtReturnBar_SetActive(LocalTvtReturnBar *bar, BOOL active);
static int LocalTvtMenu_Touch(void *work);
static int LocalTvtMenu_Key(void *work);
static void LocalTvtMenu_Draw(void *work, BOOL touchMode);
static void LocalTvtPokeSelect_DrawPanels(LocalTvtPokeSelect *select, BOOL cursor);
static void LocalTvtPokeSelect_GetGridPos(int index, s16 *row, s16 *column);
static void LocalTvtPokeSelect_GetGridSize(int index, u32 count, s16 *rows, s16 *columns);
static int LocalTvtPokeSelect_Touch(void *work);
static int LocalTvtPokeSelect_Key(void *work);
static void LocalTvtPokeSelect_Draw(void *work, BOOL touchMode);
static void LocalTvtPokeSelect_VBlank(TCB *tcb, void *data);
static void LocalTvtUtil_BlendPalette(NNS_GFD_DST_TYPE type, u16 *dest, u16 angle, u32 palette, const u16 *from,
                                      const u16 *to);
static void LocalTvtReturnIcon_Init(LocalTvtReturnIcon *icon, ClActUnit *unit, HeapID heapId);
static void LocalTvtReturnIcon_Exit(LocalTvtReturnIcon *icon);
static ClActor *LocalTvtReturnIcon_GetActor(LocalTvtReturnIcon *icon);
static void LocalTvtKeyTouch_Reset(LocalTvtKeyTouch *input);
static int LocalTvtKeyTouch_CallTouch(LocalTvtKeyTouch *input);
static int LocalTvtKeyTouch_CallKey(LocalTvtKeyTouch *input);
static void LocalTvtKeyTouch_CallDraw(LocalTvtKeyTouch *input);

static const TouchRect sLocalTvtReturnBarRects[] = {
    { 168, 192, 224, 248 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const TouchRect sLocalTvtMenuRects[] = {
    { 16, 48, 32, 216 }, { 56, 88, 32, 216 }, { 96, 128, 32, 216 }, { 136, 168, 32, 216 }, { TOUCH_RECT_END, 0, 0, 0 },
};

// The panels of the party, which are the touch rectangles as well
// BUG: the table has no end, so the touch check reads on into the tables after it
static const TouchRect sLocalTvtPokeSelectRects[] = {
    { 8, 48, 0, 120 },           { 24, 64, 128, 248 }, { 56, 96, 0, 120 },
    { 72, 112, 128, 248 },       { 104, 144, 0, 120 }, { 120, 160, 128, 248 },
#ifdef BUGFIX
    { TOUCH_RECT_END, 0, 0, 0 },
#endif
};

// The party's members by row and column
static const u32 sLocalTvtPokeSelectGrid[PARTY_ROWS][PARTY_COLUMNS] = {
    { 0, 1 },
    { 2, 3 },
    { 4, 5 },
};

u32 LocalTvtUtil_GetTime(GameData *gameData) {
    u32 season = GameData_GetSeason(gameData);
    u32 morning = GetLightChangeHoursForSeasons(season, DAY_PERIOD_MORNING);
    u32 evening = GetLightChangeHoursForSeasons(season, DAY_PERIOD_EVENING);
    u32 night = GetLightChangeHoursForSeasons(season, DAY_PERIOD_NIGHT);
    RTCTime time;

    RTC_GetCachedTime(&time);
    if (time.hour >= morning && time.hour < evening) {
        return LOCAL_TVT_TIME_DAY;
    }
    if (time.hour >= evening && time.hour < night) {
        return LOCAL_TVT_TIME_EVENING;
    }
    return LOCAL_TVT_TIME_NIGHT;
}

LocalTvtBG *LocalTvtBG_Create(GameData *gameData, HeapID heapId, u32 unk04) {
    LocalTvtBG *bg = GFL_HeapAllocate(heapId, sizeof(LocalTvtBG), TRUE, "local_tvt_util.c", 126);
    BOOL isMale = TRUE;
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARC_LOCAL_TVT_COMMON, heapId);

    if (getTrainerGender(playerInfo) != GENDER_MALE) {
        isMale = FALSE;
    }
    if (unk04 ^ isMale) {
        GFL_G2DIOLoadArcNCLRDefault(arc, 7, PALTYPE_SUB_BG, 0, 3 * 0x20, heapId);
    } else {
        GFL_G2DIOLoadArcNCLRDefault(arc, 6, PALTYPE_SUB_BG, 0, 3 * 0x20, heapId);
    }
    GFL_BGSysLoadArcNCGRStatic(arc, 12, BG_SUB_NOISE, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 14, BG_SUB_NOISE, 0, 0, FALSE, heapId);
    GFL_BGSysLoadScr(BG_SUB_NOISE);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, PALTYPE_MAIN_BG, PALETTE_BACK * 0x20, 0x20, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 8, BG_MAIN_NOISE, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 13, BG_MAIN_NOISE, 0, 0, FALSE, heapId);
    GFL_BGSysSetScrPaletteNo(BG_MAIN_NOISE, 0, 0, 32, 32, PALETTE_BACK);
    GFL_BGSysLoadScr(BG_MAIN_NOISE);
    GFL_ArcToolFree(arc);
    bg->vblankTask = GFL_VBlankTCBAdd(LocalTvtBG_VBlank, bg, 0);
    return bg;
}

void LocalTvtBG_Free(LocalTvtBG *bg) {
    GFL_TCBRemove(bg->vblankTask);
    GFL_HeapFree(bg);
}

// Moves the noise to a random place every frame
static void LocalTvtBG_VBlank(TCB *tcb, void *data) {
    GFL_BGSysMoveBG(BG_MAIN_NOISE, BG_MOVE_SET_X, GFL_RandomLCAlt(256));
    GFL_BGSysMoveBG(BG_MAIN_NOISE, BG_MOVE_SET_Y, GFL_RandomLCAlt(256));
}

LocalTvtReturnBar *LocalTvtReturnBar_Create(ClActUnit *unit, HeapID heapId) {
    LocalTvtReturnBar *bar = GFL_HeapAllocate(heapId, sizeof(LocalTvtReturnBar), TRUE, "local_tvt_util.c", 226);
    ArcTool *arc;
    NNSG2dScreenData *screen;
    void *file;
    int i;

    LocalTvtReturnIcon_Init(&bar->icon, unit, heapId);
    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, func_0202d820(), PALTYPE_SUB_BG, PALETTE_BAR * 0x20, 0x20, heapId);
    bar->chars = GFL_BGSysLoadArcNCGRDynamic(arc, func_0202d824(), BG_SUB_PANEL, 0, FALSE, heapId);
    file = GFL_G2DIOReadNSCRArc(arc, func_0202d828(), FALSE, &screen, heapId);
    for (i = 0; i < 32 * 24; i++) {
        ((u16 *)screen->rawData)[i] += (u16)bar->chars;
    }
    GFL_BGSysLoadScrArea(BG_SUB_PANEL, 0, BAR_Y, 32, BAR_HEIGHT, screen->rawData, 0, BAR_Y, 32, 24);
    GFL_HeapFree(file);
    GFL_BGSysSetScrPaletteNo(BG_SUB_PANEL, 0, BAR_Y, 32, BAR_HEIGHT, PALETTE_BAR);
    GFL_BGSysLoadScr(BG_SUB_PANEL);
    GFL_ArcToolFree(arc);
    LocalTvtReturnBar_SetActive(bar, TRUE);
    return bar;
}

void LocalTvtReturnBar_Free(LocalTvtReturnBar *bar) {
    GFL_BGSysFreeCharMemory(BG_SUB_PANEL, (u16)bar->chars, (u16)(bar->chars >> 16));
    LocalTvtReturnIcon_Exit(&bar->icon);
    GFL_BGSysClearBG(BG_SUB_PANEL);
    GFL_BGSysQueueScrLoad(BG_SUB_PANEL);
    GFL_HeapFree(bar);
}

void LocalTvtReturnBar_Main(LocalTvtReturnBar *bar) {
    if (bar->active) {
        int touched = func_0203da0c(sLocalTvtReturnBarRects);
        BOOL pressed = FALSE;

        if (touched == 0) {
            pressed = TRUE;
            func_0203d564(TRUE);
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            func_0203d564(FALSE);
            pressed = TRUE;
        }
        if (pressed) {
            func_0204c488(LocalTvtReturnIcon_GetActor(&bar->icon), RETURN_SEQUENCE_PRESSED);
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            bar->pressed = TRUE;
        }
    }
}

static void LocalTvtReturnBar_SetVisible(LocalTvtReturnBar *bar, BOOL visible) {
    func_0204c124(LocalTvtReturnIcon_GetActor(&bar->icon), visible);
    GFL_BGSysSetBGEnabled(BG_SUB_PANEL, visible);
}

BOOL LocalTvtReturnBar_IsPressed(LocalTvtReturnBar *bar) {
    return bar->pressed;
}

BOOL LocalTvtReturnBar_IsAnimEnd(LocalTvtReturnBar *bar) {
    return !func_0204c560(LocalTvtReturnIcon_GetActor(&bar->icon));
}

static void LocalTvtReturnBar_SetActive(LocalTvtReturnBar *bar, BOOL active) {
    bar->active = active;
    if (active) {
        func_0204c488(LocalTvtReturnIcon_GetActor(&bar->icon), RETURN_SEQUENCE_ACTIVE);
    } else {
        func_0204c488(LocalTvtReturnIcon_GetActor(&bar->icon), RETURN_SEQUENCE_INACTIVE);
    }
}

LocalTvtSeq *LocalTvtSeq_Create(void *work, LocalTvtSeqFunc func, HeapID heapId) {
    LocalTvtSeq *seq = GFL_HeapAllocate(heapId, sizeof(LocalTvtSeq), TRUE, "local_tvt_util.c", 428);

    seq->work = work;
    LocalTvtSeq_SetNext(seq, func);
    return seq;
}

void LocalTvtSeq_Free(LocalTvtSeq *seq) {
    GFL_HeapFree(seq);
}

void LocalTvtSeq_Main(LocalTvtSeq *seq) {
    if (seq->func != NULL) {
        seq->func(seq, &seq->step, seq->work);
    }
}

BOOL LocalTvtSeq_IsEnd(LocalTvtSeq *seq) {
    return seq->func == NULL;
}

void LocalTvtSeq_SetNext(LocalTvtSeq *seq, LocalTvtSeqFunc func) {
    seq->func = func;
    seq->step = 0;
}

void LocalTvtSeq_End(LocalTvtSeq *seq) {
    LocalTvtSeq_SetNext(seq, NULL);
}

LocalTvtMenu *LocalTvtMenu_Create(const LocalTvtMenuSetup *setup, HeapID heapId) {
    LocalTvtMenu *menu = GFL_HeapAllocate(heapId, sizeof(LocalTvtMenu), TRUE, "local_tvt_util.c", 545);
    ArcTool *arc;
    NNSG2dPaletteData *paletteData;
    void *file;
    const u16 *colors;
    ClActorPos pos;
    LocalTvtKeyTouchSetup inputSetup;
    int i;

    menu->setup = *setup;
    menu->selected = setup->cursor;
    arc = GFL_ArcSysCreateFileHandle(ARC_LOCAL_TVT, heapId);
    GFL_BGSysClearCharCore(BG_SUB_WINDOW, 0x20, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 122, BG_SUB_WINDOW, 1, 0, FALSE, heapId);
    file = GFL_G2DIOReadNCLRArc(arc, 121, &paletteData, heapId);
    colors = paletteData->rawData;
    sys_memcpy(colors, menu->palette, sizeof(menu->palette));
    colors += 16;
    sys_memcpy(colors, menu->paletteSelected, sizeof(menu->paletteSelected));
    GFL_BGSysUploadStdPalette(BG_SUB_WINDOW, menu->palette, sizeof(menu->palette), PALETTE_NORMAL * 0x20);
    GFL_BGSysUploadStdPalette(BG_SUB_WINDOW, menu->paletteSelected, sizeof(menu->paletteSelected),
                              PALETTE_SELECTED * 0x20);
    GFL_HeapFree(file);
    GFL_ArcToolFree(arc);
    // The return button is on the last topic, which says goodbye
    LocalTvtReturnIcon_Init(&menu->icon, menu->setup.unit, heapId);
    pos.x = 184;
    pos.y = (menu->setup.count - 1) * 40 + 20;
    func_0204c140(LocalTvtReturnIcon_GetActor(&menu->icon), &pos, 1);
    func_0204c488(LocalTvtReturnIcon_GetActor(&menu->icon), RETURN_SEQUENCE_ACTIVE);
    for (i = 0; i < menu->setup.count; i++) {
        menu->windows[i] = BmpWin_CreateDynamic(BG_SUB_WINDOW, 5, i * 5 + 3, 21, 2, PALETTE_NORMAL, TRUE);
        LocalTvtMessage_SetColor(menu->setup.message, MENU_COLOR);
        LocalTvtMessage_SetPosition(menu->setup.message, 0, 0, LOCAL_TVT_ALIGN_CENTER);
        LocalTvtMessage_PrintMsg(menu->setup.message, menu->windows[i], menu->setup.msgData, (u16)menu->setup.items[i]);
        BmpWin_Transfer(menu->windows[i]);
        BmpWin_DrawFrame(menu->windows[i], TRUE, 1, PALETTE_NORMAL);
    }
    sys_memset(&inputSetup, 0, sizeof(inputSetup));
    inputSetup.work = menu;
    inputSetup.touch = LocalTvtMenu_Touch;
    inputSetup.key = LocalTvtMenu_Key;
    inputSetup.draw = LocalTvtMenu_Draw;
    menu->input = LocalTvtKeyTouch_Create(&inputSetup, heapId);
    return menu;
}

void LocalTvtMenu_Free(LocalTvtMenu *menu) {
    int i;

    LocalTvtKeyTouch_Free(menu->input);
    for (i = 0; i < menu->setup.count; i++) {
        BmpWin_Free(menu->windows[i]);
    }
    GFL_BGSysClearScr(BG_SUB_WINDOW);
    GFL_BGSysQueueScrLoad(BG_SUB_WINDOW);
    LocalTvtReturnIcon_Exit(&menu->icon);
    GFL_HeapFree(menu);
}

void LocalTvtMenu_Main(LocalTvtMenu *menu) {
    int i;

    switch (menu->seq) {
    case 0:
        LocalTvtKeyTouch_Main(menu->input);
        if (LocalTvtKeyTouch_IsDecided(menu->input)) {
            menu->timer = 0;
            menu->angle = 0;
            menu->seq = 1;
            GFL_BGSysUploadStdPalette(BG_SUB_WINDOW, menu->paletteSelected, sizeof(menu->paletteSelected),
                                      PALETTE_SELECTED * 0x20);
            for (i = 0; i < menu->setup.count; i++) {
                BmpWin *window = menu->windows[i];
                u8 x = BmpWin_GetPosX(window) - 1;
                u8 y = BmpWin_GetPosY(window) - 1;
                u8 width = BmpWin_GetSizeX(window) + 2;
                u8 height = BmpWin_GetSizeY(window) + 2;

                GFL_BGSysSetScrPaletteNo(BG_SUB_WINDOW, x, y, width, height, PALETTE_NORMAL);
            }
            break;
        }
        if (menu->angle + PULSE_SPEED >= 0x10000) {
            menu->angle = menu->angle + PULSE_SPEED - 0x10000;
        } else {
            menu->angle += PULSE_SPEED;
        }
        LocalTvtUtil_BlendPalette(NNS_GFD_DST_2D_BG_PLTT_SUB, menu->paletteAnim, menu->angle, PALETTE_SELECTED,
                                  menu->palette, menu->paletteSelected);
        break;
    case 1: {
        BmpWin *window = menu->windows[menu->selected];
        u8 x = BmpWin_GetPosX(window) - 1;
        u8 y = BmpWin_GetPosY(window) - 1;
        u8 width = BmpWin_GetSizeX(window) + 2;
        u8 height = BmpWin_GetSizeY(window) + 2;
        u8 palette;

        if ((menu->timer / 4) % 2) {
            palette = PALETTE_SELECTED;
        } else {
            palette = PALETTE_NORMAL;
        }
        GFL_BGSysSetScrPaletteNo(BG_SUB_WINDOW, x, y, width, height, palette);
        GFL_BGSysLoadScr(BG_SUB_WINDOW);
        if (menu->timer++ > FLASH_FRAMES) {
            menu->seq = 2;
        }
        break;
    }
    case 2:
        menu->end = TRUE;
        break;
    }
}

BOOL LocalTvtMenu_IsEnd(LocalTvtMenu *menu) {
    return menu->end;
}

u32 LocalTvtMenu_GetSelected(LocalTvtMenu *menu) {
    return menu->selected;
}

static int LocalTvtMenu_Touch(void *work) {
    LocalTvtMenu *menu = work;
    int index = func_0203da0c(sLocalTvtMenuRects);

    if (index != TOUCH_RECT_NONE && index < menu->setup.count) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        menu->selected = index;
        return LOCAL_TVT_INPUT_DECIDE;
    }
    return LOCAL_TVT_INPUT_NONE;
}

// The B button picks the last topic, which says goodbye
static int LocalTvtMenu_Key(void *work) {
    LocalTvtMenu *menu = work;
    BOOL moved = FALSE;
    BOOL decided = FALSE;
    int i;

    if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
        menu->selected--;
        if (menu->selected < 0) {
            menu->selected = menu->setup.count - 1;
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        moved = TRUE;
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
        menu->selected++;
        menu->selected %= menu->setup.count;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        moved = TRUE;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        decided = TRUE;
    } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        decided = TRUE;
        menu->selected = menu->setup.count - 1;
    }
    if (moved) {
        for (i = 0; i < menu->setup.count; i++) {
            u8 palette = PALETTE_SELECTED;
            BmpWin *window;
            u8 x;
            u8 y;
            u8 width;
            u8 height;

            if (i != menu->selected) {
                palette = PALETTE_NORMAL;
            }
            window = menu->windows[i];
            x = BmpWin_GetPosX(window) - 1;
            y = BmpWin_GetPosY(window) - 1;
            width = BmpWin_GetSizeX(window) + 2;
            height = BmpWin_GetSizeY(window) + 2;
            GFL_BGSysSetScrPaletteNo(BG_SUB_WINDOW, x, y, width, height, palette);
        }
        menu->angle = 0;
        GFL_BGSysQueueScrLoad(BG_SUB_WINDOW);
    }
    if (decided) {
        return LOCAL_TVT_INPUT_DECIDE;
    }
    if (moved) {
        return LOCAL_TVT_INPUT_MOVE;
    }
    return LOCAL_TVT_INPUT_NONE;
}

static void LocalTvtMenu_Draw(void *work, BOOL touchMode) {
    LocalTvtMenu *menu = work;
    int i;

    if (touchMode == TRUE) {
        for (i = 0; i < menu->setup.count; i++) {
            BmpWin *window = menu->windows[i];
            u8 x = BmpWin_GetPosX(window) - 1;
            u8 y = BmpWin_GetPosY(window) - 1;
            u8 width = BmpWin_GetSizeX(window) + 2;
            u8 height = BmpWin_GetSizeY(window) + 2;

            GFL_BGSysSetScrPaletteNo(BG_SUB_WINDOW, x, y, width, height, PALETTE_NORMAL);
        }
        GFL_BGSysQueueScrLoad(BG_SUB_WINDOW);
    } else {
        BmpWin *window = menu->windows[menu->selected];
        u8 x = BmpWin_GetPosX(window) - 1;
        u8 y = BmpWin_GetPosY(window) - 1;
        u8 width = BmpWin_GetSizeX(window) + 2;
        u8 height = BmpWin_GetSizeY(window) + 2;

        GFL_BGSysSetScrPaletteNo(BG_SUB_WINDOW, x, y, width, height, PALETTE_SELECTED);
        GFL_BGSysQueueScrLoad(BG_SUB_WINDOW);
    }
}

LocalTvtPokeSelect *LocalTvtPokeSelect_Create(const LocalTvtPokeSelectSetup *setup, HeapID heapId) {
    LocalTvtPokeSelect *select = GFL_HeapAllocate(heapId, sizeof(LocalTvtPokeSelect), TRUE, "local_tvt_util.c", 1076);
    ArcTool *arc;
    NNSG2dPaletteData *paletteData;
    void *file;
    const u16 *colors;
    LocalTvtKeyTouchSetup inputSetup;
    int i;

    select->setup = *setup;
    select->cursor = 0;
    select->active = TRUE;
    GFL_BGSysSetBGEnabled(BG_SUB_PANEL, FALSE);
    GFL_BGSysSetBGEnabled(BG_SUB_BACK, FALSE);
    GFL_BGSysSetBGEnabled(BG_SUB_WINDOW, FALSE);
    arc = GFL_ArcSysCreateFileHandle(ARC_LOCAL_TVT, heapId);
    file = GFL_G2DIOReadNCLRArc(arc, 123, &paletteData, heapId);
    colors = paletteData->rawData;
    sys_memcpy(&colors[0], select->palette, sizeof(select->palette));
    sys_memcpy(&colors[16], select->paletteSelected, sizeof(select->paletteSelected));
    sys_memcpy(&colors[32], select->paletteEgg, sizeof(select->paletteEgg));
    GFL_BGSysUploadStdPalette(BG_SUB_PANEL, select->palette, sizeof(select->palette), PALETTE_NORMAL * 0x20);
    GFL_BGSysUploadStdPalette(BG_SUB_PANEL, select->paletteSelected, sizeof(select->paletteSelected),
                              PALETTE_SELECTED * 0x20);
    GFL_BGSysUploadStdPalette(BG_SUB_PANEL, select->paletteEgg, sizeof(select->paletteEgg), PALETTE_EGG * 0x20);
    GFL_HeapFree(file);
    select->chars = GFL_BGSysLoadArcNCGRDynamic(arc, 124, BG_SUB_PANEL, 0, FALSE, heapId);
    GFL_G2DIOLoadNSCRSync(arc, 125, BG_SUB_PANEL, 0, (u16)select->chars, 0, FALSE, heapId);
    LocalTvtPokeSelect_DrawPanels(select, FALSE);
    GFL_G2DIOLoadArcNCLRDefault(arc, 126, PALTYPE_SUB_BG, PALETTE_BACK * 0x20, 0x20, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 127, BG_SUB_BACK, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 128, BG_SUB_BACK, 0, 0, FALSE, heapId);
    GFL_BGSysSetScrPaletteNo(BG_SUB_BACK, 0, 0, 32, 24, PALETTE_BACK);
    GFL_BGSysLoadScr(BG_SUB_BACK);
    GFL_ArcToolFree(arc);
    select->returnBar = LocalTvtReturnBar_Create(select->setup.unit, heapId);
    LocalTvtReturnBar_SetVisible(select->returnBar, FALSE);
    sys_memset(&inputSetup, 0, sizeof(inputSetup));
    inputSetup.work = select;
    inputSetup.touch = LocalTvtPokeSelect_Touch;
    inputSetup.key = LocalTvtPokeSelect_Key;
    inputSetup.draw = LocalTvtPokeSelect_Draw;
    select->input = LocalTvtKeyTouch_Create(&inputSetup, heapId);
    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, heapId);
    select->iconPalette = func_0204bc48(arc, PokeIcon_GetPaletteFile(), 1, 8 * 0x20, heapId);
    select->iconCellAnims = func_0204bde0(arc, PokeIcon_GetCellFile_Sub(), getOBJTileMapping_SubEng(), heapId);
    for (i = 0; i < PokeParty_GetPkmCount(select->setup.party); i++) {
        PartyPkm *pkm = PokeParty_GetPkm(select->setup.party, i);
        u32 iconFile = PokeIcon_GetFileOfPkm(func_0201d624(pkm));
        u32 iconPalette = PokeIcon_GetPaletteOfPkm(func_0201d624(pkm));
        ClActorSetup actorSetup;

        sys_memset(&actorSetup, 0, sizeof(actorSetup));
        actorSetup.sequence = 1;
        actorSetup.x = sLocalTvtPokeSelectRects[i].left + 24;
        actorSetup.y = sLocalTvtPokeSelectRects[i].top + 16;
        select->iconChars[i] = func_0204b81c(arc, iconFile, FALSE, 1, heapId);
        select->icons[i] = func_0204c040(select->setup.unit, select->iconChars[i], select->iconPalette,
                                         select->iconCellAnims, &actorSetup, 1, heapId);
        func_0204c378(select->icons[i], iconPalette, 1);
        func_0204c124(select->icons[i], FALSE);
    }
    GFL_ArcToolFree(arc);
    for (i = 0; i < PokeParty_GetPkmCount(select->setup.party); i++) {
        PartyPkm *pkm = PokeParty_GetPkm(select->setup.party, i);
        u8 y = (u32)sLocalTvtPokeSelectRects[i].top / 8;
        u8 x = (u32)sLocalTvtPokeSelectRects[i].left / 8;

        select->windows[i] =
            BmpWin_CreateDynamic(BG_SUB_WINDOW, x + 1, y + 1, (u32)sLocalTvtPokeSelectRects[i].right / 8 - x,
                                 (u32)sLocalTvtPokeSelectRects[i].bottom / 8 - y, PALETTE_TEXT, TRUE);
        BmpWin_Transfer(select->windows[i]);
        LocalTvtMessage_PrintPokemon(select->setup.message, select->windows[i], pkm);
    }
    GFL_BGSysQueueScrLoad(BG_SUB_WINDOW);
    select->vblankTask = GFL_VBlankTCBAdd(LocalTvtPokeSelect_VBlank, select, 0);
    select->show = TRUE;
    return select;
}

void LocalTvtPokeSelect_Free(LocalTvtPokeSelect *select) {
    int i;

    GFL_TCBRemove(select->vblankTask);
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    G2S_BlendNone();
    for (i = 0; i < PARTY_MAX; i++) {
        if (select->windows[i] != NULL) {
            BmpWin_Free(select->windows[i]);
        }
    }
    for (i = 0; i < PARTY_MAX; i++) {
        if (select->icons[i] != NULL) {
            func_0204c108(select->icons[i]);
            func_0204b98c(select->iconChars[i]);
        }
    }
    func_0204bcd0(select->iconPalette);
    func_0204be64(select->iconCellAnims);
    LocalTvtKeyTouch_Free(select->input);
    LocalTvtReturnBar_Free(select->returnBar);
    GFL_BGSysFreeCharMemory(BG_SUB_PANEL, (u16)select->chars, (u16)(select->chars >> 16));
    GFL_BGSysClearBG(BG_SUB_BACK);
    GFL_BGSysQueueScrLoad(BG_SUB_BACK);
    GFL_BGSysClearBG(BG_SUB_PANEL);
    GFL_BGSysQueueScrLoad(BG_SUB_PANEL);
    GFL_BGSysClearBG(BG_SUB_WINDOW);
    GFL_BGSysQueueScrLoad(BG_SUB_WINDOW);
    GFL_HeapFree(select);
}

void LocalTvtPokeSelect_Main(LocalTvtPokeSelect *select) {
    if (!select->active) {
        return;
    }
    switch (select->seq) {
    case 0: {
        const u16 *from;

        LocalTvtKeyTouch_Main(select->input);
        LocalTvtReturnBar_Main(select->returnBar);
        if (LocalTvtKeyTouch_IsDecided(select->input)) {
            select->timer = 0;
            select->angle = 0;
            select->seq = 1;
            GFL_BGSysUploadStdPalette(BG_SUB_PANEL, select->paletteSelected, sizeof(select->paletteSelected),
                                      PALETTE_SELECTED * 0x20);
            LocalTvtPokeSelect_DrawPanels(select, FALSE);
            break;
        }
        if (LocalTvtReturnBar_IsPressed(select->returnBar)) {
            select->seq = 2;
        }
        if (select->touched) {
            if (select->angle + PULSE_SPEED >= 0x8000) {
                LocalTvtPokeSelect_DrawPanels(select, FALSE);
                select->touched = FALSE;
            }
        }
        if (select->angle + PULSE_SPEED >= 0x10000) {
            select->angle = select->angle + PULSE_SPEED - 0x10000;
        } else {
            select->angle += PULSE_SPEED;
        }
        if (PokeParty_GetParam(PokeParty_GetPkm(select->setup.party, select->cursor), PKM_PARAM_IS_EGG, NULL)) {
            from = select->paletteEgg;
        } else {
            from = select->palette;
        }
        LocalTvtUtil_BlendPalette(NNS_GFD_DST_2D_BG_PLTT_SUB, select->paletteAnim, select->angle, PALETTE_SELECTED,
                                  from, select->paletteSelected);
        break;
    }
    case 1: {
        u8 x = (u32)sLocalTvtPokeSelectRects[select->cursor].left / 8;
        u8 y = (u32)sLocalTvtPokeSelectRects[select->cursor].top / 8;
        u8 width = (u32)sLocalTvtPokeSelectRects[select->cursor].right / 8 - x + 1;
        u8 height = (u32)sLocalTvtPokeSelectRects[select->cursor].bottom / 8 - y + 1;
        u8 palette;

        if ((select->timer / 4) % 2) {
            palette = PALETTE_SELECTED;
        } else {
            palette = PALETTE_NORMAL;
        }
        GFL_BGSysSetScrPaletteNo(BG_SUB_PANEL, x, y, width, height, palette);
        if (select->timer++ > FLASH_FRAMES) {
            GFL_BGSysSetScrPaletteNo(BG_SUB_PANEL, x, y, width, height, PALETTE_SELECTED);
            select->result = LOCAL_TVT_POKE_SELECT_DECIDE;
            select->seq = 3;
        }
        GFL_BGSysLoadScr(BG_SUB_PANEL);
        break;
    }
    case 2:
        if (LocalTvtReturnBar_IsAnimEnd(select->returnBar)) {
            select->result = LOCAL_TVT_POKE_SELECT_CANCEL;
            select->seq = 3;
        }
        break;
    case 3:
        break;
    }
}

// The list darkens while a face talks. The windows keep the touch bar, and the communication icon at the top
// right when it shows, as they are
void LocalTvtPokeSelect_SetActive(LocalTvtPokeSelect *select, BOOL active) {
    select->active = active;
    if (active) {
        gfxRegSetBrightnessBlend(REG_DB_BLDCNT_ADDR, GX_PLANEMASK_ALL, 0);
        G2S_SetWnd0Position(0, BAR_Y * 8, 255, 192);
        G2S_SetWnd1Position(1, BAR_Y * 8, 256, 192);
        gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1, GX_BLEND_PLANEMASK_BG2, 13, 3);
    } else {
        if (func_02042788()) {
            G2S_SetWnd0Position(240, 0, 256, 16);
            G2S_SetWnd1Position(240, 0, 256, 16);
        } else {
            G2S_SetWnd0Position(0, 0, 0, 0);
            G2S_SetWnd1Position(0, 0, 0, 0);
        }
        gfxRegSetBrightnessBlend(REG_DB_BLDCNT_ADDR, GX_PLANEMASK_ALL, 8);
    }
}

void LocalTvtPokeSelect_Restart(LocalTvtPokeSelect *select) {
    select->result = LOCAL_TVT_POKE_SELECT_NONE;
    select->seq = 0;
    LocalTvtKeyTouch_Reset(select->input);
}

u32 LocalTvtPokeSelect_GetResult(LocalTvtPokeSelect *select) {
    return select->result;
}

PartyPkm *LocalTvtPokeSelect_GetPkm(LocalTvtPokeSelect *select) {
    return PokeParty_GetPkm(select->setup.party, select->cursor);
}

// Colors the panels: none where the party has no Pokémon, and the cursor's with the color that pulses
static void LocalTvtPokeSelect_DrawPanels(LocalTvtPokeSelect *select, BOOL cursor) {
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    PartyPkm *pkm;
    int i;

    for (i = 0; i < PokeParty_GetCapacity(select->setup.party); i++) {
        x = (u32)sLocalTvtPokeSelectRects[i].left / 8;
        y = (u32)sLocalTvtPokeSelectRects[i].top / 8;
        width = (u32)sLocalTvtPokeSelectRects[i].right / 8 - x + 1;
        height = (u32)sLocalTvtPokeSelectRects[i].bottom / 8 - y + 1;
        pkm = NULL;
        if (i < PokeParty_GetPkmCount(select->setup.party)) {
            pkm = PokeParty_GetPkm(select->setup.party, i);
        }
        if (pkm == NULL || !PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
            GFL_BGSysFillScrArea(BG_SUB_PANEL, 0, x, y, width, height, PALETTE_EMPTY);
        } else if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
            GFL_BGSysSetScrPaletteNo(BG_SUB_PANEL, x, y, width, height, PALETTE_EGG);
        } else {
            GFL_BGSysSetScrPaletteNo(BG_SUB_PANEL, x, y, width, height, PALETTE_NORMAL);
        }
        if (cursor && i == select->cursor) {
            GFL_BGSysSetScrPaletteNo(BG_SUB_PANEL, x, y, width, height, PALETTE_SELECTED);
        }
    }
    GFL_BGSysQueueScrLoad(BG_SUB_PANEL);
}

// The row and column of a party member
static void LocalTvtPokeSelect_GetGridPos(int index, s16 *row, s16 *column) {
    for (*row = 0; *row < PARTY_ROWS; (*row)++) {
        for (*column = 0; *column < PARTY_COLUMNS; (*column)++) {
            if (index-- == 0) {
                return;
            }
        }
    }
}

// How many rows the column of a party member has, and how many columns its row has, in a party of count
static void LocalTvtPokeSelect_GetGridSize(int index, u32 count, s16 *rows, s16 *columns) {
    s16 row;
    s16 column;

    LocalTvtPokeSelect_GetGridPos(index, &row, &column);
    for (*rows = 0; *rows < PARTY_ROWS; (*rows)++) {
        if (sLocalTvtPokeSelectGrid[*rows][column] >= count) {
            break;
        }
    }
    for (*columns = 0; *columns < PARTY_COLUMNS; (*columns)++) {
        if (sLocalTvtPokeSelectGrid[row][*columns] >= count) {
            break;
        }
    }
}

// An egg cannot be picked: the touch only shows the cursor on it
static int LocalTvtPokeSelect_Touch(void *work) {
    LocalTvtPokeSelect *select = work;
    int index = func_0203da0c(sLocalTvtPokeSelectRects);

    if (index != TOUCH_RECT_NONE && index < PokeParty_GetPkmCount(select->setup.party)) {
        PartyPkm *pkm = PokeParty_GetPkm(select->setup.party, index);

        LocalTvtPokeSelect_DrawPanels(select, FALSE);
        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
            select->cursor = index;
            select->angle = 0;
            select->touched = TRUE;
            LocalTvtPokeSelect_DrawPanels(select, TRUE);
            return LOCAL_TVT_INPUT_MOVE;
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        select->cursor = index;
        return LOCAL_TVT_INPUT_DECIDE;
    }
    return LOCAL_TVT_INPUT_NONE;
}

static int LocalTvtPokeSelect_Key(void *work) {
    LocalTvtPokeSelect *select = work;
    BOOL moved = FALSE;
    BOOL decided = FALSE;
    int cursor = select->cursor;
    int count = PokeParty_GetPkmCount(select->setup.party);
    s16 row;
    s16 column;
    s16 rows;
    s16 columns;

    LocalTvtPokeSelect_GetGridPos(cursor, &row, &column);
    LocalTvtPokeSelect_GetGridSize(cursor, count, &rows, &columns);
    if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
        row--;
        if (row < 0) {
            row = rows - 1;
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        moved = TRUE;
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
        row++;
        row %= rows;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        moved = TRUE;
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_LEFT) {
        int index = cursor - 1;

        if (index < 0) {
            index = count - 1;
        }
        LocalTvtPokeSelect_GetGridPos(index, &row, &column);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        moved = TRUE;
    } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_RIGHT) {
        int index = cursor + 1;

        if (index >= count) {
            index = 0;
        }
        LocalTvtPokeSelect_GetGridPos(index, &row, &column);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        moved = TRUE;
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        decided = TRUE;
    }
    if (moved) {
        select->cursor = sLocalTvtPokeSelectGrid[row][column];
        LocalTvtPokeSelect_DrawPanels(select, TRUE);
        select->angle = 0;
        GFL_BGSysQueueScrLoad(BG_SUB_PANEL);
    }
    if (decided) {
        if (PokeParty_GetParam(PokeParty_GetPkm(select->setup.party, select->cursor), PKM_PARAM_IS_EGG, NULL)) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
            select->angle = 0;
            return LOCAL_TVT_INPUT_NONE;
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return LOCAL_TVT_INPUT_DECIDE;
    }
    if (moved) {
        return LOCAL_TVT_INPUT_MOVE;
    }
    return LOCAL_TVT_INPUT_NONE;
}

static void LocalTvtPokeSelect_Draw(void *work, BOOL touchMode) {
    LocalTvtPokeSelect *select = work;

    if (touchMode == TRUE) {
        if (!select->touched) {
            LocalTvtPokeSelect_DrawPanels(select, FALSE);
        }
    } else {
        select->touched = FALSE;
        LocalTvtPokeSelect_DrawPanels(select, TRUE);
    }
}

// Shows the list once it is drawn, with the upper screen's face showing through the panels
static void LocalTvtPokeSelect_VBlank(TCB *tcb, void *data) {
    LocalTvtPokeSelect *select = data;
    int i;

    if (select->show) {
        for (i = 0; i < PARTY_MAX; i++) {
            if (select->icons[i] != NULL) {
                func_0204c124(select->icons[i], TRUE);
            }
        }
        select->show = FALSE;
        LocalTvtReturnBar_SetVisible(select->returnBar, TRUE);
        GFL_BGSysSetBGEnabled(BG_SUB_PANEL, TRUE);
        GFL_BGSysSetBGEnabled(BG_SUB_BACK, TRUE);
        GFL_BGSysSetBGEnabled(BG_SUB_WINDOW, TRUE);
        gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1, GX_BLEND_PLANEMASK_BG2, 13, 3);
        GXS_SetVisibleWnd(GX_WNDMASK_W0 | GX_WNDMASK_W1);
        G2S_SetWnd0Position(0, BAR_Y * 8, 255, 192);
        G2S_SetWnd1Position(1, BAR_Y * 8, 256, 192);
        G2S_SetWnd0InsidePlane(GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_OBJ, FALSE);
        G2S_SetWnd1InsidePlane(GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_OBJ, FALSE);
        G2S_SetWndOutsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 |
                                   GX_WND_PLANEMASK_BG3 | GX_WND_PLANEMASK_OBJ,
                               TRUE);
    }
}

// Blends two palettes of 16 colors by the cosine of an angle, and copies the result to a palette at the next VBlank
static void LocalTvtUtil_BlendPalette(NNS_GFD_DST_TYPE type, u16 *dest, u16 angle, u32 palette, const u16 *from,
                                      const u16 *to) {
    int i;

    for (i = 0; i < 16; i++) {
        fx16 cosine = FX_CosIdx(angle);
        fx16 rate = (cosine + FX16_ONE) / 2;
        u8 fromR = from[i] & GX_RGB_R_MASK;
        u8 fromG = (from[i] & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
        u8 fromB = (from[i] & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
        u8 toR = to[i] & GX_RGB_R_MASK;
        u8 toG = (to[i] & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;
        u8 toB = (to[i] & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
        u8 r = fromR + (((toR - fromR) * rate) >> FX32_SHIFT);
        u8 g = fromG + (((toG - fromG) * rate) >> FX32_SHIFT);
        u8 b = fromB + (((toB - fromB) * rate) >> FX32_SHIFT);

        dest[i] = GX_RGB(r, g, b);
    }
    NNS_GfdRegisterNewVramTransferTask(type, palette * 0x20, dest, 0x20);
}

static void LocalTvtReturnIcon_Init(LocalTvtReturnIcon *icon, ClActUnit *unit, HeapID heapId) {
    ArcTool *arc;
    ClActorSetup setup;

    sys_memset(icon, 0, sizeof(LocalTvtReturnIcon));
    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), heapId);
    icon->palette = func_0204bbb8(arc, func_0202d810(), 1, 11 * 0x20, 0, 3, heapId);
    icon->chars = func_0204b81c(arc, func_0202d814(), FALSE, 1, heapId);
    icon->cellAnims = func_0204bde0(arc, func_0202d818(2), func_0202d81c(2), heapId);
    GFL_ArcToolFree(arc);
    sys_memset(&setup, 0, sizeof(setup));
    setup.x = 224;
    setup.y = BAR_Y * 8;
    icon->actor = func_0204c040(unit, icon->chars, icon->palette, icon->cellAnims, &setup, 1, heapId);
    func_0204c124(icon->actor, TRUE);
    func_0204c520(icon->actor, TRUE);
}

static void LocalTvtReturnIcon_Exit(LocalTvtReturnIcon *icon) {
    func_0204c108(icon->actor);
    func_0204bcd0(icon->palette);
    func_0204b98c(icon->chars);
    func_0204be64(icon->cellAnims);
    sys_memset(icon, 0, sizeof(LocalTvtReturnIcon));
}

static ClActor *LocalTvtReturnIcon_GetActor(LocalTvtReturnIcon *icon) {
    return icon->actor;
}

LocalTvtKeyTouch *LocalTvtKeyTouch_Create(const LocalTvtKeyTouchSetup *setup, HeapID heapId) {
    LocalTvtKeyTouch *input = GFL_HeapAllocate(heapId, sizeof(LocalTvtKeyTouch), TRUE, "local_tvt_util.c", 1991);

    input->setup = *setup;
    input->touchMode = func_0203d554();
    LocalTvtKeyTouch_CallDraw(input);
    return input;
}

void LocalTvtKeyTouch_Free(LocalTvtKeyTouch *input) {
    GFL_HeapFree(input);
}

// With the touch screen in use, any key brings the cursor back; with the keys, a touch hides it
void LocalTvtKeyTouch_Main(LocalTvtKeyTouch *input) {
    if (input->touchMode == TRUE) {
        if (GCTX_HIDGetPressedKeys()) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            input->touchMode = FALSE;
            LocalTvtKeyTouch_CallDraw(input);
        } else if (LocalTvtKeyTouch_CallTouch(input) == LOCAL_TVT_INPUT_DECIDE) {
            func_0203d564(TRUE);
            input->decided = TRUE;
        }
    } else {
        switch (LocalTvtKeyTouch_CallTouch(input)) {
        case LOCAL_TVT_INPUT_DECIDE:
            func_0203d564(TRUE);
            input->decided = TRUE;
            break;
        case LOCAL_TVT_INPUT_MOVE:
            input->touchMode = TRUE;
            LocalTvtKeyTouch_CallDraw(input);
            break;
        case LOCAL_TVT_INPUT_NONE:
            if (LocalTvtKeyTouch_CallKey(input) == LOCAL_TVT_INPUT_DECIDE) {
                func_0203d564(FALSE);
                input->decided = TRUE;
            }
            break;
        }
    }
}

BOOL LocalTvtKeyTouch_IsDecided(LocalTvtKeyTouch *input) {
    return input->decided;
}

static void LocalTvtKeyTouch_Reset(LocalTvtKeyTouch *input) {
    input->decided = FALSE;
    input->touchMode = func_0203d554();
    LocalTvtKeyTouch_CallDraw(input);
}

static int LocalTvtKeyTouch_CallTouch(LocalTvtKeyTouch *input) {
    if (input->setup.touch != NULL) {
        return input->setup.touch(input->setup.work);
    }
    return LOCAL_TVT_INPUT_NONE;
}

static int LocalTvtKeyTouch_CallKey(LocalTvtKeyTouch *input) {
    if (input->setup.key != NULL) {
        return input->setup.key(input->setup.work);
    }
    return LOCAL_TVT_INPUT_NONE;
}

static void LocalTvtKeyTouch_CallDraw(LocalTvtKeyTouch *input) {
    if (input->setup.draw != NULL) {
        input->setup.draw(input->setup.work, input->touchMode);
    }
}
