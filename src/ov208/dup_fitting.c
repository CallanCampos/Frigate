#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/dup_fitting.h"
#include "field/dup_fitting_item.h"
#include "field/musical_dressup_sys.h"
#include "app/musical/mus_item_data.h"
#include "app/musical/mus_item_draw.h"
#include "app/musical/mus_poke_draw.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/blact.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/math_util.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "nnsys/g3d.h"
#include "nnsys/gfd.h"
#include "save/musical_savedata.h"
#include "save/save_control.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/title_13.h"
#include "text/system/title_3.h"
#include "text/system/title_s005d_word_set_musical_info.h"

// Overlay 208's dup_fitting.c, named after its string: the musical's dressing room. A ring of the props the player
// owns turns on the touch screen, and the player pulls them off it to put them on the Pokémon on the top screen.
// The names are guessed from what the code does

#define DUP_FITTING_ENTRY_MAX 250
// How far apart the props of the ring are, in angle (0x10000 for a full turn, over 14 props)
#define DUP_FITTING_RING_STEP 0x1249
#define DUP_FITTING_RING_PROPS 14
// The positions of the Pokémon that a prop is worn at, and the one of a prop that is not worn
typedef enum {
    DUP_FITTING_EQUIP_FIRST,
    DUP_FITTING_EQUIP_MAX = 9,
    DUP_FITTING_EQUIP_NONE = 10,
} DupFittingEquipPos;
// The category that shows the props in the order they are kept in
#define DUP_FITTING_CATEGORY_ORIGINAL 8
#define DUP_FITTING_MSG_ID_NONE 0xff

enum {
    DUP_FITTING_STATE_DEMO,
    DUP_FITTING_STATE_DRESS,
    DUP_FITTING_STATE_CONFIRM_IN,
    DUP_FITTING_STATE_CONFIRM,
    DUP_FITTING_STATE_WAIT_TIMING,
    DUP_FITTING_STATE_WAIT_SOUND,
    DUP_FITTING_STATE_CONFIRM_OUT,
    DUP_FITTING_STATE_START,
    DUP_FITTING_STATE_FADE_IN,
    DUP_FITTING_STATE_FADE_OUT,
    DUP_FITTING_STATE_DONE_WAIT,
};

struct DupFittingWork {
    HeapID heapId;
    MusicalDressUpParam *param;
    TCB *vblankTcb;
    s32 state;
    // How far the confirmation screen has slid in
    s32 transition;
    // Whether the touch screen was touched this frame, and whether it is held
    BOOL touchStart;
    BOOL touching;
    u32 touchX;
    u32 touchY;
    u32 prevTouchX;
    u32 prevTouchY;
    u16 prevAngle;
    BOOL dragging;
    // The position of the Pokémon that the held prop is over, or DUP_FITTING_EQUIP_NONE
    s32 hoverPos;
    DupFittingItemNode *heldNode;
    // How the held prop is held: 1 on the ring, 2 pulled off it
    s32 holdState;
    // Where it was taken from: 1 the ring, 2 the stage, 3 the Pokémon
    s32 holdKind;
    DupFittingPoint holdOrigin;
    // The prop that shows what the held prop looks like when it is worn, as a shadow
    MusItemDraw *shadowDraw;
    u16 shadowItemId;
    u32 shadowMaterial;
    u32 badgeMaterial;
    u32 badgeActors[DUP_FITTING_RING_PROPS];
    u16 badgeFrame;
    BOOL autoScroll;
    u32 scrolled;
    u32 scrollTarget;
    s32 category;
    u16 entryCount;
    DupFittingItemEntry entries[DUP_FITTING_ENTRY_MAX];
    DupFittingItemEntry *sorted[DUP_FITTING_ENTRY_MAX];
    // The props on the ring, on the stage, worn, and going back to the ring
    DupFittingItemList *carouselList;
    DupFittingItemList *stageList;
    DupFittingItemList *wornList;
    DupFittingItemList *returnList;
    u16 angle;
    s16 angleSpeed;
    u16 dragAccum;
    s32 carouselPos;
    s16 tilt;
    s16 tiltSpeed;
    // The demonstration of the first visit: whether it plays, and the stage it is at
    BOOL demo;
    BOOL showDemo;
    u16 demoFrame;
    u8 demoStage;
    u8 demoItem;
    ClActUnit *clactUnit;
    ClActor *buttons[2];
    ClActor *hand;
    // The palettes, characters and cells of the actors
    u32 clactRes[9];
    MusPokeDrawSys *pokeSys;
    MusPokeDraw *poke;
    MusItemDrawSys *itemSys;
    G3DCamera *camera;
    BlActScene *blact;
    ClActor *sparkles[8];
    ClActor *okMark;
    ClActor *touchEffect;
    u8 sparkleTimer[8];
    u8 confirmTimer;
    u8 bgScrollFrame;
    u16 sndCooldown;
    s16 rotateAccum;
    u8 msgSlide;
    BOOL msgClosed;
    BOOL printing;
    BOOL msgPhase;
    u16 msgItemId;
    MsgData *dressMsg;
    MsgData *itemMsg;
    MsgData *categoryMsg;
    Font *font;
    PrintQueue *printQueue;
    BmpWin *bmpWin;
};

static void DupFitting_VBlank(TCB *tcb, void *data);
static void DupFitting_InitGraphics(DupFittingWork *work);
static void DupFitting_LoadResources(DupFittingWork *work);
static void DupFitting_InitBG(const BGSetup *setup, u8 bg);
static void DupFitting_InitPoke(DupFittingWork *work);
static void DupFitting_InitItems(DupFittingWork *work);
static void DupFitting_AddStageItem(DupFittingWork *work, u16 itemId);
static void DupFitting_GetRandomStagePos(DupFittingWork *work, DupFittingPoint *pos);
static void DupFitting_FreeItems(DupFittingWork *work);
static void DupFitting_UpdateDress(DupFittingWork *work);
static DupFittingItemNode *DupFitting_UpdateCarousel(DupFittingWork *work, u16 angle, s16 delta, u16 radiusX,
                                                     u16 radiusY);
static void DupFitting_RefreshCarouselItems(DupFittingWork *work);
static void DupFitting_UpdateInput(DupFittingWork *work);
static void DupFitting_UpdateCarouselDrag(DupFittingWork *work, s16 x, s16 y);
static BOOL DupFitting_PickFromList(DupFittingWork *work, DupFittingItemList *list, s32 kind);
static void DupFitting_UpdateHeldItem(DupFittingWork *work, BOOL playSound);
static void DupFitting_PullOffCopy(DupFittingWork *work);
static void DupFitting_RaiseHeldItem(DupFittingWork *work);
static void DupFitting_StartReturn(DupFittingWork *work, BOOL unused);
static void DupFitting_WearItem(DupFittingWork *work);
static void DupFitting_ReleaseHeld(DupFittingWork *work);
static BOOL DupFitting_SelectCategory(DupFittingWork *work);
static void DupFitting_UpdateAutoScroll(DupFittingWork *work);
static void DupFitting_UpdateTilt(DupFittingWork *work);
static BOOL DupFitting_HasWornInCategory(DupFittingWork *work, s32 pos);
static s32 DupFitting_FindNearestEquipPos(DupFittingWork *work, MusItemDraw *draw, const s32 *pos, u16 *distance);
static void DupFitting_UpdateReturning(DupFittingWork *work);
static void DupFitting_UpdateShadow(DupFittingWork *work);
static BOOL DupFitting_IsInEllipse(s16 x, s16 y, int radius, f32 xScale);
static void DupFitting_SortEntries(DupFittingWork *work, s32 category, u16 start);
static u16 DupFitting_GetSortKey(DupFittingWork *work, DupFittingItemEntry *entry, s32 pos);
static void DupFitting_UpdateTransition(DupFittingWork *work);
static u32 DupFitting_UpdateConfirm(DupFittingWork *work);
static void DupFitting_ResetRotations(DupFittingWork *work);
static void DupFitting_UpdateConfirmTouch(DupFittingWork *work);
static void DupFitting_RotateHeld(DupFittingWork *work);
static void DupFitting_SaveWornItems(DupFittingWork *work);
static void DupFitting_UpdateButtons(DupFittingWork *work);
static void DupFitting_StartDemo(DupFittingWork *work);
static void DupFitting_EndDemo(DupFittingWork *work);
static void DupFitting_UpdateDemo(DupFittingWork *work);
static void DupFitting_DemoRotate(DupFittingWork *work, u16 target, s32 total, u16 frames);
static void DupFitting_DemoWait(DupFittingWork *work, u16 frames);
static void DupFitting_DemoMoveHand(DupFittingWork *work, const DupFittingPoint *from, const DupFittingPoint *to,
                                    u16 frames);
static void DupFitting_InitMessages(DupFittingWork *work);
static void DupFitting_FreeMessages(DupFittingWork *work);
static void DupFitting_PrintMessage(DupFittingWork *work, u32 messageId, WordSet *wordSet);
static void DupFitting_PrintPokeName(DupFittingWork *work);
static void DupFitting_PrintItemName(DupFittingWork *work, DupFittingItemNode *node);
static void DupFitting_PrintCategory(DupFittingWork *work, s32 category);
static void DupFitting_InitSparkles(DupFittingWork *work);
static void DupFitting_FreeSparkles(DupFittingWork *work);
static void DupFitting_UpdateSparkles(DupFittingWork *work);
static void DupFitting_ShowTouchEffect(DupFittingWork *work);
static void DupFitting_Trace(DupFittingWork *work, u8 id);

// The index of the prop at a position of the ring, which wraps around once
static inline u16 DupFitting_GetRingIndexAt(DupFittingWork *work, u32 pos) {
    u32 total = work->entryCount * DUP_FITTING_RING_STEP;

    if (pos >= total) {
        return (pos - total) / DUP_FITTING_RING_STEP;
    }
    return pos / DUP_FITTING_RING_STEP;
}

// The index of the prop on the ring that is "offset" turns past the front of the carousel
static inline u16 DupFitting_GetRingIndex(DupFittingWork *work, u32 offset) {
    return DupFitting_GetRingIndexAt(work, work->carouselPos + offset);
}

DupFittingWork *DupFitting_Create(MusicalDressUpParam *param, HeapID heapId) {
    DupFittingWork *work = GFL_HeapAllocate(heapId, sizeof(DupFittingWork), TRUE, "dup_fitting.c", 480);
    u32 pos;

    work->heapId = heapId;
    work->param = param;
    work->state = DUP_FITTING_STATE_START;
    work->transition = 0;
    work->touchStart = FALSE;
    work->touching = FALSE;
    work->dragging = FALSE;
    work->heldNode = NULL;
    work->holdState = 0;
    work->holdKind = 0;
    work->autoScroll = FALSE;
    work->category = DUP_FITTING_CATEGORY_ORIGINAL;
    work->angle = 0;
    work->tilt = 0;
    work->tiltSpeed = 0;
    work->demo = FALSE;
    work->msgSlide = 0;
    work->msgClosed = FALSE;
    work->msgPhase = FALSE;
    work->printing = FALSE;
    work->msgItemId = DUP_FITTING_MSG_ID_NONE;
    work->bgScrollFrame = 0;
    work->carouselPos = 0;
    work->bmpWin = NULL;
    work->angleSpeed = 0;
    work->hoverPos = DUP_FITTING_EQUIP_NONE;
    DupFitting_InitGraphics(work);
    DupFitting_LoadResources(work);
    DupFitting_InitPoke(work);
    DupFitting_InitItems(work);
    DupFitting_InitSparkles(work);
    DupFitting_InitMessages(work);
    work->vblankTcb = GFL_VBlankTCBAdd(DupFitting_VBlank, work, 8);
    GFL_WipeSet(WIPE_MODE_SUB_FIRST, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_WHITE, 18, 1, work->heapId);
    pos = work->carouselPos + 6 * 0x4000;
    work->sndCooldown = 10;
    DupFitting_SortEntries(work, DUP_FITTING_CATEGORY_ORIGINAL, DupFitting_GetRingIndexAt(work, pos));
    work->category = DUP_FITTING_CATEGORY_ORIGINAL;
    work->autoScroll = TRUE;
    work->scrolled = 0;
    work->scrollTarget = work->angle % DUP_FITTING_RING_STEP + 0x10000;
    if (func_0200af44(work->param->save) == FALSE) {
        func_0200af50(work->param->save, TRUE);
        work->showDemo = TRUE;
    } else {
        work->showDemo = FALSE;
    }
    GFL_SndBGMPlay(SEQ_BGM_MSL_DRESSUP, 0xffff);
    func_02042ba8(FALSE, work->heapId);
    return work;
}

void DupFitting_Free(DupFittingWork *work) {
    u8 i;

    func_0203d564(TRUE);
    func_0200ae14(work->param->save);
    func_02005d8c();
    DupFitting_FreeMessages(work);
    DupFitting_FreeSparkles(work);
    func_0204c108(work->buttons[0]);
    func_0204c108(work->buttons[1]);
    func_0204c108(work->hand);
    for (i = 0; i <= 2; i++) {
        func_0204bcd0(work->clactRes[i]);
    }
    for (i = 3; i <= 5; i++) {
        func_0204b98c(work->clactRes[i]);
    }
    for (i = 6; i <= 8; i++) {
        func_0204be64(work->clactRes[i]);
    }
    if (work->bmpWin != NULL) {
        BmpWin_Free(work->bmpWin);
    }
    func_0204bf98(work->clactUnit);
    func_0204b758();
    DupFitting_FreeItems(work);
    MusPokeDraw_DelPoke(work->pokeSys, work->poke);
    MusPokeDraw_TermSystem(work->pokeSys);
    GFL_TCBRemove(work->vblankTcb);
    func_0204e450(work->blact);
    GFL_G3DCameraFree(work->camera);
    GFL_G3DSysFree();
    GFL_BGSysReleaseBG(0);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(7);
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    GFL_HeapFree(work);
}

static inline void DupFitting_SlideMessageBGs(DupFittingWork *work) {
    GFL_BGSysMoveBGReq(6, BG_MOVE_SET_X, 128 - work->msgSlide);
    GFL_BGSysMoveBGReq(5, BG_MOVE_SET_X, work->msgSlide - 128);
}

u32 DupFitting_Main(DupFittingWork *work) {
    switch (work->state) {
    case DUP_FITTING_STATE_START:
        GFL_SndSEPlay(SEQ_SE_DECIDE3);
        work->state = DUP_FITTING_STATE_FADE_IN;
        // fallthrough
    case DUP_FITTING_STATE_FADE_IN:
        if (GFL_WipeIsFinished() == TRUE) {
            if (work->showDemo == TRUE) {
                DupFitting_StartDemo(work);
            } else {
                work->state = DUP_FITTING_STATE_DRESS;
            }
        }
        if (work->autoScroll == TRUE) {
            DupFitting_UpdateAutoScroll(work);
        }
        break;
    case DUP_FITTING_STATE_FADE_OUT:
        if (GFL_WipeIsFinished() == TRUE && GFL_SndBGMIsFading() == FALSE) {
            return 2;
        }
        break;
    case DUP_FITTING_STATE_DEMO:
        DupFitting_UpdateDemo(work);
        break;
    case DUP_FITTING_STATE_DRESS:
        DupFitting_UpdateDress(work);
        break;
    case DUP_FITTING_STATE_CONFIRM_IN:
        work->transition += 6;
        if (work->transition >= 192) {
            DupFitting_PrintMessage(work, Title3_Text_LookOk, NULL);
            work->transition = 192;
            work->state = DUP_FITTING_STATE_CONFIRM;
            work->tilt = 0;
            work->tiltSpeed = 0;
            work->rotateAccum = 0;
        }
        DupFitting_UpdateTransition(work);
        break;
    case DUP_FITTING_STATE_CONFIRM_OUT:
        work->transition -= 6;
        if (work->transition < 0) {
            work->transition = 0;
            work->state = DUP_FITTING_STATE_DRESS;
        }
        DupFitting_UpdateTransition(work);
        break;
    case DUP_FITTING_STATE_CONFIRM:
        if (DupFitting_UpdateConfirm(work) == 2) {
            work->state = DUP_FITTING_STATE_DONE_WAIT;
            work->confirmTimer = 0;
            func_0204c124(work->okMark, TRUE);
        }
        break;
    case DUP_FITTING_STATE_DONE_WAIT:
        work->confirmTimer++;
        DupFitting_UpdateButtons(work);
        if (work->confirmTimer > 60) {
            if (work->param->comm == NULL) {
                GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_WHITE, 18, 1,
                            work->heapId);
                GFL_SndBGMFadeOut(60);
                work->state = DUP_FITTING_STATE_FADE_OUT;
                work->msgClosed = TRUE;
            } else {
                DupFitting_PrintMessage(work, Title3_Text_WaitingOtherParticipantsDress, NULL);
                func_ov211_021ef988(work->param->comm, 0x45);
                work->state = DUP_FITTING_STATE_WAIT_TIMING;
            }
        }
        break;
    case DUP_FITTING_STATE_WAIT_TIMING:
        if (func_ov211_021ef99c(work->param->comm, 0x45) == TRUE) {
            work->state = DUP_FITTING_STATE_WAIT_SOUND;
        }
        break;
    case DUP_FITTING_STATE_WAIT_SOUND:
        if (func_ov211_021f0240(work->param->comm) == TRUE) {
            GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_WHITE, 18, 1, work->heapId);
            GFL_SndBGMFadeOut(60);
            work->state = DUP_FITTING_STATE_FADE_OUT;
            work->msgClosed = TRUE;
        }
        break;
    }
    DupFitting_UpdateSparkles(work);
    DupFitting_UpdateReturning(work);
    MusPokeDraw_UpdateSystem(work->pokeSys);
    MusItemDraw_UpdateSystem(work->itemSys);
    func_0204b794();
    GFL_G3DSysReset();
    GFL_G3DSysMtxViewFlush();
    MusPokeDraw_DrawSystem(work->pokeSys);
    BlActScene_Draw(work->blact, work->camera, NULL);
    GFL_G3DSysReqSwapBuffers();
    work->bgScrollFrame++;
    if (work->bgScrollFrame > 1) {
        work->bgScrollFrame = 0;
        GFL_BGSysMoveBGReq(3, BG_MOVE_RIGHT, 1);
        GFL_BGSysMoveBGReq(3, BG_MOVE_UP, 1);
    }
    if (work->msgClosed == FALSE) {
        if (work->msgSlide < 128) {
            work->msgSlide += 8;
            DupFitting_SlideMessageBGs(work);
        } else if (work->msgPhase == FALSE) {
            work->msgPhase = TRUE;
            GFL_BGSysFillScr(7, 0);
            work->bmpWin = BmpWin_CreateDynamic(7, 1, 19, 30, 4, 14, TRUE);
            if (work->showDemo == FALSE) {
                DupFitting_PrintPokeName(work);
            } else {
                DupFitting_PrintMessage(work, Title3_Text_CanPullOutProps, NULL);
            }
            BmpWin_DrawFrame(work->bmpWin, WINFRAME_TRANSFER_VBLANK, 0x80, 12);
        }
    } else if (work->msgPhase == TRUE) {
        DupFitting_Trace(work, 2);
        work->msgPhase = FALSE;
        GFL_BGSysSetBGPriority(7, 3);
        GFL_BGSysSetBGPriority(4, 0);
        GFL_BGSysSetBGPriority(5, 1);
        GFL_BGSysSetBGPriority(6, 2);
        DupFitting_Trace(work, 4);
        loadBGScrToVramByNarcNoReserveNegAlign(ARCID_MUSICAL_SHOT, 13, 7, 0, 0, FALSE, work->heapId);
        GFL_BGSysQueueScrLoad(7);
        work->printing = FALSE;
        func_02021c44(work->printQueue);
        BmpWin_Free(work->bmpWin);
        work->bmpWin = NULL;
        DupFitting_Trace(work, 6);
    } else if (work->msgSlide != 0) {
        DupFitting_Trace(work, work->msgSlide + 0x60);
        work->msgSlide -= 8;
        DupFitting_SlideMessageBGs(work);
    }
    DupFitting_Trace(work, 8);
    if (work->printing == FALSE && work->state == DUP_FITTING_STATE_DRESS) {
        if (work->heldNode != NULL && work->msgItemId != DupFittingItemNode_GetEntry(work->heldNode)->itemId) {
            DupFitting_PrintItemName(work, work->heldNode);
            work->msgItemId = DupFittingItemNode_GetEntry(work->heldNode)->itemId;
        } else if (work->msgItemId != DUP_FITTING_MSG_ID_NONE && work->heldNode == NULL) {
            DupFitting_PrintPokeName(work);
            work->msgItemId = DUP_FITTING_MSG_ID_NONE;
        }
    }
    DupFitting_Trace(work, 10);
    if (work->printing == TRUE && func_02021c0c(work->printQueue) == TRUE) {
        work->printing = FALSE;
        BmpWin_Transfer(work->bmpWin);
        GFL_BGSysSetBGPriority(4, 1);
        GFL_BGSysSetBGPriority(5, 2);
        GFL_BGSysSetBGPriority(6, 3);
        GFL_BGSysSetBGPriority(7, 0);
    }
    DupFitting_Trace(work, 12);
    if (work->printQueue != NULL) {
        func_02021a3c(work->printQueue);
    }
    DupFitting_Trace(work, 14);
    if (work->sndCooldown != 0) {
        work->sndCooldown--;
    }
    if (work->param->comm != NULL && work->state != DUP_FITTING_STATE_START &&
        work->state != DUP_FITTING_STATE_FADE_IN && work->state != DUP_FITTING_STATE_FADE_OUT) {
        DupFitting_Trace(work, 0x20);
        if (GFL_NetErrCheck()) {
            DupFitting_Trace(work, 0x22);
            GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_WHITE, 18, 1, work->heapId);
            GFL_SndBGMFadeOut(60);
            DupFitting_Trace(work, 0x24);
            work->state = DUP_FITTING_STATE_FADE_OUT;
            DupFitting_Trace(work, 0x26);
        } else {
            DupFitting_Trace(work, 0x40);
        }
    }
    DupFitting_Trace(work, 16);
    return 0;
}

static void DupFitting_VBlank(TCB *tcb, void *data) {
    DupFittingWork *work = data;
    DupFittingItemNode *node = DupFittingItemList_GetHead(work->carouselList);
    BOOL changed = FALSE;

    for (; node != NULL; node = DupFittingItemNode_GetNext(node)) {
        DupFittingItemEntry *entry;
        DupFittingItemEntry *nextEntry;

        DupFittingItemNode_GetDraw(node);
        entry = DupFittingItemNode_GetEntry(node);
        nextEntry = DupFittingItemNode_GetNextEntry(node);
        if (entry->itemId != nextEntry->itemId) {
            DupFittingItemNode_ChangeEntry(node, work->itemSys, nextEntry);
            changed = TRUE;
        }
    }
    if (changed == TRUE) {
        DupFitting_RefreshCarouselItems(work);
    }
    if (work->heldNode != NULL) {
        DupFittingItemEntry *entry = DupFittingItemNode_GetEntry(work->heldNode);

        if (work->shadowItemId != entry->itemId) {
            MusItemData *items = MusItemDraw_GetItemData(work->itemSys);
            NNSGfdPlttKey plttKey;
            NNSGfdTexKey texKey;
            u16 width;
            u16 height;
            u32 size;
            u16 material;

            work->shadowItemId = entry->itemId;
            func_0204e820(work->blact, work->shadowMaterial, &plttKey);
            func_0204e7f8(work->blact, entry->material, &texKey);
            func_0204e808(work->blact, work->shadowMaterial, texKey);
            size = MusItemData_GetTexSize(MusItemData_GetItem(items, work->shadowItemId));
            func_0204e4d0(size, &width, &height);
            material = BlActScene_AddMaterialExistTex(work->blact, texKey, plttKey, 0, size, width, height);
            MusItemDraw_ChangeItem(work->itemSys, work->shadowDraw, work->shadowItemId, material);
            func_0204e73c(work->blact, work->shadowMaterial);
            work->shadowMaterial = material;
        }
    }
    func_0204b7c8();
    MusItemDraw_DrawSystem(work->itemSys);
}

// The data are defined in the order that gives the ROM's .rodata layout under MWCC's size sort (see
// docs/matching.md): the angle table and BG7's setup first, the edge colors between the camera vectors

// The ring angles up to which a prop is full size, shrinks, and shrinks to nothing
static const u16 DUP_FITTING_SCALE_ANGLES[3] = { 0x4000, 0x6e00, 0x78e0 };

static const BGSetup DUP_FITTING_BG7_SETUP = {
    0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7800), GX_BG_CHARBASE(0x10000), 0x8000,
    GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE,
};

static const BGSysVRAMConfig DUP_FITTING_VRAM_CONFIG = {
    GX_VRAM_BG_64_E,
    GX_VRAM_BGEXTPLTT_NONE,
    GX_VRAM_SUB_BG_128_C,
    GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_16_F,
    GX_VRAM_OBJEXTPLTT_NONE,
    GX_VRAM_SUB_OBJ_16_I,
    GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_A | GX_VRAM_B | GX_VRAM_D,
    GX_VRAM_TEXPLTT_0_G,
    GX_OBJVRAMMODE_CHAR_1D_64K,
    GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const VecFx32 DUP_FITTING_CAMERA_POSITION = { 0, 0, 301 * FX32_ONE };
static const VecFx32 DUP_FITTING_CAMERA_TARGET = { 0, 0, 0 };
static const GXRgb DUP_FITTING_EDGE_COLORS[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static const VecFx32 DUP_FITTING_CAMERA_UP = { 0, FX32_ONE, 0 };
static const BGSysLCDConfig DUP_FITTING_LCD_CONFIG = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };

static const BGSetup DUP_FITTING_BG1_SETUP = {
    0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7800), GX_BG_CHARBASE(0), 0x1000,
    GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE,
};
static const BGSetup DUP_FITTING_BG2_SETUP = {
    0, 0, 0x1000, 0, BGRES_256x512, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x6800), GX_BG_CHARBASE(0x8000), 0,
    GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE,
};
static const BGSetup DUP_FITTING_BG3_SETUP = {
    0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x6000), GX_BG_CHARBASE(0x8000), 0x8000,
    GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE,
};
static const BGSetup DUP_FITTING_BG4_SETUP = {
    0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7000), GX_BG_CHARBASE(0x8000), 0,
    GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE,
};
static const BGSetup DUP_FITTING_BG5_SETUP = {
    0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x6000), GX_BG_CHARBASE(0x8000), 0,
    GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE,
};
static const BGSetup DUP_FITTING_BG6_SETUP = {
    0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x5000), GX_BG_CHARBASE(0x8000), 0,
    GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE,
};

static void DupFitting_InitGraphics(DupFittingWork *work) {
    ClActSysSetup clactSetup;

    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GX_SetVisiblePlane(0);
    GXS_SetVisiblePlane(0);
    Wipe_SetScreenCovered(0, 0);
    Wipe_SetScreenCovered(1, 0);
    Wipe_HideWindows(0);
    Wipe_HideWindows(1);
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN);
    GFL_BGSysSetVRAMBanks(&DUP_FITTING_VRAM_CONFIG);
    GFL_G3DSysCreate(FALSE, 2, FALSE, 1, 0, work->heapId, NULL);
    GFL_G3DSysSetSwapBufferParams(0, 1);
    work->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_ORTHO, FX32_CONST(12), 0, 0, FX32_CONST(16), FX32_ONE,
                                       FX32_CONST(1000), 0, &DUP_FITTING_CAMERA_POSITION, &DUP_FITTING_CAMERA_UP,
                                       &DUP_FITTING_CAMERA_TARGET, work->heapId);
    GFL_G3DCameraFlush(work->camera);
    gfxSetEdgeColorTable(DUP_FITTING_EDGE_COLORS);
    G3X_EdgeMarking(FALSE);
    GFL_G3DSysSetSwapBufferParams(0, 0);
    GFL_BGSysCreate(work->heapId);
    BmpWin_InitAllocator(work->heapId);
    GFL_BGSysSetLCDConfig(&DUP_FITTING_LCD_CONFIG);
    GFL_BGSysSet3DBGPriority(1);
    DupFitting_InitBG(&DUP_FITTING_BG1_SETUP, 1);
    DupFitting_InitBG(&DUP_FITTING_BG2_SETUP, 2);
    DupFitting_InitBG(&DUP_FITTING_BG3_SETUP, 3);
    DupFitting_InitBG(&DUP_FITTING_BG7_SETUP, 7);
    DupFitting_InitBG(&DUP_FITTING_BG4_SETUP, 4);
    DupFitting_InitBG(&DUP_FITTING_BG5_SETUP, 5);
    DupFitting_InitBG(&DUP_FITTING_BG6_SETUP, 6);
    GFL_BGSysSetBGEnabled(0, TRUE);
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, 0, 0xf, 4, 16);
    clactSetup = data_02093f08;
    clactSetup.oamStartMain = 16;
    clactSetup.oamCountMain = 112;
    ClActSys_Create(&clactSetup, &DUP_FITTING_VRAM_CONFIG, work->heapId);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    work->clactUnit = func_0204bf1c(24, 0, work->heapId);
    func_0204c028(work->clactUnit);
    {
        BlActSceneSetup blactSetup = { 0x200, 0xa0, { FX32_ONE, FX32_ONE, 0 }, 0, 0, 0, 0, 0x3f, BLACT_ORIGIN_CENTER };

        work->blact = BlActScene_Create(&blactSetup, work->heapId);
    }
}

static void DupFitting_LoadResources(DupFittingWork *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_MUSICAL_SHOT, work->heapId);
    ClActorSetup setup;

    GFL_G2DIOLoadArcNCLRDefault(arc, 4, PALTYPE_MAIN_BG, 0, 0, work->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 7, 3, 0, 0, FALSE, work->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 12, 3, 0, 0, FALSE, work->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 11, 2, 0, 0, FALSE, work->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 3, PALTYPE_SUB_BG, 0, 0, work->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 10, 4, 0, 0, FALSE, work->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 9, 7, 0, 0, FALSE, work->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 16, 4, 0, 0, FALSE, work->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 15, 6, 0, 0, FALSE, work->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 14, 5, 0, 0, FALSE, work->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 13, 7, 0, 0, FALSE, work->heapId);
    GFL_BGSysMoveBGReq(6, BG_MOVE_SET_X, 128);
    GFL_BGSysMoveBGReq(5, BG_MOVE_SET_X, -128);
    work->clactRes[0] = func_0204bba0(arc, 2, CLACT_VRAM_MAIN, 0, work->heapId);
    work->clactRes[3] = func_0204b81c(arc, 8, FALSE, CLACT_VRAM_MAIN, work->heapId);
    work->clactRes[6] = func_0204bde0(arc, 20, 24, work->heapId);
    work->clactRes[2] = func_0204bba0(arc, 1, CLACT_VRAM_SUB, 0, work->heapId);
    work->clactRes[5] = func_0204b81c(arc, 6, FALSE, CLACT_VRAM_SUB, work->heapId);
    work->clactRes[8] = func_0204bde0(arc, 19, 23, work->heapId);
    work->clactRes[1] = func_0204bba0(arc, 0, CLACT_VRAM_MAIN, 0x40, work->heapId);
    work->clactRes[4] = func_0204b81c(arc, 5, FALSE, CLACT_VRAM_MAIN, work->heapId);
    work->clactRes[7] = func_0204bde0(arc, 17, 21, work->heapId);
    setup.x = 32;
    setup.y = 224;
    setup.sequence = 0;
    setup.priority = 0;
    setup.bgPriority = 2;
    work->buttons[0] = func_0204c040(work->clactUnit, work->clactRes[3], work->clactRes[0], work->clactRes[6], &setup,
                                     CLACT_SURFACE_MAIN, work->heapId);
    setup.x = 224;
    setup.y = 224;
    setup.sequence = 1;
    work->buttons[1] = func_0204c040(work->clactUnit, work->clactRes[3], work->clactRes[0], work->clactRes[6], &setup,
                                     CLACT_SURFACE_MAIN, work->heapId);
    setup.x = 128;
    setup.y = 96;
    setup.sequence = 2;
    setup.priority = 0;
    setup.bgPriority = 0;
    work->hand = func_0204c040(work->clactUnit, work->clactRes[3], work->clactRes[0], work->clactRes[6], &setup,
                               CLACT_SURFACE_MAIN, work->heapId);
    func_0204c124(work->buttons[0], TRUE);
    func_0204c124(work->buttons[1], TRUE);
    func_0204c124(work->hand, FALSE);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadScr(3);
    GFL_BGSysLoadScr(2);
    GFL_BGSysLoadScr(7);
}

static void DupFitting_InitBG(const BGSetup *setup, u8 bg) {
    GFL_BGSysCreateBG(bg, setup, BGMODE_TEXT);
    GFL_BGSysSetBGEnabled(bg, TRUE);
    GFL_BGSysClearBG(bg);
    GFL_BGSysLoadScr(bg);
}

static void DupFitting_InitPoke(DupFittingWork *work) {
    VecFx32 *mark;
    VecFx32 pos;

    work->pokeSys = MusPokeDraw_InitSystem(work->heapId);
    MusPokeDraw_SetTexBase(work->pokeSys, 0x50000);
    MusPokeDraw_SetPlttBase(work->pokeSys, 0x3fe0);
    work->poke = MusPokeDraw_AddPoke(work->pokeSys, work->param->poke, FALSE);
    MusPokeDraw_UpdateSystem(work->pokeSys);
    MusPokeDraw_DrawSystem(work->pokeSys);
    mark = MusPokeDraw_GetMarkPos4(work->poke);
    pos.x = (0x80000 - mark->x) / 16;
    pos.y = (0xc0000 - (0x70000 - mark->y)) / 16;
    pos.z = 0x50000 / 2;
    MusPokeDraw_SetPosition(work->poke, &pos);
}

static void DupFitting_InitItems(DupFittingWork *work) {
    static const BOOL hidden = FALSE;
    MusItemData *items;
    u8 i;
    int k;
    int n;

    work->entryCount = 0;
    for (n = 0; n < DUP_FITTING_ENTRY_MAX; n++) {
        if (func_0200ad60(work->param->save, n) == TRUE) {
            work->entries[work->entryCount].itemId = n;
            work->entries[work->entryCount].used = FALSE;
            if (func_0200ad9c(work->param->save, n) == TRUE) {
                work->entries[work->entryCount].isNew = TRUE;
            } else {
                work->entries[work->entryCount].isNew = FALSE;
            }
            work->entryCount++;
        }
    }
    for (k = 0; work->entryCount + k < DUP_FITTING_ENTRY_MAX; k++) {
        work->entries[work->entryCount + k].itemId = 0xff;
        work->entries[work->entryCount + k].used = FALSE;
        // BUG: Only the first of the filler entries is flagged
#ifdef BUGFIX
        work->entries[work->entryCount + k].isNew = TRUE;
#else
        work->entries[work->entryCount].isNew = TRUE;
#endif
    }
    work->itemSys = MusItemDraw_InitSystem(work->blact, 0x98, work->heapId);
    items = MusItemDraw_GetItemData(work->itemSys);
    for (n = 0; n < work->entryCount; n++) {
        void *res;
        NNSGfdTexKey texKey;
        NNSGfdPlttKey plttKey;
        NNSGfdPlttKey dimPlttKey;
        u32 plttSize;
        int size;
        u16 *palette;
        NNSG3dResFileHeader *file;
        NNSG3dResTex *tex;
        u16 width;
        u16 height;
        u8 j;

        res = MusItemDraw_LoadTexResource(work->entries[n].itemId);
        GFL_G3DResUploadTexData(res);
        texKey = GFL_G3DResGetTexVRAMHandle(res);
        plttKey = GFL_G3DResGetPltVRAMHandle(res);
        plttSize = NNS_GfdGetPlttKeySize(plttKey);
        dimPlttKey = NNS_GfdAllocPlttVram(plttSize, FALSE, 0);
        size = MusItemData_GetTexSize(MusItemData_GetItem(items, work->entries[n].itemId));
        func_0204e4d0(size, &width, &height);
        work->entries[n].material = BlActScene_AddMaterialExistTex(work->blact, texKey, plttKey, 0, size, width, height);
        palette = GFL_G3DResGetTexPaletteData(res);
        for (j = 0; j < plttSize / 2; j++) {
            u16 color = palette[j];
            u8 b = (color & GX_RGB_B_MASK) >> GX_RGB_B_SHIFT;
            u8 r = color & GX_RGB_R_MASK;
            u8 g = (color & GX_RGB_G_MASK) >> GX_RGB_G_SHIFT;

            b >>= 3;
            r >>= 3;
            g >>= 3;
            palette[j] = GX_RGB(r, g, b);
        }
        file = GFL_G3DResGetResData(res);
        tex = GFL_G3DResGetTexData(res);
        NNS_G3DResTexSetPltGfdKey(tex, dimPlttKey);
        cp15_flushDC(file, file->fileSize);
        NNS_G3DResTexUploadPlt(tex, TRUE);
        work->entries[n].dimMaterial =
            BlActScene_AddMaterialExistTex(work->blact, texKey, dimPlttKey, 0, size, width, height);
        MusItemDraw_FreeTexResource(res);
    }
    DupFitting_SortEntries(work, DUP_FITTING_CATEGORY_ORIGINAL, 0);
    work->carouselList = DupFittingItemList_Create(work->heapId, DUP_FITTING_RING_PROPS);
    work->stageList = DupFittingItemList_Create(work->heapId, 128);
    work->wornList = DupFittingItemList_Create(work->heapId, 9);
    work->returnList = DupFittingItemList_Create(work->heapId, 4);
    for (k = 0; k < DUP_FITTING_RING_PROPS; k++) {
        VecFx32 pos = { 0, 0, 0 };
        DupFittingItemNode *node = DupFittingItemNode_Create(work->heapId, work->itemSys, &work->entries[k], &pos);

        DupFittingItemList_AddTail(work->carouselList, node);
        MusItemDraw_SetUseOffset(work->itemSys, DupFittingItemNode_GetDraw(node), FALSE);
    }
    {
        VecFx32 pos = { 0, 0, 0 };
        u16 width;
        u16 height;
        u32 size;
        void *res;

        work->shadowItemId = 0;
        size = MusItemData_GetTexSize(MusItemData_GetItem(items, work->shadowItemId));
        func_0204e4d0(size, &width, &height);
        res = MusItemDraw_LoadTexResource(work->shadowItemId);
        GFL_G3DResUploadTexData(res);
        work->shadowMaterial = BlActScene_AddMaterialExistTex(work->blact, GFL_G3DResGetTexVRAMHandle(res),
                                                              GFL_G3DResGetPltVRAMHandle(res), 0, size, width, height);
        work->shadowDraw =
            MusItemDraw_AddItemWithMaterial(work->itemSys, work->shadowItemId, work->shadowMaterial, &pos);
        MusItemDraw_SetVisible(work->itemSys, work->shadowDraw, FALSE);
        MusItemDraw_SetSize(work->itemSys, work->shadowDraw, FX32_ONE, FX32_ONE);
        MusItemDraw_SetShadowPalette(work->itemSys, work->shadowDraw, res);
        MusItemDraw_FreeTexResource(res);
    }
    work->badgeMaterial = func_0204e614(work->blact, ARCID_MUSICAL_SHOT, 25, 2, 0x22, 32, 32);
    for (i = 0; i < DUP_FITTING_RING_PROPS; i++) {
        VecFx32 pos = { 0, 0, 0 };
        u32 actor = BlActScene_AddNewActor(work->blact, work->badgeMaterial, FX32_ONE, FX32_ONE, &pos, 31, 0, 0);

        work->badgeActors[i] = actor;
        BlActScene_SetActorHidden(work->blact, actor, &hidden);
    }
    work->badgeFrame = 0;
    {
        MusicalSaveUnk1B0 *saved = func_0200ad44(work->param->save);

        for (i = 0; i < 8; i++) {
            if (saved[i].unk0 != DUP_FITTING_EQUIP_NONE) {
                DupFitting_AddStageItem(work, saved[i].itemId);
            }
        }
    }
    DupFitting_RefreshCarouselItems(work);
    work->carouselPos = work->entryCount * DUP_FITTING_RING_STEP - 0x8000;
    DupFitting_UpdateCarousel(work, 0, 0, 72, 48);
}

static void DupFitting_AddStageItem(DupFittingWork *work, u16 itemId) {
    DupFittingItemEntry *entry = NULL;
    DupFittingItemNode *node;
    VecFx32 vec;
    DupFittingPoint pos;
    u8 i;

    for (i = 0; i < work->entryCount; i++) {
        if (itemId == work->entries[i].itemId) {
            entry = &work->entries[i];
            break;
        }
    }
    if (entry != NULL) {
        DupFitting_GetRandomStagePos(work, &pos);
        vec.x = FX32_CONST(pos.x);
        vec.y = FX32_CONST(pos.y);
        vec.z = 10 * FX32_ONE - DupFittingItemList_GetCount(work->stageList) * 0x400;
        node = DupFittingItemNode_Create(work->heapId, work->itemSys, entry, &vec);
        DupFittingItemList_AddTail(work->stageList, node);
        DupFittingItemNode_SetPos(node, &pos);
        MusItemDraw_SetSize(work->itemSys, DupFittingItemNode_GetDraw(node), FX32_ONE, FX32_ONE);
        entry->used = TRUE;
    }
}

static void DupFitting_GetRandomStagePos(DupFittingWork *work, DupFittingPoint *pos) {
    do {
        pos->x = GFL_RandomLCAlt(240) + 8;
        pos->y = GFL_RandomLCAlt(128) + 32;
    } while (DupFitting_IsInEllipse(pos->x - 128, pos->y - 80, 84, 17.0f / 23.0f) == TRUE);
}

static void DupFitting_FreeItems(DupFittingWork *work) {
    DupFittingItemNode *node;
    DupFittingItemNode *next;
    int i;

    for (node = DupFittingItemList_GetHead(work->carouselList); node != NULL; node = next) {
        next = DupFittingItemNode_GetNext(node);
        DupFittingItemNode_Delete(node, work->itemSys);
    }
    for (node = DupFittingItemList_GetHead(work->stageList); node != NULL; node = next) {
        next = DupFittingItemNode_GetNext(node);
        DupFittingItemNode_Delete(node, work->itemSys);
    }
    for (node = DupFittingItemList_GetHead(work->wornList); node != NULL; node = next) {
        next = DupFittingItemNode_GetNext(node);
        DupFittingItemNode_Delete(node, work->itemSys);
    }
    for (node = DupFittingItemList_GetHead(work->returnList); node != NULL; node = next) {
        next = DupFittingItemNode_GetNext(node);
        DupFittingItemNode_Delete(node, work->itemSys);
    }
    for (i = 0; i < DUP_FITTING_RING_PROPS; i++) {
        BlActScene_ClearActorMaterial(work->blact, work->badgeActors[i]);
    }
    BlActScene_FreeMaterial(work->blact, work->badgeMaterial);
    MusItemDraw_DelItem(work->itemSys, work->shadowDraw);
    BlActScene_FreeMaterial(work->blact, work->shadowMaterial);
    for (i = 0; i < work->entryCount; i++) {
        BlActScene_FreeMaterial(work->blact, work->entries[i].material);
        BlActScene_FreeMaterial(work->blact, work->entries[i].dimMaterial);
    }
    DupFittingItemList_Free(work->carouselList);
    DupFittingItemList_Free(work->stageList);
    DupFittingItemList_Free(work->wornList);
    DupFittingItemList_Free(work->returnList);
    MusItemDraw_TermSystem(work->itemSys);
}

static void DupFitting_UpdateDress(DupFittingWork *work) {
    if (work->autoScroll == TRUE) {
        DupFitting_UpdateAutoScroll(work);
    } else {
        DupFitting_UpdateInput(work);
        DupFitting_UpdateTilt(work);
    }
    DupFitting_UpdateShadow(work);
    work->badgeFrame++;
    if (work->badgeFrame >= 90) {
        work->badgeFrame = 0;
        DupFitting_UpdateCarousel(work, work->angle, 0, 72, 48);
    }
    if (work->badgeFrame == 45) {
        DupFitting_UpdateCarousel(work, work->angle, 0, 72, 48);
    }
}

static DupFittingItemNode *DupFitting_UpdateCarousel(DupFittingWork *work, u16 angle, s16 delta, u16 radiusX,
                                                     u16 radiusY) {
    static const BOOL hidden = FALSE;
    static const BOOL shown = TRUE;
    s32 i = 0;
    DupFittingItemNode *node = DupFittingItemList_GetHead(work->carouselList);
    s32 total;
    u16 tilt;
    s32 offsetX;
    s32 offsetY;
    s16 scale;

    work->carouselPos += delta;
    if (work->carouselPos < 0) {
        work->carouselPos += work->entryCount * DUP_FITTING_RING_STEP;
    }
    total = work->entryCount * DUP_FITTING_RING_STEP;
    if (work->carouselPos >= total) {
        work->carouselPos -= total;
    }
    tilt = work->tilt;
    offsetX = (FX_SinIdx(0) - FX_SinIdx(tilt)) * 8;
    offsetY = FX_CosIdx(0) - 0x8008 - FX_CosIdx(tilt) * -8;
    if (delta > 2500 || delta < -2500) {
        if (work->sndCooldown == 0) {
            work->sndCooldown = 30;
            GFL_SndSEPlay(SEQ_SE_MSCL_03);
        }
    }
    for (; node != NULL; node = DupFittingItemNode_GetNext(node), i++) {
        MusItemDraw *draw;
        u16 a;
        s32 x;
        s32 y;
        fx16 sin;
        fx16 cos;
        DupFittingItemEntry *entry;
        DupFittingItemEntry *nextEntry;
        VecFx32 vec;
        DupFittingPoint pt;
        s32 pos;
        u16 idx;

        a = (angle + (i << 16) / DUP_FITTING_RING_PROPS) % 0x10000;
        scale = 0;
        draw = DupFittingItemNode_GetDraw(node);
        sin = FX_SinIdx(a);
        x = sin * radiusX + 128 * FX32_ONE;
        cos = FX_CosIdx(a);
        y = cos * radiusY + 80 * FX32_ONE;
        entry = DupFittingItemNode_GetEntry(node);
        nextEntry = entry;
        pos = work->carouselPos + (a + 0x8000) % 0x10000;
        while (pos < 0) {
            pos += work->entryCount * DUP_FITTING_RING_STEP;
        }
        while (pos >= work->entryCount * DUP_FITTING_RING_STEP) {
            pos -= work->entryCount * DUP_FITTING_RING_STEP;
        }
        if (work->autoScroll == FALSE || work->scrolled > (0x18000 - a) % 0x10000) {
            idx = (u16)((pos + 0x924) / DUP_FITTING_RING_STEP);
            if (idx >= work->entryCount) {
                idx -= work->entryCount;
            }
            nextEntry = work->sorted[idx];
            DupFittingItemNode_SetNextEntry(node, nextEntry);
            if (entry->itemId != nextEntry->itemId && node == work->heldNode) {
                work->heldNode = NULL;
            }
        }
        if (a >= 0x8000) {
            a = 0x10000 - a;
        }
        pt.x = FX_FX32_TO_F32(x);
        pt.y = FX_FX32_TO_F32(y);
        vec.x = x + offsetX;
        vec.y = y + offsetY;
        // The cast keeps the two constants apart, as the original has them
        vec.z = (fx32)(0x8000 - a) + 0x14000;
        MusItemDraw_SetPosition(work->itemSys, draw, &vec);
        MusItemDraw_SetRotation(work->itemSys, draw, tilt);
        DupFittingItemNode_SetPos(node, &pt);
        if (a < DUP_FITTING_SCALE_ANGLES[0]) {
            scale = FX32_ONE;
            MusItemDraw_SetSize(work->itemSys, draw, FX32_ONE, FX32_ONE);
            MusItemDraw_SetVisible(work->itemSys, draw, TRUE);
            DupFittingItemNode_SetScale(node, FX32_ONE);
        } else if (a < DUP_FITTING_SCALE_ANGLES[1]) {
            scale = ((u16)(DUP_FITTING_SCALE_ANGLES[1] - a) << 11) / 0x2e00 + 0x800;
            MusItemDraw_SetSize(work->itemSys, draw, scale, scale);
            MusItemDraw_SetVisible(work->itemSys, draw, TRUE);
            DupFittingItemNode_SetScale(node, scale);
        } else if (a < DUP_FITTING_SCALE_ANGLES[2]) {
            scale = ((u16)(DUP_FITTING_SCALE_ANGLES[2] - a) << 11) / 0xae0;
            MusItemDraw_SetSize(work->itemSys, draw, scale, scale);
            MusItemDraw_SetVisible(work->itemSys, draw, TRUE);
            DupFittingItemNode_SetScale(node, scale);
        } else {
            MusItemDraw_SetVisible(work->itemSys, draw, FALSE);
            DupFittingItemNode_SetScale(node, 0);
            scale = 0;
        }
        // The original compares the frame as a signed number here
        if (nextEntry->isNew == TRUE && (s32)work->badgeFrame < 45) {
            VecFx32 badgePos;

            badgePos.x = vec.x / 16;
            badgePos.y = (192 * FX32_ONE - (vec.y + 0x8000)) / 16;
            badgePos.z = vec.z + 192 * FX32_ONE / 512;
            BlActScene_SetActorPos(work->blact, work->badgeActors[i], &badgePos);
            func_0204ea5c(work->blact, work->badgeActors[i], &scale, &scale);
            BlActScene_SetActorHidden(work->blact, work->badgeActors[i], &shown);
        } else {
            BlActScene_SetActorHidden(work->blact, work->badgeActors[i], &hidden);
        }
    }
    return DupFittingItemList_GetHead(work->carouselList);
}

static void DupFitting_RefreshCarouselItems(DupFittingWork *work) {
    DupFittingItemNode *node;

    for (node = DupFittingItemList_GetHead(work->carouselList); node != NULL; node = DupFittingItemNode_GetNext(node)) {
        DupFittingItemEntry *entry = DupFittingItemNode_GetEntry(node);
        MusItemDraw *draw = DupFittingItemNode_GetDraw(node);

        if (entry->used == TRUE) {
            MusItemDraw_ChangeItem(work->itemSys, draw, entry->itemId, entry->dimMaterial);
        } else {
            MusItemDraw_ChangeItem(work->itemSys, draw, entry->itemId, entry->material);
        }
    }
}

static void DupFitting_UpdateInput(DupFittingWork *work) {
    // The speed under which a spin stops
    static const u16 spinStopSpeed = 128;
    BOOL ringDragged = FALSE;
    BOOL selected = FALSE;
    BOOL picked = FALSE;

    if (work->demo == FALSE) {
        work->touchStart = func_0203da48();
        work->touching = func_0203da84(&work->touchX, &work->touchY);
    }
    if (work->touchStart != FALSE || work->touching != FALSE) {
        if (work->touchY > 176) {
            work->touchY = 176;
        }
        if (work->touchY < 16) {
            work->touchY = 16;
        }
        if (work->holdState == 2 && work->heldNode != NULL) {
            DupFitting_UpdateHeldItem(work, TRUE);
        } else if (work->touchStart == TRUE) {
            picked = DupFitting_PickFromList(work, work->wornList, 3);
            if (picked == TRUE) {
                DupFittingItemList_Remove(work->wornList, work->heldNode);
                DupFittingItemList_AddTail(work->stageList, work->heldNode);
                DupFitting_UpdateHeldItem(work, FALSE);
            } else {
                selected = DupFitting_SelectCategory(work);
            }
        }
        if (picked == FALSE && selected == FALSE) {
            if ((work->heldNode == NULL && work->touchStart == TRUE) || work->holdState == 1) {
                s16 dy = work->touchY - 80;
                s16 dx = work->touchX - 128;
                BOOL inner = DupFitting_IsInEllipse(dx, dy, 28, 7.0f / 13.0f);
                BOOL outer = DupFitting_IsInEllipse(dx, dy, 68, 17.0f / 23.0f);

                if (inner == FALSE && outer == TRUE) {
                    ringDragged = TRUE;
                    DupFitting_UpdateCarouselDrag(work, dx, dy);
                } else if ((inner == FALSE && outer == FALSE) || (inner == TRUE && outer == TRUE)) {
                    if (DupFittingItemList_IsFull(work->stageList) == FALSE && work->holdState == 1 &&
                        work->heldNode != NULL && MATH_ABS(work->angleSpeed) < 364) {
                        DupFitting_PullOffCopy(work);
                    }
                }
                if (ringDragged == FALSE && work->touchStart == TRUE) {
                    DupFitting_PickFromList(work, work->stageList, 2);
                }
            }
        }
        if (work->touchStart != FALSE && work->heldNode == NULL && work->touchY > 160 && work->touchX > 96 &&
            work->touchX < 160) {
            work->state = DUP_FITTING_STATE_CONFIRM_IN;
            work->transition = 0;
            GFL_SndSEPlay(SEQ_SE_MSCL_01);
            DupFitting_ShowTouchEffect(work);
        }
    } else {
        s16 dy = work->prevTouchY - 80;
        s16 dx = work->prevTouchX - 128;
        BOOL inner = DupFitting_IsInEllipse(dx, dy, 28, 7.0f / 13.0f);
        BOOL outer = DupFitting_IsInEllipse(dx, dy, 68, 17.0f / 23.0f);

        if (work->holdState == 2 && work->heldNode != NULL) {
            if (work->hoverPos != DUP_FITTING_EQUIP_NONE) {
                DupFitting_WearItem(work);
            } else if (inner == FALSE && outer == TRUE) {
                DupFitting_StartReturn(work, FALSE);
            } else if (inner == TRUE) {
                DupFitting_ReleaseHeld(work);
            } else {
                DupFitting_RaiseHeldItem(work);
            }
        }
        work->dragging = FALSE;
        work->heldNode = NULL;
        work->holdState = 0;
        work->holdKind = 0;
        work->hoverPos = DUP_FITTING_EQUIP_NONE;
    }
    if (work->angleSpeed != 0 && ringDragged == FALSE) {
        s32 angle = work->angle + 0x10000 - work->angleSpeed;

        if (angle < 0) {
            angle += 0x10000;
        }
        if (angle >= 0x10000) {
            angle -= 0x10000;
        }
        work->angle = angle;
        DupFitting_UpdateCarousel(work, work->angle, work->angleSpeed, 72, 48);
        if (work->angleSpeed > spinStopSpeed || work->angleSpeed < -spinStopSpeed) {
            work->angleSpeed *= 0.75f;
        } else {
            work->angleSpeed = 0;
        }
    }
    work->prevTouchX = work->touchX;
    work->prevTouchY = work->touchY;
}

static void DupFitting_UpdateCarouselDrag(DupFittingWork *work, s16 x, s16 y) {
    // The tilt change per frame while dragging; the original's table has a second, unused entry
    static const u16 tiltSteps[2] = { 0x100, 0 };
    // The smallest turn of a drag that moves the ring
    static const u16 dragMinTurn = 227;
    s32 delta = func_020443d8(work->prevTouchX, work->prevTouchY * 72 / 48, work->touchX, work->touchY * 72 / 48, 128,
                              80, 72);

    work->dragAccum += MATH_ABS(delta);
    if (work->touchStart == TRUE || work->dragAccum >= 0x71c) {
        DupFittingItemNode *node = DupFittingItemList_GetHead(work->carouselList);
        BOOL hit = FALSE;

        if (work->touchStart == TRUE) {
            work->angleSpeed = 0;
        }
        work->dragAccum = 0;
        work->heldNode = NULL;
        while (node != NULL) {
            DupFittingItemEntry *entry = DupFittingItemNode_GetEntry(node);

            if (DupFittingItemNode_IsAtPoint(node, work->touchX, work->touchY) == TRUE) {
                hit = TRUE;
                if (entry->used == FALSE) {
                    work->heldNode = node;
                    work->holdState = 1;
                    node = NULL;
                }
            }
            if (work->heldNode == NULL) {
                node = DupFittingItemNode_GetNext(node);
            }
        }
        if (work->touchStart == TRUE && hit == TRUE && work->heldNode == NULL) {
            work->holdState = 1;
            GFL_SndSEPlay(SEQ_SE_BEEP);
        }
    }
    if (work->dragging == TRUE) {
        if (MATH_ABS(delta) > dragMinTurn && MATH_ABS(delta) < 0x4000) {
            s32 angle = work->angle + 0x10000 - delta;

            if (angle < 0) {
                angle += 0x10000;
            }
            if (angle >= 0x10000) {
                angle -= 0x10000;
            }
            work->angle = angle;
            DupFitting_UpdateCarousel(work, work->angle, delta, 72, 48);
            if (delta >= 0 && work->angleSpeed > 0 && delta < work->angleSpeed) {
                work->angleSpeed = (work->angleSpeed + delta) / 2;
            } else if (delta <= 0 && work->angleSpeed < 0 && delta > work->angleSpeed) {
                work->angleSpeed = (work->angleSpeed + delta) / 2;
            } else {
                work->angleSpeed = delta;
            }
            if ((work->angleSpeed > 0 && work->tilt > 0) || (work->angleSpeed < 0 && work->tilt < 0)) {
                work->tilt = 0;
            } else if (work->angleSpeed > 0 && work->tilt > -0x3000) {
                if (work->angleSpeed * 15 > work->tilt * -10) {
                    work->tilt -= tiltSteps[0];
                }
            } else if (work->angleSpeed < 0 && work->tilt < 0x3000) {
                if (work->angleSpeed * -15 > work->tilt * 10) {
                    work->tilt += tiltSteps[0];
                }
            }
        } else {
            work->angleSpeed = 0;
        }
    }
    work->dragging = TRUE;
}

static BOOL DupFitting_PickFromList(DupFittingWork *work, DupFittingItemList *list, s32 kind) {
    DupFittingItemNode *node;

    for (node = DupFittingItemList_GetHead(list); node != NULL; node = DupFittingItemNode_GetNext(node)) {
        if (DupFittingItemNode_IsAtPoint(node, work->touchX, work->touchY) == TRUE) {
            DupFittingPoint *pos = DupFittingItemNode_GetPos(node);

            work->holdState = 2;
            work->heldNode = node;
            GFL_SndSEPlay(SEQ_SE_MSCL_02);
            work->holdKind = kind;
            work->holdOrigin.x = pos->x;
            work->holdOrigin.y = pos->y;
            return TRUE;
        }
    }
    return FALSE;
}

static void DupFitting_UpdateHeldItem(DupFittingWork *work, BOOL playSound) {
    u16 distance = 12;
    VecFx32 vec = { 0, 0, FX32_CONST(70) };
    MusItemDraw *draw;
    s32 oldHover = work->hoverPos;
    DupFittingPoint target;
    DupFittingPoint touch;
    u16 rotation = 0;
    s16 scaleX;
    s16 scaleY;

    draw = DupFittingItemNode_GetDraw(work->heldNode);
    touch.x = work->touchX - 128;
    touch.y = work->touchY - 112;
    work->hoverPos = DupFitting_FindNearestEquipPos(work, draw, &touch.x, &distance);
    if (work->hoverPos != DUP_FITTING_EQUIP_NONE) {
        MusPokeDrawEquipPos *equip = MusPokeDraw_GetEquipPos(work->poke, work->hoverPos);

        target.x = (s32)FX_FX32_TO_F32(equip->info.pos.x + equip->info.offset.x) + 128;
        target.y = (s32)FX_FX32_TO_F32(equip->info.pos.y + equip->info.offset.y) + 96;
        rotation = equip->info.cellRotation;
        MusItemDraw_SetUseOffset(work->itemSys, draw, TRUE);
        if (MusItemDraw_GetItemFlag7(draw) == TRUE) {
            vec.z = 26 * FX32_ONE;
        }
        if (DupFitting_HasWornInCategory(work, work->hoverPos) == FALSE) {
            work->holdKind = 3;
            work->holdOrigin.x = target.x;
            work->holdOrigin.y = target.y;
        }
        DupFittingItemNode_SetEquipPos(work->heldNode, work->hoverPos);
        if (oldHover != work->hoverPos && playSound == TRUE) {
            GFL_SndSEPlay(SEQ_SE_MSCL_06);
        }
    } else {
        work->hoverPos = DUP_FITTING_EQUIP_NONE;
        target.x = work->touchX;
        target.y = work->touchY;
        MusItemDraw_SetUseOffset(work->itemSys, draw, FALSE);
    }
    vec.x = FX32_CONST(target.x);
    vec.y = FX32_CONST(target.y);
    MusItemDraw_SetPosition(work->itemSys, draw, &vec);
    DupFittingItemNode_SetPos(work->heldNode, &target);
    MusItemDraw_SetRotation(work->itemSys, DupFittingItemNode_GetDraw(work->heldNode), rotation);
    MusItemDraw_GetSize(work->itemSys, draw, &scaleX, &scaleY);
    if (scaleX < FX32_ONE || scaleY < FX32_ONE) {
        scaleX += 0x19a;
        scaleY += 0x19a;
        if (scaleX > FX32_ONE) {
            scaleX = FX32_ONE;
        }
        if (scaleY > FX32_ONE) {
            scaleY = FX32_ONE;
        }
        MusItemDraw_SetSize(work->itemSys, draw, scaleX, scaleY);
    }
}

static void DupFitting_PullOffCopy(DupFittingWork *work) {
    VecFx32 vec = { 0, 0, FX32_CONST(70) };
    DupFittingItemEntry *entry = DupFittingItemNode_GetEntry(work->heldNode);
    DupFittingPoint touch;
    DupFittingItemNode *node;
    MusItemDraw *srcDraw;
    MusItemDraw *newDraw;
    s16 size[2];
    DupFittingPoint *origin;

    touch.x = work->touchX;
    touch.y = work->touchY;
    vec.x = FX32_CONST(work->touchX);
    vec.y = FX32_CONST(work->touchY);
    node = DupFittingItemNode_Create(work->heapId, work->itemSys, entry, &vec);
    DupFittingItemList_AddTail(work->stageList, node);
    DupFittingItemNode_SetPos(node, &touch);
    srcDraw = DupFittingItemNode_GetDraw(work->heldNode);
    newDraw = DupFittingItemNode_GetDraw(node);
    MusItemDraw_GetSize(work->itemSys, srcDraw, &size[1], &size[0]);
    MusItemDraw_SetSize(work->itemSys, newDraw, size[1], size[0]);
    MusItemDraw_SetUseOffset(work->itemSys, newDraw, FALSE);
    origin = DupFittingItemNode_GetPos(work->heldNode);
    work->holdKind = 1;
    work->holdOrigin.x = origin->x;
    work->holdOrigin.y = origin->y;
    work->holdState = 2;
    work->heldNode = node;
    work->holdKind = 1;
    entry->used = TRUE;
    DupFitting_RefreshCarouselItems(work);
    GFL_SndSEPlay(SEQ_SE_MSCL_04);
}

static void DupFitting_RaiseHeldItem(DupFittingWork *work) {
    s32 z = 10 * FX32_ONE;
    MusItemDraw *draw = DupFittingItemNode_GetDraw(work->heldNode);
    DupFittingItemNode *node;
    VecFx32 pos;

    DupFittingItemList_Remove(work->stageList, work->heldNode);
    DupFittingItemList_AddHead(work->stageList, work->heldNode);
    MusItemDraw_SetSize(work->itemSys, draw, FX32_ONE, FX32_ONE);
    for (node = DupFittingItemList_GetHead(work->stageList); node != NULL; node = DupFittingItemNode_GetNext(node)) {
        MusItemDraw *nodeDraw = DupFittingItemNode_GetDraw(node);

        MusItemDraw_GetPosition(work->itemSys, nodeDraw, &pos);
        pos.z = z;
        MusItemDraw_SetPosition(work->itemSys, nodeDraw, &pos);
        z -= FX32_ONE / 4;
    }
}

static void DupFitting_StartReturn(DupFittingWork *work, BOOL unused) {
    DupFittingPoint touch;
    DupFittingItemEntry *entry;
    DupFittingItemNode *node;
    DupFittingItemNode *found;
    MusItemDraw *draw;

    DupFittingItemNode_GetDraw(work->heldNode);
    draw = DupFittingItemNode_GetDraw(work->heldNode);
    entry = DupFittingItemNode_GetEntry(work->heldNode);
    DupFittingItemList_Remove(work->stageList, work->heldNode);
    DupFittingItemList_AddHead(work->returnList, work->heldNode);
    VecFx32 vec = { 0, 0, FX32_CONST(70) };

    touch.x = work->touchX;
    touch.y = work->touchY;
    vec.x = FX32_CONST(work->holdOrigin.x);
    vec.y = FX32_CONST(work->holdOrigin.y);
    DupFittingItemNode_SetOrigin(work->heldNode, &touch);
    MusItemDraw_SetPosition(work->itemSys, draw, &vec);
    MusItemDraw_SetSize(work->itemSys, draw, FX32_ONE, FX32_ONE);
    DupFittingItemNode_SetScale(work->heldNode, FX32_ONE);
    found = NULL;
    for (node = DupFittingItemList_GetHead(work->carouselList); node != NULL && found == NULL;
         node = DupFittingItemNode_GetNext(node)) {
        DupFittingItemEntry *nodeEntry = DupFittingItemNode_GetEntry(node);

        if (nodeEntry->itemId == entry->itemId) {
            found = node;
        }
    }
    if (found != NULL) {
        DupFittingItemNode_SetPos(work->heldNode, DupFittingItemNode_GetPos(found));
        DupFittingItemNode_SetBaseScale(work->heldNode, DupFittingItemNode_GetScale(found));
    } else {
        DupFittingPoint shelf = { 128, 32 };

        DupFittingItemNode_SetPos(work->heldNode, &shelf);
        DupFittingItemNode_SetBaseScale(work->heldNode, 0);
    }
    DupFittingItemNode_SetFrames(work->heldNode, 10);
    entry->used = FALSE;
    DupFitting_RefreshCarouselItems(work);
    GFL_SndSEPlay(SEQ_SE_MSCL_05);
}

// Sets how deep a worn prop is drawn: its layer, and its place among the worn props
static inline void DupFitting_SetWornDepth(VecFx32 *pos, s32 z, s32 layer) {
    pos->z = z + (layer << 10);
}

static void DupFitting_WearItem(DupFittingWork *work) {
    MusItemDraw *heldDraw;
    DupFittingItemNode *node;
    s32 z;

    DupFittingItemNode_GetDraw(work->heldNode);
    heldDraw = DupFittingItemNode_GetDraw(work->heldNode);
    if (DupFitting_HasWornInCategory(work, work->hoverPos) == TRUE) {
        s32 category = func_ov210_021ef118(work->hoverPos);
        DupFittingItemNode *other;

        for (other = DupFittingItemList_GetHead(work->wornList); other != NULL; other = DupFittingItemNode_GetNext(other)) {
            if (category == func_ov210_021ef118(DupFittingItemNode_GetEquipPos(other))) {
                break;
            }
        }
        if (other != NULL) {
            VecFx32 vec = { 0, 0, FX32_CONST(70) };
            MusItemDraw *otherDraw = DupFittingItemNode_GetDraw(other);
            DupFittingPoint pt;
            s32 offset[2];
            DupFittingPoint *prevPos;
            VecFx32 nodePos;

            MusItemDraw_GetItemOffset(otherDraw, offset);
            DupFitting_GetRandomStagePos(work, &pt);
            pt.x += offset[0];
            pt.y += offset[1];
            DupFittingItemList_Remove(work->wornList, other);
            DupFittingItemList_AddHead(work->stageList, other);
            prevPos = DupFittingItemNode_GetPos(other);
            vec.x = FX32_CONST(pt.x);
            vec.y = FX32_CONST(pt.y);
            DupFittingItemNode_SetOrigin(other, prevPos);
            MusItemDraw_SetPosition(work->itemSys, otherDraw, &vec);
            MusItemDraw_SetRotation(work->itemSys, otherDraw, 0);
            DupFittingItemNode_SetPos(other, &pt);
            DupFittingItemNode_SetFrames(other, 10);
            z = 0;
            for (node = DupFittingItemList_GetHead(work->stageList); node != NULL; node = DupFittingItemNode_GetNext(node)) {
                MusItemDraw *nodeDraw = DupFittingItemNode_GetDraw(node);

                MusItemDraw_GetPosition(work->itemSys, nodeDraw, &nodePos);
                nodePos.z = z;
                MusItemDraw_SetPosition(work->itemSys, nodeDraw, &nodePos);
                z -= FX32_ONE / 4;
            }
        }
    }
    DupFittingItemList_Remove(work->stageList, work->heldNode);
    DupFittingItemList_AddHead(work->wornList, work->heldNode);
    MusItemDraw_SetSize(work->itemSys, heldDraw, FX32_ONE, FX32_ONE);
    DupFittingItemNode_SetEquipPos(work->heldNode, work->hoverPos);
    for (node = DupFittingItemList_GetHead(work->wornList), z = 0; node != NULL; node = DupFittingItemNode_GetNext(node)) {
        MusItemDraw *nodeDraw = DupFittingItemNode_GetDraw(node);
        VecFx32 nodePos;

        MusItemDraw_GetPosition(work->itemSys, nodeDraw, &nodePos);
        if (MusItemDraw_GetItemFlag7(nodeDraw) == TRUE) {
            DupFitting_SetWornDepth(&nodePos, z, 0x66);
        } else if (DupFittingItemNode_GetEquipPos(node) == 7 || DupFittingItemNode_GetEquipPos(node) == 8) {
            DupFitting_SetWornDepth(&nodePos, z, 0xde);
        } else {
            DupFitting_SetWornDepth(&nodePos, z, 0xca);
        }
        MusItemDraw_SetPosition(work->itemSys, nodeDraw, &nodePos);
        z -= FX32_ONE / 4;
    }
}

static void DupFitting_ReleaseHeld(DupFittingWork *work) {
    if (work->holdKind == 2) {
        VecFx32 vec = { 0, 0, FX32_CONST(70) };
        MusItemDraw *draw = DupFittingItemNode_GetDraw(work->heldNode);
        DupFittingPoint touch;

        touch.x = work->touchX;
        touch.y = work->touchY;
        vec.x = FX32_CONST(work->holdOrigin.x);
        vec.y = FX32_CONST(work->holdOrigin.y);
        DupFittingItemNode_SetOrigin(work->heldNode, &touch);
        MusItemDraw_SetPosition(work->itemSys, draw, &vec);
        DupFittingItemNode_SetPos(work->heldNode, &work->holdOrigin);
        DupFittingItemNode_SetFrames(work->heldNode, 10);
        DupFitting_RaiseHeldItem(work);
    } else if (work->holdKind == 3) {
        VecFx32 vec;
        MusItemDraw *draw = DupFittingItemNode_GetDraw(work->heldNode);
        s32 pos = DupFittingItemNode_GetEquipPos(work->heldNode);
        MusPokeDrawEquipPos *equip = MusPokeDraw_GetEquipPos(work->poke, pos);
        DupFittingPoint touch;
        DupFittingPoint target;
        u16 rotation;
        s32 base;

        target.x = (s32)FX_FX32_TO_F32(equip->info.pos.x + equip->info.offset.x) + 128;
        target.y = (s32)FX_FX32_TO_F32(equip->info.pos.y + equip->info.offset.y) + 96;
        vec.x = FX32_CONST(target.x);
        vec.y = FX32_CONST(target.y);
        if (MusItemDraw_GetItemFlag7(draw) == TRUE) {
            base = 0x66;
        } else if (DupFittingItemNode_GetEquipPos(work->heldNode) == 7 || DupFittingItemNode_GetEquipPos(work->heldNode) == 8) {
            base = 0xde;
        } else {
            base = 0xca;
        }
        vec.z = base << 10;
        rotation = equip->info.cellRotation;
        MusItemDraw_SetUseOffset(work->itemSys, draw, TRUE);
        MusItemDraw_SetPosition(work->itemSys, draw, &vec);
        MusItemDraw_SetRotation(work->itemSys, DupFittingItemNode_GetDraw(work->heldNode), rotation);
        DupFittingItemNode_SetPos(work->heldNode, &target);
        touch.x = work->touchX;
        touch.y = work->touchY;
        DupFittingItemNode_SetOrigin(work->heldNode, &touch);
        DupFittingItemNode_SetFrames(work->heldNode, 10);
        work->hoverPos = pos;
        DupFitting_WearItem(work);
        work->hoverPos = DUP_FITTING_EQUIP_NONE;
    } else if (work->holdKind == 1) {
        MusItemDraw *draw;
        DupFittingPoint touch;

        DupFittingItemNode_GetOrigin(work->heldNode);
        VecFx32 vec = { 0, 0, FX32_CONST(70) };
        draw = DupFittingItemNode_GetDraw(work->heldNode);
        DupFittingItemNode_SetPos(work->heldNode, &work->holdOrigin);
        DupFittingItemNode_SetFrames(work->heldNode, 10);
        touch.x = work->touchX;
        touch.y = work->touchY;
        vec.x = FX32_CONST(work->holdOrigin.x);
        vec.y = FX32_CONST(work->holdOrigin.y);
        DupFittingItemNode_SetOrigin(work->heldNode, &touch);
        MusItemDraw_SetPosition(work->itemSys, draw, &vec);
        DupFitting_StartReturn(work, TRUE);
    } else {
        DupFitting_StartReturn(work, FALSE);
    }
}

static BOOL DupFitting_SelectCategory(DupFittingWork *work) {
    u16 distance = 16;
    s32 touch[2];
    u16 index;
    s32 category = 0;
    s32 pos;

    index = DupFitting_GetRingIndex(work, 0x19249);
    touch[0] = work->touchX - 128;
    touch[1] = work->touchY - 112;
    pos = DupFitting_FindNearestEquipPos(work, NULL, touch, &distance);
    if (pos != DUP_FITTING_EQUIP_NONE) {
        category = func_ov210_021ef118(pos);
        if (category == 7) {
            category = 6;
        }
        if (category == work->category) {
            DupFitting_SortEntries(work, DUP_FITTING_CATEGORY_ORIGINAL, index);
            work->category = DUP_FITTING_CATEGORY_ORIGINAL;
        } else {
            DupFitting_SortEntries(work, category, index);
            work->category = category;
        }
        DupFitting_PrintCategory(work, work->category);
        work->autoScroll = TRUE;
        work->scrolled = 0;
        work->scrollTarget = work->angle % DUP_FITTING_RING_STEP + 0x10000;
        if (work->scrollTarget >= 0x10000 && work->scrollTarget <= 0x10002) {
            work->scrollTarget += DUP_FITTING_RING_STEP;
        }
        DupFitting_UpdateCarousel(work, work->angle, 0, 72, 48);
        GFL_SndSEPlay(SEQ_SE_MSCL_07);
        return TRUE;
    }
    return category;
}

static void DupFitting_UpdateAutoScroll(DupFittingWork *work) {
    u32 diff = work->scrollTarget - work->scrolled;
    u32 step = diff / 10;

    if (step < 0x100) {
        if (diff < 0x100) {
            step = diff;
            work->autoScroll = FALSE;
        } else {
            step = 0x100;
        }
    }
    if (diff > FX32_ONE) {
        work->tilt -= FX32_ONE / 8;
        if (work->tilt < -0x3000) {
            work->tilt = -0x3000;
        }
    } else {
        DupFitting_UpdateTilt(work);
    }
    work->scrolled += step;
    work->angle -= step;
    DupFitting_UpdateCarousel(work, work->angle, step, 72, 48);
}

static void DupFitting_UpdateTilt(DupFittingWork *work) {
    if (MATH_ABS(work->angleSpeed) < 0x30) {
        if (work->tilt != 0 || work->tiltSpeed != 0) {
            work->tilt += work->tiltSpeed;
            if (work->tilt > 0) {
                if (work->tiltSpeed > 0) {
                    work->tiltSpeed -= 0x80;
                } else {
                    work->tiltSpeed -= 0x2a;
                }
            } else if (work->tilt < 0) {
                if (work->tiltSpeed < 0) {
                    work->tiltSpeed += 0x80;
                } else {
                    work->tiltSpeed += 0x2a;
                }
            }
            if (MATH_ABS(work->tilt) < 0x200 && MATH_ABS(work->tiltSpeed) < 0x200) {
                work->tiltSpeed = 0;
                work->tilt = 0;
            }
            DupFitting_UpdateCarousel(work, work->angle, 0, 72, 48);
        }
    }
}

static BOOL DupFitting_HasWornInCategory(DupFittingWork *work, s32 pos) {
    s32 category = func_ov210_021ef118(pos);
    DupFittingItemNode *node;

    for (node = DupFittingItemList_GetHead(work->wornList); node != NULL; node = DupFittingItemNode_GetNext(node)) {
        if (category == func_ov210_021ef118(DupFittingItemNode_GetEquipPos(node))) {
            return TRUE;
        }
    }
    return FALSE;
}

static s32 DupFitting_FindNearestEquipPos(DupFittingWork *work, MusItemDraw *draw, const s32 *pos, u16 *distance) {
    u16 best = *distance * *distance;
    DupFittingEquipPos result = DUP_FITTING_EQUIP_NONE;
    DupFittingEquipPos i;

    for (i = DUP_FITTING_EQUIP_FIRST; i < DUP_FITTING_EQUIP_MAX; i++) {
        BOOL canEquip;
        MusPokeDrawEquipPos *equip;

        if (draw == NULL) {
            canEquip = TRUE;
        } else {
            canEquip = MusItemDraw_CanEquipPos(draw, i);
        }
        equip = MusPokeDraw_GetEquipPos(work->poke, i);
        if (equip->valid == TRUE && canEquip == TRUE) {
            // The position's screen coordinates (+128, +96), relative to the touch origin the callers subtract
            // (128, 112). The folded +0 is the original's `adds r0, r0, #0`
            s32 x = (s32)FX_FX32_TO_F32(equip->info.pos.x + equip->info.offset.x) + 128 - 128;
            s32 y = (s32)FX_FX32_TO_F32(equip->info.pos.y + equip->info.offset.y) + 96 - 112;
            s32 dx = pos[0] - x;
            s32 dy = pos[1] - y;
            s32 d = dx * dx + dy * dy;

            if (d < best) {
                best = d;
                result = i;
            }
        }
    }
    *distance = best;
    return result;
}

// Where a prop that moves from origin to pos is when it has the given number of its ten frames left
static inline void DupFitting_GetMovePos(VecFx32 *vec, DupFittingPoint *origin, DupFittingPoint *pos, u16 frames) {
    s32 oy;
    s32 t;
    s32 ox;
    s32 d;

    t = 10 - frames;
    ox = origin->x;
    d = pos->x - ox;
    vec->x = FX32_CONST(ox + d * t / 10);
    oy = origin->y;
    d = pos->y - oy;
    vec->y = FX32_CONST(oy + d * t / 10);
}

static void DupFitting_UpdateReturning(DupFittingWork *work) {
    DupFittingItemNode *node;
    u16 frames;
    DupFittingItemNode *cur;
    MusItemDraw *draw;
    DupFittingPoint *origin;
    DupFittingPoint *pos;
    s16 baseScale;
    BOOL secondList;
    u16 next;
    DupFittingPoint *from;

    node = DupFittingItemList_GetHead(work->returnList);
    while (node != NULL) {
        cur = node;
        draw = DupFittingItemNode_GetDraw(node);
        frames = DupFittingItemNode_GetFrames(node);
        origin = DupFittingItemNode_GetOrigin(node);
        pos = DupFittingItemNode_GetPos(node);
        baseScale = DupFittingItemNode_GetBaseScale(node);
        node = DupFittingItemNode_GetNext(node);
        frames--;
        if (frames != 0) {
            s16 scale = baseScale + (FX32_ONE - baseScale) * frames / 10;
            VecFx32 vec;

            MusItemDraw_SetSize(work->itemSys, draw, scale, scale);
            MusItemDraw_GetPosition(work->itemSys, draw, &vec);
            DupFitting_GetMovePos(&vec, origin, pos, (s32)frames);
            vec.z = FX32_CONST(70);
            MusItemDraw_SetPosition(work->itemSys, draw, &vec);
            DupFittingItemNode_SetFrames(cur, frames);
        } else {
            DupFittingItemList_Remove(work->returnList, cur);
            DupFittingItemNode_Delete(cur, work->itemSys);
        }
    }
    secondList = FALSE;
    node = DupFittingItemList_GetHead(work->stageList);
    if (node == NULL) {
        secondList = TRUE;
        node = DupFittingItemList_GetHead(work->wornList);
    }
    while (node != NULL) {
        frames = DupFittingItemNode_GetFrames(node);
        if (frames != 0) {
            draw = DupFittingItemNode_GetDraw(node);
            from = DupFittingItemNode_GetOrigin(node);
            pos = DupFittingItemNode_GetPos(node);
            next = frames - 1;
            if (next != 0) {
                VecFx32 vec;

                MusItemDraw_GetPosition(work->itemSys, draw, &vec);
                DupFitting_GetMovePos(&vec, from, pos, next);
                MusItemDraw_SetPosition(work->itemSys, draw, &vec);
            } else {
                VecFx32 vec;

                MusItemDraw_GetPosition(work->itemSys, draw, &vec);
                vec.x = FX32_CONST(pos->x);
                vec.y = FX32_CONST(pos->y);
                MusItemDraw_SetPosition(work->itemSys, draw, &vec);
                DupFittingItemNode_SetOrigin(node, pos);
            }
            DupFittingItemNode_SetFrames(node, next);
        }
        node = DupFittingItemNode_GetNext(node);
        if (node == NULL && secondList == FALSE) {
            secondList = TRUE;
            node = DupFittingItemList_GetHead(work->wornList);
        }
    }
}

static void DupFitting_UpdateShadow(DupFittingWork *work) {
    if (work->heldNode == NULL) {
        MusItemDraw_SetVisible(work->itemSys, work->shadowDraw, FALSE);
        return;
    }
    if (work->shadowItemId == DupFittingItemNode_GetEntry(work->heldNode)->itemId) {
        MusItemDraw_SetVisible(work->itemSys, work->shadowDraw, TRUE);
        MusItemDraw_CopyShadowPos(work->itemSys, DupFittingItemNode_GetDraw(work->heldNode), work->shadowDraw);
    }
}

static BOOL DupFitting_IsInEllipse(s16 x, s16 y, int radius, f32 xScale) {
    s16 scaledX = x * xScale;

    if (scaledX * scaledX + y * y < radius * radius) {
        return TRUE;
    }
    return FALSE;
}

static void DupFitting_SortEntries(DupFittingWork *work, s32 category, u16 start) {
    u16 i;
    u16 j;
    u16 first;
    u16 best;

    for (i = 0; i < work->entryCount; i++) {
        work->sorted[(u16)((start + i) % work->entryCount)] = &work->entries[i];
    }
    for (i = 0; i < work->entryCount; i++) {
        u16 key;

        first = (start + i) % work->entryCount;
        best = first;
        key = DupFitting_GetSortKey(work, work->sorted[first], category);

        for (j = i + 1; j < work->entryCount; j++) {
            u16 index = (start + j) % work->entryCount;
            u16 other = DupFitting_GetSortKey(work, work->sorted[index], category);

            if (key < other) {
                key = other;
                best = index;
            }
        }
        if (best != first) {
            DupFittingItemEntry *tmp = work->sorted[first];

            work->sorted[first] = work->sorted[best];
            work->sorted[best] = tmp;
        }
    }
}

static u16 DupFitting_GetSortKey(DupFittingWork *work, DupFittingItemEntry *entry, s32 pos) {
    BOOL ofPos = MusItemDraw_IsItemOfPos(work->itemSys, entry->itemId, pos);
    BOOL canEquip = MusItemDraw_ItemCanEquipPos(work->itemSys, entry->itemId, pos);
    s32 key;

    if (ofPos == TRUE) {
        if (entry->isNew == TRUE) {
            key = 6000;
        } else {
            key = 5000;
        }
    } else if (canEquip == TRUE) {
        if (entry->isNew == TRUE) {
            key = 4000;
        } else {
            key = 3000;
        }
    } else if (entry->isNew == TRUE) {
        key = 2000;
    } else {
        key = 1000;
    }
    return key + (999 - entry->itemId);
}

static void DupFitting_UpdateTransition(DupFittingWork *work) {
    DupFittingItemNode *node;
    ClActorPos pos;
    s32 y;
    s32 t = work->transition;

    DupFitting_UpdateCarousel(work, work->angle, 0, t + 72, t * (2.0f / 3.0f) + 48.0f);
    for (node = DupFittingItemList_GetHead(work->stageList); node != NULL; node = DupFittingItemNode_GetNext(node)) {
        DupFittingPoint *nodePos = DupFittingItemNode_GetPos(node);
        MusItemDraw *draw = DupFittingItemNode_GetDraw(node);
        s16 dx = nodePos->x - 128;
        s16 dy = nodePos->y - 80;
        fx32 dist = FX_Sqrt(FX32_CONST(dx * dx + dy * dy));
        u16 angle = FX_Atan2Idx(FX32_CONST(dy), FX32_CONST(dx));
        VecFx32 vec;
        fx32 radius;

        MusItemDraw_GetPosition(work->itemSys, draw, &vec);
        radius = dist + FX32_CONST(work->transition);
        vec.x = FX_Mul(FX_CosIdx(angle), radius) + 128 * FX32_ONE;
        radius = dist + FX32_CONST(work->transition * (2.0f / 3.0f));
        vec.y = FX_Mul(FX_SinIdx(angle), radius) + 80 * FX32_ONE;
        MusItemDraw_SetPosition(work->itemSys, draw, &vec);
    }
    GFL_BGSysMoveBG(2, BG_MOVE_SET_Y, -work->transition);
    pos.x = 32;
    y = 192 - work->transition;
    if (y >= 64) {
        y = 64;
    }
    pos.y = y + 160;
    func_0204c140(work->buttons[0], &pos, CLACT_SURFACE_MAIN);
    pos.x = 224;
    y = 192 - work->transition;
    if (y >= 64) {
        y = 64;
    }
    pos.y = y + 160;
    func_0204c140(work->buttons[1], &pos, CLACT_SURFACE_MAIN);
}

static u32 DupFitting_UpdateConfirm(DupFittingWork *work) {
    TouchRect rects[3] = {
        { 136, 192, 0, 48 },
        { 136, 192, 216, 255 },
        { TOUCH_RECT_END, 0, 0, 0 },
    };
    s32 hit;

    func_0203da84(&work->touchX, &work->touchY);
    hit = func_0203da0c(rects);
    switch (hit) {
    case 0:
        DupFitting_SaveWornItems(work);
        GFL_SndSEPlay(SEQ_SE_DECIDE3);
        DupFitting_ShowTouchEffect(work);
        return 2;
    case 1: {
        WordSet *wordSet;

        work->state = DUP_FITTING_STATE_CONFIRM_OUT;
        work->transition = 192;
        GFL_SndSEPlay(SEQ_SE_MSCL_21);
        DupFitting_ResetRotations(work);
        wordSet = GFL_WordSetSystemCreateDefault(work->heapId);
        loadPokemonNicknameToStrbuf(wordSet, 0, work->param->poke->pkm);
        DupFitting_PrintMessage(work, Title3_Text_LetsDressUpProps, wordSet);
        GFL_WordSetSystemFree(wordSet);
        DupFitting_ShowTouchEffect(work);
        break;
    }
    case TOUCH_RECT_NONE:
        DupFitting_UpdateConfirmTouch(work);
        break;
    }
    DupFitting_UpdateShadow(work);
    return 0;
}

static void DupFitting_ResetRotations(DupFittingWork *work) {
    DupFittingItemNode *node;
    u8 i;

    for (i = 0; i < 9; i++) {
        work->param->poke->equips[i].unk2 = 0;
    }
    for (node = DupFittingItemList_GetHead(work->wornList); node != NULL; node = DupFittingItemNode_GetNext(node)) {
        u16 pos = DupFittingItemNode_GetEquipPos(node);
        MusPokeDrawEquipPos *equip = MusPokeDraw_GetEquipPos(work->poke, pos);

        MusItemDraw_SetRotation(work->itemSys, DupFittingItemNode_GetDraw(node), equip->info.cellRotation);
    }
}

static void DupFitting_UpdateConfirmTouch(DupFittingWork *work) {
    work->prevTouchX = work->touchX;
    work->prevTouchY = work->touchY;
    work->touchStart = func_0203da48();
    work->touching = func_0203da84(&work->touchX, &work->touchY);
    if (work->touchStart == TRUE) {
        DupFittingItemNode *best = NULL;
        u32 bestDistance = 256;
        DupFittingItemNode *node;

        for (node = DupFittingItemList_GetHead(work->wornList); node != NULL; node = DupFittingItemNode_GetNext(node)) {
            u32 distance = DupFittingItemNode_GetDistanceSq(node, work->touchX, work->touchY);

            if (bestDistance > distance) {
                best = node;
                bestDistance = distance;
            }
        }
        if (best != NULL) {
            work->heldNode = best;
        }
    }
    if (work->touching == TRUE && work->heldNode != NULL) {
        DupFitting_RotateHeld(work);
    }
    if (work->touching == FALSE) {
        work->dragging = FALSE;
        work->heldNode = NULL;
    }
}

static void DupFitting_RotateHeld(DupFittingWork *work) {
    u16 pos = DupFittingItemNode_GetEquipPos(work->heldNode);
    MusPokeDrawEquipPos *equip = MusPokeDraw_GetEquipPos(work->poke, pos);
    DupFittingPoint *nodePos;
    u16 angle;
    s32 x;
    s32 y;
    fx32 fx;
    fx32 fy;

    DupFittingItemNode_GetDraw(work->heldNode);
    nodePos = DupFittingItemNode_GetPos(work->heldNode);
    x = nodePos->x;
    y = nodePos->y;
    fx = FX32_CONST((s32)(work->touchX - x));
    fy = FX32_CONST((s32)(work->touchY - y));
    angle = FX_Atan2Idx(fx, fy);
    if (work->dragging == TRUE) {
        s16 oldRotation = work->param->poke->equips[pos].unk2;
        s16 rotation = oldRotation + (s16)(work->prevAngle - angle);

        if (rotation < -0xaaa) {
            rotation = -0xaaa;
        }
        if (rotation > 0xaaa) {
            rotation = 0xaaa;
        }
        work->rotateAccum += (s16)(oldRotation - rotation);
        if (MATH_ABS(work->rotateAccum) > 0x71c) {
            work->rotateAccum = 0;
            GFL_SndSEPlay(SEQ_SE_MSCL_08);
        }
        MusItemDraw_SetRotation(work->itemSys, DupFittingItemNode_GetDraw(work->heldNode),
                                equip->info.cellRotation + rotation);
        work->param->poke->equips[pos].unk2 = rotation;
    }
    work->prevAngle = angle;
    work->dragging = TRUE;
}

static void DupFitting_SaveWornItems(DupFittingWork *work) {
    MusicalSaveUnk1B0 *saved = func_0200ad44(work->param->save);
    DupFittingItemNode *node;
    u8 i = 0;

    func_0200ad20(work->param->save);
    for (node = DupFittingItemList_GetHead(work->wornList); node != NULL && i < 8; node = DupFittingItemNode_GetNext(node), i++) {
        u16 pos = DupFittingItemNode_GetEquipPos(node);

        MusPokeDraw_GetEquipPos(work->poke, pos);
        saved[i].unk0 = pos;
        saved[i].itemId = DupFittingItemNode_GetEntry(node)->itemId;
        saved[i].rotation = work->param->poke->equips[pos].unk2;
        work->param->poke->equips[pos].itemId = DupFittingItemNode_GetEntry(node)->itemId;
        work->param->poke->equips[pos].slot = i;
    }
}

static void DupFitting_UpdateButtons(DupFittingWork *work) {
    ClActorPos pos;
    u8 palette;

    pos.x = 32;
    pos.y = work->confirmTimer * 2 + 160;
    if (pos.y >= 224) {
        pos.y = 224;
    }
    func_0204c140(work->buttons[0], &pos, CLACT_SURFACE_MAIN);
    pos.x = 224;
    pos.y = work->confirmTimer * 2 + 160;
    if (pos.y >= 224) {
        pos.y = 224;
    }
    func_0204c140(work->buttons[1], &pos, CLACT_SURFACE_MAIN);
    palette = 24 * work->confirmTimer / 60 + 4;
    if (palette >= 11) {
        palette = 3;
    }
    GFL_BGSysSetScrPaletteNo(2, 10, 44, 12, 12, palette);
    GFL_BGSysQueueScrLoad(2);
}

static void DupFitting_StartDemo(DupFittingWork *work) {
    u16 i;

    work->demo = TRUE;
    work->demoFrame = 0;
    work->demoStage = 1;
    i = 5;
    do {
        if (work->sorted[i]->used != TRUE) {
            break;
        }
        i++;
        if (i == work->entryCount) {
            i = 0;
        }
    } while (i != 5);
    if (i == 0) {
        work->demoItem = work->entryCount - 1;
    } else {
        work->demoItem = i - 1;
    }
    work->state = DUP_FITTING_STATE_DEMO;
}

static void DupFitting_EndDemo(DupFittingWork *work) {
    work->demo = FALSE;
    work->state = DUP_FITTING_STATE_DRESS;
    func_0204c124(work->hand, FALSE);
    DupFitting_PrintPokeName(work);
}

static void DupFitting_UpdateDemo(DupFittingWork *work) {
    switch (work->demoStage) {
    case 1: {
        u8 item = work->demoItem;

        DupFitting_DemoRotate(work, item * DUP_FITTING_RING_STEP, item * DUP_FITTING_RING_STEP, item * 15);
        break;
    }
    case 2:
        DupFitting_DemoWait(work, 30);
        break;
    case 3: {
        u8 item = work->demoItem;
        u16 target = item * DUP_FITTING_RING_STEP;

        DupFitting_DemoRotate(work, -target, -(item * DUP_FITTING_RING_STEP), item * 15);
        break;
    }
    case 4:
        DupFitting_DemoWait(work, 30);
        break;
    case 5: {
        DupFittingPoint from = { 162, 120 };
        DupFittingPoint to = { 192, 160 };

        DupFitting_DemoMoveHand(work, &from, &to, 60);
        break;
    }
    case 6:
        DupFitting_DemoWait(work, 30);
        break;
    case 7: {
        DupFittingPoint from = { 192, 160 };
        DupFittingPoint to = { 162, 120 };

        DupFitting_DemoMoveHand(work, &from, &to, 60);
        break;
    }
    case 8:
        DupFitting_DemoWait(work, 30);
        break;
    case 9:
        DupFitting_EndDemo(work);
        return;
    }
    {
        ClActorPos pos;

        pos.x = work->touchX;
        pos.y = work->touchY;
        func_0204c140(work->hand, &pos, CLACT_SURFACE_MAIN);
    }
    DupFitting_UpdateDress(work);
}

static void DupFitting_DemoRotate(DupFittingWork *work, u16 target, s32 total, u16 frames) {
    s32 step;
    u16 angle;

    work->touching = FALSE;
    work->touchStart = FALSE;
    func_0204c124(work->hand, TRUE);
    step = total / frames;
    angle = (target - step * work->demoFrame + 0x10000) % 0x10000;
    work->touchX = FX_FX32_TO_F32(72 * FX_SinIdx(angle) + 128 * FX32_ONE);
    work->touchY = FX_FX32_TO_F32(48 * FX_CosIdx(angle) + 80 * FX32_ONE);
    work->angle -= (u16)step;
    DupFitting_UpdateCarousel(work, work->angle, step, 72, 48);
    work->demoFrame++;
    if (work->demoFrame > frames) {
        work->demoFrame = 0;
        work->demoStage++;
    }
}

static void DupFitting_DemoWait(DupFittingWork *work, u16 frames) {
    work->touching = FALSE;
    work->touchStart = FALSE;
    work->demoFrame++;
    func_0204c124(work->hand, FALSE);
    if (work->demoFrame > frames) {
        work->demoFrame = 0;
        work->demoStage++;
    }
}

static void DupFitting_DemoMoveHand(DupFittingWork *work, const DupFittingPoint *from, const DupFittingPoint *to,
                                    u16 frames) {
    s32 y;

    work->touching = TRUE;
    if (work->demoFrame == 0) {
        work->touchStart = TRUE;
    } else {
        work->touchStart = FALSE;
    }
    y = from->y + (to->y - from->y) * work->demoFrame / frames;
    work->touchX = from->x + (to->x - from->x) * work->demoFrame / frames;
    work->touchY = y;
    func_0204c124(work->hand, TRUE);
    work->demoFrame++;
    if (work->demoFrame > frames) {
        work->demoFrame = 0;
        work->demoStage++;
    }
}

static void DupFitting_InitMessages(DupFittingWork *work) {
    work->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, work->heapId);
    work->dressMsg = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_3, work->heapId);
    work->itemMsg = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_13, work->heapId);
    work->categoryMsg =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE_S005D_WORD_SET_MUSICAL_INFO, work->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 0x1c0, 0x20, work->heapId);
    work->printQueue = func_02021998(work->heapId);
    LoadSysMsgBox(7, 0x80, 12, 0, work->heapId);
}

static void DupFitting_FreeMessages(DupFittingWork *work) {
    func_02021a18(work->printQueue);
    GFL_MsgDataFree(work->itemMsg);
    GFL_MsgDataFree(work->categoryMsg);
    GFL_MsgDataFree(work->dressMsg);
    GFL_FontFree(work->font);
}

static void DupFitting_PrintMessage(DupFittingWork *work, u32 messageId, WordSet *wordSet) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(work->dressMsg, messageId);

    func_02021c44(work->printQueue);
    GFL_BitmapFill(BmpWin_GetBitmap(work->bmpWin), 15);
    if (wordSet != NULL) {
        StrBuf *formatted = GFL_StrBufCreate(128, work->heapId);

        GFL_WordSetFormatStrbuf(wordSet, formatted, str);
        GFL_StrBufFree(str);
        str = formatted;
    }
    func_02021c7c(work->printQueue, BmpWin_GetBitmap(work->bmpWin), 2, 2, str, work->font, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(str);
    work->printing = TRUE;
}

static void DupFitting_PrintPokeName(DupFittingWork *work) {
    WordSet *wordSet = GFL_WordSetSystemCreateDefault(work->heapId);

    loadPokemonNicknameToStrbuf(wordSet, 0, work->param->poke->pkm);
    DupFitting_PrintMessage(work, Title3_Text_LetsDressUpProps, wordSet);
    GFL_WordSetSystemFree(wordSet);
}

static void DupFitting_PrintItemName(DupFittingWork *work, DupFittingItemNode *node) {
    MusItemData *items = MusItemDraw_GetItemData(work->itemSys);
    DupFittingItemEntry *entry = DupFittingItemNode_GetEntry(node);
    s32 category = MusItemData_GetCategory(items, entry->itemId);
    WordSet *wordSet = GFL_WordSetSystemCreateDefault(work->heapId);
    StrBuf *itemName = GFL_MsgDataLoadStrbufNew(work->itemMsg, entry->itemId + Title13_Text_PinkBarrette);
    StrBuf *categoryName;

    category += TitleS005dWordSetMusicalInfo_Text_Ear;
    categoryName = GFL_MsgDataLoadStrbufNew(work->categoryMsg, category);
    func_0202437c(wordSet, 0, itemName, 0, 1, 2);
    func_0202437c(wordSet, 1, categoryName, 0, 1, 2);
    DupFitting_PrintMessage(work, Title3_Text_PropLooksNice, wordSet);
    GFL_StrBufFree(categoryName);
    GFL_StrBufFree(itemName);
    GFL_WordSetSystemFree(wordSet);
}

static void DupFitting_PrintCategory(DupFittingWork *work, s32 category) {
    u32 messageId;
    WordSet *wordSet;
    StrBuf *name;

    if (category == DUP_FITTING_CATEGORY_ORIGINAL) {
        DupFitting_PrintMessage(work, Title3_Text_OrderPropsRestoredOriginal, NULL);
        return;
    }
    messageId = 0;
    switch (category) {
    case 0:
    case 1:
        messageId = TitleS005dWordSetMusicalInfo_Text_Ear;
        break;
    case 2:
        messageId = TitleS005dWordSetMusicalInfo_Text_Head;
        break;
    case 3:
        messageId = TitleS005dWordSetMusicalInfo_Text_Face_3;
        break;
    case 4:
        messageId = TitleS005dWordSetMusicalInfo_Text_Body;
        break;
    case 5:
        messageId = TitleS005dWordSetMusicalInfo_Text_Waist;
        break;
    case 6:
    case 7:
        messageId = TitleS005dWordSetMusicalInfo_Text_Arm;
        break;
    }
    wordSet = GFL_WordSetSystemCreateDefault(work->heapId);
    name = GFL_MsgDataLoadStrbufNew(work->categoryMsg, messageId);
    func_0202437c(wordSet, 0, name, 0, 1, 2);
    DupFitting_PrintMessage(work, Title3_Text_PropsLookNiceWere, wordSet);
    GFL_StrBufFree(name);
    GFL_WordSetSystemFree(wordSet);
}

static void DupFitting_InitSparkles(DupFittingWork *work) {
    ClActorSetup sparkle;
    ClActorSetup setup;
    u8 i;

    for (i = 0; i < 8; i++) {
        sparkle.x = 0;
        sparkle.y = 0;
        sparkle.sequence = 0;
        sparkle.priority = 0;
        sparkle.bgPriority = 0;
        work->sparkles[i] = func_0204c040(work->clactUnit, work->clactRes[2], work->clactRes[5], work->clactRes[8],
                                          &sparkle, CLACT_SURFACE_SUB, work->heapId);
        func_0204c124(work->sparkles[i], TRUE);
        func_0204c520(work->sparkles[i], TRUE);
        func_0204c318(work->sparkles[i], 0);
        work->sparkleTimer[i] = 0;
    }
    setup.x = 128;
    setup.y = 96;
    setup.sequence = 0;
    setup.priority = 0;
    setup.bgPriority = 0;
    work->okMark = func_0204c040(work->clactUnit, work->clactRes[1], work->clactRes[4], work->clactRes[7], &setup,
                                 CLACT_SURFACE_MAIN, work->heapId);
    func_0204c124(work->okMark, FALSE);
    func_0204c520(work->okMark, TRUE);
    setup.sequence = 2;
    work->touchEffect = func_0204c040(work->clactUnit, work->clactRes[1], work->clactRes[4], work->clactRes[7],
                                      &setup, CLACT_SURFACE_MAIN, work->heapId);
    func_0204c124(work->touchEffect, FALSE);
    func_0204c520(work->touchEffect, TRUE);
}

static void DupFitting_FreeSparkles(DupFittingWork *work) {
    u8 i;

    for (i = 0; i < 8; i++) {
        func_0204c108(work->sparkles[i]);
    }
    func_0204c108(work->touchEffect);
    func_0204c108(work->okMark);
}

static void DupFitting_UpdateSparkles(DupFittingWork *work) {
    ClActorPos pos;
    u8 i;

    for (i = 0; i < 8; i++) {
        func_0204c468(work->sparkles[i], GFL_BGSysGetBGPriority(4));
        func_0204c318(work->sparkles[i], 0);
        if (work->sparkleTimer[i] == 0) {
            pos.x = GFL_RandomMTRange(56);
            if (GFL_RandomMTRange(2) == 1) {
                pos.x = 256 - pos.x;
            }
            pos.y = GFL_RandomMTRange(192);
            func_0204c140(work->sparkles[i], &pos, CLACT_SURFACE_SUB);
            if (pos.y > 90) {
                func_0204c488(work->sparkles[i], GFL_RandomMTRange(2));
            } else {
                func_0204c488(work->sparkles[i], GFL_RandomMTRange(2) + 2);
            }
            work->sparkleTimer[i] = GFL_RandomMTRange(200) + 400;
        } else {
            work->sparkleTimer[i]--;
        }
    }
    if (func_0204c138(work->touchEffect) == TRUE && func_0204c560(work->touchEffect) == FALSE) {
        func_0204c124(work->touchEffect, FALSE);
    }
}

static void DupFitting_ShowTouchEffect(DupFittingWork *work) {
    ClActorPos pos;

    pos.x = work->touchX;
    pos.y = work->touchY;
    func_0204c56c(work->touchEffect);
    func_0204c124(work->touchEffect, TRUE);
    func_0204c140(work->touchEffect, &pos, CLACT_SURFACE_MAIN);
}

static void DupFitting_Trace(DupFittingWork *work, u8 id) {
}
