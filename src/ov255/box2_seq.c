#include "types.h"
#include "app/box2_bgwfrm.h"
#include "app/box2_bmp.h"
#include "app/box2_main.h"
#include "app/box2_obj.h"
#include "app/box2_seq.h"
#include "app/box2_ui.h"
#include "app/ui/ui_scene.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/math.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/box.h"
#include "system/app_taskmenu.h"
#include "system/bgwinfrm.h"
#include "system/cursor_move.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/wipe.h"

// The PC box's sequences: one function per state of the box, run each frame by Box2Seq_Main, each returning the
// next state. Names are ours

typedef int (*Box2SeqFunc)(Box2SysWork *syswk);

// What the yes/no menu's answers run, by the question asked
typedef struct {
    Box2SeqFunc yes;
    Box2SeqFunc no;
} Box2YesNoFuncs;

// The sub procs, with the state after each
typedef struct {
    Box2SeqFunc call;
    Box2SeqFunc exit;
    int nextSeq;
} Box2SubProc;

static int Box2Seq_Init(Box2SysWork *syswk);
static int Box2Seq_Release(Box2SysWork *syswk);
static int Box2Seq_Wipe(Box2SysWork *syswk);
static int Box2Seq_PaletteFade(Box2SysWork *syswk);
static int Box2Seq_Wait(Box2SysWork *syswk);
static int Box2Seq_VFunc(Box2SysWork *syswk);
static int Box2Seq_TrgWait(Box2SysWork *syswk);
static int Box2Seq_YesNo(Box2SysWork *syswk);
static int Box2Seq_ButtonAnm(Box2SysWork *syswk);
static int Box2Seq_SubProcCall(Box2SysWork *syswk);
static int Box2Seq_SubProcMain(Box2SysWork *syswk);
static int Box2Seq_Start(Box2SysWork *syswk);
static int Box2Seq_StartWait(Box2SysWork *syswk);
static int Box2Seq_PokeMoveEnd(Box2SysWork *syswk);
static int Box2Seq_OpenMenu(Box2SysWork *syswk);
static int Box2Seq_BoxListScrollTouch(Box2SysWork *syswk);
static int Box2Seq_LeaveState(Box2SysWork *syswk);
static int Box2Seq_ArrangeMain(Box2SysWork *syswk);
static int Box2Seq_ArrangeMoveToBoxList(Box2SysWork *syswk);
static int Box2Seq_ArrangeMenuClose(Box2SysWork *syswk);
static int Box2Seq_ArrangeMoveStart(Box2SysWork *syswk);
static int Box2Seq_ArrangeMoveMain(Box2SysWork *syswk);
static int Box2Seq_ArrangeMovePartyStart(Box2SysWork *syswk);
static int Box2Seq_ArrangeMoveEnd(Box2SysWork *syswk);
static int Box2Seq_ArrangeMoveErrClose(Box2SysWork *syswk);
static int Box2Seq_ArrangeMovePartyOut(Box2SysWork *syswk);
static int Box2Seq_RangeStart(Box2SysWork *syswk);
static int Box2Seq_RangeMain(Box2SysWork *syswk);
static int Box2Seq_RangePartyOut(Box2SysWork *syswk);
static int Box2Seq_RangeMoveEnd(Box2SysWork *syswk);
static int Box2Seq_RangeMoveErrClose(Box2SysWork *syswk);
static int Box2Seq_RangePartyAway(Box2SysWork *syswk);
static int Box2Seq_RangePartyIn(Box2SysWork *syswk);
static int Box2Seq_RangePartyMain(Box2SysWork *syswk);
static int Box2Seq_RangePartySwitch(Box2SysWork *syswk);
static int Box2Seq_RangePartyClose(Box2SysWork *syswk);
static int Box2Seq_ArrangePartyOut(Box2SysWork *syswk);
static int Box2Seq_ArrangePartyMain(Box2SysWork *syswk);
static int Box2Seq_ArrangePartyAway(Box2SysWork *syswk);
static int Box2Seq_ArrangePartyMenuClose(Box2SysWork *syswk);
static int Box2Seq_ArrangePartyMoveEnd(Box2SysWork *syswk);
static int Box2Seq_MovePartyIn(Box2SysWork *syswk);
static int Box2Seq_MovePartyMain(Box2SysWork *syswk);
static int Box2Seq_MovePartySwitch(Box2SysWork *syswk);
static int Box2Seq_MovePartyClose(Box2SysWork *syswk);
static int Box2Seq_ArrangeBoxMain(Box2SysWork *syswk);
static int Box2Seq_ArrangeBoxMoveEnd(Box2SysWork *syswk);
static int Box2Seq_ArrangeBoxMenuClose(Box2SysWork *syswk);
static int Box2Seq_ArrangeBoxPartyOut(Box2SysWork *syswk);
static int Box2Seq_ArrangeBoxPartyMain(Box2SysWork *syswk);
static int Box2Seq_ArrangeBoxPartyAway(Box2SysWork *syswk);
static int Box2Seq_ArrangeBoxPartyMenuClose(Box2SysWork *syswk);
static int Box2Seq_ArrangeBoxPartyMoveEnd(Box2SysWork *syswk);
static int Box2Seq_SubProcReturn(Box2SysWork *syswk);
static int Box2Seq_WithdrawMain(Box2SysWork *syswk);
static int Box2Seq_WithdrawMenuClose(Box2SysWork *syswk);
static int Box2Seq_WithdrawCheckParty(Box2SysWork *syswk);
static int Box2Seq_WithdrawTake(Box2SysWork *syswk);
static int Box2Seq_WithdrawEnd(Box2SysWork *syswk);
static int Box2Seq_DepositPartyMain(Box2SysWork *syswk);
static int Box2Seq_DepositPartyMenuClose(Box2SysWork *syswk);
static int Box2Seq_DepositPartyReturn(Box2SysWork *syswk);
static int Box2Seq_DepositPickParty(Box2SysWork *syswk);
static int Box2Seq_DepositPickBox(Box2SysWork *syswk);
static int Box2Seq_DepositPut(Box2SysWork *syswk);
static int Box2Seq_DepositFullClose(Box2SysWork *syswk);
static int Box2Seq_DepositEnd(Box2SysWork *syswk);
static int Box2Seq_ItemMain(Box2SysWork *syswk);
static int Box2Seq_ItemMenuClose(Box2SysWork *syswk);
static int Box2Seq_ItemToBag(Box2SysWork *syswk);
static int Box2Seq_ItemHideIcon(Box2SysWork *syswk);
static int Box2Seq_ItemBack(Box2SysWork *syswk);
static int Box2Seq_ItemBackToMenu(Box2SysWork *syswk);
static int Box2Seq_ItemPartyIn(Box2SysWork *syswk);
static int Box2Seq_ItemPartyOutMain(Box2SysWork *syswk);
static int Box2Seq_ItemPartyAway(Box2SysWork *syswk);
static int Box2Seq_ItemDropKey(Box2SysWork *syswk);
static int Box2Seq_ItemDropTouch(Box2SysWork *syswk);
static int Box2Seq_ItemMailClose(Box2SysWork *syswk);
static int Box2Seq_ItemDropKeyPartyOut(Box2SysWork *syswk);
static int Box2Seq_ItemPartyMain(Box2SysWork *syswk);
static int Box2Seq_ItemPartyMenuClose(Box2SysWork *syswk);
static int Box2Seq_ItemPartyDrop(Box2SysWork *syswk);
static int Box2Seq_ItemPartySwap(Box2SysWork *syswk);
static int Box2Seq_ItemPartyReturn(Box2SysWork *syswk);
static int Box2Seq_DreamWorldMain(Box2SysWork *syswk);
static int Box2Seq_DreamWorldMenuOpened(Box2SysWork *syswk);
static int Box2Seq_DreamWorldPick(Box2SysWork *syswk);
static int Box2Seq_DreamWorldMenuClose(Box2SysWork *syswk);
static int Box2Seq_CallPokeStatus(Box2SysWork *syswk);
static int Box2Seq_TakeItem(Box2SysWork *syswk);
static int Box2Seq_CallBoxSearch(Box2SysWork *syswk);
static int Box2Seq_SummaryReturn(Box2SysWork *syswk);
static int Box2Seq_NameInputReturn(Box2SysWork *syswk);
static int Box2Seq_BagReturn(Box2SysWork *syswk);
static int Box2Seq_BoxSearchReturn(Box2SysWork *syswk);
static int Box2Seq_GivenItemMsg(Box2SysWork *syswk);
static int Box2Seq_MarkingOpen(Box2SysWork *syswk);
static int Box2Seq_MarkingMain(Box2SysWork *syswk);
static int Box2Seq_MarkingClose(Box2SysWork *syswk);
static int Box2Seq_ReleaseStart(Box2SysWork *syswk);
static int Box2Seq_ReleaseCheck(Box2SysWork *syswk);
static int Box2Seq_ReleaseCheckMoves(Box2SysWork *syswk);
static int Box2Seq_ReleaseDo(Box2SysWork *syswk);
static int Box2Seq_ReleaseMoveLocked(Box2SysWork *syswk);
static int Box2Seq_BoxMenuOpen(Box2SysWork *syswk);
static int Box2Seq_BoxMenuMain(Box2SysWork *syswk);
static int Box2Seq_BoxMenuClose(Box2SysWork *syswk);
static int Box2Seq_BoxJumpOpen(Box2SysWork *syswk);
static int Box2Seq_BoxJumpMain(Box2SysWork *syswk);
static int Box2Seq_BoxJumpClose(Box2SysWork *syswk);
static int Box2Seq_BoxJumpReturn(Box2SysWork *syswk);
static int Box2Seq_WallpaperMenuOpen(Box2SysWork *syswk);
static int Box2Seq_WallpaperMenuMain(Box2SysWork *syswk);
static int Box2Seq_WallpaperChange(Box2SysWork *syswk);
static int Box2Seq_CallNameInput(Box2SysWork *syswk);
static int Box2Seq_NameInputOpen(Box2SysWork *syswk);
static int Box2Seq_YesTakeItem(Box2SysWork *syswk);
static int Box2Seq_YesRelease(Box2SysWork *syswk);
static int Box2Seq_NoRelease(Box2SysWork *syswk);
static int Box2Seq_YesItemToBag(Box2SysWork *syswk);
static int Box2Seq_Leave(Box2SysWork *syswk);
static int Box2Seq_ReturnToMain(Box2SysWork *syswk);
static int Box2Seq_YesDreamWorldPick(Box2SysWork *syswk);
static int Box2Seq_NoDreamWorldPick(Box2SysWork *syswk);
static int Box2Seq_NoBackToMenu(Box2SysWork *syswk);

static int Box2Seq_Goto(Box2SysWork *syswk, int seq);
static int Box2Seq_WipeIn(Box2SysWork *syswk, int nextSeq);
static int Box2Seq_RunRequestedVFunc(Box2SysWork *syswk, int seq);
static int Box2Seq_PokeMoveEndTo(Box2SysWork *syswk, int seq);
static int Box2Seq_OpenYesNo(Box2SysWork *syswk, u32 type);
static int Box2Seq_AnimFrameButton(Box2SysWork *syswk, u32 frame, int seq);
static int Box2Seq_AnimActorButton(Box2SysWork *syswk, u32 id, u32 pal, int seq);
static int Box2Seq_AnimMarkButton(Box2SysWork *syswk, u32 button, int seq);
static int Box2Seq_WipeOutCallSubProc(Box2SysWork *syswk, u32 type);
static int Box2Seq_ArrangeGiveItemReturn(Box2SysWork *syswk);
static int Box2Seq_ItemGiveItemReturn(Box2SysWork *syswk);
static int Box2Seq_BattleBoxGiveItemReturn(Box2SysWork *syswk);
static int Box2Seq_WithdrawSummaryReturn(Box2SysWork *syswk);
static int Box2Seq_DepositSummaryReturn(Box2SysWork *syswk);
static int Box2Seq_ArrangeSummaryReturn(Box2SysWork *syswk);
static int Box2Seq_BattleBoxSummaryReturn(Box2SysWork *syswk);
static int Box2Seq_ArrangeMoveSummaryReturn(Box2SysWork *syswk);
static int Box2Seq_BattleBoxMoveSummaryReturn(Box2SysWork *syswk);
static int Box2Seq_ScrollTrayLeft(Box2SysWork *syswk, u32 frameOut, int seq);
static int Box2Seq_ScrollTrayRight(Box2SysWork *syswk, u32 frameOut, int seq);
static int Box2Seq_ArrangePickUpTouched(Box2SysWork *syswk, u32 pos);
static int Box2Seq_ArrangePartyPickUpTouched(Box2SysWork *syswk, u32 pos);
static int Box2Seq_ArrangeBoxPartyPickUpTouched(Box2SysWork *syswk, u32 pos);
static int Box2Seq_DepositPickUpTouched(Box2SysWork *syswk, u32 pos);
static int Box2Seq_OpenPartyPokeMenu(Box2SysWork *syswk, u32 pos, int seq);
static int Box2Seq_MovePartyPickUpTouched(Box2SysWork *syswk, u32 pos);
static int Box2Seq_WithdrawPickUpTouched(Box2SysWork *syswk, u32 pos);
static int Box2Seq_OpenPokeMenu(Box2SysWork *syswk, u32 pos, int seq);
static int Box2Seq_ArrangePickUpKey(Box2SysWork *syswk, u32 pos);
static void AllocPokeMoveWork(Box2SysWork *syswk);
static void FreePokeMoveWork(Box2SysWork *syswk);
static int Box2Seq_ItemTakeTouched(Box2SysWork *syswk, u32 pos);
static int Box2Seq_ItemOpenMenu(Box2SysWork *syswk, u32 pos);
static int Box2Seq_ItemPartyOutTakeTouched(Box2SysWork *syswk, u32 pos);
static int Box2Seq_ItemPartyOutTakeKey(Box2SysWork *syswk, u32 pos);
static int Box2Seq_ItemDropHeldKey(Box2SysWork *syswk, u32 pos);
static int Box2Seq_ItemPartyTakeTouched(Box2SysWork *syswk, u32 pos);
static int Box2Seq_ItemPartyOpenMenu(Box2SysWork *syswk, u32 pos);
static int Box2Seq_MovePartyFrame(Box2SysWork *syswk, int seq);
static int Box2Seq_NextMoveMode(Box2SysWork *syswk, int seq);
static int Box2Seq_SetMoveMode(Box2SysWork *syswk, int seq);

static int Box2Seq_MovePickUpTouched(Box2SysWork *syswk, u32 pos, int seq);
static int Box2Seq_MoveDropKey(Box2SysWork *syswk, u32 pos, int seq);
static int Box2Seq_ScrollToPickedBox(Box2SysWork *syswk, int seq);
static void AllocBoxListDragWork(Box2SysWork *syswk);
static int Box2Seq_MovePartyFrameTo(Box2SysWork *syswk, int seq);
static int Box2Seq_RangePickUpTouched(Box2SysWork *syswk, int seq);
static int Box2Seq_MovePickUpKey(Box2SysWork *syswk, u32 pos, int seq);
static int Box2Seq_RangePickUpKey(Box2SysWork *syswk, u32 pos, int seq);
static int Box2Seq_RangeDropKey(Box2SysWork *syswk, u32 pos, int seq);
static u32 GetDepositBlock(Box2SysWork *syswk);
static void AllocItemMoveWork(Box2SysWork *syswk);
static void FreeItemMoveWork(Box2SysWork *syswk);
static void PutPartyLeft(Box2SysWork *syswk);
static void PutPartyRight(Box2SysWork *syswk);
static void ToggleMark(Box2SysWork *syswk, u32 mark);
static int Box2Seq_PickWallpaper(Box2SysWork *syswk, u32 pos);
static void DimActorsForNameInput(Box2SysWork *syswk);
static void CancelRange(Box2SysWork *syswk);
static void SortRangeEnds(Box2SysWork *syswk);

// The menus' items: a message and whether the item closes the menu. MWCC sorts static data by size, unstably, so
// tables of one size are declared in the order that lays them out as the ROM has them
static const Box2MenuItem sMenu70d4[] = { { 93, 0 }, { 80, 1 } };
static const Box2MenuItem sMenu70cc[] = { { 83, 0 }, { 80, 1 } };
static const Box2MenuItem sMenu70dc[] = { { 94, 0 }, { 92, 0 }, { 80, 1 } };
static const Box2MenuItem sMenu70e8[] = { { 31, 0 }, { 32, 0 }, { 33, 0 }, { 34, 1 } };
static const Box2MenuItem sMenu70f8[] = { { 82, 0 }, { 74, 0 }, { 75, 0 }, { 80, 1 } };
// The wallpaper themes and their wallpapers
static const Box2MenuItem sMenu7130[] = { { 79, 0 }, { 74, 0 }, { 76, 0 }, { 77, 0 }, { 80, 1 } };
static const Box2MenuItem sMenu7108[] = { { 58, 0 }, { 59, 0 }, { 60, 0 }, { 61, 0 }, { 66, 1 } };
static const Box2MenuItem sMenu7180[] = { { 53, 0 }, { 54, 0 }, { 55, 0 }, { 56, 0 }, { 66, 1 } };
static const Box2MenuItem sMenu711c[] = { { 35, 0 }, { 36, 0 }, { 37, 0 }, { 38, 0 }, { 66, 1 } };
static const Box2MenuItem sMenu7144[] = { { 41, 0 }, { 42, 0 }, { 43, 0 }, { 44, 0 }, { 66, 1 } };
static const Box2MenuItem sMenu7158[] = { { 45, 0 }, { 46, 0 }, { 47, 0 }, { 48, 0 }, { 66, 1 } };
static const Box2MenuItem sMenu716c[] = { { 49, 0 }, { 50, 0 }, { 51, 0 }, { 52, 0 }, { 66, 1 } };
static const Box2MenuItem sMenu7194[] = { { 78, 0 }, { 74, 0 }, { 76, 0 }, { 77, 0 }, { 80, 1 } };
static const Box2MenuItem sMenu71a8[] = { { 57, 0 }, { 35, 0 }, { 36, 0 }, { 37, 0 }, { 38, 0 }, { 66, 1 } };
static const Box2MenuItem sMenu71c0[] = { { 58, 0 }, { 59, 0 }, { 60, 0 }, { 61, 0 }, { 68, 0 }, { 66, 1 } };
static const Box2MenuItem sMenu71d8[] = { { 62, 0 }, { 63, 0 }, { 64, 0 }, { 65, 0 }, { 67, 0 }, { 66, 1 } };
static const Box2MenuItem sMenu71f0[] = { { 81, 0 }, { 74, 0 }, { 75, 0 }, { 76, 0 }, { 77, 0 }, { 80, 1 } };

static const Box2SubProc sSubProcs[] = {
    { Box2Main_PokeStatusCall, Box2Main_PokeStatusExit, 92 },
    { Box2Main_BagCall, Box2Main_BagExit, 94 },
    { Box2Main_NameInputCall, Box2Main_NameInputExit, 93 },
    { Box2Main_BoxSearchCall, Box2Main_BoxSearchExit, 95 },
};

// clang-format off
static const Box2SeqFunc sMainSeq[] = {
    Box2Seq_Init, Box2Seq_Release, Box2Seq_Wipe,
    Box2Seq_PaletteFade, Box2Seq_Wait, Box2Seq_VFunc,
    Box2Seq_TrgWait, Box2Seq_YesNo, Box2Seq_ButtonAnm,
    Box2Seq_SubProcCall, Box2Seq_SubProcMain, Box2Seq_Start,
    Box2Seq_StartWait, Box2Seq_PokeMoveEnd, Box2Seq_OpenMenu,
    Box2Seq_BoxListScrollTouch, Box2Seq_LeaveState, Box2Seq_ArrangeMain,
    Box2Seq_ArrangeMoveToBoxList, Box2Seq_ArrangeMenuClose, Box2Seq_ArrangeMoveStart,
    Box2Seq_ArrangeMoveMain, Box2Seq_ArrangeMovePartyStart, Box2Seq_ArrangeMoveEnd,
    Box2Seq_ArrangeMoveErrClose, Box2Seq_ArrangeMovePartyOut, Box2Seq_ArrangePartyOut,
    Box2Seq_ArrangePartyMain, Box2Seq_ArrangePartyAway, Box2Seq_ArrangePartyMenuClose,
    Box2Seq_ArrangePartyMoveEnd, Box2Seq_MovePartyIn, Box2Seq_MovePartyMain,
    Box2Seq_MovePartySwitch, Box2Seq_MovePartyClose, Box2Seq_RangeStart,
    Box2Seq_RangeMain, Box2Seq_RangePartyOut, Box2Seq_RangeMoveEnd,
    Box2Seq_RangeMoveErrClose, Box2Seq_RangePartyAway, Box2Seq_RangePartyIn,
    Box2Seq_RangePartyMain, Box2Seq_RangePartySwitch, Box2Seq_RangePartyClose,
    Box2Seq_ArrangeBoxMain, Box2Seq_ArrangeBoxMoveEnd, Box2Seq_ArrangeBoxMenuClose,
    Box2Seq_ArrangeBoxPartyOut, Box2Seq_ArrangeBoxPartyMain, Box2Seq_ArrangeBoxPartyAway,
    Box2Seq_ArrangeBoxPartyMenuClose, Box2Seq_ArrangeBoxPartyMoveEnd, Box2Seq_SubProcReturn,
    Box2Seq_WithdrawMain, Box2Seq_WithdrawMenuClose, Box2Seq_WithdrawCheckParty,
    Box2Seq_WithdrawTake, Box2Seq_WithdrawEnd, Box2Seq_DepositPartyMain,
    Box2Seq_DepositPartyMenuClose, Box2Seq_DepositPartyReturn, Box2Seq_DepositPickParty,
    Box2Seq_DepositPickBox, Box2Seq_DepositPut, Box2Seq_DepositFullClose,
    Box2Seq_DepositEnd, Box2Seq_ItemMain, Box2Seq_ItemMenuClose,
    Box2Seq_ItemToBag, Box2Seq_ItemHideIcon, Box2Seq_ItemBack,
    Box2Seq_ItemBackToMenu, Box2Seq_ItemPartyIn, Box2Seq_ItemPartyOutMain,
    Box2Seq_ItemPartyAway, Box2Seq_ItemDropKey, Box2Seq_ItemDropTouch,
    Box2Seq_ItemMailClose, Box2Seq_ItemDropKeyPartyOut, Box2Seq_ItemPartyMain,
    Box2Seq_ItemPartyMenuClose, Box2Seq_ItemPartyDrop, Box2Seq_ItemPartySwap,
    Box2Seq_ItemPartyReturn, Box2Seq_DreamWorldMain, Box2Seq_DreamWorldMenuOpened,
    Box2Seq_DreamWorldPick, Box2Seq_DreamWorldMenuClose, Box2Seq_CallPokeStatus,
    Box2Seq_TakeItem, Box2Seq_CallBoxSearch, Box2Seq_SummaryReturn,
    Box2Seq_NameInputReturn, Box2Seq_BagReturn, Box2Seq_BoxSearchReturn,
    Box2Seq_GivenItemMsg, Box2Seq_MarkingOpen, Box2Seq_MarkingMain,
    Box2Seq_MarkingClose, Box2Seq_ReleaseStart, Box2Seq_ReleaseCheck,
    Box2Seq_ReleaseCheckMoves, Box2Seq_ReleaseDo, Box2Seq_ReleaseMoveLocked,
    Box2Seq_BoxMenuOpen, Box2Seq_BoxMenuMain, Box2Seq_BoxMenuClose,
    Box2Seq_BoxJumpOpen, Box2Seq_BoxJumpMain, Box2Seq_BoxJumpClose,
    Box2Seq_BoxJumpReturn, Box2Seq_WallpaperMenuOpen, Box2Seq_WallpaperMenuMain,
    Box2Seq_WallpaperChange, Box2Seq_CallNameInput, Box2Seq_NameInputOpen,
};

// clang-format on

// clang-format off
static const Box2YesNoFuncs sYesNoFuncs[] = {
    { Box2Seq_YesTakeItem, Box2Seq_NoBackToMenu },
    { Box2Seq_YesRelease, Box2Seq_NoRelease },
    { Box2Seq_YesItemToBag, Box2Seq_NoBackToMenu },
    { Box2Seq_Leave, Box2Seq_ReturnToMain },
    { Box2Seq_ReturnToMain, Box2Seq_Leave },
    { Box2Seq_YesDreamWorldPick, Box2Seq_NoDreamWorldPick },
};

// clang-format on

BOOL Box2Seq_Main(Box2SysWork *syswk, u32 *seq) {
    if (syswk->app == NULL || syswk->app->printQueue == NULL || PrintQueue_IsEmpty(syswk->app->printQueue) != FALSE) {
        if (syswk->app != NULL) {
            Box2Bmp_FlushTrayName(syswk);
        }
        *seq = sMainSeq[*seq](syswk);
    }
    if (*seq == BOX2SEQ_END) {
        return FALSE;
    }
    if (syswk->app != NULL) {
        Box2Bmp_UpdatePrint(syswk->app);
        Box2Obj_UpdateAnim(syswk->app);
        Box2Obj_UpdateRangeDots(syswk);
    }
    return TRUE;
}

static int Box2Seq_Init(Box2SysWork *syswk) {
    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetDisplayLayout(0);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_BOX2_APP, 0x80000);
    syswk->app = GFL_HeapAllocate(HEAPID_BOX2_APP, sizeof(Box2AppWork), TRUE, "box2_seq.c", 737);
    syswk->app->pokeIconArc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, HEAPID_BOX2_APP);
    GCTX_HIDGetRepeat(&syswk->app->keyRepeatWait, &syswk->app->keyRepeatStart);
    setKeypressFramecounts(6, 6);
    Box2Main_InitVramBanks();
    Box2Main_InitBg(syswk);
    Box2Main_InitPaletteFade(syswk);
    Box2Main_LoadBgGraphics(syswk);
    Box2Main_InitMsg(syswk);
    Box2Main_InitDexData(syswk);
    Box2Bmp_CreateWindows(syswk);
    Box2Obj_Init(syswk);
    Box2Obj_SetTrayPokeIcons(syswk);
    Box2Obj_SetPartyPokeIcons(syswk);
    Box2Main_WallPaperSet(syswk, Box2Main_GetWallPaperNumber(syswk, syswk->tray), BOX2_TRAY_SCROLL_NONE);
    Box2Frame_Init(syswk);
    Box2Bmp_PrintTitle(syswk);
    Box2Bmp_PrintInfoLabels(syswk);
    Box2Obj_MakeAllTrayIconChars(syswk);
    Box2Ui_CreateCursor(syswk);
    Box2Main_InitYesNo(syswk);
    Box2Main_SetBlendAlpha(TRUE);
    GFL_NetSetIconScreen(TRUE, HEAPID_BOX2_APP);
    Box2Main_InitVBlank(syswk);
    return syswk->nextSeq;
}

static int Box2Seq_Release(Box2SysWork *syswk) {
    Box2Main_ExitVBlank(syswk);
    Box2Main_ExitYesNo(syswk);
    Box2Ui_DeleteCursor(syswk);
    Box2Frame_Delete(syswk->app);
    Box2Obj_Exit(syswk->app);
    Box2Bmp_FreeWindows(syswk);
    Box2Main_ExitDexData(syswk);
    Box2Main_ExitMsg(syswk);
    Box2Main_ExitPaletteFade(syswk);
    Box2Main_ExitBg(syswk);
    setKeypressFramecounts(syswk->app->keyRepeatWait, syswk->app->keyRepeatStart);
    GFL_ArcToolFree(syswk->app->pokeIconArc);
    GFL_HeapFree(syswk->app);
    GFL_HeapDelete(HEAPID_BOX2_APP);
    syswk->app = NULL;
    G2_BlendNone();
    G2S_BlendNone();
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return syswk->nextSeq;
}

static int Box2Seq_Wipe(Box2SysWork *syswk) {
    if (GFL_WipeIsFinished() == TRUE) {
        return syswk->app->wipeSeq;
    }
    return BOX2SEQ_WIPE;
}

static int Box2Seq_PaletteFade(Box2SysWork *syswk) {
    if (PaletteFade_GetActiveMask(syswk->app->palFade) == FALSE) {
        return syswk->nextSeq;
    }
    return BOX2SEQ_PALETTE_FADE;
}

static int Box2Seq_Wait(Box2SysWork *syswk) {
    if (syswk->app->wait == 0) {
        return syswk->nextSeq;
    }
    syswk->app->wait--;
    return BOX2SEQ_WAIT;
}

static int Box2Seq_VFunc(Box2SysWork *syswk) {
    if (syswk->app->vfunk.func != NULL && syswk->app->vfunk.func(syswk) == FALSE) {
        syswk->app->vfunk.func = NULL;
        return syswk->app->vfuncNextSeq;
    }
    return BOX2SEQ_VFUNC;
}

static int Box2Seq_TrgWait(Box2SysWork *syswk) {
    if (GFL_HIDIsTouchPressed() == TRUE) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        GCTX_HIDSetUsingTouch(TRUE);
        return syswk->nextSeq;
    }
    if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        GCTX_HIDSetUsingTouch(FALSE);
        return syswk->nextSeq;
    }
    return BOX2SEQ_TRGWAIT;
}

static int Box2Seq_YesNo(Box2SysWork *syswk) {
    u32 sel;

    AppTaskMenu_Update(syswk->app->yesNoMenu);
    if (AppTaskMenu_IsFlashFinished(syswk->app->yesNoMenu) == TRUE) {
        sel = AppTaskMenu_GetCursorPos(syswk->app->yesNoMenu);
        AppTaskMenu_Free(syswk->app->yesNoMenu);
        setKeypressFramecounts(6, 6);
        if (sel == 0) {
            return sYesNoFuncs[syswk->app->ynID].yes(syswk);
        }
        return sYesNoFuncs[syswk->app->ynID].no(syswk);
    }
    return BOX2SEQ_YESNO;
}

static int Box2Seq_ButtonAnm(Box2SysWork *syswk) {
    if (Box2Main_ButtonAnmMain(syswk) == FALSE) {
        return syswk->nextSeq;
    }
    return BOX2SEQ_BUTTON_ANM;
}

static int Box2Seq_SubProcCall(Box2SysWork *syswk) {
    sSubProcs[syswk->subProcType].call(syswk);
    return BOX2SEQ_SUBPROC_MAIN;
}

static int Box2Seq_SubProcMain(Box2SysWork *syswk) {
    if (syswk->procMgrResult != TRUE) {
        sSubProcs[syswk->subProcType].exit(syswk);
        syswk->nextSeq = sSubProcs[syswk->subProcType].nextSeq;
        return BOX2SEQ_INIT;
    }
    return BOX2SEQ_SUBPROC_MAIN;
}

static int Box2Seq_Start(Box2SysWork *syswk) {
    GFL_SndSEPlay(SEQ_SE_PC_LOGIN);
    switch (syswk->param->mode) {
    case 0:
        Box2Seq_PutPartyLeft(syswk);
        Box2Obj_ShowScrollArrows(syswk->app, 0);
        Box2Obj_SetIconsBlend(syswk, 1, 1);
        Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
        syswk->app->msgNextSeq = 59;
        break;
    case 1:
        Box2Main_PokeInfoPut(syswk, 0);
        syswk->app->msgNextSeq = 54;
        break;
    case 2:
        Box2Main_PokeInfoPut(syswk, 0);
        Box2Frame_ShowPartyButton(syswk->app);
        syswk->app->msgNextSeq = 17;
        break;
    case 3:
        Box2Obj_SetIconsBlend(syswk, 0x81, 1);
        Box2Main_PokeInfoPut(syswk, 0);
        Box2Frame_ShowPartyButton(syswk->app);
        syswk->app->msgNextSeq = 67;
        break;
    case 4:
        if (PokeParty_GetPkmCount(syswk->param->party) == 0) {
            Box2Frame_ShowPartyButton(syswk->app);
            Box2Main_PokeInfoPut(syswk, 0);
            Box2Ui_ChangeCursor(syswk, 13, 0);
            syswk->app->msgNextSeq = 45;
        } else {
            Box2Seq_PutPartyLeft(syswk);
            Box2Obj_ShowScrollArrows(syswk->app, 0);
            Box2Obj_SetIconsBlend(syswk, 1, 1);
            Box2Frame_ShowBoxListButton(syswk->app);
            Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
            Box2Ui_ChangeCursor(syswk, 14, 0);
            syswk->app->msgNextSeq = 49;
        }
        break;
    case 5:
        Box2Main_PokeInfoPut(syswk, 0);
        Box2Obj_SetIconsBlend(syswk, 0x81, 1);
        Box2Obj_SetButtons(syswk, 0, 1, 1, 1);
        syswk->app->msgNextSeq = 85;
        break;
    }
    return Box2Seq_StartWait(syswk);
}

static int Box2Seq_StartWait(Box2SysWork *syswk) {
    if (PrintQueue_IsEmpty(syswk->app->printQueue) == FALSE) {
        return BOX2SEQ_START_WAIT;
    }
    return Box2Seq_WipeOut(syswk, syswk->app->msgNextSeq);
}

static int Box2Seq_PokeMoveEnd(Box2SysWork *syswk) {
    GFL_HeapFree(syswk->app->vfunk.work);
    if (syswk->app->unkA552 == 1) {
        if (syswk->param->mode == 3) {
            if (syswk->app->getItem == 0) {
                Box2Bmp_PrintItemMsg(syswk, 0, 24);
            } else {
                Box2Bmp_PrintItemMsg(syswk, 1, 24);
            }
        } else {
            Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
        }
        syswk->app->unkA552 = 0;
    }
    return syswk->nextSeq;
}

static int Box2Seq_OpenMenu(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        switch (syswk->param->mode) {
        case 1:
            Box2Bmp_OpenMenu(syswk, sMenu7194, 5);
            break;
        case 0:
            Box2Bmp_OpenMenu(syswk, sMenu7130, 5);
            break;
        case 2:
            Box2Bmp_OpenMenu(syswk, sMenu71f0, 6);
            break;
        case 3:
            Box2Seq_OpenItemMenu(syswk, syswk->app->getItem);
            break;
        case 4:
            Box2Bmp_OpenMenu(syswk, sMenu70f8, 4);
            break;
        case 5:
            Box2Bmp_ClearMsgWindow(syswk->app, 27);
            Box2Bmp_OpenMenu(syswk, sMenu70cc, 2);
            break;
        }
        if (syswk->app->unkA550 == 1) {
            syswk->app->unkA550 = 0;
            if (syswk->param->mode == 3) {
                if (syswk->pos >= BOX2_PARTY_POS) {
                    Box2Obj_SetIconsBlend(syswk, 0x82, 0);
                } else {
                    Box2Obj_ShowScrollArrows(syswk->app, 1);
                    Box2Obj_SetIconsBlend(syswk, 0x81, 0);
                }
            } else if (syswk->pos >= BOX2_PARTY_POS) {
                Box2Obj_SetIconsBlend(syswk, 2, 0);
            } else {
                Box2Obj_ShowScrollArrows(syswk->app, 1);
                Box2Obj_SetIconsBlend(syswk, 1, 0);
            }
        }
        Box2Frame_SlideMenuIn(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 14);
    case 1:
        syswk->app->subSeq = 0;
        Box2Obj_ShowCursor(syswk, 1);
        Box2Obj_SetToolButtons(syswk, 9, 1);
        switch (syswk->param->mode) {
        case 1:
            Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
            return 54;
        case 0:
            Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
            return 59;
        case 2:
            Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
            if (syswk->pos < BOX2_PARTY_POS) {
                return 17;
            }
            return 27;
        case 3:
            if (syswk->app->getItem == 0) {
                Box2Bmp_PrintItemMsg(syswk, 0, 24);
            } else {
                Box2Bmp_PrintItemMsg(syswk, 1, 24);
            }
            if (syswk->pos < BOX2_PARTY_POS) {
                return 67;
            }
            return 80;
        case 4:
            Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
            if (syswk->pos < BOX2_PARTY_POS) {
                return 45;
            }
            return 49;
        case 5:
            Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
            return 85;
        }
    }
    return 14;
}

// Scrolls the box list by touch
static int Box2Seq_BoxListScrollTouch(Box2SysWork *syswk) {
    u32 x, y;
    Box2BoxListDrag *work;
    u32 oldY;
    int diff;

    if (Box2Ui_GetBoxListTouch(&x, &y) == FALSE) {
        int pos;

        GFL_HeapFree(syswk->app->subWork);
        pos = Box2Ui_GetBoxListRowAt(syswk->app->tpx, syswk->app->tpy);
        if (pos >= 0) {
            Box2Obj_ShowBoxNameFrame(syswk, pos, 1);
        }
        return syswk->nextSeq;
    }
    work = syswk->app->subWork;
    oldY = syswk->app->tpy;
    syswk->app->tpy = y;
    diff = oldY - y;
    if (MATH_ABS(diff) >= 3) {
        work->cnt = MATH_ABS(diff) / 8;
        if (y < oldY) {
            work->dir = 1;
            Box2Main_StartBoxListScroll(syswk, 1);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (syswk->nextSeq == 21) {
                return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollLeft, 15);
            }
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxMoveScrollLeft, 15);
        }
        if (y > oldY) {
            work->dir = -1;
            Box2Main_StartBoxListScroll(syswk, -1);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (syswk->nextSeq == 21) {
                return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollRight, 15);
            }
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxMoveScrollRight, 15);
        }
    }
    if (work->cnt != 0) {
        work->cnt--;
        if (work->dir == 1) {
            Box2Main_StartBoxListScroll(syswk, 1);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (syswk->nextSeq == 21) {
                return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollLeft, 15);
            }
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxMoveScrollLeft, 15);
        }
        if (work->dir == -1) {
            Box2Main_StartBoxListScroll(syswk, -1);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (syswk->nextSeq == 21) {
                return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollRight, 15);
            }
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxMoveScrollRight, 15);
        }
    }
    return 15;
}

static int Box2Seq_LeaveState(Box2SysWork *syswk) {
    return Box2Seq_Leave(syswk);
}

// The arrangement's main state: waits for a touch or for the cursor to pick something
static int Box2Seq_ArrangeMain(Box2SysWork *syswk) {
    u32 res;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        syswk->unk1B = 0;
        return Box2Seq_NextMoveMode(syswk, 20);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) && Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == FALSE) {
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
        syswk->unk13 = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_ShowCursor(syswk);
        return Box2Seq_AnimActorButton(syswk, 30, 1, 91);
    }

    res = Box2Ui_GetTrayPokeTrigger();
    if (res != 0xffffffff) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
            if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == FALSE) {
                Box2Bmp_OpenMenu(syswk, sMenu71f0, 6);
            }
            Box2Main_PokeInfoPut(syswk, res);
            return Box2Seq_ArrangePickUpTouched(syswk, res);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        Box2Ui_PlaceCursorActor(syswk, res);
        Box2Main_PokeInfoOff(syswk);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            Box2Obj_ShowPokeOutline(syswk, 0);
            Box2Bmp_ClearMsgWindow(syswk->app, 24);
            Box2Main_PutMessageFrame(syswk);
            Box2Frame_ShowPartyButton(syswk->app);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 17);
        }
        return 17;
    }

    res = Box2Ui_UpdateCursor3(syswk);
    switch (res) {
    case 30:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return Box2Seq_Goto(syswk, 105);
    case 31:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        syswk->app->oldCurPos = 30;
        Box2Ui_PlaceCursorActor(syswk, 30);
        return Box2Seq_ScrollTrayLeft(syswk, 1, 17);
    case 32:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        syswk->app->oldCurPos = 30;
        Box2Ui_PlaceCursorActor(syswk, 30);
        return Box2Seq_ScrollTrayRight(syswk, 1, 17);
    case 33:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, 10, Box2Seq_Goto(syswk, 26));
    case 34:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->closed = TRUE;
        return Box2Seq_AnimActorButton(syswk, 7, 8, 16);
    case 35:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->param->closed = FALSE;
        return Box2Seq_AnimActorButton(syswk, 6, 9, 116);
    case 36:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk1B = 0;
        syswk->moveMode = 0;
        return Box2Seq_AnimFrameButton(syswk, 0, Box2Seq_Goto(syswk, 20));
    case 37:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk13 = 0;
        syswk->getTray = syswk->tray;
        syswk->curRcvPos = 37;
        return Box2Seq_AnimFrameButton(syswk, 1, 89);
    case 38:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk13 = 0;
        syswk->curRcvPos = 38;
        return Box2Seq_AnimFrameButton(syswk, 2, Box2Seq_Goto(syswk, 90));
    case 39:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, 3, Box2Seq_Goto(syswk, 97));
    case 40:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, 4, Box2Seq_Goto(syswk, 101));
    case 41: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos;
        Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        syswk->app->oldCurPos = pos;
        return Box2Seq_AnimFrameButton(syswk, 5, 19);
    }
    case 42:
        break;
    case 43:
        syswk->unk1B = 0;
        syswk->moveMode = 1;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_SetMoveMode(syswk, 20);
    case 44:
        syswk->unk1B = 0;
        syswk->moveMode = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_SetMoveMode(syswk, 35);
    case 45:
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
        syswk->unk13 = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, 30, 1, 91);
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return Box2Seq_ScrollTrayLeft(syswk, 1, 17);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return Box2Seq_ScrollTrayRight(syswk, 1, 17);
        }
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        if (pos < BOX2_PARTY_POS) {
            Box2Main_PokeInfoPut(syswk, pos);
        } else if (pos != 36 && pos != 37 && pos != 38 && pos != 39 && pos != 40 && pos != 41) {
            Box2Main_PokeInfoOff(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, 17));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos;

            Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            syswk->app->oldCurPos = pos;
            return Box2Seq_AnimFrameButton(syswk, 5, 19);
        }
        syswk->param->closed = FALSE;
        return Box2Seq_AnimActorButton(syswk, 6, 9, 116);
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_NONE:
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Bmp_OpenMenu(syswk, sMenu71f0, 6);
            Box2Main_PokeInfoPut(syswk, res);
            Box2Ui_StartCursorMove(syswk->app, BOX2_BOXLIST_POS, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
            syswk->app->oldCurPos = BOX2_BOXLIST_POS;
            return Box2Seq_OpenPokeMenu(syswk, res, 17);
        }
        break;
    }
    return 17;
}

static int Box2Seq_ArrangeMoveToBoxList(Box2SysWork *syswk) {
    Box2Main_PokeDataMove(syswk);
    FreePokeMoveWork(syswk);
    CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
    syswk->app->oldCurPos = BOX2_BOXLIST_POS;
    Box2Ui_PlaceCursorActor(syswk, BOX2_BOXLIST_POS);
    Box2Obj_ShowCursor(syswk, 1);
    Box2Obj_SetButtons(syswk, 0, 0, 1, 0);
    Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
    return 17;
}

static int Box2Seq_ArrangeMenuClose(Box2SysWork *syswk) {
    Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
    Box2Obj_ShowPokeOutline(syswk, 0);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    Box2Frame_ShowPartyButton(syswk->app);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMoveFrame, Box2Seq_PokeMoveEndTo(syswk, 17));
}

static int Box2Seq_ArrangeMoveStart(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == FALSE) {
            syswk->trayScroll = syswk->tray;
        }
        if (syswk->moveMode == 1 && syswk->unk1C_4 == 0) {
            syswk->getTray = BOX2_GET_NONE;
            syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
            if (syswk->pos >= BOX2_PARTY_POS) {
                syswk->pos = 0;
                Box2Main_PokeInfoPut(syswk, syswk->pos);
            }
        } else {
            syswk->getTray = syswk->tray;
            Box2Bmp_ClearMsgWindow(syswk->app, 24);
            Box2Main_PutMessageFrame(syswk);
            Box2Frame_ShowPartyButton(syswk->app);
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        }
        Box2Obj_ShowCursor(syswk, 0);
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 20);
    case 1:
        syswk->app->subSeq++;
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == FALSE) {
            GFL_SndSEPlay(SEQ_SE_SYS_42);
            return Box2Seq_MovePartyFrame(syswk, 20);
        }
    case 2:
        Box2Ui_ChangeCursor(syswk, 4, syswk->pos);
        Box2Obj_SetCursorAnim(syswk, 6);
        syswk->app->oldCurPos = syswk->pos;
        syswk->app->subSeq = 0;
        if (syswk->moveMode == 1) {
            if (syswk->unk1C_4 == 0) {
                if (Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                    Box2Obj_SetButtons(syswk, 0, 1, 2, 0);
                } else {
                    Box2Obj_SetButtons(syswk, 0, 1, 0, 0);
                }
                Box2Obj_SetToolButtons(syswk, 10, 1);
                return 21;
            }
            Box2Obj_SetToolButtons(syswk, 10, 1);
        } else {
            Box2Obj_SetToolButtons(syswk, 9, 1);
        }
        return Box2Seq_ArrangePickUpKey(syswk, syswk->pos);
    }
    return 20;
}

// Sets the held Pokémon's position and tray
static inline void Box2Seq_SetGetPos(Box2SysWork *syswk, u8 pos, u8 tray) {
    syswk->pos = pos;
    syswk->getTray = tray;
}

// Moving Pokémon: waits for a touch or for the cursor to pick one
static int Box2Seq_ArrangeMoveMain(Box2SysWork *syswk) {
    u32 x, y;
    u32 res;

    if (syswk->unk1C_6 == 1) {
        syswk->moveMode = 0;
        syswk->unk1C_6 = 0;
        return Box2Seq_Goto(syswk, 22);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) && syswk->moveMode != 0 && syswk->unk18 == 0) {
        Box2Main_ShowCursor(syswk);
        syswk->moveMode = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, 22);
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) {
        if (syswk->unk18 == 0) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            Box2Seq_SetGetPos(syswk, pos, syswk->tray);
            syswk->unk13 = 2;
        } else {
            syswk->unk13 = 3;
            syswk->unk1D = CursorMove_GetPos(syswk->app->cursorMove);
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_ShowCursor(syswk);
        return Box2Seq_AnimActorButton(syswk, 30, 1, 91);
    }
    if (syswk->unk18 == 0) {
        res = Box2Ui_GetTrayPokeTrigger();
        if (res != 0xffffffff) {
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            Box2Obj_ShowBoxNameFrame(syswk, 0, 0);
            if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
                GFL_SndSEPlay(SEQ_SE_SYS_39);
                GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
                Box2Main_PokeInfoPut(syswk, res);
                CursorMove_SetPos(syswk->app->cursorMove, res);
                return Box2Seq_MovePickUpTouched(syswk, res, 21);
            }
            CursorMove_SetPos(syswk->app->cursorMove, res);
            Box2Ui_SetButtonStates(syswk, res);
            syswk->app->oldCurPos = res;
            Box2Ui_PlaceCursorActor(syswk, res);
            Box2Main_PokeInfoOff(syswk);
            return 21;
        }
    }
    if (Box2Ui_GetBoxListScrollTouch(&x, &y) == TRUE) {
        syswk->app->tpx = x;
        syswk->app->tpy = y;
        syswk->nextSeq = 21;
        AllocBoxListDragWork(syswk);
        return 15;
    }

    res = Box2Ui_UpdateCursor4(syswk);
    switch (res) {
    case 30:
        Box2Obj_ShowBoxNameFrame(syswk, 0, 0);
        if (syswk->unk18 != 0) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return Box2Seq_Goto(syswk, 105);
    case 31:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Obj_ShowBoxNameFrame(syswk, 0, 0);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        Box2Ui_PlaceCursorActor(syswk, 30);
        Box2Ui_SetButtonStates(syswk, 30);
        syswk->app->oldCurPos = 30;
        if (syswk->unk18 == 0) {
            Box2Main_PokeSelectOff(syswk);
        } else {
            Box2Obj_MoveHeldIcons(syswk);
        }
        return Box2Seq_ScrollTrayLeft(syswk, 0, 21);
    case 32:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Obj_ShowBoxNameFrame(syswk, 0, 0);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        Box2Ui_PlaceCursorActor(syswk, 30);
        Box2Ui_SetButtonStates(syswk, 30);
        syswk->app->oldCurPos = 30;
        if (syswk->unk18 == 0) {
            Box2Main_PokeSelectOff(syswk);
        } else {
            Box2Obj_MoveHeldIcons(syswk);
        }
        return Box2Seq_ScrollTrayRight(syswk, 0, 21);
    case 39:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->unk18 == 0) {
            Box2Main_PokeInfoOff(syswk);
        }
        return Box2Seq_AnimFrameButton(syswk, 10, Box2Seq_Goto(syswk, 25));
    case 40:
        if (syswk->unk18 == 0) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            if (pos >= BOX2_PARTY_POS ||
                Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                break;
            }
            syswk->pos = pos;
            syswk->getTray = syswk->tray;
            syswk->unk13 = 2;
        } else {
            syswk->unk13 = 3;
            syswk->unk1D = CursorMove_GetPos(syswk->app->cursorMove);
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, 8, 1, 89);
    case 41:
        if (syswk->unk18 != 0) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = 0;
        return Box2Seq_AnimActorButton(syswk, 6, 9, Box2Seq_Goto(syswk, 22));
    case 42:
        if (syswk->moveMode == 0 || syswk->unk18 != 0) {
            break;
        }
        syswk->moveMode = 0;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, 22);
    case 43:
        break;
    case 44:
        if (syswk->moveMode == 0 || syswk->unk18 != 0) {
            break;
        }
        syswk->moveMode = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, 22);
    case 45:
        if (syswk->unk18 == 0) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            Box2Seq_SetGetPos(syswk, pos, syswk->tray);
            syswk->unk13 = 2;
        } else {
            syswk->unk13 = 3;
            syswk->unk1D = CursorMove_GetPos(syswk->app->cursorMove);
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, 30, 1, 91);
    case CURSORMOVE_CANCEL:
        if (syswk->unk18 == 1) {
            return Box2Seq_MoveDropKey(syswk, BOX2_GET_NONE, 21);
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = 0;
        return Box2Seq_AnimActorButton(syswk, 6, 9, Box2Seq_Goto(syswk, 22));
    case CURSORMOVE_UNK_8:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 34) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_StartBoxListScroll(syswk, -1);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollRight, 21);
        }
        break;
    case CURSORMOVE_UNK_7:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 37) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_StartBoxListScroll(syswk, 1);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollLeft, 21);
        }
        break;
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return Box2Seq_ScrollTrayLeft(syswk, 0, 21);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return Box2Seq_ScrollTrayRight(syswk, 0, 21);
        }
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_SELECT1);
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (syswk->unk18 == 0) {
            if (pos < BOX2_PARTY_POS) {
                Box2Main_PokeInfoPut(syswk, pos);
            } else {
                Box2Main_PokeInfoOff(syswk);
            }
        }
        if (pos >= 34 && pos <= 37) {
            Box2Obj_ShowBoxNameFrame(syswk, pos - 34, 1);
        } else {
            Box2Obj_ShowBoxNameFrame(syswk, 0, 0);
        }
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, 21));
    }
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case 33:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_StartBoxListScroll(syswk, -1);
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollRight, 21);
    case 38:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_StartBoxListScroll(syswk, 1);
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollLeft, 21);
    case 34:
    case 35:
    case 36:
    case 37:
        Box2Obj_ShowBoxNameFrame(syswk, res - 34, 1);
        if (syswk->unk18 == 1) {
            return Box2Seq_MoveDropKey(syswk, res + 2, 21);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeInfoOff(syswk);
        syswk->app->unkA55F = syswk->trayScroll + res - 34;
        if (syswk->app->unkA55F >= syswk->trayMax) {
            syswk->app->unkA55F -= syswk->trayMax;
        }
        if (syswk->app->unkA55F != syswk->tray) {
            return Box2Seq_ScrollToPickedBox(syswk, 21);
        }
        break;
    case CURSORMOVE_NONE:
        break;
    default:
        Box2Obj_ShowBoxNameFrame(syswk, 0, 0);
        if (syswk->unk18 == 1) {
            return Box2Seq_MoveDropKey(syswk, res, 21);
        }
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            Box2Seq_SetGetPos(syswk, res, syswk->tray);
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
            return Box2Seq_ArrangePickUpKey(syswk, syswk->pos);
        }
        break;
    }
    return 21;
}

static int Box2Seq_ArrangeMovePartyStart(Box2SysWork *syswk) {
    u8 pos;

    switch (syswk->app->subSeq) {
    case 0:
        Box2Obj_ShowCursor(syswk, 0);
        Box2Obj_ShowBoxNameFrame(syswk, 0, 0);
        if (CursorMove_GetPos(syswk->app->cursorMove) >= BOX2_PARTY_POS) {
            Box2Main_PokeInfoOff(syswk);
        }
        if (syswk->moveMode != 2) {
            Box2Obj_SetButtons(syswk, 0, 0, 1, 0);
            GFL_SndSEPlay(SEQ_SE_SYS_42);
            syswk->app->subSeq++;
            return Box2Seq_MovePartyFrameTo(syswk, 22);
        }
        return 35;
    case 1:
        if (syswk->unk1B == 1) {
            Box2Frame_PutPartyRightBelow(syswk->app->bgWinFrame);
            Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
            syswk->app->subSeq++;
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 22);
        }
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos >= BOX2_PARTY_POS) {
            pos = 0;
        }
        Box2Ui_ChangeCursor(syswk, 3, pos);
        Box2Obj_SetCursorAnim(syswk, 6);
        Box2Main_PokeInfoPut(syswk, pos);
        syswk->app->subSeq = 0;
        Box2Obj_SetToolButtons(syswk, 9, 1);
        return 17;
    case 2:
        Box2Frame_ShowBoxListButton(syswk->app);
        syswk->app->subSeq = 0;
        return 33;
    }
    return 22;
}

// After a move: puts the cursor back, and says why a move failed
static int Box2Seq_ArrangeMoveEnd(Box2SysWork *syswk) {
    u8 pos;
    u8 n;

    Box2Main_PokeDataMove(syswk);
    syswk->unk18 = 0;
    syswk->getTray = BOX2_GET_NONE;
    if (syswk->app->moveErr == BOX2_MOVE_ERR_NONE) {
        pos = CursorMove_GetPos(syswk->app->cursorMove);
    } else {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
        Box2Ui_PlaceCursorActor(syswk, syswk->pos);
        Box2Obj_ShowCursor(syswk, 1);
        pos = syswk->pos;
    }
    syswk->app->oldCurPos = pos;
    FreePokeMoveWork(syswk);
    if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == TRUE) {
        if (syswk->nextSeq == 32) {
            Box2Ui_ChangeCursor(syswk, 4, pos);
            Box2Main_PokeInfoPut(syswk, pos);
            syswk->nextSeq = 21;
            n = 0;
            if (Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                n = 2;
            }
        } else if (pos < BOX2_PARTY_POS &&
                   Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            Box2Main_PokeInfoPut(syswk, pos);
            n = 0;
        } else {
            Box2Main_PokeInfoOff(syswk);
            n = 2;
        }
    } else if (Box2Frame_IsPartyAtRight(syswk->app->bgWinFrame) == TRUE) {
        if (syswk->nextSeq == 21) {
            pos = CursorMove_GetPos(syswk->app->cursorMove);
            if (pos > 32) {
                pos = syswk->pos;
            }
            Box2Ui_ChangeCursor(syswk, 6, pos);
            Box2Main_PokeInfoPut(syswk, pos);
            syswk->nextSeq = 32;
            n = 0;
            if (Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                n = 2;
            }
        } else if (pos < BOX2_BOXLIST_POS &&
                   Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            Box2Main_PokeInfoPut(syswk, pos);
            n = 0;
        } else {
            Box2Main_PokeInfoOff(syswk);
            n = 2;
        }
    } else if (pos < BOX2_BOXLIST_POS &&
               Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
        Box2Main_PokeInfoPut(syswk, pos);
        n = 0;
    } else {
        Box2Main_PokeInfoOff(syswk);
        n = 2;
    }
    if (syswk->moveMode == 0) {
        syswk->unk1C_6 = 1;
        n = syswk->unk21;
    }
    Box2Obj_SetButtons(syswk, 0, 1, n, 0);
    switch (syswk->app->moveErr) {
    case BOX2_MOVE_ERR_MAIL:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Obj_ShowCursor(syswk, 0);
        Box2Bmp_PrintDenyMsg(syswk, BOX2_MOVE_ERR_MAIL, 24);
        return 24;
    case BOX2_MOVE_ERR_LAST_BATTLER:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Obj_ShowCursor(syswk, 0);
        Box2Bmp_PrintDenyMsg(syswk, BOX2_MOVE_ERR_LAST_BATTLER, 24);
        return 24;
    case BOX2_MOVE_ERR_EGG:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Obj_ShowCursor(syswk, 0);
        Box2Bmp_PrintDenyMsg(syswk, BOX2_MOVE_ERR_EGG, 24);
        return 24;
    case BOX2_MOVE_ERR_BATTLE_BOX_LOCKED:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Obj_ShowCursor(syswk, 0);
        Box2Bmp_PrintBattleBoxLocked(syswk, 0);
        return 24;
    }
    if (syswk->param->mode == 2) {
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_ShowPartyButton(syswk->app);
        } else {
            Box2Frame_ShowBoxListButton(syswk->app);
        }
    }
    return syswk->nextSeq;
}

// Closes the message about a move that failed
static int Box2Seq_ArrangeMoveErrClose(Box2SysWork *syswk) {
    if (Box2Seq_TrgWait(syswk) == BOX2SEQ_TRGWAIT) {
        return 24;
    }
    if (CursorMove_IsCursorVisible(syswk->app->cursorMove) == TRUE) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, TRUE);
    }
    Box2Bmp_ClearMsgWindow(syswk->app, syswk->app->moveErr == BOX2_MOVE_ERR_BATTLE_BOX_LOCKED ? 27 : 24);
    Box2Main_PutMessageFrame(syswk);
    if (syswk->param->mode == 2) {
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_ShowPartyButton(syswk->app);
        } else {
            Box2Frame_ShowBoxListButton(syswk->app);
        }
    }
    syswk->app->moveErr = BOX2_MOVE_ERR_NONE;
    return syswk->nextSeq;
}

static int Box2Seq_ArrangeMovePartyOut(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Obj_ShowBoxNameFrame(syswk, 0, 0);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_MovePartyFrameTo(syswk, 25);
    case 1:
        Box2Frame_PutPartyRightBelow(syswk->app->bgWinFrame);
        Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
        if (syswk->unk18 == 1) {
            Box2Obj_PutPartyIconsLeftBelow(syswk, 1);
        } else {
            Box2Obj_PutPartyIconsLeftBelow(syswk, 0);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 25);
    case 2:
        Box2Ui_ChangeCursor(syswk, 6, BOX2_PARTY_POS);
        Box2Ui_SetButtonStates(syswk, BOX2_PARTY_POS);
        if (syswk->unk18 == 0) {
            Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
        }
        Box2Frame_ShowBoxListButton(syswk->app);
        syswk->app->oldCurPos = BOX2_PARTY_POS;
        syswk->app->subSeq = 0;
        return 32;
    }
    return 25;
}

// Starts picking a range
static int Box2Seq_RangeStart(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        syswk->unk1C_4 = 0;
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == FALSE) {
            syswk->trayScroll = syswk->tray;
        }
        syswk->getTray = BOX2_GET_NONE;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (syswk->pos >= BOX2_PARTY_POS) {
            syswk->pos = 0;
            Box2Main_PokeInfoPut(syswk, syswk->pos);
        }
        Box2Obj_ShowCursor(syswk, 0);
        syswk->app->subSeq++;
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            Box2Obj_ShowPokeOutline(syswk, 0);
            Box2Bmp_ClearMsgWindow(syswk->app, 24);
            Box2Main_PutMessageFrame(syswk);
            Box2Frame_ShowPartyButton(syswk->app);
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 35);
        }
    case 1:
        syswk->app->subSeq++;
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == FALSE) {
            GFL_SndSEPlay(SEQ_SE_SYS_42);
            return Box2Seq_MovePartyFrame(syswk, 35);
        }
    case 2:
        Box2Ui_ChangeCursor(syswk, 4, syswk->pos);
        Box2Obj_SetCursorAnim(syswk, 6);
        syswk->app->oldCurPos = syswk->pos;
        syswk->app->subSeq = 0;
        if (Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
            Box2Obj_SetButtons(syswk, 0, 1, 2, 0);
        } else {
            Box2Obj_SetButtons(syswk, 0, 1, 0, 0);
        }
        Box2Obj_SetToolButtons(syswk, 11, 1);
        return 36;
    }
    return 35;
}

// Picking a range: waits for a touch or for the cursor to pick or drop one
static int Box2Seq_RangeMain(Box2SysWork *syswk) {
    u32 x, y;
    u32 res;

    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) && syswk->moveMode != 0 && syswk->unk18 == 0) {
        Box2Main_ShowCursor(syswk);
        syswk->moveMode = 0;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, 37);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) && syswk->unk18 == 0) {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        Box2Seq_SetGetPos(syswk, pos, syswk->tray);
        syswk->unk13 = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_ShowCursor(syswk);
        return Box2Seq_AnimActorButton(syswk, 30, 1, 91);
    }

    if (syswk->unk18 == 0) {
        res = Box2Ui_GetTrayPokeTrigger();
        if (res != 0xffffffff) {
            Box2Main_PokeInfoPut(syswk, res);
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            Box2Obj_ShowBoxNameFrame(syswk, 0, 0);
            Box2Obj_SetRangeAnm(syswk, 1);
            Box2Obj_SetRangeRect(syswk, res, res, 0);
            syswk->app->rangeSelect.startPos = res;
            syswk->app->rangeSelect.endPos = res;
            syswk->app->unkA5B4 = FALSE;
            syswk->unk18 = 3;
            syswk->getTray = syswk->tray;
            Box2Ui_SetButtonStates(syswk, res);
            return 36;
        }
    } else if (syswk->unk18 == 3) {
        if (GFL_HIDIsTouchHeld() == TRUE) {
            res = Box2Ui_GetTrayPokeHeld();
            if (res != 0xffffffff) {
                Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, res, 0);
                syswk->app->rangeSelect.endPos = res;
            }
        } else {
            u32 width, height;

            SortRangeEnds(syswk);
            syswk->pos = Box2Obj_GetTrayRangeStart(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos);
            CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
            Box2Ui_PlaceCursorActor(syswk, syswk->pos);
            Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, 0);
            Box2Obj_GetRangeSize(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, &width, &height);
            syswk->app->rangeWidth = width;
            syswk->app->rangeHeight = height;
            Box2Main_SetRangeFlags(syswk);
            if (Box2Main_GetRangeCount(syswk->app) != 0) {
                Box2Obj_PickUpRangeIcons(syswk, syswk->pos, width, height);
                Box2Obj_ResetRangeIconPriority(syswk->app, syswk->pos, width, height, 0);
                Box2Bmp_PutRangeCount(syswk);
                syswk->unk18 = 2;
                syswk->app->oldCurPos = syswk->pos;
            } else {
                CancelRange(syswk);
            }
        }
        return 36;
    } else if (syswk->unk18 == 2 && syswk->app->unkA5B4 == FALSE) {
        res = Box2Ui_GetTrayPokeTrigger();
        if (res != 0xffffffff) {
            // The touch position's y doubles as the row counter
            for (y = 0; y < syswk->app->rangeHeight; y++) {
                if (res >= syswk->pos + y * 6 && res < syswk->app->rangeWidth + (syswk->pos + y * 6)) {
                    GFL_SndSEPlay(SEQ_SE_SYS_39);
                    GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
                    CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
                    Box2Main_PokeInfoPut(syswk, syswk->pos);
                    syswk->app->unkA5B4 = TRUE;
                    return Box2Seq_RangePickUpTouched(syswk, 36);
                }
            }
        }
    }

    if (Box2Ui_GetBoxListScrollTouch(&x, &y) == TRUE) {
        syswk->app->tpx = x;
        syswk->app->tpy = y;
        syswk->nextSeq = 36;
        AllocBoxListDragWork(syswk);
        return 15;
    }

    res = Box2Ui_UpdateCursor4(syswk);
    if (res != CURSORMOVE_NONE) {
        syswk->app->unkA5B4 = TRUE;
    }
    switch (res) {
    case 30:
        Box2Obj_ShowBoxNameFrame(syswk, 0, 0);
        if (syswk->unk18 == 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Main_PokeInfoOff(syswk);
            return Box2Seq_Goto(syswk, 105);
        }
        if (syswk->unk18 == 1) {
            CancelRange(syswk);
            Box2Main_PokeInfoOff(syswk);
        }
        break;
    case 31:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Obj_ShowBoxNameFrame(syswk, 0, 0);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        Box2Ui_PlaceCursorActor(syswk, 30);
        Box2Ui_SetButtonStates(syswk, 30);
        syswk->app->oldCurPos = 30;
        if (syswk->unk18 == 0) {
            Box2Main_PokeSelectOff(syswk);
        } else if (syswk->unk18 == 1) {
            Box2Main_PokeSelectOff(syswk);
            CancelRange(syswk);
        } else {
            Box2Obj_MoveHeldIcons(syswk);
        }
        return Box2Seq_ScrollTrayLeft(syswk, 0, 36);
    case 32:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Obj_ShowBoxNameFrame(syswk, 0, 0);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        Box2Ui_PlaceCursorActor(syswk, 30);
        Box2Ui_SetButtonStates(syswk, 30);
        syswk->app->oldCurPos = 30;
        if (syswk->unk18 == 0) {
            Box2Main_PokeSelectOff(syswk);
        } else if (syswk->unk18 == 1) {
            Box2Main_PokeSelectOff(syswk);
            CancelRange(syswk);
        } else {
            Box2Obj_MoveHeldIcons(syswk);
        }
        return Box2Seq_ScrollTrayRight(syswk, 0, 36);
    case 39:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->unk18 == 0) {
            Box2Main_PokeInfoOff(syswk);
        } else if (syswk->unk18 == 1) {
            CancelRange(syswk);
            Box2Main_PokeInfoOff(syswk);
        }
        return Box2Seq_AnimFrameButton(syswk, 10, Box2Seq_Goto(syswk, 40));
    case 40: {
        u8 pos;

        if (syswk->unk18 != 0) {
            break;
        }
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos >= BOX2_PARTY_POS ||
            Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
            break;
        }
        syswk->pos = pos;
        syswk->getTray = syswk->tray;
        syswk->unk13 = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, 8, 1, 89);
    }
    case 41:
        if (syswk->unk18 != 0) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = 0;
        return Box2Seq_AnimActorButton(syswk, 6, 9, Box2Seq_Goto(syswk, 37));
    case 42:
        if (syswk->unk18 != 0) {
            break;
        }
        syswk->moveMode = 0;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, 37);
    case 43:
        if (syswk->unk18 != 0) {
            break;
        }
        syswk->moveMode = 1;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, 37);
    case 44:
        break;
    case 45:
        if (syswk->unk18 != 0) {
            break;
        }
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        Box2Seq_SetGetPos(syswk, pos, syswk->tray);
        syswk->unk13 = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, 30, 1, 91);
    case CURSORMOVE_CANCEL:
        if (syswk->unk18 == 1) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            if (pos < BOX2_PARTY_POS) {
                Box2Main_PokeInfoPut(syswk, pos);
            } else {
                Box2Main_PokeInfoOff(syswk);
            }
            CancelRange(syswk);
            break;
        }
        if (syswk->unk18 != 0) {
            Box2Obj_SetRangeAnm(syswk, 0);
            return Box2Seq_RangeDropKey(syswk, BOX2_GET_NONE, 36);
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = 0;
        return Box2Seq_AnimActorButton(syswk, 6, 9, Box2Seq_Goto(syswk, 37));
    case CURSORMOVE_UNK_8:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 34) {
            if (syswk->unk18 == 1) {
                Box2Main_PokeSelectOff(syswk);
                CancelRange(syswk);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_StartBoxListScroll(syswk, -1);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollRight, 36);
        }
        break;
    case CURSORMOVE_UNK_7:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 37) {
            if (syswk->unk18 == 1) {
                Box2Main_PokeSelectOff(syswk);
                CancelRange(syswk);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_StartBoxListScroll(syswk, 1);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollLeft, 36);
        }
        break;
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            if (syswk->unk18 == 1) {
                CancelRange(syswk);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return Box2Seq_ScrollTrayLeft(syswk, 0, 36);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            if (syswk->unk18 == 1) {
                CancelRange(syswk);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return Box2Seq_ScrollTrayRight(syswk, 0, 36);
        }
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_SELECT1);
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (syswk->unk18 == 0) {
            if (pos < BOX2_PARTY_POS) {
                Box2Main_PokeInfoPut(syswk, pos);
            } else {
                Box2Main_PokeInfoOff(syswk);
            }
        } else if (syswk->unk18 == 1 && pos < BOX2_PARTY_POS) {
            Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, pos, 0);
        }
        if (pos >= 34 && pos <= 37) {
            Box2Obj_ShowBoxNameFrame(syswk, pos - 34, 1);
        } else {
            Box2Obj_ShowBoxNameFrame(syswk, 0, 0);
        }
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, 36));
    }
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case 33:
        if (syswk->unk18 == 1) {
            CancelRange(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_StartBoxListScroll(syswk, -1);
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollRight, 36);
    case 38:
        if (syswk->unk18 == 1) {
            CancelRange(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_StartBoxListScroll(syswk, 1);
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListScrollLeft, 36);
    case 34:
    case 35:
    case 36:
    case 37: {
        // The button's slot in the box list
        u32 slot = res - 34;

        Box2Obj_ShowBoxNameFrame(syswk, slot, 1);
        if (syswk->unk18 == 2) {
            return Box2Seq_RangeDropKey(syswk, slot + 36, 36);
        }
        if (syswk->unk18 == 1) {
            CancelRange(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeInfoOff(syswk);
        syswk->app->unkA55F = syswk->trayScroll + slot;
        if (syswk->app->unkA55F >= syswk->trayMax) {
            syswk->app->unkA55F -= syswk->trayMax;
        }
        if (syswk->app->unkA55F != syswk->tray) {
            return Box2Seq_ScrollToPickedBox(syswk, 36);
        }
        break;
    }
    case CURSORMOVE_NONE:
        break;
    default:
        Box2Obj_ShowBoxNameFrame(syswk, 0, 0);
        if (syswk->unk18 == 1) {
            u32 width, height;

            syswk->app->rangeSelect.endPos = res;
            SortRangeEnds(syswk);
            syswk->pos = Box2Obj_GetTrayRangeStart(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos);
            CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
            Box2Ui_PlaceCursorActor(syswk, syswk->pos);
            Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, 0);
            Box2Obj_GetRangeSize(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, &width, &height);
            syswk->app->rangeWidth = width;
            syswk->app->rangeHeight = height;
            Box2Main_SetRangeFlags(syswk);
            if (Box2Main_GetRangeCount(syswk->app) != 0) {
                Box2Obj_PickUpRangeIcons(syswk, syswk->pos, width, height);
                Box2Obj_ResetRangeIconPriority(syswk->app, syswk->pos, width, height, 0);
                Box2Bmp_PutRangeCount(syswk);
                syswk->unk18 = 2;
                syswk->app->oldCurPos = syswk->pos;
                return Box2Seq_RangePickUpKey(syswk, syswk->pos, 36);
            }
            CancelRange(syswk);
        } else if (syswk->unk18 == 2) {
            if (Box2Main_RangePutCheck(syswk, syswk->tray, res) != FALSE) {
                Box2Obj_SetRangeAnm(syswk, 0);
                return Box2Seq_RangeDropKey(syswk, res, 36);
            }
            GFL_SndSEPlay(SEQ_SE_BEEP);
        } else {
            syswk->getTray = syswk->tray;
            syswk->pos = res;
            syswk->app->rangeSelect.startPos = res;
            syswk->app->rangeSelect.endPos = res;
            syswk->app->unkA5B4 = TRUE;
            Box2Obj_SetRangeRect(syswk, res, res, 0);
            Box2Obj_SetRangeAnm(syswk, 1);
            syswk->unk18 = 1;
            syswk->app->rangeWidth = 1;
            syswk->app->rangeHeight = 1;
            Box2Ui_SetButtonStates(syswk, res);
        }
        break;
    }
    return 36;
}

// Range mode: brings the party's frame out
static int Box2Seq_RangePartyOut(Box2SysWork *syswk) {
    u8 pos;

    switch (syswk->app->subSeq) {
    case 0:
        Box2Main_ClearRangeFlags(syswk);
        Box2Obj_ShowCursor(syswk, 0);
        Box2Obj_ShowBoxNameFrame(syswk, 0, 0);
        if (CursorMove_GetPos(syswk->app->cursorMove) >= BOX2_PARTY_POS) {
            Box2Main_PokeInfoOff(syswk);
        }
        if (syswk->moveMode == 0) {
            Box2Obj_SetButtons(syswk, 0, 0, 1, 0);
            GFL_SndSEPlay(SEQ_SE_SYS_42);
            syswk->app->subSeq++;
            return Box2Seq_MovePartyFrameTo(syswk, 37);
        }
        return 20;
    case 1:
        if (syswk->unk1B == 1) {
            Box2Frame_PutPartyRightBelow(syswk->app->bgWinFrame);
            Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
            syswk->app->subSeq++;
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 37);
        }
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos >= BOX2_PARTY_POS) {
            pos = 0;
        }
        Box2Ui_ChangeCursor(syswk, 3, pos);
        Box2Obj_SetCursorAnim(syswk, 6);
        Box2Obj_SetToolButtons(syswk, 9, 1);
        Box2Main_PokeInfoPut(syswk, pos);
        syswk->app->subSeq = 0;
        return 17;
    case 2:
        Box2Frame_ShowBoxListButton(syswk->app);
        syswk->app->subSeq = 0;
        return 33;
    }
    return 37;
}

// After a move in range mode: puts the cursor back, and says why a move failed
static int Box2Seq_RangeMoveEnd(Box2SysWork *syswk) {
    u8 pos;
    u8 n;

    Box2Main_PokeDataMove(syswk);
    syswk->unk18 = 0;
    syswk->getTray = BOX2_GET_NONE;
    if (syswk->app->moveErr == BOX2_MOVE_ERR_NONE) {
        pos = CursorMove_GetPos(syswk->app->cursorMove);
    } else {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
        Box2Ui_PlaceCursorActor(syswk, syswk->pos);
        Box2Obj_ShowCursor(syswk, 1);
        pos = syswk->pos;
    }
    syswk->app->oldCurPos = pos;
    FreePokeMoveWork(syswk);
    if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == TRUE) {
        if (syswk->nextSeq == 42) {
            Box2Ui_ChangeCursor(syswk, 4, pos);
            Box2Main_PokeInfoPut(syswk, pos);
            syswk->nextSeq = 36;
            n = 0;
            if (Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                n = 2;
            }
        } else if (pos < BOX2_PARTY_POS &&
                   Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            Box2Main_PokeInfoPut(syswk, pos);
            n = 0;
        } else {
            Box2Main_PokeInfoOff(syswk);
            n = 2;
        }
    } else if (Box2Frame_IsPartyAtRight(syswk->app->bgWinFrame) == TRUE) {
        if (syswk->nextSeq == 36) {
            pos = CursorMove_GetPos(syswk->app->cursorMove);
            if (pos > 32) {
                pos = syswk->pos;
            }
            Box2Ui_ChangeCursor(syswk, 6, pos);
            Box2Main_PokeInfoPut(syswk, pos);
            syswk->nextSeq = 42;
            n = 0;
            if (Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                n = 2;
            }
        } else if (pos < BOX2_BOXLIST_POS &&
                   Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            Box2Main_PokeInfoPut(syswk, pos);
            n = 0;
        } else {
            Box2Main_PokeInfoOff(syswk);
            n = 2;
        }
    } else if (pos < BOX2_BOXLIST_POS &&
               Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
        Box2Main_PokeInfoPut(syswk, pos);
        n = 0;
    } else {
        Box2Main_PokeInfoOff(syswk);
        n = 2;
    }
    if (syswk->moveMode == 0) {
        syswk->unk1C_6 = 1;
        n = syswk->unk21;
    }
    Box2Obj_SetButtons(syswk, 0, 1, n, 0);
    Box2Obj_SetRangeAnm(syswk, 0);
    switch (syswk->app->moveErr) {
    case BOX2_MOVE_ERR_MAIL:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Obj_ShowCursor(syswk, 0);
        Box2Bmp_PrintDenyMsg(syswk, BOX2_MOVE_ERR_MAIL, 24);
        return 39;
    case BOX2_MOVE_ERR_LAST_BATTLER:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Obj_ShowCursor(syswk, 0);
        Box2Bmp_PrintDenyMsg(syswk, BOX2_MOVE_ERR_LAST_BATTLER, 24);
        return 39;
    case BOX2_MOVE_ERR_EGG:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Obj_ShowCursor(syswk, 0);
        Box2Bmp_PrintDenyMsg(syswk, BOX2_MOVE_ERR_EGG, 24);
        return 39;
    case BOX2_MOVE_ERR_BATTLE_BOX_LOCKED:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Obj_ShowCursor(syswk, 0);
        Box2Bmp_PrintBattleBoxLocked(syswk, 0);
        return 39;
    }
    if (syswk->param->mode == 2) {
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_ShowPartyButton(syswk->app);
        } else {
            Box2Frame_ShowBoxListButton(syswk->app);
        }
    }
    return syswk->nextSeq;
}

// Closes the message about a move in range mode that failed
static int Box2Seq_RangeMoveErrClose(Box2SysWork *syswk) {
    if (Box2Seq_TrgWait(syswk) == BOX2SEQ_TRGWAIT) {
        return 39;
    }
    if (CursorMove_IsCursorVisible(syswk->app->cursorMove) == TRUE) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, TRUE);
    }
    Box2Bmp_ClearMsgWindow(syswk->app, syswk->app->moveErr == BOX2_MOVE_ERR_BATTLE_BOX_LOCKED ? 27 : 24);
    Box2Main_PutMessageFrame(syswk);
    if (syswk->param->mode == 2) {
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_ShowPartyButton(syswk->app);
        } else {
            Box2Frame_ShowBoxListButton(syswk->app);
        }
    }
    syswk->app->moveErr = BOX2_MOVE_ERR_NONE;
    return syswk->nextSeq;
}

// Range mode: puts the party's frame away
static int Box2Seq_RangePartyAway(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Obj_ShowBoxNameFrame(syswk, 0, 0);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_MovePartyFrameTo(syswk, 40);
    case 1:
        Box2Frame_PutPartyRightBelow(syswk->app->bgWinFrame);
        Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
        if (syswk->unk18 == 2) {
            Box2Obj_PutPartyIconsLeftBelow(syswk, 1);
            Box2Obj_HidePartyRangeIcons(syswk);
        } else if (syswk->unk18 == 0) {
            Box2Obj_PutPartyIconsLeftBelow(syswk, 0);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 40);
    case 2:
        Box2Ui_ChangeCursor(syswk, 6, BOX2_PARTY_POS);
        Box2Ui_SetButtonStates(syswk, BOX2_PARTY_POS);
        if (syswk->unk18 == 0) {
            Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
        }
        Box2Frame_ShowBoxListButton(syswk->app);
        syswk->app->oldCurPos = BOX2_PARTY_POS;
        if (syswk->unk18 == 2) {
            Box2Obj_ShowRangeIcons(syswk, 0);
        }
        syswk->app->subSeq = 0;
        return 42;
    }
    return 40;
}

// Range mode: brings the party's frame in and puts the menu away
static int Box2Seq_RangePartyIn(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        syswk->unk1C_4 = 0;
        if (Box2Frame_IsPartyAtRight(syswk->app->bgWinFrame) == TRUE) {
            syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
            syswk->app->subSeq = 2;
            break;
        }
        if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
            if (syswk->pos >= 6) {
                syswk->pos = BOX2_PARTY_POS;
            } else {
                syswk->pos += BOX2_PARTY_POS;
            }
        } else if (syswk->pos >= BOX2_PARTY_POS) {
            syswk->pos = 0;
        }
        syswk->getTray = BOX2_GET_NONE;
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        Box2Bmp_ClearMsgWindow(syswk->app, 24);
        Box2Main_PutMessageFrame(syswk);
        Box2Frame_ShowBoxListButton(syswk->app);
        Box2Obj_ShowPokeOutline(syswk, 0);
        Box2Obj_ShowCursor(syswk, 0);
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 41);
    case 1:
        if (syswk->param->mode == 4 && syswk->param->battleBoxLocked == 1) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
            Box2Bmp_PrintBattleBoxLocked(syswk, 0);
            syswk->moveMode = 0;
            syswk->app->subSeq = 0;
            syswk->nextSeq = 53;
            return BOX2SEQ_TRGWAIT;
        }
        syswk->app->subSeq++;
        if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_SlidePartyRight(syswk->app->bgWinFrame);
        } else {
            Box2Frame_PutPartyRightBelow(syswk->app->bgWinFrame);
            Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
            Box2Obj_PutPartyIconsLeftBelow(syswk, 0);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 41);
    case 2:
        Box2Obj_SetIconsBlend(syswk, 1, 0);
        Box2Obj_ShowScrollArrows(syswk->app, 1);
        Box2Ui_ChangeCursor(syswk, 6, syswk->pos);
        Box2Obj_SetCursorAnim(syswk, 6);
        syswk->app->oldCurPos = syswk->pos;
        syswk->app->subSeq = 0;
        if (Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
            Box2Obj_SetButtons(syswk, 0, 1, 2, 0);
        } else {
            Box2Obj_SetButtons(syswk, 0, 1, 0, 0);
        }
        if (syswk->param->mode == 4) {
            Box2Frame_HidePartyButton(syswk->app);
            Box2Frame_HideBoxListButton(syswk->app);
            CursorMove_DisablePos(syswk->app->cursorMove, 39);
        }
        Box2Obj_SetToolButtons(syswk, 11, 1);
        return 42;
    }
    return 41;
}

// Picking a range with the party out: waits for a touch or for the cursor to pick or drop one
static int Box2Seq_RangePartyMain(Box2SysWork *syswk) {
    u32 i;
    u32 res;

    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) && syswk->unk18 == 0) {
        Box2Main_ShowCursor(syswk);
        syswk->moveMode = 0;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, 43);
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) {
        if (syswk->unk18 == 0) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            Box2Seq_SetGetPos(syswk, pos, syswk->tray);
            syswk->unk13 = 4;
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Main_ShowCursor(syswk);
            return Box2Seq_AnimActorButton(syswk, 30, 1, 91);
        }
        return 42;
    }

    if (syswk->unk18 == 0) {
        res = Box2Ui_GetTrayPokeTrigger();
        if (res != 0xffffffff) {
            Box2Main_PokeInfoPut(syswk, res);
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            Box2Obj_SetRangeAnm(syswk, 1);
            Box2Obj_SetRangeRect(syswk, res, res, 0);
            syswk->app->rangeSelect.startPos = (u8)res;
            syswk->app->rangeSelect.endPos = (u8)res;
            syswk->app->unkA5B4 = FALSE;
            syswk->unk18 = 3;
            syswk->getTray = syswk->tray;
            Box2Ui_SetButtonStates(syswk, res);
            return 42;
        }
        res = Box2Ui_GetPartyPokeTrigger();
        if (res != 0xffffffff) {
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            Box2Obj_SetRangeAnm(syswk, 1);
            Box2Obj_SetRangeRect(syswk, res + BOX2_PARTY_POS, res + BOX2_PARTY_POS, 0);
            res += BOX2_PARTY_POS;
            syswk->app->rangeSelect.startPos = (u8)res;
            syswk->app->rangeSelect.endPos = (u8)res;
            syswk->app->unkA5B4 = FALSE;
            syswk->unk18 = 4;
            syswk->getTray = BOX2_GET_NONE;
            Box2Ui_SetButtonStates(syswk, res);
            return 42;
        }
    } else if (syswk->unk18 == 3) {
        if (GFL_HIDIsTouchHeld() == TRUE) {
            res = Box2Ui_GetTrayPokeHeld();
            if (res != 0xffffffff) {
                Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, res, 0);
                syswk->app->rangeSelect.endPos = res;
            }
        } else {
            u32 width, height;

            SortRangeEnds(syswk);
            syswk->pos = Box2Obj_GetTrayRangeStart(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos);
            CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
            Box2Ui_PlaceCursorActor(syswk, syswk->pos);
            Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, 0);
            Box2Obj_GetRangeSize(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, &width, &height);
            syswk->app->rangeWidth = width;
            syswk->app->rangeHeight = height;
            Box2Main_SetRangeFlags(syswk);
            if (Box2Main_GetRangeCount(syswk->app) != 0) {
                Box2Obj_PickUpRangeIcons(syswk, syswk->pos, width, height);
                Box2Obj_ResetRangeIconPriority(syswk->app, syswk->pos, width, height, 0);
                Box2Bmp_PutRangeCount(syswk);
                syswk->unk18 = 2;
                syswk->app->oldCurPos = syswk->pos;
            } else {
                CancelRange(syswk);
            }
        }
        return 42;
    } else if (syswk->unk18 == 4) {
        if (GFL_HIDIsTouchHeld() == TRUE) {
            if (Box2Ui_IsPartyRightHeld() == TRUE) {
                res = Box2Ui_GetPartyPokeHeld();
                if (res != 0xffffffff) {
                    Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, res + BOX2_PARTY_POS, 0);
                    syswk->app->rangeSelect.endPos = res + BOX2_PARTY_POS;
                }
            }
        } else {
            u32 width, height;

            SortRangeEnds(syswk);
            syswk->pos = Box2Obj_GetPartyRangeStart(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos);
            CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
            Box2Ui_PlaceCursorActor(syswk, syswk->pos);
            Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, 0);
            Box2Obj_GetRangeSize(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, &width, &height);
            syswk->app->rangeWidth = width;
            syswk->app->rangeHeight = height;
            Box2Main_SetRangeFlags(syswk);
            if (Box2Main_GetRangeCount(syswk->app) != 0) {
                Box2Obj_PickUpRangeIcons(syswk, syswk->pos, width, height);
                Box2Obj_ResetRangeIconPriority(syswk->app, syswk->pos, width, height, 0);
                Box2Bmp_PutRangeCount(syswk);
                syswk->unk18 = 2;
                syswk->app->oldCurPos = syswk->pos;
            } else {
                CancelRange(syswk);
            }
        }
        return 42;
    } else if (syswk->unk18 == 2 && syswk->app->unkA5B4 == FALSE) {
        if (syswk->pos < BOX2_PARTY_POS) {
            res = Box2Ui_GetTrayPokeTrigger();
            if (res != 0xffffffff) {
                for (i = 0; i < syswk->app->rangeHeight; i++) {
                    if (res >= syswk->pos + i * 6 && res < syswk->app->rangeWidth + (syswk->pos + i * 6)) {
                        GFL_SndSEPlay(SEQ_SE_SYS_39);
                        GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
                        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
                        Box2Main_PokeInfoPut(syswk, syswk->pos);
                        syswk->app->unkA5B4 = TRUE;
                        return Box2Seq_RangePickUpTouched(syswk, 42);
                    }
                }
            }
        } else {
            res = Box2Ui_GetPartyPokeTrigger();
            if (res != 0xffffffff) {
                // The range's first slot in the party
                u32 top = syswk->pos - BOX2_PARTY_POS;

                for (i = 0; i < syswk->app->rangeHeight; i++) {
                    if (res >= top + i * 2 && res < syswk->app->rangeWidth + (top + i * 2)) {
                        GFL_SndSEPlay(SEQ_SE_SYS_39);
                        Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos,
                                             1);
                        GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
                        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
                        Box2Main_PokeInfoPut(syswk, syswk->pos);
                        syswk->app->unkA5B4 = TRUE;
                        return Box2Seq_RangePickUpTouched(syswk, 42);
                    }
                }
            }
        }
    }

    res = Box2Ui_UpdateCursor6(syswk);
    if (res != CURSORMOVE_NONE) {
        syswk->app->unkA5B4 = TRUE;
    }
    switch (res) {
    case 36:
        if (syswk->unk18 == 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Main_PokeInfoOff(syswk);
            return Box2Seq_Goto(syswk, 105);
        }
        if (syswk->unk18 == 1) {
            CancelRange(syswk);
            Box2Main_PokeInfoOff(syswk);
        }
        break;
    case 37:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
        Box2Ui_PlaceCursorActor(syswk, BOX2_BOXLIST_POS);
        Box2Ui_SetButtonStates(syswk, BOX2_BOXLIST_POS);
        syswk->app->oldCurPos = BOX2_BOXLIST_POS;
        if (syswk->unk18 == 0) {
            Box2Main_PokeSelectOff(syswk);
        } else if (syswk->unk18 == 1) {
            Box2Main_PokeSelectOff(syswk);
            CancelRange(syswk);
        } else {
            Box2Obj_MoveHeldIcons(syswk);
        }
        return Box2Seq_ScrollTrayLeft(syswk, 0, 42);
    case 38:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
        Box2Ui_PlaceCursorActor(syswk, BOX2_BOXLIST_POS);
        Box2Ui_SetButtonStates(syswk, BOX2_BOXLIST_POS);
        syswk->app->oldCurPos = BOX2_BOXLIST_POS;
        if (syswk->unk18 == 0) {
            Box2Main_PokeSelectOff(syswk);
        } else if (syswk->unk18 == 1) {
            Box2Main_PokeSelectOff(syswk);
            CancelRange(syswk);
        } else {
            Box2Obj_MoveHeldIcons(syswk);
        }
        return Box2Seq_ScrollTrayRight(syswk, 0, 42);
    case 39:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->unk18 == 0) {
            Box2Main_PokeInfoOff(syswk);
        } else if (syswk->unk18 == 1) {
            CancelRange(syswk);
            Box2Main_PokeInfoOff(syswk);
        }
        return Box2Seq_AnimFrameButton(syswk, 11, Box2Seq_Goto(syswk, 44));
    case 40: {
        u8 pos;

        if (syswk->unk18 != 0) {
            break;
        }
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos >= BOX2_BOXLIST_POS ||
            Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
            break;
        }
        syswk->pos = pos;
        syswk->getTray = syswk->tray;
        syswk->unk13 = 4;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, 8, 1, 89);
    }
    case 41:
        if (syswk->unk18 != 0) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = 0;
        return Box2Seq_AnimActorButton(syswk, 6, 9, Box2Seq_Goto(syswk, 43));
    case 42:
        if (syswk->unk18 != 0) {
            break;
        }
        syswk->moveMode = 0;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, 43);
    case 43:
        if (syswk->unk18 != 0) {
            break;
        }
        syswk->moveMode = 1;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, 43);
    case 44:
        break;
    case 45: {
        u8 pos;

        if (syswk->unk18 != 0) {
            break;
        }
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        Box2Seq_SetGetPos(syswk, pos, syswk->tray);
        syswk->unk13 = 4;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, 30, 1, 91);
    }
    case CURSORMOVE_CANCEL:
        if (syswk->unk18 == 1) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            if (pos < BOX2_BOXLIST_POS) {
                Box2Main_PokeInfoPut(syswk, pos);
            } else {
                Box2Main_PokeInfoOff(syswk);
            }
            CancelRange(syswk);
            break;
        }
        if (syswk->unk18 != 0) {
            Box2Obj_SetRangeAnm(syswk, 0);
            return Box2Seq_RangeDropKey(syswk, BOX2_GET_NONE, 42);
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = 0;
        return Box2Seq_AnimActorButton(syswk, 6, 9, Box2Seq_Goto(syswk, 43));
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BOXLIST_POS) {
            if (syswk->unk18 == 1) {
                Box2Main_PokeSelectOff(syswk);
                CancelRange(syswk);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return Box2Seq_ScrollTrayLeft(syswk, 0, 42);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BOXLIST_POS) {
            if (syswk->unk18 == 1) {
                Box2Main_PokeSelectOff(syswk);
                CancelRange(syswk);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return Box2Seq_ScrollTrayRight(syswk, 0, 42);
        }
        break;
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_SELECT1);
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (syswk->unk18 == 0) {
            if (pos < BOX2_BOXLIST_POS) {
                Box2Main_PokeInfoPut(syswk, pos);
            } else {
                Box2Main_PokeInfoOff(syswk);
            }
        } else if (syswk->unk18 == 1) {
            // The range stays in the tray or in the party it started in
            if (syswk->app->rangeSelect.startPos < BOX2_PARTY_POS) {
                if (pos < BOX2_PARTY_POS) {
                    Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, pos, 0);
                }
            } else if (pos >= BOX2_PARTY_POS && pos < BOX2_BOXLIST_POS) {
                Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, pos, 0);
            }
        }
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, 42));
    }
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
    case CURSORMOVE_NONE:
        break;
    default:
        if (syswk->unk18 == 1) {
            u32 width, height;

            if ((syswk->app->rangeSelect.endPos < BOX2_PARTY_POS && res >= BOX2_PARTY_POS) ||
                (syswk->app->rangeSelect.endPos >= BOX2_PARTY_POS && res < BOX2_PARTY_POS)) {
                CancelRange(syswk);
                Box2Main_PokeInfoPut(syswk, CursorMove_GetPos(syswk->app->cursorMove));
                break;
            }
            syswk->app->rangeSelect.endPos = res;
            SortRangeEnds(syswk);
            if (syswk->app->rangeSelect.startPos < BOX2_PARTY_POS) {
                syswk->pos =
                    Box2Obj_GetTrayRangeStart(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos);
            } else {
                syswk->pos =
                    Box2Obj_GetPartyRangeStart(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos);
            }
            CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
            Box2Ui_PlaceCursorActor(syswk, syswk->pos);
            Box2Obj_SetRangeRect(syswk, syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, 0);
            Box2Obj_GetRangeSize(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, &width, &height);
            syswk->app->rangeWidth = width;
            syswk->app->rangeHeight = height;
            Box2Main_SetRangeFlags(syswk);
            if (Box2Main_GetRangeCount(syswk->app) != 0) {
                Box2Obj_PickUpRangeIcons(syswk, syswk->pos, width, height);
                Box2Obj_ResetRangeIconPriority(syswk->app, syswk->pos, width, height, 0);
                Box2Bmp_PutRangeCount(syswk);
                syswk->unk18 = 2;
                syswk->app->oldCurPos = syswk->pos;
                return Box2Seq_RangePickUpKey(syswk, syswk->pos, 42);
            }
            CancelRange(syswk);
        } else if (syswk->unk18 == 2) {
            if (Box2Main_RangePutCheck(syswk, syswk->tray, res) != FALSE) {
                Box2Obj_SetRangeAnm(syswk, 0);
                return Box2Seq_RangeDropKey(syswk, res, 42);
            }
            GFL_SndSEPlay(SEQ_SE_BEEP);
        } else {
            if (res < BOX2_PARTY_POS) {
                syswk->getTray = syswk->tray;
            }
            syswk->pos = (u8)res;
            syswk->app->rangeSelect.startPos = (u8)res;
            syswk->app->rangeSelect.endPos = (u8)res;
            syswk->app->unkA5B4 = TRUE;
            Box2Obj_SetRangeRect(syswk, res, res, 0);
            Box2Obj_SetRangeAnm(syswk, 1);
            syswk->unk18 = 1;
            syswk->app->rangeWidth = 1;
            syswk->app->rangeHeight = 1;
            Box2Ui_SetButtonStates(syswk, res);
        }
        break;
    }
    return 42;
}

// Range mode: puts the party's frame away or out
static int Box2Seq_RangePartySwitch(Box2SysWork *syswk) {
    u8 pos;

    switch (syswk->app->subSeq) {
    case 0:
        Box2Main_ClearRangeFlags(syswk);
        if (syswk->moveMode == 0) {
            Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
            Box2Obj_SetButtons(syswk, 0, 0, 1, 0);
            if (syswk->unk1B == 0) {
                Box2Frame_SlidePartyDown(syswk->app->bgWinFrame);
                syswk->app->subSeq = 2;
            } else {
                Box2Obj_SetIconsBlend(syswk, 1, 1);
                Box2Obj_ShowScrollArrows(syswk->app, 0);
                Box2Frame_SlidePartyLeft(syswk->app->bgWinFrame);
                syswk->app->subSeq = 1;
            }
            GFL_SndSEPlay(SEQ_SE_SYS_42);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 43);
        }
        return 31;
    case 1:
        syswk->app->subSeq = 0;
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos < BOX2_PARTY_POS || pos >= BOX2_BOXLIST_POS) {
            pos = 0;
        } else {
            pos -= BOX2_PARTY_POS;
        }
        Box2Main_PokeInfoPut(syswk, pos + BOX2_PARTY_POS);
        Box2Obj_SetCursorAnim(syswk, 6);
        Box2Obj_SetToolButtons(syswk, 9, 1);
        if (syswk->param->mode == 2) {
            Box2Ui_ChangeCursor(syswk, 5, pos);
            return 27;
        }
        Box2Frame_ShowBoxListButton(syswk->app);
        Box2Ui_ChangeCursor(syswk, 14, pos);
        return 49;
    case 2:
        syswk->app->subSeq = 0;
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos >= BOX2_PARTY_POS) {
            pos = 0;
        }
        Box2Main_PokeInfoPut(syswk, pos);
        Box2Frame_ShowPartyButton(syswk->app);
        Box2Obj_SetCursorAnim(syswk, 6);
        Box2Obj_SetToolButtons(syswk, 9, 1);
        if (syswk->param->mode == 4) {
            Box2Ui_ChangeCursor(syswk, 13, pos);
            return 45;
        }
        Box2Ui_ChangeCursor(syswk, 3, pos);
        return 17;
    }
    return 43;
}

// Range mode: closes the party's frame
static int Box2Seq_RangePartyClose(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Frame_SlidePartyDown(syswk->app->bgWinFrame);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 44);
    case 1:
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == FALSE) {
            syswk->trayScroll = syswk->tray;
        }
        syswk->app->subSeq++;
        return Box2Seq_MovePartyFrame(syswk, 44);
    case 2:
        Box2Ui_ChangeCursor(syswk, 4, 39);
        Box2Frame_ShowPartyButton(syswk->app);
        syswk->app->oldCurPos = 39;
        syswk->app->subSeq = 0;
        return 36;
    }
    return 44;
}

// Brings the party's frame out
static int Box2Seq_ArrangePartyOut(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        Box2Main_PokeSelectOff(syswk);
        Box2Main_PutMessageFrame(syswk);
        Box2Obj_ShowScrollArrows(syswk->app, 0);
        Box2Obj_SetIconsBlend(syswk, 1, 1);
        Box2Obj_ShowCursor(syswk, 0);
        Box2Frame_PutPartyLeftBelow(syswk->app->bgWinFrame);
        Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
        Box2Obj_PutPartyIconsLeftBelow(syswk, 0);
        if (syswk->param->mode == 3) {
            Box2Obj_SetIconsBlend(syswk, 0x82, 1);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 26);
    case 1:
        syswk->app->subSeq = 0;
        Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
        Box2Frame_ShowBoxListButton(syswk->app);
        if (syswk->param->mode == 3) {
            Box2Ui_ChangeCursor(syswk, 8, 0);
            return 80;
        }
        Box2Ui_ChangeCursor(syswk, 5, 0);
        return 27;
    }
    return 26;
}

// The party's main state: waits for a touch or for the cursor to pick something
static int Box2Seq_ArrangePartyMain(Box2SysWork *syswk) {
    u32 res;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        syswk->unk1B = 1;
        return Box2Seq_NextMoveMode(syswk, 31);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) && Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == FALSE) {
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove) + BOX2_PARTY_POS;
        syswk->unk13 = 4;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_ShowCursor(syswk);
        return Box2Seq_AnimActorButton(syswk, 30, 1, 91);
    }

    res = Box2Ui_GetMenuButtonTrigger();
    if (res != 0xffffffff) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
            if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == FALSE) {
                Box2Bmp_OpenMenu(syswk, sMenu71f0, 6);
            }
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            return Box2Seq_ArrangePartyPickUpTouched(syswk, res + BOX2_PARTY_POS);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        Box2Ui_PlaceCursorActor(syswk, res);
        Box2Main_PokeInfoOff(syswk);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            Box2Obj_ShowPokeOutline(syswk, 0);
            Box2Bmp_ClearMsgWindow(syswk->app, 24);
            Box2Main_PutMessageFrame(syswk);
            Box2Frame_ShowBoxListButton(syswk->app);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 27);
        }
        return 27;
    }

    res = Box2Ui_UpdateCursor5(syswk);
    switch (res) {
    case 6:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, 10, Box2Seq_Goto(syswk, 28));
    case 7:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->closed = TRUE;
        return Box2Seq_AnimActorButton(syswk, 7, 8, 16);
    case 8:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return Box2Seq_AnimActorButton(syswk, 6, 9, Box2Seq_Goto(syswk, 28));
    case 9:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk1B = 1;
        syswk->moveMode = 0;
        return Box2Seq_AnimFrameButton(syswk, 0, Box2Seq_Goto(syswk, 31));
    case 10:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->getTray = syswk->tray;
        syswk->unk13 = 1;
        return Box2Seq_AnimFrameButton(syswk, 1, 89);
    case 11:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk13 = 1;
        return Box2Seq_AnimFrameButton(syswk, 2, Box2Seq_Goto(syswk, 90));
    case 12:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, 3, Box2Seq_Goto(syswk, 97));
    case 13:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, 4, Box2Seq_Goto(syswk, 101));
    case 14: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos - BOX2_PARTY_POS;
        Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        syswk->app->oldCurPos = pos;
        return Box2Seq_AnimFrameButton(syswk, 5, 29);
    }
    case 15:
        break;
    case 16:
        syswk->unk1B = 1;
        syswk->moveMode = 1;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_SetMoveMode(syswk, 31);
    case 17:
        syswk->unk1B = 1;
        syswk->moveMode = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_SetMoveMode(syswk, 41);
    case 18:
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove) + BOX2_PARTY_POS;
        syswk->unk13 = 4;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, 30, 1, 91);
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        if (pos < 6) {
            Box2Main_PokeInfoPut(syswk, pos + BOX2_PARTY_POS);
        } else if (pos == 6) {
            Box2Main_PokeInfoOff(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, 27));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos - BOX2_PARTY_POS;

            Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            syswk->app->oldCurPos = pos;
            return Box2Seq_AnimFrameButton(syswk, 5, 29);
        }
        return Box2Seq_AnimActorButton(syswk, 6, 9, Box2Seq_Goto(syswk, 28));
    case CURSORMOVE_NONE:
    case CURSORMOVE_SCROLL_R:
    case CURSORMOVE_SCROLL_L:
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Bmp_OpenMenu(syswk, sMenu71f0, 6);
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            Box2Ui_StartCursorMove(syswk->app, 9, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, 9);
            syswk->app->oldCurPos = 9;
            return Box2Seq_OpenPartyPokeMenu(syswk, res + BOX2_PARTY_POS, 27);
        }
        break;
    }
    return 27;
}

// Puts the party's frame away
static int Box2Seq_ArrangePartyAway(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        Box2Main_PokeSelectOff(syswk);
        Box2Main_PutMessageFrame(syswk);
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Frame_SlidePartyDown(syswk->app->bgWinFrame);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 28);
    case 1:
        syswk->app->subSeq = 0;
        Box2Obj_ShowScrollArrows(syswk->app, 1);
        Box2Frame_ShowPartyButton(syswk->app);
        if (syswk->param->mode == 3) {
            Box2Obj_SetIconsBlend(syswk, 0x81, 1);
            Box2Ui_ChangeCursor(syswk, 7, 33);
            return 67;
        }
        Box2Obj_SetIconsBlend(syswk, 1, 0);
        Box2Ui_ChangeCursor(syswk, 3, 33);
        return 17;
    }
    return 28;
}

// Closes the party's menu
static int Box2Seq_ArrangePartyMenuClose(Box2SysWork *syswk) {
    Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
    Box2Obj_ShowPokeOutline(syswk, 0);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    Box2Frame_ShowBoxListButton(syswk->app);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMoveFrame, Box2Seq_PokeMoveEndTo(syswk, 27));
}

// After a move in the party
static int Box2Seq_ArrangePartyMoveEnd(Box2SysWork *syswk) {
    GFL_SndSEPlay(SEQ_SE_SYS_40);
    Box2Main_PokeDataMove(syswk);
    FreePokeMoveWork(syswk);
    CursorMove_SetPos(syswk->app->cursorMove, 9);
    syswk->app->oldCurPos = 9;
    Box2Ui_PlaceCursorActor(syswk, 9);
    Box2Obj_ShowCursor(syswk, 1);
    Box2Obj_SetButtons(syswk, 0, 0, 1, 0);
    Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
    return 27;
}

// Moving Pokémon with the party out: brings the party's frame in
static int Box2Seq_MovePartyIn(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        if (Box2Frame_IsPartyAtRight(syswk->app->bgWinFrame) == TRUE) {
            syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
            syswk->app->subSeq = 2;
            break;
        }
        if (syswk->moveMode == 1 && syswk->unk1C_4 == 0) {
            syswk->getTray = BOX2_GET_NONE;
            syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
            if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
                if (syswk->pos >= 6) {
                    syswk->pos = BOX2_PARTY_POS;
                } else {
                    syswk->pos += BOX2_PARTY_POS;
                }
            } else if (syswk->pos >= BOX2_PARTY_POS) {
                syswk->pos = 0;
            }
            Box2Main_PokeInfoPut(syswk, syswk->pos);
        } else {
            if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
                syswk->getTray = BOX2_GET_NONE;
            } else {
                syswk->getTray = syswk->tray;
            }
            Box2Bmp_ClearMsgWindow(syswk->app, 24);
            Box2Main_PutMessageFrame(syswk);
            Box2Frame_ShowBoxListButton(syswk->app);
            Box2Obj_ShowPokeOutline(syswk, 0);
        }
        Box2Obj_ShowCursor(syswk, 0);
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 31);
    case 1:
        if (syswk->param->mode == 4 && syswk->param->battleBoxLocked == 1) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
            Box2Bmp_PrintBattleBoxLocked(syswk, 0);
            syswk->moveMode = 0;
            syswk->app->subSeq = 0;
            syswk->nextSeq = 53;
            return BOX2SEQ_TRGWAIT;
        }
        syswk->app->subSeq++;
        if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_SlidePartyRight(syswk->app->bgWinFrame);
        } else {
            Box2Frame_PutPartyRightBelow(syswk->app->bgWinFrame);
            Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
            Box2Obj_PutPartyIconsLeftBelow(syswk, 0);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 31);
    case 2:
        Box2Obj_SetIconsBlend(syswk, 1, 0);
        Box2Obj_ShowScrollArrows(syswk->app, 1);
        Box2Ui_ChangeCursor(syswk, 6, syswk->pos);
        Box2Obj_SetCursorAnim(syswk, 6);
        syswk->app->oldCurPos = syswk->pos;
        syswk->app->subSeq = 0;
        if (syswk->moveMode == 1) {
            if (syswk->unk1C_4 == 0) {
                if (Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                    Box2Obj_SetButtons(syswk, 0, 1, 2, 0);
                } else {
                    Box2Obj_SetButtons(syswk, 0, 1, 0, 0);
                }
                if (syswk->param->mode == 4) {
                    Box2Frame_HidePartyButton(syswk->app);
                    Box2Frame_HideBoxListButton(syswk->app);
                    CursorMove_DisablePos(syswk->app->cursorMove, 39);
                }
                Box2Obj_SetToolButtons(syswk, 10, 1);
                return 32;
            }
            Box2Obj_SetToolButtons(syswk, 10, 1);
        } else {
            Box2Obj_SetToolButtons(syswk, 9, 1);
        }
        Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        return Box2Seq_MovePickUpKey(syswk, syswk->pos, 32);
    }
    return 31;
}

// Moving Pokémon with the party out: waits for a touch or for the cursor to pick one
static int Box2Seq_MovePartyMain(Box2SysWork *syswk) {
    u32 res;

    if (syswk->unk1C_6 == 1) {
        syswk->moveMode = 0;
        syswk->unk1C_6 = 0;
        return Box2Seq_Goto(syswk, 33);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) && syswk->moveMode != 0 && syswk->unk18 == 0) {
        Box2Main_ShowCursor(syswk);
        syswk->moveMode = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, 33);
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) {
        if (syswk->unk18 == 0) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            Box2Seq_SetGetPos(syswk, pos, syswk->tray);
            syswk->unk13 = 4;
        } else {
            syswk->unk13 = 5;
            syswk->unk1D = CursorMove_GetPos(syswk->app->cursorMove);
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_ShowCursor(syswk);
        return Box2Seq_AnimActorButton(syswk, 30, 1, 91);
    }
    if (syswk->unk18 == 0) {
        res = Box2Ui_GetTrayPokeTrigger();
        if (res != 0xffffffff) {
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
                GFL_SndSEPlay(SEQ_SE_SYS_39);
                GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
                Box2Main_PokeInfoPut(syswk, res);
                return Box2Seq_MovePickUpTouched(syswk, res, 32);
            }
            CursorMove_SetPos(syswk->app->cursorMove, res);
            Box2Ui_SetButtonStates(syswk, res);
            syswk->app->oldCurPos = res;
            Box2Ui_PlaceCursorActor(syswk, res);
            Box2Main_PokeInfoOff(syswk);
            return 32;
        }
        res = Box2Ui_GetPartyPokeTrigger();
        if (res != 0xffffffff) {
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
                GFL_SndSEPlay(SEQ_SE_SYS_39);
                GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
                Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
                return Box2Seq_MovePickUpTouched(syswk, res + BOX2_PARTY_POS, 32);
            }
            CursorMove_SetPos(syswk->app->cursorMove, res + BOX2_PARTY_POS);
            Box2Ui_SetButtonStates(syswk, res + BOX2_PARTY_POS);
            syswk->app->oldCurPos = res + BOX2_PARTY_POS;
            Box2Ui_PlaceCursorActor(syswk, res + BOX2_PARTY_POS);
            Box2Main_PokeInfoOff(syswk);
            return 32;
        }
    }

    res = Box2Ui_UpdateCursor6(syswk);
    switch (res) {
    case 36:
        if (syswk->unk18 != 0) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return Box2Seq_Goto(syswk, 105);
    case 37:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
        Box2Ui_PlaceCursorActor(syswk, BOX2_BOXLIST_POS);
        Box2Ui_SetButtonStates(syswk, BOX2_BOXLIST_POS);
        syswk->app->oldCurPos = BOX2_BOXLIST_POS;
        if (syswk->unk18 == 0) {
            Box2Main_PokeSelectOff(syswk);
        } else {
            Box2Obj_MoveHeldIcons(syswk);
        }
        return Box2Seq_ScrollTrayLeft(syswk, 0, 32);
    case 38:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
        Box2Ui_PlaceCursorActor(syswk, BOX2_BOXLIST_POS);
        Box2Ui_SetButtonStates(syswk, BOX2_BOXLIST_POS);
        syswk->app->oldCurPos = BOX2_BOXLIST_POS;
        if (syswk->unk18 == 0) {
            Box2Main_PokeSelectOff(syswk);
        } else {
            Box2Obj_MoveHeldIcons(syswk);
        }
        return Box2Seq_ScrollTrayRight(syswk, 0, 32);
    case 39:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->unk18 == 0) {
            Box2Main_PokeInfoOff(syswk);
        }
        return Box2Seq_AnimFrameButton(syswk, 11, Box2Seq_Goto(syswk, 34));
    case 40:
        if (syswk->unk18 == 0) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            if (pos >= BOX2_BOXLIST_POS ||
                Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                break;
            }
            syswk->pos = pos;
            syswk->getTray = syswk->tray;
            syswk->unk13 = 4;
        } else {
            syswk->unk13 = 5;
            syswk->unk1D = CursorMove_GetPos(syswk->app->cursorMove);
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, 8, 1, 89);
    case 41:
        if (syswk->unk18 != 0) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = 0;
        return Box2Seq_AnimActorButton(syswk, 6, 9, Box2Seq_Goto(syswk, 33));
    case 42:
        if (syswk->unk18 != 0) {
            break;
        }
        syswk->moveMode = 0;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, 33);
    case 43:
        break;
    case 44:
        if (syswk->unk18 != 0) {
            break;
        }
        syswk->moveMode = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_Goto(syswk, 33);
    case 45:
        if (syswk->unk18 == 0) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            Box2Seq_SetGetPos(syswk, pos, syswk->tray);
            syswk->unk13 = 4;
        } else {
            syswk->unk13 = 5;
            syswk->unk1D = CursorMove_GetPos(syswk->app->cursorMove);
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, 30, 1, 91);
    case CURSORMOVE_CANCEL:
        if (syswk->unk18 == 1) {
            return Box2Seq_MoveDropKey(syswk, BOX2_GET_NONE, 32);
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = 0;
        return Box2Seq_AnimActorButton(syswk, 6, 9, Box2Seq_Goto(syswk, 33));
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BOXLIST_POS) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return Box2Seq_ScrollTrayLeft(syswk, 0, 32);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BOXLIST_POS) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return Box2Seq_ScrollTrayRight(syswk, 0, 32);
        }
        break;
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_CURSOR_MOVE:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        if (syswk->unk18 == 0) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            if (pos < BOX2_BOXLIST_POS) {
                Box2Main_PokeInfoPut(syswk, pos);
            } else {
                Box2Main_PokeInfoOff(syswk);
            }
        }
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, 32));
    case CURSORMOVE_NONE:
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
        break;
    default:
        if (syswk->unk18 == 1) {
            return Box2Seq_MoveDropKey(syswk, res, 32);
        }
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            Box2Seq_SetGetPos(syswk, res, syswk->tray);
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
            return Box2Seq_MovePickUpKey(syswk, syswk->pos, 32);
        }
        break;
    }
    return 32;
}

// Moving Pokémon with the party out: puts the party's frame away or out
static int Box2Seq_MovePartySwitch(Box2SysWork *syswk) {
    u8 pos;

    switch (syswk->app->subSeq) {
    case 0:
        if (syswk->moveMode != 2) {
            Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
            Box2Obj_SetButtons(syswk, 0, 0, 1, 0);
            if (syswk->unk1B == 0) {
                Box2Frame_SlidePartyDown(syswk->app->bgWinFrame);
                syswk->app->subSeq = 2;
            } else {
                Box2Obj_SetIconsBlend(syswk, 1, 1);
                Box2Obj_ShowScrollArrows(syswk->app, 0);
                Box2Frame_SlidePartyLeft(syswk->app->bgWinFrame);
                syswk->app->subSeq = 1;
            }
            GFL_SndSEPlay(SEQ_SE_SYS_42);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 33);
        }
        return 41;
    case 1:
        syswk->app->subSeq = 0;
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos < BOX2_PARTY_POS || pos >= BOX2_BOXLIST_POS) {
            pos = 0;
        } else {
            pos -= BOX2_PARTY_POS;
        }
        Box2Main_PokeInfoPut(syswk, pos + BOX2_PARTY_POS);
        Box2Obj_SetCursorAnim(syswk, 6);
        Box2Obj_SetToolButtons(syswk, 9, 1);
        if (syswk->param->mode == 2) {
            Box2Ui_ChangeCursor(syswk, 5, pos);
            return 27;
        }
        Box2Frame_ShowBoxListButton(syswk->app);
        Box2Ui_ChangeCursor(syswk, 14, pos);
        return 49;
    case 2:
        syswk->app->subSeq = 0;
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos >= BOX2_PARTY_POS) {
            pos = 0;
        }
        Box2Main_PokeInfoPut(syswk, pos);
        Box2Frame_ShowPartyButton(syswk->app);
        Box2Obj_SetCursorAnim(syswk, 6);
        Box2Obj_SetToolButtons(syswk, 9, 1);
        if (syswk->param->mode == 4) {
            Box2Ui_ChangeCursor(syswk, 13, pos);
            return 45;
        }
        Box2Ui_ChangeCursor(syswk, 3, pos);
        return 17;
    }
    return 33;
}

// Moving Pokémon: closes the party's frame
static int Box2Seq_MovePartyClose(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Frame_SlidePartyDown(syswk->app->bgWinFrame);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 34);
    case 1:
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == FALSE) {
            syswk->trayScroll = syswk->tray;
        }
        syswk->app->subSeq++;
        return Box2Seq_MovePartyFrame(syswk, 34);
    case 2:
        Box2Ui_ChangeCursor(syswk, 4, 39);
        Box2Frame_ShowPartyButton(syswk->app);
        syswk->app->oldCurPos = 39;
        syswk->app->subSeq = 0;
        return 21;
    }
    return 34;
}

// The box's main state while the party is out of the way: waits for a touch or for the cursor to pick something
static int Box2Seq_ArrangeBoxMain(Box2SysWork *syswk) {
    u32 res;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        syswk->unk1B = 0;
        return Box2Seq_NextMoveMode(syswk, 31);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) && Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == FALSE) {
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
        syswk->unk13 = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_ShowCursor(syswk);
        return Box2Seq_AnimActorButton(syswk, 30, 1, 91);
    }

    res = Box2Ui_GetTrayPokeTrigger();
    if (res != 0xffffffff) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
            if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == FALSE) {
                Box2Bmp_OpenMenu(syswk, sMenu70f8, 4);
            }
            Box2Main_PokeInfoPut(syswk, res);
            syswk->unk1B = 0;
            return Box2Seq_MovePartyPickUpTouched(syswk, res);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        Box2Ui_PlaceCursorActor(syswk, res);
        Box2Main_PokeInfoOff(syswk);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            Box2Obj_ShowPokeOutline(syswk, 0);
            Box2Bmp_ClearMsgWindow(syswk->app, 24);
            Box2Main_PutMessageFrame(syswk);
            Box2Frame_ShowPartyButton(syswk->app);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 45);
        }
        return 45;
    }

    res = Box2Ui_UpdateCursor13(syswk);
    switch (res) {
    case 30:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return Box2Seq_Goto(syswk, 105);
    case 31:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        syswk->app->oldCurPos = 30;
        Box2Ui_PlaceCursorActor(syswk, 30);
        return Box2Seq_ScrollTrayLeft(syswk, 1, 45);
    case 32:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        syswk->app->oldCurPos = 30;
        Box2Ui_PlaceCursorActor(syswk, 30);
        return Box2Seq_ScrollTrayRight(syswk, 1, 45);
    case 33:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, 10, Box2Seq_Goto(syswk, 48));
    case 34:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->closed = TRUE;
        return Box2Seq_AnimActorButton(syswk, 7, 8, 16);
    case 35:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->param->closed = FALSE;
        return Box2Seq_AnimActorButton(syswk, 6, 9, 116);
    case 36:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk1B = 0;
        syswk->moveMode = 0;
        return Box2Seq_AnimFrameButton(syswk, 2, Box2Seq_Goto(syswk, 31));
    case 37:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk13 = 0;
        syswk->getTray = syswk->tray;
        syswk->curRcvPos = 37;
        return Box2Seq_AnimFrameButton(syswk, 3, 89);
    case 38:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk13 = 0;
        syswk->curRcvPos = 38;
        return Box2Seq_AnimFrameButton(syswk, 4, Box2Seq_Goto(syswk, 90));
    case 39: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos;
        Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        syswk->app->oldCurPos = pos;
        return Box2Seq_AnimFrameButton(syswk, 5, 47);
    }
    case 40:
        break;
    case 41:
        syswk->unk1B = 0;
        syswk->moveMode = 1;
        return Box2Seq_SetMoveMode(syswk, 31);
    case 42:
        syswk->unk1B = 0;
        syswk->moveMode = 2;
        return Box2Seq_SetMoveMode(syswk, 41);
    case 43:
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
        syswk->unk13 = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, 30, 1, 91);
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return Box2Seq_ScrollTrayLeft(syswk, 1, 45);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return Box2Seq_ScrollTrayRight(syswk, 1, 45);
        }
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        if (pos < BOX2_PARTY_POS) {
            Box2Main_PokeInfoPut(syswk, pos);
        } else if (pos != 36 && pos != 37 && pos != 38 && pos != 39) {
            Box2Main_PokeInfoOff(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, 45));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos;

            Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            syswk->app->oldCurPos = pos;
            return Box2Seq_AnimFrameButton(syswk, 5, 47);
        }
        syswk->param->closed = FALSE;
        return Box2Seq_AnimActorButton(syswk, 6, 9, 116);
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_NONE:
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Bmp_OpenMenu(syswk, sMenu70f8, 4);
            Box2Main_PokeInfoPut(syswk, res);
            Box2Ui_StartCursorMove(syswk->app, BOX2_BOXLIST_POS, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
            syswk->app->oldCurPos = BOX2_BOXLIST_POS;
            return Box2Seq_OpenPokeMenu(syswk, res, 45);
        }
        break;
    }
    return 45;
}

// After a move while the party is out of the way
static int Box2Seq_ArrangeBoxMoveEnd(Box2SysWork *syswk) {
    GFL_SndSEPlay(SEQ_SE_SYS_40);
    Box2Main_PokeDataMove(syswk);
    FreePokeMoveWork(syswk);
    CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
    syswk->app->oldCurPos = BOX2_BOXLIST_POS;
    Box2Ui_PlaceCursorActor(syswk, BOX2_BOXLIST_POS);
    Box2Obj_SetButtons(syswk, 0, 0, 1, 0);
    Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
    return 45;
}

// Closes the menu while the party is out of the way
static int Box2Seq_ArrangeBoxMenuClose(Box2SysWork *syswk) {
    Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
    Box2Obj_ShowPokeOutline(syswk, 0);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    Box2Frame_ShowPartyButton(syswk->app);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMoveFrame, Box2Seq_PokeMoveEndTo(syswk, 45));
}

// Brings the party's frame out from the box's main state
static int Box2Seq_ArrangeBoxPartyOut(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        Box2Main_PokeSelectOff(syswk);
        Box2Main_PutMessageFrame(syswk);
        Box2Obj_ShowScrollArrows(syswk->app, 0);
        Box2Obj_SetIconsBlend(syswk, 1, 1);
        Box2Obj_ShowCursor(syswk, 0);
        Box2Frame_PutPartyLeftBelow(syswk->app->bgWinFrame);
        Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
        Box2Obj_PutPartyIconsLeftBelow(syswk, 0);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 48);
    case 1:
        syswk->app->subSeq = 0;
        Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
        Box2Frame_ShowBoxListButton(syswk->app);
        Box2Ui_ChangeCursor(syswk, 14, 0);
        return 49;
    }
    return 48;
}

// The party's main state from the box's main state: waits for a touch or for the cursor to pick something
static int Box2Seq_ArrangeBoxPartyMain(Box2SysWork *syswk) {
    u32 res;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        syswk->unk1B = 1;
        return Box2Seq_NextMoveMode(syswk, 31);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) && Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == FALSE) {
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove) + BOX2_PARTY_POS;
        syswk->unk13 = 4;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_ShowCursor(syswk);
        return Box2Seq_AnimActorButton(syswk, 30, 1, 91);
    }

    res = Box2Ui_GetMenuButtonTrigger();
    if (res != 0xffffffff) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
            if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == FALSE) {
                Box2Bmp_OpenMenu(syswk, sMenu70f8, 4);
            }
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            return Box2Seq_ArrangeBoxPartyPickUpTouched(syswk, res + BOX2_PARTY_POS);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        Box2Ui_PlaceCursorActor(syswk, res);
        Box2Main_PokeInfoOff(syswk);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            Box2Obj_ShowPokeOutline(syswk, 0);
            Box2Bmp_ClearMsgWindow(syswk->app, 24);
            Box2Main_PutMessageFrame(syswk);
            Box2Frame_ShowBoxListButton(syswk->app);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 49);
        }
        return 49;
    }

    res = Box2Ui_UpdateCursor14(syswk);
    switch (res) {
    case 6:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, 10, Box2Seq_Goto(syswk, 50));
    case 7:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->closed = TRUE;
        return Box2Seq_AnimActorButton(syswk, 7, 8, 16);
    case 8:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return Box2Seq_AnimActorButton(syswk, 6, 9, Box2Seq_Goto(syswk, 50));
    case 9:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk1B = 1;
        syswk->moveMode = 0;
        return Box2Seq_AnimFrameButton(syswk, 2, Box2Seq_Goto(syswk, 31));
    case 10:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk13 = 1;
        syswk->getTray = syswk->tray;
        return Box2Seq_AnimFrameButton(syswk, 3, 89);
    case 11:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk13 = 1;
        return Box2Seq_AnimFrameButton(syswk, 4, Box2Seq_Goto(syswk, 90));
    case 12: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos - BOX2_PARTY_POS;
        Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        syswk->app->oldCurPos = pos;
        return Box2Seq_AnimFrameButton(syswk, 5, 51);
    }
    case 13:
        break;
    case 14:
        syswk->unk1B = 1;
        syswk->moveMode = 1;
        return Box2Seq_SetMoveMode(syswk, 31);
    case 15:
        syswk->unk1B = 1;
        syswk->moveMode = 2;
        return Box2Seq_SetMoveMode(syswk, 41);
    case 16:
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove) + BOX2_PARTY_POS;
        syswk->unk13 = 4;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimActorButton(syswk, 30, 1, 91);
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos - BOX2_PARTY_POS;

            Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            syswk->app->oldCurPos = pos;
            return Box2Seq_AnimFrameButton(syswk, 5, 51);
        }
        return Box2Seq_AnimActorButton(syswk, 6, 9, Box2Seq_Goto(syswk, 50));
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        if (pos < 6) {
            Box2Main_PokeInfoPut(syswk, pos + BOX2_PARTY_POS);
        } else if (pos == 6) {
            Box2Main_PokeInfoOff(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, 49));
    }
    case CURSORMOVE_NONE:
    case CURSORMOVE_SCROLL_R:
    case CURSORMOVE_SCROLL_L:
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Bmp_OpenMenu(syswk, sMenu70f8, 4);
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            Box2Ui_StartCursorMove(syswk->app, 9, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, 9);
            syswk->app->oldCurPos = 9;
            return Box2Seq_OpenPartyPokeMenu(syswk, res + BOX2_PARTY_POS, 49);
        }
        break;
    }
    return 49;
}

// Puts the party's frame away, back to the box's main state
static int Box2Seq_ArrangeBoxPartyAway(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        Box2Main_PokeSelectOff(syswk);
        Box2Main_PutMessageFrame(syswk);
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Frame_SlidePartyDown(syswk->app->bgWinFrame);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 50);
    case 1:
        syswk->app->subSeq = 0;
        Box2Obj_ShowScrollArrows(syswk->app, 1);
        Box2Obj_SetIconsBlend(syswk, 1, 0);
        Box2Frame_ShowPartyButton(syswk->app);
        Box2Ui_ChangeCursor(syswk, 13, 33);
        return 45;
    }
    return 50;
}

// Closes the party's menu, from the box's main state
static int Box2Seq_ArrangeBoxPartyMenuClose(Box2SysWork *syswk) {
    Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
    Box2Obj_ShowPokeOutline(syswk, 0);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    Box2Frame_ShowBoxListButton(syswk->app);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMoveFrame, Box2Seq_PokeMoveEndTo(syswk, 49));
}

// After a move in the party, from the box's main state
static int Box2Seq_ArrangeBoxPartyMoveEnd(Box2SysWork *syswk) {
    Box2Main_PokeDataMove(syswk);
    FreePokeMoveWork(syswk);
    CursorMove_SetPos(syswk->app->cursorMove, 9);
    syswk->app->oldCurPos = 9;
    Box2Ui_PlaceCursorActor(syswk, 9);
    Box2Obj_SetButtons(syswk, 0, 0, 1, 0);
    Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
    return 49;
}

// Back from a sub process to the box's main state or the party
static int Box2Seq_SubProcReturn(Box2SysWork *syswk) {
    Box2Bmp_ClearMsgWindow(syswk->app, 27);
    Box2Obj_ShowPokeOutline(syswk, 0);
    Box2Main_PutMessageFrame(syswk);
    if (GCTX_HIDIsUsingTouch() == FALSE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
    }
    if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos - BOX2_PARTY_POS);
        Box2Ui_PlaceCursorActor(syswk, syswk->pos - BOX2_PARTY_POS);
        Box2Frame_ShowBoxListButton(syswk->app);
        return 49;
    }
    CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
    Box2Ui_PlaceCursorActor(syswk, syswk->pos);
    Box2Frame_ShowPartyButton(syswk->app);
    return 45;
}

// The box's main state in mode 1: waits for a touch or for the cursor to pick something
static int Box2Seq_WithdrawMain(Box2SysWork *syswk) {
    u32 res;

    res = Box2Ui_GetTrayPokeTrigger();
    if (res != 0xffffffff) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
            if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == FALSE) {
                Box2Bmp_OpenMenu(syswk, sMenu7194, 5);
            }
            Box2Main_PokeInfoPut(syswk, res);
            return Box2Seq_WithdrawPickUpTouched(syswk, res);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        Box2Ui_PlaceCursorActor(syswk, res);
        Box2Main_PokeInfoOff(syswk);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            Box2Obj_ShowPokeOutline(syswk, 0);
            Box2Bmp_ClearMsgWindow(syswk->app, 24);
            Box2Main_PutMessageFrame(syswk);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 54);
        }
        return 54;
    }

    res = Box2Ui_UpdateCursor2(syswk);
    switch (res) {
    case 30:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return Box2Seq_Goto(syswk, 105);
    case 31:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        Box2Ui_PlaceCursorActor(syswk, 30);
        return Box2Seq_ScrollTrayLeft(syswk, 0, 54);
    case 32:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        Box2Ui_PlaceCursorActor(syswk, 30);
        return Box2Seq_ScrollTrayRight(syswk, 0, 54);
    case 33:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->closed = TRUE;
        return Box2Seq_AnimActorButton(syswk, 7, 8, 16);
    case 34:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->param->closed = FALSE;
        return Box2Seq_AnimActorButton(syswk, 6, 9, 116);
    case 35:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, 1, Box2Seq_Goto(syswk, 56));
    case 36:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->getTray = syswk->tray;
        syswk->curRcvPos = 36;
        return Box2Seq_AnimFrameButton(syswk, 2, 89);
    case 37:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, 3, Box2Seq_Goto(syswk, 97));
    case 38:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, 4, Box2Seq_Goto(syswk, 101));
    case 39: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos;
        Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        return Box2Seq_AnimFrameButton(syswk, 5, 55);
    }
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return Box2Seq_ScrollTrayLeft(syswk, 0, 54);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return Box2Seq_ScrollTrayRight(syswk, 0, 54);
        }
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        if (pos < BOX2_PARTY_POS) {
            Box2Main_PokeInfoPut(syswk, pos);
        } else if (pos != 35 && pos != 36 && pos != 37 && pos != 38 && pos != 39) {
            Box2Main_PokeInfoOff(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, 54));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos;

            Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            return Box2Seq_AnimFrameButton(syswk, 5, 55);
        }
        syswk->param->closed = FALSE;
        return Box2Seq_AnimActorButton(syswk, 6, 9, 116);
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_NONE:
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Bmp_OpenMenu(syswk, sMenu7194, 5);
            Box2Main_PokeInfoPut(syswk, res);
            Box2Ui_StartCursorMove(syswk->app, 35, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, 35);
            return Box2Seq_OpenPokeMenu(syswk, res, 54);
        }
        break;
    }
    return 54;
}

// Closes the menu in mode 1
static int Box2Seq_WithdrawMenuClose(Box2SysWork *syswk) {
    Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
    Box2Obj_ShowPokeOutline(syswk, 0);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMoveFrame, Box2Seq_PokeMoveEndTo(syswk, 54));
}

// Mode 1: closes the menu before taking a Pokémon into the party, unless it is full
static int Box2Seq_WithdrawCheckParty(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        Box2Obj_ShowCursor(syswk, 0);
        Box2Bmp_ClearMsgWindow(syswk->app, 24);
        Box2Main_PutMessageFrame(syswk);
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 56);
    case 1:
        if (PokeParty_GetPkmCount(syswk->param->party) != 6) {
            return Box2Seq_Goto(syswk, 57);
        }
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Bmp_PrintPokeMoveMsg(syswk, 0, 2, 24);
        syswk->nextSeq = 14;
        return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
    }
    return 56;
}

// Mode 1: takes a Pokémon into the party
static int Box2Seq_WithdrawTake(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        Box2Obj_PutPartyIconsRightBelow(syswk);
        Box2Frame_PutPartyRightBelow(syswk->app->bgWinFrame);
        Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
        syswk->unk1A = 2;
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 57);
    case 1:
        syswk->getTray = syswk->tray;
        Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        AllocPokeMoveWork(syswk);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyInPokeMove, 57);
    case 2:
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        Box2Main_PokeDataMove(syswk);
        FreePokeMoveWork(syswk);
        Box2Main_PokeInfoOff(syswk);
        Box2Frame_SlidePartyDown(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 57);
    case 3:
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
        Box2Ui_PlaceCursorActor(syswk, syswk->pos);
        Box2Obj_ShowCursor(syswk, 1);
        syswk->pos = BOX2_GET_NONE;
        syswk->app->subSeq = 0;
        return 54;
    }
    return 57;
}

// Mode 1: after a Pokémon was taken into the party
static int Box2Seq_WithdrawEnd(Box2SysWork *syswk) {
    Box2PokeMoveWork *work = syswk->app->vfunk.work;
    u32 pos = work->getPos;

    Box2Main_PokeDataMove(syswk);
    FreePokeMoveWork(syswk);
    Box2Obj_ShowScrollArrows(syswk->app, 1);
    Box2Obj_SetButtons(syswk, 0, 0, 1, 1);
    if (syswk->pos != BOX2_GET_NONE) {
        Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
        pos = 35;
    }
    CursorMove_SetPos(syswk->app->cursorMove, pos);
    Box2Ui_PlaceCursorActor(syswk, pos);
    return 54;
}

// The party's main state in mode 0: waits for a touch or for the cursor to pick something
static int Box2Seq_DepositPartyMain(Box2SysWork *syswk) {
    u32 res;

    res = Box2Ui_GetMenuButtonTrigger();
    if (res != 0xffffffff) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
            if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == FALSE) {
                Box2Bmp_OpenMenu(syswk, sMenu7130, 5);
            }
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            return Box2Seq_DepositPickUpTouched(syswk, res + BOX2_PARTY_POS);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        Box2Ui_PlaceCursorActor(syswk, res + 60);
        Box2Main_PokeInfoOff(syswk);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            Box2Obj_ShowPokeOutline(syswk, 0);
            Box2Bmp_ClearMsgWindow(syswk->app, 24);
            Box2Main_PutMessageFrame(syswk);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 59);
        }
        return 59;
    }

    res = Box2Ui_UpdateCursor0(syswk);
    switch (res) {
    case 6:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->closed = TRUE;
        return Box2Seq_AnimActorButton(syswk, 7, 8, 16);
    case 7:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->param->closed = FALSE;
        return Box2Seq_AnimActorButton(syswk, 6, 9, 116);
    case 8:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, 1, Box2Seq_Goto(syswk, 62));
    case 9:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->curRcvPos = 9;
        return Box2Seq_AnimFrameButton(syswk, 2, 89);
    case 10:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, 3, Box2Seq_Goto(syswk, 97));
    case 11:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, 4, Box2Seq_Goto(syswk, 101));
    case 12: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos - BOX2_PARTY_POS;
        Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        return Box2Seq_AnimFrameButton(syswk, 5, 60);
    }
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        if (pos < 6) {
            Box2Main_PokeInfoPut(syswk, pos + BOX2_PARTY_POS);
        } else if (pos != 8 && pos != 9 && pos != 10 && pos != 11 && pos != 12) {
            Box2Main_PokeInfoOff(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, 59));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos - BOX2_PARTY_POS;

            Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            return Box2Seq_AnimFrameButton(syswk, 5, 60);
        }
        syswk->param->closed = FALSE;
        return Box2Seq_AnimActorButton(syswk, 6, 9, 116);
    case CURSORMOVE_NONE:
    case CURSORMOVE_SCROLL_R:
    case CURSORMOVE_SCROLL_L:
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Bmp_OpenMenu(syswk, sMenu7130, 5);
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            Box2Ui_StartCursorMove(syswk->app, 8, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, 8);
            return Box2Seq_OpenPartyPokeMenu(syswk, res + BOX2_PARTY_POS, 59);
        }
        break;
    }
    return 59;
}

// Closes the party's menu in mode 0
static int Box2Seq_DepositPartyMenuClose(Box2SysWork *syswk) {
    Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
    Box2Obj_ShowPokeOutline(syswk, 0);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMoveFrame, Box2Seq_PokeMoveEndTo(syswk, 59));
}

// Back to the party's main state in mode 0
static int Box2Seq_DepositPartyReturn(Box2SysWork *syswk) {
    CursorMove_SetPos(syswk->app->cursorMove, syswk->pos - BOX2_PARTY_POS);
    Box2Ui_PlaceCursorActor(syswk, syswk->pos - BOX2_PARTY_POS);
    Box2Obj_ShowCursor(syswk, 1);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    return 59;
}

// Mode 0: picks a Pokémon of the party up to deposit it, unless it can't leave the party
static int Box2Seq_DepositPickParty(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        Box2Obj_ShowCursor(syswk, 0);
        Box2Bmp_ClearMsgWindow(syswk->app, 24);
        Box2Main_PutMessageFrame(syswk);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 62);
    case 1:
        switch (GetDepositBlock(syswk)) {
        case 0:
            syswk->unk18 = 1;
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
            Box2Obj_SetIconPriority(syswk->app, BOX2_BOXLIST_POS, FALSE);
            Box2Obj_ShowCursor(syswk, 1);
            Box2Ui_PlaceCursorActor(syswk, syswk->pos - BOX2_PARTY_POS);
            syswk->app->subSeq++;
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMoveGetKey, 62);
        case 1:
            GFL_SndSEPlay(SEQ_SE_BEEP);
            Box2Obj_ShowPokeOutline(syswk, 0);
            Box2Bmp_PrintReleaseMsg(syswk, 6, 24);
            syswk->nextSeq = 61;
            syswk->app->subSeq = 0;
            return BOX2SEQ_TRGWAIT;
        case 2:
            GFL_SndSEPlay(SEQ_SE_BEEP);
            Box2Obj_ShowPokeOutline(syswk, 0);
            Box2Bmp_PrintPokeMoveMsg(syswk, 0, 5, 24);
            syswk->nextSeq = 61;
            syswk->app->subSeq = 0;
            return BOX2SEQ_TRGWAIT;
        }
        break;
    case 2:
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        Box2Frame_SlidePartyDown(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 62);
    case 3:
        Box2Ui_StartMoveToPartyPos(syswk, syswk->pos - BOX2_PARTY_POS);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMove, 62);
    case 4:
        GFL_HeapFree(syswk->app->vfunk.work);
        Box2Obj_ShowScrollArrows(syswk->app, 1);
        Box2Bmp_PrintPokeMoveMsg(syswk, 0, 3, 26);
        Box2Obj_SetButtons(syswk, 0, 1, 1, 1);
        Box2Ui_ChangeCursor(syswk, 1, 0);
        syswk->app->subSeq = 0;
        return 63;
    }
    return 62;
}

// Mode 0: picks the box to deposit the Pokémon in
static int Box2Seq_DepositPickBox(Box2SysWork *syswk) {
    u32 res = CursorMove_Update(syswk->app->cursorMove);

    if (res == CURSORMOVE_NONE) {
        if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_L) {
            Box2Main_ShowCursor(syswk);
            res = 2;
        } else if (GCTX_HIDGetTypedKeys() & PAD_BUTTON_R) {
            Box2Main_ShowCursor(syswk);
            res = 3;
        }
    }

    switch (res) {
    case 0:
    case 1:
        CursorMove_SetPos(syswk->app->cursorMove, 0);
        if (countEmptySlotsInBox(syswk->param->boxes, syswk->tray) == 0) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
            Box2Bmp_PrintPokeMoveMsg(syswk, 0, 4, 24);
            syswk->nextSeq = 65;
            return BOX2SEQ_TRGWAIT;
        } else {
            int pos;
            int tray;

            tray = syswk->tray;
            pos = 0;

            BoxSaveAccessor_GetNextFreeBoxSlot(syswk->param->boxes, &tray, &pos);
            syswk->app->pokePutKey = pos;
            syswk->unk1A = 0;
            syswk->getTray = syswk->tray;
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return Box2Seq_Goto(syswk, 64);
        }
    case 2:
        CursorMove_SetPos(syswk->app->cursorMove, 0);
    case CURSORMOVE_SCROLL_L:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_ScrollTrayLeft(syswk, 0, 63);
    case 3:
        CursorMove_SetPos(syswk->app->cursorMove, 0);
    case CURSORMOVE_SCROLL_R:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_ScrollTrayRight(syswk, 0, 63);
    case 4:
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->app->pokePutKey = BOX2_GET_NONE;
        syswk->getTray = syswk->tray;
        syswk->unk1A = 1;
        return Box2Seq_AnimActorButton(syswk, 6, 9, Box2Seq_Goto(syswk, 64));
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_NONE:
    case CURSORMOVE_CURSOR_MOVE:
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
        break;
    }
    return 63;
}

// Mode 0: deposits the Pokémon in the picked box, or puts it back in the party
static int Box2Seq_DepositPut(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        if (syswk->app->pokePutKey == BOX2_GET_NONE) {
            Box2Ui_StartMoveToCursor1(syswk, syswk->pos);
            syswk->app->subSeq = 1;
        } else {
            Box2Ui_StartMoveToCursor1(syswk, syswk->app->pokePutKey);
            syswk->app->subSeq = 4;
        }
        Box2Obj_ShowScrollArrows(syswk->app, 0);
        Box2Bmp_ClearMsgWindow(syswk->app, 24);
        Box2Main_PutMessageFrame(syswk);
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMove, 64);
    case 1:
        GFL_HeapFree(syswk->app->vfunk.work);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
        syswk->app->subSeq = 2;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 64);
    case 2:
        AllocPokeMoveWork(syswk);
        syswk->app->subSeq = 6;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMovePutKey, 64);
    case 3:
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
        syswk->app->subSeq = 7;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 64);
    case 4:
        GFL_HeapFree(syswk->app->vfunk.work);
        AllocPokeMoveWork(syswk);
        Box2Obj_ShowPokeOutline(syswk, 0);
        syswk->app->subSeq = 5;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyOutPutKey, 64);
    case 5:
        Box2Main_PokeDataMove(syswk);
        FreePokeMoveWork(syswk);
        Box2Main_PokeInfoOff(syswk);
        Box2Obj_SetIconsBlend(syswk, 1, 1);
        Box2Obj_SetIconsBlend(syswk, 2, 0);
        syswk->app->subSeq = 3;
        break;
    case 6:
        FreePokeMoveWork(syswk);
    case 7:
        syswk->unk18 = 0;
        Box2Obj_ShowPokeOutline(syswk, 0);
        Box2Obj_SetButtons(syswk, 0, 0, 1, 1);
        Box2Ui_ChangeCursor(syswk, 0, syswk->pos - BOX2_PARTY_POS);
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        syswk->app->subSeq = 0;
        return 59;
    }
    return 64;
}

// Mode 0: closes the message about a full box
static int Box2Seq_DepositFullClose(Box2SysWork *syswk) {
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    Box2Bmp_PrintPokeMoveMsg(syswk, 0, 3, 26);
    Box2Obj_SetButtons(syswk, 0, 1, 1, 1);
    return 63;
}

// Mode 0: after a deposit
static int Box2Seq_DepositEnd(Box2SysWork *syswk) {
    Box2Main_PokeDataMove(syswk);
    FreePokeMoveWork(syswk);
    Box2Obj_SetButtons(syswk, 0, 0, 1, 1);
    if (syswk->pos == BOX2_GET_NONE) {
        CursorMove_SetPos(syswk->app->cursorMove, 0);
        Box2Ui_PlaceCursorActor(syswk, 0);
        Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
    } else {
        Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
        CursorMove_SetPos(syswk->app->cursorMove, 8);
        Box2Ui_PlaceCursorActor(syswk, 8);
    }
    return 59;
}

// The item arrangement's main state: waits for a touch or for the cursor to pick something
static int Box2Seq_ItemMain(Box2SysWork *syswk) {
    u32 res;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        res = Box2Seq_NextMoveMode(syswk, 73);
        if (syswk->moveMode == 1 && syswk->unk1C_4 == 0) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            if (pos < BOX2_PARTY_POS) {
                syswk->pos = pos;
            } else {
                syswk->pos = 0;
            }
        }
        return res;
    }

    res = Box2Ui_GetTrayPokeTrigger();
    if (res != 0xffffffff) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
            Box2Main_PokeInfoPut(syswk, res);
            return Box2Seq_ItemTakeTouched(syswk, res);
        }
        Box2Main_PokeInfoOff(syswk);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            Box2Ui_StartCursorMove(syswk->app, res, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, res);
            return Box2Seq_ItemMenuClose(syswk);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        Box2Ui_PlaceCursorActor(syswk, res);
        return 67;
    }

    res = Box2Ui_UpdateCursor7(syswk);
    switch (res) {
    case 30:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return Box2Seq_Goto(syswk, 105);
    case 31:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        Box2Ui_PlaceCursorActor(syswk, 30);
        return Box2Seq_ScrollTrayLeft(syswk, 1, 67);
    case 32:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        Box2Ui_PlaceCursorActor(syswk, 30);
        return Box2Seq_ScrollTrayRight(syswk, 1, 67);
    case 33:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, 10, Box2Seq_Goto(syswk, 26));
    case 34:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->closed = TRUE;
        return Box2Seq_AnimActorButton(syswk, 7, 8, 16);
    case 35:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->param->closed = FALSE;
        return Box2Seq_AnimActorButton(syswk, 6, 9, 116);
    case 36:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->moveMode = 0;
        return Box2Seq_AnimFrameButton(syswk, 3, Box2Seq_Goto(syswk, 73));
    case 37:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->getItem == 0) {
            syswk->unk13 = 0;
            syswk->curRcvPos = 37;
            return Box2Seq_AnimFrameButton(syswk, 4, Box2Seq_Goto(syswk, 90));
        }
        return Box2Seq_AnimFrameButton(syswk, 4, Box2Seq_Goto(syswk, 90));
    case 38: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos;
        Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        return Box2Seq_AnimFrameButton(syswk, 5, 68);
    }
    case 39:
        if (syswk->moveMode == 1) {
            syswk->moveMode = 0;
            return Box2Seq_SetMoveMode(syswk, 73);
        }
        break;
    case 40:
        if (syswk->moveMode == 0) {
            syswk->moveMode = 1;
            res = Box2Seq_SetMoveMode(syswk, 73);
            if (syswk->unk1C_4 == 0) {
                u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

                if (pos < BOX2_PARTY_POS) {
                    syswk->pos = pos;
                } else {
                    syswk->pos = 0;
                }
            }
            return res;
        }
        break;
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return Box2Seq_ScrollTrayLeft(syswk, 1, 67);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return Box2Seq_ScrollTrayRight(syswk, 1, 67);
        }
        break;
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        if (pos < BOX2_PARTY_POS) {
            Box2Main_PokeInfoPut(syswk, pos);
        } else if (pos != 36 && pos != 37 && pos != 38) {
            Box2Main_PokeInfoOff(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, 67));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos;

            Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            return Box2Seq_AnimFrameButton(syswk, 5, 68);
        }
        syswk->param->closed = FALSE;
        return Box2Seq_AnimActorButton(syswk, 6, 9, 116);
    case CURSORMOVE_NONE:
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return Box2Seq_ItemOpenMenu(syswk, res);
        }
        break;
    }
    return 67;
}

// The item arrangement: closes the menu
static int Box2Seq_ItemMenuClose(Box2SysWork *syswk) {
    Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
    Box2Obj_ShowPokeOutline(syswk, 0);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    Box2Frame_ShowPartyButton(syswk->app);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeMenuOpen, Box2Seq_PokeMoveEndTo(syswk, 67));
}

// The item arrangement: puts the held item in the bag
static int Box2Seq_ItemToBag(Box2SysWork *syswk) {
    if (Box2Obj_IsAnimActive(syswk->app, BOX2_ACTOR_ITEM_ICON) == TRUE) {
        return 69;
    }
    Box2Seq_OpenItemMenu(syswk, 0);
    Box2Obj_SetItemIconAffine(syswk->app, FALSE);
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
    Box2Bmp_PrintItemMsg(syswk, 3, 24);
    if (syswk->pos < BOX2_PARTY_POS) {
        Box2Obj_SetIconsBlend(syswk, 0x81, 1);
        Box2Obj_ShowScrollArrows(syswk->app, 1);
    } else {
        Box2Obj_SetIconsBlend(syswk, 0x82, 1);
    }
    syswk->app->getItem = 0;
    syswk->nextSeq = 71;
    return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
}

// The item arrangement: hides the item icon
static int Box2Seq_ItemHideIcon(Box2SysWork *syswk) {
    Box2Obj_ShowPokeOutline(syswk, 0);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    if (syswk->pos < BOX2_PARTY_POS) {
        Box2Frame_ShowPartyButton(syswk->app);
    } else {
        Box2Frame_ShowBoxListButton(syswk->app);
    }
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemIconHide, 71);
}

// The item arrangement: back to the box's or the party's main state
static int Box2Seq_ItemBack(Box2SysWork *syswk) {
    int seq;

    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    if (syswk->pos < BOX2_PARTY_POS) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
        Box2Ui_PlaceCursorActor(syswk, syswk->pos);
        Box2Frame_ShowPartyButton(syswk->app);
        seq = 67;
    } else {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos - BOX2_PARTY_POS);
        Box2Ui_PlaceCursorActor(syswk, syswk->pos - BOX2_PARTY_POS);
        Box2Frame_ShowBoxListButton(syswk->app);
        seq = 80;
    }
    Box2Obj_ShowCursor(syswk, 1);
    Box2Obj_SetToolButtons(syswk, 9, 1);
    return seq;
}

// The item arrangement: back to the main state with the cursor on the menu's button
static int Box2Seq_ItemBackToMenu(Box2SysWork *syswk) {
    u32 pos = 37;

    if (syswk->app->getItem != 0) {
        pos = 36;
    }
    CursorMove_SetPos(syswk->app->cursorMove, pos);
    syswk->app->oldCurPos = pos;
    Box2Ui_PlaceCursorActor(syswk, pos);
    Box2Obj_ShowCursor(syswk, 1);
    return 67;
}

// The item arrangement: brings the party's frame in
static int Box2Seq_ItemPartyIn(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        syswk->app->subSeq++;
        Box2Obj_ShowCursor(syswk, 0);
        if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
            syswk->unk1E = 1;
        } else {
            syswk->unk1E = 0;
        }
        if (syswk->moveMode == 1) {
            if (syswk->unk1C_4 == 0) {
                syswk->getTray = BOX2_GET_NONE;
                syswk->app->getItem = 0;
                Box2Main_PutMessageFrame(syswk);
                Box2Main_PokeInfoPut(syswk, syswk->pos);
            } else {
                if (syswk->app->getItem == 0) {
                    syswk->getTray = BOX2_GET_NONE;
                    syswk->unk1C_4 = 0;
                }
                Box2Bmp_ClearMsgWindow(syswk->app, 24);
                Box2Main_PutMessageFrame(syswk);
                Box2Obj_ShowPokeOutline(syswk, 0);
                Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
                return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeFrameMove, 73);
            }
        } else {
            Box2Bmp_ClearMsgWindow(syswk->app, 24);
            Box2Main_PutMessageFrame(syswk);
            Box2Obj_ShowPokeOutline(syswk, 0);
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeFrameMove, 73);
        }
    case 1:
        syswk->app->subSeq++;
        if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_SlidePartyRight(syswk->app->bgWinFrame);
        } else {
            Box2Frame_PutPartyRightBelow(syswk->app->bgWinFrame);
            Box2Frame_SlidePartyUp(syswk->app->bgWinFrame);
            Box2Obj_PutPartyIconsLeftBelow(syswk, 0);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 73);
    case 2:
        syswk->app->subSeq = 0;
        Box2Obj_SetIconsBlend(syswk, 0x81, 1);
        Box2Obj_SetIconsBlend(syswk, 0x82, 1);
        Box2Obj_ShowScrollArrows(syswk->app, 1);
        Box2Ui_ChangeCursor(syswk, 6, syswk->pos);
        Box2Obj_SetCursorAnim(syswk, 6);
        CursorMove_DisablePos(syswk->app->cursorMove, 39);
        CursorMove_DisablePos(syswk->app->cursorMove, 40);
        if (syswk->moveMode == 1) {
            if (syswk->unk1C_4 == 0) {
                Box2Obj_SetButtons(syswk, 0, 1, 1, 1);
                Box2Obj_SetToolButtons(syswk, 10, 1);
                return 74;
            }
            Box2Obj_SetToolButtons(syswk, 10, 1);
        } else {
            Box2Obj_SetToolButtons(syswk, 9, 1);
        }
        Box2Obj_SetButtons(syswk, 2, 1, 1, 1);
        return Box2Seq_ItemPartyOutTakeKey(syswk, syswk->pos);
    }
    return 73;
}

// The item arrangement with the party out: waits for a touch or for the cursor to pick something
static int Box2Seq_ItemPartyOutMain(Box2SysWork *syswk) {
    u32 res;

    if (syswk->unk1C_6 == 1) {
        syswk->moveMode = 0;
        syswk->unk1C_6 = 0;
        return Box2Seq_Goto(syswk, 75);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) && syswk->moveMode != 0 && syswk->unk18 == 0) {
        Box2Main_ShowCursor(syswk);
        syswk->moveMode = 0;
        return Box2Seq_Goto(syswk, 75);
    }
    if (syswk->unk18 == 0) {
        res = Box2Ui_GetPartyPokeTrigger();
        if (res != 0xffffffff) {
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
                Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
                Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
                return Box2Seq_ItemPartyOutTakeTouched(syswk, res + BOX2_PARTY_POS);
            }
            Box2Main_PokeInfoOff(syswk);
            CursorMove_SetPos(syswk->app->cursorMove, res + BOX2_PARTY_POS);
            return 74;
        }
        res = Box2Ui_GetTrayPokeTrigger();
        if (res != 0xffffffff) {
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
                Box2Main_PokeInfoPut(syswk, res);
                Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
                return Box2Seq_ItemPartyOutTakeTouched(syswk, res);
            }
            Box2Main_PokeInfoOff(syswk);
            CursorMove_SetPos(syswk->app->cursorMove, res);
            return 74;
        }
    }

    res = Box2Ui_UpdateCursor6(syswk);
    switch (res) {
    case 36:
        if (syswk->unk18 != 0) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return Box2Seq_Goto(syswk, 105);
    case 37:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
        Box2Ui_PlaceCursorActor(syswk, BOX2_BOXLIST_POS);
        if (syswk->unk18 == 0) {
            Box2Main_PokeSelectOff(syswk);
        } else {
            Box2Obj_MoveHeldIcons(syswk);
        }
        return Box2Seq_ScrollTrayLeft(syswk, 0, 74);
    case 38:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
        Box2Ui_PlaceCursorActor(syswk, BOX2_BOXLIST_POS);
        if (syswk->unk18 == 0) {
            Box2Main_PokeSelectOff(syswk);
        } else {
            Box2Obj_MoveHeldIcons(syswk);
        }
        return Box2Seq_ScrollTrayRight(syswk, 0, 74);
    case 39:
    case 40:
        break;
    case 41:
        if (syswk->unk18 != 0) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = 0;
        return Box2Seq_AnimActorButton(syswk, 6, 9, Box2Seq_Goto(syswk, 75));
    case 42:
        if (syswk->moveMode == 0 || syswk->unk18 != 0) {
            break;
        }
        syswk->moveMode = 0;
        return Box2Seq_Goto(syswk, 75);
    case 43:
    case 44:
    case 45:
        break;
    case CURSORMOVE_CANCEL:
        if (syswk->unk18 == 1) {
            return Box2Seq_ItemDropHeldKey(syswk, BOX2_GET_NONE);
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = 0;
        return Box2Seq_AnimActorButton(syswk, 6, 9, Box2Seq_Goto(syswk, 75));
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BOXLIST_POS) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return Box2Seq_ScrollTrayLeft(syswk, 0, 74);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BOXLIST_POS) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return Box2Seq_ScrollTrayRight(syswk, 0, 74);
        }
        break;
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_CURSOR_MOVE:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        if (syswk->unk18 == 0) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            if (pos < BOX2_BOXLIST_POS) {
                Box2Main_PokeInfoPut(syswk, pos);
            } else {
                Box2Main_PokeInfoOff(syswk);
            }
        }
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, 74));
    case CURSORMOVE_NONE:
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
        break;
    default:
        if (syswk->unk18 == 1) {
            return Box2Seq_ItemDropHeldKey(syswk, res);
        }
        return Box2Seq_ItemPartyOutTakeKey(syswk, res);
    }
    return 74;
}

// The item arrangement: puts the party's frame away
static int Box2Seq_ItemPartyAway(Box2SysWork *syswk) {
    u8 pos;

    switch (syswk->app->subSeq) {
    case 0:
        Box2Main_PokeInfoOff(syswk);
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        if (syswk->unk1E == 0) {
            Box2Frame_SlidePartyDown(syswk->app->bgWinFrame);
        } else {
            Box2Obj_ShowScrollArrows(syswk->app, 0);
            Box2Obj_SetIconsBlend(syswk, 1, 1);
            Box2Frame_SlidePartyLeft(syswk->app->bgWinFrame);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 75);
    case 1:
        syswk->app->subSeq = 0;
        Box2Obj_SetCursorAnim(syswk, 6);
        Box2Obj_SetToolButtons(syswk, 9, 1);
        Box2Obj_SetButtons(syswk, 0, 0, 1, 1);
        if (syswk->unk1E == 0) {
            pos = CursorMove_GetPos(syswk->app->cursorMove);
            if (pos >= BOX2_BOXLIST_POS) {
                pos = 0;
            } else if (pos >= BOX2_PARTY_POS) {
                pos = 0;
            }
            Box2Ui_ChangeCursor(syswk, 7, pos);
            Box2Main_PokeInfoPut(syswk, pos);
            Box2Frame_ShowPartyButton(syswk->app);
            return 67;
        } else {
            u8 cur;

            pos = 0;
            cur = CursorMove_GetPos(syswk->app->cursorMove);
            if (cur < BOX2_BOXLIST_POS && cur >= BOX2_PARTY_POS) {
                pos = cur - BOX2_PARTY_POS;
            }
            Box2Ui_ChangeCursor(syswk, 8, pos);
            Box2Main_PokeInfoPut(syswk, pos + BOX2_PARTY_POS);
            Box2Frame_ShowBoxListButton(syswk->app);
            return 80;
        }
    }
    return 75;
}

// The item arrangement: an item was dropped on a Pokémon by the cursor
static int Box2Seq_ItemDropKey(Box2SysWork *syswk) {
    Box2ItemMoveWork *work;
    u16 item;

    if (syswk->moveMode == 0) {
        syswk->unk1C_6 = 1;
    }
    Box2Obj_ShowPokeOutline(syswk, 0);
    work = syswk->app->vfunk.work;
    if (work->putPos == syswk->pos && (syswk->getTray == BOX2_GET_NONE || syswk->getTray == syswk->tray)) {
        u16 setPos = work->setPos;

        FreeItemMoveWork(syswk);
        Box2Obj_SetIconBlend(syswk->app, syswk->pos, FALSE);
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
        Box2Obj_SetButtons(syswk, 0, 1, 1, 1);
        if (PML_ItemIsMail(syswk->app->getItem) == TRUE && setPos != syswk->pos) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
            Box2Bmp_PrintMailCantMove(syswk, 24);
            syswk->nextSeq = 78;
            return BOX2SEQ_TRGWAIT;
        }
        return 74;
    }

    if (syswk->getTray == syswk->tray || syswk->pos >= BOX2_PARTY_POS) {
        Box2Obj_SetIconBlend(syswk->app, syswk->pos, TRUE);
    }
    Box2Obj_SetIconBlend(syswk->app, work->putPos, FALSE);
    item = Box2Main_GetPokeParam(syswk, work->putPos, syswk->tray, PKM_PARAM_ITEM, NULL);
    Box2Main_SetPokeParam(syswk, work->putPos, syswk->tray, PKM_PARAM_ITEM, syswk->app->getItem);
    if (Box2Main_PokeItemFormChange(syswk, Box2Main_GetBoxPkm(syswk, syswk->tray, work->putPos)) == TRUE) {
        Box2Main_RecalcPartyStats(syswk, work->putPos);
        Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, work->putPos, syswk->app->pokeIconId[work->putPos]);
    }
    Box2Main_PokeInfoPut(syswk, work->putPos);
    CursorMove_SetPos(syswk->app->cursorMove, work->putPos);

    syswk->app->getItem = item;
    Box2Main_SetPokeParam(syswk, syswk->pos, syswk->getTray, PKM_PARAM_ITEM, syswk->app->getItem);
    if (Box2Main_PokeItemFormChange(syswk, Box2Main_GetBoxPkm(syswk, syswk->getTray, syswk->pos)) == TRUE) {
        Box2Main_RecalcPartyStats(syswk, syswk->pos);
        if (syswk->getTray == BOX2_GET_NONE || syswk->getTray == syswk->tray) {
            Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
    }

    if (syswk->app->getItem == 0) {
        syswk->pos = work->putPos;
        FreeItemMoveWork(syswk);
        Box2Obj_SetButtons(syswk, 0, 1, 1, 1);
        return 74;
    }
    Box2Obj_SetItemIcon(syswk->app, syswk->app->getItem);
    Box2Obj_SetItemIconAffine(syswk->app, TRUE);
    Box2Obj_SetAnim(syswk->app, BOX2_ACTOR_ITEM_ICON, 1);
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, TRUE);
    Box2Obj_InitItemOutline(syswk->app);
    GFL_SndSEPlay(SEQ_SE_SYS_39);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemIconPutBack, 77);
}

// The item arrangement: an item was dropped on a Pokémon by touch
static int Box2Seq_ItemDropTouch(Box2SysWork *syswk) {
    Box2ItemMoveWork *work;
    u8 oldPos;
    u16 setPos;
    u16 item;
    u8 pos;

    if (syswk->moveMode == 0) {
        syswk->unk1C_6 = 1;
    }
    work = syswk->app->vfunk.work;
    Box2Obj_SetIconBlend(syswk->app, work->putPos, FALSE);
    oldPos = syswk->pos;
    setPos = work->setPos;
    syswk->pos = work->putPos;
    FreeItemMoveWork(syswk);
    item = syswk->app->getItem;
    syswk->app->getItem = 0;
    if (PML_ItemIsMail(item) == TRUE && setPos != oldPos) {
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Obj_ShowCursor(syswk, 0);
        Box2Bmp_PrintMailCantMove(syswk, 24);
        syswk->nextSeq = 78;
        return BOX2SEQ_TRGWAIT;
    }
    Box2Obj_ShowCursor(syswk, 1);
    pos = CursorMove_GetPos(syswk->app->cursorMove);
    if (pos < BOX2_BOXLIST_POS) {
        Box2Main_PokeInfoPut(syswk, pos);
    }
    Box2Obj_SetButtons(syswk, 0, 1, 1, 1);
    return 74;
}

// The item arrangement: closes the message about mail
static int Box2Seq_ItemMailClose(Box2SysWork *syswk) {
    Box2Bmp_ClearMsgWindowNow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    if (CursorMove_IsCursorVisible(syswk->app->cursorMove) == TRUE) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, TRUE);
    }
    return 74;
}

// The item arrangement: an item was dropped on a Pokémon by the cursor, with the party out
static int Box2Seq_ItemDropKeyPartyOut(Box2SysWork *syswk) {
    u16 item;

    if (syswk->moveMode == 0) {
        syswk->unk1C_6 = 1;
    }
    if (syswk->app->pokePutKey == syswk->app->getItemInitPos &&
        (syswk->getTray == BOX2_GET_NONE || syswk->getTray == syswk->tray)) {
        syswk->app->getItem = 0;
        Box2Obj_SetButtons(syswk, 0, 1, 1, 1);
        return 74;
    }

    if (syswk->getTray == syswk->tray || syswk->app->getItemInitPos >= BOX2_PARTY_POS) {
        Box2Obj_SetIconBlend(syswk->app, syswk->app->getItemInitPos, TRUE);
    }
    Box2Obj_SetIconBlend(syswk->app, syswk->app->pokePutKey, FALSE);
    item = Box2Main_GetPokeParam(syswk, syswk->app->pokePutKey, syswk->tray, PKM_PARAM_ITEM, NULL);
    Box2Main_SetPokeParam(syswk, syswk->app->pokePutKey, syswk->tray, PKM_PARAM_ITEM, syswk->app->getItem);
    if (Box2Main_PokeItemFormChange(syswk, Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->app->pokePutKey)) == TRUE) {
        Box2Main_RecalcPartyStats(syswk, syswk->app->pokePutKey);
        Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, syswk->app->pokePutKey,
                                syswk->app->pokeIconId[syswk->app->pokePutKey]);
    }
    Box2Main_PokeInfoPut(syswk, syswk->app->pokePutKey);

    syswk->app->getItem = item;
    Box2Main_SetPokeParam(syswk, syswk->app->getItemInitPos, syswk->getTray, PKM_PARAM_ITEM, syswk->app->getItem);
    if (Box2Main_PokeItemFormChange(syswk, Box2Main_GetBoxPkm(syswk, syswk->getTray, syswk->app->getItemInitPos)) ==
        TRUE) {
        Box2Main_RecalcPartyStats(syswk, syswk->app->getItemInitPos);
        if (syswk->getTray == BOX2_GET_NONE || syswk->getTray == syswk->tray) {
            Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, syswk->app->getItemInitPos,
                                    syswk->app->pokeIconId[syswk->app->getItemInitPos]);
        }
    }

    if (syswk->app->getItem == 0) {
        Box2Obj_SetButtons(syswk, 0, 1, 1, 1);
        return 74;
    }
    AllocItemMoveWork(syswk);
    ((Box2ItemMoveWork *)syswk->app->vfunk.work)->putPos = syswk->app->pokePutKey;
    Box2Obj_SetItemIcon(syswk->app, syswk->app->getItem);
    Box2Obj_SetItemIconAffine(syswk->app, TRUE);
    Box2Obj_SetAnim(syswk->app, BOX2_ACTOR_ITEM_ICON, 1);
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, TRUE);
    Box2Obj_InitItemOutline(syswk->app);
    GFL_SndSEPlay(SEQ_SE_SYS_39);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemIconPutBack, 77);
}

// The item arrangement in the party: waits for a touch or for the cursor to pick something
static int Box2Seq_ItemPartyMain(Box2SysWork *syswk) {
    u32 res;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        res = Box2Seq_NextMoveMode(syswk, 73);
        if (syswk->moveMode == 1 && syswk->unk1C_4 == 0) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            if (pos < 6) {
                syswk->pos = pos + BOX2_PARTY_POS;
            } else {
                syswk->pos = BOX2_PARTY_POS;
            }
        }
        return res;
    }

    res = Box2Ui_GetMenuButtonTrigger();
    if (res != 0xffffffff) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            GFL_HIDGetTouchPressedPoint(&syswk->app->tpx, &syswk->app->tpy);
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            return Box2Seq_ItemPartyTakeTouched(syswk, res + BOX2_PARTY_POS);
        }
        Box2Main_PokeInfoOff(syswk);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            Box2Ui_StartCursorMove(syswk->app, (u8)res, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, res + BOX2_PARTY_POS);
            return Box2Seq_ItemPartyMenuClose(syswk);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        Box2Ui_PlaceCursorActor(syswk, res);
        return 80;
    }

    res = Box2Ui_UpdateCursor8(syswk);
    switch (res) {
    case 6:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, 10, Box2Seq_Goto(syswk, 28));
    case 7:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->closed = TRUE;
        return Box2Seq_AnimActorButton(syswk, 7, 8, 16);
    case 8:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return Box2Seq_AnimActorButton(syswk, 6, 9, Box2Seq_Goto(syswk, 28));
    case 9:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->moveMode = 0;
        return Box2Seq_AnimFrameButton(syswk, 3, Box2Seq_Goto(syswk, 73));
    case 10:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->getItem == 0) {
            syswk->unk13 = 1;
            syswk->curRcvPos = 10;
            return Box2Seq_AnimFrameButton(syswk, 4, Box2Seq_Goto(syswk, 90));
        }
        return Box2Seq_AnimFrameButton(syswk, 4, Box2Seq_Goto(syswk, 90));
    case 11: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos - BOX2_PARTY_POS;
        Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        return Box2Seq_AnimFrameButton(syswk, 5, 81);
    }
    case 12:
        if (syswk->moveMode == 1) {
            syswk->moveMode = 0;
            return Box2Seq_SetMoveMode(syswk, 73);
        }
        break;
    case 13:
        if (syswk->moveMode == 0) {
            syswk->moveMode = 1;
            res = Box2Seq_SetMoveMode(syswk, 73);
            if (syswk->unk1C_4 == 0) {
                u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

                if (pos < 6) {
                    syswk->pos = pos + BOX2_PARTY_POS;
                } else {
                    syswk->pos = BOX2_PARTY_POS;
                }
            }
            return res;
        }
        break;
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        if (pos < 6) {
            Box2Main_PokeInfoPut(syswk, pos + BOX2_PARTY_POS);
        } else if (pos == 6) {
            Box2Main_PokeInfoOff(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, 80));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos - BOX2_PARTY_POS;

            Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            return Box2Seq_AnimFrameButton(syswk, 5, 81);
        }
        return Box2Seq_AnimActorButton(syswk, 6, 9, Box2Seq_Goto(syswk, 28));
    case CURSORMOVE_NONE:
    case CURSORMOVE_SCROLL_R:
    case CURSORMOVE_SCROLL_L:
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return Box2Seq_ItemPartyOpenMenu(syswk, res + BOX2_PARTY_POS);
        }
        break;
    }
    return 80;
}

// The item arrangement in the party: closes the menu
static int Box2Seq_ItemPartyMenuClose(Box2SysWork *syswk) {
    Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
    Box2Obj_ShowPokeOutline(syswk, 0);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    Box2Frame_ShowBoxListButton(syswk->app);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeMenuOpen, Box2Seq_PokeMoveEndTo(syswk, 80));
}

// The item arrangement in the party: an item was dropped
static int Box2Seq_ItemPartyDrop(Box2SysWork *syswk) {
    Box2ItemMoveWork *work = syswk->app->vfunk.work;

    if (syswk->app->getItem == 0) {
        FreeItemMoveWork(syswk);
        Box2Bmp_PrintItemMsg(syswk, 0, 24);
        CursorMove_SetPos(syswk->app->cursorMove, 10);
        syswk->app->oldCurPos = 10;
        Box2Ui_PlaceCursorActor(syswk, 10);
        Box2Obj_ShowCursor(syswk, 1);
        return 80;
    }
    if (work->putPos == syswk->pos) {
        u16 setPos = work->setPos;

        FreeItemMoveWork(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, 9);
        syswk->app->oldCurPos = 9;
        Box2Ui_PlaceCursorActor(syswk, 9);
        if (PML_ItemIsMail(syswk->app->getItem) == TRUE && setPos != syswk->pos) {
            Box2Obj_SetIconBlend(syswk->app, syswk->pos, FALSE);
            Box2Bmp_PrintItemMsg(syswk, 7, 24);
            GFL_SndSEPlay(SEQ_SE_BEEP);
            syswk->nextSeq = 14;
            return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
        }
        Box2Bmp_PrintItemMsg(syswk, 1, 24);
        Box2Obj_ShowCursor(syswk, 1);
        return 80;
    }
    if (PML_ItemIsMail(syswk->app->getItem) == TRUE) {
        FreeItemMoveWork(syswk);
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Obj_SetIconBlend(syswk->app, syswk->pos, FALSE);
        Box2Bmp_PrintItemMsg(syswk, 7, 24);
        GFL_SndSEPlay(SEQ_SE_BEEP);
        CursorMove_SetPos(syswk->app->cursorMove, 9);
        syswk->app->oldCurPos = 9;
        Box2Ui_PlaceCursorActor(syswk, 9);
        Box2Obj_ShowCursor(syswk, 1);
        syswk->nextSeq = 14;
        return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
    }
    Box2Obj_SetIconBlend(syswk->app, work->putPos, FALSE);
    Box2Obj_SetIconBlend(syswk->app, syswk->pos, TRUE);
    Box2Obj_ShowPokeOutline(syswk, 0);
    Box2Obj_SetItemIconAffine(syswk->app, TRUE);
    Box2Obj_SetAnim(syswk->app, BOX2_ACTOR_ITEM_ICON, 2);
    return 83;
}

// The item arrangement in the party: swaps the items once the icon has landed
static int Box2Seq_ItemPartySwap(Box2SysWork *syswk) {
    Box2ItemMoveWork *work;
    u16 item;

    if (Box2Obj_IsAnimActive(syswk->app, BOX2_ACTOR_ITEM_ICON) == TRUE) {
        return 83;
    }
    Box2Obj_SetItemIconAffine(syswk->app, FALSE);
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
    work = syswk->app->vfunk.work;
    item = Box2Main_GetPokeParam(syswk, work->putPos, syswk->tray, PKM_PARAM_ITEM, NULL);
    Box2Main_SetPokeParam(syswk, work->putPos, syswk->tray, PKM_PARAM_ITEM, syswk->app->getItem);
    if (Box2Main_PokeItemFormChange(syswk, Box2Main_GetBoxPkm(syswk, syswk->tray, work->putPos)) == TRUE) {
        Box2Main_RecalcPartyStats(syswk, work->putPos);
        Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, work->putPos, syswk->app->pokeIconId[work->putPos]);
    }
    Box2Main_PokeInfoPut(syswk, work->putPos);

    syswk->app->getItem = item;
    Box2Main_SetPokeParam(syswk, syswk->pos, syswk->getTray, PKM_PARAM_ITEM, syswk->app->getItem);
    if (syswk->getTray == BOX2_GET_NONE || syswk->getTray == syswk->tray) {
        if (Box2Main_PokeItemFormChange(syswk, Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos)) == TRUE) {
            Box2Main_RecalcPartyStats(syswk, syswk->pos);
            Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
    }

    if (syswk->app->getItem == 0) {
        syswk->pos = work->putPos;
        FreeItemMoveWork(syswk);
        Box2Frame_ShowBoxListButton(syswk->app);
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos - BOX2_PARTY_POS);
        syswk->app->oldCurPos = syswk->pos - BOX2_PARTY_POS;
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
        return 80;
    }
    GFL_SndSEPlay(SEQ_SE_SYS_39);
    Box2Obj_SetItemIcon(syswk->app, syswk->app->getItem);
    Box2Obj_SetItemIconAffine(syswk->app, TRUE);
    Box2Obj_SetAnim(syswk->app, BOX2_ACTOR_ITEM_ICON, 1);
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, TRUE);
    Box2Obj_InitItemOutline(syswk->app);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeMenuCancel, Box2Seq_Goto(syswk, 84));
}

// The item arrangement in the party: puts the item back where it was taken
static int Box2Seq_ItemPartyReturn(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_SYS_40);
        Box2Obj_SetIconBlend(syswk->app, ((Box2ItemMoveWork *)syswk->app->vfunk.work)->putPos, FALSE);
        Box2Obj_ShowPokeOutline(syswk, 0);
        Box2Obj_SetItemIconAffine(syswk->app, TRUE);
        Box2Obj_SetAnim(syswk->app, BOX2_ACTOR_ITEM_ICON, 2);
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos - BOX2_PARTY_POS);
        syswk->app->oldCurPos = syswk->pos - BOX2_PARTY_POS;
        FreeItemMoveWork(syswk);
        syswk->app->subSeq++;
        break;
    case 1:
        if (Box2Obj_IsAnimActive(syswk->app, BOX2_ACTOR_ITEM_ICON) != TRUE) {
            Box2Main_PokeInfoPut(syswk, syswk->pos);
            CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
            Box2Frame_ShowBoxListButton(syswk->app);
            return 80;
        }
        break;
    }
    return 84;
}

// Mode 5's main state: waits for a touch or for the cursor to pick a Pokémon
static int Box2Seq_DreamWorldMain(Box2SysWork *syswk) {
    u32 res = Box2Ui_UpdateCursor15(syswk);

    switch (res) {
    case 30:
        Box2Main_PokeSelectOff(syswk);
        break;
    case 31:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        Box2Ui_PlaceCursorActor(syswk, 30);
        return Box2Seq_ScrollTrayLeft(syswk, 0, 85);
    case 32:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        Box2Ui_PlaceCursorActor(syswk, 30);
        return Box2Seq_ScrollTrayRight(syswk, 0, 85);
    case 33:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->param->closed = FALSE;
        syswk->param->tray = BOX2_GET_NONE;
        syswk->param->position = BOX2_GET_NONE;
        return Box2Seq_AnimActorButton(syswk, 6, 9, 116);
    case 34:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, 4, Box2Seq_Goto(syswk, 87));
    case 35: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos;
        Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        return Box2Seq_AnimFrameButton(syswk, 5, 88);
    }
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return Box2Seq_ScrollTrayLeft(syswk, 0, 85);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return Box2Seq_ScrollTrayRight(syswk, 0, 85);
        }
        break;
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case CURSORMOVE_CURSOR_MOVE: {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        if (pos < BOX2_PARTY_POS) {
            Box2Main_PokeInfoPut(syswk, pos);
        } else if (pos != 34 && pos != 35) {
            Box2Main_PokeInfoOff(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, 85));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos;

            Box2Ui_StartCursorMove(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            return Box2Seq_AnimFrameButton(syswk, 5, 88);
        }
        syswk->param->closed = FALSE;
        syswk->param->tray = BOX2_GET_NONE;
        syswk->param->position = BOX2_GET_NONE;
        return Box2Seq_AnimActorButton(syswk, 6, 9, 116);
    case CURSORMOVE_NONE:
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            syswk->pos = res;
            Box2Bmp_OpenMenu(syswk, sMenu70cc, 2);
            Box2Main_PokeInfoPut(syswk, res);
            Box2Obj_PutPokeOutline(syswk);
            CursorMove_SetPos(syswk->app->cursorMove, 34);
            if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
                return Box2Seq_DreamWorldMenuOpened(syswk);
            }
            Box2Ui_StartCursorMove(syswk->app, 34, res);
            Box2Frame_SlideMenuIn(syswk->app->bgWinFrame);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMoveFrame, Box2Seq_PokeMoveEndTo(syswk, 86));
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeInfoPut(syswk, res);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            Box2Obj_ShowPokeOutline(syswk, 0);
            Box2Bmp_ClearMsgWindow(syswk->app, 24);
            Box2Main_PutMessageFrame(syswk);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 85);
        }
        break;
    }
    return 85;
}

// Mode 5: the picked Pokémon's menu was already open
static int Box2Seq_DreamWorldMenuOpened(Box2SysWork *syswk) {
    Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
    return 85;
}

// Mode 5: picks the Pokémon, unless it is an egg or can't be picked
static int Box2Seq_DreamWorldPick(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        Box2Obj_ShowCursor(syswk, 0);
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 87);
    case 1:
        if (Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_IS_EGG, NULL) != 0) {
            Box2Bmp_PrintDenyMsg(syswk, BOX2_MOVE_ERR_EGG, 24);
            syswk->nextSeq = 14;
            return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
        }
        if (Box2Main_IsSpeciesFlagged(syswk, syswk->pos) == FALSE) {
            Box2Bmp_PrintDenyMsg(syswk, BOX2_MOVE_ERR_NOT_SLEEPY, 27);
            syswk->nextSeq = 14;
            return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
        }
        Box2Obj_SetIconsBlend(syswk, 1, 1);
        Box2Obj_SetIconBlend(syswk->app, syswk->pos, FALSE);
        Box2Bmp_PrintTuckIn(syswk);
        syswk->app->subSeq = 0;
        return Box2Seq_OpenYesNo(syswk, 5);
    }
    return 87;
}

// Mode 5: closes the menu
static int Box2Seq_DreamWorldMenuClose(Box2SysWork *syswk) {
    Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
    Box2Obj_ShowPokeOutline(syswk, 0);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMoveFrame, Box2Seq_PokeMoveEndTo(syswk, 85));
}

static int Box2Seq_CallPokeStatus(Box2SysWork *syswk) {
    return Box2Seq_WipeOutCallSubProc(syswk, 0);
}

// Takes the held item into the bag, unless the Pokémon is an egg or holds mail
static int Box2Seq_TakeItem(Box2SysWork *syswk) {
    u32 item;

    switch (syswk->app->subSeq) {
    case 0:
        if (syswk->param->mode == 4 && syswk->param->battleBoxLocked == 1) {
            Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            syswk->app->subSeq = 3;
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 90);
        }
        if (Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_IS_EGG, NULL) == 0) {
            item = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);
            if (item == 0) {
                return Box2Seq_WipeOutCallSubProc(syswk, 1);
            }
            Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
            Box2Obj_SetToolButtons(syswk, 9, 0);
            Box2Obj_ShowScrollArrows(syswk->app, 0);
            if (PML_ItemIsMail(item) == TRUE) {
                Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
                syswk->app->subSeq = 1;
                return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 90);
            }
            if (syswk->pos < BOX2_PARTY_POS) {
                Box2Obj_SetIconsBlend(syswk, 1, 1);
            } else {
                Box2Obj_SetIconsBlend(syswk, 2, 1);
            }
            Box2Obj_SetIconBlend(syswk->app, syswk->pos, FALSE);
            syswk->app->unkA550 = 1;
            Box2Frame_HideMenu(syswk->app->bgWinFrame);
            Box2Bmp_PrintTakeItem(syswk, item, 24);
            if (syswk->param->mode == 3) {
                return Box2Seq_OpenYesNo(syswk, 2);
            }
            return Box2Seq_OpenYesNo(syswk, 0);
        }
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq = 2;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 90);
    case 1:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Bmp_PrintMailCantTake(syswk, 24);
        syswk->nextSeq = 14;
        return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
    case 2:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Bmp_PrintItemMsg(syswk, 5, 24);
        syswk->nextSeq = 14;
        return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
    case 3:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Bmp_PrintBattleBoxLocked(syswk, 1);
        syswk->nextSeq = 53;
        return BOX2SEQ_TRGWAIT;
    }
    return 90;
}

static int Box2Seq_CallBoxSearch(Box2SysWork *syswk) {
    return Box2Seq_WipeOutCallSubProc(syswk, 3);
}

// Back from the summary: shows the Pokémon and fades in
static int Box2Seq_SummaryReturn(Box2SysWork *syswk) {
    if (syswk->getTray != BOX2_GET_NONE) {
        Box2Main_PokeInfoPutCore(syswk, syswk->getTray, syswk->pos);
    } else {
        Box2Main_PokeInfoPut(syswk, syswk->pos);
    }
    Box2Obj_SetCursorAnim(syswk, 6);
    Box2Obj_SetToolButtons(syswk, syswk->moveMode + 9, 1);
    switch (syswk->param->mode) {
    case 0:
        syswk->app->msgNextSeq = Box2Seq_DepositSummaryReturn(syswk);
        break;
    case 1:
        syswk->app->msgNextSeq = Box2Seq_WithdrawSummaryReturn(syswk);
        break;
    case 2:
        syswk->app->msgNextSeq = Box2Seq_ArrangeSummaryReturn(syswk);
        break;
    case 4:
        syswk->app->msgNextSeq = Box2Seq_BattleBoxSummaryReturn(syswk);
        break;
    }
    return Box2Seq_StartWait(syswk);
}

// Back from the bag: puts the cursor on the box list and fades in
static int Box2Seq_NameInputReturn(Box2SysWork *syswk) {
    Box2Main_PokeInfoOff(syswk);
    switch (syswk->param->mode) {
    case 1:
        Box2Ui_ChangeCursor(syswk, 2, BOX2_PARTY_POS);
        syswk->app->oldCurPos = BOX2_PARTY_POS;
        syswk->app->msgNextSeq = 54;
        break;
    case 2:
        Box2Frame_ShowPartyButton(syswk->app);
        Box2Ui_ChangeCursor(syswk, 3, BOX2_PARTY_POS);
        syswk->app->oldCurPos = BOX2_PARTY_POS;
        syswk->app->msgNextSeq = 17;
        break;
    case 3:
        Box2Obj_SetIconsBlend(syswk, 0x81, 1);
        Box2Frame_ShowPartyButton(syswk->app);
        Box2Ui_ChangeCursor(syswk, 7, BOX2_PARTY_POS);
        syswk->app->oldCurPos = BOX2_PARTY_POS;
        syswk->app->msgNextSeq = 67;
        break;
    case 4:
        Box2Frame_ShowPartyButton(syswk->app);
        Box2Ui_ChangeCursor(syswk, 13, BOX2_PARTY_POS);
        syswk->app->oldCurPos = BOX2_PARTY_POS;
        syswk->app->msgNextSeq = 45;
        break;
    }
    return Box2Seq_StartWait(syswk);
}

// Back from the name entry: fades in
static int Box2Seq_BagReturn(Box2SysWork *syswk) {
    switch (syswk->param->mode) {
    case 2:
        syswk->app->msgNextSeq = Box2Seq_ArrangeGiveItemReturn(syswk);
        break;
    case 3:
        syswk->app->msgNextSeq = Box2Seq_ItemGiveItemReturn(syswk);
        break;
    case 4:
        syswk->app->msgNextSeq = Box2Seq_BattleBoxGiveItemReturn(syswk);
        break;
    }
    return Box2Seq_StartWait(syswk);
}

// Back from the bag after giving an item: shows the Pokémon and fades in
static int Box2Seq_BoxSearchReturn(Box2SysWork *syswk) {
    u32 max;

    if (syswk->unk13 == 2 || syswk->unk13 == 3) {
        max = BOX2_PARTY_POS;
    } else {
        max = BOX2_BOXLIST_POS;
    }
    if (syswk->pos < max) {
        if (syswk->getTray != BOX2_GET_NONE) {
            Box2Main_PokeInfoPutCore(syswk, syswk->getTray, syswk->pos);
        } else {
            Box2Main_PokeInfoPut(syswk, syswk->pos);
        }
    } else {
        Box2Main_PokeInfoOff(syswk);
    }
    Box2Obj_SetCursorAnim(syswk, 6);
    Box2Obj_SetToolButtons(syswk, syswk->moveMode + 9, 1);
    switch (syswk->param->mode) {
    case 2:
        syswk->app->msgNextSeq = Box2Seq_ArrangeMoveSummaryReturn(syswk);
        break;
    case 4:
        syswk->app->msgNextSeq = Box2Seq_BattleBoxMoveSummaryReturn(syswk);
        break;
    }
    return Box2Seq_StartWait(syswk);
}

// Says which item was given
static int Box2Seq_GivenItemMsg(Box2SysWork *syswk) {
    Box2Bmp_PrintGiveItem(syswk, syswk->subRet, 24);
    if (syswk->param->mode == 3) {
        syswk->nextSeq = 70;
    } else {
        syswk->nextSeq = 14;
    }
    return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
}

// Opens the markings
static int Box2Seq_MarkingOpen(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Obj_SetToolButtons(syswk, 9, 0);
        Box2Bmp_ClearMsgWindow(syswk->app, 24);
        Box2Main_PutMessageFrame(syswk);
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 97);
    case 1:
        syswk->app->unkA554 = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_MARKINGS, NULL);
        Box2Main_MarkingPutMain(syswk, syswk->app->unkA554);
        Box2Frame_SlideMarkingIn(syswk->app->bgWinFrame);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncMarkingFrameMove, 97);
    case 2:
        Box2Ui_ChangeCursor(syswk, 9, 0);
        Box2Obj_ShowScrollArrows(syswk->app, 0);
        Box2Obj_SetIconsBlend(syswk, 1, 1);
        Box2Obj_SetIconsBlend(syswk, 2, 1);
        Box2Obj_SetIconBlend(syswk->app, syswk->pos, FALSE);
        Box2Bmp_PrintTouchMark(syswk, 24);
        return Box2Seq_Goto(syswk, 98);
    }
    return 97;
}

// The markings: toggles a mark, or sets or cancels them
static int Box2Seq_MarkingMain(Box2SysWork *syswk) {
    switch (CursorMove_Update(syswk->app->cursorMove)) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        ToggleMark(syswk, 0);
        break;
    case 1:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        ToggleMark(syswk, 1);
        break;
    case 2:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        ToggleMark(syswk, 2);
        break;
    case 3:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        ToggleMark(syswk, 3);
        break;
    case 4:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        ToggleMark(syswk, 4);
        break;
    case 5:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        ToggleMark(syswk, 5);
        break;
    case 6: {
        u8 mark = syswk->app->unkA554;

        Box2Main_SetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_MARKINGS, mark);
        Box2Main_MarkingPutSub(syswk, mark);
        Box2Obj_UpdatePokeIconPalette(syswk, syswk->tray, syswk->pos);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimMarkButton(syswk, 0, 99);
    }
    case 7:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return Box2Seq_AnimMarkButton(syswk, 1, 99);
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return Box2Seq_AnimMarkButton(syswk, 1, 99);
    case CURSORMOVE_CURSOR_MOVE:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, 100);
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    }
    return 98;
}

// Closes the markings
static int Box2Seq_MarkingClose(Box2SysWork *syswk) {
    switch (syswk->param->mode) {
    case 0:
        Box2Ui_ChangeCursor(syswk, 0, 10);
        break;
    case 1:
        Box2Obj_ShowScrollArrows(syswk->app, 1);
        Box2Ui_ChangeCursor(syswk, 2, 37);
        break;
    case 2:
    default:
        if (syswk->pos < BOX2_PARTY_POS) {
            Box2Obj_ShowScrollArrows(syswk->app, 1);
            Box2Ui_ChangeCursor(syswk, 3, 39);
        } else {
            Box2Ui_ChangeCursor(syswk, 5, 12);
        }
        break;
    }
    Box2Obj_ShowCursor(syswk, 0);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    if (syswk->pos < BOX2_PARTY_POS) {
        Box2Obj_SetIconsBlend(syswk, 1, 0);
    }
    Box2Obj_SetIconsBlend(syswk, 2, 0);
    Box2Frame_SlideMarkingOut(syswk->app->bgWinFrame);
    GFL_SndSEPlay(SEQ_SE_SYS_42);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncMarkingFrameMove, Box2Seq_Goto(syswk, 14));
}

static int Box2Seq_ReleaseStart(Box2SysWork *syswk) {
    GFL_HeapFree(syswk->app->vfunk.work);
    return 98;
}

// Releasing: checks the Pokémon can be released
static int Box2Seq_ReleaseCheck(Box2SysWork *syswk) {
    BoxPkm *pkm;
    u16 species;
    u8 form;
    u32 msg;

    switch (syswk->app->subSeq) {
    case 0:
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Obj_SetToolButtons(syswk, 9, 0);
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 101);
    case 1:
        if (syswk->pos >= BOX2_PARTY_POS) {
            u32 partyPos;

            Box2Obj_SetIconsBlend(syswk, 2, 1);
            Box2Obj_SetIconBlend(syswk->app, syswk->pos, FALSE);
            partyPos = syswk->pos - BOX2_PARTY_POS;
            if (Box2Main_BattlePokeCheck(syswk, partyPos) == FALSE) {
                GFL_SndSEPlay(SEQ_SE_BEEP);
                Box2Bmp_PrintReleaseMsg(syswk, 6, 24);
                syswk->app->unkA550 = 1;
                syswk->nextSeq = 14;
                return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
            }
            if (PML_ItemIsMail(PokeParty_GetParam(PokeParty_GetPkm(syswk->param->party, partyPos), PKM_PARAM_ITEM,
                                                  NULL)) == TRUE) {
                GFL_SndSEPlay(SEQ_SE_BEEP);
                Box2Bmp_PrintPokeMoveMsg(syswk, 0, 5, 24);
                syswk->app->unkA550 = 1;
                syswk->nextSeq = 14;
                return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
            }
        } else {
            Box2Obj_ShowScrollArrows(syswk->app, 0);
            Box2Obj_SetIconsBlend(syswk, 1, 1);
            Box2Obj_SetIconBlend(syswk->app, syswk->pos, FALSE);
        }
        pkm = Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos);
        species = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
        form = PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);
        msg = 0xffff;
        if (PML_PkmGetParam(pkm, PKM_PARAM_IS_EGG, NULL) != 0) {
            msg = 3;
        } else if (isKyuremTransformed(species, form)) {
            msg = 7;
        }
        if (msg != 0xffff) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
            Box2Bmp_PrintReleaseMsg(syswk, msg, 24);
            syswk->app->unkA550 = 1;
            syswk->nextSeq = 14;
            return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
        }
        Box2Bmp_PrintReleaseMsg(syswk, 0, 24);
        return Box2Seq_Goto(syswk, Box2Seq_OpenYesNo(syswk, 1));
    }
    return 101;
}

// Releasing: checks the moves the Pokémon takes away
static int Box2Seq_ReleaseCheckMoves(Box2SysWork *syswk) {
    Box2Main_PokeFreeMoveCheck(syswk);
    if (Box2PokeFree_Shrink(syswk->app->subWork) == FALSE) {
        if (((Box2PokeFreeWork *)syswk->app->subWork)->checkFlag != 0) {
            return Box2Seq_Goto(syswk, 104);
        }
        Box2PokeFree_End(syswk->app->subWork);
        Box2Main_PokeFreeExit(syswk);
        return Box2Seq_Goto(syswk, 103);
    }
    return 102;
}

// Releasing: says goodbye and removes the Pokémon
static int Box2Seq_ReleaseDo(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        Box2Bmp_PrintReleaseMsg(syswk, 1, 24);
        syswk->nextSeq = 103;
        syswk->app->subSeq++;
        return BOX2SEQ_TRGWAIT;
    case 1:
        Box2Bmp_PrintReleaseMsg(syswk, 2, 24);
        syswk->nextSeq = 103;
        syswk->app->subSeq++;
        return BOX2SEQ_TRGWAIT;
    case 2:
        Box2Main_ClearPokeData(syswk, syswk->tray, syswk->pos);
        Box2Bmp_ClearMsgWindow(syswk->app, 24);
        Box2Main_PutMessageFrame(syswk);
        Box2Main_PokeInfoOff(syswk);
        if (syswk->pos < BOX2_PARTY_POS) {
            Box2Obj_MakeTrayIconChar(syswk, syswk->tray);
            Box2Obj_RefreshTrayIcon(syswk, syswk->tray);
            if (syswk->param->mode == 1) {
                syswk->nextSeq = 54;
            } else {
                syswk->nextSeq = 17;
                Box2Frame_ShowPartyButton(syswk->app);
            }
            CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
            Box2Ui_PlaceCursorActor(syswk, syswk->pos);
            Box2Obj_ShowCursor(syswk, 1);
            Box2Obj_ShowScrollArrows(syswk->app, 1);
            Box2Obj_SetIconsBlend(syswk, 1, 0);
            Box2Obj_SetToolButtons(syswk, 9, 1);
            syswk->pos = BOX2_GET_NONE;
            syswk->app->subSeq = 0;
            return syswk->nextSeq;
        }
        AllocPokeMoveWork(syswk);
        syswk->unk1A = 1;
        Box2Obj_SetIconsBlend(syswk, 2, 0);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyPokeFreeSort, 103);
    case 3:
        syswk->app->subSeq = 0;
        FreePokeMoveWork(syswk);
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos - BOX2_PARTY_POS);
        Box2Ui_PlaceCursorActor(syswk, syswk->pos - BOX2_PARTY_POS);
        Box2Obj_ShowCursor(syswk, 1);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->param->mode == 0) {
            return 59;
        }
        Box2Obj_SetToolButtons(syswk, 9, 1);
        Box2Frame_ShowBoxListButton(syswk->app);
        return 27;
    }
    return 103;
}

// Releasing: says the Pokémon's move can't be lost, and puts it back
static int Box2Seq_ReleaseMoveLocked(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        if (Box2PokeFree_Grow(syswk->app->subWork) == FALSE) {
            Box2PokeFree_ResetActor(syswk->app->subWork);
            Box2Main_PokeFreeExit(syswk);
            Box2Obj_PutPokeOutline(syswk);
            Box2Bmp_PrintReleaseMsg(syswk, 4, 24);
            syswk->nextSeq = 104;
            syswk->app->subSeq++;
            return BOX2SEQ_TRGWAIT;
        }
        break;
    case 1:
        Box2Bmp_PrintReleaseMsg(syswk, 5, 24);
        syswk->nextSeq = 104;
        syswk->app->subSeq++;
        return BOX2SEQ_TRGWAIT;
    case 2:
        syswk->app->subSeq = 0;
        return Box2Seq_NoRelease(syswk);
    }
    return 104;
}

// Opens the box menu
static int Box2Seq_BoxMenuOpen(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        syswk->app->subSeq++;
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        Box2Obj_SetToolButtons(syswk, 9, 0);
        Box2Obj_ShowScrollArrows(syswk->app, 0);
        if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
            if (Box2Obj_IsVisible(syswk->app, BOX2_ACTOR_ITEM_ICON) == TRUE) {
                Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
            }
            Box2Obj_ShowPokeOutline(syswk, 0);
            Box2Bmp_ClearMsgWindow(syswk->app, 24);
            Box2Main_PutMessageFrame(syswk);
            Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 105);
        }
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == TRUE) {
            Box2Obj_ShowBoxNameFrame(syswk, 0, 0);
            Box2Frame_HidePartyButton(syswk->app);
            return Box2Seq_MovePartyFrameTo(syswk, 105);
        }
        if (Box2Frame_IsPartyAtRight(syswk->app->bgWinFrame) == TRUE) {
            Box2Frame_HideBoxListButton(syswk->app);
            Box2Frame_SlidePartyDown(syswk->app->bgWinFrame);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyFrameMove, 105);
        }
        Box2Frame_HidePartyButton(syswk->app);
    case 1:
        Box2Bmp_OpenMenu(syswk, sMenu70e8, 4);
        Box2Frame_SlideMenuIn(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 105);
    case 2:
        syswk->app->subSeq = 0;
        Box2Obj_SetIconsBlend(syswk, 1, 1);
        Box2Bmp_PrintPrompt(syswk, 0, 24);
        Box2Ui_ChangeCursor(syswk, 10, 0);
        syswk->moveMode = 0;
        Box2Obj_SetCursorAnim(syswk, 6);
        return 106;
    }
    return 105;
}

// The box menu: picks a choice
static int Box2Seq_BoxMenuMain(Box2SysWork *syswk) {
    switch (CursorMove_Update(syswk->app->cursorMove)) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, 2, Box2Seq_Goto(syswk, 108));
    case 1:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->app->unkA55C = 0;
        return Box2Seq_AnimFrameButton(syswk, 3, Box2Seq_Goto(syswk, 112));
    case 2:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return Box2Seq_AnimFrameButton(syswk, 4, 115);
    case 3:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return Box2Seq_AnimFrameButton(syswk, 5, Box2Seq_Goto(syswk, 107));
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return Box2Seq_AnimFrameButton(syswk, 5, Box2Seq_Goto(syswk, 107));
    case CURSORMOVE_CURSOR_MOVE:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, 106));
    }
    return 106;
}

// Closes the box menu
static int Box2Seq_BoxMenuClose(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        Box2Obj_ShowCursor(syswk, 0);
        Box2Bmp_ClearMsgWindow(syswk->app, 24);
        Box2Main_PutMessageFrame(syswk);
        if (syswk->param->mode == 2 || syswk->param->mode == 4) {
            Box2Obj_SetButtons(syswk, 0, 0, 1, 0);
        } else {
            Box2Obj_SetButtons(syswk, 0, 0, 1, 1);
        }
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 107);
    case 1:
        syswk->app->subSeq = 0;
        Box2Obj_ShowScrollArrows(syswk->app, 1);
        Box2Obj_SetToolButtons(syswk, 9, 1);
        if (syswk->param->mode == 2) {
            Box2Obj_SetIconsBlend(syswk, 1, 0);
            Box2Frame_ShowPartyButton(syswk->app);
            Box2Ui_ChangeCursor(syswk, 3, BOX2_PARTY_POS);
            return 17;
        }
        if (syswk->param->mode == 1) {
            Box2Obj_SetIconsBlend(syswk, 1, 0);
            Box2Ui_ChangeCursor(syswk, 2, BOX2_PARTY_POS);
            return 54;
        }
        if (syswk->param->mode == 3) {
            Box2Obj_SetIconsBlend(syswk, 0x81, 1);
            Box2Frame_ShowPartyButton(syswk->app);
            Box2Ui_ChangeCursor(syswk, 7, BOX2_PARTY_POS);
            return 67;
        }
        if (syswk->param->mode == 4) {
            Box2Obj_SetIconsBlend(syswk, 1, 0);
            Box2Frame_ShowPartyButton(syswk->app);
            Box2Ui_ChangeCursor(syswk, 13, BOX2_PARTY_POS);
            return 45;
        }
    }
    return 107;
}

// Opens the box jump
static int Box2Seq_BoxJumpOpen(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        syswk->app->unkA55A = 1;
        Box2Obj_ShowCursor(syswk, 0);
        Box2Bmp_ClearMsgWindow(syswk->app, 24);
        Box2Main_PutMessageFrame(syswk);
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 108);
    case 1:
        if (Box2Frame_IsBoxListIn(syswk->app->bgWinFrame) == FALSE) {
            syswk->trayScroll = syswk->tray;
        }
        syswk->app->subSeq++;
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        return Box2Seq_MovePartyFrame(syswk, 108);
    case 2:
        syswk->app->subSeq = 0;
        Box2Obj_ShowBoxNameFrame(syswk, 0, 1);
        Box2Ui_ChangeCursor(syswk, 12, 1);
        syswk->app->oldCurPos = 1;
        Box2Bmp_PrintPrompt(syswk, 1, 25);
        Box2Obj_SetButtons(syswk, 0, 1, 1, 1);
        return 109;
    }
    return 108;
}

// The box jump: picks a box in the list
static int Box2Seq_BoxJumpMain(Box2SysWork *syswk) {
    u32 x, y;
    u32 res;

    if (Box2Ui_GetBoxListScrollTouch(&x, &y) == TRUE) {
        syswk->app->tpy = y;
        syswk->nextSeq = 109;
        AllocBoxListDragWork(syswk);
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        return 15;
    }

    res = CursorMove_Update(syswk->app->cursorMove);
    switch (res) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_StartBoxListScroll(syswk, -1);
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxMoveScrollRight, 109);
    case 5:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_StartBoxListScroll(syswk, 1);
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxMoveScrollLeft, 109);
    case 1:
    case 2:
    case 3:
    case 4:
        syswk->app->unkA55F = syswk->trayScroll + res - 1;
        if (syswk->app->unkA55F >= syswk->trayMax) {
            syswk->app->unkA55F -= syswk->trayMax;
        }
        if (syswk->app->unkA55F != syswk->tray) {
            Box2Obj_ShowBoxNameFrame(syswk, 0, 0);
            return Box2Seq_ScrollToPickedBox(syswk, 111);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Obj_ShowBoxNameFrame(syswk, res - 1, 1);
        break;
    case 6:
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return Box2Seq_AnimActorButton(syswk, 6, 9, Box2Seq_Goto(syswk, 110));
    case CURSORMOVE_CURSOR_MOVE:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, 109));
    case CURSORMOVE_UNK_8:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 1) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_StartBoxListScroll(syswk, -1);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxMoveScrollRight, 109);
        }
        break;
    case CURSORMOVE_UNK_7:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 4) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_StartBoxListScroll(syswk, 1);
            return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxMoveScrollLeft, 109);
        }
        break;
    }
    return 109;
}

// Closes the box jump
static int Box2Seq_BoxJumpClose(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        syswk->app->unkA55A = 0;
        Box2Obj_ShowCursor(syswk, 0);
        Box2Bmp_ClearMsgWindow(syswk->app, 25);
        Box2Main_PutMessageFrame(syswk);
        Box2Obj_ShowBoxNameFrame(syswk, 0, 0);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return Box2Seq_MovePartyFrameTo(syswk, 110);
    case 1:
        syswk->app->subSeq = 0;
        Box2Obj_ShowScrollArrows(syswk->app, 1);
        if (syswk->param->mode == 2 || syswk->param->mode == 4) {
            Box2Obj_SetButtons(syswk, 0, 0, 1, 0);
        } else {
            Box2Obj_SetButtons(syswk, 0, 0, 1, 1);
        }
        Box2Obj_SetToolButtons(syswk, 9, 1);
        if (syswk->param->mode == 2) {
            Box2Obj_SetIconsBlend(syswk, 1, 0);
            Box2Frame_ShowPartyButton(syswk->app);
            Box2Ui_ChangeCursor(syswk, 3, BOX2_PARTY_POS);
            return 17;
        }
        if (syswk->param->mode == 1) {
            Box2Obj_SetIconsBlend(syswk, 1, 0);
            Box2Ui_ChangeCursor(syswk, 2, BOX2_PARTY_POS);
            return 54;
        }
        if (syswk->param->mode == 3) {
            Box2Obj_SetIconsBlend(syswk, 0x81, 1);
            Box2Frame_ShowPartyButton(syswk->app);
            Box2Ui_ChangeCursor(syswk, 7, BOX2_PARTY_POS);
            return 67;
        }
        if (syswk->param->mode == 4) {
            Box2Obj_SetIconsBlend(syswk, 1, 0);
            Box2Frame_ShowPartyButton(syswk->app);
            Box2Ui_ChangeCursor(syswk, 13, BOX2_PARTY_POS);
            return 45;
        }
    }
    return 110;
}

// The box jump: back from showing a box
static int Box2Seq_BoxJumpReturn(Box2SysWork *syswk) {
    Box2Obj_ShowBoxNameFrame(syswk, CursorMove_GetPos(syswk->app->cursorMove) - 1, 1);
    return 109;
}

// Opens a wallpaper menu: the themes, or a theme's wallpapers
static int Box2Seq_WallpaperMenuOpen(Box2SysWork *syswk) {
    u32 all;

    switch (syswk->app->subSeq) {
    case 0:
        Box2Obj_ShowCursor(syswk, 0);
        Box2Bmp_ClearMsgWindow(syswk->app, 24);
        Box2Main_PutMessageFrame(syswk);
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 112);
    case 1:
        if (syswk->app->unkA55C == 0) {
            if (BoxSaveAccessor_CheckFlags(syswk->param->boxes, 1) == TRUE) {
                Box2Bmp_OpenMenu(syswk, sMenu71a8, 6);
            } else {
                Box2Bmp_OpenMenu(syswk, sMenu711c, 5);
            }
        } else if (syswk->app->unkA55C == 1) {
            Box2Bmp_OpenMenu(syswk, sMenu7144, 5);
        } else if (syswk->app->unkA55C == 2) {
            Box2Bmp_OpenMenu(syswk, sMenu7158, 5);
        } else if (syswk->app->unkA55C == 3) {
            Box2Bmp_OpenMenu(syswk, sMenu716c, 5);
        } else if (syswk->app->unkA55C == 4) {
            Box2Bmp_OpenMenu(syswk, sMenu7180, 5);
        } else if (syswk->app->unkA55C == 5) {
            if (BoxSaveAccessor_CheckFlags(syswk->param->boxes, 2) == TRUE) {
                Box2Bmp_OpenMenu(syswk, sMenu71c0, 6);
            } else {
                Box2Bmp_OpenMenu(syswk, sMenu7108, 5);
            }
        } else if (syswk->app->unkA55C == 6) {
            Box2Bmp_OpenMenu(syswk, sMenu71d8, 6);
        }
        Box2Frame_SlideMenuIn(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, 112);
    case 2:
        syswk->app->subSeq = 0;
        if (syswk->app->unkA55C == 0) {
            Box2Bmp_PrintPrompt(syswk, 2, 24);
        } else {
            Box2Bmp_PrintPrompt(syswk, 3, 24);
        }
        all = FALSE;
        if (syswk->app->unkA55C == 0) {
            if (BoxSaveAccessor_CheckFlags(syswk->param->boxes, 1) == FALSE) {
                all = TRUE;
            }
        } else if (syswk->app->unkA55C == 5) {
            if (BoxSaveAccessor_CheckFlags(syswk->param->boxes, 2) == FALSE) {
                all = TRUE;
            }
        } else if (syswk->app->unkA55C != 6) {
            all = TRUE;
        }
        if (all == TRUE) {
            Box2Ui_ChangeCursor(syswk, 11, 1);
            CursorMove_DisablePos(syswk->app->cursorMove, 0);
        } else {
            Box2Ui_ChangeCursor(syswk, 11, 0);
        }
        return 113;
    }
    return 112;
}

// The wallpaper menu: picks a theme or a wallpaper
static int Box2Seq_WallpaperMenuMain(Box2SysWork *syswk) {
    switch (CursorMove_Update(syswk->app->cursorMove)) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->unkA55C == 0) {
            syswk->app->unkA55C = 5;
            return Box2Seq_AnimFrameButton(syswk, 0, Box2Seq_Goto(syswk, 112));
        }
        return Box2Seq_PickWallpaper(syswk, 0);
    case 1:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->unkA55C == 0) {
            syswk->app->unkA55C = 1;
            return Box2Seq_AnimFrameButton(syswk, 1, Box2Seq_Goto(syswk, 112));
        }
        return Box2Seq_PickWallpaper(syswk, 1);
    case 2:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->unkA55C == 0) {
            syswk->app->unkA55C = 2;
            return Box2Seq_AnimFrameButton(syswk, 2, Box2Seq_Goto(syswk, 112));
        }
        return Box2Seq_PickWallpaper(syswk, 2);
    case 3:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->unkA55C == 0) {
            syswk->app->unkA55C = 3;
            return Box2Seq_AnimFrameButton(syswk, 3, Box2Seq_Goto(syswk, 112));
        }
        return Box2Seq_PickWallpaper(syswk, 3);
    case 4:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->unkA55C == 0) {
            syswk->app->unkA55C = 4;
            return Box2Seq_AnimFrameButton(syswk, 4, Box2Seq_Goto(syswk, 112));
        }
        if (syswk->app->unkA55C == 5 && BoxSaveAccessor_CheckFlags(syswk->param->boxes, 2) == TRUE) {
            syswk->app->unkA55C = 6;
            return Box2Seq_AnimFrameButton(syswk, 4, Box2Seq_Goto(syswk, 112));
        }
        if (syswk->app->unkA55C == 6) {
            syswk->app->unkA55C = 5;
            return Box2Seq_AnimFrameButton(syswk, 4, Box2Seq_Goto(syswk, 112));
        }
        return Box2Seq_PickWallpaper(syswk, 4);
    case 5:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (syswk->app->unkA55C == 0) {
            return Box2Seq_AnimFrameButton(syswk, 5, Box2Seq_Goto(syswk, 107));
        }
        syswk->app->unkA55C = 0;
        return Box2Seq_AnimFrameButton(syswk, 5, Box2Seq_Goto(syswk, 112));
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (syswk->app->unkA55C == 0) {
            return Box2Seq_AnimFrameButton(syswk, 5, Box2Seq_Goto(syswk, 107));
        }
        syswk->app->unkA55C = 0;
        return Box2Seq_AnimFrameButton(syswk, 5, Box2Seq_Goto(syswk, 112));
    case CURSORMOVE_CURSOR_MOVE:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return Box2Seq_RunRequestedVFunc(syswk, Box2Seq_PokeMoveEndTo(syswk, 113));
    }
    return 113;
}

// Changes the box's wallpaper behind a fade to white
static int Box2Seq_WallpaperChange(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        Box2Obj_ShowCursor(syswk, 0);
        PaletteFade_LoadFromVRAM(syswk->app->palFade, 0, 0, 0x200);
        PaletteFade_StartFade(syswk->app->palFade, 1, 0xc000, 0, 0, 16, 0x7fff, GFL_VBlankGetTCBMgr());
        syswk->nextSeq = 114;
        syswk->app->subSeq++;
        return BOX2SEQ_PALETTE_FADE;
    case 1:
        Box2Main_WallPaperChange(syswk, syswk->app->wallpaperPos);
        BoxSaveAccessor_SetWallpaper(syswk->param->boxes, syswk->tray, syswk->app->wallpaperPos);
        Box2Obj_MakeTrayIconChar(syswk, syswk->tray);
        syswk->app->subSeq++;
        break;
    case 2:
        PaletteFade_StartFade(syswk->app->palFade, 1, 0xc000, 0, 16, 0, 0x7fff, GFL_VBlankGetTCBMgr());
        syswk->nextSeq = 114;
        syswk->app->subSeq++;
        return BOX2SEQ_PALETTE_FADE;
    case 3:
        Box2Obj_ShowCursor(syswk, 1);
        syswk->app->subSeq = 0;
        return 113;
    }
    return 114;
}

static int Box2Seq_CallNameInput(Box2SysWork *syswk) {
    return Box2Seq_WipeOutCallSubProc(syswk, 2);
}

// Opens the box's name entry
static int Box2Seq_NameInputOpen(Box2SysWork *syswk) {
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
    Box2Obj_SetToolButtons(syswk, 9, 0);
    Box2Bmp_PrintContinue(syswk, 24);
    DimActorsForNameInput(syswk);
    return Box2Seq_OpenYesNo(syswk, 4);
}

// Goes to another sequence from its first step
static int Box2Seq_Goto(Box2SysWork *syswk, int seq) {
    syswk->app->subSeq = 0;
    return seq;
}

// Wipes out, then goes to nextSeq
int Box2Seq_WipeOut(Box2SysWork *syswk, int nextSeq) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, 0, 6, 1, HEAPID_BOX2_APP);
    syswk->app->wipeSeq = nextSeq;
    return BOX2SEQ_WIPE;
}

// Wipes in, then goes to nextSeq
static int Box2Seq_WipeIn(Box2SysWork *syswk, int nextSeq) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0, 6, 1, HEAPID_BOX2_APP);
    syswk->app->wipeSeq = nextSeq;
    return BOX2SEQ_WIPE;
}

// Runs a function each frame until it ends, then goes to nextSeq
int Box2Seq_RunVFunc(Box2SysWork *syswk, Box2VFunc func, int nextSeq) {
    syswk->app->vfuncNextSeq = nextSeq;
    Box2Main_VFuncSet(syswk->app, func);
    return BOX2SEQ_VFUNC;
}

// Runs the requested function each frame until it ends, then goes to nextSeq
static int Box2Seq_RunRequestedVFunc(Box2SysWork *syswk, int nextSeq) {
    syswk->app->vfuncNextSeq = nextSeq;
    Box2Main_VFuncReqSet(syswk->app);
    return BOX2SEQ_VFUNC;
}

static int Box2Seq_PokeMoveEndTo(Box2SysWork *syswk, int seq) {
    syswk->nextSeq = seq;
    return 13;
}

// Opens a yes/no question
static int Box2Seq_OpenYesNo(Box2SysWork *syswk, u32 type) {
    syswk->app->ynID = type;
    if (type == 1) {
        Box2Main_OpenYesNo(syswk, 1);
    } else {
        Box2Main_OpenYesNo(syswk, 0);
    }
    setKeypressFramecounts(syswk->app->keyRepeatWait, syswk->app->keyRepeatStart);
    return BOX2SEQ_YESNO;
}

// Takes the Pokémon's held item into the bag
static int Box2Seq_YesTakeItem(Box2SysWork *syswk) {
    u32 item = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);

    if (BagSave_AddItem(syswk->param->bag, item, 1, HEAPID_BOX2_APP) == TRUE) {
        BoxPkm *pkm = Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos);

        Box2Bmp_PrintTookItem(syswk, item, 24);
        Box2Main_SetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, 0);
        if (Box2Main_PokeItemFormChange(syswk, pkm) == TRUE) {
            Box2Main_RecalcPartyStats(syswk, syswk->pos);
            Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
        Box2Main_PokeInfoRewrite(syswk, syswk->pos);
        Box2Obj_UpdatePokeIconPalette(syswk, syswk->tray, syswk->pos);
    } else {
        GFL_SndSEPlay(SEQ_SE_BEEP);
        Box2Bmp_PrintBagFull(syswk, 24);
    }
    if (GCTX_HIDIsUsingTouch() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
        Box2Obj_ShowCursor(syswk, 0);
    }
    syswk->nextSeq = 14;
    return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
}

// Releasing: starts the release
static int Box2Seq_YesRelease(Box2SysWork *syswk) {
    Box2Obj_ShowPokeOutline(syswk, 0);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    Box2Main_PokeFreeCreate(syswk);
    Box2PokeFree_Start(syswk->app->subWork);
    if (GCTX_HIDIsUsingTouch() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
        Box2Obj_ShowCursor(syswk, 0);
    }
    return 102;
}

// Releasing: the release was cancelled
static int Box2Seq_NoRelease(Box2SysWork *syswk) {
    Box2Obj_ShowPokeOutline(syswk, 1);
    Box2Bmp_ClearMsgWindowNow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    if (syswk->pos < BOX2_PARTY_POS) {
        Box2Obj_ShowScrollArrows(syswk->app, 1);
        Box2Obj_SetIconsBlend(syswk, 1, 0);
    } else {
        Box2Obj_SetIconsBlend(syswk, 2, 0);
    }
    if (GCTX_HIDIsUsingTouch() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
        Box2Obj_ShowCursor(syswk, 0);
    }
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncFrameMove, Box2Seq_Goto(syswk, 14));
}

// The item arrangement: puts the held item in the bag
static int Box2Seq_YesItemToBag(Box2SysWork *syswk) {
    if (GCTX_HIDIsUsingTouch() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
        Box2Obj_ShowCursor(syswk, 0);
    }
    if (BagSave_AddItem(syswk->param->bag, syswk->app->getItem, 1, HEAPID_BOX2_APP) == TRUE) {
        BoxPkm *pkm = Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos);

        Box2Main_SetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, 0);
        if (Box2Main_PokeItemFormChange(syswk, pkm) == TRUE) {
            Box2Main_RecalcPartyStats(syswk, syswk->pos);
            Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        Box2Obj_ShowPokeOutline(syswk, 0);
        Box2Bmp_ClearMsgWindow(syswk->app, 24);
        Box2Main_PutMessageFrame(syswk);
        Box2Frame_ShowPartyButton(syswk->app);
        Box2Obj_SetItemIconAffine(syswk->app, TRUE);
        Box2Obj_SetAnim(syswk->app, BOX2_ACTOR_ITEM_ICON, 2);
        return 69;
    }
    GFL_SndSEPlay(SEQ_SE_BEEP);
    Box2Bmp_PrintItemMsg(syswk, 6, 24);
    syswk->nextSeq = 14;
    return Box2Seq_Goto(syswk, BOX2SEQ_TRGWAIT);
}

// Leaves the box
static int Box2Seq_Leave(Box2SysWork *syswk) {
    GFL_SndSEPlay(SEQ_SE_PC_LOGOFF);
    Box2Main_UpdateChatter(syswk);
    syswk->nextSeq = BOX2SEQ_END;
    return Box2Seq_WipeIn(syswk, 1);
}

// Closes a message and goes back to the mode's main state
static int Box2Seq_ReturnToMain(Box2SysWork *syswk) {
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    if (GCTX_HIDIsUsingTouch() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
    }
    Box2Obj_SetToolButtons(syswk, 9, 1);
    switch (syswk->param->mode) {
    case 0:
        Box2Obj_SetIconsBlend(syswk, 2, 0);
        return 59;
    case 1:
        Box2Obj_ShowScrollArrows(syswk->app, 1);
        Box2Obj_SetIconsBlend(syswk, 1, 0);
        return 54;
    case 2:
        if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
            Box2Obj_SetIconsBlend(syswk, 2, 0);
            Box2Frame_ShowBoxListButton(syswk->app);
            return 27;
        }
        Box2Obj_ShowScrollArrows(syswk->app, 1);
        Box2Obj_SetIconsBlend(syswk, 1, 0);
        Box2Frame_ShowPartyButton(syswk->app);
        return 17;
    case 3:
        if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
            Box2Obj_SetIconsBlend(syswk, 0x82, 0);
            Box2Frame_ShowBoxListButton(syswk->app);
            return 80;
        }
        Box2Obj_ShowScrollArrows(syswk->app, 1);
        Box2Obj_SetIconsBlend(syswk, 0x81, 0);
        Box2Frame_ShowPartyButton(syswk->app);
        return 67;
    case 4:
        if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
            Box2Obj_SetIconsBlend(syswk, 2, 0);
            Box2Frame_ShowBoxListButton(syswk->app);
            return 49;
        }
        Box2Obj_ShowScrollArrows(syswk->app, 1);
        Box2Obj_SetIconsBlend(syswk, 1, 0);
        Box2Frame_ShowPartyButton(syswk->app);
        return 45;
    case 5:
        Box2Obj_ShowScrollArrows(syswk->app, 1);
        Box2Obj_SetIconsBlend(syswk, 0x81, 1);
        return 85;
    }
    return 17;
}

// Mode 5: picks the Pokémon and leaves the box
static int Box2Seq_YesDreamWorldPick(Box2SysWork *syswk) {
    Box2Main_UpdateChatter(syswk);
    syswk->param->tray = syswk->tray;
    syswk->param->position = syswk->pos;
    syswk->nextSeq = BOX2SEQ_END;
    return Box2Seq_WipeIn(syswk, 1);
}

// Mode 5: the Pokémon wasn't picked
static int Box2Seq_NoDreamWorldPick(Box2SysWork *syswk) {
    Box2Bmp_ClearMsgWindow(syswk->app, 27);
    Box2Main_PutMessageFrame(syswk);
    CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
    Box2Obj_SetIconsBlend(syswk, 0x81, 1);
    Box2Obj_ShowPokeOutline(syswk, 0);
    if (GCTX_HIDIsUsingTouch() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
    }
    return 85;
}

static int Box2Seq_NoBackToMenu(Box2SysWork *syswk) {
    if (GCTX_HIDIsUsingTouch() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
        Box2Obj_ShowCursor(syswk, 0);
    }
    return Box2Seq_OpenMenu(syswk);
}

// Animates a frame's button, then goes to seq
static int Box2Seq_AnimFrameButton(Box2SysWork *syswk, u32 frame, int seq) {
    Box2Main_SetFrameButtonAnm(syswk, frame);
    syswk->nextSeq = seq;
    return BOX2SEQ_BUTTON_ANM;
}

// Animates one of the markings' buttons, then goes to seq
static int Box2Seq_AnimMarkButton(Box2SysWork *syswk, u32 button, int seq) {
    syswk->app->bawk.mode = BOX2_BTN_ANM_MODE_BG;
    syswk->app->bawk.id = BGWinFrame_GetBG(syswk->app->bgWinFrame, 7);
    syswk->app->bawk.pal1 = 13;
    syswk->app->bawk.pal2 = 12;
    syswk->app->bawk.seq = 0;
    syswk->app->bawk.cnt = 0;
    if (button == 0) {
        syswk->app->bawk.py = 14;
    } else {
        syswk->app->bawk.py = 17;
    }
    syswk->app->bawk.px = 21;
    syswk->app->bawk.sx = 11;
    syswk->app->bawk.sy = 3;
    syswk->nextSeq = seq;
    return BOX2SEQ_BUTTON_ANM;
}

// Animates an actor's button, then goes to seq
static int Box2Seq_AnimActorButton(Box2SysWork *syswk, u32 id, u32 pal, int seq) {
    syswk->app->bawk.mode = BOX2_BTN_ANM_MODE_OBJ;
    syswk->app->bawk.id = id;
    syswk->app->bawk.pal1 = pal;
    syswk->app->bawk.pal2 = 0;
    syswk->app->bawk.seq = 0;
    syswk->app->bawk.cnt = 0;
    syswk->app->bawk.px = 0;
    syswk->app->bawk.py = 0;
    syswk->app->bawk.sx = 0;
    syswk->app->bawk.sy = 0;
    syswk->nextSeq = seq;
    return BOX2SEQ_BUTTON_ANM;
}

// Wipes out to call a sub process
static int Box2Seq_WipeOutCallSubProc(Box2SysWork *syswk, u32 type) {
    if (CursorMove_IsCursorVisible(syswk->app->cursorMove) == TRUE) {
        GCTX_HIDSetUsingTouch(FALSE);
    } else {
        GCTX_HIDSetUsingTouch(TRUE);
    }
    syswk->nextSeq = BOX2SEQ_SUBPROC_CALL;
    syswk->subProcType = type;
    return Box2Seq_WipeIn(syswk, 1);
}

// Back from the bag after giving an item, in the arrangement
static int Box2Seq_ArrangeGiveItemReturn(Box2SysWork *syswk) {
    int seq;

    switch (syswk->unk13) {
    case 0:
        Box2Frame_ShowPartyButton(syswk->app);
        seq = 17;
        break;
    case 1:
        PutPartyLeft(syswk);
        Box2Obj_SetIconsBlend(syswk, 1, 1);
        Box2Obj_ShowScrollArrows(syswk->app, 0);
        Box2Ui_ChangeCursor(syswk, 5, 11);
        Box2Frame_ShowBoxListButton(syswk->app);
        seq = 27;
        break;
    }
    Box2Obj_PutPokeOutline(syswk);
    if (syswk->subRet == 0) {
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        Box2Bmp_OpenMenu(syswk, sMenu71f0, 6);
        Box2Frame_ShowMenu(syswk->app->bgWinFrame);
        Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
        return seq;
    }
    if (PML_ItemIsMail(syswk->subRet) == FALSE) {
        BoxPkm *pkm = Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos);

        Box2Main_SetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, syswk->subRet);
        Box2Obj_UpdatePokeIconPalette(syswk, syswk->tray, syswk->pos);
        if (Box2Main_PokeItemFormChange(syswk, pkm) == TRUE) {
            Box2Main_RecalcPartyStats(syswk, syswk->pos);
            Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
        BagSave_SubItem(syswk->param->bag, syswk->subRet, 1, HEAPID_TAIL(HEAPID_BOX2_APP));
    }
    Box2Main_PokeInfoPut(syswk, syswk->pos);
    Box2Obj_ShowCursor(syswk, 0);
    return 96;
}

// Back from the bag after giving an item, in the item arrangement
static int Box2Seq_ItemGiveItemReturn(Box2SysWork *syswk) {
    int seq;

    switch (syswk->unk13) {
    case 0:
        Box2Obj_PutItemIconAtIcon(syswk->app, syswk->pos, 0);
        seq = 67;
        break;
    case 1:
        PutPartyLeft(syswk);
        Box2Obj_ShowScrollArrows(syswk->app, 0);
        Box2Obj_PutItemIconAtIcon(syswk->app, syswk->pos, 1);
        Box2Ui_ChangeCursor(syswk, 8, 10);
        seq = 80;
        break;
    }
    Box2Seq_OpenItemMenu(syswk, (s16)syswk->subRet);
    if (syswk->subRet == 0) {
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        Box2Frame_ShowMenu(syswk->app->bgWinFrame);
        Box2Bmp_PrintItemMsg(syswk, 0, 24);
        if (syswk->unk13 == 0) {
            Box2Obj_SetIconsBlend(syswk, 0x81, 1);
        } else {
            Box2Obj_SetIconsBlend(syswk, 1, 1);
            Box2Obj_SetIconsBlend(syswk, 0x82, 1);
        }
        return seq;
    }
    if (PML_ItemIsMail(syswk->subRet) == FALSE) {
        BoxPkm *pkm = Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos);

        Box2Main_SetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, syswk->subRet);
        if (Box2Main_PokeItemFormChange(syswk, pkm) == TRUE) {
            Box2Main_RecalcPartyStats(syswk, syswk->pos);
            Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
        BagSave_SubItem(syswk->param->bag, syswk->subRet, 1, HEAPID_TAIL(HEAPID_BOX2_APP));
        syswk->app->getItem = syswk->subRet;
        Box2Obj_SetItemIcon(syswk->app, syswk->app->getItem);
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, TRUE);
        Box2Obj_InitItemOutline(syswk->app);
        Box2Obj_ShowItemOutline(syswk->app);
    }
    if (syswk->unk13 == 0) {
        Box2Obj_SetIconsBlend(syswk, 0x81, 1);
        Box2Frame_ShowPartyButton(syswk->app);
    } else {
        Box2Obj_SetIconsBlend(syswk, 1, 1);
        Box2Obj_SetIconsBlend(syswk, 0x82, 1);
        Box2Frame_ShowBoxListButton(syswk->app);
    }
    Box2Main_PokeInfoPut(syswk, syswk->pos);
    Box2Obj_ShowCursor(syswk, 0);
    return 96;
}

// Back from the bag after giving an item, in mode 4
static int Box2Seq_BattleBoxGiveItemReturn(Box2SysWork *syswk) {
    int seq;

    switch (syswk->unk13) {
    case 0:
        Box2Frame_ShowPartyButton(syswk->app);
        seq = 45;
        break;
    case 1:
        PutPartyLeft(syswk);
        Box2Obj_SetIconsBlend(syswk, 1, 1);
        Box2Obj_ShowScrollArrows(syswk->app, 0);
        Box2Ui_ChangeCursor(syswk, 14, 11);
        Box2Frame_ShowBoxListButton(syswk->app);
        seq = 49;
        break;
    }
    Box2Obj_PutPokeOutline(syswk);
    if (syswk->subRet == 0) {
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        Box2Bmp_OpenMenu(syswk, sMenu70f8, 4);
        Box2Frame_ShowMenu(syswk->app->bgWinFrame);
        Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
        return seq;
    }
    if (PML_ItemIsMail(syswk->subRet) == FALSE) {
        BoxPkm *pkm = Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos);

        Box2Main_SetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, syswk->subRet);
        Box2Obj_UpdatePokeIconPalette(syswk, syswk->tray, syswk->pos);
        if (Box2Main_PokeItemFormChange(syswk, pkm) == TRUE) {
            Box2Main_RecalcPartyStats(syswk, syswk->pos);
            Box2Obj_SetTrayPokeIcon(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
        BagSave_SubItem(syswk->param->bag, syswk->subRet, 1, HEAPID_TAIL(HEAPID_BOX2_APP));
    }
    Box2Main_PokeInfoPut(syswk, syswk->pos);
    Box2Obj_ShowCursor(syswk, 0);
    return 96;
}

// Back from the summary in mode 1
static int Box2Seq_WithdrawSummaryReturn(Box2SysWork *syswk) {
    Box2Bmp_OpenMenu(syswk, sMenu7194, 5);
    Box2Frame_ShowMenu(syswk->app->bgWinFrame);
    Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
    Box2Obj_PutPokeOutline(syswk);
    return 54;
}

// Back from the summary in mode 0
static int Box2Seq_DepositSummaryReturn(Box2SysWork *syswk) {
    Box2Bmp_OpenMenu(syswk, sMenu7130, 5);
    Box2Frame_ShowMenu(syswk->app->bgWinFrame);
    Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
    Box2Obj_SetIconsBlend(syswk, 1, 1);
    Box2Obj_ShowScrollArrows(syswk->app, 0);
    PutPartyLeft(syswk);
    Box2Obj_PutPokeOutline(syswk);
    return 59;
}

// Back from the summary in the arrangement
static int Box2Seq_ArrangeSummaryReturn(Box2SysWork *syswk) {
    switch (syswk->unk13) {
    case 0:
        Box2Bmp_OpenMenu(syswk, sMenu71f0, 6);
        Box2Frame_ShowMenu(syswk->app->bgWinFrame);
        Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
        Box2Obj_PutPokeOutline(syswk);
        return 17;
    case 1:
        Box2Bmp_OpenMenu(syswk, sMenu71f0, 6);
        Box2Frame_ShowMenu(syswk->app->bgWinFrame);
        Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
        Box2Obj_SetIconsBlend(syswk, 1, 1);
        Box2Obj_ShowScrollArrows(syswk->app, 0);
        Box2Ui_ChangeCursor(syswk, 5, 10);
        PutPartyLeft(syswk);
        Box2Obj_PutPokeOutline(syswk);
        return 27;
    case 2:
        if (syswk->unk1B == 1) {
            Box2Obj_PutPartyIconsRightBelow(syswk);
        }
        Box2Ui_ChangeCursor(syswk, 4, syswk->pos);
        Box2Main_OpenBoxList(syswk);
        while (Box2Main_VFuncBoxListSlideOut(syswk) != FALSE) {
        }
        Box2Obj_PlaceTrayHighlight(syswk);
        Box2Frame_ShowPartyButton(syswk->app);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->moveMode == 2) {
            return 36;
        }
        return 21;
    case 3:
        syswk->app->rangeHeight = 1;
        syswk->app->rangeWidth = 1;
        if (syswk->pos >= BOX2_PARTY_POS) {
            Box2Obj_PutPartyIconsRightBelow(syswk);
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        } else if (syswk->tray == syswk->getTray) {
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        } else {
            Box2Obj_SetTrayPokeIcon(syswk, syswk->getTray, syswk->pos, syswk->app->pokeIconId[BOX2_BOXLIST_POS]);
        }
        Box2Main_HandGetPokeSet(syswk);
        Box2Obj_UpdateOutlinePriority(syswk, BOX2_BOXLIST_POS);
        Box2Ui_ChangeCursor(syswk, 4, syswk->unk1D);
        Box2Main_OpenBoxList(syswk);
        while (Box2Main_VFuncBoxListSlideOut(syswk) != FALSE) {
        }
        Box2Obj_PlaceTrayHighlight(syswk);
        Box2Frame_ShowPartyButton(syswk->app);
        if (syswk->unk1D >= 34 && syswk->unk1D <= 37) {
            Box2Obj_ShowBoxNameFrame(syswk, syswk->unk1D - 34, 1);
        }
        Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
        return 21;
    case 4:
        PutPartyRight(syswk);
        Box2Ui_ChangeCursor(syswk, 6, syswk->pos);
        Box2Frame_ShowBoxListButton(syswk->app);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->moveMode == 2) {
            return 42;
        }
        return 32;
    case 5:
        syswk->app->rangeHeight = 1;
        syswk->app->rangeWidth = 1;
        PutPartyRight(syswk);
        if (syswk->pos < BOX2_PARTY_POS && syswk->tray != syswk->getTray) {
            Box2Obj_SetTrayPokeIcon(syswk, syswk->getTray, syswk->pos, syswk->app->pokeIconId[BOX2_BOXLIST_POS]);
        } else {
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        }
        Box2Main_HandGetPokeSet(syswk);
        Box2Obj_UpdateOutlinePriority(syswk, BOX2_BOXLIST_POS);
        Box2Ui_ChangeCursor(syswk, 6, syswk->unk1D);
        Box2Frame_ShowBoxListButton(syswk->app);
        Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
        return 32;
    }
    return 0;
}

// Back from the summary in mode 4
static int Box2Seq_BattleBoxSummaryReturn(Box2SysWork *syswk) {
    switch (syswk->unk13) {
    case 0:
        Box2Bmp_OpenMenu(syswk, sMenu70f8, 4);
        Box2Frame_ShowMenu(syswk->app->bgWinFrame);
        Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
        Box2Ui_ChangeCursor(syswk, 13, 37);
        Box2Obj_PutPokeOutline(syswk);
        return 45;
    case 1:
        Box2Bmp_OpenMenu(syswk, sMenu70f8, 4);
        Box2Frame_ShowMenu(syswk->app->bgWinFrame);
        Box2Bmp_PrintPokeMoveMsg(syswk, syswk->pos, 1, 24);
        Box2Obj_SetIconsBlend(syswk, 1, 1);
        Box2Obj_ShowScrollArrows(syswk->app, 0);
        Box2Ui_ChangeCursor(syswk, 14, 10);
        PutPartyLeft(syswk);
        Box2Obj_PutPokeOutline(syswk);
        return 49;
    case 4:
        PutPartyRight(syswk);
        Box2Ui_ChangeCursor(syswk, 6, syswk->pos);
        CursorMove_DisablePos(syswk->app->cursorMove, 39);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->moveMode == 2) {
            return 42;
        }
        return 32;
    case 5:
        syswk->app->rangeHeight = 1;
        syswk->app->rangeWidth = 1;
        PutPartyRight(syswk);
        if (syswk->pos < BOX2_PARTY_POS && syswk->tray != syswk->getTray) {
            Box2Obj_SetTrayPokeIcon(syswk, syswk->getTray, syswk->pos, syswk->app->pokeIconId[BOX2_BOXLIST_POS]);
        } else {
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        }
        Box2Main_HandGetPokeSet(syswk);
        Box2Obj_UpdateOutlinePriority(syswk, BOX2_BOXLIST_POS);
        Box2Ui_ChangeCursor(syswk, 6, syswk->unk1D);
        CursorMove_DisablePos(syswk->app->cursorMove, 39);
        Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
        return 32;
    }
    return 0;
}

// Back from the summary while moving Pokémon in the arrangement
static int Box2Seq_ArrangeMoveSummaryReturn(Box2SysWork *syswk) {
    switch (syswk->unk13) {
    case 2:
        if (syswk->moveMode == 0) {
            Box2Frame_ShowPartyButton(syswk->app);
            Box2Ui_ChangeCursor(syswk, 3, syswk->pos);
            syswk->pos = BOX2_GET_NONE;
            return 17;
        }
        if (syswk->unk1B == 1) {
            Box2Obj_PutPartyIconsRightBelow(syswk);
        }
        Box2Ui_ChangeCursor(syswk, 4, syswk->pos);
        Box2Main_OpenBoxList(syswk);
        while (Box2Main_VFuncBoxListSlideOut(syswk) != FALSE) {
        }
        Box2Obj_PlaceTrayHighlight(syswk);
        Box2Frame_ShowPartyButton(syswk->app);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->moveMode == 2) {
            return 36;
        }
        return 21;
    case 3:
        syswk->app->rangeHeight = 1;
        syswk->app->rangeWidth = 1;
        if (syswk->pos >= BOX2_PARTY_POS) {
            Box2Obj_PutPartyIconsRightBelow(syswk);
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        } else if (syswk->tray == syswk->getTray) {
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        } else {
            Box2Obj_SetTrayPokeIcon(syswk, syswk->getTray, syswk->pos, syswk->app->pokeIconId[BOX2_BOXLIST_POS]);
        }
        Box2Main_HandGetPokeSet(syswk);
        Box2Obj_UpdateOutlinePriority(syswk, BOX2_BOXLIST_POS);
        Box2Ui_ChangeCursor(syswk, 4, syswk->unk1D);
        Box2Main_OpenBoxList(syswk);
        while (Box2Main_VFuncBoxListSlideOut(syswk) != FALSE) {
        }
        Box2Obj_PlaceTrayHighlight(syswk);
        Box2Frame_ShowPartyButton(syswk->app);
        if (syswk->unk1D >= 34 && syswk->unk1D <= 37) {
            Box2Obj_ShowBoxNameFrame(syswk, syswk->unk1D - 34, 1);
        }
        Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
        return 21;
    case 4:
        if (syswk->moveMode == 0) {
            PutPartyLeft(syswk);
            Box2Frame_ShowBoxListButton(syswk->app);
            Box2Obj_SetIconsBlend(syswk, 1, 1);
            Box2Obj_ShowScrollArrows(syswk->app, 0);
            Box2Ui_ChangeCursor(syswk, 5, syswk->pos - BOX2_PARTY_POS);
            syswk->pos = BOX2_GET_NONE;
            return 27;
        }
        PutPartyRight(syswk);
        Box2Ui_ChangeCursor(syswk, 6, syswk->pos);
        Box2Frame_ShowBoxListButton(syswk->app);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->moveMode == 2) {
            return 42;
        }
        return 32;
    case 5:
        syswk->app->rangeHeight = 1;
        syswk->app->rangeWidth = 1;
        PutPartyRight(syswk);
        if (syswk->pos < BOX2_PARTY_POS && syswk->tray != syswk->getTray) {
            Box2Obj_SetTrayPokeIcon(syswk, syswk->getTray, syswk->pos, syswk->app->pokeIconId[BOX2_BOXLIST_POS]);
        } else {
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        }
        Box2Main_HandGetPokeSet(syswk);
        Box2Obj_UpdateOutlinePriority(syswk, BOX2_BOXLIST_POS);
        Box2Ui_ChangeCursor(syswk, 6, syswk->unk1D);
        Box2Frame_ShowBoxListButton(syswk->app);
        Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
        return 32;
    }
    return 0;
}

// Back from the summary while moving Pokémon in mode 4
static int Box2Seq_BattleBoxMoveSummaryReturn(Box2SysWork *syswk) {
    switch (syswk->unk13) {
    case 2:
        Box2Frame_ShowPartyButton(syswk->app);
        Box2Ui_ChangeCursor(syswk, 13, syswk->pos);
        syswk->pos = BOX2_GET_NONE;
        return 45;
    case 4:
        if (syswk->moveMode == 0) {
            PutPartyLeft(syswk);
            Box2Frame_ShowBoxListButton(syswk->app);
            Box2Obj_SetIconsBlend(syswk, 1, 1);
            Box2Obj_ShowScrollArrows(syswk->app, 0);
            Box2Ui_ChangeCursor(syswk, 14, syswk->pos - BOX2_PARTY_POS);
            syswk->pos = BOX2_GET_NONE;
            return 49;
        }
        PutPartyRight(syswk);
        Box2Ui_ChangeCursor(syswk, 6, syswk->pos);
        CursorMove_DisablePos(syswk->app->cursorMove, 39);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->moveMode == 2) {
            return 42;
        }
        return 32;
    case 5:
        syswk->app->rangeHeight = 1;
        syswk->app->rangeWidth = 1;
        PutPartyRight(syswk);
        if (syswk->pos < BOX2_PARTY_POS && syswk->tray != syswk->getTray) {
            Box2Obj_SetTrayPokeIcon(syswk, syswk->getTray, syswk->pos, syswk->app->pokeIconId[BOX2_BOXLIST_POS]);
        } else {
            Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
        }
        Box2Main_HandGetPokeSet(syswk);
        Box2Obj_UpdateOutlinePriority(syswk, BOX2_BOXLIST_POS);
        Box2Ui_ChangeCursor(syswk, 6, syswk->unk1D);
        CursorMove_DisablePos(syswk->app->cursorMove, 39);
        Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
        return 32;
    }
    return 0;
}

// Scrolls the tray left
static int Box2Seq_ScrollTrayLeft(Box2SysWork *syswk, u32 frameOut, int seq) {
    Box2Main_ScrollTray(syswk, FALSE);
    if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        Box2Bmp_ClearMsgWindow(syswk->app, 24);
        Box2Main_PutMessageFrame(syswk);
        if (frameOut == 1) {
            Box2Frame_ShowPartyButton(syswk->app);
        }
    }
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncTrayScrollLeft, seq);
}

// Scrolls the tray right
static int Box2Seq_ScrollTrayRight(Box2SysWork *syswk, u32 frameOut, int seq) {
    Box2Main_ScrollTray(syswk, TRUE);
    if (Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
        Box2Frame_SlideMenuOut(syswk->app->bgWinFrame);
        Box2Bmp_ClearMsgWindow(syswk->app, 24);
        Box2Main_PutMessageFrame(syswk);
        if (frameOut == 1) {
            Box2Frame_ShowPartyButton(syswk->app);
        }
    }
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncTrayScrollRight, seq);
}

// Moving Pokémon: picks up a touched Pokémon
static int Box2Seq_ArrangePickUpTouched(Box2SysWork *syswk, u32 pos) {
    syswk->unk1B = 0;
    syswk->moveMode = 0;
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    Box2Obj_SetIconPriority(syswk->app, syswk->pos, FALSE);
    Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
    Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
    AllocPokeMoveWork(syswk);
    Box2Obj_SetButtons(syswk, 2, 2, 1, 1);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    Box2Frame_ShowPartyButton(syswk->app);
    syswk->nextSeq = 21;
    syswk->unk1A = 0;
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMoveTouch, 18);
}

// Moving Pokémon with the party out: picks up a touched Pokémon
static int Box2Seq_MovePartyPickUpTouched(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    syswk->unk1B = 0;
    Box2Obj_SetIconPriority(syswk->app, syswk->pos, FALSE);
    Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
    Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
    AllocPokeMoveWork(syswk);
    Box2Obj_SetButtons(syswk, 2, 2, 1, 1);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    syswk->nextSeq = 32;
    syswk->unk1A = 0;
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncRangeMoveTouch, 46);
}

// Mode 1: picks up a touched Pokémon to take it into the party
static int Box2Seq_WithdrawPickUpTouched(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    Box2Obj_SetIconPriority(syswk->app, syswk->pos, FALSE);
    Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
    Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
    Box2Obj_ShowScrollArrows(syswk->app, 0);
    Box2Obj_PutPartyIconsRightBelow(syswk);
    Box2Obj_SetIconsBlend(syswk, 1, 1);
    Box2Obj_SetIconsBlend(syswk, 2, 1);
    Box2Obj_SetIconBlend(syswk->app, BOX2_BOXLIST_POS, FALSE);
    AllocPokeMoveWork(syswk);
    Box2Obj_SetButtons(syswk, 2, 2, 1, 1);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    syswk->unk1A = 2;
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyInTouch, 58);
}

// Opens a Pokémon's menu
static int Box2Seq_OpenPokeMenu(Box2SysWork *syswk, u32 pos, int seq) {
    syswk->pos = pos;
    syswk->app->unkA552 = 1;
    Box2Obj_SetIconPriority(syswk->app, syswk->pos, TRUE);
    Box2Obj_PutPokeOutline(syswk);
    Box2Frame_SlideMenuIn(syswk->app->bgWinFrame);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMoveFrame, Box2Seq_PokeMoveEndTo(syswk, seq));
}

// The party's main state: picks up a touched Pokémon
static int Box2Seq_ArrangePartyPickUpTouched(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    syswk->moveMode = 0;
    syswk->getTray = syswk->tray;
    syswk->unk1B = 1;
    Box2Obj_SetIconPriority(syswk->app, syswk->pos, FALSE);
    Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
    Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
    PokeParty_GetPkm(syswk->param->party, pos - BOX2_PARTY_POS);
    AllocPokeMoveWork(syswk);
    Box2Obj_SetButtons(syswk, 2, 2, 1, 1);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    Box2Frame_ShowBoxListButton(syswk->app);
    syswk->nextSeq = 32;
    syswk->unk1A = 1;
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMoveTouchParty, 30);
}

// The party's main state from the box's: picks up a touched Pokémon
static int Box2Seq_ArrangeBoxPartyPickUpTouched(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    syswk->unk1B = 1;
    Box2Obj_SetIconPriority(syswk->app, syswk->pos, FALSE);
    Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
    Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
    PokeParty_GetPkm(syswk->param->party, pos - BOX2_PARTY_POS);
    AllocPokeMoveWork(syswk);
    Box2Obj_SetButtons(syswk, 2, 2, 1, 1);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    syswk->nextSeq = 32;
    syswk->unk1A = 1;
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMoveTouchParty, 52);
}

// Mode 0: picks up a touched Pokémon of the party to deposit it
static int Box2Seq_DepositPickUpTouched(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    Box2Obj_SetIconPriority(syswk->app, syswk->pos, FALSE);
    Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
    Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
    Box2Obj_SetIconsBlend(syswk, 2, 1);
    Box2Obj_SetIconBlend(syswk->app, BOX2_BOXLIST_POS, FALSE);
    PokeParty_GetPkm(syswk->param->party, pos - BOX2_PARTY_POS);
    AllocPokeMoveWork(syswk);
    Box2Obj_SetButtons(syswk, 2, 2, 1, 1);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    syswk->unk1A = 1;
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPartyOutTouch, 66);
}

// Opens a party Pokémon's menu
static int Box2Seq_OpenPartyPokeMenu(Box2SysWork *syswk, u32 pos, int seq) {
    syswk->pos = pos;
    syswk->app->unkA552 = 1;
    Box2Obj_SetIconPriority(syswk->app, syswk->pos, TRUE);
    Box2Obj_PutPokeOutline(syswk);
    Box2Frame_SlideMenuIn(syswk->app->bgWinFrame);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncCursorMoveFrame, Box2Seq_PokeMoveEndTo(syswk, seq));
}

// Moving Pokémon: picks up a touched Pokémon
static int Box2Seq_MovePickUpTouched(Box2SysWork *syswk, u32 pos, int seq) {
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    syswk->app->oldCurPos = pos;
    Box2Obj_PickUpRangeIcons(syswk, syswk->pos, 1, 1);
    Box2Obj_SetIconPriority(syswk->app, BOX2_BOXLIST_POS, FALSE);
    Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
    Box2Obj_SetButtons(syswk, 2, 1, 2, 1);
    AllocPokeMoveWork(syswk);
    syswk->nextSeq = seq;
    syswk->unk1A = 2;
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMoveTouch, 23);
}

// Picking a range: picks up the touched range
static int Box2Seq_RangePickUpTouched(Box2SysWork *syswk, int seq) {
    syswk->app->oldCurPos = syswk->pos;
    Box2Obj_ResetRangeIconPriority(syswk->app, syswk->pos, syswk->app->rangeWidth, syswk->app->rangeHeight, 0);
    Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
    Box2Obj_ShowPokeOutline(syswk, 0);
    Box2Obj_SetButtons(syswk, 2, 1, 2, 2);
    AllocPokeMoveWork(syswk);
    syswk->nextSeq = seq;
    syswk->unk1A = 2;
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMoveTouch, 38);
}

// Moving Pokémon by the keys: picks up the Pokémon under the cursor
static int Box2Seq_MovePickUpKey(Box2SysWork *syswk, u32 pos, int seq) {
    syswk->unk18 = 1;
    syswk->pos = pos;
    Box2Obj_SetIconPriority(syswk->app, BOX2_BOXLIST_POS, FALSE);
    Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
    Box2Obj_SetButtons(syswk, 2, 1, 0, 0);
    if (syswk->param->mode == 4) {
        Box2Frame_HidePartyButton(syswk->app);
        Box2Frame_HideBoxListButton(syswk->app);
        CursorMove_DisablePos(syswk->app->cursorMove, 39);
    }
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMoveGetKey, seq);
}

// Moving Pokémon by the keys in the arrangement: picks up the Pokémon under the cursor
static int Box2Seq_ArrangePickUpKey(Box2SysWork *syswk, u32 pos) {
    syswk->unk18 = 1;
    syswk->pos = pos;
    Box2Obj_SetIconPriority(syswk->app, BOX2_BOXLIST_POS, FALSE);
    Box2Obj_PutPokeOutlineAt(syswk, BOX2_BOXLIST_POS);
    Box2Obj_SetButtons(syswk, 2, 1, 0, 0);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMoveGetKey, 21);
}

// Picking a range: picks up the range by the keys
static int Box2Seq_RangePickUpKey(Box2SysWork *syswk, u32 pos, int seq) {
    syswk->pos = pos;
    Box2Obj_SetIconPriority(syswk->app, BOX2_BOXLIST_POS, FALSE);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMoveGetKey, seq);
}

// Moving Pokémon by the keys: drops the Pokémon, unless the box picked in the list is this one or full
static int Box2Seq_MoveDropKey(Box2SysWork *syswk, u32 pos, int seq) {
    if (pos >= BOX2_PARTY_POS) {
        // A box of the list
        if (pos >= BOX2_BOXLIST_POS && pos != BOX2_GET_NONE) {
            u32 tray = syswk->trayScroll + pos - BOX2_BOXLIST_POS;

            if (tray >= syswk->trayMax) {
                tray -= syswk->trayMax;
            }
            if (tray == syswk->tray) {
                GFL_SndSEPlay(SEQ_SE_BEEP);
                return 21;
            }
            if (countEmptySlotsInBox(syswk->param->boxes, tray) == 0) {
                GFL_SndSEPlay(SEQ_SE_BEEP);
                return 21;
            }
        }
    }
    syswk->app->pokePutKey = pos;
    syswk->unk1A = 2;
    syswk->nextSeq = seq;
    AllocPokeMoveWork(syswk);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMovePutKey, 23);
}

// Picking a range: drops the range, unless the box picked in the list is this one or has too little room
static int Box2Seq_RangeDropKey(Box2SysWork *syswk, u32 pos, int seq) {
    if (pos >= BOX2_PARTY_POS) {
        // A box of the list
        if (pos >= BOX2_BOXLIST_POS && pos != BOX2_GET_NONE) {
            u32 tray = syswk->trayScroll + pos - BOX2_BOXLIST_POS;

            if (tray >= syswk->trayMax) {
                tray -= syswk->trayMax;
            }
            if (tray == syswk->tray) {
                GFL_SndSEPlay(SEQ_SE_BEEP);
                return 36;
            }
            if (countEmptySlotsInBox(syswk->param->boxes, tray) < Box2Main_GetRangeCount(syswk->app)) {
                GFL_SndSEPlay(SEQ_SE_BEEP);
                return 36;
            }
        }
    }
    Box2Obj_SetRangeAnm(syswk, 0);
    syswk->app->pokePutKey = pos;
    syswk->unk1A = 2;
    syswk->nextSeq = seq;
    AllocPokeMoveWork(syswk);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncPokeMovePutKey, 38);
}

// Allocates the work of a Pokémon move
static void AllocPokeMoveWork(Box2SysWork *syswk) {
    syswk->app->vfunk.work = GFL_HeapAllocate(HEAPID_BOX2_APP, sizeof(Box2PokeMoveWork), TRUE, "box2_seq.c", 10826);
}

static void FreePokeMoveWork(Box2SysWork *syswk) {
    GFL_HeapFree(syswk->app->vfunk.work);
}

// The item arrangement: takes the item of a touched Pokémon
static int Box2Seq_ItemTakeTouched(Box2SysWork *syswk, u32 pos) {
    s16 x, y;

    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    syswk->app->unkA552 = 1;
    syswk->app->getItem = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);
    Box2Frame_HideMenu(syswk->app->bgWinFrame);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    if (syswk->app->getItem != 0) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        Box2Obj_PutItemIconAtIcon(syswk->app, syswk->pos, 0);
        Box2Obj_SetItemIcon(syswk->app, syswk->app->getItem);
        Box2Obj_InitItemOutline(syswk->app);
        Box2Obj_SetButtons(syswk, 2, 2, 1, 1);
    } else if (Box2Obj_IsVisible(syswk->app, BOX2_ACTOR_ITEM_ICON) == TRUE) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        Box2Obj_ShowPokeOutline(syswk, 0);
    }
    Box2Obj_GetIconPos(syswk->pos, &x, &y, 0);
    syswk->app->tpx = x + 8;
    syswk->app->tpy = y + 8;
    AllocItemMoveWork(syswk);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeGetTouch, Box2Seq_PokeMoveEndTo(syswk, 72));
}

// The item arrangement: opens the menu of a picked Pokémon
static int Box2Seq_ItemOpenMenu(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    syswk->app->unkA552 = 1;
    Box2Main_PokeInfoPut(syswk, syswk->pos);
    syswk->app->getItem = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);
    Box2Seq_OpenItemMenu(syswk, (s16)syswk->app->getItem);
    Box2Frame_SlideMenuIn(syswk->app->bgWinFrame);
    if (syswk->app->getItem != 0) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        Box2Obj_PutItemIconAtIcon(syswk->app, syswk->pos, 0);
        Box2Obj_SetItemIcon(syswk->app, syswk->app->getItem);
        Box2Obj_InitItemOutline(syswk->app);
        Box2Ui_StartCursorMove(syswk->app, 36, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, 36);
        syswk->app->oldCurPos = 36;
    } else {
        Box2Ui_StartCursorMove(syswk->app, 37, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, 37);
        syswk->app->oldCurPos = 37;
    }
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeMenuClose, Box2Seq_PokeMoveEndTo(syswk, 67));
}

// The item arrangement with the party out: takes the item of a touched Pokémon
static int Box2Seq_ItemPartyOutTakeTouched(Box2SysWork *syswk, u32 pos) {
    s16 x, y;

    syswk->pos = pos;
    if (pos < BOX2_PARTY_POS) {
        syswk->getTray = syswk->tray;
    } else {
        syswk->getTray = BOX2_GET_NONE;
    }
    syswk->app->getItem = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);
    if (syswk->app->getItem == 0) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
        return 74;
    }
    GFL_SndSEPlay(SEQ_SE_SYS_39);
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
    Box2Obj_PutItemIconAtIcon(syswk->app, syswk->pos, 2);
    Box2Obj_SetItemIcon(syswk->app, syswk->app->getItem);
    Box2Obj_InitItemOutline(syswk->app);
    Box2Obj_SetButtons(syswk, 2, 1, 1, 1);
    Box2Obj_GetIconPos(syswk->pos, &x, &y, 2);
    syswk->app->tpx = x + 8;
    syswk->app->tpy = y + 8;
    AllocItemMoveWork(syswk);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangePartyGetTouch, 76);
}

// The item arrangement with the party out: takes the item of the Pokémon under the cursor
static int Box2Seq_ItemPartyOutTakeKey(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    syswk->app->getItemInitPos = pos;
    if (pos < BOX2_PARTY_POS) {
        syswk->getTray = syswk->tray;
    } else {
        syswk->getTray = BOX2_GET_NONE;
    }
    syswk->app->getItem = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);
    if (syswk->app->getItem == 0) {
        return 74;
    }
    syswk->unk18 = 1;
    Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
    Box2Obj_PutItemIconAtCursor(syswk->app, syswk->pos, 2);
    Box2Obj_SetItemIcon(syswk->app, syswk->app->getItem);
    Box2Obj_SetButtons(syswk, 2, 1, 1, 1);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeGetKey, 74);
}

// The item arrangement: puts a held item back by the keys
static int Box2Seq_ItemPutBackKey(Box2SysWork *syswk) {
    u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

    if (pos < BOX2_BOXLIST_POS) {
        Box2Main_PokeInfoPut(syswk, pos);
    } else {
        Box2Main_PokeInfoOff(syswk);
    }
    AllocItemMoveWork(syswk);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeKeyCancel, 77);
}

// The item arrangement: drops a held item by the keys
static int Box2Seq_ItemDropHeldKey(Box2SysWork *syswk, u32 pos) {
    s16 x, y;

    syswk->app->pokePutKey = pos;
    syswk->unk18 = 0;
    Box2Obj_GetPos(syswk->app, BOX2_ACTOR_CURSOR, &x, &y, 0);
    Box2Obj_SetPos(syswk->app, BOX2_ACTOR_ITEM_ICON, x, y + 8, 0);
    Box2Obj_ShowPokeOutline(syswk, 0);
    if (syswk->app->pokePutKey == BOX2_GET_NONE) {
        return Box2Seq_ItemPutBackKey(syswk);
    }
    if (Box2Main_PokeItemMoveCheck(syswk, syswk->pos, pos) == FALSE) {
        return Box2Seq_ItemPutBackKey(syswk);
    }
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangePutKey, 79);
}

// The item arrangement in the party: takes the item of a touched Pokémon
static int Box2Seq_ItemPartyTakeTouched(Box2SysWork *syswk, u32 pos) {
    s16 x, y;

    syswk->pos = pos;
    syswk->getTray = BOX2_GET_NONE;
    syswk->app->getItem = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);
    Box2Frame_HideMenu(syswk->app->bgWinFrame);
    Box2Bmp_ClearMsgWindow(syswk->app, 24);
    Box2Main_PutMessageFrame(syswk);
    if (syswk->app->getItem != 0) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        Box2Obj_PutItemIconAtIcon(syswk->app, syswk->pos, 1);
        Box2Obj_SetItemIcon(syswk->app, syswk->app->getItem);
        Box2Obj_InitItemOutline(syswk->app);
        Box2Obj_SetButtons(syswk, 2, 2, 1, 1);
    } else if (Box2Obj_IsVisible(syswk->app, BOX2_ACTOR_ITEM_ICON) == TRUE) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        Box2Obj_ShowPokeOutline(syswk, 0);
    }
    Box2Obj_GetIconPos(syswk->pos, &x, &y, 1);
    syswk->app->tpx = x + 8;
    syswk->app->tpy = y + 8;
    AllocItemMoveWork(syswk);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeBoxPartyGetTouch, 82);
}

// The item arrangement in the party: opens the menu of a picked Pokémon
static int Box2Seq_ItemPartyOpenMenu(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    syswk->app->unkA552 = 1;
    Box2Main_PokeInfoPut(syswk, syswk->pos);
    syswk->app->getItem = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);
    Box2Seq_OpenItemMenu(syswk, (s16)syswk->app->getItem);
    Box2Frame_SlideMenuIn(syswk->app->bgWinFrame);
    if (syswk->app->getItem != 0) {
        Box2Obj_SetVisible(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        Box2Obj_PutItemIconAtIcon(syswk->app, syswk->pos, 1);
        Box2Obj_SetItemIcon(syswk->app, syswk->app->getItem);
        Box2Obj_InitItemOutline(syswk->app);
        Box2Ui_StartCursorMove(syswk->app, 9, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, 9);
        syswk->app->oldCurPos = 9;
    } else {
        Box2Ui_StartCursorMove(syswk->app, 10, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, 10);
        syswk->app->oldCurPos = 10;
    }
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncItemArrangeMenuClose, Box2Seq_PokeMoveEndTo(syswk, 80));
}

// Allocates the work of an item icon's move
static void AllocItemMoveWork(Box2SysWork *syswk) {
    syswk->app->vfunk.work = GFL_HeapAllocate(HEAPID_BOX2_APP, sizeof(Box2ItemMoveWork), FALSE, "box2_seq.c", 11181);
}

static void FreeItemMoveWork(Box2SysWork *syswk) {
    GFL_HeapFree(syswk->app->vfunk.work);
}

static void PutPartyLeft(Box2SysWork *syswk) {
    Box2Frame_PutPartyLeft(syswk->app->bgWinFrame);
    Box2Obj_PutPartyIconsLeft(syswk);
}

static void PutPartyRight(Box2SysWork *syswk) {
    Box2Frame_PutPartyRight(syswk->app->bgWinFrame);
    Box2Obj_PutPartyIconsRight(syswk);
}

void Box2Seq_PutPartyLeft(Box2SysWork *syswk) {
    Box2Obj_PutPartyIconsLeft(syswk);
    Box2Frame_PutPartyLeft(syswk->app->bgWinFrame);
}

// Scrolls to the box picked in the list
static int Box2Seq_ScrollToPickedBox(Box2SysWork *syswk, int seq) {
    BOOL right = Box2Main_IsTrayScrollRight(syswk, syswk->tray, syswk->app->unkA55F);
    Box2VFunc func;

    syswk->tray = syswk->app->unkA55F;
    Box2Obj_LoadTrayPokeIcons(syswk, syswk->tray);
    Box2Main_WallPaperSet(syswk, Box2Main_GetWallPaperNumber(syswk, syswk->tray), right);
    if (right == TRUE) {
        func = Box2Main_VFuncTrayScrollRight;
    } else {
        func = Box2Main_VFuncTrayScrollLeft;
    }
    syswk->nextSeq = seq;
    GFL_SndSEPlay(SEQ_SE_SELECT1);
    return Box2Seq_RunVFunc(syswk, func, syswk->nextSeq);
}

// Allocates the work of a drag of the box list
static void AllocBoxListDragWork(Box2SysWork *syswk) {
    Box2BoxListDrag *work =
        GFL_HeapAllocate(HEAPID_TAIL(HEAPID_BOX2_APP), sizeof(Box2BoxListDrag), FALSE, "box2_seq.c", 11290);

    work->cnt = 0;
    work->dir = 0;
    syswk->app->subWork = work;
}

// Moves the party's frame in or out, then goes to seq
static int Box2Seq_MovePartyFrame(Box2SysWork *syswk, int seq) {
    Box2Main_OpenBoxList(syswk);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListSlideOut, seq);
}

// Moves the party's frame, then goes to seq
static int Box2Seq_MovePartyFrameTo(Box2SysWork *syswk, int seq) {
    Box2Frame_SlideBoxListOut(syswk->app->bgWinFrame);
    return Box2Seq_RunVFunc(syswk, Box2Main_VFuncBoxListSlideIn, seq);
}

// Opens the item arrangement's menu, with or without a held item
void Box2Seq_OpenItemMenu(Box2SysWork *syswk, s16 item) {
    if (item != 0) {
        if (syswk->pos < BOX2_PARTY_POS) {
            CursorMove_EnablePos(syswk->app->cursorMove, 36);
        } else {
            CursorMove_EnablePos(syswk->app->cursorMove, 9);
        }
        Box2Bmp_OpenMenu(syswk, sMenu70dc, 3);
    } else {
        if (syswk->pos < BOX2_PARTY_POS) {
            CursorMove_DisablePos(syswk->app->cursorMove, 36);
        } else {
            CursorMove_DisablePos(syswk->app->cursorMove, 9);
        }
        Box2Bmp_OpenMenu(syswk, sMenu70d4, 2);
    }
}

// Mode 0: whether the party Pokémon can be deposited: 0 if so, 1 for the last battler, 2 for mail
static u32 GetDepositBlock(Box2SysWork *syswk) {
    // The party position, then the result
    u32 val = syswk->pos - BOX2_PARTY_POS;

    if (Box2Main_BattlePokeCheck(syswk, val) == FALSE) {
        return 1;
    }
    if (PML_ItemIsMail(PokeParty_GetParam(PokeParty_GetPkm(syswk->param->party, val), PKM_PARAM_ITEM, NULL)) == TRUE) {
        val = 2;
    } else {
        val = 0;
    }
    return val;
}

// Toggles a mark
static void ToggleMark(Box2SysWork *syswk, u32 mark) {
    syswk->app->unkA554 ^= 1 << mark;
    Box2Main_MarkingPutMain(syswk, syswk->app->unkA554);
}

// Picks a wallpaper of the theme's menu
static int Box2Seq_PickWallpaper(Box2SysWork *syswk, u32 pos) {
    if (syswk->app->unkA55C <= 4 ||
        (syswk->app->unkA55C == 5 && BoxSaveAccessor_CheckFlags(syswk->param->boxes, 2) == FALSE)) {
        syswk->app->wallpaperPos = pos + (syswk->app->unkA55C - 1) * 4 - 1;
    } else {
        syswk->app->wallpaperPos = pos + (syswk->app->unkA55C - 1) * 4;
    }
    return Box2Seq_AnimFrameButton(syswk, pos, Box2Seq_Goto(syswk, 114));
}

// Dims the actors behind the box's name entry
static void DimActorsForNameInput(Box2SysWork *syswk) {
    switch (syswk->param->mode) {
    case 0:
        Box2Obj_SetIconsBlend(syswk, 2, 1);
        break;
    case 1:
    case 3:
        Box2Obj_ShowScrollArrows(syswk->app, 0);
        Box2Obj_SetIconsBlend(syswk, 1, 1);
        break;
    case 2:
    case 4:
    case 5:
        Box2Obj_ShowScrollArrows(syswk->app, 0);
        if (Box2Frame_IsPartyAtLeft(syswk->app->bgWinFrame) == TRUE) {
            Box2Obj_SetIconsBlend(syswk, 2, 1);
        } else {
            Box2Obj_SetIconsBlend(syswk, 1, 1);
        }
        break;
    }
}

// Switches to the next way of moving Pokémon, by Select
static int Box2Seq_NextMoveMode(Box2SysWork *syswk, int seq) {
    GFL_SndSEPlay(SEQ_SE_DECIDE1);
    syswk->unk1C_4 = 0;
    if (syswk->moveMode == 2) {
        syswk->moveMode = 0;
    } else {
        syswk->moveMode++;
        if (syswk->moveMode == 2 && syswk->param->mode == 3) {
            syswk->moveMode = 0;
        }
    }
    if (syswk->moveMode == 1 && Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
        syswk->unk1C_4 = 1;
    }
    syswk->unk1C_6 = 0;
    Box2Main_ShowCursor(syswk);
    return Box2Seq_Goto(syswk, seq);
}

// Sets the way of moving Pokémon picked in the menu
static int Box2Seq_SetMoveMode(Box2SysWork *syswk, int seq) {
    GFL_SndSEPlay(SEQ_SE_DECIDE1);
    syswk->unk1C_4 = 0;
    if (syswk->moveMode == 1 && Box2Frame_IsMenuOnScreen(syswk->app->bgWinFrame) == TRUE) {
        syswk->unk1C_4 = 1;
    }
    syswk->unk1C_6 = 0;
    return Box2Seq_Goto(syswk, seq);
}

// Cancels a range pick
static void CancelRange(Box2SysWork *syswk) {
    Box2Obj_SetRangeAnm(syswk, 0);
    syswk->pos = BOX2_GET_NONE;
    syswk->unk18 = 0;
    Box2Ui_SetButtonStates(syswk, CursorMove_GetPos(syswk->app->cursorMove));
}

// Puts a range's ends in order
static void SortRangeEnds(Box2SysWork *syswk) {
    if (syswk->app->rangeSelect.startPos > syswk->app->rangeSelect.endPos) {
        u8 end = syswk->app->rangeSelect.endPos;

        syswk->app->rangeSelect.endPos = syswk->app->rangeSelect.startPos;
        syswk->app->rangeSelect.startPos = end;
    }
}
