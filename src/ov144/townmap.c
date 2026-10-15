#include "types.h"
#include "app/townmap.h"
#include "app/townmap/townmap_data.h"
#include "app/ui/touchbar.h"
#include "app/ui/ui_scene.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "constants/zones.h"
#include "field/townmap_util.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/calctool.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/math.h"
#include "save/join_avenue.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/town_map.h"

// The town map: the process, and the parts it is made of, each with its own work: the sequence, the touch bar with
// the zoom button, the cursor, the places and the player's marker, the map's BGs, the description on the upper
// screen, the message and the place's name, and the dragging of the map. The ROM names the file; all of the range is
// this one file, since every part is only reached from the process's functions and its .rodata is a single run. It
// ends with the empty VBlank task that the process adds, just before townmap_grh.c's TownMapGraphic_Create

#define ARCID_TOWNMAP_GRA 84

// The parameters of the process, which the field's callers allocate (they declare the layout themselves)
typedef struct {
    // TOWNMAP_MODE_*
    u32 mode;
    // TOWNMAP_RESULT_*
    u32 result;
    // The zone the player is in, then the zone picked to fly to
    u16 zoneId;
    u16 escapeZoneId;
    // Where to land in the zone picked
    u32 warpX;
    u32 warpZ;
    GameSystem *gsys;
} TownMapParam;

// A place's parameters that townmap_data.h doesn't name: where a flight to it lands
#define TOWNMAP_PARAM_WARP_X 13
#define TOWNMAP_PARAM_WARP_Z 14

// The center of the screen, which the map zooms about
#define TOWNMAP_CENTER_X 128
#define TOWNMAP_CENTER_Y 96
// Where the cursor starts when the player is nowhere on the map
#define TOWNMAP_CURSOR_X 118
#define TOWNMAP_CURSOR_Y 78
// The area the cursor moves in, and the area of the map that the touch screen drags
#define TOWNMAP_CURSOR_LEFT 0
#define TOWNMAP_CURSOR_RIGHT 248
#define TOWNMAP_CURSOR_TOP 8
#define TOWNMAP_CURSOR_BOTTOM 168
// How far the zoomed map scrolls from the center, and how many frames the zoom takes
#define TOWNMAP_SCROLL_TOP (-96)
#define TOWNMAP_SCROLL_BOTTOM 72
#define TOWNMAP_SCROLL_LEFT (-128)
#define TOWNMAP_SCROLL_RIGHT 128
#define TOWNMAP_ZOOM_FRAMES 10
// The places' markers scroll in their own limits
#define TOWNMAP_PLACES_TOP (-72)
#define TOWNMAP_PLACES_BOTTOM 96

// What the touch bar's own icons are, and the zoom button, which the bar doesn't know of
#define TOWNMAP_ICON_CLOSE 0
#define TOWNMAP_ICON_RETURN 1
#define TOWNMAP_ICON_ZOOM TOUCHBAR_ICON_CUSTOM
#define TOWNMAP_ICON_NONE 0xffffffff

// The animations of the map's cells
enum {
    TOWNMAP_ANIM_NAME_PLATE = 0,
    TOWNMAP_ANIM_CITY = 1,
    TOWNMAP_ANIM_CITY_SELECTED = 2,
    TOWNMAP_ANIM_SELECTED = 3,
    TOWNMAP_ANIM_PLAYER = 6,
    // The zoom button, at rest and pressed, when zoomed out and in
    TOWNMAP_ANIM_ZOOM_IN = 7,
    TOWNMAP_ANIM_ZOOM_OUT = 8,
    TOWNMAP_ANIM_ZOOM_IN_PRESSED = 9,
    TOWNMAP_ANIM_ZOOM_OUT_PRESSED = 10,
    TOWNMAP_ANIM_NONE = 0xff,
};

typedef struct {
    s32 x;
    s32 y;
} TownMapPoint;

typedef struct TownMapSeq TownMapSeq;
typedef void (*TownMapSeqFunc)(TownMapSeq *seq, u32 *state, void *work);

struct TownMapSeq {
    TownMapSeqFunc func;
    BOOL ended;
    u32 state;
    void *work;
};

typedef struct {
    u32 pltt;
    u32 chars;
    u32 cellAnims;
    TouchBar *bar;
    GameData *gameData;
    ClActor *zoomButton;
    u32 decided;
    u32 state;
} TownMapAppBar;

typedef struct TownMapPlace TownMapPlace;

// The parameters a place keeps scaled: TOWNMAP_PARAM_X to TOWNMAP_PARAM_HIT_RADIUS, the last of which is a length
#define TOWNMAP_PLACE_PARAM_COUNT 9

typedef struct {
    ClActor *cursor;
    ClActor *ring;
    ClActorPos pos;
    BOOL decided;
    // Whether the cursor is pulled to the nearest place
    BOOL pull;
    // The place the ring is around
    TownMapPlace *place;
} TownMapCursor;

struct TownMapPlace {
    ClActor *actor;
    void *data;
    u32 index;
    s32 params[TOWNMAP_PLACE_PARAM_COUNT];
    u8 anim;
    u8 animSelected;
    u8 animNow;
    fx32 scale;
    BOOL arrived;
    BOOL canFly;
    BOOL hidden;
};

typedef struct {
    ClActor *actor;
    TownMapPlace *place;
} TownMapPlayer;

typedef struct {
    GameData *gameData;
    u32 unk4;
    TownMapPlace *places;
    u32 count;
    TownMapPoint scroll;
    s16 limitTop;
    s16 limitLeft;
    s16 limitRight;
    s16 limitBottom;
    TownMapPlayer player;
} TownMapPlaces;

typedef struct {
    u8 bgBack;
    u8 bgFront;
    u16 frame;
    s16 limitTop;
    s16 limitLeft;
    s16 limitRight;
    s16 limitBottom;
    // Set for the frame the zoom ends in
    BOOL finished;
    BOOL moving;
    fx32 scale;
    fx32 scaleEnd;
    fx32 scaleStart;
    TownMapPoint pos;
    TownMapPoint end;
    TownMapPoint start;
} TownMapMap;

typedef struct {
    GameData *gameData;
    BmpWin *window;
    StrBuf *strbuf;
    Font *font;
    WordSet *wordSet;
    MsgData *placeNames;
    MsgData *msgData;
} TownMapInfo;

typedef struct {
    BmpWin *window;
    StrBuf *strbuf;
    Font *font;
    MsgData *msgData;
} TownMapMsgWin;

typedef struct {
    GameData *gameData;
    TownMapMsgWin msgWin;
    ClActor *plate;
    TownMapPlace *place;
    // Waiting for the plate's animation before the name is drawn
    BOOL opening;
    BOOL requested;
} TownMapPlaceName;

typedef struct {
    VecFx32 start;
    VecFx32 now;
    u32 unk18;
    BOOL dragging;
    // Whether the next drag is measured from where this one ended
    BOOL restart;
    VecFx32 delta;
} TownMapTouch;

typedef struct {
    TownMapGraphic *graphic;
    TownMapSeq seq;
    TownMapAppBar appBar;
    void *data;
    TownMapCursor cursor;
    TownMapPlaces places;
    TownMapMap map;
    TownMapInfo info;
    TownMapMsgWin msgWin;
    TownMapPlaceName placeName;
    TownMapTouch touch;
    Font *font;
    MsgData *placeNames;
    MsgData *msgData;
    TownMapParam *param;
    TownMapPlace *selected;
    BOOL zoomed;
    TCB *vblankTask;
    u32 unk14C;
} TownMapWork;

static BOOL TownMap_ProcInit(GameProc *proc, u32 *state, void *paramPtr, void *workPtr);
static BOOL TownMap_ProcExit(GameProc *proc, u32 *state, void *paramPtr, void *workPtr);
static BOOL TownMap_ProcMain(GameProc *proc, u32 *state, void *paramPtr, void *workPtr);
static void TownMapSeq_Init(TownMapSeq *seq, void *work, TownMapSeqFunc func);
static void TownMapSeq_Free(TownMapSeq *seq);
static void TownMapSeq_Main(TownMapSeq *seq);
static BOOL TownMapSeq_IsEnd(TownMapSeq *seq);
static void TownMapSeq_SetNext(TownMapSeq *seq, TownMapSeqFunc func);
static void TownMapSeq_End(TownMapSeq *seq);
static void TownMap_SeqFadeIn(TownMapSeq *seq, u32 *state, void *data);
static void TownMap_SeqFadeOut(TownMapSeq *seq, u32 *state, void *data);
static void TownMap_SeqMain(TownMapSeq *seq, u32 *state, void *data);
static void TownMapAppBar_Init(TownMapAppBar *appBar, u32 mode, ClActUnit *unit, u8 bg, u8 bgPalette, u8 objPalette,
                               GameData *gameData, HeapID heapId);
static void TownMapAppBar_Free(TownMapAppBar *appBar);
static void TownMapAppBar_Main(TownMapAppBar *appBar, TownMapMap *map);
static u32 TownMapAppBar_GetDecided(TownMapAppBar *appBar);
static BOOL TownMapAppBar_IsTouching(TownMapAppBar *appBar);
static void TownMapCursor_Init(TownMapCursor *cursor, ClActor *actor, ClActor *ring, HeapID heapId);
static void TownMapCursor_Free(TownMapCursor *cursor);
static void TownMapCursor_Main(TownMapCursor *cursor, TownMapPlaces *places);
static void TownMapCursor_GetPos(const TownMapCursor *cursor, TownMapPoint *pos);
static void TownMapCursor_SetPos(TownMapCursor *cursor, const TownMapPoint *pos);
static BOOL TownMapCursor_IsDecided(const TownMapCursor *cursor);
static u32 TownMapCursor_GetHeldKeys(const TownMapCursor *cursor);
static void TownMapCursor_SetPlace(TownMapCursor *cursor, TownMapPlace *place, const TownMapPoint *scroll);
static void TownMapCursor_UpdateRing(TownMapCursor *cursor, const TownMapPoint *scroll);
static TownMapPlace *TownMapCursor_GetPlace(const TownMapCursor *cursor);
static void TownMapCursor_SetPull(TownMapCursor *cursor, BOOL pull);
static void TownMapPlaces_Init(TownMapPlaces *places, u16 zoneId, u16 escapeZoneId, void *data,
                               TownMapGraphic *graphic, GameData *gameData, HeapID heapId, BOOL all);
static void TownMapPlaces_Free(TownMapPlaces *places);
static void TownMapPlaces_Main(TownMapPlaces *places);
static void TownMapPlaces_SetScale(TownMapPlaces *places, fx32 scale, const TownMapPoint *center);
static void TownMapPlaces_SetScroll(TownMapPlaces *places, const TownMapPoint *scroll);
static void TownMapPlaces_GetScroll(const TownMapPlaces *places, TownMapPoint *scroll);
static void TownMapPlaces_AddScroll(TownMapPlaces *places, const TownMapPoint *move);
static TownMapPlace *TownMapPlaces_GetHit(const TownMapPlaces *places, const TownMapCursor *cursor);
static TownMapPlace *TownMapPlaces_GetNearest(const TownMapPlaces *places, const TownMapCursor *cursor);
static s32 TownMapPlace_GetParam(const TownMapPlace *place, int param);
static void TownMapPlaces_Select(TownMapPlaces *places, const TownMapPlace *place);
static void TownMapPlaces_Deselect(TownMapPlaces *places);
static TownMapPlace *TownMapPlaces_GetByZone(const TownMapPlaces *places, u16 zoneId, u16 escapeZoneId);
static BOOL TownMapPlace_IsDescribed(const TownMapPlace *place);
static void TownMapPlace_Init(TownMapPlace *place, void *data, u32 index, ClActUnit *unit, u32 chars, u32 cellAnims,
                              u32 pltt, GameData *gameData, HeapID heapId, BOOL all);
static void TownMapPlace_Free(TownMapPlace *place);
static void TownMapPlace_SetVisible(TownMapPlace *place, BOOL visible);
static BOOL TownMapPlace_IsHit(const TownMapPlace *place, const TownMapPoint *pos, const TownMapPoint *scroll);
static BOOL TownMapPlace_GetDistance(const TownMapPlace *place, const TownMapPoint *pos, const TownMapPoint *scroll,
                                     u32 *distance);
static void TownMapPlace_SetScale(TownMapPlace *place, fx32 scale, const TownMapPoint *center);
static void TownMapPlace_UpdatePos(TownMapPlace *place, const TownMapPoint *scroll);
static void TownMapPlace_SetAnim(TownMapPlace *place, u8 anim);
static void TownMapPlace_Select(TownMapPlace *place);
static void TownMapPlace_Deselect(TownMapPlace *place);
static void TownMapPlayer_Init(TownMapPlayer *player, TownMapPlace *place, ClActUnit *unit, u32 chars, u32 cellAnims,
                               u32 pltt, HeapID heapId);
static void TownMapPlayer_Free(TownMapPlayer *player);
static void TownMapPlayer_UpdatePos(TownMapPlayer *player, const TownMapPoint *scroll);
static void TownMapMap_Init(TownMapMap *map, u8 bgBack, u8 bgFront, HeapID heapId);
static void TownMapMap_Free(TownMapMap *map);
static void TownMapMap_Main(TownMapMap *map, TownMapPlaces *places);
static void TownMapMap_StartZoom(TownMapMap *map, fx32 scale, const TownMapPoint *start, const TownMapPoint *end);
static BOOL TownMapMap_IsFinished(const TownMapMap *map);
static BOOL TownMapMap_IsMoving(const TownMapMap *map);
static void TownMapMap_SetPos(TownMapMap *map, const TownMapPoint *pos);
static void TownMapMap_GetPos(const TownMapMap *map, TownMapPoint *pos);
static void TownMapMap_AddPos(TownMapMap *map, const TownMapPoint *move);
static void TownMapInfo_Init(TownMapInfo *info, u8 bg, Font *font, MsgData *placeNames, MsgData *msgData,
                             GameData *gameData, HeapID heapId);
static void TownMapInfo_Free(TownMapInfo *info);
static void TownMapInfo_Draw(TownMapInfo *info, TownMapPlace *place);
static void TownMapInfo_Clear(TownMapInfo *info);
static void TownMapMsgWin_Init(TownMapMsgWin *msgWin, u8 bg, Font *font, MsgData *msgData, u8 x, u8 y, u8 width,
                               u8 height, HeapID heapId);
static void TownMapMsgWin_Free(TownMapMsgWin *msgWin);
static void TownMapMsgWin_Print(TownMapMsgWin *msgWin, u32 messageId, u16 x, u16 y);
static void TownMapMsgWin_PrintStrBuf(TownMapMsgWin *msgWin, const StrBuf *strbuf, u16 x, u16 y);
static void TownMapMsgWin_Clear(TownMapMsgWin *msgWin);
static void TownMapPlaceName_Init(TownMapPlaceName *placeName, u8 bg, Font *font, MsgData *placeNames, ClActor *plate,
                                  GameData *gameData, HeapID heapId);
static void TownMapPlaceName_Free(TownMapPlaceName *placeName);
static void TownMapPlaceName_Main(TownMapPlaceName *placeName);
static void TownMapPlaceName_SetPlace(TownMapPlaceName *placeName, TownMapPlace *place);
static void TownMapPlaceName_SetVisible(TownMapPlaceName *placeName, BOOL visible);
static void TownMapTouch_Init(TownMapTouch *touch, HeapID heapId);
static void TownMapTouch_Free(TownMapTouch *touch);
static void TownMapTouch_Main(TownMapTouch *touch);
static BOOL TownMapTouch_GetDrag(const TownMapTouch *touch, VecFx32 *delta, u32 *unk18);
static void TownMapTouch_Restart(TownMapTouch *touch);
static void TownMap_VBlank(TCB *tcb, void *data);

static const TownMapPoint sTownMapCenter = { TOWNMAP_CENTER_X, TOWNMAP_CENTER_Y };

const GameProcFunctions TOWNMAP_PROC_FUNCTIONS = { TownMap_ProcInit, TownMap_ProcMain, TownMap_ProcExit };

static BOOL TownMap_ProcInit(GameProc *proc, u32 *state, void *paramPtr, void *workPtr) {
    TownMapParam *param = paramPtr;
    TownMapWork *work;
    GameData *gameData = GSYS_GetGameData(param->gsys);
    TownMapPlace *place;
    BOOL all;

    GFL_OvlLoad(OVERLAY_APP_UI);
    all = TRUE;
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_TOWNMAP, 0x60000);
    work = GFL_ProcInitSubsystem(proc, sizeof(TownMapWork), HEAPID_TOWNMAP);
    sys_memset(work, 0, sizeof(TownMapWork));
    work->param = param;
    if (work->param->mode != TOWNMAP_MODE_FLY_ALL) {
        all = FALSE;
    }
    work->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_TOWNMAP);
    work->placeNames = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_PLACE_NAMES, HEAPID_TOWNMAP);
    work->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TOWN_MAP, HEAPID_TOWNMAP);
    work->graphic = TownMapGraphic_Create(HEAPID_TOWNMAP);
    work->data = TownMapData_Load(HEAPID_TOWNMAP);
    TownMapSeq_Init(&work->seq, work, TownMap_SeqFadeIn);
    if (work->param->mode != TOWNMAP_MODE_VIEW) {
        TownMapMsgWin_Init(&work->msgWin, TownMapGraphic_GetBG(work->graphic, TOWNMAP_BG_TEXT), work->font,
                           work->msgData, 0, 21, 24, 3, HEAPID_TOWNMAP);
        GFL_TextRndUpdateColorIndexLUT(15, 1, 0);
        TownMapMsgWin_Print(&work->msgWin, TownMap_Text_FlyWhere, 16, 0);
        func_020232d8();
    }
    TownMapAppBar_Init(&work->appBar, work->param->mode, TownMapGraphic_GetUnit(work->graphic, TOWNMAP_UNIT_MAIN),
                       TownMapGraphic_GetBG(work->graphic, TOWNMAP_BG_BAR), 15, 8, gameData, HEAPID_TOWNMAP);
    TownMapPlaceName_Init(&work->placeName, TownMapGraphic_GetBG(work->graphic, TOWNMAP_BG_TEXT), work->font,
                          work->placeNames, TownMapGraphic_GetActor(work->graphic, TOWNMAP_ACTOR_NAME_PLATE), gameData,
                          HEAPID_TOWNMAP);
    GFL_BGSysMoveBG(TownMapGraphic_GetBG(work->graphic, TOWNMAP_BG_TEXT), BG_MOVE_SET_Y, -4);
    TownMapPlaces_Init(&work->places, param->zoneId, param->escapeZoneId, work->data, work->graphic, gameData,
                       HEAPID_TOWNMAP, all);
    TownMapCursor_Init(&work->cursor, TownMapGraphic_GetActor(work->graphic, TOWNMAP_ACTOR_CURSOR),
                       TownMapGraphic_GetActor(work->graphic, TOWNMAP_ACTOR_RING), HEAPID_TOWNMAP);
    place = TownMapPlaces_GetByZone(&work->places, param->zoneId, param->escapeZoneId);
    {
        TownMapPoint pos;

        if (place != NULL) {
            pos.x = TownMapPlace_GetParam(place, TOWNMAP_PARAM_CURSOR_X);
            pos.y = TownMapPlace_GetParam(place, TOWNMAP_PARAM_CURSOR_Y);
        } else {
            pos.x = TOWNMAP_CURSOR_X;
            pos.y = TOWNMAP_CURSOR_Y;
        }
        TownMapCursor_SetPos(&work->cursor, &pos);
    }
    TownMapMap_Init(&work->map, TownMapGraphic_GetBG(work->graphic, TOWNMAP_BG_MAP_BACK),
                    TownMapGraphic_GetBG(work->graphic, TOWNMAP_BG_MAP_FRONT), HEAPID_TOWNMAP);
    TownMapInfo_Init(&work->info, TownMapGraphic_GetBG(work->graphic, TOWNMAP_BG_INFO), work->font, work->placeNames,
                     work->msgData, gameData, HEAPID_TOWNMAP);
    TownMapTouch_Init(&work->touch, HEAPID_TOWNMAP);
    place = TownMapPlaces_GetHit(&work->places, &work->cursor);
    if (place != NULL) {
        TownMapPoint scroll;

        TownMapPlaceName_SetPlace(&work->placeName, place);
        TownMapInfo_Draw(&work->info, place);
        TownMapPlaces_Select(&work->places, place);
        work->selected = place;
        TownMapPlaces_GetScroll(&work->places, &scroll);
        TownMapCursor_SetPlace(&work->cursor, place, &scroll);
    }
    if (func_02042788()) {
        func_02042ba8(TRUE, HEAPID_TOWNMAP);
    }
    work->vblankTask = GFL_VBlankTCBAdd(TownMap_VBlank, work, 0);
    return TRUE;
}

static BOOL TownMap_ProcExit(GameProc *proc, u32 *state, void *paramPtr, void *workPtr) {
    TownMapWork *work = workPtr;

    GFL_TCBRemove(work->vblankTask);
    TownMapTouch_Free(&work->touch);
    TownMapInfo_Free(&work->info);
    TownMapMap_Free(&work->map);
    TownMapPlaces_Free(&work->places);
    TownMapCursor_Free(&work->cursor);
    TownMapPlaceName_Free(&work->placeName);
    if (work->param->mode != TOWNMAP_MODE_VIEW) {
        TownMapMsgWin_Free(&work->msgWin);
    }
    TownMapAppBar_Free(&work->appBar);
    TownMapSeq_Free(&work->seq);
    TownMapData_Free(work->data);
    TownMapGraphic_Free(work->graphic);
    GFL_MsgDataFree(work->msgData);
    GFL_MsgDataFree(work->placeNames);
    GFL_FontFree(work->font);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_TOWNMAP);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return TRUE;
}

static BOOL TownMap_ProcMain(GameProc *proc, u32 *state, void *paramPtr, void *workPtr) {
    TownMapWork *work = workPtr;

    TownMapSeq_Main(&work->seq);
    TownMapGraphic_Update(work->graphic);
    if (TownMapSeq_IsEnd(&work->seq)) {
        return TRUE;
    }
    return FALSE;
}

static void TownMapSeq_Init(TownMapSeq *seq, void *work, TownMapSeqFunc func) {
    sys_memset(seq, 0, sizeof(TownMapSeq));
    seq->work = work;
    TownMapSeq_SetNext(seq, func);
}

static void TownMapSeq_Free(TownMapSeq *seq) {
    sys_memset(seq, 0, sizeof(TownMapSeq));
}

static void TownMapSeq_Main(TownMapSeq *seq) {
    if (!seq->ended) {
        seq->func(seq, &seq->state, seq->work);
    }
}

static BOOL TownMapSeq_IsEnd(TownMapSeq *seq) {
    return seq->ended;
}

static void TownMapSeq_SetNext(TownMapSeq *seq, TownMapSeqFunc func) {
    seq->func = func;
    seq->state = 0;
}

static void TownMapSeq_End(TownMapSeq *seq) {
    seq->ended = TRUE;
}

static void TownMap_SeqFadeIn(TownMapSeq *seq, u32 *state, void *data) {
    switch (*state) {
    case 0:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
        *state = 1;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            *state = 2;
        }
        break;
    case 2:
        TownMapSeq_SetNext(seq, TownMap_SeqMain);
        break;
    }
}

static void TownMap_SeqFadeOut(TownMapSeq *seq, u32 *state, void *data) {
    switch (*state) {
    case 0:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 0);
        *state = 1;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            *state = 2;
        }
        break;
    case 2:
        TownMapSeq_End(seq);
        break;
    }
}

static void TownMap_SeqMain(TownMapSeq *seq, u32 *state, void *data) {
    TownMapWork *work = data;
    BOOL scrolled = FALSE;

    if (!TownMapAppBar_IsTouching(&work->appBar)) {
        // The zoomed map scrolls when the cursor is pushed against an edge, or with a drag
        if (work->zoomed && !TownMapMap_IsMoving(&work->map)) {
            TownMapPoint pos;
            TownMapPoint scroll = { 0, 0 };
            u32 keys;

            TownMapCursor_GetPos(&work->cursor, &pos);
            keys = TownMapCursor_GetHeldKeys(&work->cursor);
            if (pos.y < 16 && (keys & PAD_KEY_UP)) {
                scroll.y -= 4;
                scrolled = TRUE;
            }
            if (pos.y <= TOWNMAP_CURSOR_BOTTOM && pos.y > 136 && (keys & PAD_KEY_DOWN)) {
                scroll.y += 4;
                scrolled = TRUE;
            }
            if (pos.x < 16 && (keys & PAD_KEY_LEFT)) {
                scroll.x -= 4;
                scrolled = TRUE;
            }
            if (pos.x > 240 && (keys & PAD_KEY_RIGHT)) {
                scroll.x += 4;
                scrolled = TRUE;
            }
            if (!TownMapMap_IsMoving(&work->map)) {
                VecFx32 delta;
                u32 unk18;

                if (TownMapTouch_GetDrag(&work->touch, &delta, &unk18)) {
                    scroll.x = -FX_Mul(delta.x, FX32_ONE) >> FX32_SHIFT;
                    scroll.y = -FX_Mul(delta.y, FX32_ONE) >> FX32_SHIFT;
                    TownMapTouch_Restart(&work->touch);
                    scrolled = TRUE;
                }
            }
            if (scrolled) {
                TownMapPoint placesScroll;
                TownMapPoint ringScroll;

                TownMapPlaces_GetScroll(&work->places, &placesScroll);
                TownMapMap_AddPos(&work->map, &scroll);
                scroll.x *= -1;
                scroll.y *= -1;
                TownMapPlaces_AddScroll(&work->places, &scroll);
                TownMapPlaces_GetScroll(&work->places, &ringScroll);
                TownMapCursor_UpdateRing(&work->cursor, &ringScroll);
            }
        }
        if (!scrolled && !TownMapMap_IsMoving(&work->map)) {
            BOOL decided;

            if (work->param->mode == TOWNMAP_MODE_FLY || work->param->mode == TOWNMAP_MODE_FLY_ALL) {
                TownMapPlace *ringed = TownMapCursor_GetPlace(&work->cursor);
                TownMapPlace *hit = TownMapPlaces_GetHit(&work->places, &work->cursor);

                if (ringed != NULL && ringed == hit && TownMapCursor_IsDecided(&work->cursor) &&
                    TownMapPlace_GetParam(ringed, TOWNMAP_PARAM_FLY)) {
                    work->param->result = TOWNMAP_RESULT_FLY;
                    work->param->zoneId = TownMapPlace_GetParam(ringed, TOWNMAP_PARAM_ZONE);
                    work->param->warpX = TownMapPlace_GetParam(ringed, TOWNMAP_PARAM_WARP_X);
                    work->param->warpZ = TownMapPlace_GetParam(ringed, TOWNMAP_PARAM_WARP_Z);
                    // Join Avenue is flown to from Route 4
                    if (work->param->zoneId == ZONE_JOIN_AVENUE) {
                        work->param->zoneId = ZONE_ROUTE_4;
                    }
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                    TownMapSeq_SetNext(seq, TownMap_SeqFadeOut);
                }
            }
            // With the keys, the place under the cursor is selected at once; a touch has to decide it
            if (func_0203d554() == FALSE) {
                decided = TRUE;
            } else {
                decided = TownMapCursor_IsDecided(&work->cursor);
            }
            if (decided) {
                TownMapPlace *place = TownMapPlaces_GetHit(&work->places, &work->cursor);

                if (place != work->selected && place != NULL) {
                    TownMapPoint placesScroll;

                    GFL_SndSEPlay(SEQ_SE_SELECT1);
                    TownMapInfo_Draw(&work->info, place);
                    TownMapPlaces_Select(&work->places, place);
                    work->selected = place;
                    TownMapPlaces_GetScroll(&work->places, &placesScroll);
                    TownMapCursor_SetPlace(&work->cursor, place, &placesScroll);
                    if (!work->zoomed) {
                        TownMapPlaceName_SetPlace(&work->placeName, place);
                    }
                } else if (place == NULL && work->selected != NULL) {
                    TownMapInfo_Clear(&work->info);
                    TownMapPlaceName_SetVisible(&work->placeName, FALSE);
                    TownMapPlaces_Deselect(&work->places);
                    TownMapCursor_SetPlace(&work->cursor, NULL, NULL);
                    work->selected = NULL;
                }
            }
        }
        if (work->zoomed) {
            TownMapPlaceName_SetVisible(&work->placeName, FALSE);
        }
        TownMapCursor_SetPull(&work->cursor, scrolled ? FALSE : TRUE);
    }
    TownMapAppBar_Main(&work->appBar, &work->map);
    if (!TownMapMap_IsMoving(&work->map)) {
        TownMapCursor_Main(&work->cursor, &work->places);
    }
    TownMapPlaces_Main(&work->places);
    TownMapMap_Main(&work->map, &work->places);
    TownMapPlaceName_Main(&work->placeName);
    TownMapTouch_Main(&work->touch);
    switch (TownMapAppBar_GetDecided(&work->appBar)) {
    case TOWNMAP_ICON_CLOSE:
        work->param->result = TOWNMAP_RESULT_CLOSE;
        TownMapSeq_SetNext(seq, TownMap_SeqFadeOut);
        break;
    case TOWNMAP_ICON_RETURN:
        work->param->result = TOWNMAP_RESULT_RETURN;
        TownMapSeq_SetNext(seq, TownMap_SeqFadeOut);
        break;
    case TOWNMAP_ICON_ZOOM:
        if (!work->zoomed && !TownMapMap_IsMoving(&work->map)) {
            TownMapPoint pos;

            TownMapCursor_GetPos(&work->cursor, &pos);
            TownMapMap_StartZoom(&work->map, 2 * FX32_ONE, &sTownMapCenter, &pos);
            GFL_SndSEPlay(SEQ_SE_SYS_09);
            if (work->selected != NULL) {
                TownMapInfo_Clear(&work->info);
                TownMapPlaceName_SetVisible(&work->placeName, FALSE);
                TownMapPlaces_Deselect(&work->places);
                TownMapCursor_SetPlace(&work->cursor, NULL, NULL);
                work->selected = NULL;
            }
            work->zoomed = TRUE;
        } else if (work->zoomed == TRUE && !TownMapMap_IsMoving(&work->map)) {
            TownMapPoint pos;

            TownMapMap_GetPos(&work->map, &pos);
            pos.x += TOWNMAP_CENTER_X;
            pos.y += TOWNMAP_CENTER_Y;
            TownMapMap_StartZoom(&work->map, FX32_ONE, &pos, &sTownMapCenter);
            GFL_SndSEPlay(SEQ_SE_SYS_10);
            if (work->selected != NULL) {
                TownMapInfo_Clear(&work->info);
                TownMapPlaceName_SetVisible(&work->placeName, FALSE);
                TownMapPlaces_Deselect(&work->places);
                TownMapCursor_SetPlace(&work->cursor, NULL, NULL);
                work->selected = NULL;
            }
            work->zoomed = FALSE;
        }
        break;
    }
    TownMapMap_IsFinished(&work->map);
}

static void TownMapAppBar_Init(TownMapAppBar *appBar, u32 mode, ClActUnit *unit, u8 bg, u8 bgPalette, u8 objPalette,
                               GameData *gameData, HeapID heapId) {
    ArcTool *arc;

    sys_memset(appBar, 0, sizeof(TownMapAppBar));
    appBar->gameData = gameData;
    arc = GFL_ArcSysCreateFileHandle(ARCID_TOWNMAP_GRA, heapId);
    appBar->pltt = func_0204bbb8(arc, 2, CLACT_VRAM_MAIN, 0, 0, 4, heapId);
    appBar->chars = func_0204b81c(arc, 8, FALSE, CLACT_VRAM_MAIN, heapId);
    appBar->cellAnims = func_0204bde0(arc, 12, 19, heapId);
    GFL_ArcToolFree(arc);
    {
        ClActorSetup setup;

        sys_memset(&setup, 0, sizeof(ClActorSetup));
        setup.x = 168;
        setup.y = 168;
        setup.sequence = TOWNMAP_ANIM_ZOOM_IN;
        appBar->zoomButton =
            func_0204c040(unit, appBar->chars, appBar->pltt, appBar->cellAnims, &setup, CLACT_SURFACE_MAIN, heapId);
        func_0204c520(appBar->zoomButton, TRUE);
        func_0204c550(appBar->zoomButton);
    }
    {
        TouchBarSetup setup;
        TouchBarItem items[] = {
            { TOWNMAP_ICON_RETURN, { 232, 168 } },
            { TOWNMAP_ICON_CLOSE, { 200, 168 } },
        };

        sys_memset(&setup, 0, sizeof(TouchBarSetup));
        setup.items = items;
        setup.count = NELEMS(items);
        setup.unit = unit;
        setup.bg = bg;
        setup.bgPalette = bgPalette;
        setup.objPalette = objPalette + 4;
        setup.vramType = 2;
        appBar->bar = TouchBar_Create(&setup, heapId);
        TouchBar_SetPriority(appBar->bar, 10);
    }
}

static void TownMapAppBar_Free(TownMapAppBar *appBar) {
    TouchBar_Free(appBar->bar);
    func_0204c108(appBar->zoomButton);
    func_0204be64(appBar->cellAnims);
    func_0204b98c(appBar->chars);
    func_0204bcd0(appBar->pltt);
    sys_memset(appBar, 0, sizeof(TownMapAppBar));
}

static void TownMapAppBar_Main(TownMapAppBar *appBar, TownMapMap *map) {
    TouchBar_Main(appBar->bar);
    appBar->decided = TOWNMAP_ICON_NONE;
    switch (appBar->state) {
    case 0: {
        BOOL pressed = FALSE;
        u32 x;
        u32 y;
        ClActorPos pos;

        func_0204c178(appBar->zoomButton, &pos, CLACT_SURFACE_MAIN);
        if (func_0203dac8(&x, &y)) {
            if (x - pos.x <= 24 && y - pos.y <= 24) {
                func_0203d564(TRUE);
                pressed = TRUE;
            }
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
            func_0203d564(FALSE);
            pressed = TRUE;
        }
        if (pressed && !func_0204c560(appBar->zoomButton) && !TownMapMap_IsMoving(map)) {
            if (func_0204c4a0(appBar->zoomButton) == TOWNMAP_ANIM_ZOOM_IN) {
                func_0204c488(appBar->zoomButton, TOWNMAP_ANIM_ZOOM_IN_PRESSED);
            } else {
                func_0204c488(appBar->zoomButton, TOWNMAP_ANIM_ZOOM_OUT_PRESSED);
            }
            func_0204c540(appBar->zoomButton);
            appBar->decided = TOWNMAP_ICON_ZOOM;
            appBar->state = 1;
        }
        break;
    }
    case 1:
        if (!func_0204c560(appBar->zoomButton)) {
            if (func_0204c4a0(appBar->zoomButton) == TOWNMAP_ANIM_ZOOM_IN_PRESSED) {
                func_0204c488(appBar->zoomButton, TOWNMAP_ANIM_ZOOM_OUT);
            } else {
                func_0204c488(appBar->zoomButton, TOWNMAP_ANIM_ZOOM_IN);
            }
            appBar->state = 0;
        }
        break;
    }
}

// The icon decided: one of the touch bar's, or the zoom button in the frame it is pressed
static u32 TownMapAppBar_GetDecided(TownMapAppBar *appBar) {
    u32 decided = TouchBar_GetDecided(appBar->bar);

    if (decided == TOWNMAP_ICON_NONE) {
        decided = appBar->decided;
    }
    return decided;
}

static BOOL TownMapAppBar_IsTouching(TownMapAppBar *appBar) {
    return TouchBar_IsTouching(appBar->bar);
}

static void TownMapCursor_Init(TownMapCursor *cursor, ClActor *actor, ClActor *ring, HeapID heapId) {
    sys_memset(cursor, 0, sizeof(TownMapCursor));
    cursor->pos.x = TOWNMAP_CURSOR_X;
    cursor->pos.y = TOWNMAP_CURSOR_Y;
    cursor->cursor = actor;
    cursor->ring = ring;
    func_0204c140(cursor->cursor, &cursor->pos, CLACT_SURFACE_MAIN);
    func_0204c140(cursor->ring, &cursor->pos, CLACT_SURFACE_MAIN);
    func_0204c124(cursor->cursor, TRUE);
    func_0204c124(cursor->ring, FALSE);
    func_0204c468(cursor->cursor, 0);
    func_0204c438(cursor->cursor, 0);
    func_0204c468(cursor->ring, 2);
    func_0204c438(cursor->ring, 3);
    func_0204c520(cursor->ring, TRUE);
    // The cursor shows only while the keys are used
    if (!func_0203d554()) {
        func_0204c124(cursor->cursor, TRUE);
    } else {
        func_0204c124(cursor->cursor, FALSE);
    }
}

static void TownMapCursor_Free(TownMapCursor *cursor) {
    sys_memset(cursor, 0, sizeof(TownMapCursor));
}

static void TownMapCursor_Main(TownMapCursor *cursor, TownMapPlaces *places) {
    VecFx32 move = { 0, 0, 0 };
    VecFx32 pull = { 0, 0, 0 };
    BOOL moved = FALSE;

    cursor->decided = FALSE;
    if (GCTX_HIDGetHeldKeys() & PAD_KEY_UP) {
        func_0203d564(FALSE);
        move.y -= FX32_ONE;
        moved = TRUE;
    }
    if (GCTX_HIDGetHeldKeys() & PAD_KEY_DOWN) {
        func_0203d564(FALSE);
        move.y += FX32_ONE;
        moved = TRUE;
    }
    if (GCTX_HIDGetHeldKeys() & PAD_KEY_LEFT) {
        func_0203d564(FALSE);
        move.x -= FX32_ONE;
        moved = TRUE;
    }
    if (GCTX_HIDGetHeldKeys() & PAD_KEY_RIGHT) {
        func_0203d564(FALSE);
        move.x += FX32_ONE;
        moved = TRUE;
    }
    // A place near the cursor pulls it in
    if (cursor->pull && !func_0203d554()) {
        TownMapPlace *place = TownMapPlaces_GetNearest(places, cursor);

        if (place != NULL) {
            TownMapPoint scroll;
            VecFx32 to;
            VecFx32 from;

            TownMapPlaces_GetScroll(places, &scroll);
            to.x = FX32_CONST(scroll.x + TownMapPlace_GetParam(place, TOWNMAP_PARAM_CURSOR_X));
            to.y = FX32_CONST(scroll.y + TownMapPlace_GetParam(place, TOWNMAP_PARAM_CURSOR_Y));
            to.z = 0;
            from.x = FX32_CONST(cursor->pos.x);
            from.y = FX32_CONST(cursor->pos.y);
            from.z = 0;
            VEC_Subtract(&to, &from, &pull);
            if (VEC_Mag(&pull) > FX32_ONE) {
                VEC_Normalize(&pull, &pull);
                vecfx_mul(&pull, FX32_ONE, &pull);
            }
            moved = TRUE;
        }
    }
    if (moved) {
        fx32 x;
        fx32 y;

        VEC_Normalize(&move, &move);
        x = pull.x + move.x * 3;
        y = pull.y + move.y * 3;
        // At least a pixel
        if (0 < x && x < FX32_ONE) {
            x = FX32_ONE;
        } else if (-FX32_ONE < x && x < 0) {
            x = -FX32_ONE;
        }
        if (0 < y && y < FX32_ONE) {
            y = FX32_ONE;
        } else if (-FX32_ONE < y && y < 0) {
            y = -FX32_ONE;
        }
        cursor->pos.x += x >> FX32_SHIFT;
        cursor->pos.y += y >> FX32_SHIFT;
        cursor->pos.x = MATH_CLAMP(cursor->pos.x, TOWNMAP_CURSOR_LEFT, TOWNMAP_CURSOR_RIGHT);
        cursor->pos.y = MATH_CLAMP(cursor->pos.y, TOWNMAP_CURSOR_TOP, TOWNMAP_CURSOR_BOTTOM);
        func_0204c140(cursor->cursor, &cursor->pos, CLACT_SURFACE_MAIN);
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        func_0203d564(FALSE);
        cursor->decided = TRUE;
    }
    {
        u32 x;
        u32 y;

        if (func_0203dac8(&x, &y)) {
            func_0203d564(TRUE);
            if (x <= TOWNMAP_CURSOR_RIGHT && y - TOWNMAP_CURSOR_TOP <= TOWNMAP_CURSOR_BOTTOM - TOWNMAP_CURSOR_TOP) {
                cursor->decided = TRUE;
                cursor->pos.x = x;
                cursor->pos.y = y;
            }
        }
    }
    if (!func_0203d554()) {
        func_0204c124(cursor->cursor, TRUE);
    } else {
        func_0204c124(cursor->cursor, FALSE);
    }
}

static void TownMapCursor_GetPos(const TownMapCursor *cursor, TownMapPoint *pos) {
    pos->x = cursor->pos.x;
    pos->y = cursor->pos.y;
}

static void TownMapCursor_SetPos(TownMapCursor *cursor, const TownMapPoint *pos) {
    cursor->pos.x = pos->x;
    cursor->pos.y = pos->y;
    func_0204c140(cursor->cursor, &cursor->pos, CLACT_SURFACE_MAIN);
}

static BOOL TownMapCursor_IsDecided(const TownMapCursor *cursor) {
    return cursor->decided;
}

static u32 TownMapCursor_GetHeldKeys(const TownMapCursor *cursor) {
    return GCTX_HIDGetHeldKeys();
}

// Puts the ring around a place, whose position is relative to the places' scroll, or hides it
static void TownMapCursor_SetPlace(TownMapCursor *cursor, TownMapPlace *place, const TownMapPoint *scroll) {
    if (place == NULL) {
        func_0204c124(cursor->ring, FALSE);
        cursor->place = NULL;
    } else if (cursor->place != place) {
        ClActorPos pos;

        pos.x = scroll->x + TownMapPlace_GetParam(place, TOWNMAP_PARAM_CURSOR_X);
        pos.y = scroll->y + TownMapPlace_GetParam(place, TOWNMAP_PARAM_CURSOR_Y);
        func_0204c140(cursor->ring, &pos, CLACT_SURFACE_MAIN);
        func_0204c124(cursor->ring, TRUE);
        func_0204c56c(cursor->ring);
        cursor->place = place;
    }
}

static void TownMapCursor_UpdateRing(TownMapCursor *cursor, const TownMapPoint *scroll) {
    if (cursor->place != NULL) {
        ClActorPos pos;

        pos.x = scroll->x + TownMapPlace_GetParam(cursor->place, TOWNMAP_PARAM_CURSOR_X);
        pos.y = scroll->y + TownMapPlace_GetParam(cursor->place, TOWNMAP_PARAM_CURSOR_Y);
        func_0204c140(cursor->ring, &pos, CLACT_SURFACE_MAIN);
    }
}

static TownMapPlace *TownMapCursor_GetPlace(const TownMapCursor *cursor) {
    return cursor->place;
}

static void TownMapCursor_SetPull(TownMapCursor *cursor, BOOL pull) {
    cursor->pull = pull;
}

// Makes every place of the table but the other version's city. all makes every place arrived at and flown to
static void TownMapPlaces_Init(TownMapPlaces *places, u16 zoneId, u16 escapeZoneId, void *data,
                               TownMapGraphic *graphic, GameData *gameData, HeapID heapId, BOOL all) {
    int i;
    u32 count = 0;
    ClActUnit *unit = TownMapGraphic_GetUnit(graphic, TOWNMAP_UNIT_MAIN);
    u32 chars;
    u32 cellAnims;
    u32 pltt;
    TownMapPlace *place;

    sys_memset(places, 0, sizeof(TownMapPlaces));
    places->count = TOWNMAP_PLACE_COUNT - 1;
    places->gameData = gameData;
    places->places = GFL_HeapAllocate(heapId, places->count * sizeof(TownMapPlace), FALSE, "townmap.c", 2261);
    sys_memset(places->places, 0, places->count * sizeof(TownMapPlace));
    TownMapGraphic_GetRes(graphic, &chars, &cellAnims, &pltt);
    for (i = 0; i < TOWNMAP_PLACE_COUNT; i++) {
        u32 offset = 0;
        u16 zone = TownMapData_GetParam(data, i, TOWNMAP_PARAM_ZONE);

        // Black City's place is followed by White Forest's, which is shown in its stead
        if (zone == ZONE_BLACK_CITY) {
            switch (func_02017220(gameData)) {
            case CITY_BLACK_CITY:
                break;
            case CITY_WHITE_FOREST:
                offset = 1;
                break;
            }
        }
        if (zone != ZONE_WHITE_FOREST) {
            TownMapPlace_Init(&places->places[count], data, i + offset, unit, chars, cellAnims, pltt, gameData, heapId,
                              all);
            count++;
        }
    }
    place = TownMapPlaces_GetByZone(places, zoneId, escapeZoneId);
    {
        u32 chars;
        u32 cellAnims;
        u32 pltt;

        TownMapGraphic_GetRes(graphic, &chars, &cellAnims, &pltt);
        TownMapPlayer_Init(&places->player, place, unit, chars, cellAnims, pltt, heapId);
    }
    places->limitTop = 0;
    places->limitLeft = 0;
    places->limitRight = 0;
    places->limitBottom = 0;
    TownMapPlaces_Deselect(places);
}

static void TownMapPlaces_Free(TownMapPlaces *places) {
    u32 i;

    TownMapPlayer_Free(&places->player);
    for (i = 0; i < places->count; i++) {
        TownMapPlace_Free(&places->places[i]);
    }
    GFL_HeapFree(places->places);
    sys_memset(places, 0, sizeof(TownMapPlaces));
}

static void TownMapPlaces_Main(TownMapPlaces *places) {
}

// Scales the places about the center of the screen, with the map scrolled to center
static void TownMapPlaces_SetScale(TownMapPlaces *places, fx32 scale, const TownMapPoint *center) {
    u32 i;
    TownMapPoint scroll;

    if (scale == FX32_ONE) {
        places->limitTop = 0;
        places->limitLeft = 0;
        places->limitRight = 0;
        places->limitBottom = 0;
    } else {
        places->limitTop = TOWNMAP_PLACES_TOP;
        places->limitLeft = TOWNMAP_SCROLL_LEFT;
        places->limitRight = TOWNMAP_SCROLL_RIGHT;
        places->limitBottom = TOWNMAP_PLACES_BOTTOM;
    }
    scroll.x = -center->x;
    scroll.y = -center->y;
    TownMapPlaces_SetScroll(places, &scroll);
    for (i = 0; i < places->count; i++) {
        TownMapPlace_SetScale(&places->places[i], scale, &sTownMapCenter);
        TownMapPlace_UpdatePos(&places->places[i], &places->scroll);
    }
    TownMapPlayer_UpdatePos(&places->player, &places->scroll);
}

static void TownMapPlaces_SetScroll(TownMapPlaces *places, const TownMapPoint *scroll) {
    u32 i;

    places->scroll.x = MATH_CLAMP(scroll->x, places->limitLeft, places->limitRight);
    places->scroll.y = MATH_CLAMP(scroll->y, places->limitTop, places->limitBottom);
    for (i = 0; i < places->count; i++) {
        TownMapPlace_UpdatePos(&places->places[i], &places->scroll);
    }
    TownMapPlayer_UpdatePos(&places->player, &places->scroll);
}

static void TownMapPlaces_GetScroll(const TownMapPlaces *places, TownMapPoint *scroll) {
    *scroll = places->scroll;
}

static void TownMapPlaces_AddScroll(TownMapPlaces *places, const TownMapPoint *move) {
    places->scroll.x += move->x;
    places->scroll.y += move->y;
    TownMapPlaces_SetScroll(places, &places->scroll);
}

// The place the cursor is on, or NULL
static TownMapPlace *TownMapPlaces_GetHit(const TownMapPlaces *places, const TownMapCursor *cursor) {
    u32 i;
    TownMapPoint pos;

    TownMapCursor_GetPos(cursor, &pos);
    for (i = 0; i < places->count; i++) {
        if (TownMapPlace_IsHit(&places->places[i], &pos, &places->scroll)) {
            return &places->places[i];
        }
    }
    return NULL;
}

// The nearest of the places whose radius the cursor is in, or NULL
static TownMapPlace *TownMapPlaces_GetNearest(const TownMapPlaces *places, const TownMapCursor *cursor) {
    u32 i;
    TownMapPoint pos;
    u32 distance;
    u32 nearest;
    u32 nearestDistance;

    TownMapCursor_GetPos(cursor, &pos);
    nearest = 0xffffffff;
    nearestDistance = 0xffffffff;
    for (i = 0; i < places->count; i++) {
        if (TownMapPlace_GetDistance(&places->places[i], &pos, &places->scroll, &distance)) {
            if (nearestDistance > distance) {
                nearest = i;
                nearestDistance = distance;
            }
        }
    }
    if (nearest == 0xffffffff) {
        return NULL;
    }
    return &places->places[nearest];
}

// A TOWNMAP_PARAM_* of a place: its positions and radii as the zoom scales them, and whether it can be flown to now.
// The parameter is an int here and where the loops pass it on, which TownMapData_GetParam must take without narrowing
static s32 TownMapPlace_GetParam(const TownMapPlace *place, int param) {
    if (param >= TOWNMAP_PARAM_X && param <= TOWNMAP_PARAM_HIT_RADIUS) {
        return place->params[param - TOWNMAP_PARAM_X];
    }
    if (param == TOWNMAP_PARAM_FLY) {
        return place->canFly;
    }
    return TownMapData_GetParam(place->data, place->index, param);
}

static void TownMapPlaces_Select(TownMapPlaces *places, const TownMapPlace *place) {
    u32 i;

    for (i = 0; i < places->count; i++) {
        if (&places->places[i] == place) {
            TownMapPlace_Select(&places->places[i]);
        } else {
            TownMapPlace_Deselect(&places->places[i]);
        }
    }
}

static void TownMapPlaces_Deselect(TownMapPlaces *places) {
    u32 i;

    for (i = 0; i < places->count; i++) {
        TownMapPlace_Deselect(&places->places[i]);
    }
}

// The place the town map shows for a zone, or NULL
static TownMapPlace *TownMapPlaces_GetByZone(const TownMapPlaces *places, u16 zoneId, u16 escapeZoneId) {
    u32 i;
    u16 zone = func_ov012_02160eb4(places->gameData, zoneId);

    for (i = 0; i < places->count; i++) {
        TownMapPlace *place = &places->places[i];

        if (zone == TownMapPlace_GetParam(place, TOWNMAP_PARAM_ZONE)) {
            return place;
        }
    }
    return NULL;
}

// Whether the place's landmarks are listed: it was arrived at and is shown, or it has no flag of arrival
static BOOL TownMapPlace_IsDescribed(const TownMapPlace *place) {
    u16 flag = TownMapData_GetParam(place->data, place->index, TOWNMAP_PARAM_ARRIVAL_FLAG);

    if ((flag != TOWNMAP_NO_FLAG && place->arrived && !place->hidden) || flag == TOWNMAP_NO_FLAG) {
        return TRUE;
    }
    return FALSE;
}

static void TownMapPlace_Init(TownMapPlace *place, void *data, u32 index, ClActUnit *unit, u32 chars, u32 cellAnims,
                              u32 pltt, GameData *gameData, HeapID heapId, BOOL all) {
    int i;
    u16 flag;

    sys_memset(place, 0, sizeof(TownMapPlace));
    place->scale = FX32_ONE;
    place->data = data;
    place->index = index;
    flag = TownMapData_GetParam(data, index, TOWNMAP_PARAM_ARRIVAL_FLAG);
    if (flag != TOWNMAP_NO_FLAG) {
        place->arrived = func_ov012_02160f74(gameData, flag);
    } else {
        place->arrived = FALSE;
    }
    if (all) {
        place->arrived = TRUE;
    }
    for (i = 0; i < TOWNMAP_PLACE_PARAM_COUNT; i++) {
        place->params[i] = TownMapData_GetParam(place->data, place->index, TOWNMAP_PARAM_X + i);
    }
    if (all) {
        place->canFly = TRUE;
    } else {
        place->canFly = place->arrived && TownMapData_GetParam(place->data, place->index, TOWNMAP_PARAM_FLY);
    }
    flag = TownMapData_GetParam(place->data, place->index, TOWNMAP_PARAM_FLAG);
    if (flag != TOWNMAP_NO_FLAG) {
        place->hidden = func_ov012_02160f74(gameData, flag) == FALSE ? TRUE : FALSE;
    } else {
        place->hidden = FALSE;
    }
    if (all) {
        place->hidden = FALSE;
    }
    switch (TownMapPlace_GetParam(place, TOWNMAP_PARAM_TYPE)) {
    case 0:
        place->animSelected = TOWNMAP_ANIM_CITY_SELECTED;
        place->anim = TOWNMAP_ANIM_CITY;
        break;
    case 2:
        place->animSelected = TOWNMAP_ANIM_SELECTED;
        place->anim = TOWNMAP_ANIM_NONE;
        place->arrived = TRUE;
        break;
    case 1:
        place->animSelected = TOWNMAP_ANIM_NONE;
        place->anim = TOWNMAP_ANIM_NONE;
        break;
    case 3:
        place->animSelected = TOWNMAP_ANIM_SELECTED;
        place->anim = 13;
        break;
    case TOWNMAP_PLACE_TYPE_TOWN:
        place->animSelected = 15;
        place->anim = 14;
        break;
    case 5:
        place->animSelected = 18;
        if (place->arrived) {
            place->anim = 17;
        } else {
            place->anim = 16;
        }
        break;
    }
    // A route has no marker
    if (TownMapPlace_GetParam(place, TOWNMAP_PARAM_TYPE) != 1) {
        ClActorSetup setup;

        sys_memset(&setup, 0, sizeof(ClActorSetup));
        setup.x = TownMapPlace_GetParam(place, TOWNMAP_PARAM_X);
        setup.y = TownMapPlace_GetParam(place, TOWNMAP_PARAM_Y);
        place->actor = func_0204c040(unit, chars, pltt, cellAnims, &setup, CLACT_SURFACE_MAIN, heapId);
        func_0204c244(place->actor, 2);
        func_0204c468(place->actor, 2);
        func_0204c438(place->actor, 4);
    }
    TownMapPlace_Deselect(place);
}

static void TownMapPlace_Free(TownMapPlace *place) {
    if (place->actor != NULL) {
        func_0204c108(place->actor);
    }
    sys_memset(place, 0, sizeof(TownMapPlace));
}

static void TownMapPlace_SetVisible(TownMapPlace *place, BOOL visible) {
    if (place->actor != NULL && place->animNow != TOWNMAP_ANIM_NONE) {
        if (place->hidden) {
            func_0204c124(place->actor, FALSE);
        } else if (TownMapPlace_GetParam(place, TOWNMAP_PARAM_TYPE) == TOWNMAP_PLACE_TYPE_TOWN ||
                   TownMapPlace_GetParam(place, TOWNMAP_PARAM_TYPE) == 5) {
            TownMapPlace_SetAnim(place, place->anim);
            func_0204c124(place->actor, TRUE);
        } else {
            func_0204c124(place->actor, visible);
        }
    }
}

// Whether a point is in the place's capsule
static BOOL TownMapPlace_IsHit(const TownMapPlace *place, const TownMapPoint *pos, const TownMapPoint *scroll) {
    CalcSphere sphere;
    CalcCapsule capsule;
    VecFx32 start;
    VecFx32 end;

    start.x = FX32_CONST(pos->x);
    start.y = FX32_CONST(pos->y);
    start.z = 0;
    CalcSphere_Set(&sphere, &start, 0);
    start.x = FX32_CONST(scroll->x + TownMapPlace_GetParam(place, TOWNMAP_PARAM_HIT_START_X));
    start.y = FX32_CONST(scroll->y + TownMapPlace_GetParam(place, TOWNMAP_PARAM_HIT_START_Y));
    start.z = 0;
    end.x = FX32_CONST(scroll->x + TownMapPlace_GetParam(place, TOWNMAP_PARAM_HIT_END_X));
    end.y = FX32_CONST(scroll->y + TownMapPlace_GetParam(place, TOWNMAP_PARAM_HIT_END_Y));
    end.z = 0;
    CalcCapsule_Set(&capsule, &start, &end, FX32_CONST(TownMapPlace_GetParam(place, TOWNMAP_PARAM_HIT_RADIUS)));
    if (place->hidden) {
        return FALSE;
    }
    return CalcCapsule_HitSphere(&capsule, &sphere, NULL);
}

// Whether a point is in the place's radius, and the square of how far from the place it is
static BOOL TownMapPlace_GetDistance(const TownMapPlace *place, const TownMapPoint *pos, const TownMapPoint *scroll,
                                     u32 *distance) {
    s32 x = scroll->x + TownMapPlace_GetParam(place, TOWNMAP_PARAM_CURSOR_X);
    s32 y = scroll->y + TownMapPlace_GetParam(place, TOWNMAP_PARAM_CURSOR_Y);
    u32 radius = TownMapPlace_GetParam(place, TOWNMAP_PARAM_RADIUS);
    u32 squareX = (x - pos->x) * (x - pos->x);
    u32 squareY = (y - pos->y) * (y - pos->y);

    if (place->hidden) {
        return FALSE;
    }
    if (squareX + squareY < (radius * radius * MATH_ABS(place->scale)) >> FX32_SHIFT) {
        if (distance != NULL) {
            *distance = squareX + squareY;
        }
        return TRUE;
    }
    return FALSE;
}

static void TownMapPlace_SetScale(TownMapPlace *place, fx32 scale, const TownMapPoint *center) {
    int i;

    for (i = 0; i < TOWNMAP_PLACE_PARAM_COUNT - 1; i++) {
        s32 origin;
        s32 value;

        if (i % 2 == 0) {
            origin = center->x;
        } else {
            origin = center->y;
        }
        value = TownMapData_GetParam(place->data, place->index, TOWNMAP_PARAM_X + i);
        value -= origin;
        value *= scale;
        place->params[i] = (value >> FX32_SHIFT) + origin;
    }
    place->params[TOWNMAP_PARAM_HIT_RADIUS - TOWNMAP_PARAM_X] =
        (TownMapData_GetParam(place->data, place->index, TOWNMAP_PARAM_HIT_RADIUS) * scale) >> FX32_SHIFT;
    place->scale = scale;
    if (place->actor != NULL) {
        ClActorScale actorScale;

        actorScale.x = scale;
        actorScale.y = scale;
        func_0204c270(place->actor, &actorScale);
    }
}

static void TownMapPlace_UpdatePos(TownMapPlace *place, const TownMapPoint *scroll) {
    ClActorPos pos;

    pos.x = scroll->x + TownMapPlace_GetParam(place, TOWNMAP_PARAM_X);
    pos.y = scroll->y + TownMapPlace_GetParam(place, TOWNMAP_PARAM_Y);
    if (place->actor != NULL) {
        func_0204c140(place->actor, &pos, CLACT_SURFACE_MAIN);
    }
}

static void TownMapPlace_SetAnim(TownMapPlace *place, u8 anim) {
    place->animNow = anim;
    if (place->animNow != TOWNMAP_ANIM_NONE) {
        if (place->actor != NULL) {
            func_0204c124(place->actor, TRUE);
            func_0204c488(place->actor, place->animNow);
        }
    } else {
        if (place->actor != NULL) {
            func_0204c124(place->actor, FALSE);
        }
    }
}

static void TownMapPlace_Select(TownMapPlace *place) {
    BOOL shown = FALSE;

    if (TownMapPlace_GetParam(place, TOWNMAP_PARAM_TYPE) == TOWNMAP_PLACE_TYPE_TOWN ||
        TownMapPlace_GetParam(place, TOWNMAP_PARAM_TYPE) == 5) {
        shown = TRUE;
        if (place->hidden) {
            shown = FALSE;
        }
    }
    if (place->arrived || shown) {
        TownMapPlace_SetVisible(place, TRUE);
        TownMapPlace_SetAnim(place, place->animSelected);
    } else {
        TownMapPlace_SetVisible(place, FALSE);
    }
}

static void TownMapPlace_Deselect(TownMapPlace *place) {
    BOOL shown = FALSE;

    if (TownMapPlace_GetParam(place, TOWNMAP_PARAM_TYPE) == TOWNMAP_PLACE_TYPE_TOWN ||
        TownMapPlace_GetParam(place, TOWNMAP_PARAM_TYPE) == 5) {
        shown = TRUE;
        if (place->hidden) {
            shown = FALSE;
        }
    }
    if (place->arrived || shown) {
        if (place->actor != NULL) {
            TownMapPlace_SetVisible(place, TRUE);
            TownMapPlace_SetAnim(place, place->anim);
        }
    } else {
        TownMapPlace_SetVisible(place, FALSE);
    }
}

// The marker of where the player is, hidden when that is no place of the map
static void TownMapPlayer_Init(TownMapPlayer *player, TownMapPlace *place, ClActUnit *unit, u32 chars, u32 cellAnims,
                               u32 pltt, HeapID heapId) {
    ClActorSetup setup;

    sys_memset(player, 0, sizeof(TownMapPlayer));
    player->place = place;
    sys_memset(&setup, 0, sizeof(ClActorSetup));
    if (place != NULL) {
        setup.x = TownMapPlace_GetParam(place, TOWNMAP_PARAM_X);
        setup.y = TownMapPlace_GetParam(place, TOWNMAP_PARAM_Y);
    }
    player->actor = func_0204c040(unit, chars, pltt, cellAnims, &setup, CLACT_SURFACE_MAIN, heapId);
    func_0204c468(player->actor, 2);
    func_0204c438(player->actor, 2);
    func_0204c488(player->actor, TOWNMAP_ANIM_PLAYER);
    func_0204c520(player->actor, TRUE);
    if (place == NULL) {
        func_0204c124(player->actor, FALSE);
    }
}

static void TownMapPlayer_Free(TownMapPlayer *player) {
    func_0204c108(player->actor);
    sys_memset(player, 0, sizeof(TownMapPlayer));
}

static void TownMapPlayer_UpdatePos(TownMapPlayer *player, const TownMapPoint *scroll) {
    ClActorPos pos;

    if (player->place != NULL) {
        pos.x = scroll->x + TownMapPlace_GetParam(player->place, TOWNMAP_PARAM_X);
        pos.y = scroll->y + TownMapPlace_GetParam(player->place, TOWNMAP_PARAM_Y);
    }
    func_0204c140(player->actor, &pos, CLACT_SURFACE_MAIN);
}

static void TownMapMap_Init(TownMapMap *map, u8 bgBack, u8 bgFront, HeapID heapId) {
    sys_memset(map, 0, sizeof(TownMapMap));
    map->bgBack = bgBack;
    map->bgFront = bgFront;
    map->scale = FX32_ONE;
    map->limitTop = 0;
    map->limitLeft = 0;
    map->limitRight = 0;
    map->limitBottom = 0;
}

static void TownMapMap_Free(TownMapMap *map) {
    sys_memset(map, 0, sizeof(TownMapMap));
}

static inline void TownMapMap_SetLimits(TownMapMap *map, s16 top, s16 left, s16 right, s16 bottom) {
    map->limitTop = top;
    map->limitLeft = left;
    map->limitRight = right;
    map->limitBottom = bottom;
}

// Moves the zoom on, and the places with it
static void TownMapMap_Main(TownMapMap *map, TownMapPlaces *places) {
    map->finished = FALSE;
    if (map->moving) {
        fx32 step = (map->scaleEnd - map->scaleStart) / TOWNMAP_ZOOM_FRAMES;
        fx32 moveX;
        fx32 moveY;

        map->scale = map->scaleStart + step * map->frame;
        moveY = (map->end.y - map->start.y) << FX32_SHIFT;
        moveX = (map->end.x - map->start.x) << FX32_SHIFT;
        map->pos.x = map->start.x + ((moveX * map->frame / TOWNMAP_ZOOM_FRAMES) >> FX32_SHIFT);
        map->pos.y = map->start.y + ((moveY * map->frame / TOWNMAP_ZOOM_FRAMES) >> FX32_SHIFT);
        if (map->scaleEnd == FX32_ONE) {
            map->limitTop = TOWNMAP_SCROLL_TOP - TOWNMAP_SCROLL_TOP / TOWNMAP_ZOOM_FRAMES * map->frame;
            map->limitBottom = TOWNMAP_SCROLL_BOTTOM - TOWNMAP_SCROLL_BOTTOM / TOWNMAP_ZOOM_FRAMES * map->frame;
            map->limitLeft = TOWNMAP_SCROLL_LEFT - TOWNMAP_SCROLL_LEFT / TOWNMAP_ZOOM_FRAMES * map->frame;
            map->limitRight = TOWNMAP_SCROLL_RIGHT - TOWNMAP_SCROLL_RIGHT / TOWNMAP_ZOOM_FRAMES * map->frame;
        } else {
            map->limitTop = TOWNMAP_SCROLL_TOP / TOWNMAP_ZOOM_FRAMES * map->frame;
            map->limitBottom = TOWNMAP_SCROLL_BOTTOM / TOWNMAP_ZOOM_FRAMES * map->frame;
            map->limitLeft = TOWNMAP_SCROLL_LEFT / TOWNMAP_ZOOM_FRAMES * map->frame;
            map->limitRight = TOWNMAP_SCROLL_RIGHT / TOWNMAP_ZOOM_FRAMES * map->frame;
        }
        if (map->frame++ >= TOWNMAP_ZOOM_FRAMES) {
            map->frame = 0;
            map->moving = FALSE;
            map->scale = map->scaleEnd;
            map->pos = map->end;
            if (map->scale == FX32_ONE) {
                TownMapMap_SetLimits(map, 0, 0, 0, 0);
            } else {
                TownMapMap_SetLimits(map, TOWNMAP_SCROLL_TOP, TOWNMAP_SCROLL_LEFT, TOWNMAP_SCROLL_RIGHT, TOWNMAP_SCROLL_BOTTOM);
            }
            map->finished = TRUE;
        }
        TownMapPlaces_SetScale(places, map->scale, &map->pos);
        TownMapMap_SetPos(map, &map->pos);
    }
}

// Starts a zoom to a scale, between two points of the screen that the map is centered on
static void TownMapMap_StartZoom(TownMapMap *map, fx32 scale, const TownMapPoint *start, const TownMapPoint *end) {
    map->moving = TRUE;
    map->scaleEnd = scale;
    map->scaleStart = map->scale;
    map->start.x = start->x - TOWNMAP_CENTER_X;
    map->start.y = start->y - TOWNMAP_CENTER_Y;
    map->end.x = end->x - TOWNMAP_CENTER_X;
    map->end.y = end->y - TOWNMAP_CENTER_Y;
}

static BOOL TownMapMap_IsFinished(const TownMapMap *map) {
    return map->finished;
}

static BOOL TownMapMap_IsMoving(const TownMapMap *map) {
    return map->moving;
}

static void TownMapMap_SetPos(TownMapMap *map, const TownMapPoint *pos) {
    map->pos.x = MATH_CLAMP(pos->x, map->limitLeft, map->limitRight);
    map->pos.y = MATH_CLAMP(pos->y, map->limitTop, map->limitBottom);
    GFL_BGSysMoveBGReq(map->bgBack, BG_MOVE_SET_X, map->pos.x);
    GFL_BGSysMoveBGReq(map->bgBack, BG_MOVE_SET_Y, map->pos.y);
    GFL_BGSysAdjustBGOriginReq(map->bgBack, BG_CENTER_SET_X, TOWNMAP_CENTER_X);
    GFL_BGSysAdjustBGOriginReq(map->bgBack, BG_CENTER_SET_Y, TOWNMAP_CENTER_Y);
    GFL_BGSysScaleBGReq(map->bgBack, BG_SCALE_SET_X, map->scale);
    GFL_BGSysScaleBGReq(map->bgBack, BG_SCALE_SET_Y, map->scale);
    GFL_BGSysMoveBGReq(map->bgFront, BG_MOVE_SET_X, map->pos.x);
    GFL_BGSysMoveBGReq(map->bgFront, BG_MOVE_SET_Y, map->pos.y);
    GFL_BGSysAdjustBGOriginReq(map->bgFront, BG_CENTER_SET_X, TOWNMAP_CENTER_X);
    GFL_BGSysAdjustBGOriginReq(map->bgFront, BG_CENTER_SET_Y, TOWNMAP_CENTER_Y);
    GFL_BGSysScaleBGReq(map->bgFront, BG_SCALE_SET_X, map->scale);
    GFL_BGSysScaleBGReq(map->bgFront, BG_SCALE_SET_Y, map->scale);
}

static void TownMapMap_GetPos(const TownMapMap *map, TownMapPoint *pos) {
    *pos = map->pos;
}

static void TownMapMap_AddPos(TownMapMap *map, const TownMapPoint *move) {
    map->pos.x += move->x;
    map->pos.y += move->y;
    TownMapMap_SetPos(map, &map->pos);
}

static void TownMapInfo_Init(TownMapInfo *info, u8 bg, Font *font, MsgData *placeNames, MsgData *msgData,
                             GameData *gameData, HeapID heapId) {
    sys_memset(info, 0, sizeof(TownMapInfo));
    info->font = font;
    info->msgData = msgData;
    info->placeNames = placeNames;
    info->gameData = gameData;
    info->wordSet = GFL_WordSetSystemCreateDefault(heapId);
    info->window = BmpWin_CreateDynamic(bg, 0, 0, 32, 24, 14, 0);
    info->strbuf = GFL_StrBufCreate(255, heapId);
}

static void TownMapInfo_Free(TownMapInfo *info) {
    GFL_StrBufFree(info->strbuf);
    BmpWin_Free(info->window);
    GFL_WordSetSystemFree(info->wordSet);
    sys_memset(info, 0, sizeof(TownMapInfo));
}

// Draws a place's name, description and landmarks on the upper screen; for Join Avenue, its name, rank and numbers
static void TownMapInfo_Draw(TownMapInfo *info, TownMapPlace *place) {
    // Join Avenue's two counts of shoppers served
    u32 record80;
    u32 record7F;
    GFLBitmap *bitmap = BmpWin_GetBitmap(info->window);
    SaveControl *save = GameData_GetSaveControl(info->gameData);
    JoinAvenueSave *joinAvenue = SaveControl_GetJoinAvenue(save);
    JoinAvenueInfo *avenueInfo = JoinAvenue_GetInfo(joinAvenue);
    JoinAvenuePersonList *persons;
    JoinAvenueEntryList *entries;
    JoinAvenuePersonList **visitors;
    u16 zoneId;
    u32 rank;
    u32 people;
    BOOL landmarks;

    getAddressOfBeginningOfOccupants(joinAvenue);
    persons = JoinAvenue_GetPersonList(joinAvenue);
    entries = func_02010054(joinAvenue);
    visitors = GameData_GetJoinAvenuePersonListPtr(info->gameData);
    GFL_BitmapFill(bitmap, 0);
    if (place != NULL) {
        u16 messageId;

        landmarks = TRUE;
        zoneId = TownMapPlace_GetParam(place, TOWNMAP_PARAM_ZONE);
        if (IsZoneJoinAvenue(zoneId)) {
            GFL_StrBufLoadString(info->strbuf, (const u16 *)JoinAvenue_GetParam(avenueInfo, 0, NULL));
            if (JoinAvenue_GetParam(avenueInfo, 7, NULL)) {
                landmarks = FALSE;
            }
        } else {
            GFL_MsgDataLoadStrbuf(info->placeNames, ZoneData_GetPlaceNameID(zoneId), info->strbuf);
        }
        {
            s16 center = BmpWin_GetSizeX(info->window) * 4;
            s16 half = GFL_FontGetBlockWidth(info->strbuf, info->font, 0) / 2;

            center -= half;
            GFL_TextRendererDrawToBitmap(bitmap, center, 48, info->strbuf, info->font);
        }
        messageId = TownMapPlace_GetParam(place, TOWNMAP_PARAM_DESCRIPTION);
        if (messageId != 0xffff) {
            GFL_MsgDataLoadStrbuf(info->msgData, messageId, info->strbuf);
            GFL_TextRendererDrawToBitmap(bitmap, 28, 80, info->strbuf, info->font);
        }
        if (TownMapPlace_IsDescribed(place) && landmarks) {
            messageId = TownMapPlace_GetParam(place, TOWNMAP_PARAM_LANDMARK_FIRST);
            if (messageId != 0xffff) {
                GFL_MsgDataLoadStrbuf(info->msgData, messageId, info->strbuf);
                GFL_TextRendererDrawToBitmap(bitmap, 16, 128, info->strbuf, info->font);
            }
            messageId = TownMapPlace_GetParam(place, TOWNMAP_PARAM_LANDMARK_FIRST + 1);
            if (messageId != 0xffff) {
                GFL_MsgDataLoadStrbuf(info->msgData, messageId, info->strbuf);
                GFL_TextRendererDrawToBitmap(bitmap, 16, 144, info->strbuf, info->font);
            }
            messageId = TownMapPlace_GetParam(place, TOWNMAP_PARAM_LANDMARK_FIRST + 2);
            if (messageId != 0xffff) {
                GFL_MsgDataLoadStrbuf(info->msgData, messageId, info->strbuf);
                GFL_TextRendererDrawToBitmap(bitmap, 16, 160, info->strbuf, info->font);
            }
            if (IsZoneJoinAvenue(zoneId)) {
                // The right column's three messages take the avenue's rank, the shoppers served and its people
                GameRecords *records = getTrainerCardInfoBlkAddress(save);
                StrBuf *format;

                rank = JoinAvenue_GetParam(avenueInfo, 2, NULL);
                record7F = RecordGet(records, 0x7f);
                record80 = RecordGet(records, 0x80);
                people = func_02038030(persons) + func_02037ed8(entries);
                format = GFL_StrBufCreate(128, HEAPID_TOWNMAP);

                if (*visitors != NULL) {
                    people += func_02038030(*visitors);
                }
                messageId = TownMapPlace_GetParam(place, TOWNMAP_PARAM_LANDMARK_FIRST + 3);
                if (messageId != 0xffff) {
                    u32 x;

                    GFL_MsgDataLoadStrbuf(info->msgData, messageId, format);
                    WordSetNumber(info->wordSet, 0, rank, 3, 1, TRUE);
                    GFL_WordSetFormatStrbuf(info->wordSet, info->strbuf, format);
                    x = GFL_BitmapGetWidth(bitmap) - GFL_FontGetBlockWidth(info->strbuf, info->font, 0) - 30;
                    GFL_TextRendererDrawToBitmap(bitmap, x, 128, info->strbuf, info->font);
                }
                messageId = TownMapPlace_GetParam(place, TOWNMAP_PARAM_LANDMARK_FIRST + 4);
                if (messageId != 0xffff) {
                    u32 x;

                    GFL_MsgDataLoadStrbuf(info->msgData, messageId, format);
                    WordSetNumber(info->wordSet, 0, record7F + record80, 7, 1, TRUE);
                    GFL_WordSetFormatStrbuf(info->wordSet, info->strbuf, format);
                    x = GFL_BitmapGetWidth(bitmap) - GFL_FontGetBlockWidth(info->strbuf, info->font, 0) - 30;
                    GFL_TextRendererDrawToBitmap(bitmap, x, 144, info->strbuf, info->font);
                }
                messageId = TownMapPlace_GetParam(place, TOWNMAP_PARAM_LANDMARK_LAST);
                if (messageId != 0xffff) {
                    u32 x;

                    GFL_MsgDataLoadStrbuf(info->msgData, messageId, format);
                    WordSetNumber(info->wordSet, 0, people, 2, 1, TRUE);
                    GFL_WordSetFormatStrbuf(info->wordSet, info->strbuf, format);
                    x = GFL_BitmapGetWidth(bitmap) - GFL_FontGetBlockWidth(info->strbuf, info->font, 0) - 30;
                    GFL_TextRendererDrawToBitmap(bitmap, x, 160, info->strbuf, info->font);
                }
                GFL_StrBufFree(format);
            } else {
                messageId = TownMapPlace_GetParam(place, TOWNMAP_PARAM_LANDMARK_FIRST + 3);
                if (messageId != 0xffff) {
                    GFL_MsgDataLoadStrbuf(info->msgData, messageId, info->strbuf);
                    GFL_TextRendererDrawToBitmap(bitmap, 136, 128, info->strbuf, info->font);
                }
                messageId = TownMapPlace_GetParam(place, TOWNMAP_PARAM_LANDMARK_FIRST + 4);
                if (messageId != 0xffff) {
                    GFL_MsgDataLoadStrbuf(info->msgData, messageId, info->strbuf);
                    GFL_TextRendererDrawToBitmap(bitmap, 136, 144, info->strbuf, info->font);
                }
                messageId = TownMapPlace_GetParam(place, TOWNMAP_PARAM_LANDMARK_LAST);
                if (messageId != 0xffff) {
                    GFL_MsgDataLoadStrbuf(info->msgData, messageId, info->strbuf);
                    GFL_TextRendererDrawToBitmap(bitmap, 136, 160, info->strbuf, info->font);
                }
            }
        }
    }
    BmpWin_Transfer(info->window);
}

static void TownMapInfo_Clear(TownMapInfo *info) {
    BmpWin *window = info->window;

    BmpWin_ClearScreen(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
}

static void TownMapMsgWin_Init(TownMapMsgWin *msgWin, u8 bg, Font *font, MsgData *msgData, u8 x, u8 y, u8 width,
                               u8 height, HeapID heapId) {
    sys_memset(msgWin, 0, sizeof(TownMapMsgWin));
    msgWin->font = font;
    msgWin->msgData = msgData;
    msgWin->window = BmpWin_CreateDynamic(bg, x, y, width, height, 14, 0);
    msgWin->strbuf = GFL_StrBufCreate(255, heapId);
}

static void TownMapMsgWin_Free(TownMapMsgWin *msgWin) {
    GFL_StrBufFree(msgWin->strbuf);
    BmpWin_Free(msgWin->window);
    sys_memset(msgWin, 0, sizeof(TownMapMsgWin));
}

static void TownMapMsgWin_Print(TownMapMsgWin *msgWin, u32 messageId, u16 x, u16 y) {
    GFL_BitmapFill(BmpWin_GetBitmap(msgWin->window), 0);
    GFL_MsgDataLoadStrbuf(msgWin->msgData, messageId, msgWin->strbuf);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(msgWin->window), x, y, msgWin->strbuf, msgWin->font);
    BmpWin_Transfer(msgWin->window);
}

static void TownMapMsgWin_PrintStrBuf(TownMapMsgWin *msgWin, const StrBuf *strbuf, u16 x, u16 y) {
    GFL_BitmapFill(BmpWin_GetBitmap(msgWin->window), 0);
    GFL_StrBufCopy(msgWin->strbuf, strbuf);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(msgWin->window), x, y, msgWin->strbuf, msgWin->font);
    BmpWin_Transfer(msgWin->window);
}

static void TownMapMsgWin_Clear(TownMapMsgWin *msgWin) {
    BmpWin *window = msgWin->window;

    BmpWin_ClearScreen(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
}

// The name of the selected place, on a plate in the middle of the lower screen
static void TownMapPlaceName_Init(TownMapPlaceName *placeName, u8 bg, Font *font, MsgData *placeNames, ClActor *plate,
                                  GameData *gameData, HeapID heapId) {
    ClActorPos pos;

    sys_memset(placeName, 0, sizeof(TownMapPlaceName));
    placeName->plate = plate;
    placeName->gameData = gameData;
    TownMapMsgWin_Init(&placeName->msgWin, bg, font, placeNames, 0, 0, 24, 3, heapId);
    pos.x = TOWNMAP_CENTER_X;
    pos.y = TOWNMAP_CENTER_Y;
    func_0204c140(placeName->plate, &pos, CLACT_SURFACE_MAIN);
    func_0204c468(placeName->plate, 1);
    func_0204c438(placeName->plate, 1);
    func_0204c124(placeName->plate, TRUE);
    func_0204c488(placeName->plate, TOWNMAP_ANIM_NAME_PLATE);
    func_0204c520(placeName->plate, TRUE);
    func_0204c550(placeName->plate);
}

static void TownMapPlaceName_Free(TownMapPlaceName *placeName) {
    TownMapMsgWin_Free(&placeName->msgWin);
    sys_memset(placeName, 0, sizeof(TownMapPlaceName));
}

static void TownMapPlaceName_Main(TownMapPlaceName *placeName) {
    if (placeName->requested) {
        placeName->requested = FALSE;
        func_0204c488(placeName->plate, TOWNMAP_ANIM_NAME_PLATE);
        func_0204c540(placeName->plate);
        func_0204c124(placeName->plate, TRUE);
        TownMapMsgWin_Clear(&placeName->msgWin);
        placeName->opening = TRUE;
    }
    if (placeName->opening && !func_0204c560(placeName->plate)) {
        u16 zoneId = TownMapPlace_GetParam(placeName->place, TOWNMAP_PARAM_ZONE);

        if (IsZoneJoinAvenue(zoneId)) {
            JoinAvenueInfo *avenueInfo =
                JoinAvenue_GetInfo(SaveControl_GetJoinAvenue(GameData_GetSaveControl(placeName->gameData)));
            StrBuf *name = GFL_StrBufCreate(128, HEAPID_TOWNMAP);

            GFL_StrBufLoadString(name, (const u16 *)JoinAvenue_GetParam(avenueInfo, 0, NULL));
            TownMapMsgWin_PrintStrBuf(&placeName->msgWin, name, 16, 2);
            GFL_StrBufFree(name);
        } else {
            TownMapMsgWin_Print(&placeName->msgWin, ZoneData_GetPlaceNameID(zoneId), 16, 2);
        }
        func_0204c550(placeName->plate);
        placeName->opening = FALSE;
    }
}

static void TownMapPlaceName_SetPlace(TownMapPlaceName *placeName, TownMapPlace *place) {
    placeName->requested = TRUE;
    placeName->place = place;
}

static void TownMapPlaceName_SetVisible(TownMapPlaceName *placeName, BOOL visible) {
    func_0204c124(placeName->plate, visible);
    if (!visible) {
        placeName->requested = FALSE;
        placeName->opening = FALSE;
        TownMapMsgWin_Clear(&placeName->msgWin);
    }
}

static void TownMapTouch_Init(TownMapTouch *touch, HeapID heapId) {
    sys_memset(touch, 0, sizeof(TownMapTouch));
}

static void TownMapTouch_Free(TownMapTouch *touch) {
    sys_memset(touch, 0, sizeof(TownMapTouch));
}

// Follows a touch on the map: how far it moved since it started, or since the last drag was taken
static void TownMapTouch_Main(TownMapTouch *touch) {
    u32 x;
    u32 y;

    touch->dragging = FALSE;
    if (func_0203dac8(&x, &y)) {
        if (y <= TOWNMAP_CURSOR_BOTTOM) {
            touch->start.x = x << FX32_SHIFT;
            touch->start.y = y << FX32_SHIFT;
            touch->start.z = 0;
            touch->now.x = x << FX32_SHIFT;
            touch->now.y = y << FX32_SHIFT;
            touch->now.z = 0;
        }
    } else if (func_0203da84(&x, &y)) {
        if (y <= TOWNMAP_CURSOR_BOTTOM) {
            if (touch->restart) {
                touch->start = touch->now;
                touch->restart = FALSE;
            }
            touch->now.x = x << FX32_SHIFT;
            touch->now.y = y << FX32_SHIFT;
            touch->now.z = 0;
            VEC_Subtract(&touch->now, &touch->start, &touch->delta);
            if (VEC_Mag(&touch->delta) >= FX32_ONE) {
                touch->dragging = TRUE;
            }
        } else {
            touch->start.x = x << FX32_SHIFT;
            touch->start.y = y << FX32_SHIFT;
            touch->start.z = 0;
            touch->now.x = x << FX32_SHIFT;
            touch->now.y = y << FX32_SHIFT;
            touch->now.z = 0;
        }
    }
}

static BOOL TownMapTouch_GetDrag(const TownMapTouch *touch, VecFx32 *delta, u32 *unk18) {
    if (touch->dragging) {
        if (delta != NULL) {
            *delta = touch->delta;
        }
        if (unk18 != NULL) {
            *unk18 = touch->unk18;
        }
        return TRUE;
    }
    return FALSE;
}

static void TownMapTouch_Restart(TownMapTouch *touch) {
    touch->restart = TRUE;
}

static void TownMap_VBlank(TCB *tcb, void *data) {
}
