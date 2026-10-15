#include "types.h"
#include "constants/arc.h"
#include "constants/flags.h"
#include "constants/sound.h"
#include "constants/zones.h"
#include "field/dowsing.h"
#include "field/field.h"
#include "field/field_map.h"
#include "field/hidden_item.h"
#include "field/player_state.h"
#include "field/subscreen.h"
#include "field/zone.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/hw.h"
#include "save/event_work.h"
#include "system/app_menu_common.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/infowin.h"
#include "system/palanm.h"

// The Dowsing MCHN's touch screen, named after the ROM's "dowsing.c". Every frame it looks for the closest hidden item
// in the area around the player, picks a signal from the map of the archive (how near it is), and shows it with the
// sound that goes with it. The names are ours

// The event flag that says the Hidden Grotto's entrance is waiting to be found
#define EVENT_FLAG_HIDDEN_GROTTO_FOUND EVENT_FLAG_0x097f

// The zone of no zone, for a position outside the map
#define ZONE_NONE 0xffff

// What the search found: nothing or a hidden item
#define HIT_NONE 0
#define HIT_HIDDEN_ITEM 1

// The area that is searched around the player, in tiles: the signal map is 32 wide, the player at its (15, 11)
#define MAP_WIDTH 32
#define MAP_PLAYER_X 15
#define MAP_PLAYER_Z 11
#define SEARCH_LEFT 15
#define SEARCH_RIGHT 17
#define SEARCH_TOP 11
#define SEARCH_BOTTOM 13

// The signals, which are the values of the signal map. 8 is the closest, 9 is none
#define SIGNAL_CLOSEST 8
#define SIGNAL_NONE 9

// The sub screen's BGs and the touch bar's
#define BG_SIGNAL 7
#define BG_INFOWIN 4

// The icons
#define ICON_COUNT 4
#define ICON_RING 0
#define ICON_DOT 1
#define ICON_DIRECTION 2
#define ICON_CLOSEST 3

// The fade commands
#define FADE_CMD_NONE 0
#define FADE_CMD_IN 1
#define FADE_CMD_OUT 2

// The states of the exit button
#define BUTTON_WAIT 0
#define BUTTON_PRESSED 1
#define BUTTON_RELEASED 2
#define BUTTON_DONE 3

// Where the exit button is on the touch screen
#define BUTTON_X 0xe8
#define BUTTON_Y 0xa8
#define BUTTON_WIDTH 0x18
#define BUTTON_HEIGHT 0x18

// The hidden items are found in order, each time asking whether the next is still there to find
typedef struct {
    EventWork *eventWork;
    u16 index;
} DowsingSearch;

// A hidden item that the search found
typedef struct {
    u32 type;
    const HiddenItem *item;
} DowsingHit;

typedef struct {
    u32 chars;
    u32 cellAnims;
    ClActor *actor;
} DowsingIcon;

// What a signal shows: the animation of each of the first three icons, where the direction icon is, and the pan of the
// sound, where 8 is the center
typedef struct {
    u32 anims0 : 3;
    u32 anims1 : 3;
    u32 anims2 : 3;
    u32 x : 8;
    u32 y : 8;
    u32 pan : 7;
} DowsingSignal;

struct DowsingWork {
    u16 heapId;
    u16 subscreenHeapId;
    FieldSubscreen *subscreen;
    Field *field;
    ClActUnit *unit;
    // The characters of the signal BG
    u32 signalChars;
    // The characters of the touch bar's BG
    u32 barChars;
    u32 buttonPalette;
    u32 buttonChars;
    u32 buttonCellAnims;
    ClActor *button;
    u32 buttonState;
    u32 iconPalette;
    DowsingIcon icons[ICON_COUNT];
    // The signal of each tile around the player, MAP_WIDTH wide
    u8 *signalMap;
    // The signal before this frame's and this frame's
    s32 prevSignal;
    s32 signal;
    // The signal the icons show
    s32 shownSignal;
    // The hit that this frame's signal comes from and the one of the frame before
    DowsingHit prevHit;
    DowsingHit hit;
    // Where the player was when the signal was last found
    s32 lastX;
    s32 lastY;
    s32 lastZ;
    u32 unk8C;
    s32 soundPlayer;
    BOOL soundOn;
    u16 soundTimer;
    BOOL restartIcon;
    TCB *vblankTCB;
    u8 frame;
    BOOL active;
    TCBManager *tcbManager;
    void *tcbMemory;
    PaletteFade *paletteFade;
    u32 fadeCmd;
    u32 fadeState;
    // Whether the search starts again, and whether it is the first since the screen opened
    BOOL searchRestart;
    BOOL searchFirst;
    DowsingSearch *search;
};

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} DowsingBGSetup;

typedef struct {
    u8 file;
    u8 cellFile;
    u8 animFile;
} DowsingIconRes;

static DowsingSearch *DowsingSearch_Create(HeapID heapId, EventWork *eventWork);
static void DowsingSearch_Free(DowsingSearch *search);
static BOOL DowsingSearch_IsHere(DowsingSearch *search, const DowsingHit *hit);
static void DowsingSearch_Reset(DowsingSearch *search);
static BOOL DowsingSearch_Next(DowsingSearch *search, DowsingHit *hit, u32 mode, u32 zoneId);
static void Dowsing_VBlank(TCB *tcb, void *data);
static void Dowsing_InitBG(DowsingWork *work);
static void Dowsing_ExitBG(DowsingWork *work);
static void Dowsing_LoadSignalBG(DowsingWork *work);
static void Dowsing_FreeSignalBG(DowsingWork *work);
static void Dowsing_InitIcons(DowsingWork *work);
static void Dowsing_FreeIcons(DowsingWork *work);
static void Dowsing_InitButton(DowsingWork *work);
static void Dowsing_ExitButton(DowsingWork *work);
static void Dowsing_UpdateButton(DowsingWork *work);
static void Dowsing_InitFade(DowsingWork *work);
static void Dowsing_FreeFade(DowsingWork *work);
static void Dowsing_UpdateFade(DowsingWork *work);

// The Hidden Grotto's entrance, as if it were a hidden item of the grotto's own zone
static const HiddenItem sHiddenGrottoItem = { 0, 0, 0, ZONE_HIDDEN_GROTTO, 15, 12 };

static const DowsingIconRes sIconRes[ICON_COUNT] = {
    { 3, 4, 8 },
    { 3, 4, 8 },
    { 11, 5, 9 },
    { 12, 6, 10 },
};

static const DowsingSignal sSignals[10] = {
    { 0, 0, 0, 128, 88, 8 },
    { 1, 1, 1, 128, 104, 8 },
    { 3, 3, 3, 128, 64, 0 },
    { 2, 2, 2, 128, 64, 16 },
    { 5, 5, 5, 128, 88, 4 },
    { 7, 7, 7, 128, 104, 4 },
    { 4, 4, 4, 128, 88, 12 },
    { 6, 6, 6, 128, 104, 12 },
    { 4, 5, 0, 128, 152, 0 },
    { 5, 4, 0, 128, 88, 0 },
};

static const DowsingBGSetup sDowsingBGSetups[] = {
    { BG_INFOWIN,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7000), GX_BG_CHARBASE(0x04000), 0x2000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
    { BG_SIGNAL,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x6000), GX_BG_CHARBASE(0x00000), 0x4000,
        GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

static DowsingSearch *DowsingSearch_Create(HeapID heapId, EventWork *eventWork) {
    DowsingSearch *search = GFL_HeapAllocate(heapId, sizeof(DowsingSearch), TRUE, "dowsing.c", 123);
    search->eventWork = eventWork;
    search->index = 0;
    return search;
}

static void DowsingSearch_Free(DowsingSearch *search) {
    GFL_HeapFree(search);
}

// Whether a hit is still there to find: a hidden item that has not been picked up, or the grotto's entrance once it is
// open
static BOOL DowsingSearch_IsHere(DowsingSearch *search, const DowsingHit *hit) {
    if (hit->type == HIT_HIDDEN_ITEM) {
        if (!GetZoneIsHiddenGroto(hit->item->zoneId)) {
            if (EventWork_FlagGet(search->eventWork, hit->item->index + HIDDEN_ITEM_FLAG_START)) {
                return FALSE;
            }
            return TRUE;
        }
        return EventWork_FlagGet(search->eventWork, EVENT_FLAG_HIDDEN_GROTTO_FOUND);
    }
    return FALSE;
}

static void DowsingSearch_Reset(DowsingSearch *search) {
    search->index = 0;
}

// Finds the next hit of a mode, 0 or 1, in a zone, and returns whether there was one
static BOOL DowsingSearch_Next(DowsingSearch *search, DowsingHit *hit, u32 mode, u32 zoneId) {
    u16 count;
    BOOL found = FALSE;
    DowsingHit candidate;
    const HiddenItem *items = func_ov012_0215f2c4(&count);

    if (mode <= 1) {
        if (!GetZoneIsHiddenGroto(zoneId)) {
            while (search->index < count) {
                candidate.type = HIT_HIDDEN_ITEM;
                candidate.item = &items[search->index];
                if (DowsingSearch_IsHere(search, &candidate)) {
                    found = TRUE;
                    hit->type = candidate.type;
                    hit->item = candidate.item;
                }
                search->index++;
                if (found) {
                    return TRUE;
                }
            }
        } else {
            if (search->index < 1) {
                BOOL open = EventWork_FlagGet(search->eventWork, EVENT_FLAG_HIDDEN_GROTTO_FOUND);
                search->index++;
                if (open == TRUE) {
                    hit->type = HIT_HIDDEN_ITEM;
                    hit->item = &sHiddenGrottoItem;
                    return TRUE;
                }
            }
            hit->type = HIT_NONE;
            return FALSE;
        }
    }
    hit->type = HIT_NONE;
    return FALSE;
}

DowsingWork *Dowsing_Create(HeapID heapId, HeapID subscreenHeapId, FieldSubscreen *subscreen, Field *field) {
    GameData *gameData;
    EventWork *eventWork;
    u32 size;
    DowsingWork *work = GFL_HeapAllocate(heapId, sizeof(DowsingWork), TRUE, "dowsing.c", 603);
    work->heapId = heapId;
    work->subscreenHeapId = subscreenHeapId;
    work->subscreen = subscreen;
    work->field = field;
    Dowsing_InitBG(work);
    Dowsing_LoadSignalBG(work);
    Dowsing_InitIcons(work);
    Dowsing_InitButton(work);
    work->signalMap = GFL_ArcSysReadHeapNewLZGetLen(ARCID_DOWSING, 13, FALSE, work->heapId, &size);
    work->vblankTCB = GFL_VBlankTCBAdd(Dowsing_VBlank, work, 1);
    work->frame = 0;
    work->active = TRUE;
    Dowsing_InitFade(work);
    work->prevSignal = 10;
    work->signal = SIGNAL_NONE;
    work->shownSignal = 10;
    work->prevHit.type = HIT_NONE;
    work->hit.type = HIT_NONE;
    work->lastX = 0;
    work->lastY = 0;
    work->lastZ = 0;
    work->unk8C = 0;
    work->soundOn = FALSE;
    work->restartIcon = FALSE;
    Dowsing_UpdateAltFrame(work, TRUE);
    work->searchRestart = TRUE;
    work->searchFirst = TRUE;
    gameData = GSYS_GetGameData(Field_GetGameSystem(work->field));
    eventWork = GameData_GetEventWork(gameData);
    GameData_GetSaveControl(gameData);
    work->search = DowsingSearch_Create(work->heapId, eventWork);
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, 0x10, 0x18, 1, 7);
    func_02042ba8(FALSE, work->heapId);
    return work;
}

void Dowsing_Free(DowsingWork *work) {
    *(vu16 *)REG_DB_BLDCNT_ADDR = 0;
    DowsingSearch_Free(work->search);
    Dowsing_FreeFade(work);
    GFL_TCBRemove(work->vblankTCB);
    GFL_HeapFree(work->signalMap);
    Dowsing_ExitButton(work);
    Dowsing_FreeIcons(work);
    Dowsing_FreeSignalBG(work);
    Dowsing_ExitBG(work);
    GFL_HeapFree(work);
}

// Whether the hit that the signal comes from has moved since the frame before, so that the signal's sound plays again
static inline BOOL Dowsing_HitMoved(DowsingWork *work) {
    u32 oldZ;
    u32 oldX;
    u32 newZ;
    u32 newX;
    BOOL changed = FALSE;
    if (work->prevHit.type == HIT_HIDDEN_ITEM) {
        oldX = work->prevHit.item->x;
        oldZ = work->prevHit.item->z;
    } else {
        return TRUE;
    }
    if (work->hit.type == HIT_HIDDEN_ITEM) {
        newX = work->hit.item->x;
        newZ = work->hit.item->z;
    }
    if ((oldX != newX || oldZ != newZ) && work->prevSignal != work->signal) {
        changed = TRUE;
    }
    return changed;
}

void Dowsing_Update(DowsingWork *work, BOOL active) {
    MapMatrix *matrix;
    GameSystem *gsys;
    s32 playerX;
    s32 playerY;
    s32 playerZ;
    u32 zoneId;
    BOOL zoneIsMatrix0;
    u16 i;
    u16 count;
    s32 minX;
    s32 maxX;
    s32 minZ;
    s32 maxZ;
    s32 bestDistance;
    u16 bestIndex;
    VecFx32 *pos;
    s32 x;
    s32 z;

    work->active = active;
    if (active) {
        work->fadeCmd = FADE_CMD_IN;
    } else {
        work->fadeCmd = FADE_CMD_OUT;
    }
    InfoWin_Update();
    if (!active) {
        return;
    }

    work->prevSignal = work->signal;
    work->prevHit.type = work->hit.type;
    if (work->prevHit.type == HIT_HIDDEN_ITEM) {
        work->prevHit.item = work->hit.item;
    }

    gsys = Field_GetGameSystem(work->field);
    pos = PlayerState_GetWPos(GSYS_GetPlayerState(gsys));
    playerX = (pos->x >> 4) / 4096;
    playerY = (pos->y >> 4) / 4096;
    playerZ = (pos->z >> 4) / 4096;

    matrix = GetMapMatrixSystem(GSYS_GetGameData(gsys));
    zoneId = ZONE_NONE;
    zoneIsMatrix0 = FALSE;
    if (RangeCheckChunkCoordinateWorld(matrix, pos->x, pos->z) == TRUE) {
        zoneId = GetZoneIDAtMatrixXZWorld(matrix, pos->x, pos->z);
        zoneIsMatrix0 = GetIsZoneMatrix0(zoneId);
    }

    minX = playerX - SEARCH_LEFT;
    maxX = playerX + SEARCH_RIGHT;
    minZ = playerZ - SEARCH_TOP;
    maxZ = playerZ + SEARCH_BOTTOM;
    DowsingHit hit = { HIT_NONE, NULL };
    DowsingHit best = hit;
    bestDistance = -1;

    if (work->searchRestart) {
        DowsingSearch_Reset(work->search);
        work->searchRestart = FALSE;
    }
    if (work->searchFirst) {
        count = ZONE_NONE;
        work->searchFirst = FALSE;
    } else {
        count = 11;
    }

    for (i = 0; i < count; i++) {
        if (i == 0) {
            if (work->prevHit.type != HIT_NONE) {
                if (DowsingSearch_IsHere(work->search, &work->prevHit)) {
                    hit.type = work->prevHit.type;
                    if (hit.type == HIT_HIDDEN_ITEM) {
                        hit.item = work->prevHit.item;
                    }
                } else {
                    work->searchRestart = TRUE;
                    work->searchFirst = TRUE;
                    break;
                }
            }
        } else {
            work->searchRestart = !DowsingSearch_Next(work->search, &hit, 0, zoneId);
            if (work->searchRestart) {
                break;
            }
        }
        if (zoneId != ZONE_NONE) {
            if (hit.type == HIT_HIDDEN_ITEM) {
                const HiddenItem *item = hit.item;
                if (zoneId != item->zoneId) {
                    if (!item->outdoors || !zoneIsMatrix0 || GetZoneIsEntralinkAny(item->zoneId)
                        || item->zoneId == ZONE_ROUTE_4_16) {
                        continue;
                    }
                }
                x = item->x;
                z = item->z;
            }
            if (minX <= x && x < maxX && minZ <= z && z < maxZ) {
                s32 dz = z - playerZ;
                s32 dx = x - playerX;
                u16 index = (dx + MAP_PLAYER_X) + (dz + MAP_PLAYER_Z) * MAP_WIDTH;
                if (work->signalMap[index] != SIGNAL_NONE) {
                    u32 distance = dx * dx + dz * dz;
                    if (distance < bestDistance) {
                        bestIndex = index;
                        bestDistance = distance;
                        best.type = hit.type;
                        if (hit.type == HIT_HIDDEN_ITEM) {
                            best.item = hit.item;
                        }
                    }
                }
            }
        }
    }

    work->hit.type = best.type;
    if (best.type != HIT_NONE) {
        work->signal = work->signalMap[bestIndex];
        if (best.type == HIT_HIDDEN_ITEM) {
            work->hit.item = best.item;
        }
    } else {
        work->signal = SIGNAL_NONE;
    }

    {
        BOOL quiet = TRUE;
        if (work->signal == SIGNAL_CLOSEST) {
            BOOL start = FALSE;
            if (!work->soundOn) {
                start = TRUE;
                work->soundOn = TRUE;
            } else if (work->soundTimer == 6) {
                if (!GFL_SndPlayerIsActive(work->soundPlayer)) {
                    work->soundTimer--;
                }
            } else if (work->soundTimer == 0) {
                start = TRUE;
            } else {
                work->soundTimer--;
            }
            if (start) {
                work->soundPlayer = GFL_SndSeqGetPlayerIndex(SEQ_SE_FLD_120);
                GFL_SEPlayKeepVol(SEQ_SE_FLD_120, work->soundPlayer);
                GFL_SndPlayerSetParams(work->soundPlayer, -1, 6 << 6, -1);
                work->soundTimer = 6;
                work->restartIcon = TRUE;
            }
            quiet = FALSE;
        } else {
            work->soundOn = FALSE;
        }

        if (quiet && work->signal != SIGNAL_CLOSEST && work->hit.type != HIT_NONE) {
            if (Dowsing_HitMoved(work)) {
                s32 player = GFL_SndSeqGetPlayerIndex(SEQ_SE_FLD_120);
                s32 pan = (sSignals[work->signal].pan - 8) * 16;
                if (pan < -127) {
                    pan = -127;
                } else if (pan > 127) {
                    pan = 127;
                }
                GFL_SEPlayKeepVol(SEQ_SE_FLD_120, player);
                GFL_SndPlayerSetParams(player, -1, -1, pan);
                quiet = FALSE;
            }
        }

        if (quiet && work->prevSignal != work->signal) {
            GFL_SndSEPlay(SEQ_SE_FLD_122);
        }
    }

    work->lastX = playerX;
    work->lastY = playerY;
    work->lastZ = playerZ;
    Dowsing_UpdateButton(work);
}

void Dowsing_UpdateAltFrame(DowsingWork *work, BOOL active) {
    Dowsing_UpdateFade(work);
    if (active) {
        if (work->shownSignal != work->signal) {
            ClActorPos pos;
            func_0204c124(work->icons[ICON_DIRECTION].actor, work->signal != SIGNAL_CLOSEST && work->signal != SIGNAL_NONE);
            func_0204c124(work->icons[ICON_CLOSEST].actor, work->signal == SIGNAL_CLOSEST);
            pos.x = sSignals[work->signal].x;
            pos.y = sSignals[work->signal].y;
            func_0204c140(work->icons[ICON_DIRECTION].actor, &pos, 1);
            func_0204c488(work->icons[ICON_RING].actor, sSignals[work->signal].anims0);
            func_0204c488(work->icons[ICON_DOT].actor, sSignals[work->signal].anims1);
            func_0204c488(work->icons[ICON_DIRECTION].actor, sSignals[work->signal].anims2);
            work->shownSignal = work->signal;
        }
        if (work->signal == SIGNAL_CLOSEST && work->restartIcon) {
            func_0204c488(work->icons[ICON_CLOSEST].actor, 0);
            func_0204c520(work->icons[ICON_CLOSEST].actor, TRUE);
            func_0204c4d4(work->icons[ICON_CLOSEST].actor, 0);
            work->restartIcon = FALSE;
        }
    }
}

static void Dowsing_VBlank(TCB *tcb, void *data) {
    DowsingWork *work = data;
    PaletteFade_Transfer(work->paletteFade);
    if (work->active) {
        if (work->frame >= 4) {
            work->frame = 0;
        }
        work->frame++;
    }
}

static void Dowsing_InitBG(DowsingWork *work) {
    int i;

    for (i = 4; i <= 7; i++) {
        GFL_BGSysSetBGEnabled(i, FALSE);
    }
    for (i = 0; i < NELEMS(sDowsingBGSetups); i++) {
        const DowsingBGSetup *bgSetup = &sDowsingBGSetups[i];
        u32 bg = bgSetup->bg;
        GFL_BGSysCreateBG(bg, &bgSetup->setup, bgSetup->mode);
        GFL_BGSysClearBG(bg);
        GFL_BGSysSetBGEnabled(bg, bgSetup->enabled);
        GFL_BGSysMoveBG(bg, BG_MOVE_SET_X, 0);
        GFL_BGSysMoveBG(bg, BG_MOVE_SET_Y, 0);
    }
    work->unit = func_0204bf1c(8, 0, work->heapId);
    func_0204c028(work->unit);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void Dowsing_ExitBG(DowsingWork *work) {
    int i;

    func_0204bf98(work->unit);
    for (i = 0; i < NELEMS(sDowsingBGSetups); i++) {
        GFL_BGSysReleaseBG(sDowsingBGSetups[i].bg);
    }
}

static void Dowsing_LoadSignalBG(DowsingWork *work) {
    u32 chars;
    HeapID heapId = work->subscreenHeapId;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_DOWSING, heapId);

    GFL_G2DIOLoadArcNCLR(arc, 0, PALTYPE_SUB_BG, 0, 0, 0x40, heapId);
    chars = GFL_BGSysLoadArcNCGRDynamic(arc, 2, BG_SIGNAL, 0, FALSE, heapId);
    GFL_G2DIOLoadNSCRSync(arc, 7, BG_SIGNAL, 0, CHAR_POS(chars), 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysSetScrPaletteNo(BG_SIGNAL, 0, 0, 32, 24, 1);
    GFL_BGSysQueueScrLoad(BG_SIGNAL);
    work->signalChars = chars;
}

static void Dowsing_FreeSignalBG(DowsingWork *work) {
    GFL_BGSysFreeCharMemory(BG_SIGNAL, CHAR_POS(work->signalChars), CHAR_SIZE(work->signalChars));
}

static void Dowsing_InitIcons(DowsingWork *work) {
    u8 i;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_DOWSING, work->subscreenHeapId);

    work->iconPalette = func_0204bbb8(arc, 1, 1, 0, 0, 1, work->heapId);
    for (i = 0; i < ICON_COUNT; i++) {
        ClActorSetup setup = { 0, 0, 0, 0, 0 };
        if (i == ICON_DOT) {
            work->icons[i].chars = work->icons[ICON_RING].chars;
            work->icons[i].cellAnims = work->icons[ICON_RING].cellAnims;
        } else {
            const DowsingIconRes *res = &sIconRes[i];
            work->icons[i].chars = func_0204b81c(arc, res->file, FALSE, 1, work->heapId);
            work->icons[i].cellAnims = func_0204bde0(arc, res->cellFile, res->animFile, work->heapId);
        }
        setup.priority = i;
        work->icons[i].actor = func_0204c040(work->unit, work->icons[i].chars, work->iconPalette,
                                             work->icons[i].cellAnims, &setup, 1, work->heapId);
        func_0204c520(work->icons[i].actor, TRUE);
        func_0204c318(work->icons[i].actor, 1);
    }
    GFL_ArcToolFree(arc);

    {
        ClActorPos pos;
        pos.x = 0x50;
        pos.y = 0x60;
        func_0204c140(work->icons[ICON_RING].actor, &pos, 1);
        pos.x = 0xb0;
        pos.y = 0x60;
        func_0204c140(work->icons[ICON_DOT].actor, &pos, 1);
        pos.x = 0x80;
        pos.y = 0x60;
        func_0204c140(work->icons[ICON_CLOSEST].actor, &pos, 1);
    }
}

static void Dowsing_FreeIcons(DowsingWork *work) {
    u8 i;

    for (i = 0; i < ICON_COUNT; i++) {
        func_0204c108(work->icons[i].actor);
        if (i != ICON_DOT) {
            func_0204be64(work->icons[i].cellAnims);
            func_0204b98c(work->icons[i].chars);
        }
    }
    func_0204bcd0(work->iconPalette);
}

static void Dowsing_InitButton(DowsingWork *work) {
    u16 i;
    NNSG2dScreenData *screen;
    void *screenBuffer;
    u16 chars;
    u16 *tiles;
    u16 heapId = work->subscreenHeapId;
    ArcTool *arc;

    InfoWin_Init(BG_INFOWIN, 3, GSYS_GetGameData(Field_GetGameSystem(work->field)), work->heapId);
    ClActorSetup setup = { 0, 0, 0, 0, 0 };
    arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, func_0202d820(), PALTYPE_SUB_BG, 0x40, 0x20, heapId);
    work->barChars = GFL_BGSysLoadArcNCGRDynamic(arc, func_0202d824(), BG_INFOWIN, 0, FALSE, heapId);
    screenBuffer = GFL_G2DIOReadNSCRArc(arc, func_0202d828(), FALSE, &screen, heapId);
    chars = work->barChars;
    tiles = (u16 *)screen->rawData;
    for (i = 0; i < 0x300; i++) {
        tiles[i] &= 0xfff;
        tiles[i] = (chars + tiles[i]) | (2 << 12);
    }
    GFL_BGSysLoadScrArea(BG_INFOWIN, 0, 0x15, 0x20, 3, screen->rawData, 0, 0x15, 0x20, 0x18);
    GFL_BGSysQueueScrLoad(BG_INFOWIN);
    GFL_HeapFree(screenBuffer);

    work->buttonPalette = func_0204bbb8(arc, func_0202d810(), 1, 0x20, 0, 3, work->heapId);
    work->buttonChars = func_0204b81c(arc, func_0202d814(), FALSE, 1, work->heapId);
    work->buttonCellAnims = func_0204bde0(arc, func_0202d818(0), func_0202d81c(0), work->heapId);
    setup.x = BUTTON_X;
    setup.y = BUTTON_Y;
    work->button = func_0204c040(work->unit, work->buttonChars, work->buttonPalette, work->buttonCellAnims, &setup, 1,
                                 work->heapId);
    func_0204c520(work->button, TRUE);
    func_0204c53c(work->button, 1 << 13);
    func_0204c488(work->button, 0);
    GFL_ArcToolFree(arc);
    work->buttonState = BUTTON_WAIT;
}

static void Dowsing_ExitButton(DowsingWork *work) {
    func_0204c108(work->button);
    func_0204be64(work->buttonCellAnims);
    func_0204b98c(work->buttonChars);
    func_0204bcd0(work->buttonPalette);
    GFL_BGSysFreeCharMemory(BG_INFOWIN, CHAR_POS(work->barChars), CHAR_SIZE(work->barChars));
    InfoWin_Exit();
}

static void Dowsing_UpdateButton(DowsingWork *work) {
    u32 x;
    u32 y;
    BOOL pressed;

    switch (work->buttonState) {
    case BUTTON_WAIT:
        pressed = FALSE;
        if (func_0203dac8(&x, &y) && x >= BUTTON_X && x < 256 && y >= BUTTON_Y && y < BUTTON_Y + BUTTON_HEIGHT) {
            pressed = TRUE;
            func_0203d564(TRUE);
        }
        if (!pressed && (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X)) {
            func_0203d564(FALSE);
            pressed = TRUE;
        }
        if (pressed) {
            work->buttonState = BUTTON_PRESSED;
            func_0204c488(work->button, 8);
            GFL_SndSEPlay(SEQ_SE_CLOSE1);
            return;
        }
        break;
    case BUTTON_PRESSED:
        if (!func_0204c560(work->button)) {
            work->buttonState = BUTTON_RELEASED;
            func_0204c488(work->button, 0);
        }
        break;
    case BUTTON_RELEASED:
        FieldSubscreen_ReqChange(work->subscreen, 0);
        work->buttonState = BUTTON_DONE;
        break;
    case BUTTON_DONE:
        break;
    }
}

static void Dowsing_InitFade(DowsingWork *work) {
    u8 i;

    work->tcbMemory = GFL_HeapAllocate(work->heapId, GFL_TCBMgrCalcAllocSize(2), TRUE, "dowsing.c", 1628);
    work->tcbManager = GFL_TCBMgrCreate(2, work->tcbMemory);
    work->paletteFade = PaletteFade_Create(work->heapId);
    PaletteFade_SetTransferAll(work->paletteFade, TRUE);
    PaletteFade_AllocBuffer(work->paletteFade, 1, 0x60, work->heapId);
    PaletteFade_AllocBuffer(work->paletteFade, 3, 0x80, work->heapId);
    PaletteFade_LoadFromVRAM(work->paletteFade, 1, 0, 0x60);
    PaletteFade_LoadFromVRAM(work->paletteFade, 3, 0, 0x80);
    work->fadeCmd = FADE_CMD_NONE;
    work->fadeState = 0;
}

static void Dowsing_FreeFade(DowsingWork *work) {
    PaletteFade_FreeBuffer(work->paletteFade, 1);
    PaletteFade_FreeBuffer(work->paletteFade, 3);
    PaletteFade_Free(work->paletteFade);
    func_0203a610(work->tcbManager);
    GFL_HeapFree(work->tcbMemory);
}

static void Dowsing_UpdateFade(DowsingWork *work) {
    u8 i;
    u16 mask;
    u16 mask2;

    GFL_TCBMgrUpdate(work->tcbManager);
    if (work->fadeCmd == FADE_CMD_NONE || PaletteFade_GetActiveMask(work->paletteFade) != 0) {
        return;
    }

    switch (work->fadeCmd) {
    case FADE_CMD_IN:
        if (work->fadeState != 0) {
            mask = 0;
            for (i = 0; i < 3; i++) {
                mask |= 1 << i;
            }
            PaletteFade_StartFade(work->paletteFade, 2, mask, 0, 0, 0, 0, work->tcbManager);
            mask2 = 0;
            for (i = 0; i < 4; i++) {
                mask2 |= 1 << i;
            }
            PaletteFade_StartFade(work->paletteFade, 8, mask2, 0, 0, 0, 0, work->tcbManager);
            work->fadeState = 0;
            for (i = 0; i < ICON_COUNT; i++) {
                func_0204c540(work->icons[i].actor);
            }
        }
        break;
    case FADE_CMD_OUT:
        if (work->fadeState != 1) {
            mask = 0;
            for (i = 0; i < 3; i++) {
                mask |= 1 << i;
            }
            PaletteFade_StartFade(work->paletteFade, 2, mask, 0, 8, 8, 0, work->tcbManager);
            mask2 = 0;
            for (i = 0; i < 4; i++) {
                mask2 |= 1 << i;
            }
            PaletteFade_StartFade(work->paletteFade, 8, mask2, 0, 8, 8, 0, work->tcbManager);
            work->fadeState = 1;
            for (i = 0; i < ICON_COUNT; i++) {
                func_0204c550(work->icons[i].actor);
            }
        }
        break;
    }
    work->fadeCmd = FADE_CMD_NONE;
}
