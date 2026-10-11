#include "types.h"
#include "app/box2_seq.h"
#include "app/box2_bgwfrm.h"
#include "app/box2_bmp.h"
#include "app/box2_main.h"
#include "app/box2_obj.h"
#include "app/box2_ui.h"
#include "app/ui/ui_scene.h"
#include "constants/arc.h"
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
#include "constants/pokemon.h"
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
static int func_ov255_021c2d6c(Box2SysWork *syswk);
static int func_ov255_021c2dc0(Box2SysWork *syswk);
static int func_ov255_021c2f78(Box2SysWork *syswk);
static int func_ov255_021c30f0(Box2SysWork *syswk);
static int func_ov255_021c30f8(Box2SysWork *syswk);
static int func_ov255_021c366c(Box2SysWork *syswk);
static int func_ov255_021c36c0(Box2SysWork *syswk);
static int func_ov255_021c3700(Box2SysWork *syswk);
static int func_ov255_021c385c(Box2SysWork *syswk);
static int func_ov255_021c3ec8(Box2SysWork *syswk);
static int func_ov255_021c3fc0(Box2SysWork *syswk);
static int func_ov255_021c41e8(Box2SysWork *syswk);
static int func_ov255_021c4258(Box2SysWork *syswk);
static int func_ov255_021c4328(Box2SysWork *syswk);
static int func_ov255_021c445c(Box2SysWork *syswk);
static int func_ov255_021c4da4(Box2SysWork *syswk);
static int func_ov255_021c4ea0(Box2SysWork *syswk);
static int func_ov255_021c50d0(Box2SysWork *syswk);
static int func_ov255_021c5140(Box2SysWork *syswk);
static int func_ov255_021c522c(Box2SysWork *syswk);
static int func_ov255_021c53f4(Box2SysWork *syswk);
static int func_ov255_021c5d9c(Box2SysWork *syswk);
static int func_ov255_021c5ef4(Box2SysWork *syswk);
static int func_ov255_021c5f94(Box2SysWork *syswk);
static int func_ov255_021c6058(Box2SysWork *syswk);
static int func_ov255_021c64fc(Box2SysWork *syswk);
static int func_ov255_021c65a4(Box2SysWork *syswk);
static int func_ov255_021c65e4(Box2SysWork *syswk);
static int func_ov255_021c6644(Box2SysWork *syswk);
static int func_ov255_021c686c(Box2SysWork *syswk);
static int func_ov255_021c6de0(Box2SysWork *syswk);
static int func_ov255_021c6f34(Box2SysWork *syswk);
static int func_ov255_021c6fd4(Box2SysWork *syswk);
static int func_ov255_021c74fc(Box2SysWork *syswk);
static int func_ov255_021c7554(Box2SysWork *syswk);
static int func_ov255_021c7594(Box2SysWork *syswk);
static int func_ov255_021c762c(Box2SysWork *syswk);
static int func_ov255_021c7a88(Box2SysWork *syswk);
static int func_ov255_021c7b0c(Box2SysWork *syswk);
static int func_ov255_021c7b4c(Box2SysWork *syswk);
static int func_ov255_021c7b98(Box2SysWork *syswk);
static int func_ov255_021c7c10(Box2SysWork *syswk);
static int func_ov255_021c8024(Box2SysWork *syswk);
static int func_ov255_021c8060(Box2SysWork *syswk);
static int func_ov255_021c80ec(Box2SysWork *syswk);
static int func_ov255_021c81d8(Box2SysWork *syswk);
static int func_ov255_021c8238(Box2SysWork *syswk);
static int func_ov255_021c8594(Box2SysWork *syswk);
static int func_ov255_021c85d0(Box2SysWork *syswk);
static int func_ov255_021c8608(Box2SysWork *syswk);
static int func_ov255_021c8798(Box2SysWork *syswk);
static int func_ov255_021c8920(Box2SysWork *syswk);
static int func_ov255_021c8a9c(Box2SysWork *syswk);
static int func_ov255_021c8ad0(Box2SysWork *syswk);
static int func_ov255_021c8b38(Box2SysWork *syswk);
static int func_ov255_021c8fc4(Box2SysWork *syswk);
static int func_ov255_021c9004(Box2SysWork *syswk);
static int func_ov255_021c907c(Box2SysWork *syswk);
static int func_ov255_021c90b8(Box2SysWork *syswk);
static int func_ov255_021c9120(Box2SysWork *syswk);
static int func_ov255_021c9160(Box2SysWork *syswk);
static int func_ov255_021c9338(Box2SysWork *syswk);
static int func_ov255_021c96b8(Box2SysWork *syswk);
static int func_ov255_021c97c8(Box2SysWork *syswk);
static int func_ov255_021c99a8(Box2SysWork *syswk);
static int func_ov255_021c9a44(Box2SysWork *syswk);
static int func_ov255_021c9a70(Box2SysWork *syswk);
static int func_ov255_021c9c34(Box2SysWork *syswk);
static int func_ov255_021c9ffc(Box2SysWork *syswk);
static int func_ov255_021ca03c(Box2SysWork *syswk);
static int func_ov255_021ca194(Box2SysWork *syswk);
static int func_ov255_021ca314(Box2SysWork *syswk);
static int func_ov255_021ca3b4(Box2SysWork *syswk);
static int func_ov255_021ca6c8(Box2SysWork *syswk);
static int func_ov255_021ca6d8(Box2SysWork *syswk);
static int func_ov255_021ca79c(Box2SysWork *syswk);
static int func_ov255_021ca7d8(Box2SysWork *syswk);
static int func_ov255_021ca7e4(Box2SysWork *syswk);
static int func_ov255_021ca9a4(Box2SysWork *syswk);
static int func_ov255_021ca9b0(Box2SysWork *syswk);
static int func_ov255_021caa30(Box2SysWork *syswk);
static int func_ov255_021caadc(Box2SysWork *syswk);
static int func_ov255_021cab14(Box2SysWork *syswk);
static int func_ov255_021cab94(Box2SysWork *syswk);
static int func_ov255_021cabbc(Box2SysWork *syswk);
static int func_ov255_021cacac(Box2SysWork *syswk);
static int func_ov255_021cadcc(Box2SysWork *syswk);
static int func_ov255_021cae74(Box2SysWork *syswk);
static int func_ov255_021cae84(Box2SysWork *syswk);
static int func_ov255_021cb020(Box2SysWork *syswk);
static int func_ov255_021cb068(Box2SysWork *syswk);
static int func_ov255_021cb1d8(Box2SysWork *syswk);
static int func_ov255_021cb258(Box2SysWork *syswk);
static int func_ov255_021cb3a0(Box2SysWork *syswk);
static int func_ov255_021cb488(Box2SysWork *syswk);
static int func_ov255_021cb5b0(Box2SysWork *syswk);
static int func_ov255_021cb67c(Box2SysWork *syswk);
static int func_ov255_021cb82c(Box2SysWork *syswk);
static int func_ov255_021cb960(Box2SysWork *syswk);
static int func_ov255_021cb97c(Box2SysWork *syswk);
static int func_ov255_021cbb00(Box2SysWork *syswk);
static int func_ov255_021cbd2c(Box2SysWork *syswk);
static int func_ov255_021cbe1c(Box2SysWork *syswk);
static int func_ov255_021cbe28(Box2SysWork *syswk);
static int func_ov255_021cbf18(Box2SysWork *syswk);
static int func_ov255_021cbfe8(Box2SysWork *syswk);
static int func_ov255_021cc040(Box2SysWork *syswk);
static int func_ov255_021cc0b4(Box2SysWork *syswk);
static int func_ov255_021cc198(Box2SysWork *syswk);
static int func_ov255_021cc1bc(Box2SysWork *syswk);
static int func_ov255_021cc308(Box2SysWork *syswk);
static int func_ov255_021cc330(Box2SysWork *syswk);
static int func_ov255_021cc380(Box2SysWork *syswk);


static int func_ov255_021cbe58(Box2SysWork *syswk, int seq);
static int func_ov255_021cbe98(Box2SysWork *syswk, int nextSeq);
static int func_ov255_021cbed8(Box2SysWork *syswk, int seq);
static int func_ov255_021cbee8(Box2SysWork *syswk, int seq);
static int func_ov255_021cbef0(Box2SysWork *syswk, u32 type);
static int func_ov255_021cc3b0(Box2SysWork *syswk, u32 frame, int seq);
static int func_ov255_021cc460(Box2SysWork *syswk, u32 id, u32 pal, int seq);
static int func_ov255_021cc3c0(Box2SysWork *syswk, u32 button, int seq);
static int func_ov255_021cc4e0(Box2SysWork *syswk, u32 type);
static int func_ov255_021cc50c(Box2SysWork *syswk);
static int func_ov255_021cc608(Box2SysWork *syswk);
static int func_ov255_021cc768(Box2SysWork *syswk);
static int func_ov255_021cc864(Box2SysWork *syswk);
static int func_ov255_021cc894(Box2SysWork *syswk);
static int func_ov255_021cc8dc(Box2SysWork *syswk);
static int func_ov255_021ccae4(Box2SysWork *syswk);
static int func_ov255_021ccc28(Box2SysWork *syswk);
static int func_ov255_021cce14(Box2SysWork *syswk);
static int func_ov255_021ccf1c(Box2SysWork *syswk, u32 frameOut, int seq);
static int func_ov255_021ccf68(Box2SysWork *syswk, u32 frameOut, int seq);
static int func_ov255_021ccfb4(Box2SysWork *syswk, u32 pos);
static int func_ov255_021cd16c(Box2SysWork *syswk, u32 pos);
static int func_ov255_021cd1f0(Box2SysWork *syswk, u32 pos);
static int func_ov255_021cd268(Box2SysWork *syswk, u32 pos);
static int func_ov255_021cd2e8(Box2SysWork *syswk, u32 pos, int seq);
static int func_ov255_021cd02c(Box2SysWork *syswk, u32 pos);
static int func_ov255_021cd098(Box2SysWork *syswk, u32 pos);
static int func_ov255_021cd128(Box2SysWork *syswk, u32 pos, int seq);
static int func_ov255_021cd458(Box2SysWork *syswk, u32 pos);
static void func_ov255_021cd5b0(Box2SysWork *syswk);
static void func_ov255_021cd5d8(Box2SysWork *syswk);
static int func_ov255_021cd5e4(Box2SysWork *syswk, u32 pos);
static int func_ov255_021cd6cc(Box2SysWork *syswk, u32 pos);
static int func_ov255_021cd798(Box2SysWork *syswk, u32 pos);
static int func_ov255_021cd858(Box2SysWork *syswk, u32 pos);
static int func_ov255_021cd910(Box2SysWork *syswk, u32 pos);
static int func_ov255_021cd994(Box2SysWork *syswk, u32 pos);
static int func_ov255_021cda6c(Box2SysWork *syswk, u32 pos);
static int func_ov255_021cdc38(Box2SysWork *syswk, int seq);
static int func_ov255_021cddf0(Box2SysWork *syswk, int seq);
static int func_ov255_021cde88(Box2SysWork *syswk, int seq);

static int func_ov255_021cd32c(Box2SysWork *syswk, u32 pos, int seq);
static int func_ov255_021cd4b8(Box2SysWork *syswk, u32 pos, int seq);
static int func_ov255_021cdba4(Box2SysWork *syswk, int seq);
static void func_ov255_021cdc04(Box2SysWork *syswk);
static int func_ov255_021cdc54(Box2SysWork *syswk, int seq);
static int func_ov255_021cd390(Box2SysWork *syswk, int seq);
static int func_ov255_021cd3f8(Box2SysWork *syswk, u32 pos, int seq);
static int func_ov255_021cd494(Box2SysWork *syswk, u32 pos, int seq);
static int func_ov255_021cd52c(Box2SysWork *syswk, u32 pos, int seq);
static u32 func_ov255_021cdcc8(Box2SysWork *syswk);
static void func_ov255_021cdb34(Box2SysWork *syswk);
static void func_ov255_021cdb5c(Box2SysWork *syswk);
static void func_ov255_021cdb68(Box2SysWork *syswk);
static void func_ov255_021cdb7c(Box2SysWork *syswk);
static void func_ov255_021cdd04(Box2SysWork *syswk, u32 mark);
static int func_ov255_021cdd24(Box2SysWork *syswk, u32 pos);
static void func_ov255_021cdd80(Box2SysWork *syswk);
static void func_ov255_021cded4(Box2SysWork *syswk);
static void func_ov255_021cdef8(Box2SysWork *syswk);

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
    { Box2Main_NameInCall, Box2Main_NameInExit, 93 },
    { Box2Main_BoxSearchCall, Box2Main_BoxSearchExit, 95 },
};

static const Box2SeqFunc sMainSeq[] = {
    Box2Seq_Init, Box2Seq_Release, Box2Seq_Wipe,
    Box2Seq_PaletteFade, Box2Seq_Wait, Box2Seq_VFunc,
    Box2Seq_TrgWait, Box2Seq_YesNo, Box2Seq_ButtonAnm,
    Box2Seq_SubProcCall, Box2Seq_SubProcMain, Box2Seq_Start,
    Box2Seq_StartWait, func_ov255_021c2d6c, func_ov255_021c2dc0,
    func_ov255_021c2f78, func_ov255_021c30f0, func_ov255_021c30f8,
    func_ov255_021c366c, func_ov255_021c36c0, func_ov255_021c3700,
    func_ov255_021c385c, func_ov255_021c3ec8, func_ov255_021c3fc0,
    func_ov255_021c41e8, func_ov255_021c4258, func_ov255_021c5f94,
    func_ov255_021c6058, func_ov255_021c64fc, func_ov255_021c65a4,
    func_ov255_021c65e4, func_ov255_021c6644, func_ov255_021c686c,
    func_ov255_021c6de0, func_ov255_021c6f34, func_ov255_021c4328,
    func_ov255_021c445c, func_ov255_021c4da4, func_ov255_021c4ea0,
    func_ov255_021c50d0, func_ov255_021c5140, func_ov255_021c522c,
    func_ov255_021c53f4, func_ov255_021c5d9c, func_ov255_021c5ef4,
    func_ov255_021c6fd4, func_ov255_021c74fc, func_ov255_021c7554,
    func_ov255_021c7594, func_ov255_021c762c, func_ov255_021c7a88,
    func_ov255_021c7b0c, func_ov255_021c7b4c, func_ov255_021c7b98,
    func_ov255_021c7c10, func_ov255_021c8024, func_ov255_021c8060,
    func_ov255_021c80ec, func_ov255_021c81d8, func_ov255_021c8238,
    func_ov255_021c8594, func_ov255_021c85d0, func_ov255_021c8608,
    func_ov255_021c8798, func_ov255_021c8920, func_ov255_021c8a9c,
    func_ov255_021c8ad0, func_ov255_021c8b38, func_ov255_021c8fc4,
    func_ov255_021c9004, func_ov255_021c907c, func_ov255_021c90b8,
    func_ov255_021c9120, func_ov255_021c9160, func_ov255_021c9338,
    func_ov255_021c96b8, func_ov255_021c97c8, func_ov255_021c99a8,
    func_ov255_021c9a44, func_ov255_021c9a70, func_ov255_021c9c34,
    func_ov255_021c9ffc, func_ov255_021ca03c, func_ov255_021ca194,
    func_ov255_021ca314, func_ov255_021ca3b4, func_ov255_021ca6c8,
    func_ov255_021ca6d8, func_ov255_021ca79c, func_ov255_021ca7d8,
    func_ov255_021ca7e4, func_ov255_021ca9a4, func_ov255_021ca9b0,
    func_ov255_021caa30, func_ov255_021caadc, func_ov255_021cab14,
    func_ov255_021cab94, func_ov255_021cabbc, func_ov255_021cacac,
    func_ov255_021cadcc, func_ov255_021cae74, func_ov255_021cae84,
    func_ov255_021cb020, func_ov255_021cb068, func_ov255_021cb1d8,
    func_ov255_021cb258, func_ov255_021cb3a0, func_ov255_021cb488,
    func_ov255_021cb5b0, func_ov255_021cb67c, func_ov255_021cb82c,
    func_ov255_021cb960, func_ov255_021cb97c, func_ov255_021cbb00,
    func_ov255_021cbd2c, func_ov255_021cbe1c, func_ov255_021cbe28,
};

static const Box2YesNoFuncs sYesNoFuncs[] = {
    { func_ov255_021cbf18, func_ov255_021cc380 },
    { func_ov255_021cbfe8, func_ov255_021cc040 },
    { func_ov255_021cc0b4, func_ov255_021cc380 },
    { func_ov255_021cc198, func_ov255_021cc1bc },
    { func_ov255_021cc1bc, func_ov255_021cc198 },
    { func_ov255_021cc308, func_ov255_021cc330 },
};

BOOL Box2Seq_Main(Box2SysWork *syswk, u32 *seq) {
    if (syswk->app == NULL || syswk->app->printQueue == NULL || func_02021c0c(syswk->app->printQueue) != FALSE) {
        if (syswk->app != NULL) {
            func_ov255_021ced6c(syswk);
        }
        *seq = sMainSeq[*seq](syswk);
    }
    if (*seq == BOX2SEQ_END) {
        return FALSE;
    }
    if (syswk->app != NULL) {
        func_ov255_021cdfe8(syswk->app);
        func_ov255_021cf5b0(syswk->app);
        func_ov255_021d1e38(syswk);
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
    func_ov255_021cdf18(syswk);
    func_ov255_021cf3c0(syswk);
    func_ov255_021cfc74(syswk);
    func_ov255_021cfc90(syswk);
    Box2Main_WallPaperSet(syswk, Box2Main_GetWallPaperNumber(syswk, syswk->tray), BOX2_TRAY_SCROLL_NONE);
    func_ov255_021d364c(syswk);
    func_ov255_021ce140(syswk);
    func_ov255_021ce198(syswk);
    func_ov255_021d15dc(syswk);
    func_ov255_021d23d8(syswk);
    Box2Main_InitYesNo(syswk);
    Box2Main_SetBlendAlpha(TRUE);
    func_02042ba8(TRUE, HEAPID_BOX2_APP);
    Box2Main_InitVBlank(syswk);
    return syswk->nextSeq;
}

static int Box2Seq_Release(Box2SysWork *syswk) {
    Box2Main_ExitVBlank(syswk);
    Box2Main_ExitYesNo(syswk);
    func_ov255_021d24e0(syswk);
    func_ov255_021d36e8(syswk->app);
    func_ov255_021cf414(syswk->app);
    func_ov255_021cdf5c(syswk);
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
    if (func_0203da48() == TRUE) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        func_0203d564(TRUE);
        return syswk->nextSeq;
    }
    if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        func_0203d564(FALSE);
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
        func_ov255_021cdb90(syswk);
        func_ov255_021d1348(syswk->app, 0);
        func_ov255_021d0310(syswk, 1, 1);
        Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
        syswk->app->msgNextSeq = 59;
        break;
    case 1:
        Box2Main_PokeInfoPut(syswk, 0);
        syswk->app->msgNextSeq = 54;
        break;
    case 2:
        Box2Main_PokeInfoPut(syswk, 0);
        func_ov255_021d3a48(syswk->app);
        syswk->app->msgNextSeq = 17;
        break;
    case 3:
        func_ov255_021d0310(syswk, 0x81, 1);
        Box2Main_PokeInfoPut(syswk, 0);
        func_ov255_021d3a48(syswk->app);
        syswk->app->msgNextSeq = 67;
        break;
    case 4:
        if (PokeParty_GetPkmCount(syswk->param->party) == 0) {
            func_ov255_021d3a48(syswk->app);
            Box2Main_PokeInfoPut(syswk, 0);
            func_ov255_021d2478(syswk, 13, 0);
            syswk->app->msgNextSeq = 45;
        } else {
            func_ov255_021cdb90(syswk);
            func_ov255_021d1348(syswk->app, 0);
            func_ov255_021d0310(syswk, 1, 1);
            func_ov255_021d3a64(syswk->app);
            Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
            func_ov255_021d2478(syswk, 14, 0);
            syswk->app->msgNextSeq = 49;
        }
        break;
    case 5:
        Box2Main_PokeInfoPut(syswk, 0);
        func_ov255_021d0310(syswk, 0x81, 1);
        func_ov255_021d1af8(syswk, 0, 1, 1, 1);
        syswk->app->msgNextSeq = 85;
        break;
    }
    return Box2Seq_StartWait(syswk);
}

static int Box2Seq_StartWait(Box2SysWork *syswk) {
    if (func_02021c0c(syswk->app->printQueue) == FALSE) {
        return BOX2SEQ_START_WAIT;
    }
    return func_ov255_021cbe68(syswk, syswk->app->msgNextSeq);
}

static int func_ov255_021c2d6c(Box2SysWork *syswk) {
    GFL_HeapFree(syswk->app->vfunk.work);
    if (syswk->app->unkA552 == 1) {
        if (syswk->param->mode == 3) {
            if (syswk->app->getItem == 0) {
                func_ov255_021cf208(syswk, 0, 24);
            } else {
                func_ov255_021cf208(syswk, 1, 24);
            }
        } else {
            func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
        }
        syswk->app->unkA552 = 0;
    }
    return syswk->nextSeq;
}

static int func_ov255_021c2dc0(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        switch (syswk->param->mode) {
        case 1:
            func_ov255_021ceed0(syswk, sMenu7194, 5);
            break;
        case 0:
            func_ov255_021ceed0(syswk, sMenu7130, 5);
            break;
        case 2:
            func_ov255_021ceed0(syswk, sMenu71f0, 6);
            break;
        case 3:
            func_ov255_021cdc74(syswk, syswk->app->getItem);
            break;
        case 4:
            func_ov255_021ceed0(syswk, sMenu70f8, 4);
            break;
        case 5:
            func_ov255_021cefa4(syswk->app, 27);
            func_ov255_021ceed0(syswk, sMenu70cc, 2);
            break;
        }
        if (syswk->app->unkA550 == 1) {
            syswk->app->unkA550 = 0;
            if (syswk->param->mode == 3) {
                if (syswk->pos >= BOX2_PARTY_POS) {
                    func_ov255_021d0310(syswk, 0x82, 0);
                } else {
                    func_ov255_021d1348(syswk->app, 1);
                    func_ov255_021d0310(syswk, 0x81, 0);
                }
            } else if (syswk->pos >= BOX2_PARTY_POS) {
                func_ov255_021d0310(syswk, 2, 0);
            } else {
                func_ov255_021d1348(syswk->app, 1);
                func_ov255_021d0310(syswk, 1, 0);
            }
        }
        func_ov255_021d390c(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 14);
    case 1:
        syswk->app->subSeq = 0;
        func_ov255_021d101c(syswk, 1);
        func_ov255_021d0f88(syswk, 9, 1);
        switch (syswk->param->mode) {
        case 1:
            func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
            return 54;
        case 0:
            func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
            return 59;
        case 2:
            func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
            if (syswk->pos < BOX2_PARTY_POS) {
                return 17;
            }
            return 27;
        case 3:
            if (syswk->app->getItem == 0) {
                func_ov255_021cf208(syswk, 0, 24);
            } else {
                func_ov255_021cf208(syswk, 1, 24);
            }
            if (syswk->pos < BOX2_PARTY_POS) {
                return 67;
            }
            return 80;
        case 4:
            func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
            if (syswk->pos < BOX2_PARTY_POS) {
                return 45;
            }
            return 49;
        case 5:
            func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
            return 85;
        }
    }
    return 14;
}

// Scrolls the box list by touch
static int func_ov255_021c2f78(Box2SysWork *syswk) {
    u32 x, y;
    Box2BoxListDrag *work;
    u32 oldY;
    int diff;

    if (func_ov255_021d357c(&x, &y) == FALSE) {
        int pos;

        GFL_HeapFree(syswk->app->subWork);
        pos = func_ov255_021d35a4(syswk->app->tpx, syswk->app->tpy);
        if (pos >= 0) {
            func_ov255_021d1ac8(syswk, pos, 1);
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
            func_ov255_021bc09c(syswk, 1);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (syswk->nextSeq == 21) {
                return func_ov255_021cbec8(syswk, Box2Main_VFuncBoxListScrollLeft, 15);
            }
            return func_ov255_021cbec8(syswk, Box2Main_VFuncBoxMoveScrollLeft, 15);
        }
        if (y > oldY) {
            work->dir = -1;
            func_ov255_021bc09c(syswk, -1);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (syswk->nextSeq == 21) {
                return func_ov255_021cbec8(syswk, Box2Main_VFuncBoxListScrollRight, 15);
            }
            return func_ov255_021cbec8(syswk, Box2Main_VFuncBoxMoveScrollRight, 15);
        }
    }
    if (work->cnt != 0) {
        work->cnt--;
        if (work->dir == 1) {
            func_ov255_021bc09c(syswk, 1);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (syswk->nextSeq == 21) {
                return func_ov255_021cbec8(syswk, Box2Main_VFuncBoxListScrollLeft, 15);
            }
            return func_ov255_021cbec8(syswk, Box2Main_VFuncBoxMoveScrollLeft, 15);
        }
        if (work->dir == -1) {
            func_ov255_021bc09c(syswk, -1);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            if (syswk->nextSeq == 21) {
                return func_ov255_021cbec8(syswk, Box2Main_VFuncBoxListScrollRight, 15);
            }
            return func_ov255_021cbec8(syswk, Box2Main_VFuncBoxMoveScrollRight, 15);
        }
    }
    return 15;
}

static int func_ov255_021c30f0(Box2SysWork *syswk) {
    return func_ov255_021cc198(syswk);
}

// The arrangement's main state: waits for a touch or for the cursor to pick something
static int func_ov255_021c30f8(Box2SysWork *syswk) {
    u32 res;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        syswk->unk1B = 0;
        return func_ov255_021cddf0(syswk, 20);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) && func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE) {
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
        syswk->unk13 = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_ShowCursor(syswk);
        return func_ov255_021cc460(syswk, 30, 1, 91);
    }

    res = func_ov255_021d34d0();
    if (res != 0xffffffff) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            func_0203dac8(&syswk->app->tpx, &syswk->app->tpy);
            if (func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE) {
                func_ov255_021ceed0(syswk, sMenu71f0, 6);
            }
            Box2Main_PokeInfoPut(syswk, res);
            return func_ov255_021ccfb4(syswk, res);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        func_ov255_021d24f8(syswk, res);
        Box2Main_PokeInfoOff(syswk);
        if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d3954(syswk->app->bgWinFrame);
            func_ov255_021d11a4(syswk, 0);
            func_ov255_021cefa4(syswk->app, 24);
            func_ov255_021bc018(syswk);
            func_ov255_021d3a48(syswk->app);
            return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 17);
        }
        return 17;
    }

    res = func_ov255_021d2770(syswk);
    switch (res) {
    case 30:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return func_ov255_021cbe58(syswk, 105);
    case 31:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        syswk->app->oldCurPos = 30;
        func_ov255_021d24f8(syswk, 30);
        return func_ov255_021ccf1c(syswk, 1, 17);
    case 32:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        syswk->app->oldCurPos = 30;
        func_ov255_021d24f8(syswk, 30);
        return func_ov255_021ccf68(syswk, 1, 17);
    case 33:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc3b0(syswk, 10, func_ov255_021cbe58(syswk, 26));
    case 34:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->unk28 = 1;
        return func_ov255_021cc460(syswk, 7, 8, 16);
    case 35:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->param->unk28 = 0;
        return func_ov255_021cc460(syswk, 6, 9, 116);
    case 36:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk1B = 0;
        syswk->moveMode = 0;
        return func_ov255_021cc3b0(syswk, 0, func_ov255_021cbe58(syswk, 20));
    case 37:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk13 = 0;
        syswk->getTray = syswk->tray;
        syswk->curRcvPos = 37;
        return func_ov255_021cc3b0(syswk, 1, 89);
    case 38:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk13 = 0;
        syswk->curRcvPos = 38;
        return func_ov255_021cc3b0(syswk, 2, func_ov255_021cbe58(syswk, 90));
    case 39:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc3b0(syswk, 3, func_ov255_021cbe58(syswk, 97));
    case 40:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc3b0(syswk, 4, func_ov255_021cbe58(syswk, 101));
    case 41: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos;
        func_ov255_021d32d4(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        syswk->app->oldCurPos = pos;
        return func_ov255_021cc3b0(syswk, 5, 19);
    }
    case 42:
        break;
    case 43:
        syswk->unk1B = 0;
        syswk->moveMode = 1;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cde88(syswk, 20);
    case 44:
        syswk->unk1B = 0;
        syswk->moveMode = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cde88(syswk, 35);
    case 45:
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
        syswk->unk13 = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc460(syswk, 30, 1, 91);
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return func_ov255_021ccf1c(syswk, 1, 17);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return func_ov255_021ccf68(syswk, 1, 17);
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
        return func_ov255_021cbed8(syswk, func_ov255_021cbee8(syswk, 17));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos;

            func_ov255_021d32d4(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            syswk->app->oldCurPos = pos;
            return func_ov255_021cc3b0(syswk, 5, 19);
        }
        syswk->param->unk28 = 0;
        return func_ov255_021cc460(syswk, 6, 9, 116);
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
            func_ov255_021ceed0(syswk, sMenu71f0, 6);
            Box2Main_PokeInfoPut(syswk, res);
            func_ov255_021d32d4(syswk->app, BOX2_BOXLIST_POS, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
            syswk->app->oldCurPos = BOX2_BOXLIST_POS;
            return func_ov255_021cd128(syswk, res, 17);
        }
        break;
    }
    return 17;
}

static int func_ov255_021c366c(Box2SysWork *syswk) {
    Box2Main_PokeDataMove(syswk);
    func_ov255_021cd5d8(syswk);
    CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
    syswk->app->oldCurPos = BOX2_BOXLIST_POS;
    func_ov255_021d24f8(syswk, BOX2_BOXLIST_POS);
    func_ov255_021d101c(syswk, 1);
    func_ov255_021d1af8(syswk, 0, 0, 1, 0);
    func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
    return 17;
}

static int func_ov255_021c36c0(Box2SysWork *syswk) {
    func_ov255_021d3954(syswk->app->bgWinFrame);
    func_ov255_021d11a4(syswk, 0);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    func_ov255_021d3a48(syswk->app);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncCursorMoveFrame, func_ov255_021cbee8(syswk, 17));
}

static int func_ov255_021c3700(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        if (func_ov255_021d3b48(syswk->app->bgWinFrame) == FALSE) {
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
            func_ov255_021cefa4(syswk->app, 24);
            func_ov255_021bc018(syswk);
            func_ov255_021d3a48(syswk->app);
            func_ov255_021d0374(syswk, syswk->pos, 1, 1);
        }
        func_ov255_021d101c(syswk, 0);
        func_ov255_021d3954(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 20);
    case 1:
        syswk->app->subSeq++;
        if (func_ov255_021d3b48(syswk->app->bgWinFrame) == FALSE) {
            GFL_SndSEPlay(SEQ_SE_SYS_42);
            return func_ov255_021cdc38(syswk, 20);
        }
    case 2:
        func_ov255_021d2478(syswk, 4, syswk->pos);
        func_ov255_021d0ff8(syswk, 6);
        syswk->app->oldCurPos = syswk->pos;
        syswk->app->subSeq = 0;
        if (syswk->moveMode == 1) {
            if (syswk->unk1C_4 == 0) {
                if (Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                    func_ov255_021d1af8(syswk, 0, 1, 2, 0);
                } else {
                    func_ov255_021d1af8(syswk, 0, 1, 0, 0);
                }
                func_ov255_021d0f88(syswk, 10, 1);
                return 21;
            }
            func_ov255_021d0f88(syswk, 10, 1);
        } else {
            func_ov255_021d0f88(syswk, 9, 1);
        }
        return func_ov255_021cd458(syswk, syswk->pos);
    }
    return 20;
}

// Sets the held Pokémon's position and tray
static inline void Box2Seq_SetGetPos(Box2SysWork *syswk, u8 pos, u8 tray) {
    syswk->pos = pos;
    syswk->getTray = tray;
}

// Moving Pokémon: waits for a touch or for the cursor to pick one
static int func_ov255_021c385c(Box2SysWork *syswk) {
    u32 x, y;
    u32 res;

    if (syswk->unk1C_6 == 1) {
        syswk->moveMode = 0;
        syswk->unk1C_6 = 0;
        return func_ov255_021cbe58(syswk, 22);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) && syswk->moveMode != 0 && syswk->unk18 == 0) {
        Box2Main_ShowCursor(syswk);
        syswk->moveMode = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cbe58(syswk, 22);
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
        return func_ov255_021cc460(syswk, 30, 1, 91);
    }
    if (syswk->unk18 == 0) {
        res = func_ov255_021d34d0();
        if (res != 0xffffffff) {
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            func_ov255_021d1ac8(syswk, 0, 0);
            if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
                GFL_SndSEPlay(SEQ_SE_SYS_39);
                func_0203dac8(&syswk->app->tpx, &syswk->app->tpy);
                Box2Main_PokeInfoPut(syswk, res);
                CursorMove_SetPos(syswk->app->cursorMove, res);
                return func_ov255_021cd32c(syswk, res, 21);
            }
            CursorMove_SetPos(syswk->app->cursorMove, res);
            func_ov255_021d28c4(syswk, res);
            syswk->app->oldCurPos = res;
            func_ov255_021d24f8(syswk, res);
            Box2Main_PokeInfoOff(syswk);
            return 21;
        }
    }
    if (func_ov255_021d3554(&x, &y) == TRUE) {
        syswk->app->tpx = x;
        syswk->app->tpy = y;
        syswk->nextSeq = 21;
        func_ov255_021cdc04(syswk);
        return 15;
    }

    res = func_ov255_021d29e8(syswk);
    switch (res) {
    case 30:
        func_ov255_021d1ac8(syswk, 0, 0);
        if (syswk->unk18 != 0) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return func_ov255_021cbe58(syswk, 105);
    case 31:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        func_ov255_021d1ac8(syswk, 0, 0);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        func_ov255_021d24f8(syswk, 30);
        func_ov255_021d28c4(syswk, 30);
        syswk->app->oldCurPos = 30;
        if (syswk->unk18 == 0) {
            Box2Main_PokeSelectOff(syswk);
        } else {
            func_ov255_021d052c(syswk);
        }
        return func_ov255_021ccf1c(syswk, 0, 21);
    case 32:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        func_ov255_021d1ac8(syswk, 0, 0);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        func_ov255_021d24f8(syswk, 30);
        func_ov255_021d28c4(syswk, 30);
        syswk->app->oldCurPos = 30;
        if (syswk->unk18 == 0) {
            Box2Main_PokeSelectOff(syswk);
        } else {
            func_ov255_021d052c(syswk);
        }
        return func_ov255_021ccf68(syswk, 0, 21);
    case 39:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->unk18 == 0) {
            Box2Main_PokeInfoOff(syswk);
        }
        return func_ov255_021cc3b0(syswk, 10, func_ov255_021cbe58(syswk, 25));
    case 40:
        if (syswk->unk18 == 0) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            if (pos >= BOX2_PARTY_POS
                || Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
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
        return func_ov255_021cc460(syswk, 8, 1, 89);
    case 41:
        if (syswk->unk18 != 0) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = 0;
        return func_ov255_021cc460(syswk, 6, 9, func_ov255_021cbe58(syswk, 22));
    case 42:
        if (syswk->moveMode == 0 || syswk->unk18 != 0) {
            break;
        }
        syswk->moveMode = 0;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cbe58(syswk, 22);
    case 43:
        break;
    case 44:
        if (syswk->moveMode == 0 || syswk->unk18 != 0) {
            break;
        }
        syswk->moveMode = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cbe58(syswk, 22);
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
        return func_ov255_021cc460(syswk, 30, 1, 91);
    case CURSORMOVE_CANCEL:
        if (syswk->unk18 == 1) {
            return func_ov255_021cd4b8(syswk, BOX2_GET_NONE, 21);
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = 0;
        return func_ov255_021cc460(syswk, 6, 9, func_ov255_021cbe58(syswk, 22));
    case CURSORMOVE_UNK_8:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 34) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            func_ov255_021bc09c(syswk, -1);
            return func_ov255_021cbec8(syswk, Box2Main_VFuncBoxListScrollRight, 21);
        }
        break;
    case CURSORMOVE_UNK_7:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 37) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            func_ov255_021bc09c(syswk, 1);
            return func_ov255_021cbec8(syswk, Box2Main_VFuncBoxListScrollLeft, 21);
        }
        break;
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return func_ov255_021ccf1c(syswk, 0, 21);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return func_ov255_021ccf68(syswk, 0, 21);
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
            func_ov255_021d1ac8(syswk, pos - 34, 1);
        } else {
            func_ov255_021d1ac8(syswk, 0, 0);
        }
        return func_ov255_021cbed8(syswk, func_ov255_021cbee8(syswk, 21));
    }
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case 33:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        func_ov255_021bc09c(syswk, -1);
        return func_ov255_021cbec8(syswk, Box2Main_VFuncBoxListScrollRight, 21);
    case 38:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        func_ov255_021bc09c(syswk, 1);
        return func_ov255_021cbec8(syswk, Box2Main_VFuncBoxListScrollLeft, 21);
    case 34:
    case 35:
    case 36:
    case 37:
        func_ov255_021d1ac8(syswk, res - 34, 1);
        if (syswk->unk18 == 1) {
            return func_ov255_021cd4b8(syswk, res + 2, 21);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeInfoOff(syswk);
        syswk->app->unkA55F = syswk->trayScroll + res - 34;
        if (syswk->app->unkA55F >= syswk->trayMax) {
            syswk->app->unkA55F -= syswk->trayMax;
        }
        if (syswk->app->unkA55F != syswk->tray) {
            return func_ov255_021cdba4(syswk, 21);
        }
        break;
    case CURSORMOVE_NONE:
        break;
    default:
        func_ov255_021d1ac8(syswk, 0, 0);
        if (syswk->unk18 == 1) {
            return func_ov255_021cd4b8(syswk, res, 21);
        }
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            Box2Seq_SetGetPos(syswk, res, syswk->tray);
            func_ov255_021d0374(syswk, syswk->pos, 1, 1);
            return func_ov255_021cd458(syswk, syswk->pos);
        }
        break;
    }
    return 21;
}

static int func_ov255_021c3ec8(Box2SysWork *syswk) {
    u8 pos;

    switch (syswk->app->subSeq) {
    case 0:
        func_ov255_021d101c(syswk, 0);
        func_ov255_021d1ac8(syswk, 0, 0);
        if (CursorMove_GetPos(syswk->app->cursorMove) >= BOX2_PARTY_POS) {
            Box2Main_PokeInfoOff(syswk);
        }
        if (syswk->moveMode != 2) {
            func_ov255_021d1af8(syswk, 0, 0, 1, 0);
            GFL_SndSEPlay(SEQ_SE_SYS_42);
            syswk->app->subSeq++;
            return func_ov255_021cdc54(syswk, 22);
        }
        return 35;
    case 1:
        if (syswk->unk1B == 1) {
            func_ov255_021d3734(syswk->app->bgWinFrame);
            func_ov255_021d3744(syswk->app->bgWinFrame);
            syswk->app->subSeq++;
            return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 22);
        }
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos >= BOX2_PARTY_POS) {
            pos = 0;
        }
        func_ov255_021d2478(syswk, 3, pos);
        func_ov255_021d0ff8(syswk, 6);
        Box2Main_PokeInfoPut(syswk, pos);
        syswk->app->subSeq = 0;
        func_ov255_021d0f88(syswk, 9, 1);
        return 17;
    case 2:
        func_ov255_021d3a64(syswk->app);
        syswk->app->subSeq = 0;
        return 33;
    }
    return 22;
}

// After a move: puts the cursor back, and says why a move failed
static int func_ov255_021c3fc0(Box2SysWork *syswk) {
    u8 pos;
    u8 n;

    Box2Main_PokeDataMove(syswk);
    syswk->unk18 = 0;
    syswk->getTray = BOX2_GET_NONE;
    if (syswk->app->moveErr == BOX2_MOVE_ERR_NONE) {
        pos = CursorMove_GetPos(syswk->app->cursorMove);
    } else {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
        func_ov255_021d24f8(syswk, syswk->pos);
        func_ov255_021d101c(syswk, 1);
        pos = syswk->pos;
    }
    syswk->app->oldCurPos = pos;
    func_ov255_021cd5d8(syswk);
    if (func_ov255_021d3b48(syswk->app->bgWinFrame) == TRUE) {
        if (syswk->nextSeq == 32) {
            func_ov255_021d2478(syswk, 4, pos);
            Box2Main_PokeInfoPut(syswk, pos);
            syswk->nextSeq = 21;
            n = 0;
            if (Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                n = 2;
            }
        } else if (pos < BOX2_PARTY_POS
                   && Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            Box2Main_PokeInfoPut(syswk, pos);
            n = 0;
        } else {
            Box2Main_PokeInfoOff(syswk);
            n = 2;
        }
    } else if (func_ov255_021d3834(syswk->app->bgWinFrame) == TRUE) {
        if (syswk->nextSeq == 21) {
            pos = CursorMove_GetPos(syswk->app->cursorMove);
            if (pos > 32) {
                pos = syswk->pos;
            }
            func_ov255_021d2478(syswk, 6, pos);
            Box2Main_PokeInfoPut(syswk, pos);
            syswk->nextSeq = 32;
            n = 0;
            if (Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                n = 2;
            }
        } else if (pos < BOX2_BOXLIST_POS
                   && Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            Box2Main_PokeInfoPut(syswk, pos);
            n = 0;
        } else {
            Box2Main_PokeInfoOff(syswk);
            n = 2;
        }
    } else if (pos < BOX2_BOXLIST_POS
               && Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
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
    func_ov255_021d1af8(syswk, 0, 1, n, 0);
    switch (syswk->app->moveErr) {
    case BOX2_MOVE_ERR_MAIL:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        func_ov255_021d101c(syswk, 0);
        func_ov255_021cf270(syswk, 2, 24);
        return 24;
    case BOX2_MOVE_ERR_LAST_BATTLER:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        func_ov255_021d101c(syswk, 0);
        func_ov255_021cf270(syswk, 3, 24);
        return 24;
    case 4:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        func_ov255_021d101c(syswk, 0);
        func_ov255_021cf270(syswk, 4, 24);
        return 24;
    case 5:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        func_ov255_021d101c(syswk, 0);
        func_ov255_021cf2cc(syswk, 0);
        return 24;
    }
    if (syswk->param->mode == 2) {
        if (func_ov255_021d3b48(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d3a48(syswk->app);
        } else {
            func_ov255_021d3a64(syswk->app);
        }
    }
    return syswk->nextSeq;
}

// Closes the message about a move that failed
static int func_ov255_021c41e8(Box2SysWork *syswk) {
    if (Box2Seq_TrgWait(syswk) == BOX2SEQ_TRGWAIT) {
        return 24;
    }
    if (CursorMove_IsCursorVisible(syswk->app->cursorMove) == TRUE) {
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, TRUE);
    }
    func_ov255_021cefa4(syswk->app, syswk->app->moveErr == 5 ? 27 : 24);
    func_ov255_021bc018(syswk);
    if (syswk->param->mode == 2) {
        if (func_ov255_021d3b48(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d3a48(syswk->app);
        } else {
            func_ov255_021d3a64(syswk->app);
        }
    }
    syswk->app->moveErr = BOX2_MOVE_ERR_NONE;
    return syswk->nextSeq;
}

static int func_ov255_021c4258(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        func_ov255_021d1ac8(syswk, 0, 0);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return func_ov255_021cdc54(syswk, 25);
    case 1:
        func_ov255_021d3734(syswk->app->bgWinFrame);
        func_ov255_021d3744(syswk->app->bgWinFrame);
        if (syswk->unk18 == 1) {
            func_ov255_021cfd34(syswk, 1);
        } else {
            func_ov255_021cfd34(syswk, 0);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 25);
    case 2:
        func_ov255_021d2478(syswk, 6, BOX2_PARTY_POS);
        func_ov255_021d28c4(syswk, BOX2_PARTY_POS);
        if (syswk->unk18 == 0) {
            Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
        }
        func_ov255_021d3a64(syswk->app);
        syswk->app->oldCurPos = BOX2_PARTY_POS;
        syswk->app->subSeq = 0;
        return 32;
    }
    return 25;
}

// Starts picking a range
static int func_ov255_021c4328(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        syswk->unk1C_4 = 0;
        if (func_ov255_021d3b48(syswk->app->bgWinFrame) == FALSE) {
            syswk->trayScroll = syswk->tray;
        }
        syswk->getTray = BOX2_GET_NONE;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (syswk->pos >= BOX2_PARTY_POS) {
            syswk->pos = 0;
            Box2Main_PokeInfoPut(syswk, syswk->pos);
        }
        func_ov255_021d101c(syswk, 0);
        syswk->app->subSeq++;
        if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d11a4(syswk, 0);
            func_ov255_021cefa4(syswk->app, 24);
            func_ov255_021bc018(syswk);
            func_ov255_021d3a48(syswk->app);
            func_ov255_021d3954(syswk->app->bgWinFrame);
            return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 35);
        }
    case 1:
        syswk->app->subSeq++;
        if (func_ov255_021d3b48(syswk->app->bgWinFrame) == FALSE) {
            GFL_SndSEPlay(SEQ_SE_SYS_42);
            return func_ov255_021cdc38(syswk, 35);
        }
    case 2:
        func_ov255_021d2478(syswk, 4, syswk->pos);
        func_ov255_021d0ff8(syswk, 6);
        syswk->app->oldCurPos = syswk->pos;
        syswk->app->subSeq = 0;
        if (Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
            func_ov255_021d1af8(syswk, 0, 1, 2, 0);
        } else {
            func_ov255_021d1af8(syswk, 0, 1, 0, 0);
        }
        func_ov255_021d0f88(syswk, 11, 1);
        return 36;
    }
    return 35;
}

// Picking a range: waits for a touch or for the cursor to pick or drop one
static int func_ov255_021c445c(Box2SysWork *syswk) {
    u32 x, y;
    u32 res;

    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) && syswk->moveMode != 0 && syswk->unk18 == 0) {
        Box2Main_ShowCursor(syswk);
        syswk->moveMode = 0;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cbe58(syswk, 37);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) && syswk->unk18 == 0) {
        u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

        Box2Seq_SetGetPos(syswk, pos, syswk->tray);
        syswk->unk13 = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_ShowCursor(syswk);
        return func_ov255_021cc460(syswk, 30, 1, 91);
    }

    if (syswk->unk18 == 0) {
        res = func_ov255_021d34d0();
        if (res != 0xffffffff) {
            Box2Main_PokeInfoPut(syswk, res);
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            func_ov255_021d1ac8(syswk, 0, 0);
            func_ov255_021d1e2c(syswk, 1);
            func_ov255_021d208c(syswk, res, res, 0);
            syswk->app->rangeSelect.startPos = res;
            syswk->app->rangeSelect.endPos = res;
            syswk->app->unkA5B4 = FALSE;
            syswk->unk18 = 3;
            syswk->getTray = syswk->tray;
            func_ov255_021d28c4(syswk, res);
            return 36;
        }
    } else if (syswk->unk18 == 3) {
        if (func_0203da2c() == TRUE) {
            res = func_ov255_021d34e0();
            if (res != 0xffffffff) {
                func_ov255_021d208c(syswk, syswk->app->rangeSelect.startPos, res, 0);
                syswk->app->rangeSelect.endPos = res;
            }
        } else {
            u32 width, height;

            func_ov255_021cdef8(syswk);
            syswk->pos = func_ov255_021d21ec(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos);
            CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
            func_ov255_021d24f8(syswk, syswk->pos);
            func_ov255_021d208c(syswk, syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, 0);
            func_ov255_021d21a4(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, &width, &height);
            syswk->app->rangeWidth = width;
            syswk->app->rangeHeight = height;
            Box2Main_SetRangeFlags(syswk);
            if (Box2Main_GetRangeCount(syswk->app) != 0) {
                func_ov255_021d0374(syswk, syswk->pos, width, height);
                func_ov255_021d2238(syswk->app, syswk->pos, width, height, 0);
                func_ov255_021cf2e8(syswk);
                syswk->unk18 = 2;
                syswk->app->oldCurPos = syswk->pos;
            } else {
                func_ov255_021cded4(syswk);
            }
        }
        return 36;
    } else if (syswk->unk18 == 2 && syswk->app->unkA5B4 == FALSE) {
        res = func_ov255_021d34d0();
        if (res != 0xffffffff) {
            // The touch position's y doubles as the row counter
            for (y = 0; y < syswk->app->rangeHeight; y++) {
                if (res >= syswk->pos + y * 6 && res < syswk->app->rangeWidth + (syswk->pos + y * 6)) {
                    GFL_SndSEPlay(SEQ_SE_SYS_39);
                    func_0203dac8(&syswk->app->tpx, &syswk->app->tpy);
                    CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
                    Box2Main_PokeInfoPut(syswk, syswk->pos);
                    syswk->app->unkA5B4 = TRUE;
                    return func_ov255_021cd390(syswk, 36);
                }
            }
        }
    }

    if (func_ov255_021d3554(&x, &y) == TRUE) {
        syswk->app->tpx = x;
        syswk->app->tpy = y;
        syswk->nextSeq = 36;
        func_ov255_021cdc04(syswk);
        return 15;
    }

    res = func_ov255_021d29e8(syswk);
    if (res != CURSORMOVE_NONE) {
        syswk->app->unkA5B4 = TRUE;
    }
    switch (res) {
    case 30:
        func_ov255_021d1ac8(syswk, 0, 0);
        if (syswk->unk18 == 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Main_PokeInfoOff(syswk);
            return func_ov255_021cbe58(syswk, 105);
        }
        if (syswk->unk18 == 1) {
            func_ov255_021cded4(syswk);
            Box2Main_PokeInfoOff(syswk);
        }
        break;
    case 31:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        func_ov255_021d1ac8(syswk, 0, 0);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        func_ov255_021d24f8(syswk, 30);
        func_ov255_021d28c4(syswk, 30);
        syswk->app->oldCurPos = 30;
        if (syswk->unk18 == 0) {
            Box2Main_PokeSelectOff(syswk);
        } else if (syswk->unk18 == 1) {
            Box2Main_PokeSelectOff(syswk);
            func_ov255_021cded4(syswk);
        } else {
            func_ov255_021d052c(syswk);
        }
        return func_ov255_021ccf1c(syswk, 0, 36);
    case 32:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        func_ov255_021d1ac8(syswk, 0, 0);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        func_ov255_021d24f8(syswk, 30);
        func_ov255_021d28c4(syswk, 30);
        syswk->app->oldCurPos = 30;
        if (syswk->unk18 == 0) {
            Box2Main_PokeSelectOff(syswk);
        } else if (syswk->unk18 == 1) {
            Box2Main_PokeSelectOff(syswk);
            func_ov255_021cded4(syswk);
        } else {
            func_ov255_021d052c(syswk);
        }
        return func_ov255_021ccf68(syswk, 0, 36);
    case 39:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->unk18 == 0) {
            Box2Main_PokeInfoOff(syswk);
        } else if (syswk->unk18 == 1) {
            func_ov255_021cded4(syswk);
            Box2Main_PokeInfoOff(syswk);
        }
        return func_ov255_021cc3b0(syswk, 10, func_ov255_021cbe58(syswk, 40));
    case 40: {
        u8 pos;

        if (syswk->unk18 != 0) {
            break;
        }
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos >= BOX2_PARTY_POS
            || Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
            break;
        }
        syswk->pos = pos;
        syswk->getTray = syswk->tray;
        syswk->unk13 = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc460(syswk, 8, 1, 89);
    }
    case 41:
        if (syswk->unk18 != 0) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = 0;
        return func_ov255_021cc460(syswk, 6, 9, func_ov255_021cbe58(syswk, 37));
    case 42:
        if (syswk->unk18 != 0) {
            break;
        }
        syswk->moveMode = 0;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cbe58(syswk, 37);
    case 43:
        if (syswk->unk18 != 0) {
            break;
        }
        syswk->moveMode = 1;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cbe58(syswk, 37);
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
        return func_ov255_021cc460(syswk, 30, 1, 91);
    case CURSORMOVE_CANCEL:
        if (syswk->unk18 == 1) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            if (pos < BOX2_PARTY_POS) {
                Box2Main_PokeInfoPut(syswk, pos);
            } else {
                Box2Main_PokeInfoOff(syswk);
            }
            func_ov255_021cded4(syswk);
            break;
        }
        if (syswk->unk18 != 0) {
            func_ov255_021d1e2c(syswk, 0);
            return func_ov255_021cd52c(syswk, BOX2_GET_NONE, 36);
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = 0;
        return func_ov255_021cc460(syswk, 6, 9, func_ov255_021cbe58(syswk, 37));
    case CURSORMOVE_UNK_8:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 34) {
            if (syswk->unk18 == 1) {
                Box2Main_PokeSelectOff(syswk);
                func_ov255_021cded4(syswk);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            func_ov255_021bc09c(syswk, -1);
            return func_ov255_021cbec8(syswk, Box2Main_VFuncBoxListScrollRight, 36);
        }
        break;
    case CURSORMOVE_UNK_7:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 37) {
            if (syswk->unk18 == 1) {
                Box2Main_PokeSelectOff(syswk);
                func_ov255_021cded4(syswk);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            func_ov255_021bc09c(syswk, 1);
            return func_ov255_021cbec8(syswk, Box2Main_VFuncBoxListScrollLeft, 36);
        }
        break;
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            if (syswk->unk18 == 1) {
                func_ov255_021cded4(syswk);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return func_ov255_021ccf1c(syswk, 0, 36);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            if (syswk->unk18 == 1) {
                func_ov255_021cded4(syswk);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return func_ov255_021ccf68(syswk, 0, 36);
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
            func_ov255_021d208c(syswk, syswk->app->rangeSelect.startPos, pos, 0);
        }
        if (pos >= 34 && pos <= 37) {
            func_ov255_021d1ac8(syswk, pos - 34, 1);
        } else {
            func_ov255_021d1ac8(syswk, 0, 0);
        }
        return func_ov255_021cbed8(syswk, func_ov255_021cbee8(syswk, 36));
    }
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    case 33:
        if (syswk->unk18 == 1) {
            func_ov255_021cded4(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        func_ov255_021bc09c(syswk, -1);
        return func_ov255_021cbec8(syswk, Box2Main_VFuncBoxListScrollRight, 36);
    case 38:
        if (syswk->unk18 == 1) {
            func_ov255_021cded4(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        func_ov255_021bc09c(syswk, 1);
        return func_ov255_021cbec8(syswk, Box2Main_VFuncBoxListScrollLeft, 36);
    case 34:
    case 35:
    case 36:
    case 37: {
        // The button's slot in the box list
        u32 slot = res - 34;

        func_ov255_021d1ac8(syswk, slot, 1);
        if (syswk->unk18 == 2) {
            return func_ov255_021cd52c(syswk, slot + 36, 36);
        }
        if (syswk->unk18 == 1) {
            func_ov255_021cded4(syswk);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeInfoOff(syswk);
        syswk->app->unkA55F = syswk->trayScroll + slot;
        if (syswk->app->unkA55F >= syswk->trayMax) {
            syswk->app->unkA55F -= syswk->trayMax;
        }
        if (syswk->app->unkA55F != syswk->tray) {
            return func_ov255_021cdba4(syswk, 36);
        }
        break;
    }
    case CURSORMOVE_NONE:
        break;
    default:
        func_ov255_021d1ac8(syswk, 0, 0);
        if (syswk->unk18 == 1) {
            u32 width, height;

            syswk->app->rangeSelect.endPos = res;
            func_ov255_021cdef8(syswk);
            syswk->pos = func_ov255_021d21ec(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos);
            CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
            func_ov255_021d24f8(syswk, syswk->pos);
            func_ov255_021d208c(syswk, syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, 0);
            func_ov255_021d21a4(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, &width, &height);
            syswk->app->rangeWidth = width;
            syswk->app->rangeHeight = height;
            Box2Main_SetRangeFlags(syswk);
            if (Box2Main_GetRangeCount(syswk->app) != 0) {
                func_ov255_021d0374(syswk, syswk->pos, width, height);
                func_ov255_021d2238(syswk->app, syswk->pos, width, height, 0);
                func_ov255_021cf2e8(syswk);
                syswk->unk18 = 2;
                syswk->app->oldCurPos = syswk->pos;
                return func_ov255_021cd494(syswk, syswk->pos, 36);
            }
            func_ov255_021cded4(syswk);
        } else if (syswk->unk18 == 2) {
            if (Box2Main_RangePutCheck(syswk, syswk->tray, res) != FALSE) {
                func_ov255_021d1e2c(syswk, 0);
                return func_ov255_021cd52c(syswk, res, 36);
            }
            GFL_SndSEPlay(SEQ_SE_BEEP);
        } else {
            syswk->getTray = syswk->tray;
            syswk->pos = res;
            syswk->app->rangeSelect.startPos = res;
            syswk->app->rangeSelect.endPos = res;
            syswk->app->unkA5B4 = TRUE;
            func_ov255_021d208c(syswk, res, res, 0);
            func_ov255_021d1e2c(syswk, 1);
            syswk->unk18 = 1;
            syswk->app->rangeWidth = 1;
            syswk->app->rangeHeight = 1;
            func_ov255_021d28c4(syswk, res);
        }
        break;
    }
    return 36;
}

// Range mode: brings the party's frame out
static int func_ov255_021c4da4(Box2SysWork *syswk) {
    u8 pos;

    switch (syswk->app->subSeq) {
    case 0:
        Box2Main_ClearRangeFlags(syswk);
        func_ov255_021d101c(syswk, 0);
        func_ov255_021d1ac8(syswk, 0, 0);
        if (CursorMove_GetPos(syswk->app->cursorMove) >= BOX2_PARTY_POS) {
            Box2Main_PokeInfoOff(syswk);
        }
        if (syswk->moveMode == 0) {
            func_ov255_021d1af8(syswk, 0, 0, 1, 0);
            GFL_SndSEPlay(SEQ_SE_SYS_42);
            syswk->app->subSeq++;
            return func_ov255_021cdc54(syswk, 37);
        }
        return 20;
    case 1:
        if (syswk->unk1B == 1) {
            func_ov255_021d3734(syswk->app->bgWinFrame);
            func_ov255_021d3744(syswk->app->bgWinFrame);
            syswk->app->subSeq++;
            return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 37);
        }
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos >= BOX2_PARTY_POS) {
            pos = 0;
        }
        func_ov255_021d2478(syswk, 3, pos);
        func_ov255_021d0ff8(syswk, 6);
        func_ov255_021d0f88(syswk, 9, 1);
        Box2Main_PokeInfoPut(syswk, pos);
        syswk->app->subSeq = 0;
        return 17;
    case 2:
        func_ov255_021d3a64(syswk->app);
        syswk->app->subSeq = 0;
        return 33;
    }
    return 37;
}

// After a move in range mode: puts the cursor back, and says why a move failed
static int func_ov255_021c4ea0(Box2SysWork *syswk) {
    u8 pos;
    u8 n;

    Box2Main_PokeDataMove(syswk);
    syswk->unk18 = 0;
    syswk->getTray = BOX2_GET_NONE;
    if (syswk->app->moveErr == BOX2_MOVE_ERR_NONE) {
        pos = CursorMove_GetPos(syswk->app->cursorMove);
    } else {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
        func_ov255_021d24f8(syswk, syswk->pos);
        func_ov255_021d101c(syswk, 1);
        pos = syswk->pos;
    }
    syswk->app->oldCurPos = pos;
    func_ov255_021cd5d8(syswk);
    if (func_ov255_021d3b48(syswk->app->bgWinFrame) == TRUE) {
        if (syswk->nextSeq == 42) {
            func_ov255_021d2478(syswk, 4, pos);
            Box2Main_PokeInfoPut(syswk, pos);
            syswk->nextSeq = 36;
            n = 0;
            if (Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                n = 2;
            }
        } else if (pos < BOX2_PARTY_POS
                   && Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            Box2Main_PokeInfoPut(syswk, pos);
            n = 0;
        } else {
            Box2Main_PokeInfoOff(syswk);
            n = 2;
        }
    } else if (func_ov255_021d3834(syswk->app->bgWinFrame) == TRUE) {
        if (syswk->nextSeq == 36) {
            pos = CursorMove_GetPos(syswk->app->cursorMove);
            if (pos > 32) {
                pos = syswk->pos;
            }
            func_ov255_021d2478(syswk, 6, pos);
            Box2Main_PokeInfoPut(syswk, pos);
            syswk->nextSeq = 42;
            n = 0;
            if (Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                n = 2;
            }
        } else if (pos < BOX2_BOXLIST_POS
                   && Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            Box2Main_PokeInfoPut(syswk, pos);
            n = 0;
        } else {
            Box2Main_PokeInfoOff(syswk);
            n = 2;
        }
    } else if (pos < BOX2_BOXLIST_POS
               && Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
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
    func_ov255_021d1af8(syswk, 0, 1, n, 0);
    func_ov255_021d1e2c(syswk, 0);
    switch (syswk->app->moveErr) {
    case BOX2_MOVE_ERR_MAIL:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        func_ov255_021d101c(syswk, 0);
        func_ov255_021cf270(syswk, 2, 24);
        return 39;
    case BOX2_MOVE_ERR_LAST_BATTLER:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        func_ov255_021d101c(syswk, 0);
        func_ov255_021cf270(syswk, 3, 24);
        return 39;
    case 4:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        func_ov255_021d101c(syswk, 0);
        func_ov255_021cf270(syswk, 4, 24);
        return 39;
    case 5:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        func_ov255_021d101c(syswk, 0);
        func_ov255_021cf2cc(syswk, 0);
        return 39;
    }
    if (syswk->param->mode == 2) {
        if (func_ov255_021d3b48(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d3a48(syswk->app);
        } else {
            func_ov255_021d3a64(syswk->app);
        }
    }
    return syswk->nextSeq;
}

// Closes the message about a move in range mode that failed
static int func_ov255_021c50d0(Box2SysWork *syswk) {
    if (Box2Seq_TrgWait(syswk) == BOX2SEQ_TRGWAIT) {
        return 39;
    }
    if (CursorMove_IsCursorVisible(syswk->app->cursorMove) == TRUE) {
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, TRUE);
    }
    func_ov255_021cefa4(syswk->app, syswk->app->moveErr == 5 ? 27 : 24);
    func_ov255_021bc018(syswk);
    if (syswk->param->mode == 2) {
        if (func_ov255_021d3b48(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d3a48(syswk->app);
        } else {
            func_ov255_021d3a64(syswk->app);
        }
    }
    syswk->app->moveErr = BOX2_MOVE_ERR_NONE;
    return syswk->nextSeq;
}

// Range mode: puts the party's frame away
static int func_ov255_021c5140(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        func_ov255_021d1ac8(syswk, 0, 0);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return func_ov255_021cdc54(syswk, 40);
    case 1:
        func_ov255_021d3734(syswk->app->bgWinFrame);
        func_ov255_021d3744(syswk->app->bgWinFrame);
        if (syswk->unk18 == 2) {
            func_ov255_021cfd34(syswk, 1);
            func_ov255_021cfdb0(syswk);
        } else if (syswk->unk18 == 0) {
            func_ov255_021cfd34(syswk, 0);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 40);
    case 2:
        func_ov255_021d2478(syswk, 6, BOX2_PARTY_POS);
        func_ov255_021d28c4(syswk, BOX2_PARTY_POS);
        if (syswk->unk18 == 0) {
            Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
        }
        func_ov255_021d3a64(syswk->app);
        syswk->app->oldCurPos = BOX2_PARTY_POS;
        if (syswk->unk18 == 2) {
            func_ov255_021d232c(syswk, 0);
        }
        syswk->app->subSeq = 0;
        return 42;
    }
    return 40;
}

// Range mode: brings the party's frame in and puts the menu away
static int func_ov255_021c522c(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        syswk->unk1C_4 = 0;
        if (func_ov255_021d3834(syswk->app->bgWinFrame) == TRUE) {
            syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
            syswk->app->subSeq = 2;
            break;
        }
        if (func_ov255_021d3858(syswk->app->bgWinFrame) == TRUE) {
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
        func_ov255_021cefa4(syswk->app, 24);
        func_ov255_021bc018(syswk);
        func_ov255_021d3a64(syswk->app);
        func_ov255_021d11a4(syswk, 0);
        func_ov255_021d101c(syswk, 0);
        func_ov255_021d3954(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 41);
    case 1:
        if (syswk->param->mode == 4 && syswk->param->unk14 == 1) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
            func_ov255_021cf2cc(syswk, 0);
            syswk->moveMode = 0;
            syswk->app->subSeq = 0;
            syswk->nextSeq = 53;
            return BOX2SEQ_TRGWAIT;
        }
        syswk->app->subSeq++;
        if (func_ov255_021d3858(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d37b0(syswk->app->bgWinFrame);
        } else {
            func_ov255_021d3734(syswk->app->bgWinFrame);
            func_ov255_021d3744(syswk->app->bgWinFrame);
            func_ov255_021cfd34(syswk, 0);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 41);
    case 2:
        func_ov255_021d0310(syswk, 1, 0);
        func_ov255_021d1348(syswk->app, 1);
        func_ov255_021d2478(syswk, 6, syswk->pos);
        func_ov255_021d0ff8(syswk, 6);
        syswk->app->oldCurPos = syswk->pos;
        syswk->app->subSeq = 0;
        if (Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
            func_ov255_021d1af8(syswk, 0, 1, 2, 0);
        } else {
            func_ov255_021d1af8(syswk, 0, 1, 0, 0);
        }
        if (syswk->param->mode == 4) {
            func_ov255_021d3a58(syswk->app);
            func_ov255_021d3a74(syswk->app);
            CursorMove_DisablePos(syswk->app->cursorMove, 39);
        }
        func_ov255_021d0f88(syswk, 11, 1);
        return 42;
    }
    return 41;
}

// Picking a range with the party out: waits for a touch or for the cursor to pick or drop one
static int func_ov255_021c53f4(Box2SysWork *syswk) {
    u32 i;
    u32 res;

    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) && syswk->unk18 == 0) {
        Box2Main_ShowCursor(syswk);
        syswk->moveMode = 0;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cbe58(syswk, 43);
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) {
        if (syswk->unk18 == 0) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            Box2Seq_SetGetPos(syswk, pos, syswk->tray);
            syswk->unk13 = 4;
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Main_ShowCursor(syswk);
            return func_ov255_021cc460(syswk, 30, 1, 91);
        }
        return 42;
    }

    if (syswk->unk18 == 0) {
        res = func_ov255_021d34d0();
        if (res != 0xffffffff) {
            Box2Main_PokeInfoPut(syswk, res);
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            func_ov255_021d1e2c(syswk, 1);
            func_ov255_021d208c(syswk, res, res, 0);
            syswk->app->rangeSelect.startPos = (u8)res;
            syswk->app->rangeSelect.endPos = (u8)res;
            syswk->app->unkA5B4 = FALSE;
            syswk->unk18 = 3;
            syswk->getTray = syswk->tray;
            func_ov255_021d28c4(syswk, res);
            return 42;
        }
        res = func_ov255_021d3514();
        if (res != 0xffffffff) {
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            func_ov255_021d1e2c(syswk, 1);
            func_ov255_021d208c(syswk, res + BOX2_PARTY_POS, res + BOX2_PARTY_POS, 0);
            res += BOX2_PARTY_POS;
            syswk->app->rangeSelect.startPos = (u8)res;
            syswk->app->rangeSelect.endPos = (u8)res;
            syswk->app->unkA5B4 = FALSE;
            syswk->unk18 = 4;
            syswk->getTray = BOX2_GET_NONE;
            func_ov255_021d28c4(syswk, res);
            return 42;
        }
    } else if (syswk->unk18 == 3) {
        if (func_0203da2c() == TRUE) {
            res = func_ov255_021d34e0();
            if (res != 0xffffffff) {
                func_ov255_021d208c(syswk, syswk->app->rangeSelect.startPos, res, 0);
                syswk->app->rangeSelect.endPos = res;
            }
        } else {
            u32 width, height;

            func_ov255_021cdef8(syswk);
            syswk->pos = func_ov255_021d21ec(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos);
            CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
            func_ov255_021d24f8(syswk, syswk->pos);
            func_ov255_021d208c(syswk, syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, 0);
            func_ov255_021d21a4(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, &width, &height);
            syswk->app->rangeWidth = width;
            syswk->app->rangeHeight = height;
            Box2Main_SetRangeFlags(syswk);
            if (Box2Main_GetRangeCount(syswk->app) != 0) {
                func_ov255_021d0374(syswk, syswk->pos, width, height);
                func_ov255_021d2238(syswk->app, syswk->pos, width, height, 0);
                func_ov255_021cf2e8(syswk);
                syswk->unk18 = 2;
                syswk->app->oldCurPos = syswk->pos;
            } else {
                func_ov255_021cded4(syswk);
            }
        }
        return 42;
    } else if (syswk->unk18 == 4) {
        if (func_0203da2c() == TRUE) {
            if (func_ov255_021d3604() == TRUE) {
                res = func_ov255_021d3524();
                if (res != 0xffffffff) {
                    func_ov255_021d208c(syswk, syswk->app->rangeSelect.startPos, res + BOX2_PARTY_POS, 0);
                    syswk->app->rangeSelect.endPos = res + BOX2_PARTY_POS;
                }
            }
        } else {
            u32 width, height;

            func_ov255_021cdef8(syswk);
            syswk->pos = func_ov255_021d2210(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos);
            CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
            func_ov255_021d24f8(syswk, syswk->pos);
            func_ov255_021d208c(syswk, syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, 0);
            func_ov255_021d21a4(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, &width, &height);
            syswk->app->rangeWidth = width;
            syswk->app->rangeHeight = height;
            Box2Main_SetRangeFlags(syswk);
            if (Box2Main_GetRangeCount(syswk->app) != 0) {
                func_ov255_021d0374(syswk, syswk->pos, width, height);
                func_ov255_021d2238(syswk->app, syswk->pos, width, height, 0);
                func_ov255_021cf2e8(syswk);
                syswk->unk18 = 2;
                syswk->app->oldCurPos = syswk->pos;
            } else {
                func_ov255_021cded4(syswk);
            }
        }
        return 42;
    } else if (syswk->unk18 == 2 && syswk->app->unkA5B4 == FALSE) {
        if (syswk->pos < BOX2_PARTY_POS) {
            res = func_ov255_021d34d0();
            if (res != 0xffffffff) {
                for (i = 0; i < syswk->app->rangeHeight; i++) {
                    if (res >= syswk->pos + i * 6 && res < syswk->app->rangeWidth + (syswk->pos + i * 6)) {
                        GFL_SndSEPlay(SEQ_SE_SYS_39);
                        func_0203dac8(&syswk->app->tpx, &syswk->app->tpy);
                        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
                        Box2Main_PokeInfoPut(syswk, syswk->pos);
                        syswk->app->unkA5B4 = TRUE;
                        return func_ov255_021cd390(syswk, 42);
                    }
                }
            }
        } else {
            res = func_ov255_021d3514();
            if (res != 0xffffffff) {
                // The range's first slot in the party
                u32 top = syswk->pos - BOX2_PARTY_POS;

                for (i = 0; i < syswk->app->rangeHeight; i++) {
                    if (res >= top + i * 2 && res < syswk->app->rangeWidth + (top + i * 2)) {
                        GFL_SndSEPlay(SEQ_SE_SYS_39);
                        func_ov255_021d208c(syswk, syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, 1);
                        func_0203dac8(&syswk->app->tpx, &syswk->app->tpy);
                        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
                        Box2Main_PokeInfoPut(syswk, syswk->pos);
                        syswk->app->unkA5B4 = TRUE;
                        return func_ov255_021cd390(syswk, 42);
                    }
                }
            }
        }
    }

    res = func_ov255_021d2b88(syswk);
    if (res != CURSORMOVE_NONE) {
        syswk->app->unkA5B4 = TRUE;
    }
    switch (res) {
    case 36:
        if (syswk->unk18 == 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            Box2Main_PokeInfoOff(syswk);
            return func_ov255_021cbe58(syswk, 105);
        }
        if (syswk->unk18 == 1) {
            func_ov255_021cded4(syswk);
            Box2Main_PokeInfoOff(syswk);
        }
        break;
    case 37:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
        func_ov255_021d24f8(syswk, BOX2_BOXLIST_POS);
        func_ov255_021d28c4(syswk, BOX2_BOXLIST_POS);
        syswk->app->oldCurPos = BOX2_BOXLIST_POS;
        if (syswk->unk18 == 0) {
            Box2Main_PokeSelectOff(syswk);
        } else if (syswk->unk18 == 1) {
            Box2Main_PokeSelectOff(syswk);
            func_ov255_021cded4(syswk);
        } else {
            func_ov255_021d052c(syswk);
        }
        return func_ov255_021ccf1c(syswk, 0, 42);
    case 38:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
        func_ov255_021d24f8(syswk, BOX2_BOXLIST_POS);
        func_ov255_021d28c4(syswk, BOX2_BOXLIST_POS);
        syswk->app->oldCurPos = BOX2_BOXLIST_POS;
        if (syswk->unk18 == 0) {
            Box2Main_PokeSelectOff(syswk);
        } else if (syswk->unk18 == 1) {
            Box2Main_PokeSelectOff(syswk);
            func_ov255_021cded4(syswk);
        } else {
            func_ov255_021d052c(syswk);
        }
        return func_ov255_021ccf68(syswk, 0, 42);
    case 39:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->unk18 == 0) {
            Box2Main_PokeInfoOff(syswk);
        } else if (syswk->unk18 == 1) {
            func_ov255_021cded4(syswk);
            Box2Main_PokeInfoOff(syswk);
        }
        return func_ov255_021cc3b0(syswk, 11, func_ov255_021cbe58(syswk, 44));
    case 40: {
        u8 pos;

        if (syswk->unk18 != 0) {
            break;
        }
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos >= BOX2_BOXLIST_POS
            || Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
            break;
        }
        syswk->pos = pos;
        syswk->getTray = syswk->tray;
        syswk->unk13 = 4;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc460(syswk, 8, 1, 89);
    }
    case 41:
        if (syswk->unk18 != 0) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = 0;
        return func_ov255_021cc460(syswk, 6, 9, func_ov255_021cbe58(syswk, 43));
    case 42:
        if (syswk->unk18 != 0) {
            break;
        }
        syswk->moveMode = 0;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cbe58(syswk, 43);
    case 43:
        if (syswk->unk18 != 0) {
            break;
        }
        syswk->moveMode = 1;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cbe58(syswk, 43);
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
        return func_ov255_021cc460(syswk, 30, 1, 91);
    }
    case CURSORMOVE_CANCEL:
        if (syswk->unk18 == 1) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            if (pos < BOX2_BOXLIST_POS) {
                Box2Main_PokeInfoPut(syswk, pos);
            } else {
                Box2Main_PokeInfoOff(syswk);
            }
            func_ov255_021cded4(syswk);
            break;
        }
        if (syswk->unk18 != 0) {
            func_ov255_021d1e2c(syswk, 0);
            return func_ov255_021cd52c(syswk, BOX2_GET_NONE, 42);
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = 0;
        return func_ov255_021cc460(syswk, 6, 9, func_ov255_021cbe58(syswk, 43));
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BOXLIST_POS) {
            if (syswk->unk18 == 1) {
                Box2Main_PokeSelectOff(syswk);
                func_ov255_021cded4(syswk);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return func_ov255_021ccf1c(syswk, 0, 42);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BOXLIST_POS) {
            if (syswk->unk18 == 1) {
                Box2Main_PokeSelectOff(syswk);
                func_ov255_021cded4(syswk);
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return func_ov255_021ccf68(syswk, 0, 42);
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
                    func_ov255_021d208c(syswk, syswk->app->rangeSelect.startPos, pos, 0);
                }
            } else if (pos >= BOX2_PARTY_POS && pos < BOX2_BOXLIST_POS) {
                func_ov255_021d208c(syswk, syswk->app->rangeSelect.startPos, pos, 0);
            }
        }
        return func_ov255_021cbed8(syswk, func_ov255_021cbee8(syswk, 42));
    }
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
    case CURSORMOVE_NONE:
        break;
    default:
        if (syswk->unk18 == 1) {
            u32 width, height;

            if ((syswk->app->rangeSelect.endPos < BOX2_PARTY_POS && res >= BOX2_PARTY_POS)
                || (syswk->app->rangeSelect.endPos >= BOX2_PARTY_POS && res < BOX2_PARTY_POS)) {
                func_ov255_021cded4(syswk);
                Box2Main_PokeInfoPut(syswk, CursorMove_GetPos(syswk->app->cursorMove));
                break;
            }
            syswk->app->rangeSelect.endPos = res;
            func_ov255_021cdef8(syswk);
            if (syswk->app->rangeSelect.startPos < BOX2_PARTY_POS) {
                syswk->pos = func_ov255_021d21ec(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos);
            } else {
                syswk->pos = func_ov255_021d2210(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos);
            }
            CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
            func_ov255_021d24f8(syswk, syswk->pos);
            func_ov255_021d208c(syswk, syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, 0);
            func_ov255_021d21a4(syswk->app->rangeSelect.startPos, syswk->app->rangeSelect.endPos, &width, &height);
            syswk->app->rangeWidth = width;
            syswk->app->rangeHeight = height;
            Box2Main_SetRangeFlags(syswk);
            if (Box2Main_GetRangeCount(syswk->app) != 0) {
                func_ov255_021d0374(syswk, syswk->pos, width, height);
                func_ov255_021d2238(syswk->app, syswk->pos, width, height, 0);
                func_ov255_021cf2e8(syswk);
                syswk->unk18 = 2;
                syswk->app->oldCurPos = syswk->pos;
                return func_ov255_021cd494(syswk, syswk->pos, 42);
            }
            func_ov255_021cded4(syswk);
        } else if (syswk->unk18 == 2) {
            if (Box2Main_RangePutCheck(syswk, syswk->tray, res) != FALSE) {
                func_ov255_021d1e2c(syswk, 0);
                return func_ov255_021cd52c(syswk, res, 42);
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
            func_ov255_021d208c(syswk, res, res, 0);
            func_ov255_021d1e2c(syswk, 1);
            syswk->unk18 = 1;
            syswk->app->rangeWidth = 1;
            syswk->app->rangeHeight = 1;
            func_ov255_021d28c4(syswk, res);
        }
        break;
    }
    return 42;
}

// Range mode: puts the party's frame away or out
static int func_ov255_021c5d9c(Box2SysWork *syswk) {
    u8 pos;

    switch (syswk->app->subSeq) {
    case 0:
        Box2Main_ClearRangeFlags(syswk);
        if (syswk->moveMode == 0) {
            func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
            func_ov255_021d1af8(syswk, 0, 0, 1, 0);
            if (syswk->unk1B == 0) {
                func_ov255_021d3778(syswk->app->bgWinFrame);
                syswk->app->subSeq = 2;
            } else {
                func_ov255_021d0310(syswk, 1, 1);
                func_ov255_021d1348(syswk->app, 0);
                func_ov255_021d37c4(syswk->app->bgWinFrame);
                syswk->app->subSeq = 1;
            }
            GFL_SndSEPlay(SEQ_SE_SYS_42);
            return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 43);
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
        func_ov255_021d0ff8(syswk, 6);
        func_ov255_021d0f88(syswk, 9, 1);
        if (syswk->param->mode == 2) {
            func_ov255_021d2478(syswk, 5, pos);
            return 27;
        }
        func_ov255_021d3a64(syswk->app);
        func_ov255_021d2478(syswk, 14, pos);
        return 49;
    case 2:
        syswk->app->subSeq = 0;
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos >= BOX2_PARTY_POS) {
            pos = 0;
        }
        Box2Main_PokeInfoPut(syswk, pos);
        func_ov255_021d3a48(syswk->app);
        func_ov255_021d0ff8(syswk, 6);
        func_ov255_021d0f88(syswk, 9, 1);
        if (syswk->param->mode == 4) {
            func_ov255_021d2478(syswk, 13, pos);
            return 45;
        }
        func_ov255_021d2478(syswk, 3, pos);
        return 17;
    }
    return 43;
}

// Range mode: closes the party's frame
static int func_ov255_021c5ef4(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        func_ov255_021d3778(syswk->app->bgWinFrame);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 44);
    case 1:
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        if (func_ov255_021d3b48(syswk->app->bgWinFrame) == FALSE) {
            syswk->trayScroll = syswk->tray;
        }
        syswk->app->subSeq++;
        return func_ov255_021cdc38(syswk, 44);
    case 2:
        func_ov255_021d2478(syswk, 4, 39);
        func_ov255_021d3a48(syswk->app);
        syswk->app->oldCurPos = 39;
        syswk->app->subSeq = 0;
        return 36;
    }
    return 44;
}

// Brings the party's frame out
static int func_ov255_021c5f94(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        Box2Main_PokeSelectOff(syswk);
        func_ov255_021bc018(syswk);
        func_ov255_021d1348(syswk->app, 0);
        func_ov255_021d0310(syswk, 1, 1);
        func_ov255_021d101c(syswk, 0);
        func_ov255_021d3724(syswk->app->bgWinFrame);
        func_ov255_021d3744(syswk->app->bgWinFrame);
        func_ov255_021cfd34(syswk, 0);
        if (syswk->param->mode == 3) {
            func_ov255_021d0310(syswk, 0x82, 1);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 26);
    case 1:
        syswk->app->subSeq = 0;
        Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
        func_ov255_021d3a64(syswk->app);
        if (syswk->param->mode == 3) {
            func_ov255_021d2478(syswk, 8, 0);
            return 80;
        }
        func_ov255_021d2478(syswk, 5, 0);
        return 27;
    }
    return 26;
}

// The party's main state: waits for a touch or for the cursor to pick something
static int func_ov255_021c6058(Box2SysWork *syswk) {
    u32 res;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        syswk->unk1B = 1;
        return func_ov255_021cddf0(syswk, 31);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) && func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE) {
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove) + BOX2_PARTY_POS;
        syswk->unk13 = 4;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_ShowCursor(syswk);
        return func_ov255_021cc460(syswk, 30, 1, 91);
    }

    res = func_ov255_021d3504();
    if (res != 0xffffffff) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            func_0203dac8(&syswk->app->tpx, &syswk->app->tpy);
            if (func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE) {
                func_ov255_021ceed0(syswk, sMenu71f0, 6);
            }
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            return func_ov255_021cd16c(syswk, res + BOX2_PARTY_POS);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        func_ov255_021d24f8(syswk, res);
        Box2Main_PokeInfoOff(syswk);
        if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d3954(syswk->app->bgWinFrame);
            func_ov255_021d11a4(syswk, 0);
            func_ov255_021cefa4(syswk->app, 24);
            func_ov255_021bc018(syswk);
            func_ov255_021d3a64(syswk->app);
            return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 27);
        }
        return 27;
    }

    res = func_ov255_021d2a64(syswk);
    switch (res) {
    case 6:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc3b0(syswk, 10, func_ov255_021cbe58(syswk, 28));
    case 7:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->unk28 = 1;
        return func_ov255_021cc460(syswk, 7, 8, 16);
    case 8:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return func_ov255_021cc460(syswk, 6, 9, func_ov255_021cbe58(syswk, 28));
    case 9:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk1B = 1;
        syswk->moveMode = 0;
        return func_ov255_021cc3b0(syswk, 0, func_ov255_021cbe58(syswk, 31));
    case 10:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->getTray = syswk->tray;
        syswk->unk13 = 1;
        return func_ov255_021cc3b0(syswk, 1, 89);
    case 11:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk13 = 1;
        return func_ov255_021cc3b0(syswk, 2, func_ov255_021cbe58(syswk, 90));
    case 12:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc3b0(syswk, 3, func_ov255_021cbe58(syswk, 97));
    case 13:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc3b0(syswk, 4, func_ov255_021cbe58(syswk, 101));
    case 14: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos - BOX2_PARTY_POS;
        func_ov255_021d32d4(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        syswk->app->oldCurPos = pos;
        return func_ov255_021cc3b0(syswk, 5, 29);
    }
    case 15:
        break;
    case 16:
        syswk->unk1B = 1;
        syswk->moveMode = 1;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cde88(syswk, 31);
    case 17:
        syswk->unk1B = 1;
        syswk->moveMode = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cde88(syswk, 41);
    case 18:
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove) + BOX2_PARTY_POS;
        syswk->unk13 = 4;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc460(syswk, 30, 1, 91);
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
        return func_ov255_021cbed8(syswk, func_ov255_021cbee8(syswk, 27));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos - BOX2_PARTY_POS;

            func_ov255_021d32d4(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            syswk->app->oldCurPos = pos;
            return func_ov255_021cc3b0(syswk, 5, 29);
        }
        return func_ov255_021cc460(syswk, 6, 9, func_ov255_021cbe58(syswk, 28));
    case CURSORMOVE_NONE:
    case CURSORMOVE_SCROLL_R:
    case CURSORMOVE_SCROLL_L:
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            func_ov255_021ceed0(syswk, sMenu71f0, 6);
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            func_ov255_021d32d4(syswk->app, 9, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, 9);
            syswk->app->oldCurPos = 9;
            return func_ov255_021cd2e8(syswk, res + BOX2_PARTY_POS, 27);
        }
        break;
    }
    return 27;
}

// Puts the party's frame away
static int func_ov255_021c64fc(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        Box2Main_PokeSelectOff(syswk);
        func_ov255_021bc018(syswk);
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        func_ov255_021d3778(syswk->app->bgWinFrame);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 28);
    case 1:
        syswk->app->subSeq = 0;
        func_ov255_021d1348(syswk->app, 1);
        func_ov255_021d3a48(syswk->app);
        if (syswk->param->mode == 3) {
            func_ov255_021d0310(syswk, 0x81, 1);
            func_ov255_021d2478(syswk, 7, 33);
            return 67;
        }
        func_ov255_021d0310(syswk, 1, 0);
        func_ov255_021d2478(syswk, 3, 33);
        return 17;
    }
    return 28;
}

// Closes the party's menu
static int func_ov255_021c65a4(Box2SysWork *syswk) {
    func_ov255_021d3954(syswk->app->bgWinFrame);
    func_ov255_021d11a4(syswk, 0);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    func_ov255_021d3a64(syswk->app);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncCursorMoveFrame, func_ov255_021cbee8(syswk, 27));
}

// After a move in the party
static int func_ov255_021c65e4(Box2SysWork *syswk) {
    GFL_SndSEPlay(SEQ_SE_SYS_40);
    Box2Main_PokeDataMove(syswk);
    func_ov255_021cd5d8(syswk);
    CursorMove_SetPos(syswk->app->cursorMove, 9);
    syswk->app->oldCurPos = 9;
    func_ov255_021d24f8(syswk, 9);
    func_ov255_021d101c(syswk, 1);
    func_ov255_021d1af8(syswk, 0, 0, 1, 0);
    func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
    return 27;
}

// Moving Pokémon with the party out: brings the party's frame in
static int func_ov255_021c6644(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        if (func_ov255_021d3834(syswk->app->bgWinFrame) == TRUE) {
            syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
            syswk->app->subSeq = 2;
            break;
        }
        if (syswk->moveMode == 1 && syswk->unk1C_4 == 0) {
            syswk->getTray = BOX2_GET_NONE;
            syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
            if (func_ov255_021d3858(syswk->app->bgWinFrame) == TRUE) {
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
            if (func_ov255_021d3858(syswk->app->bgWinFrame) == TRUE) {
                syswk->getTray = BOX2_GET_NONE;
            } else {
                syswk->getTray = syswk->tray;
            }
            func_ov255_021cefa4(syswk->app, 24);
            func_ov255_021bc018(syswk);
            func_ov255_021d3a64(syswk->app);
            func_ov255_021d11a4(syswk, 0);
        }
        func_ov255_021d101c(syswk, 0);
        func_ov255_021d3954(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 31);
    case 1:
        if (syswk->param->mode == 4 && syswk->param->unk14 == 1) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
            func_ov255_021cf2cc(syswk, 0);
            syswk->moveMode = 0;
            syswk->app->subSeq = 0;
            syswk->nextSeq = 53;
            return BOX2SEQ_TRGWAIT;
        }
        syswk->app->subSeq++;
        if (func_ov255_021d3858(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d37b0(syswk->app->bgWinFrame);
        } else {
            func_ov255_021d3734(syswk->app->bgWinFrame);
            func_ov255_021d3744(syswk->app->bgWinFrame);
            func_ov255_021cfd34(syswk, 0);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 31);
    case 2:
        func_ov255_021d0310(syswk, 1, 0);
        func_ov255_021d1348(syswk->app, 1);
        func_ov255_021d2478(syswk, 6, syswk->pos);
        func_ov255_021d0ff8(syswk, 6);
        syswk->app->oldCurPos = syswk->pos;
        syswk->app->subSeq = 0;
        if (syswk->moveMode == 1) {
            if (syswk->unk1C_4 == 0) {
                if (Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
                    func_ov255_021d1af8(syswk, 0, 1, 2, 0);
                } else {
                    func_ov255_021d1af8(syswk, 0, 1, 0, 0);
                }
                if (syswk->param->mode == 4) {
                    func_ov255_021d3a58(syswk->app);
                    func_ov255_021d3a74(syswk->app);
                    CursorMove_DisablePos(syswk->app->cursorMove, 39);
                }
                func_ov255_021d0f88(syswk, 10, 1);
                return 32;
            }
            func_ov255_021d0f88(syswk, 10, 1);
        } else {
            func_ov255_021d0f88(syswk, 9, 1);
        }
        func_ov255_021d0374(syswk, syswk->pos, 1, 1);
        return func_ov255_021cd3f8(syswk, syswk->pos, 32);
    }
    return 31;
}

// Moving Pokémon with the party out: waits for a touch or for the cursor to pick one
static int func_ov255_021c686c(Box2SysWork *syswk) {
    u32 res;

    if (syswk->unk1C_6 == 1) {
        syswk->moveMode = 0;
        syswk->unk1C_6 = 0;
        return func_ov255_021cbe58(syswk, 33);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) && syswk->moveMode != 0 && syswk->unk18 == 0) {
        Box2Main_ShowCursor(syswk);
        syswk->moveMode = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cbe58(syswk, 33);
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
        return func_ov255_021cc460(syswk, 30, 1, 91);
    }
    if (syswk->unk18 == 0) {
        res = func_ov255_021d34d0();
        if (res != 0xffffffff) {
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
                GFL_SndSEPlay(SEQ_SE_SYS_39);
                func_0203dac8(&syswk->app->tpx, &syswk->app->tpy);
                Box2Main_PokeInfoPut(syswk, res);
                return func_ov255_021cd32c(syswk, res, 32);
            }
            CursorMove_SetPos(syswk->app->cursorMove, res);
            func_ov255_021d28c4(syswk, res);
            syswk->app->oldCurPos = res;
            func_ov255_021d24f8(syswk, res);
            Box2Main_PokeInfoOff(syswk);
            return 32;
        }
        res = func_ov255_021d3514();
        if (res != 0xffffffff) {
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
                GFL_SndSEPlay(SEQ_SE_SYS_39);
                func_0203dac8(&syswk->app->tpx, &syswk->app->tpy);
                Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
                return func_ov255_021cd32c(syswk, res + BOX2_PARTY_POS, 32);
            }
            CursorMove_SetPos(syswk->app->cursorMove, res + BOX2_PARTY_POS);
            func_ov255_021d28c4(syswk, res + BOX2_PARTY_POS);
            syswk->app->oldCurPos = res + BOX2_PARTY_POS;
            func_ov255_021d24f8(syswk, res + BOX2_PARTY_POS);
            Box2Main_PokeInfoOff(syswk);
            return 32;
        }
    }

    res = func_ov255_021d2b88(syswk);
    switch (res) {
    case 36:
        if (syswk->unk18 != 0) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return func_ov255_021cbe58(syswk, 105);
    case 37:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
        func_ov255_021d24f8(syswk, BOX2_BOXLIST_POS);
        func_ov255_021d28c4(syswk, BOX2_BOXLIST_POS);
        syswk->app->oldCurPos = BOX2_BOXLIST_POS;
        if (syswk->unk18 == 0) {
            Box2Main_PokeSelectOff(syswk);
        } else {
            func_ov255_021d052c(syswk);
        }
        return func_ov255_021ccf1c(syswk, 0, 32);
    case 38:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
        func_ov255_021d24f8(syswk, BOX2_BOXLIST_POS);
        func_ov255_021d28c4(syswk, BOX2_BOXLIST_POS);
        syswk->app->oldCurPos = BOX2_BOXLIST_POS;
        if (syswk->unk18 == 0) {
            Box2Main_PokeSelectOff(syswk);
        } else {
            func_ov255_021d052c(syswk);
        }
        return func_ov255_021ccf68(syswk, 0, 32);
    case 39:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->unk18 == 0) {
            Box2Main_PokeInfoOff(syswk);
        }
        return func_ov255_021cc3b0(syswk, 11, func_ov255_021cbe58(syswk, 34));
    case 40:
        if (syswk->unk18 == 0) {
            u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

            if (pos >= BOX2_BOXLIST_POS
                || Box2Main_GetPokeParam(syswk, pos, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
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
        return func_ov255_021cc460(syswk, 8, 1, 89);
    case 41:
        if (syswk->unk18 != 0) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = 0;
        return func_ov255_021cc460(syswk, 6, 9, func_ov255_021cbe58(syswk, 33));
    case 42:
        if (syswk->unk18 != 0) {
            break;
        }
        syswk->moveMode = 0;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cbe58(syswk, 33);
    case 43:
        break;
    case 44:
        if (syswk->unk18 != 0) {
            break;
        }
        syswk->moveMode = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cbe58(syswk, 33);
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
        return func_ov255_021cc460(syswk, 30, 1, 91);
    case CURSORMOVE_CANCEL:
        if (syswk->unk18 == 1) {
            return func_ov255_021cd4b8(syswk, BOX2_GET_NONE, 32);
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = 0;
        return func_ov255_021cc460(syswk, 6, 9, func_ov255_021cbe58(syswk, 33));
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BOXLIST_POS) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return func_ov255_021ccf1c(syswk, 0, 32);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BOXLIST_POS) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return func_ov255_021ccf68(syswk, 0, 32);
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
        return func_ov255_021cbed8(syswk, func_ov255_021cbee8(syswk, 32));
    case CURSORMOVE_NONE:
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
        break;
    default:
        if (syswk->unk18 == 1) {
            return func_ov255_021cd4b8(syswk, res, 32);
        }
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            Box2Seq_SetGetPos(syswk, res, syswk->tray);
            func_ov255_021d0374(syswk, syswk->pos, 1, 1);
            return func_ov255_021cd3f8(syswk, syswk->pos, 32);
        }
        break;
    }
    return 32;
}

// Moving Pokémon with the party out: puts the party's frame away or out
static int func_ov255_021c6de0(Box2SysWork *syswk) {
    u8 pos;

    switch (syswk->app->subSeq) {
    case 0:
        if (syswk->moveMode != 2) {
            func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
            func_ov255_021d1af8(syswk, 0, 0, 1, 0);
            if (syswk->unk1B == 0) {
                func_ov255_021d3778(syswk->app->bgWinFrame);
                syswk->app->subSeq = 2;
            } else {
                func_ov255_021d0310(syswk, 1, 1);
                func_ov255_021d1348(syswk->app, 0);
                func_ov255_021d37c4(syswk->app->bgWinFrame);
                syswk->app->subSeq = 1;
            }
            GFL_SndSEPlay(SEQ_SE_SYS_42);
            return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 33);
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
        func_ov255_021d0ff8(syswk, 6);
        func_ov255_021d0f88(syswk, 9, 1);
        if (syswk->param->mode == 2) {
            func_ov255_021d2478(syswk, 5, pos);
            return 27;
        }
        func_ov255_021d3a64(syswk->app);
        func_ov255_021d2478(syswk, 14, pos);
        return 49;
    case 2:
        syswk->app->subSeq = 0;
        pos = CursorMove_GetPos(syswk->app->cursorMove);
        if (pos >= BOX2_PARTY_POS) {
            pos = 0;
        }
        Box2Main_PokeInfoPut(syswk, pos);
        func_ov255_021d3a48(syswk->app);
        func_ov255_021d0ff8(syswk, 6);
        func_ov255_021d0f88(syswk, 9, 1);
        if (syswk->param->mode == 4) {
            func_ov255_021d2478(syswk, 13, pos);
            return 45;
        }
        func_ov255_021d2478(syswk, 3, pos);
        return 17;
    }
    return 33;
}

// Moving Pokémon: closes the party's frame
static int func_ov255_021c6f34(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        func_ov255_021d3778(syswk->app->bgWinFrame);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 34);
    case 1:
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        if (func_ov255_021d3b48(syswk->app->bgWinFrame) == FALSE) {
            syswk->trayScroll = syswk->tray;
        }
        syswk->app->subSeq++;
        return func_ov255_021cdc38(syswk, 34);
    case 2:
        func_ov255_021d2478(syswk, 4, 39);
        func_ov255_021d3a48(syswk->app);
        syswk->app->oldCurPos = 39;
        syswk->app->subSeq = 0;
        return 21;
    }
    return 34;
}

// The box's main state while the party is out of the way: waits for a touch or for the cursor to pick something
static int func_ov255_021c6fd4(Box2SysWork *syswk) {
    u32 res;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        syswk->unk1B = 0;
        return func_ov255_021cddf0(syswk, 31);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) && func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE) {
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
        syswk->unk13 = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_ShowCursor(syswk);
        return func_ov255_021cc460(syswk, 30, 1, 91);
    }

    res = func_ov255_021d34d0();
    if (res != 0xffffffff) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            func_0203dac8(&syswk->app->tpx, &syswk->app->tpy);
            if (func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE) {
                func_ov255_021ceed0(syswk, sMenu70f8, 4);
            }
            Box2Main_PokeInfoPut(syswk, res);
            syswk->unk1B = 0;
            return func_ov255_021cd02c(syswk, res);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        func_ov255_021d24f8(syswk, res);
        Box2Main_PokeInfoOff(syswk);
        if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d3954(syswk->app->bgWinFrame);
            func_ov255_021d11a4(syswk, 0);
            func_ov255_021cefa4(syswk->app, 24);
            func_ov255_021bc018(syswk);
            func_ov255_021d3a48(syswk->app);
            return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 45);
        }
        return 45;
    }

    res = func_ov255_021d2f88(syswk);
    switch (res) {
    case 30:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return func_ov255_021cbe58(syswk, 105);
    case 31:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        syswk->app->oldCurPos = 30;
        func_ov255_021d24f8(syswk, 30);
        return func_ov255_021ccf1c(syswk, 1, 45);
    case 32:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        syswk->app->oldCurPos = 30;
        func_ov255_021d24f8(syswk, 30);
        return func_ov255_021ccf68(syswk, 1, 45);
    case 33:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc3b0(syswk, 10, func_ov255_021cbe58(syswk, 48));
    case 34:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->unk28 = 1;
        return func_ov255_021cc460(syswk, 7, 8, 16);
    case 35:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->param->unk28 = 0;
        return func_ov255_021cc460(syswk, 6, 9, 116);
    case 36:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk1B = 0;
        syswk->moveMode = 0;
        return func_ov255_021cc3b0(syswk, 2, func_ov255_021cbe58(syswk, 31));
    case 37:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk13 = 0;
        syswk->getTray = syswk->tray;
        syswk->curRcvPos = 37;
        return func_ov255_021cc3b0(syswk, 3, 89);
    case 38:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk13 = 0;
        syswk->curRcvPos = 38;
        return func_ov255_021cc3b0(syswk, 4, func_ov255_021cbe58(syswk, 90));
    case 39: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos;
        func_ov255_021d32d4(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        syswk->app->oldCurPos = pos;
        return func_ov255_021cc3b0(syswk, 5, 47);
    }
    case 40:
        break;
    case 41:
        syswk->unk1B = 0;
        syswk->moveMode = 1;
        return func_ov255_021cde88(syswk, 31);
    case 42:
        syswk->unk1B = 0;
        syswk->moveMode = 2;
        return func_ov255_021cde88(syswk, 41);
    case 43:
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove);
        syswk->unk13 = 2;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc460(syswk, 30, 1, 91);
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return func_ov255_021ccf1c(syswk, 1, 45);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return func_ov255_021ccf68(syswk, 1, 45);
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
        return func_ov255_021cbed8(syswk, func_ov255_021cbee8(syswk, 45));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos;

            func_ov255_021d32d4(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            syswk->app->oldCurPos = pos;
            return func_ov255_021cc3b0(syswk, 5, 47);
        }
        syswk->param->unk28 = 0;
        return func_ov255_021cc460(syswk, 6, 9, 116);
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
            func_ov255_021ceed0(syswk, sMenu70f8, 4);
            Box2Main_PokeInfoPut(syswk, res);
            func_ov255_021d32d4(syswk->app, BOX2_BOXLIST_POS, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
            syswk->app->oldCurPos = BOX2_BOXLIST_POS;
            return func_ov255_021cd128(syswk, res, 45);
        }
        break;
    }
    return 45;
}

// After a move while the party is out of the way
static int func_ov255_021c74fc(Box2SysWork *syswk) {
    GFL_SndSEPlay(SEQ_SE_SYS_40);
    Box2Main_PokeDataMove(syswk);
    func_ov255_021cd5d8(syswk);
    CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
    syswk->app->oldCurPos = BOX2_BOXLIST_POS;
    func_ov255_021d24f8(syswk, BOX2_BOXLIST_POS);
    func_ov255_021d1af8(syswk, 0, 0, 1, 0);
    func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
    return 45;
}

// Closes the menu while the party is out of the way
static int func_ov255_021c7554(Box2SysWork *syswk) {
    func_ov255_021d3954(syswk->app->bgWinFrame);
    func_ov255_021d11a4(syswk, 0);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    func_ov255_021d3a48(syswk->app);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncCursorMoveFrame, func_ov255_021cbee8(syswk, 45));
}

// Brings the party's frame out from the box's main state
static int func_ov255_021c7594(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        Box2Main_PokeSelectOff(syswk);
        func_ov255_021bc018(syswk);
        func_ov255_021d1348(syswk->app, 0);
        func_ov255_021d0310(syswk, 1, 1);
        func_ov255_021d101c(syswk, 0);
        func_ov255_021d3724(syswk->app->bgWinFrame);
        func_ov255_021d3744(syswk->app->bgWinFrame);
        func_ov255_021cfd34(syswk, 0);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 48);
    case 1:
        syswk->app->subSeq = 0;
        Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
        func_ov255_021d3a64(syswk->app);
        func_ov255_021d2478(syswk, 14, 0);
        return 49;
    }
    return 48;
}

// The party's main state from the box's main state: waits for a touch or for the cursor to pick something
static int func_ov255_021c762c(Box2SysWork *syswk) {
    u32 res;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        syswk->unk1B = 1;
        return func_ov255_021cddf0(syswk, 31);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_START) && func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE) {
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove) + BOX2_PARTY_POS;
        syswk->unk13 = 4;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_ShowCursor(syswk);
        return func_ov255_021cc460(syswk, 30, 1, 91);
    }

    res = func_ov255_021d3504();
    if (res != 0xffffffff) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            func_0203dac8(&syswk->app->tpx, &syswk->app->tpy);
            if (func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE) {
                func_ov255_021ceed0(syswk, sMenu70f8, 4);
            }
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            return func_ov255_021cd1f0(syswk, res + BOX2_PARTY_POS);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        func_ov255_021d24f8(syswk, res);
        Box2Main_PokeInfoOff(syswk);
        if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d3954(syswk->app->bgWinFrame);
            func_ov255_021d11a4(syswk, 0);
            func_ov255_021cefa4(syswk->app, 24);
            func_ov255_021bc018(syswk);
            func_ov255_021d3a64(syswk->app);
            return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 49);
        }
        return 49;
    }

    res = func_ov255_021d30e0(syswk);
    switch (res) {
    case 6:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc3b0(syswk, 10, func_ov255_021cbe58(syswk, 50));
    case 7:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->unk28 = 1;
        return func_ov255_021cc460(syswk, 7, 8, 16);
    case 8:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return func_ov255_021cc460(syswk, 6, 9, func_ov255_021cbe58(syswk, 50));
    case 9:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk1B = 1;
        syswk->moveMode = 0;
        return func_ov255_021cc3b0(syswk, 2, func_ov255_021cbe58(syswk, 31));
    case 10:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk13 = 1;
        syswk->getTray = syswk->tray;
        return func_ov255_021cc3b0(syswk, 3, 89);
    case 11:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->unk13 = 1;
        return func_ov255_021cc3b0(syswk, 4, func_ov255_021cbe58(syswk, 90));
    case 12: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos - BOX2_PARTY_POS;
        func_ov255_021d32d4(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        syswk->app->oldCurPos = pos;
        return func_ov255_021cc3b0(syswk, 5, 51);
    }
    case 13:
        break;
    case 14:
        syswk->unk1B = 1;
        syswk->moveMode = 1;
        return func_ov255_021cde88(syswk, 31);
    case 15:
        syswk->unk1B = 1;
        syswk->moveMode = 2;
        return func_ov255_021cde88(syswk, 41);
    case 16:
        syswk->getTray = syswk->tray;
        syswk->pos = CursorMove_GetPos(syswk->app->cursorMove) + BOX2_PARTY_POS;
        syswk->unk13 = 4;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc460(syswk, 30, 1, 91);
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos - BOX2_PARTY_POS;

            func_ov255_021d32d4(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            syswk->app->oldCurPos = pos;
            return func_ov255_021cc3b0(syswk, 5, 51);
        }
        return func_ov255_021cc460(syswk, 6, 9, func_ov255_021cbe58(syswk, 50));
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
        return func_ov255_021cbed8(syswk, func_ov255_021cbee8(syswk, 49));
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
            func_ov255_021ceed0(syswk, sMenu70f8, 4);
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            func_ov255_021d32d4(syswk->app, 9, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, 9);
            syswk->app->oldCurPos = 9;
            return func_ov255_021cd2e8(syswk, res + BOX2_PARTY_POS, 49);
        }
        break;
    }
    return 49;
}

// Puts the party's frame away, back to the box's main state
static int func_ov255_021c7a88(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        Box2Main_PokeSelectOff(syswk);
        func_ov255_021bc018(syswk);
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        func_ov255_021d3778(syswk->app->bgWinFrame);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 50);
    case 1:
        syswk->app->subSeq = 0;
        func_ov255_021d1348(syswk->app, 1);
        func_ov255_021d0310(syswk, 1, 0);
        func_ov255_021d3a48(syswk->app);
        func_ov255_021d2478(syswk, 13, 33);
        return 45;
    }
    return 50;
}

// Closes the party's menu, from the box's main state
static int func_ov255_021c7b0c(Box2SysWork *syswk) {
    func_ov255_021d3954(syswk->app->bgWinFrame);
    func_ov255_021d11a4(syswk, 0);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    func_ov255_021d3a64(syswk->app);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncCursorMoveFrame, func_ov255_021cbee8(syswk, 49));
}

// After a move in the party, from the box's main state
static int func_ov255_021c7b4c(Box2SysWork *syswk) {
    Box2Main_PokeDataMove(syswk);
    func_ov255_021cd5d8(syswk);
    CursorMove_SetPos(syswk->app->cursorMove, 9);
    syswk->app->oldCurPos = 9;
    func_ov255_021d24f8(syswk, 9);
    func_ov255_021d1af8(syswk, 0, 0, 1, 0);
    func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
    return 49;
}

// Back from a sub process to the box's main state or the party
static int func_ov255_021c7b98(Box2SysWork *syswk) {
    func_ov255_021cefa4(syswk->app, 27);
    func_ov255_021d11a4(syswk, 0);
    func_ov255_021bc018(syswk);
    if (func_0203d554() == FALSE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
    }
    if (func_ov255_021d3858(syswk->app->bgWinFrame) == TRUE) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos - BOX2_PARTY_POS);
        func_ov255_021d24f8(syswk, syswk->pos - BOX2_PARTY_POS);
        func_ov255_021d3a64(syswk->app);
        return 49;
    }
    CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
    func_ov255_021d24f8(syswk, syswk->pos);
    func_ov255_021d3a48(syswk->app);
    return 45;
}

// The box's main state in mode 1: waits for a touch or for the cursor to pick something
static int func_ov255_021c7c10(Box2SysWork *syswk) {
    u32 res;

    res = func_ov255_021d34d0();
    if (res != 0xffffffff) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            func_0203dac8(&syswk->app->tpx, &syswk->app->tpy);
            if (func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE) {
                func_ov255_021ceed0(syswk, sMenu7194, 5);
            }
            Box2Main_PokeInfoPut(syswk, res);
            return func_ov255_021cd098(syswk, res);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        func_ov255_021d24f8(syswk, res);
        Box2Main_PokeInfoOff(syswk);
        if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d3954(syswk->app->bgWinFrame);
            func_ov255_021d11a4(syswk, 0);
            func_ov255_021cefa4(syswk->app, 24);
            func_ov255_021bc018(syswk);
            return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 54);
        }
        return 54;
    }

    res = func_ov255_021d2690(syswk);
    switch (res) {
    case 30:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return func_ov255_021cbe58(syswk, 105);
    case 31:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        func_ov255_021d24f8(syswk, 30);
        return func_ov255_021ccf1c(syswk, 0, 54);
    case 32:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        func_ov255_021d24f8(syswk, 30);
        return func_ov255_021ccf68(syswk, 0, 54);
    case 33:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->unk28 = 1;
        return func_ov255_021cc460(syswk, 7, 8, 16);
    case 34:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->param->unk28 = 0;
        return func_ov255_021cc460(syswk, 6, 9, 116);
    case 35:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc3b0(syswk, 1, func_ov255_021cbe58(syswk, 56));
    case 36:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->getTray = syswk->tray;
        syswk->curRcvPos = 36;
        return func_ov255_021cc3b0(syswk, 2, 89);
    case 37:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc3b0(syswk, 3, func_ov255_021cbe58(syswk, 97));
    case 38:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc3b0(syswk, 4, func_ov255_021cbe58(syswk, 101));
    case 39: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos;
        func_ov255_021d32d4(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        return func_ov255_021cc3b0(syswk, 5, 55);
    }
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return func_ov255_021ccf1c(syswk, 0, 54);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return func_ov255_021ccf68(syswk, 0, 54);
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
        return func_ov255_021cbed8(syswk, func_ov255_021cbee8(syswk, 54));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos;

            func_ov255_021d32d4(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            return func_ov255_021cc3b0(syswk, 5, 55);
        }
        syswk->param->unk28 = 0;
        return func_ov255_021cc460(syswk, 6, 9, 116);
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
            func_ov255_021ceed0(syswk, sMenu7194, 5);
            Box2Main_PokeInfoPut(syswk, res);
            func_ov255_021d32d4(syswk->app, 35, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, 35);
            return func_ov255_021cd128(syswk, res, 54);
        }
        break;
    }
    return 54;
}

// Closes the menu in mode 1
static int func_ov255_021c8024(Box2SysWork *syswk) {
    func_ov255_021d3954(syswk->app->bgWinFrame);
    func_ov255_021d11a4(syswk, 0);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncCursorMoveFrame, func_ov255_021cbee8(syswk, 54));
}

// Mode 1: closes the menu before taking a Pokémon into the party, unless it is full
static int func_ov255_021c8060(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        func_ov255_021d101c(syswk, 0);
        func_ov255_021cefa4(syswk->app, 24);
        func_ov255_021bc018(syswk);
        func_ov255_021d3954(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 56);
    case 1:
        if (PokeParty_GetPkmCount(syswk->param->party) != 6) {
            return func_ov255_021cbe58(syswk, 57);
        }
        GFL_SndSEPlay(SEQ_SE_BEEP);
        func_ov255_021cf1ac(syswk, 0, 2, 24);
        syswk->nextSeq = 14;
        return func_ov255_021cbe58(syswk, BOX2SEQ_TRGWAIT);
    }
    return 56;
}

// Mode 1: takes a Pokémon into the party
static int func_ov255_021c80ec(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        func_ov255_021cfe0c(syswk);
        func_ov255_021d3734(syswk->app->bgWinFrame);
        func_ov255_021d3744(syswk->app->bgWinFrame);
        syswk->unk1A = 2;
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 57);
    case 1:
        syswk->getTray = syswk->tray;
        func_ov255_021d0374(syswk, syswk->pos, 1, 1);
        func_ov255_021cd5b0(syswk);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyInPokeMove, 57);
    case 2:
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        Box2Main_PokeDataMove(syswk);
        func_ov255_021cd5d8(syswk);
        Box2Main_PokeInfoOff(syswk);
        func_ov255_021d3778(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 57);
    case 3:
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
        func_ov255_021d24f8(syswk, syswk->pos);
        func_ov255_021d101c(syswk, 1);
        syswk->pos = BOX2_GET_NONE;
        syswk->app->subSeq = 0;
        return 54;
    }
    return 57;
}

// Mode 1: after a Pokémon was taken into the party
static int func_ov255_021c81d8(Box2SysWork *syswk) {
    Box2PokeMoveWork *work = syswk->app->vfunk.work;
    u32 pos = work->getPos;

    Box2Main_PokeDataMove(syswk);
    func_ov255_021cd5d8(syswk);
    func_ov255_021d1348(syswk->app, 1);
    func_ov255_021d1af8(syswk, 0, 0, 1, 1);
    if (syswk->pos != BOX2_GET_NONE) {
        func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
        pos = 35;
    }
    CursorMove_SetPos(syswk->app->cursorMove, pos);
    func_ov255_021d24f8(syswk, pos);
    return 54;
}

// The party's main state in mode 0: waits for a touch or for the cursor to pick something
static int func_ov255_021c8238(Box2SysWork *syswk) {
    u32 res;

    res = func_ov255_021d3504();
    if (res != 0xffffffff) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            func_0203dac8(&syswk->app->tpx, &syswk->app->tpy);
            if (func_ov255_021d39c0(syswk->app->bgWinFrame) == FALSE) {
                func_ov255_021ceed0(syswk, sMenu7130, 5);
            }
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            return func_ov255_021cd268(syswk, res + BOX2_PARTY_POS);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        func_ov255_021d24f8(syswk, res + 60);
        Box2Main_PokeInfoOff(syswk);
        if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d3954(syswk->app->bgWinFrame);
            func_ov255_021d11a4(syswk, 0);
            func_ov255_021cefa4(syswk->app, 24);
            func_ov255_021bc018(syswk);
            return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 59);
        }
        return 59;
    }

    res = func_ov255_021d2588(syswk);
    switch (res) {
    case 6:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->unk28 = 1;
        return func_ov255_021cc460(syswk, 7, 8, 16);
    case 7:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->param->unk28 = 0;
        return func_ov255_021cc460(syswk, 6, 9, 116);
    case 8:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc3b0(syswk, 1, func_ov255_021cbe58(syswk, 62));
    case 9:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->curRcvPos = 9;
        return func_ov255_021cc3b0(syswk, 2, 89);
    case 10:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc3b0(syswk, 3, func_ov255_021cbe58(syswk, 97));
    case 11:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc3b0(syswk, 4, func_ov255_021cbe58(syswk, 101));
    case 12: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos - BOX2_PARTY_POS;
        func_ov255_021d32d4(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        return func_ov255_021cc3b0(syswk, 5, 60);
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
        return func_ov255_021cbed8(syswk, func_ov255_021cbee8(syswk, 59));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos - BOX2_PARTY_POS;

            func_ov255_021d32d4(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            return func_ov255_021cc3b0(syswk, 5, 60);
        }
        syswk->param->unk28 = 0;
        return func_ov255_021cc460(syswk, 6, 9, 116);
    case CURSORMOVE_NONE:
    case CURSORMOVE_SCROLL_R:
    case CURSORMOVE_SCROLL_L:
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            func_ov255_021ceed0(syswk, sMenu7130, 5);
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            func_ov255_021d32d4(syswk->app, 8, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, 8);
            return func_ov255_021cd2e8(syswk, res + BOX2_PARTY_POS, 59);
        }
        break;
    }
    return 59;
}

// Closes the party's menu in mode 0
static int func_ov255_021c8594(Box2SysWork *syswk) {
    func_ov255_021d3954(syswk->app->bgWinFrame);
    func_ov255_021d11a4(syswk, 0);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncCursorMoveFrame, func_ov255_021cbee8(syswk, 59));
}

// Back to the party's main state in mode 0
static int func_ov255_021c85d0(Box2SysWork *syswk) {
    CursorMove_SetPos(syswk->app->cursorMove, syswk->pos - BOX2_PARTY_POS);
    func_ov255_021d24f8(syswk, syswk->pos - BOX2_PARTY_POS);
    func_ov255_021d101c(syswk, 1);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    return 59;
}

// Mode 0: picks a Pokémon of the party up to deposit it, unless it can't leave the party
static int func_ov255_021c8608(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        func_ov255_021d3954(syswk->app->bgWinFrame);
        func_ov255_021d101c(syswk, 0);
        func_ov255_021cefa4(syswk->app, 24);
        func_ov255_021bc018(syswk);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 62);
    case 1:
        switch (func_ov255_021cdcc8(syswk)) {
        case 0:
            syswk->unk18 = 1;
            func_ov255_021d0374(syswk, syswk->pos, 1, 1);
            func_ov255_021cff58(syswk->app, BOX2_BOXLIST_POS, FALSE);
            func_ov255_021d101c(syswk, 1);
            func_ov255_021d24f8(syswk, syswk->pos - BOX2_PARTY_POS);
            syswk->app->subSeq++;
            return func_ov255_021cbec8(syswk, Box2Main_VFuncPokeMoveGetKey, 62);
        case 1:
            GFL_SndSEPlay(SEQ_SE_BEEP);
            func_ov255_021d11a4(syswk, 0);
            func_ov255_021cf114(syswk, 6, 24);
            syswk->nextSeq = 61;
            syswk->app->subSeq = 0;
            return BOX2SEQ_TRGWAIT;
        case 2:
            GFL_SndSEPlay(SEQ_SE_BEEP);
            func_ov255_021d11a4(syswk, 0);
            func_ov255_021cf1ac(syswk, 0, 5, 24);
            syswk->nextSeq = 61;
            syswk->app->subSeq = 0;
            return BOX2SEQ_TRGWAIT;
        }
        break;
    case 2:
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        func_ov255_021d3778(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 62);
    case 3:
        func_ov255_021d2634(syswk, syswk->pos - BOX2_PARTY_POS);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncCursorMove, 62);
    case 4:
        GFL_HeapFree(syswk->app->vfunk.work);
        func_ov255_021d1348(syswk->app, 1);
        func_ov255_021cf1ac(syswk, 0, 3, 26);
        func_ov255_021d1af8(syswk, 0, 1, 1, 1);
        func_ov255_021d2478(syswk, 1, 0);
        syswk->app->subSeq = 0;
        return 63;
    }
    return 62;
}

// Mode 0: picks the box to deposit the Pokémon in
static int func_ov255_021c8798(Box2SysWork *syswk) {
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
            func_ov255_021cf1ac(syswk, 0, 4, 24);
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
            return func_ov255_021cbe58(syswk, 64);
        }
    case 2:
        CursorMove_SetPos(syswk->app->cursorMove, 0);
    case CURSORMOVE_SCROLL_L:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return func_ov255_021ccf1c(syswk, 0, 63);
    case 3:
        CursorMove_SetPos(syswk->app->cursorMove, 0);
    case CURSORMOVE_SCROLL_R:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return func_ov255_021ccf68(syswk, 0, 63);
    case 4:
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->app->pokePutKey = BOX2_GET_NONE;
        syswk->getTray = syswk->tray;
        syswk->unk1A = 1;
        return func_ov255_021cc460(syswk, 6, 9, func_ov255_021cbe58(syswk, 64));
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
static int func_ov255_021c8920(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        if (syswk->app->pokePutKey == BOX2_GET_NONE) {
            func_ov255_021d2658(syswk, syswk->pos);
            syswk->app->subSeq = 1;
        } else {
            func_ov255_021d2658(syswk, syswk->app->pokePutKey);
            syswk->app->subSeq = 4;
        }
        func_ov255_021d1348(syswk->app, 0);
        func_ov255_021cefa4(syswk->app, 24);
        func_ov255_021bc018(syswk);
        return func_ov255_021cbec8(syswk, Box2Main_VFuncCursorMove, 64);
    case 1:
        GFL_HeapFree(syswk->app->vfunk.work);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        func_ov255_021d3744(syswk->app->bgWinFrame);
        syswk->app->subSeq = 2;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 64);
    case 2:
        func_ov255_021cd5b0(syswk);
        syswk->app->subSeq = 6;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPokeMovePutKey, 64);
    case 3:
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        func_ov255_021d3744(syswk->app->bgWinFrame);
        syswk->app->subSeq = 7;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 64);
    case 4:
        GFL_HeapFree(syswk->app->vfunk.work);
        func_ov255_021cd5b0(syswk);
        func_ov255_021d11a4(syswk, 0);
        syswk->app->subSeq = 5;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyOutPutKey, 64);
    case 5:
        Box2Main_PokeDataMove(syswk);
        func_ov255_021cd5d8(syswk);
        Box2Main_PokeInfoOff(syswk);
        func_ov255_021d0310(syswk, 1, 1);
        func_ov255_021d0310(syswk, 2, 0);
        syswk->app->subSeq = 3;
        break;
    case 6:
        func_ov255_021cd5d8(syswk);
    case 7:
        syswk->unk18 = 0;
        func_ov255_021d11a4(syswk, 0);
        func_ov255_021d1af8(syswk, 0, 0, 1, 1);
        func_ov255_021d2478(syswk, 0, syswk->pos - BOX2_PARTY_POS);
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        syswk->app->subSeq = 0;
        return 59;
    }
    return 64;
}

// Mode 0: closes the message about a full box
static int func_ov255_021c8a9c(Box2SysWork *syswk) {
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    func_ov255_021cf1ac(syswk, 0, 3, 26);
    func_ov255_021d1af8(syswk, 0, 1, 1, 1);
    return 63;
}

// Mode 0: after a deposit
static int func_ov255_021c8ad0(Box2SysWork *syswk) {
    Box2Main_PokeDataMove(syswk);
    func_ov255_021cd5d8(syswk);
    func_ov255_021d1af8(syswk, 0, 0, 1, 1);
    if (syswk->pos == BOX2_GET_NONE) {
        CursorMove_SetPos(syswk->app->cursorMove, 0);
        func_ov255_021d24f8(syswk, 0);
        Box2Main_PokeInfoPut(syswk, BOX2_PARTY_POS);
    } else {
        func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
        CursorMove_SetPos(syswk->app->cursorMove, 8);
        func_ov255_021d24f8(syswk, 8);
    }
    return 59;
}

// The item arrangement's main state: waits for a touch or for the cursor to pick something
static int func_ov255_021c8b38(Box2SysWork *syswk) {
    u32 res;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        res = func_ov255_021cddf0(syswk, 73);
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

    res = func_ov255_021d34d0();
    if (res != 0xffffffff) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            func_0203dac8(&syswk->app->tpx, &syswk->app->tpy);
            Box2Main_PokeInfoPut(syswk, res);
            return func_ov255_021cd5e4(syswk, res);
        }
        Box2Main_PokeInfoOff(syswk);
        if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d32d4(syswk->app, res, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, res);
            return func_ov255_021c8fc4(syswk);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        func_ov255_021d24f8(syswk, res);
        return 67;
    }

    res = func_ov255_021d2c7c(syswk);
    switch (res) {
    case 30:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return func_ov255_021cbe58(syswk, 105);
    case 31:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        func_ov255_021d24f8(syswk, 30);
        return func_ov255_021ccf1c(syswk, 1, 67);
    case 32:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        func_ov255_021d24f8(syswk, 30);
        return func_ov255_021ccf68(syswk, 1, 67);
    case 33:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc3b0(syswk, 10, func_ov255_021cbe58(syswk, 26));
    case 34:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->unk28 = 1;
        return func_ov255_021cc460(syswk, 7, 8, 16);
    case 35:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->param->unk28 = 0;
        return func_ov255_021cc460(syswk, 6, 9, 116);
    case 36:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->moveMode = 0;
        return func_ov255_021cc3b0(syswk, 3, func_ov255_021cbe58(syswk, 73));
    case 37:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->getItem == 0) {
            syswk->unk13 = 0;
            syswk->curRcvPos = 37;
            return func_ov255_021cc3b0(syswk, 4, func_ov255_021cbe58(syswk, 90));
        }
        return func_ov255_021cc3b0(syswk, 4, func_ov255_021cbe58(syswk, 90));
    case 38: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos;
        func_ov255_021d32d4(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        return func_ov255_021cc3b0(syswk, 5, 68);
    }
    case 39:
        if (syswk->moveMode == 1) {
            syswk->moveMode = 0;
            return func_ov255_021cde88(syswk, 73);
        }
        break;
    case 40:
        if (syswk->moveMode == 0) {
            syswk->moveMode = 1;
            res = func_ov255_021cde88(syswk, 73);
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
            return func_ov255_021ccf1c(syswk, 1, 67);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return func_ov255_021ccf68(syswk, 1, 67);
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
        return func_ov255_021cbed8(syswk, func_ov255_021cbee8(syswk, 67));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos;

            func_ov255_021d32d4(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            return func_ov255_021cc3b0(syswk, 5, 68);
        }
        syswk->param->unk28 = 0;
        return func_ov255_021cc460(syswk, 6, 9, 116);
    case CURSORMOVE_NONE:
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return func_ov255_021cd6cc(syswk, res);
        }
        break;
    }
    return 67;
}

// The item arrangement: closes the menu
static int func_ov255_021c8fc4(Box2SysWork *syswk) {
    func_ov255_021d3954(syswk->app->bgWinFrame);
    func_ov255_021d11a4(syswk, 0);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    func_ov255_021d3a48(syswk->app);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncItemArrangeMenuOpen, func_ov255_021cbee8(syswk, 67));
}

// The item arrangement: puts the held item in the bag
static int func_ov255_021c9004(Box2SysWork *syswk) {
    if (func_ov255_021cf628(syswk->app, BOX2_ACTOR_ITEM_ICON) == TRUE) {
        return 69;
    }
    func_ov255_021cdc74(syswk, 0);
    func_ov255_021d0b08(syswk->app, FALSE);
    func_ov255_021cf63c(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
    func_ov255_021cf208(syswk, 3, 24);
    if (syswk->pos < BOX2_PARTY_POS) {
        func_ov255_021d0310(syswk, 0x81, 1);
        func_ov255_021d1348(syswk->app, 1);
    } else {
        func_ov255_021d0310(syswk, 0x82, 1);
    }
    syswk->app->getItem = 0;
    syswk->nextSeq = 71;
    return func_ov255_021cbe58(syswk, BOX2SEQ_TRGWAIT);
}

// The item arrangement: hides the item icon
static int func_ov255_021c907c(Box2SysWork *syswk) {
    func_ov255_021d11a4(syswk, 0);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    if (syswk->pos < BOX2_PARTY_POS) {
        func_ov255_021d3a48(syswk->app);
    } else {
        func_ov255_021d3a64(syswk->app);
    }
    return func_ov255_021cbec8(syswk, Box2Main_VFuncItemIconHide, 71);
}

// The item arrangement: back to the box's or the party's main state
static int func_ov255_021c90b8(Box2SysWork *syswk) {
    int seq;

    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    if (syswk->pos < BOX2_PARTY_POS) {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
        func_ov255_021d24f8(syswk, syswk->pos);
        func_ov255_021d3a48(syswk->app);
        seq = 67;
    } else {
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos - BOX2_PARTY_POS);
        func_ov255_021d24f8(syswk, syswk->pos - BOX2_PARTY_POS);
        func_ov255_021d3a64(syswk->app);
        seq = 80;
    }
    func_ov255_021d101c(syswk, 1);
    func_ov255_021d0f88(syswk, 9, 1);
    return seq;
}

// The item arrangement: back to the main state with the cursor on the menu's button
static int func_ov255_021c9120(Box2SysWork *syswk) {
    u32 pos = 37;

    if (syswk->app->getItem != 0) {
        pos = 36;
    }
    CursorMove_SetPos(syswk->app->cursorMove, pos);
    syswk->app->oldCurPos = pos;
    func_ov255_021d24f8(syswk, pos);
    func_ov255_021d101c(syswk, 1);
    return 67;
}

// The item arrangement: brings the party's frame in
static int func_ov255_021c9160(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        syswk->app->subSeq++;
        func_ov255_021d101c(syswk, 0);
        if (func_ov255_021d3858(syswk->app->bgWinFrame) == TRUE) {
            syswk->unk1E = 1;
        } else {
            syswk->unk1E = 0;
        }
        if (syswk->moveMode == 1) {
            if (syswk->unk1C_4 == 0) {
                syswk->getTray = BOX2_GET_NONE;
                syswk->app->getItem = 0;
                func_ov255_021bc018(syswk);
                Box2Main_PokeInfoPut(syswk, syswk->pos);
            } else {
                if (syswk->app->getItem == 0) {
                    syswk->getTray = BOX2_GET_NONE;
                    syswk->unk1C_4 = 0;
                }
                func_ov255_021cefa4(syswk->app, 24);
                func_ov255_021bc018(syswk);
                func_ov255_021d11a4(syswk, 0);
                func_ov255_021d3954(syswk->app->bgWinFrame);
                return func_ov255_021cbec8(syswk, Box2Main_VFuncItemArrangeFrameMove, 73);
            }
        } else {
            func_ov255_021cefa4(syswk->app, 24);
            func_ov255_021bc018(syswk);
            func_ov255_021d11a4(syswk, 0);
            func_ov255_021d3954(syswk->app->bgWinFrame);
            return func_ov255_021cbec8(syswk, Box2Main_VFuncItemArrangeFrameMove, 73);
        }
    case 1:
        syswk->app->subSeq++;
        if (func_ov255_021d3858(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d37b0(syswk->app->bgWinFrame);
        } else {
            func_ov255_021d3734(syswk->app->bgWinFrame);
            func_ov255_021d3744(syswk->app->bgWinFrame);
            func_ov255_021cfd34(syswk, 0);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 73);
    case 2:
        syswk->app->subSeq = 0;
        func_ov255_021d0310(syswk, 0x81, 1);
        func_ov255_021d0310(syswk, 0x82, 1);
        func_ov255_021d1348(syswk->app, 1);
        func_ov255_021d2478(syswk, 6, syswk->pos);
        func_ov255_021d0ff8(syswk, 6);
        CursorMove_DisablePos(syswk->app->cursorMove, 39);
        CursorMove_DisablePos(syswk->app->cursorMove, 40);
        if (syswk->moveMode == 1) {
            if (syswk->unk1C_4 == 0) {
                func_ov255_021d1af8(syswk, 0, 1, 1, 1);
                func_ov255_021d0f88(syswk, 10, 1);
                return 74;
            }
            func_ov255_021d0f88(syswk, 10, 1);
        } else {
            func_ov255_021d0f88(syswk, 9, 1);
        }
        func_ov255_021d1af8(syswk, 2, 1, 1, 1);
        return func_ov255_021cd858(syswk, syswk->pos);
    }
    return 73;
}

// The item arrangement with the party out: waits for a touch or for the cursor to pick something
static int func_ov255_021c9338(Box2SysWork *syswk) {
    u32 res;

    if (syswk->unk1C_6 == 1) {
        syswk->moveMode = 0;
        syswk->unk1C_6 = 0;
        return func_ov255_021cbe58(syswk, 75);
    }
    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) && syswk->moveMode != 0 && syswk->unk18 == 0) {
        Box2Main_ShowCursor(syswk);
        syswk->moveMode = 0;
        return func_ov255_021cbe58(syswk, 75);
    }
    if (syswk->unk18 == 0) {
        res = func_ov255_021d3514();
        if (res != 0xffffffff) {
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
                Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
                func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
                return func_ov255_021cd798(syswk, res + BOX2_PARTY_POS);
            }
            Box2Main_PokeInfoOff(syswk);
            CursorMove_SetPos(syswk->app->cursorMove, res + BOX2_PARTY_POS);
            return 74;
        }
        res = func_ov255_021d34d0();
        if (res != 0xffffffff) {
            CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
            if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
                Box2Main_PokeInfoPut(syswk, res);
                func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
                return func_ov255_021cd798(syswk, res);
            }
            Box2Main_PokeInfoOff(syswk);
            CursorMove_SetPos(syswk->app->cursorMove, res);
            return 74;
        }
    }

    res = func_ov255_021d2b88(syswk);
    switch (res) {
    case 36:
        if (syswk->unk18 != 0) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        Box2Main_PokeInfoOff(syswk);
        return func_ov255_021cbe58(syswk, 105);
    case 37:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
        func_ov255_021d24f8(syswk, BOX2_BOXLIST_POS);
        if (syswk->unk18 == 0) {
            Box2Main_PokeSelectOff(syswk);
        } else {
            func_ov255_021d052c(syswk);
        }
        return func_ov255_021ccf1c(syswk, 0, 74);
    case 38:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        CursorMove_SetPos(syswk->app->cursorMove, BOX2_BOXLIST_POS);
        func_ov255_021d24f8(syswk, BOX2_BOXLIST_POS);
        if (syswk->unk18 == 0) {
            Box2Main_PokeSelectOff(syswk);
        } else {
            func_ov255_021d052c(syswk);
        }
        return func_ov255_021ccf68(syswk, 0, 74);
    case 39:
    case 40:
        break;
    case 41:
        if (syswk->unk18 != 0) {
            break;
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = 0;
        return func_ov255_021cc460(syswk, 6, 9, func_ov255_021cbe58(syswk, 75));
    case 42:
        if (syswk->moveMode == 0 || syswk->unk18 != 0) {
            break;
        }
        syswk->moveMode = 0;
        return func_ov255_021cbe58(syswk, 75);
    case 43:
    case 44:
    case 45:
        break;
    case CURSORMOVE_CANCEL:
        if (syswk->unk18 == 1) {
            return func_ov255_021cd910(syswk, BOX2_GET_NONE);
        }
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->moveMode = 0;
        return func_ov255_021cc460(syswk, 6, 9, func_ov255_021cbe58(syswk, 75));
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BOXLIST_POS) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return func_ov255_021ccf1c(syswk, 0, 74);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == BOX2_BOXLIST_POS) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return func_ov255_021ccf68(syswk, 0, 74);
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
        return func_ov255_021cbed8(syswk, func_ov255_021cbee8(syswk, 74));
    case CURSORMOVE_NONE:
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
        break;
    default:
        if (syswk->unk18 == 1) {
            return func_ov255_021cd910(syswk, res);
        }
        return func_ov255_021cd858(syswk, res);
    }
    return 74;
}

// The item arrangement: puts the party's frame away
static int func_ov255_021c96b8(Box2SysWork *syswk) {
    u8 pos;

    switch (syswk->app->subSeq) {
    case 0:
        Box2Main_PokeInfoOff(syswk);
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        if (syswk->unk1E == 0) {
            func_ov255_021d3778(syswk->app->bgWinFrame);
        } else {
            func_ov255_021d1348(syswk->app, 0);
            func_ov255_021d0310(syswk, 1, 1);
            func_ov255_021d37c4(syswk->app->bgWinFrame);
        }
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 75);
    case 1:
        syswk->app->subSeq = 0;
        func_ov255_021d0ff8(syswk, 6);
        func_ov255_021d0f88(syswk, 9, 1);
        func_ov255_021d1af8(syswk, 0, 0, 1, 1);
        if (syswk->unk1E == 0) {
            pos = CursorMove_GetPos(syswk->app->cursorMove);
            if (pos >= BOX2_BOXLIST_POS) {
                pos = 0;
            } else if (pos >= BOX2_PARTY_POS) {
                pos = 0;
            }
            func_ov255_021d2478(syswk, 7, pos);
            Box2Main_PokeInfoPut(syswk, pos);
            func_ov255_021d3a48(syswk->app);
            return 67;
        } else {
            u8 cur;

            pos = 0;
            cur = CursorMove_GetPos(syswk->app->cursorMove);
            if (cur < BOX2_BOXLIST_POS && cur >= BOX2_PARTY_POS) {
                pos = cur - BOX2_PARTY_POS;
            }
            func_ov255_021d2478(syswk, 8, pos);
            Box2Main_PokeInfoPut(syswk, pos + BOX2_PARTY_POS);
            func_ov255_021d3a64(syswk->app);
            return 80;
        }
    }
    return 75;
}

// The item arrangement: an item was dropped on a Pokémon by the cursor
static int func_ov255_021c97c8(Box2SysWork *syswk) {
    Box2ItemMoveWork *work;
    u16 item;

    if (syswk->moveMode == 0) {
        syswk->unk1C_6 = 1;
    }
    func_ov255_021d11a4(syswk, 0);
    work = syswk->app->vfunk.work;
    if (work->putPos == syswk->pos && (syswk->getTray == BOX2_GET_NONE || syswk->getTray == syswk->tray)) {
        u16 setPos = work->setPos;

        func_ov255_021cdb5c(syswk);
        func_ov255_021d0350(syswk->app, syswk->pos, FALSE);
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
        func_ov255_021d1af8(syswk, 0, 1, 1, 1);
        if (PML_ItemIsMail(syswk->app->getItem) == TRUE && setPos != syswk->pos) {
            GFL_SndSEPlay(SEQ_SE_BEEP);
            func_ov255_021cf074(syswk, 24);
            syswk->nextSeq = 78;
            return BOX2SEQ_TRGWAIT;
        }
        return 74;
    }

    if (syswk->getTray == syswk->tray || syswk->pos >= BOX2_PARTY_POS) {
        func_ov255_021d0350(syswk->app, syswk->pos, TRUE);
    }
    func_ov255_021d0350(syswk->app, work->putPos, FALSE);
    item = Box2Main_GetPokeParam(syswk, work->putPos, syswk->tray, PKM_PARAM_ITEM, NULL);
    Box2Main_SetPokeParam(syswk, work->putPos, syswk->tray, PKM_PARAM_ITEM, syswk->app->getItem);
    if (Box2Main_PokeItemFormChange(syswk, Box2Main_GetBoxPkm(syswk, syswk->tray, work->putPos)) == TRUE) {
        Box2Main_RecalcPartyStats(syswk, work->putPos);
        func_ov255_021cfc20(syswk, syswk->tray, work->putPos, syswk->app->pokeIconId[work->putPos]);
    }
    Box2Main_PokeInfoPut(syswk, work->putPos);
    CursorMove_SetPos(syswk->app->cursorMove, work->putPos);

    syswk->app->getItem = item;
    Box2Main_SetPokeParam(syswk, syswk->pos, syswk->getTray, PKM_PARAM_ITEM, syswk->app->getItem);
    if (Box2Main_PokeItemFormChange(syswk, Box2Main_GetBoxPkm(syswk, syswk->getTray, syswk->pos)) == TRUE) {
        Box2Main_RecalcPartyStats(syswk, syswk->pos);
        if (syswk->getTray == BOX2_GET_NONE || syswk->getTray == syswk->tray) {
            func_ov255_021cfc20(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
    }

    if (syswk->app->getItem == 0) {
        syswk->pos = work->putPos;
        func_ov255_021cdb5c(syswk);
        func_ov255_021d1af8(syswk, 0, 1, 1, 1);
        return 74;
    }
    func_ov255_021d0a94(syswk->app, syswk->app->getItem);
    func_ov255_021d0b08(syswk->app, TRUE);
    func_ov255_021cf5e4(syswk->app, BOX2_ACTOR_ITEM_ICON, 1);
    func_ov255_021cf63c(syswk->app, BOX2_ACTOR_ITEM_ICON, TRUE);
    func_ov255_021d0bc8(syswk->app);
    GFL_SndSEPlay(SEQ_SE_SYS_39);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncItemIconPutBack, 77);
}

// The item arrangement: an item was dropped on a Pokémon by touch
static int func_ov255_021c99a8(Box2SysWork *syswk) {
    Box2ItemMoveWork *work;
    u8 oldPos;
    u16 setPos;
    u16 item;
    u8 pos;

    if (syswk->moveMode == 0) {
        syswk->unk1C_6 = 1;
    }
    work = syswk->app->vfunk.work;
    func_ov255_021d0350(syswk->app, work->putPos, FALSE);
    oldPos = syswk->pos;
    setPos = work->setPos;
    syswk->pos = work->putPos;
    func_ov255_021cdb5c(syswk);
    item = syswk->app->getItem;
    syswk->app->getItem = 0;
    if (PML_ItemIsMail(item) == TRUE && setPos != oldPos) {
        GFL_SndSEPlay(SEQ_SE_BEEP);
        func_ov255_021d101c(syswk, 0);
        func_ov255_021cf074(syswk, 24);
        syswk->nextSeq = 78;
        return BOX2SEQ_TRGWAIT;
    }
    func_ov255_021d101c(syswk, 1);
    pos = CursorMove_GetPos(syswk->app->cursorMove);
    if (pos < BOX2_BOXLIST_POS) {
        Box2Main_PokeInfoPut(syswk, pos);
    }
    func_ov255_021d1af8(syswk, 0, 1, 1, 1);
    return 74;
}

// The item arrangement: closes the message about mail
static int func_ov255_021c9a44(Box2SysWork *syswk) {
    func_ov255_021cefb8(syswk->app, 24);
    func_ov255_021bc018(syswk);
    if (CursorMove_IsCursorVisible(syswk->app->cursorMove) == TRUE) {
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, TRUE);
    }
    return 74;
}

// The item arrangement: an item was dropped on a Pokémon by the cursor, with the party out
static int func_ov255_021c9a70(Box2SysWork *syswk) {
    u16 item;

    if (syswk->moveMode == 0) {
        syswk->unk1C_6 = 1;
    }
    if (syswk->app->pokePutKey == syswk->app->getItemInitPos
        && (syswk->getTray == BOX2_GET_NONE || syswk->getTray == syswk->tray)) {
        syswk->app->getItem = 0;
        func_ov255_021d1af8(syswk, 0, 1, 1, 1);
        return 74;
    }

    if (syswk->getTray == syswk->tray || syswk->app->getItemInitPos >= BOX2_PARTY_POS) {
        func_ov255_021d0350(syswk->app, syswk->app->getItemInitPos, TRUE);
    }
    func_ov255_021d0350(syswk->app, syswk->app->pokePutKey, FALSE);
    item = Box2Main_GetPokeParam(syswk, syswk->app->pokePutKey, syswk->tray, PKM_PARAM_ITEM, NULL);
    Box2Main_SetPokeParam(syswk, syswk->app->pokePutKey, syswk->tray, PKM_PARAM_ITEM, syswk->app->getItem);
    if (Box2Main_PokeItemFormChange(syswk, Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->app->pokePutKey)) == TRUE) {
        Box2Main_RecalcPartyStats(syswk, syswk->app->pokePutKey);
        func_ov255_021cfc20(syswk, syswk->tray, syswk->app->pokePutKey,
                            syswk->app->pokeIconId[syswk->app->pokePutKey]);
    }
    Box2Main_PokeInfoPut(syswk, syswk->app->pokePutKey);

    syswk->app->getItem = item;
    Box2Main_SetPokeParam(syswk, syswk->app->getItemInitPos, syswk->getTray, PKM_PARAM_ITEM, syswk->app->getItem);
    if (Box2Main_PokeItemFormChange(syswk, Box2Main_GetBoxPkm(syswk, syswk->getTray, syswk->app->getItemInitPos))
        == TRUE) {
        Box2Main_RecalcPartyStats(syswk, syswk->app->getItemInitPos);
        if (syswk->getTray == BOX2_GET_NONE || syswk->getTray == syswk->tray) {
            func_ov255_021cfc20(syswk, syswk->tray, syswk->app->getItemInitPos,
                                syswk->app->pokeIconId[syswk->app->getItemInitPos]);
        }
    }

    if (syswk->app->getItem == 0) {
        func_ov255_021d1af8(syswk, 0, 1, 1, 1);
        return 74;
    }
    func_ov255_021cdb34(syswk);
    ((Box2ItemMoveWork *)syswk->app->vfunk.work)->putPos = syswk->app->pokePutKey;
    func_ov255_021d0a94(syswk->app, syswk->app->getItem);
    func_ov255_021d0b08(syswk->app, TRUE);
    func_ov255_021cf5e4(syswk->app, BOX2_ACTOR_ITEM_ICON, 1);
    func_ov255_021cf63c(syswk->app, BOX2_ACTOR_ITEM_ICON, TRUE);
    func_ov255_021d0bc8(syswk->app);
    GFL_SndSEPlay(SEQ_SE_SYS_39);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncItemIconPutBack, 77);
}

// The item arrangement in the party: waits for a touch or for the cursor to pick something
static int func_ov255_021c9c34(Box2SysWork *syswk) {
    u32 res;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_SELECT) {
        res = func_ov255_021cddf0(syswk, 73);
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

    res = func_ov255_021d3504();
    if (res != 0xffffffff) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_SYS_39);
            func_0203dac8(&syswk->app->tpx, &syswk->app->tpy);
            Box2Main_PokeInfoPut(syswk, res + BOX2_PARTY_POS);
            return func_ov255_021cd994(syswk, res + BOX2_PARTY_POS);
        }
        Box2Main_PokeInfoOff(syswk);
        if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d32d4(syswk->app, (u8)res, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, res + BOX2_PARTY_POS);
            return func_ov255_021c9ffc(syswk);
        }
        CursorMove_SetPos(syswk->app->cursorMove, res);
        func_ov255_021d24f8(syswk, res);
        return 80;
    }

    res = func_ov255_021d2dac(syswk);
    switch (res) {
    case 6:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc3b0(syswk, 10, func_ov255_021cbe58(syswk, 28));
    case 7:
        GFL_SndSEPlay(SEQ_SE_CLOSE1);
        syswk->param->unk28 = 1;
        return func_ov255_021cc460(syswk, 7, 8, 16);
    case 8:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return func_ov255_021cc460(syswk, 6, 9, func_ov255_021cbe58(syswk, 28));
    case 9:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->moveMode = 0;
        return func_ov255_021cc3b0(syswk, 3, func_ov255_021cbe58(syswk, 73));
    case 10:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->getItem == 0) {
            syswk->unk13 = 1;
            syswk->curRcvPos = 10;
            return func_ov255_021cc3b0(syswk, 4, func_ov255_021cbe58(syswk, 90));
        }
        return func_ov255_021cc3b0(syswk, 4, func_ov255_021cbe58(syswk, 90));
    case 11: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos - BOX2_PARTY_POS;
        func_ov255_021d32d4(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        return func_ov255_021cc3b0(syswk, 5, 81);
    }
    case 12:
        if (syswk->moveMode == 1) {
            syswk->moveMode = 0;
            return func_ov255_021cde88(syswk, 73);
        }
        break;
    case 13:
        if (syswk->moveMode == 0) {
            syswk->moveMode = 1;
            res = func_ov255_021cde88(syswk, 73);
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
        return func_ov255_021cbed8(syswk, func_ov255_021cbee8(syswk, 80));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos - BOX2_PARTY_POS;

            func_ov255_021d32d4(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            return func_ov255_021cc3b0(syswk, 5, 81);
        }
        return func_ov255_021cc460(syswk, 6, 9, func_ov255_021cbe58(syswk, 28));
    case CURSORMOVE_NONE:
    case CURSORMOVE_SCROLL_R:
    case CURSORMOVE_SCROLL_L:
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res + BOX2_PARTY_POS, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return func_ov255_021cda6c(syswk, res + BOX2_PARTY_POS);
        }
        break;
    }
    return 80;
}

// The item arrangement in the party: closes the menu
static int func_ov255_021c9ffc(Box2SysWork *syswk) {
    func_ov255_021d3954(syswk->app->bgWinFrame);
    func_ov255_021d11a4(syswk, 0);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    func_ov255_021d3a64(syswk->app);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncItemArrangeMenuOpen, func_ov255_021cbee8(syswk, 80));
}

// The item arrangement in the party: an item was dropped
static int func_ov255_021ca03c(Box2SysWork *syswk) {
    Box2ItemMoveWork *work = syswk->app->vfunk.work;

    if (syswk->app->getItem == 0) {
        func_ov255_021cdb5c(syswk);
        func_ov255_021cf208(syswk, 0, 24);
        CursorMove_SetPos(syswk->app->cursorMove, 10);
        syswk->app->oldCurPos = 10;
        func_ov255_021d24f8(syswk, 10);
        func_ov255_021d101c(syswk, 1);
        return 80;
    }
    if (work->putPos == syswk->pos) {
        u16 setPos = work->setPos;

        func_ov255_021cdb5c(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, 9);
        syswk->app->oldCurPos = 9;
        func_ov255_021d24f8(syswk, 9);
        if (PML_ItemIsMail(syswk->app->getItem) == TRUE && setPos != syswk->pos) {
            func_ov255_021d0350(syswk->app, syswk->pos, FALSE);
            func_ov255_021cf208(syswk, 7, 24);
            GFL_SndSEPlay(SEQ_SE_BEEP);
            syswk->nextSeq = 14;
            return func_ov255_021cbe58(syswk, BOX2SEQ_TRGWAIT);
        }
        func_ov255_021cf208(syswk, 1, 24);
        func_ov255_021d101c(syswk, 1);
        return 80;
    }
    if (PML_ItemIsMail(syswk->app->getItem) == TRUE) {
        func_ov255_021cdb5c(syswk);
        GFL_SndSEPlay(SEQ_SE_BEEP);
        func_ov255_021d0350(syswk->app, syswk->pos, FALSE);
        func_ov255_021cf208(syswk, 7, 24);
        GFL_SndSEPlay(SEQ_SE_BEEP);
        CursorMove_SetPos(syswk->app->cursorMove, 9);
        syswk->app->oldCurPos = 9;
        func_ov255_021d24f8(syswk, 9);
        func_ov255_021d101c(syswk, 1);
        syswk->nextSeq = 14;
        return func_ov255_021cbe58(syswk, BOX2SEQ_TRGWAIT);
    }
    func_ov255_021d0350(syswk->app, work->putPos, FALSE);
    func_ov255_021d0350(syswk->app, syswk->pos, TRUE);
    func_ov255_021d11a4(syswk, 0);
    func_ov255_021d0b08(syswk->app, TRUE);
    func_ov255_021cf5e4(syswk->app, BOX2_ACTOR_ITEM_ICON, 2);
    return 83;
}

// The item arrangement in the party: swaps the items once the icon has landed
static int func_ov255_021ca194(Box2SysWork *syswk) {
    Box2ItemMoveWork *work;
    u16 item;

    if (func_ov255_021cf628(syswk->app, BOX2_ACTOR_ITEM_ICON) == TRUE) {
        return 83;
    }
    func_ov255_021d0b08(syswk->app, FALSE);
    func_ov255_021cf63c(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
    work = syswk->app->vfunk.work;
    item = Box2Main_GetPokeParam(syswk, work->putPos, syswk->tray, PKM_PARAM_ITEM, NULL);
    Box2Main_SetPokeParam(syswk, work->putPos, syswk->tray, PKM_PARAM_ITEM, syswk->app->getItem);
    if (Box2Main_PokeItemFormChange(syswk, Box2Main_GetBoxPkm(syswk, syswk->tray, work->putPos)) == TRUE) {
        Box2Main_RecalcPartyStats(syswk, work->putPos);
        func_ov255_021cfc20(syswk, syswk->tray, work->putPos, syswk->app->pokeIconId[work->putPos]);
    }
    Box2Main_PokeInfoPut(syswk, work->putPos);

    syswk->app->getItem = item;
    Box2Main_SetPokeParam(syswk, syswk->pos, syswk->getTray, PKM_PARAM_ITEM, syswk->app->getItem);
    if (syswk->getTray == BOX2_GET_NONE || syswk->getTray == syswk->tray) {
        if (Box2Main_PokeItemFormChange(syswk, Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos)) == TRUE) {
            Box2Main_RecalcPartyStats(syswk, syswk->pos);
            func_ov255_021cfc20(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
    }

    if (syswk->app->getItem == 0) {
        syswk->pos = work->putPos;
        func_ov255_021cdb5c(syswk);
        func_ov255_021d3a64(syswk->app);
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos - BOX2_PARTY_POS);
        syswk->app->oldCurPos = syswk->pos - BOX2_PARTY_POS;
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
        return 80;
    }
    GFL_SndSEPlay(SEQ_SE_SYS_39);
    func_ov255_021d0a94(syswk->app, syswk->app->getItem);
    func_ov255_021d0b08(syswk->app, TRUE);
    func_ov255_021cf5e4(syswk->app, BOX2_ACTOR_ITEM_ICON, 1);
    func_ov255_021cf63c(syswk->app, BOX2_ACTOR_ITEM_ICON, TRUE);
    func_ov255_021d0bc8(syswk->app);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncItemArrangeMenuCancel, func_ov255_021cbe58(syswk, 84));
}

// The item arrangement in the party: puts the item back where it was taken
static int func_ov255_021ca314(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_SYS_40);
        func_ov255_021d0350(syswk->app, ((Box2ItemMoveWork *)syswk->app->vfunk.work)->putPos, FALSE);
        func_ov255_021d11a4(syswk, 0);
        func_ov255_021d0b08(syswk->app, TRUE);
        func_ov255_021cf5e4(syswk->app, BOX2_ACTOR_ITEM_ICON, 2);
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos - BOX2_PARTY_POS);
        syswk->app->oldCurPos = syswk->pos - BOX2_PARTY_POS;
        func_ov255_021cdb5c(syswk);
        syswk->app->subSeq++;
        break;
    case 1:
        if (func_ov255_021cf628(syswk->app, BOX2_ACTOR_ITEM_ICON) != TRUE) {
            Box2Main_PokeInfoPut(syswk, syswk->pos);
            CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
            func_ov255_021d3a64(syswk->app);
            return 80;
        }
        break;
    }
    return 84;
}

// Mode 5's main state: waits for a touch or for the cursor to pick a Pokémon
static int func_ov255_021ca3b4(Box2SysWork *syswk) {
    u32 res = func_ov255_021d3204(syswk);

    switch (res) {
    case 30:
        Box2Main_PokeSelectOff(syswk);
        break;
    case 31:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        func_ov255_021d24f8(syswk, 30);
        return func_ov255_021ccf1c(syswk, 0, 85);
    case 32:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeSelectOff(syswk);
        CursorMove_SetPos(syswk->app->cursorMove, 30);
        func_ov255_021d24f8(syswk, 30);
        return func_ov255_021ccf68(syswk, 0, 85);
    case 33:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        syswk->param->unk28 = 0;
        syswk->param->tray = BOX2_GET_NONE;
        syswk->param->position = BOX2_GET_NONE;
        return func_ov255_021cc460(syswk, 6, 9, 116);
    case 34:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc3b0(syswk, 4, func_ov255_021cbe58(syswk, 87));
    case 35: {
        u8 pos;

        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        pos = syswk->pos;
        func_ov255_021d32d4(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, pos);
        return func_ov255_021cc3b0(syswk, 5, 88);
    }
    case CURSORMOVE_SCROLL_L:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return func_ov255_021ccf1c(syswk, 0, 85);
        }
        break;
    case CURSORMOVE_SCROLL_R:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 30) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            Box2Main_PokeSelectOff(syswk);
            return func_ov255_021ccf68(syswk, 0, 85);
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
        return func_ov255_021cbed8(syswk, func_ov255_021cbee8(syswk, 85));
    }
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
            u8 pos = syswk->pos;

            func_ov255_021d32d4(syswk->app, pos, CursorMove_GetPos(syswk->app->cursorMove));
            CursorMove_SetPos(syswk->app->cursorMove, pos);
            return func_ov255_021cc3b0(syswk, 5, 88);
        }
        syswk->param->unk28 = 0;
        syswk->param->tray = BOX2_GET_NONE;
        syswk->param->position = BOX2_GET_NONE;
        return func_ov255_021cc460(syswk, 6, 9, 116);
    case CURSORMOVE_NONE:
    case CURSORMOVE_UNK_7:
    case CURSORMOVE_UNK_8:
        break;
    default:
        if (Box2Main_GetPokeParam(syswk, res, syswk->tray, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            syswk->pos = res;
            func_ov255_021ceed0(syswk, sMenu70cc, 2);
            Box2Main_PokeInfoPut(syswk, res);
            func_ov255_021d1048(syswk);
            CursorMove_SetPos(syswk->app->cursorMove, 34);
            if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
                return func_ov255_021ca6c8(syswk);
            }
            func_ov255_021d32d4(syswk->app, 34, res);
            func_ov255_021d390c(syswk->app->bgWinFrame);
            return func_ov255_021cbec8(syswk, Box2Main_VFuncCursorMoveFrame, func_ov255_021cbee8(syswk, 86));
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        Box2Main_PokeInfoPut(syswk, res);
        if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d3954(syswk->app->bgWinFrame);
            func_ov255_021d11a4(syswk, 0);
            func_ov255_021cefa4(syswk->app, 24);
            func_ov255_021bc018(syswk);
            return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 85);
        }
        break;
    }
    return 85;
}

// Mode 5: the picked Pokémon's menu was already open
static int func_ov255_021ca6c8(Box2SysWork *syswk) {
    func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
    return 85;
}

// Mode 5: picks the Pokémon, unless it is an egg or can't be picked
static int func_ov255_021ca6d8(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        func_ov255_021d101c(syswk, 0);
        func_ov255_021d3954(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 87);
    case 1:
        if (Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_IS_EGG, NULL) != 0) {
            func_ov255_021cf270(syswk, 4, 24);
            syswk->nextSeq = 14;
            return func_ov255_021cbe58(syswk, BOX2SEQ_TRGWAIT);
        }
        if (Box2Main_IsSpeciesFlagged(syswk, syswk->pos) == FALSE) {
            func_ov255_021cf270(syswk, 6, 27);
            syswk->nextSeq = 14;
            return func_ov255_021cbe58(syswk, BOX2SEQ_TRGWAIT);
        }
        func_ov255_021d0310(syswk, 1, 1);
        func_ov255_021d0350(syswk->app, syswk->pos, FALSE);
        func_ov255_021cf2a4(syswk);
        syswk->app->subSeq = 0;
        return func_ov255_021cbef0(syswk, 5);
    }
    return 87;
}

// Mode 5: closes the menu
static int func_ov255_021ca79c(Box2SysWork *syswk) {
    func_ov255_021d3954(syswk->app->bgWinFrame);
    func_ov255_021d11a4(syswk, 0);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncCursorMoveFrame, func_ov255_021cbee8(syswk, 85));
}

static int func_ov255_021ca7d8(Box2SysWork *syswk) {
    return func_ov255_021cc4e0(syswk, 0);
}

// Takes the held item into the bag, unless the Pokémon is an egg or holds mail
static int func_ov255_021ca7e4(Box2SysWork *syswk) {
    u32 item;

    switch (syswk->app->subSeq) {
    case 0:
        if (syswk->param->mode == 4 && syswk->param->unk14 == 1) {
            func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
            func_ov255_021d3954(syswk->app->bgWinFrame);
            syswk->app->subSeq = 3;
            return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 90);
        }
        if (Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_IS_EGG, NULL) == 0) {
            item = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);
            if (item == 0) {
                return func_ov255_021cc4e0(syswk, 1);
            }
            func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
            func_ov255_021d0f88(syswk, 9, 0);
            func_ov255_021d1348(syswk->app, 0);
            if (PML_ItemIsMail(item) == TRUE) {
                func_ov255_021d3954(syswk->app->bgWinFrame);
                syswk->app->subSeq = 1;
                return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 90);
            }
            if (syswk->pos < BOX2_PARTY_POS) {
                func_ov255_021d0310(syswk, 1, 1);
            } else {
                func_ov255_021d0310(syswk, 2, 1);
            }
            func_ov255_021d0350(syswk->app, syswk->pos, FALSE);
            syswk->app->unkA550 = 1;
            func_ov255_021d38e0(syswk->app->bgWinFrame);
            func_ov255_021cf044(syswk, item, 24);
            if (syswk->param->mode == 3) {
                return func_ov255_021cbef0(syswk, 2);
            }
            return func_ov255_021cbef0(syswk, 0);
        }
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        func_ov255_021d3954(syswk->app->bgWinFrame);
        syswk->app->subSeq = 2;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 90);
    case 1:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        func_ov255_021cf068(syswk, 24);
        syswk->nextSeq = 14;
        return func_ov255_021cbe58(syswk, BOX2SEQ_TRGWAIT);
    case 2:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        func_ov255_021cf208(syswk, 5, 24);
        syswk->nextSeq = 14;
        return func_ov255_021cbe58(syswk, BOX2SEQ_TRGWAIT);
    case 3:
        GFL_SndSEPlay(SEQ_SE_BEEP);
        func_ov255_021cf2cc(syswk, 1);
        syswk->nextSeq = 53;
        return BOX2SEQ_TRGWAIT;
    }
    return 90;
}

static int func_ov255_021ca9a4(Box2SysWork *syswk) {
    return func_ov255_021cc4e0(syswk, 3);
}

// Back from the summary: shows the Pokémon and fades in
static int func_ov255_021ca9b0(Box2SysWork *syswk) {
    if (syswk->getTray != BOX2_GET_NONE) {
        Box2Main_PokeInfoPutCore(syswk, syswk->getTray, syswk->pos);
    } else {
        Box2Main_PokeInfoPut(syswk, syswk->pos);
    }
    func_ov255_021d0ff8(syswk, 6);
    func_ov255_021d0f88(syswk, syswk->moveMode + 9, 1);
    switch (syswk->param->mode) {
    case 0:
        syswk->app->msgNextSeq = func_ov255_021cc894(syswk);
        break;
    case 1:
        syswk->app->msgNextSeq = func_ov255_021cc864(syswk);
        break;
    case 2:
        syswk->app->msgNextSeq = func_ov255_021cc8dc(syswk);
        break;
    case 4:
        syswk->app->msgNextSeq = func_ov255_021ccae4(syswk);
        break;
    }
    return Box2Seq_StartWait(syswk);
}

// Back from the bag: puts the cursor on the box list and fades in
static int func_ov255_021caa30(Box2SysWork *syswk) {
    Box2Main_PokeInfoOff(syswk);
    switch (syswk->param->mode) {
    case 1:
        func_ov255_021d2478(syswk, 2, BOX2_PARTY_POS);
        syswk->app->oldCurPos = BOX2_PARTY_POS;
        syswk->app->msgNextSeq = 54;
        break;
    case 2:
        func_ov255_021d3a48(syswk->app);
        func_ov255_021d2478(syswk, 3, BOX2_PARTY_POS);
        syswk->app->oldCurPos = BOX2_PARTY_POS;
        syswk->app->msgNextSeq = 17;
        break;
    case 3:
        func_ov255_021d0310(syswk, 0x81, 1);
        func_ov255_021d3a48(syswk->app);
        func_ov255_021d2478(syswk, 7, BOX2_PARTY_POS);
        syswk->app->oldCurPos = BOX2_PARTY_POS;
        syswk->app->msgNextSeq = 67;
        break;
    case 4:
        func_ov255_021d3a48(syswk->app);
        func_ov255_021d2478(syswk, 13, BOX2_PARTY_POS);
        syswk->app->oldCurPos = BOX2_PARTY_POS;
        syswk->app->msgNextSeq = 45;
        break;
    }
    return Box2Seq_StartWait(syswk);
}

// Back from the name entry: fades in
static int func_ov255_021caadc(Box2SysWork *syswk) {
    switch (syswk->param->mode) {
    case 2:
        syswk->app->msgNextSeq = func_ov255_021cc50c(syswk);
        break;
    case 3:
        syswk->app->msgNextSeq = func_ov255_021cc608(syswk);
        break;
    case 4:
        syswk->app->msgNextSeq = func_ov255_021cc768(syswk);
        break;
    }
    return Box2Seq_StartWait(syswk);
}

// Back from the bag after giving an item: shows the Pokémon and fades in
static int func_ov255_021cab14(Box2SysWork *syswk) {
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
    func_ov255_021d0ff8(syswk, 6);
    func_ov255_021d0f88(syswk, syswk->moveMode + 9, 1);
    switch (syswk->param->mode) {
    case 2:
        syswk->app->msgNextSeq = func_ov255_021ccc28(syswk);
        break;
    case 4:
        syswk->app->msgNextSeq = func_ov255_021cce14(syswk);
        break;
    }
    return Box2Seq_StartWait(syswk);
}

// Says which item was given
static int func_ov255_021cab94(Box2SysWork *syswk) {
    func_ov255_021cf0b0(syswk, syswk->subRet, 24);
    if (syswk->param->mode == 3) {
        syswk->nextSeq = 70;
    } else {
        syswk->nextSeq = 14;
    }
    return func_ov255_021cbe58(syswk, BOX2SEQ_TRGWAIT);
}

// Opens the markings
static int func_ov255_021cabbc(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        func_ov255_021d0f88(syswk, 9, 0);
        func_ov255_021cefa4(syswk->app, 24);
        func_ov255_021bc018(syswk);
        func_ov255_021d3954(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 97);
    case 1:
        syswk->app->unkA554 = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_MARKINGS, NULL);
        Box2Main_MarkingPutMain(syswk, syswk->app->unkA554);
        func_ov255_021d3a80(syswk->app->bgWinFrame);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, func_ov255_021c05cc, 97);
    case 2:
        func_ov255_021d2478(syswk, 9, 0);
        func_ov255_021d1348(syswk->app, 0);
        func_ov255_021d0310(syswk, 1, 1);
        func_ov255_021d0310(syswk, 2, 1);
        func_ov255_021d0350(syswk->app, syswk->pos, FALSE);
        func_ov255_021cf108(syswk, 24);
        return func_ov255_021cbe58(syswk, 98);
    }
    return 97;
}

// The markings: toggles a mark, or sets or cancels them
static int func_ov255_021cacac(Box2SysWork *syswk) {
    switch (CursorMove_Update(syswk->app->cursorMove)) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        func_ov255_021cdd04(syswk, 0);
        break;
    case 1:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        func_ov255_021cdd04(syswk, 1);
        break;
    case 2:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        func_ov255_021cdd04(syswk, 2);
        break;
    case 3:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        func_ov255_021cdd04(syswk, 3);
        break;
    case 4:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        func_ov255_021cdd04(syswk, 4);
        break;
    case 5:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        func_ov255_021cdd04(syswk, 5);
        break;
    case 6: {
        u8 mark = syswk->app->unkA554;

        Box2Main_SetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_MARKINGS, mark);
        Box2Main_MarkingPutSub(syswk, mark);
        func_ov255_021d0640(syswk, syswk->tray, syswk->pos);
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc3c0(syswk, 0, 99);
    }
    case 7:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return func_ov255_021cc3c0(syswk, 1, 99);
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return func_ov255_021cc3c0(syswk, 1, 99);
    case CURSORMOVE_CURSOR_MOVE:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return func_ov255_021cbed8(syswk, 100);
    case CURSORMOVE_CURSOR_ON:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        break;
    }
    return 98;
}

// Closes the markings
static int func_ov255_021cadcc(Box2SysWork *syswk) {
    switch (syswk->param->mode) {
    case 0:
        func_ov255_021d2478(syswk, 0, 10);
        break;
    case 1:
        func_ov255_021d1348(syswk->app, 1);
        func_ov255_021d2478(syswk, 2, 37);
        break;
    case 2:
    default:
        if (syswk->pos < BOX2_PARTY_POS) {
            func_ov255_021d1348(syswk->app, 1);
            func_ov255_021d2478(syswk, 3, 39);
        } else {
            func_ov255_021d2478(syswk, 5, 12);
        }
        break;
    }
    func_ov255_021d101c(syswk, 0);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    if (syswk->pos < BOX2_PARTY_POS) {
        func_ov255_021d0310(syswk, 1, 0);
    }
    func_ov255_021d0310(syswk, 2, 0);
    func_ov255_021d3aa4(syswk->app->bgWinFrame);
    GFL_SndSEPlay(SEQ_SE_SYS_42);
    return func_ov255_021cbec8(syswk, func_ov255_021c05cc, func_ov255_021cbe58(syswk, 14));
}

static int func_ov255_021cae74(Box2SysWork *syswk) {
    GFL_HeapFree(syswk->app->vfunk.work);
    return 98;
}

// Releasing: checks the Pokémon can be released
static int func_ov255_021cae84(Box2SysWork *syswk) {
    BoxPkm *pkm;
    u16 species;
    u8 form;
    u32 msg;

    switch (syswk->app->subSeq) {
    case 0:
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        func_ov255_021d0f88(syswk, 9, 0);
        func_ov255_021d3954(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 101);
    case 1:
        if (syswk->pos >= BOX2_PARTY_POS) {
            u32 partyPos;

            func_ov255_021d0310(syswk, 2, 1);
            func_ov255_021d0350(syswk->app, syswk->pos, FALSE);
            partyPos = syswk->pos - BOX2_PARTY_POS;
            if (Box2Main_BattlePokeCheck(syswk, partyPos) == FALSE) {
                GFL_SndSEPlay(SEQ_SE_BEEP);
                func_ov255_021cf114(syswk, 6, 24);
                syswk->app->unkA550 = 1;
                syswk->nextSeq = 14;
                return func_ov255_021cbe58(syswk, BOX2SEQ_TRGWAIT);
            }
            if (PML_ItemIsMail(PokeParty_GetParam(PokeParty_GetPkm(syswk->param->party, partyPos), PKM_PARAM_ITEM, NULL))
                == TRUE) {
                GFL_SndSEPlay(SEQ_SE_BEEP);
                func_ov255_021cf1ac(syswk, 0, 5, 24);
                syswk->app->unkA550 = 1;
                syswk->nextSeq = 14;
                return func_ov255_021cbe58(syswk, BOX2SEQ_TRGWAIT);
            }
        } else {
            func_ov255_021d1348(syswk->app, 0);
            func_ov255_021d0310(syswk, 1, 1);
            func_ov255_021d0350(syswk->app, syswk->pos, FALSE);
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
            func_ov255_021cf114(syswk, msg, 24);
            syswk->app->unkA550 = 1;
            syswk->nextSeq = 14;
            return func_ov255_021cbe58(syswk, BOX2SEQ_TRGWAIT);
        }
        func_ov255_021cf114(syswk, 0, 24);
        return func_ov255_021cbe58(syswk, func_ov255_021cbef0(syswk, 1));
    }
    return 101;
}

// Releasing: checks the moves the Pokémon takes away
static int func_ov255_021cb020(Box2SysWork *syswk) {
    Box2Main_PokeFreeWazaCheck(syswk);
    if (func_ov255_021d0184(syswk->app->subWork) == FALSE) {
        if (((Box2PokeFreeWork *)syswk->app->subWork)->checkFlag != 0) {
            return func_ov255_021cbe58(syswk, 104);
        }
        func_ov255_021d0214(syswk->app->subWork);
        Box2Main_PokeFreeExit(syswk);
        return func_ov255_021cbe58(syswk, 103);
    }
    return 102;
}

// Releasing: says goodbye and removes the Pokémon
static int func_ov255_021cb068(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        func_ov255_021cf114(syswk, 1, 24);
        syswk->nextSeq = 103;
        syswk->app->subSeq++;
        return BOX2SEQ_TRGWAIT;
    case 1:
        func_ov255_021cf114(syswk, 2, 24);
        syswk->nextSeq = 103;
        syswk->app->subSeq++;
        return BOX2SEQ_TRGWAIT;
    case 2:
        Box2Main_ClearPokeData(syswk, syswk->tray, syswk->pos);
        func_ov255_021cefa4(syswk->app, 24);
        func_ov255_021bc018(syswk);
        Box2Main_PokeInfoOff(syswk);
        if (syswk->pos < BOX2_PARTY_POS) {
            func_ov255_021d1570(syswk, syswk->tray);
            func_ov255_021d15f4(syswk, syswk->tray);
            if (syswk->param->mode == 1) {
                syswk->nextSeq = 54;
            } else {
                syswk->nextSeq = 17;
                func_ov255_021d3a48(syswk->app);
            }
            CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
            func_ov255_021d24f8(syswk, syswk->pos);
            func_ov255_021d101c(syswk, 1);
            func_ov255_021d1348(syswk->app, 1);
            func_ov255_021d0310(syswk, 1, 0);
            func_ov255_021d0f88(syswk, 9, 1);
            syswk->pos = BOX2_GET_NONE;
            syswk->app->subSeq = 0;
            return syswk->nextSeq;
        }
        func_ov255_021cd5b0(syswk);
        syswk->unk1A = 1;
        func_ov255_021d0310(syswk, 2, 0);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyPokeFreeSort, 103);
    case 3:
        syswk->app->subSeq = 0;
        func_ov255_021cd5d8(syswk);
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        CursorMove_SetPos(syswk->app->cursorMove, syswk->pos - BOX2_PARTY_POS);
        func_ov255_021d24f8(syswk, syswk->pos - BOX2_PARTY_POS);
        func_ov255_021d101c(syswk, 1);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->param->mode == 0) {
            return 59;
        }
        func_ov255_021d0f88(syswk, 9, 1);
        func_ov255_021d3a64(syswk->app);
        return 27;
    }
    return 103;
}

// Releasing: says the Pokémon's move can't be lost, and puts it back
static int func_ov255_021cb1d8(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        if (func_ov255_021d01c8(syswk->app->subWork) == FALSE) {
            func_ov255_021d0228(syswk->app->subWork);
            Box2Main_PokeFreeExit(syswk);
            func_ov255_021d1048(syswk);
            func_ov255_021cf114(syswk, 4, 24);
            syswk->nextSeq = 104;
            syswk->app->subSeq++;
            return BOX2SEQ_TRGWAIT;
        }
        break;
    case 1:
        func_ov255_021cf114(syswk, 5, 24);
        syswk->nextSeq = 104;
        syswk->app->subSeq++;
        return BOX2SEQ_TRGWAIT;
    case 2:
        syswk->app->subSeq = 0;
        return func_ov255_021cc040(syswk);
    }
    return 104;
}

// Opens the box menu
static int func_ov255_021cb258(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        syswk->app->subSeq++;
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
        func_ov255_021d0f88(syswk, 9, 0);
        func_ov255_021d1348(syswk->app, 0);
        if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
            if (func_ov255_021cf658(syswk->app, BOX2_ACTOR_ITEM_ICON) == TRUE) {
                func_ov255_021cf63c(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
            }
            func_ov255_021d11a4(syswk, 0);
            func_ov255_021cefa4(syswk->app, 24);
            func_ov255_021bc018(syswk);
            func_ov255_021d3954(syswk->app->bgWinFrame);
            return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 105);
        }
        if (func_ov255_021d3b48(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d1ac8(syswk, 0, 0);
            func_ov255_021d3a58(syswk->app);
            return func_ov255_021cdc54(syswk, 105);
        }
        if (func_ov255_021d3834(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d3a74(syswk->app);
            func_ov255_021d3778(syswk->app->bgWinFrame);
            return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyFrameMove, 105);
        }
        func_ov255_021d3a58(syswk->app);
    case 1:
        func_ov255_021ceed0(syswk, sMenu70e8, 4);
        func_ov255_021d390c(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 105);
    case 2:
        syswk->app->subSeq = 0;
        func_ov255_021d0310(syswk, 1, 1);
        func_ov255_021cf17c(syswk, 0, 24);
        func_ov255_021d2478(syswk, 10, 0);
        syswk->moveMode = 0;
        func_ov255_021d0ff8(syswk, 6);
        return 106;
    }
    return 105;
}

// The box menu: picks a choice
static int func_ov255_021cb3a0(Box2SysWork *syswk) {
    switch (CursorMove_Update(syswk->app->cursorMove)) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc3b0(syswk, 2, func_ov255_021cbe58(syswk, 108));
    case 1:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        syswk->app->unkA55C = 0;
        return func_ov255_021cc3b0(syswk, 3, func_ov255_021cbe58(syswk, 112));
    case 2:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        return func_ov255_021cc3b0(syswk, 4, 115);
    case 3:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return func_ov255_021cc3b0(syswk, 5, func_ov255_021cbe58(syswk, 107));
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return func_ov255_021cc3b0(syswk, 5, func_ov255_021cbe58(syswk, 107));
    case CURSORMOVE_CURSOR_MOVE:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return func_ov255_021cbed8(syswk, func_ov255_021cbee8(syswk, 106));
    }
    return 106;
}

// Closes the box menu
static int func_ov255_021cb488(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        func_ov255_021d101c(syswk, 0);
        func_ov255_021cefa4(syswk->app, 24);
        func_ov255_021bc018(syswk);
        if (syswk->param->mode == 2 || syswk->param->mode == 4) {
            func_ov255_021d1af8(syswk, 0, 0, 1, 0);
        } else {
            func_ov255_021d1af8(syswk, 0, 0, 1, 1);
        }
        func_ov255_021d3954(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 107);
    case 1:
        syswk->app->subSeq = 0;
        func_ov255_021d1348(syswk->app, 1);
        func_ov255_021d0f88(syswk, 9, 1);
        if (syswk->param->mode == 2) {
            func_ov255_021d0310(syswk, 1, 0);
            func_ov255_021d3a48(syswk->app);
            func_ov255_021d2478(syswk, 3, BOX2_PARTY_POS);
            return 17;
        }
        if (syswk->param->mode == 1) {
            func_ov255_021d0310(syswk, 1, 0);
            func_ov255_021d2478(syswk, 2, BOX2_PARTY_POS);
            return 54;
        }
        if (syswk->param->mode == 3) {
            func_ov255_021d0310(syswk, 0x81, 1);
            func_ov255_021d3a48(syswk->app);
            func_ov255_021d2478(syswk, 7, BOX2_PARTY_POS);
            return 67;
        }
        if (syswk->param->mode == 4) {
            func_ov255_021d0310(syswk, 1, 0);
            func_ov255_021d3a48(syswk->app);
            func_ov255_021d2478(syswk, 13, BOX2_PARTY_POS);
            return 45;
        }
    }
    return 107;
}

// Opens the box jump
static int func_ov255_021cb5b0(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        syswk->app->unkA55A = 1;
        func_ov255_021d101c(syswk, 0);
        func_ov255_021cefa4(syswk->app, 24);
        func_ov255_021bc018(syswk);
        func_ov255_021d3954(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 108);
    case 1:
        if (func_ov255_021d3b48(syswk->app->bgWinFrame) == FALSE) {
            syswk->trayScroll = syswk->tray;
        }
        syswk->app->subSeq++;
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        return func_ov255_021cdc38(syswk, 108);
    case 2:
        syswk->app->subSeq = 0;
        func_ov255_021d1ac8(syswk, 0, 1);
        func_ov255_021d2478(syswk, 12, 1);
        syswk->app->oldCurPos = 1;
        func_ov255_021cf17c(syswk, 1, 25);
        func_ov255_021d1af8(syswk, 0, 1, 1, 1);
        return 109;
    }
    return 108;
}

// The box jump: picks a box in the list
static int func_ov255_021cb67c(Box2SysWork *syswk) {
    u32 x, y;
    u32 res;

    if (func_ov255_021d3554(&x, &y) == TRUE) {
        syswk->app->tpy = y;
        syswk->nextSeq = 109;
        func_ov255_021cdc04(syswk);
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
        return 15;
    }

    res = CursorMove_Update(syswk->app->cursorMove);
    switch (res) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        func_ov255_021bc09c(syswk, -1);
        return func_ov255_021cbec8(syswk, Box2Main_VFuncBoxMoveScrollRight, 109);
    case 5:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        func_ov255_021bc09c(syswk, 1);
        return func_ov255_021cbec8(syswk, Box2Main_VFuncBoxMoveScrollLeft, 109);
    case 1:
    case 2:
    case 3:
    case 4:
        syswk->app->unkA55F = syswk->trayScroll + res - 1;
        if (syswk->app->unkA55F >= syswk->trayMax) {
            syswk->app->unkA55F -= syswk->trayMax;
        }
        if (syswk->app->unkA55F != syswk->tray) {
            func_ov255_021d1ac8(syswk, 0, 0);
            return func_ov255_021cdba4(syswk, 111);
        }
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        func_ov255_021d1ac8(syswk, res - 1, 1);
        break;
    case 6:
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return func_ov255_021cc460(syswk, 6, 9, func_ov255_021cbe58(syswk, 110));
    case CURSORMOVE_CURSOR_MOVE:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return func_ov255_021cbed8(syswk, func_ov255_021cbee8(syswk, 109));
    case CURSORMOVE_UNK_8:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 1) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            func_ov255_021bc09c(syswk, -1);
            return func_ov255_021cbec8(syswk, Box2Main_VFuncBoxMoveScrollRight, 109);
        }
        break;
    case CURSORMOVE_UNK_7:
        if (CursorMove_GetPos(syswk->app->cursorMove) == 4) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            func_ov255_021bc09c(syswk, 1);
            return func_ov255_021cbec8(syswk, Box2Main_VFuncBoxMoveScrollLeft, 109);
        }
        break;
    }
    return 109;
}

// Closes the box jump
static int func_ov255_021cb82c(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        syswk->app->unkA55A = 0;
        func_ov255_021d101c(syswk, 0);
        func_ov255_021cefa4(syswk->app, 25);
        func_ov255_021bc018(syswk);
        func_ov255_021d1ac8(syswk, 0, 0);
        GFL_SndSEPlay(SEQ_SE_SYS_42);
        syswk->app->subSeq++;
        return func_ov255_021cdc54(syswk, 110);
    case 1:
        syswk->app->subSeq = 0;
        func_ov255_021d1348(syswk->app, 1);
        if (syswk->param->mode == 2 || syswk->param->mode == 4) {
            func_ov255_021d1af8(syswk, 0, 0, 1, 0);
        } else {
            func_ov255_021d1af8(syswk, 0, 0, 1, 1);
        }
        func_ov255_021d0f88(syswk, 9, 1);
        if (syswk->param->mode == 2) {
            func_ov255_021d0310(syswk, 1, 0);
            func_ov255_021d3a48(syswk->app);
            func_ov255_021d2478(syswk, 3, BOX2_PARTY_POS);
            return 17;
        }
        if (syswk->param->mode == 1) {
            func_ov255_021d0310(syswk, 1, 0);
            func_ov255_021d2478(syswk, 2, BOX2_PARTY_POS);
            return 54;
        }
        if (syswk->param->mode == 3) {
            func_ov255_021d0310(syswk, 0x81, 1);
            func_ov255_021d3a48(syswk->app);
            func_ov255_021d2478(syswk, 7, BOX2_PARTY_POS);
            return 67;
        }
        if (syswk->param->mode == 4) {
            func_ov255_021d0310(syswk, 1, 0);
            func_ov255_021d3a48(syswk->app);
            func_ov255_021d2478(syswk, 13, BOX2_PARTY_POS);
            return 45;
        }
    }
    return 110;
}

// The box jump: back from showing a box
static int func_ov255_021cb960(Box2SysWork *syswk) {
    func_ov255_021d1ac8(syswk, CursorMove_GetPos(syswk->app->cursorMove) - 1, 1);
    return 109;
}

// Opens a wallpaper menu: the themes, or a theme's wallpapers
static int func_ov255_021cb97c(Box2SysWork *syswk) {
    u32 all;

    switch (syswk->app->subSeq) {
    case 0:
        func_ov255_021d101c(syswk, 0);
        func_ov255_021cefa4(syswk->app, 24);
        func_ov255_021bc018(syswk);
        func_ov255_021d3954(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 112);
    case 1:
        if (syswk->app->unkA55C == 0) {
            if (func_02007da4(syswk->param->boxes, 1) == TRUE) {
                func_ov255_021ceed0(syswk, sMenu71a8, 6);
            } else {
                func_ov255_021ceed0(syswk, sMenu711c, 5);
            }
        } else if (syswk->app->unkA55C == 1) {
            func_ov255_021ceed0(syswk, sMenu7144, 5);
        } else if (syswk->app->unkA55C == 2) {
            func_ov255_021ceed0(syswk, sMenu7158, 5);
        } else if (syswk->app->unkA55C == 3) {
            func_ov255_021ceed0(syswk, sMenu716c, 5);
        } else if (syswk->app->unkA55C == 4) {
            func_ov255_021ceed0(syswk, sMenu7180, 5);
        } else if (syswk->app->unkA55C == 5) {
            if (func_02007da4(syswk->param->boxes, 2) == TRUE) {
                func_ov255_021ceed0(syswk, sMenu71c0, 6);
            } else {
                func_ov255_021ceed0(syswk, sMenu7108, 5);
            }
        } else if (syswk->app->unkA55C == 6) {
            func_ov255_021ceed0(syswk, sMenu71d8, 6);
        }
        func_ov255_021d390c(syswk->app->bgWinFrame);
        syswk->app->subSeq++;
        return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, 112);
    case 2:
        syswk->app->subSeq = 0;
        if (syswk->app->unkA55C == 0) {
            func_ov255_021cf17c(syswk, 2, 24);
        } else {
            func_ov255_021cf17c(syswk, 3, 24);
        }
        all = FALSE;
        if (syswk->app->unkA55C == 0) {
            if (func_02007da4(syswk->param->boxes, 1) == FALSE) {
                all = TRUE;
            }
        } else if (syswk->app->unkA55C == 5) {
            if (func_02007da4(syswk->param->boxes, 2) == FALSE) {
                all = TRUE;
            }
        } else if (syswk->app->unkA55C != 6) {
            all = TRUE;
        }
        if (all == TRUE) {
            func_ov255_021d2478(syswk, 11, 1);
            CursorMove_DisablePos(syswk->app->cursorMove, 0);
        } else {
            func_ov255_021d2478(syswk, 11, 0);
        }
        return 113;
    }
    return 112;
}

// The wallpaper menu: picks a theme or a wallpaper
static int func_ov255_021cbb00(Box2SysWork *syswk) {
    switch (CursorMove_Update(syswk->app->cursorMove)) {
    case 0:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->unkA55C == 0) {
            syswk->app->unkA55C = 5;
            return func_ov255_021cc3b0(syswk, 0, func_ov255_021cbe58(syswk, 112));
        }
        return func_ov255_021cdd24(syswk, 0);
    case 1:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->unkA55C == 0) {
            syswk->app->unkA55C = 1;
            return func_ov255_021cc3b0(syswk, 1, func_ov255_021cbe58(syswk, 112));
        }
        return func_ov255_021cdd24(syswk, 1);
    case 2:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->unkA55C == 0) {
            syswk->app->unkA55C = 2;
            return func_ov255_021cc3b0(syswk, 2, func_ov255_021cbe58(syswk, 112));
        }
        return func_ov255_021cdd24(syswk, 2);
    case 3:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->unkA55C == 0) {
            syswk->app->unkA55C = 3;
            return func_ov255_021cc3b0(syswk, 3, func_ov255_021cbe58(syswk, 112));
        }
        return func_ov255_021cdd24(syswk, 3);
    case 4:
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (syswk->app->unkA55C == 0) {
            syswk->app->unkA55C = 4;
            return func_ov255_021cc3b0(syswk, 4, func_ov255_021cbe58(syswk, 112));
        }
        if (syswk->app->unkA55C == 5 && func_02007da4(syswk->param->boxes, 2) == TRUE) {
            syswk->app->unkA55C = 6;
            return func_ov255_021cc3b0(syswk, 4, func_ov255_021cbe58(syswk, 112));
        }
        if (syswk->app->unkA55C == 6) {
            syswk->app->unkA55C = 5;
            return func_ov255_021cc3b0(syswk, 4, func_ov255_021cbe58(syswk, 112));
        }
        return func_ov255_021cdd24(syswk, 4);
    case 5:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (syswk->app->unkA55C == 0) {
            return func_ov255_021cc3b0(syswk, 5, func_ov255_021cbe58(syswk, 107));
        }
        syswk->app->unkA55C = 0;
        return func_ov255_021cc3b0(syswk, 5, func_ov255_021cbe58(syswk, 112));
    case CURSORMOVE_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        if (syswk->app->unkA55C == 0) {
            return func_ov255_021cc3b0(syswk, 5, func_ov255_021cbe58(syswk, 107));
        }
        syswk->app->unkA55C = 0;
        return func_ov255_021cc3b0(syswk, 5, func_ov255_021cbe58(syswk, 112));
    case CURSORMOVE_CURSOR_MOVE:
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return func_ov255_021cbed8(syswk, func_ov255_021cbee8(syswk, 113));
    }
    return 113;
}

// Changes the box's wallpaper behind a fade to white
static int func_ov255_021cbd2c(Box2SysWork *syswk) {
    switch (syswk->app->subSeq) {
    case 0:
        func_ov255_021d101c(syswk, 0);
        PaletteFade_LoadFromVRAM(syswk->app->palFade, 0, 0, 0x200);
        PaletteFade_StartFade(syswk->app->palFade, 1, 0xc000, 0, 0, 16, 0x7fff, GFL_VBlankGetTCBMgr());
        syswk->nextSeq = 114;
        syswk->app->subSeq++;
        return BOX2SEQ_PALETTE_FADE;
    case 1:
        Box2Main_WallPaperChange(syswk, syswk->app->wallpaperPos);
        func_02007b00(syswk->param->boxes, syswk->tray, syswk->app->wallpaperPos);
        func_ov255_021d1570(syswk, syswk->tray);
        syswk->app->subSeq++;
        break;
    case 2:
        PaletteFade_StartFade(syswk->app->palFade, 1, 0xc000, 0, 16, 0, 0x7fff, GFL_VBlankGetTCBMgr());
        syswk->nextSeq = 114;
        syswk->app->subSeq++;
        return BOX2SEQ_PALETTE_FADE;
    case 3:
        func_ov255_021d101c(syswk, 1);
        syswk->app->subSeq = 0;
        return 113;
    }
    return 114;
}

static int func_ov255_021cbe1c(Box2SysWork *syswk) {
    return func_ov255_021cc4e0(syswk, 2);
}

// Opens the box's name entry
static int func_ov255_021cbe28(Box2SysWork *syswk) {
    func_ov255_021cf63c(syswk->app, BOX2_ACTOR_CURSOR, FALSE);
    func_ov255_021d0f88(syswk, 9, 0);
    func_ov255_021cf028(syswk, 24);
    func_ov255_021cdd80(syswk);
    return func_ov255_021cbef0(syswk, 4);
}

// Goes to another sequence from its first step
static int func_ov255_021cbe58(Box2SysWork *syswk, int seq) {
    syswk->app->subSeq = 0;
    return seq;
}

// Wipes out, then goes to nextSeq
int func_ov255_021cbe68(Box2SysWork *syswk, int nextSeq) {
    GFL_WipeSet(0, 1, 1, 0, 6, 1, HEAPID_BOX2_APP);
    syswk->app->wipeSeq = nextSeq;
    return BOX2SEQ_WIPE;
}

// Wipes in, then goes to nextSeq
static int func_ov255_021cbe98(Box2SysWork *syswk, int nextSeq) {
    GFL_WipeSet(0, 0, 0, 0, 6, 1, HEAPID_BOX2_APP);
    syswk->app->wipeSeq = nextSeq;
    return BOX2SEQ_WIPE;
}

// Runs a function each frame until it ends, then goes to nextSeq
int func_ov255_021cbec8(Box2SysWork *syswk, Box2VFunc func, int nextSeq) {
    syswk->app->vfuncNextSeq = nextSeq;
    Box2Main_VFuncSet(syswk->app, func);
    return BOX2SEQ_VFUNC;
}

// Runs the requested function each frame until it ends, then goes to nextSeq
static int func_ov255_021cbed8(Box2SysWork *syswk, int nextSeq) {
    syswk->app->vfuncNextSeq = nextSeq;
    Box2Main_VFuncReqSet(syswk->app);
    return BOX2SEQ_VFUNC;
}

static int func_ov255_021cbee8(Box2SysWork *syswk, int seq) {
    syswk->nextSeq = seq;
    return 13;
}

// Opens a yes/no question
static int func_ov255_021cbef0(Box2SysWork *syswk, u32 type) {
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
static int func_ov255_021cbf18(Box2SysWork *syswk) {
    u32 item = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);

    if (BagSave_AddItem(syswk->param->bag, item, 1, HEAPID_BOX2_APP) == TRUE) {
        BoxPkm *pkm = Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos);

        func_ov255_021cf080(syswk, item, 24);
        Box2Main_SetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, 0);
        if (Box2Main_PokeItemFormChange(syswk, pkm) == TRUE) {
            Box2Main_RecalcPartyStats(syswk, syswk->pos);
            func_ov255_021cfc20(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
        Box2Main_PokeInfoRewrite(syswk, syswk->pos);
        func_ov255_021d0640(syswk, syswk->tray, syswk->pos);
    } else {
        GFL_SndSEPlay(SEQ_SE_BEEP);
        func_ov255_021cf0a4(syswk, 24);
    }
    if (func_0203d554() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
        func_ov255_021d101c(syswk, 0);
    }
    syswk->nextSeq = 14;
    return func_ov255_021cbe58(syswk, BOX2SEQ_TRGWAIT);
}

// Releasing: starts the release
static int func_ov255_021cbfe8(Box2SysWork *syswk) {
    func_ov255_021d11a4(syswk, 0);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    Box2Main_PokeFreeCreate(syswk);
    func_ov255_021d013c(syswk->app->subWork);
    if (func_0203d554() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
        func_ov255_021d101c(syswk, 0);
    }
    return 102;
}

// Releasing: the release was cancelled
static int func_ov255_021cc040(Box2SysWork *syswk) {
    func_ov255_021d11a4(syswk, 1);
    func_ov255_021cefb8(syswk->app, 24);
    func_ov255_021bc018(syswk);
    if (syswk->pos < BOX2_PARTY_POS) {
        func_ov255_021d1348(syswk->app, 1);
        func_ov255_021d0310(syswk, 1, 0);
    } else {
        func_ov255_021d0310(syswk, 2, 0);
    }
    if (func_0203d554() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
        func_ov255_021d101c(syswk, 0);
    }
    return func_ov255_021cbec8(syswk, Box2Main_VFuncFrameMove, func_ov255_021cbe58(syswk, 14));
}

// The item arrangement: puts the held item in the bag
static int func_ov255_021cc0b4(Box2SysWork *syswk) {
    if (func_0203d554() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
        func_ov255_021d101c(syswk, 0);
    }
    if (BagSave_AddItem(syswk->param->bag, syswk->app->getItem, 1, HEAPID_BOX2_APP) == TRUE) {
        BoxPkm *pkm = Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos);

        Box2Main_SetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, 0);
        if (Box2Main_PokeItemFormChange(syswk, pkm) == TRUE) {
            Box2Main_RecalcPartyStats(syswk, syswk->pos);
            func_ov255_021cfc20(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        func_ov255_021d11a4(syswk, 0);
        func_ov255_021cefa4(syswk->app, 24);
        func_ov255_021bc018(syswk);
        func_ov255_021d3a48(syswk->app);
        func_ov255_021d0b08(syswk->app, TRUE);
        func_ov255_021cf5e4(syswk->app, BOX2_ACTOR_ITEM_ICON, 2);
        return 69;
    }
    GFL_SndSEPlay(SEQ_SE_BEEP);
    func_ov255_021cf208(syswk, 6, 24);
    syswk->nextSeq = 14;
    return func_ov255_021cbe58(syswk, BOX2SEQ_TRGWAIT);
}

// Leaves the box
static int func_ov255_021cc198(Box2SysWork *syswk) {
    GFL_SndSEPlay(SEQ_SE_PC_LOGOFF);
    Box2Main_UpdateChatter(syswk);
    syswk->nextSeq = BOX2SEQ_END;
    return func_ov255_021cbe98(syswk, 1);
}

// Closes a message and goes back to the mode's main state
static int func_ov255_021cc1bc(Box2SysWork *syswk) {
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    if (func_0203d554() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
    }
    func_ov255_021d0f88(syswk, 9, 1);
    switch (syswk->param->mode) {
    case 0:
        func_ov255_021d0310(syswk, 2, 0);
        return 59;
    case 1:
        func_ov255_021d1348(syswk->app, 1);
        func_ov255_021d0310(syswk, 1, 0);
        return 54;
    case 2:
        if (func_ov255_021d3858(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d0310(syswk, 2, 0);
            func_ov255_021d3a64(syswk->app);
            return 27;
        }
        func_ov255_021d1348(syswk->app, 1);
        func_ov255_021d0310(syswk, 1, 0);
        func_ov255_021d3a48(syswk->app);
        return 17;
    case 3:
        if (func_ov255_021d3858(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d0310(syswk, 0x82, 0);
            func_ov255_021d3a64(syswk->app);
            return 80;
        }
        func_ov255_021d1348(syswk->app, 1);
        func_ov255_021d0310(syswk, 0x81, 0);
        func_ov255_021d3a48(syswk->app);
        return 67;
    case 4:
        if (func_ov255_021d3858(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d0310(syswk, 2, 0);
            func_ov255_021d3a64(syswk->app);
            return 49;
        }
        func_ov255_021d1348(syswk->app, 1);
        func_ov255_021d0310(syswk, 1, 0);
        func_ov255_021d3a48(syswk->app);
        return 45;
    case 5:
        func_ov255_021d1348(syswk->app, 1);
        func_ov255_021d0310(syswk, 0x81, 1);
        return 85;
    }
    return 17;
}

// Mode 5: picks the Pokémon and leaves the box
static int func_ov255_021cc308(Box2SysWork *syswk) {
    Box2Main_UpdateChatter(syswk);
    syswk->param->tray = syswk->tray;
    syswk->param->position = syswk->pos;
    syswk->nextSeq = BOX2SEQ_END;
    return func_ov255_021cbe98(syswk, 1);
}

// Mode 5: the Pokémon wasn't picked
static int func_ov255_021cc330(Box2SysWork *syswk) {
    func_ov255_021cefa4(syswk->app, 27);
    func_ov255_021bc018(syswk);
    CursorMove_SetPos(syswk->app->cursorMove, syswk->pos);
    func_ov255_021d0310(syswk, 0x81, 1);
    func_ov255_021d11a4(syswk, 0);
    if (func_0203d554() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
    }
    return 85;
}

static int func_ov255_021cc380(Box2SysWork *syswk) {
    if (func_0203d554() == TRUE) {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, FALSE);
    } else {
        CursorMove_SetCursorVisible(syswk->app->cursorMove, TRUE);
        func_ov255_021d101c(syswk, 0);
    }
    return func_ov255_021c2dc0(syswk);
}

// Animates a frame's button, then goes to seq
static int func_ov255_021cc3b0(Box2SysWork *syswk, u32 frame, int seq) {
    Box2Main_SetFrameButtonAnm(syswk, frame);
    syswk->nextSeq = seq;
    return BOX2SEQ_BUTTON_ANM;
}

// Animates one of the markings' buttons, then goes to seq
static int func_ov255_021cc3c0(Box2SysWork *syswk, u32 button, int seq) {
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
static int func_ov255_021cc460(Box2SysWork *syswk, u32 id, u32 pal, int seq) {
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
static int func_ov255_021cc4e0(Box2SysWork *syswk, u32 type) {
    if (CursorMove_IsCursorVisible(syswk->app->cursorMove) == TRUE) {
        func_0203d564(FALSE);
    } else {
        func_0203d564(TRUE);
    }
    syswk->nextSeq = BOX2SEQ_SUBPROC_CALL;
    syswk->subProcType = type;
    return func_ov255_021cbe98(syswk, 1);
}

// Back from the bag after giving an item, in the arrangement
static int func_ov255_021cc50c(Box2SysWork *syswk) {
    int seq;

    switch (syswk->unk13) {
    case 0:
        func_ov255_021d3a48(syswk->app);
        seq = 17;
        break;
    case 1:
        func_ov255_021cdb68(syswk);
        func_ov255_021d0310(syswk, 1, 1);
        func_ov255_021d1348(syswk->app, 0);
        func_ov255_021d2478(syswk, 5, 11);
        func_ov255_021d3a64(syswk->app);
        seq = 27;
        break;
    }
    func_ov255_021d1048(syswk);
    if (syswk->subRet == 0) {
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        func_ov255_021ceed0(syswk, sMenu71f0, 6);
        func_ov255_021d38bc(syswk->app->bgWinFrame);
        func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
        return seq;
    }
    if (PML_ItemIsMail(syswk->subRet) == FALSE) {
        BoxPkm *pkm = Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos);

        Box2Main_SetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, syswk->subRet);
        func_ov255_021d0640(syswk, syswk->tray, syswk->pos);
        if (Box2Main_PokeItemFormChange(syswk, pkm) == TRUE) {
            Box2Main_RecalcPartyStats(syswk, syswk->pos);
            func_ov255_021cfc20(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
        BagSave_SubItem(syswk->param->bag, syswk->subRet, 1, HEAPID_TAIL(HEAPID_BOX2_APP));
    }
    Box2Main_PokeInfoPut(syswk, syswk->pos);
    func_ov255_021d101c(syswk, 0);
    return 96;
}

// Back from the bag after giving an item, in the item arrangement
static int func_ov255_021cc608(Box2SysWork *syswk) {
    int seq;

    switch (syswk->unk13) {
    case 0:
        func_ov255_021d0b64(syswk->app, syswk->pos, 0);
        seq = 67;
        break;
    case 1:
        func_ov255_021cdb68(syswk);
        func_ov255_021d1348(syswk->app, 0);
        func_ov255_021d0b64(syswk->app, syswk->pos, 1);
        func_ov255_021d2478(syswk, 8, 10);
        seq = 80;
        break;
    }
    func_ov255_021cdc74(syswk, (s16)syswk->subRet);
    if (syswk->subRet == 0) {
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        func_ov255_021d38bc(syswk->app->bgWinFrame);
        func_ov255_021cf208(syswk, 0, 24);
        if (syswk->unk13 == 0) {
            func_ov255_021d0310(syswk, 0x81, 1);
        } else {
            func_ov255_021d0310(syswk, 1, 1);
            func_ov255_021d0310(syswk, 0x82, 1);
        }
        return seq;
    }
    if (PML_ItemIsMail(syswk->subRet) == FALSE) {
        BoxPkm *pkm = Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos);

        Box2Main_SetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, syswk->subRet);
        if (Box2Main_PokeItemFormChange(syswk, pkm) == TRUE) {
            Box2Main_RecalcPartyStats(syswk, syswk->pos);
            func_ov255_021cfc20(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
        BagSave_SubItem(syswk->param->bag, syswk->subRet, 1, HEAPID_TAIL(HEAPID_BOX2_APP));
        syswk->app->getItem = syswk->subRet;
        func_ov255_021d0a94(syswk->app, syswk->app->getItem);
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_ITEM_ICON, TRUE);
        func_ov255_021d0bc8(syswk->app);
        func_ov255_021d0cf4(syswk->app);
    }
    if (syswk->unk13 == 0) {
        func_ov255_021d0310(syswk, 0x81, 1);
        func_ov255_021d3a48(syswk->app);
    } else {
        func_ov255_021d0310(syswk, 1, 1);
        func_ov255_021d0310(syswk, 0x82, 1);
        func_ov255_021d3a64(syswk->app);
    }
    Box2Main_PokeInfoPut(syswk, syswk->pos);
    func_ov255_021d101c(syswk, 0);
    return 96;
}

// Back from the bag after giving an item, in mode 4
static int func_ov255_021cc768(Box2SysWork *syswk) {
    int seq;

    switch (syswk->unk13) {
    case 0:
        func_ov255_021d3a48(syswk->app);
        seq = 45;
        break;
    case 1:
        func_ov255_021cdb68(syswk);
        func_ov255_021d0310(syswk, 1, 1);
        func_ov255_021d1348(syswk->app, 0);
        func_ov255_021d2478(syswk, 14, 11);
        func_ov255_021d3a64(syswk->app);
        seq = 49;
        break;
    }
    func_ov255_021d1048(syswk);
    if (syswk->subRet == 0) {
        Box2Main_PokeInfoPut(syswk, syswk->pos);
        func_ov255_021ceed0(syswk, sMenu70f8, 4);
        func_ov255_021d38bc(syswk->app->bgWinFrame);
        func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
        return seq;
    }
    if (PML_ItemIsMail(syswk->subRet) == FALSE) {
        BoxPkm *pkm = Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos);

        Box2Main_SetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, syswk->subRet);
        func_ov255_021d0640(syswk, syswk->tray, syswk->pos);
        if (Box2Main_PokeItemFormChange(syswk, pkm) == TRUE) {
            Box2Main_RecalcPartyStats(syswk, syswk->pos);
            func_ov255_021cfc20(syswk, syswk->tray, syswk->pos, syswk->app->pokeIconId[syswk->pos]);
        }
        BagSave_SubItem(syswk->param->bag, syswk->subRet, 1, HEAPID_TAIL(HEAPID_BOX2_APP));
    }
    Box2Main_PokeInfoPut(syswk, syswk->pos);
    func_ov255_021d101c(syswk, 0);
    return 96;
}

// Back from the summary in mode 1
static int func_ov255_021cc864(Box2SysWork *syswk) {
    func_ov255_021ceed0(syswk, sMenu7194, 5);
    func_ov255_021d38bc(syswk->app->bgWinFrame);
    func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
    func_ov255_021d1048(syswk);
    return 54;
}

// Back from the summary in mode 0
static int func_ov255_021cc894(Box2SysWork *syswk) {
    func_ov255_021ceed0(syswk, sMenu7130, 5);
    func_ov255_021d38bc(syswk->app->bgWinFrame);
    func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
    func_ov255_021d0310(syswk, 1, 1);
    func_ov255_021d1348(syswk->app, 0);
    func_ov255_021cdb68(syswk);
    func_ov255_021d1048(syswk);
    return 59;
}

// Back from the summary in the arrangement
static int func_ov255_021cc8dc(Box2SysWork *syswk) {
    switch (syswk->unk13) {
    case 0:
        func_ov255_021ceed0(syswk, sMenu71f0, 6);
        func_ov255_021d38bc(syswk->app->bgWinFrame);
        func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
        func_ov255_021d1048(syswk);
        return 17;
    case 1:
        func_ov255_021ceed0(syswk, sMenu71f0, 6);
        func_ov255_021d38bc(syswk->app->bgWinFrame);
        func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
        func_ov255_021d0310(syswk, 1, 1);
        func_ov255_021d1348(syswk->app, 0);
        func_ov255_021d2478(syswk, 5, 10);
        func_ov255_021cdb68(syswk);
        func_ov255_021d1048(syswk);
        return 27;
    case 2:
        if (syswk->unk1B == 1) {
            func_ov255_021cfe0c(syswk);
        }
        func_ov255_021d2478(syswk, 4, syswk->pos);
        func_ov255_021bc0c0(syswk);
        while (func_ov255_021c05e8(syswk) != FALSE) {
        }
        func_ov255_021d1a1c(syswk);
        func_ov255_021d3a48(syswk->app);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->moveMode == 2) {
            return 36;
        }
        return 21;
    case 3:
        syswk->app->rangeHeight = 1;
        syswk->app->rangeWidth = 1;
        if (syswk->pos >= BOX2_PARTY_POS) {
            func_ov255_021cfe0c(syswk);
            func_ov255_021d0374(syswk, syswk->pos, 1, 1);
        } else if (syswk->tray == syswk->getTray) {
            func_ov255_021d0374(syswk, syswk->pos, 1, 1);
        } else {
            func_ov255_021cfc20(syswk, syswk->getTray, syswk->pos, syswk->app->pokeIconId[BOX2_BOXLIST_POS]);
        }
        Box2Main_HandGetPokeSet(syswk);
        func_ov255_021d121c(syswk, BOX2_BOXLIST_POS);
        func_ov255_021d2478(syswk, 4, syswk->unk1D);
        func_ov255_021bc0c0(syswk);
        while (func_ov255_021c05e8(syswk) != FALSE) {
        }
        func_ov255_021d1a1c(syswk);
        func_ov255_021d3a48(syswk->app);
        if (syswk->unk1D >= 34 && syswk->unk1D <= 37) {
            func_ov255_021d1ac8(syswk, syswk->unk1D - 34, 1);
        }
        func_ov255_021d1054(syswk, BOX2_BOXLIST_POS);
        return 21;
    case 4:
        func_ov255_021cdb7c(syswk);
        func_ov255_021d2478(syswk, 6, syswk->pos);
        func_ov255_021d3a64(syswk->app);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->moveMode == 2) {
            return 42;
        }
        return 32;
    case 5:
        syswk->app->rangeHeight = 1;
        syswk->app->rangeWidth = 1;
        func_ov255_021cdb7c(syswk);
        if (syswk->pos < BOX2_PARTY_POS && syswk->tray != syswk->getTray) {
            func_ov255_021cfc20(syswk, syswk->getTray, syswk->pos, syswk->app->pokeIconId[BOX2_BOXLIST_POS]);
        } else {
            func_ov255_021d0374(syswk, syswk->pos, 1, 1);
        }
        Box2Main_HandGetPokeSet(syswk);
        func_ov255_021d121c(syswk, BOX2_BOXLIST_POS);
        func_ov255_021d2478(syswk, 6, syswk->unk1D);
        func_ov255_021d3a64(syswk->app);
        func_ov255_021d1054(syswk, BOX2_BOXLIST_POS);
        return 32;
    }
    return 0;
}

// Back from the summary in mode 4
static int func_ov255_021ccae4(Box2SysWork *syswk) {
    switch (syswk->unk13) {
    case 0:
        func_ov255_021ceed0(syswk, sMenu70f8, 4);
        func_ov255_021d38bc(syswk->app->bgWinFrame);
        func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
        func_ov255_021d2478(syswk, 13, 37);
        func_ov255_021d1048(syswk);
        return 45;
    case 1:
        func_ov255_021ceed0(syswk, sMenu70f8, 4);
        func_ov255_021d38bc(syswk->app->bgWinFrame);
        func_ov255_021cf1ac(syswk, syswk->pos, 1, 24);
        func_ov255_021d0310(syswk, 1, 1);
        func_ov255_021d1348(syswk->app, 0);
        func_ov255_021d2478(syswk, 14, 10);
        func_ov255_021cdb68(syswk);
        func_ov255_021d1048(syswk);
        return 49;
    case 4:
        func_ov255_021cdb7c(syswk);
        func_ov255_021d2478(syswk, 6, syswk->pos);
        CursorMove_DisablePos(syswk->app->cursorMove, 39);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->moveMode == 2) {
            return 42;
        }
        return 32;
    case 5:
        syswk->app->rangeHeight = 1;
        syswk->app->rangeWidth = 1;
        func_ov255_021cdb7c(syswk);
        if (syswk->pos < BOX2_PARTY_POS && syswk->tray != syswk->getTray) {
            func_ov255_021cfc20(syswk, syswk->getTray, syswk->pos, syswk->app->pokeIconId[BOX2_BOXLIST_POS]);
        } else {
            func_ov255_021d0374(syswk, syswk->pos, 1, 1);
        }
        Box2Main_HandGetPokeSet(syswk);
        func_ov255_021d121c(syswk, BOX2_BOXLIST_POS);
        func_ov255_021d2478(syswk, 6, syswk->unk1D);
        CursorMove_DisablePos(syswk->app->cursorMove, 39);
        func_ov255_021d1054(syswk, BOX2_BOXLIST_POS);
        return 32;
    }
    return 0;
}

// Back from the summary while moving Pokémon in the arrangement
static int func_ov255_021ccc28(Box2SysWork *syswk) {
    switch (syswk->unk13) {
    case 2:
        if (syswk->moveMode == 0) {
            func_ov255_021d3a48(syswk->app);
            func_ov255_021d2478(syswk, 3, syswk->pos);
            syswk->pos = BOX2_GET_NONE;
            return 17;
        }
        if (syswk->unk1B == 1) {
            func_ov255_021cfe0c(syswk);
        }
        func_ov255_021d2478(syswk, 4, syswk->pos);
        func_ov255_021bc0c0(syswk);
        while (func_ov255_021c05e8(syswk) != FALSE) {
        }
        func_ov255_021d1a1c(syswk);
        func_ov255_021d3a48(syswk->app);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->moveMode == 2) {
            return 36;
        }
        return 21;
    case 3:
        syswk->app->rangeHeight = 1;
        syswk->app->rangeWidth = 1;
        if (syswk->pos >= BOX2_PARTY_POS) {
            func_ov255_021cfe0c(syswk);
            func_ov255_021d0374(syswk, syswk->pos, 1, 1);
        } else if (syswk->tray == syswk->getTray) {
            func_ov255_021d0374(syswk, syswk->pos, 1, 1);
        } else {
            func_ov255_021cfc20(syswk, syswk->getTray, syswk->pos, syswk->app->pokeIconId[BOX2_BOXLIST_POS]);
        }
        Box2Main_HandGetPokeSet(syswk);
        func_ov255_021d121c(syswk, BOX2_BOXLIST_POS);
        func_ov255_021d2478(syswk, 4, syswk->unk1D);
        func_ov255_021bc0c0(syswk);
        while (func_ov255_021c05e8(syswk) != FALSE) {
        }
        func_ov255_021d1a1c(syswk);
        func_ov255_021d3a48(syswk->app);
        if (syswk->unk1D >= 34 && syswk->unk1D <= 37) {
            func_ov255_021d1ac8(syswk, syswk->unk1D - 34, 1);
        }
        func_ov255_021d1054(syswk, BOX2_BOXLIST_POS);
        return 21;
    case 4:
        if (syswk->moveMode == 0) {
            func_ov255_021cdb68(syswk);
            func_ov255_021d3a64(syswk->app);
            func_ov255_021d0310(syswk, 1, 1);
            func_ov255_021d1348(syswk->app, 0);
            func_ov255_021d2478(syswk, 5, syswk->pos - BOX2_PARTY_POS);
            syswk->pos = BOX2_GET_NONE;
            return 27;
        }
        func_ov255_021cdb7c(syswk);
        func_ov255_021d2478(syswk, 6, syswk->pos);
        func_ov255_021d3a64(syswk->app);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->moveMode == 2) {
            return 42;
        }
        return 32;
    case 5:
        syswk->app->rangeHeight = 1;
        syswk->app->rangeWidth = 1;
        func_ov255_021cdb7c(syswk);
        if (syswk->pos < BOX2_PARTY_POS && syswk->tray != syswk->getTray) {
            func_ov255_021cfc20(syswk, syswk->getTray, syswk->pos, syswk->app->pokeIconId[BOX2_BOXLIST_POS]);
        } else {
            func_ov255_021d0374(syswk, syswk->pos, 1, 1);
        }
        Box2Main_HandGetPokeSet(syswk);
        func_ov255_021d121c(syswk, BOX2_BOXLIST_POS);
        func_ov255_021d2478(syswk, 6, syswk->unk1D);
        func_ov255_021d3a64(syswk->app);
        func_ov255_021d1054(syswk, BOX2_BOXLIST_POS);
        return 32;
    }
    return 0;
}

// Back from the summary while moving Pokémon in mode 4
static int func_ov255_021cce14(Box2SysWork *syswk) {
    switch (syswk->unk13) {
    case 2:
        func_ov255_021d3a48(syswk->app);
        func_ov255_021d2478(syswk, 13, syswk->pos);
        syswk->pos = BOX2_GET_NONE;
        return 45;
    case 4:
        if (syswk->moveMode == 0) {
            func_ov255_021cdb68(syswk);
            func_ov255_021d3a64(syswk->app);
            func_ov255_021d0310(syswk, 1, 1);
            func_ov255_021d1348(syswk->app, 0);
            func_ov255_021d2478(syswk, 14, syswk->pos - BOX2_PARTY_POS);
            syswk->pos = BOX2_GET_NONE;
            return 49;
        }
        func_ov255_021cdb7c(syswk);
        func_ov255_021d2478(syswk, 6, syswk->pos);
        CursorMove_DisablePos(syswk->app->cursorMove, 39);
        syswk->pos = BOX2_GET_NONE;
        if (syswk->moveMode == 2) {
            return 42;
        }
        return 32;
    case 5:
        syswk->app->rangeHeight = 1;
        syswk->app->rangeWidth = 1;
        func_ov255_021cdb7c(syswk);
        if (syswk->pos < BOX2_PARTY_POS && syswk->tray != syswk->getTray) {
            func_ov255_021cfc20(syswk, syswk->getTray, syswk->pos, syswk->app->pokeIconId[BOX2_BOXLIST_POS]);
        } else {
            func_ov255_021d0374(syswk, syswk->pos, 1, 1);
        }
        Box2Main_HandGetPokeSet(syswk);
        func_ov255_021d121c(syswk, BOX2_BOXLIST_POS);
        func_ov255_021d2478(syswk, 6, syswk->unk1D);
        CursorMove_DisablePos(syswk->app->cursorMove, 39);
        func_ov255_021d1054(syswk, BOX2_BOXLIST_POS);
        return 32;
    }
    return 0;
}

// Scrolls the tray left
static int func_ov255_021ccf1c(Box2SysWork *syswk, u32 frameOut, int seq) {
    Box2Main_ScrollTray(syswk, FALSE);
    if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
        func_ov255_021d3954(syswk->app->bgWinFrame);
        func_ov255_021cefa4(syswk->app, 24);
        func_ov255_021bc018(syswk);
        if (frameOut == 1) {
            func_ov255_021d3a48(syswk->app);
        }
    }
    return func_ov255_021cbec8(syswk, Box2Main_VFuncTrayScrollLeft, seq);
}

// Scrolls the tray right
static int func_ov255_021ccf68(Box2SysWork *syswk, u32 frameOut, int seq) {
    Box2Main_ScrollTray(syswk, TRUE);
    if (func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
        func_ov255_021d3954(syswk->app->bgWinFrame);
        func_ov255_021cefa4(syswk->app, 24);
        func_ov255_021bc018(syswk);
        if (frameOut == 1) {
            func_ov255_021d3a48(syswk->app);
        }
    }
    return func_ov255_021cbec8(syswk, Box2Main_VFuncTrayScrollRight, seq);
}

// Moving Pokémon: picks up a touched Pokémon
static int func_ov255_021ccfb4(Box2SysWork *syswk, u32 pos) {
    syswk->unk1B = 0;
    syswk->moveMode = 0;
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    func_ov255_021cff58(syswk->app, syswk->pos, FALSE);
    func_ov255_021d0374(syswk, syswk->pos, 1, 1);
    func_ov255_021d1054(syswk, BOX2_BOXLIST_POS);
    func_ov255_021cd5b0(syswk);
    func_ov255_021d1af8(syswk, 2, 2, 1, 1);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    func_ov255_021d3a48(syswk->app);
    syswk->nextSeq = 21;
    syswk->unk1A = 0;
    return func_ov255_021cbec8(syswk, Box2Main_VFuncPokeMoveTouch, 18);
}

// Moving Pokémon with the party out: picks up a touched Pokémon
static int func_ov255_021cd02c(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    syswk->unk1B = 0;
    func_ov255_021cff58(syswk->app, syswk->pos, FALSE);
    func_ov255_021d0374(syswk, syswk->pos, 1, 1);
    func_ov255_021d1054(syswk, BOX2_BOXLIST_POS);
    func_ov255_021cd5b0(syswk);
    func_ov255_021d1af8(syswk, 2, 2, 1, 1);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    syswk->nextSeq = 32;
    syswk->unk1A = 0;
    return func_ov255_021cbec8(syswk, Box2Main_VFuncRangeMoveTouch, 46);
}

// Mode 1: picks up a touched Pokémon to take it into the party
static int func_ov255_021cd098(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    func_ov255_021cff58(syswk->app, syswk->pos, FALSE);
    func_ov255_021d0374(syswk, syswk->pos, 1, 1);
    func_ov255_021d1054(syswk, BOX2_BOXLIST_POS);
    func_ov255_021d1348(syswk->app, 0);
    func_ov255_021cfe0c(syswk);
    func_ov255_021d0310(syswk, 1, 1);
    func_ov255_021d0310(syswk, 2, 1);
    func_ov255_021d0350(syswk->app, BOX2_BOXLIST_POS, FALSE);
    func_ov255_021cd5b0(syswk);
    func_ov255_021d1af8(syswk, 2, 2, 1, 1);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    syswk->unk1A = 2;
    return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyInTouch, 58);
}

// Opens a Pokémon's menu
static int func_ov255_021cd128(Box2SysWork *syswk, u32 pos, int seq) {
    syswk->pos = pos;
    syswk->app->unkA552 = 1;
    func_ov255_021cff58(syswk->app, syswk->pos, TRUE);
    func_ov255_021d1048(syswk);
    func_ov255_021d390c(syswk->app->bgWinFrame);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncCursorMoveFrame, func_ov255_021cbee8(syswk, seq));
}

// The party's main state: picks up a touched Pokémon
static int func_ov255_021cd16c(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    syswk->moveMode = 0;
    syswk->getTray = syswk->tray;
    syswk->unk1B = 1;
    func_ov255_021cff58(syswk->app, syswk->pos, FALSE);
    func_ov255_021d0374(syswk, syswk->pos, 1, 1);
    func_ov255_021d1054(syswk, BOX2_BOXLIST_POS);
    PokeParty_GetPkm(syswk->param->party, pos - BOX2_PARTY_POS);
    func_ov255_021cd5b0(syswk);
    func_ov255_021d1af8(syswk, 2, 2, 1, 1);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    func_ov255_021d3a64(syswk->app);
    syswk->nextSeq = 32;
    syswk->unk1A = 1;
    return func_ov255_021cbec8(syswk, Box2Main_VFuncPokeMoveTouchParty, 30);
}

// The party's main state from the box's: picks up a touched Pokémon
static int func_ov255_021cd1f0(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    syswk->unk1B = 1;
    func_ov255_021cff58(syswk->app, syswk->pos, FALSE);
    func_ov255_021d0374(syswk, syswk->pos, 1, 1);
    func_ov255_021d1054(syswk, BOX2_BOXLIST_POS);
    PokeParty_GetPkm(syswk->param->party, pos - BOX2_PARTY_POS);
    func_ov255_021cd5b0(syswk);
    func_ov255_021d1af8(syswk, 2, 2, 1, 1);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    syswk->nextSeq = 32;
    syswk->unk1A = 1;
    return func_ov255_021cbec8(syswk, Box2Main_VFuncPokeMoveTouchParty, 52);
}

// Mode 0: picks up a touched Pokémon of the party to deposit it
static int func_ov255_021cd268(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    func_ov255_021cff58(syswk->app, syswk->pos, FALSE);
    func_ov255_021d0374(syswk, syswk->pos, 1, 1);
    func_ov255_021d1054(syswk, BOX2_BOXLIST_POS);
    func_ov255_021d0310(syswk, 2, 1);
    func_ov255_021d0350(syswk->app, BOX2_BOXLIST_POS, FALSE);
    PokeParty_GetPkm(syswk->param->party, pos - BOX2_PARTY_POS);
    func_ov255_021cd5b0(syswk);
    func_ov255_021d1af8(syswk, 2, 2, 1, 1);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    syswk->unk1A = 1;
    return func_ov255_021cbec8(syswk, Box2Main_VFuncPartyOutTouch, 66);
}

// Opens a party Pokémon's menu
static int func_ov255_021cd2e8(Box2SysWork *syswk, u32 pos, int seq) {
    syswk->pos = pos;
    syswk->app->unkA552 = 1;
    func_ov255_021cff58(syswk->app, syswk->pos, TRUE);
    func_ov255_021d1048(syswk);
    func_ov255_021d390c(syswk->app->bgWinFrame);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncCursorMoveFrame, func_ov255_021cbee8(syswk, seq));
}

// Moving Pokémon: picks up a touched Pokémon
static int func_ov255_021cd32c(Box2SysWork *syswk, u32 pos, int seq) {
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    syswk->app->oldCurPos = pos;
    func_ov255_021d0374(syswk, syswk->pos, 1, 1);
    func_ov255_021cff58(syswk->app, BOX2_BOXLIST_POS, FALSE);
    func_ov255_021d1054(syswk, BOX2_BOXLIST_POS);
    func_ov255_021d1af8(syswk, 2, 1, 2, 1);
    func_ov255_021cd5b0(syswk);
    syswk->nextSeq = seq;
    syswk->unk1A = 2;
    return func_ov255_021cbec8(syswk, Box2Main_VFuncPokeMoveTouch, 23);
}

// Picking a range: picks up the touched range
static int func_ov255_021cd390(Box2SysWork *syswk, int seq) {
    syswk->app->oldCurPos = syswk->pos;
    func_ov255_021d2238(syswk->app, syswk->pos, syswk->app->rangeWidth, syswk->app->rangeHeight, 0);
    func_ov255_021d1054(syswk, BOX2_BOXLIST_POS);
    func_ov255_021d11a4(syswk, 0);
    func_ov255_021d1af8(syswk, 2, 1, 2, 2);
    func_ov255_021cd5b0(syswk);
    syswk->nextSeq = seq;
    syswk->unk1A = 2;
    return func_ov255_021cbec8(syswk, Box2Main_VFuncPokeMoveTouch, 38);
}

// Moving Pokémon by the keys: picks up the Pokémon under the cursor
static int func_ov255_021cd3f8(Box2SysWork *syswk, u32 pos, int seq) {
    syswk->unk18 = 1;
    syswk->pos = pos;
    func_ov255_021cff58(syswk->app, BOX2_BOXLIST_POS, FALSE);
    func_ov255_021d1054(syswk, BOX2_BOXLIST_POS);
    func_ov255_021d1af8(syswk, 2, 1, 0, 0);
    if (syswk->param->mode == 4) {
        func_ov255_021d3a58(syswk->app);
        func_ov255_021d3a74(syswk->app);
        CursorMove_DisablePos(syswk->app->cursorMove, 39);
    }
    return func_ov255_021cbec8(syswk, Box2Main_VFuncPokeMoveGetKey, seq);
}

// Moving Pokémon by the keys in the arrangement: picks up the Pokémon under the cursor
static int func_ov255_021cd458(Box2SysWork *syswk, u32 pos) {
    syswk->unk18 = 1;
    syswk->pos = pos;
    func_ov255_021cff58(syswk->app, BOX2_BOXLIST_POS, FALSE);
    func_ov255_021d1054(syswk, BOX2_BOXLIST_POS);
    func_ov255_021d1af8(syswk, 2, 1, 0, 0);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncPokeMoveGetKey, 21);
}

// Picking a range: picks up the range by the keys
static int func_ov255_021cd494(Box2SysWork *syswk, u32 pos, int seq) {
    syswk->pos = pos;
    func_ov255_021cff58(syswk->app, BOX2_BOXLIST_POS, FALSE);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncPokeMoveGetKey, seq);
}

// Moving Pokémon by the keys: drops the Pokémon, unless the box picked in the list is this one or full
static int func_ov255_021cd4b8(Box2SysWork *syswk, u32 pos, int seq) {
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
    func_ov255_021cd5b0(syswk);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncPokeMovePutKey, 23);
}

// Picking a range: drops the range, unless the box picked in the list is this one or has too little room
static int func_ov255_021cd52c(Box2SysWork *syswk, u32 pos, int seq) {
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
    func_ov255_021d1e2c(syswk, 0);
    syswk->app->pokePutKey = pos;
    syswk->unk1A = 2;
    syswk->nextSeq = seq;
    func_ov255_021cd5b0(syswk);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncPokeMovePutKey, 38);
}

// Allocates the work of a Pokémon move
static void func_ov255_021cd5b0(Box2SysWork *syswk) {
    syswk->app->vfunk.work = GFL_HeapAllocate(HEAPID_BOX2_APP, sizeof(Box2PokeMoveWork), TRUE, "box2_seq.c", 10826);
}

static void func_ov255_021cd5d8(Box2SysWork *syswk) {
    GFL_HeapFree(syswk->app->vfunk.work);
}

// The item arrangement: takes the item of a touched Pokémon
static int func_ov255_021cd5e4(Box2SysWork *syswk, u32 pos) {
    s16 x, y;

    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    syswk->app->unkA552 = 1;
    syswk->app->getItem = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);
    func_ov255_021d38e0(syswk->app->bgWinFrame);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    if (syswk->app->getItem != 0) {
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        func_ov255_021d0b64(syswk->app, syswk->pos, 0);
        func_ov255_021d0a94(syswk->app, syswk->app->getItem);
        func_ov255_021d0bc8(syswk->app);
        func_ov255_021d1af8(syswk, 2, 2, 1, 1);
    } else if (func_ov255_021cf658(syswk->app, BOX2_ACTOR_ITEM_ICON) == TRUE) {
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        func_ov255_021d11a4(syswk, 0);
    }
    func_ov255_021cfcdc(syswk->pos, &x, &y, 0);
    syswk->app->tpx = x + 8;
    syswk->app->tpy = y + 8;
    func_ov255_021cdb34(syswk);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncItemArrangeGetTouch, func_ov255_021cbee8(syswk, 72));
}

// The item arrangement: opens the menu of a picked Pokémon
static int func_ov255_021cd6cc(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    syswk->app->unkA552 = 1;
    Box2Main_PokeInfoPut(syswk, syswk->pos);
    syswk->app->getItem = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);
    func_ov255_021cdc74(syswk, (s16)syswk->app->getItem);
    func_ov255_021d390c(syswk->app->bgWinFrame);
    if (syswk->app->getItem != 0) {
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        func_ov255_021d0b64(syswk->app, syswk->pos, 0);
        func_ov255_021d0a94(syswk->app, syswk->app->getItem);
        func_ov255_021d0bc8(syswk->app);
        func_ov255_021d32d4(syswk->app, 36, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, 36);
        syswk->app->oldCurPos = 36;
    } else {
        func_ov255_021d32d4(syswk->app, 37, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, 37);
        syswk->app->oldCurPos = 37;
    }
    return func_ov255_021cbec8(syswk, Box2Main_VFuncItemArrangeMenuClose, func_ov255_021cbee8(syswk, 67));
}

// The item arrangement with the party out: takes the item of a touched Pokémon
static int func_ov255_021cd798(Box2SysWork *syswk, u32 pos) {
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
    func_ov255_021cf63c(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
    func_ov255_021d0b64(syswk->app, syswk->pos, 2);
    func_ov255_021d0a94(syswk->app, syswk->app->getItem);
    func_ov255_021d0bc8(syswk->app);
    func_ov255_021d1af8(syswk, 2, 1, 1, 1);
    func_ov255_021cfcdc(syswk->pos, &x, &y, 2);
    syswk->app->tpx = x + 8;
    syswk->app->tpy = y + 8;
    func_ov255_021cdb34(syswk);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncItemArrangePartyGetTouch, 76);
}

// The item arrangement with the party out: takes the item of the Pokémon under the cursor
static int func_ov255_021cd858(Box2SysWork *syswk, u32 pos) {
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
    func_ov255_021cf63c(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
    func_ov255_021d0b98(syswk->app, syswk->pos, 2);
    func_ov255_021d0a94(syswk->app, syswk->app->getItem);
    func_ov255_021d1af8(syswk, 2, 1, 1, 1);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncItemArrangeGetKey, 74);
}

// The item arrangement: puts a held item back by the keys
static int func_ov255_021cd8d8(Box2SysWork *syswk) {
    u8 pos = CursorMove_GetPos(syswk->app->cursorMove);

    if (pos < BOX2_BOXLIST_POS) {
        Box2Main_PokeInfoPut(syswk, pos);
    } else {
        Box2Main_PokeInfoOff(syswk);
    }
    func_ov255_021cdb34(syswk);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncItemArrangeKeyCancel, 77);
}

// The item arrangement: drops a held item by the keys
static int func_ov255_021cd910(Box2SysWork *syswk, u32 pos) {
    s16 x, y;

    syswk->app->pokePutKey = pos;
    syswk->unk18 = 0;
    func_ov255_021cf6ec(syswk->app, BOX2_ACTOR_CURSOR, &x, &y, 0);
    func_ov255_021cf6c8(syswk->app, BOX2_ACTOR_ITEM_ICON, x, y + 8, 0);
    func_ov255_021d11a4(syswk, 0);
    if (syswk->app->pokePutKey == BOX2_GET_NONE) {
        return func_ov255_021cd8d8(syswk);
    }
    if (Box2Main_PokeItemMoveCheck(syswk, syswk->pos, pos) == FALSE) {
        return func_ov255_021cd8d8(syswk);
    }
    return func_ov255_021cbec8(syswk, Box2Main_VFuncItemArrangePutKey, 79);
}

// The item arrangement in the party: takes the item of a touched Pokémon
static int func_ov255_021cd994(Box2SysWork *syswk, u32 pos) {
    s16 x, y;

    syswk->pos = pos;
    syswk->getTray = BOX2_GET_NONE;
    syswk->app->getItem = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);
    func_ov255_021d38e0(syswk->app->bgWinFrame);
    func_ov255_021cefa4(syswk->app, 24);
    func_ov255_021bc018(syswk);
    if (syswk->app->getItem != 0) {
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        func_ov255_021d0b64(syswk->app, syswk->pos, 1);
        func_ov255_021d0a94(syswk->app, syswk->app->getItem);
        func_ov255_021d0bc8(syswk->app);
        func_ov255_021d1af8(syswk, 2, 2, 1, 1);
    } else if (func_ov255_021cf658(syswk->app, BOX2_ACTOR_ITEM_ICON) == TRUE) {
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        func_ov255_021d11a4(syswk, 0);
    }
    func_ov255_021cfcdc(syswk->pos, &x, &y, 1);
    syswk->app->tpx = x + 8;
    syswk->app->tpy = y + 8;
    func_ov255_021cdb34(syswk);
    return func_ov255_021cbec8(syswk, Box2Main_VFuncItemArrangeBoxPartyGetTouch, 82);
}

// The item arrangement in the party: opens the menu of a picked Pokémon
static int func_ov255_021cda6c(Box2SysWork *syswk, u32 pos) {
    syswk->pos = pos;
    syswk->getTray = syswk->tray;
    syswk->app->unkA552 = 1;
    Box2Main_PokeInfoPut(syswk, syswk->pos);
    syswk->app->getItem = Box2Main_GetPokeParam(syswk, syswk->pos, syswk->tray, PKM_PARAM_ITEM, NULL);
    func_ov255_021cdc74(syswk, (s16)syswk->app->getItem);
    func_ov255_021d390c(syswk->app->bgWinFrame);
    if (syswk->app->getItem != 0) {
        func_ov255_021cf63c(syswk->app, BOX2_ACTOR_ITEM_ICON, FALSE);
        func_ov255_021d0b64(syswk->app, syswk->pos, 1);
        func_ov255_021d0a94(syswk->app, syswk->app->getItem);
        func_ov255_021d0bc8(syswk->app);
        func_ov255_021d32d4(syswk->app, 9, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, 9);
        syswk->app->oldCurPos = 9;
    } else {
        func_ov255_021d32d4(syswk->app, 10, CursorMove_GetPos(syswk->app->cursorMove));
        CursorMove_SetPos(syswk->app->cursorMove, 10);
        syswk->app->oldCurPos = 10;
    }
    return func_ov255_021cbec8(syswk, Box2Main_VFuncItemArrangeMenuClose, func_ov255_021cbee8(syswk, 80));
}

// Allocates the work of an item icon's move
static void func_ov255_021cdb34(Box2SysWork *syswk) {
    syswk->app->vfunk.work = GFL_HeapAllocate(HEAPID_BOX2_APP, sizeof(Box2ItemMoveWork), FALSE, "box2_seq.c", 11181);
}

static void func_ov255_021cdb5c(Box2SysWork *syswk) {
    GFL_HeapFree(syswk->app->vfunk.work);
}

static void func_ov255_021cdb68(Box2SysWork *syswk) {
    func_ov255_021d3704(syswk->app->bgWinFrame);
    func_ov255_021cfe78(syswk);
}

static void func_ov255_021cdb7c(Box2SysWork *syswk) {
    func_ov255_021d3714(syswk->app->bgWinFrame);
    func_ov255_021cfee4(syswk);
}

void func_ov255_021cdb90(Box2SysWork *syswk) {
    func_ov255_021cfe78(syswk);
    func_ov255_021d3704(syswk->app->bgWinFrame);
}

// Scrolls to the box picked in the list
static int func_ov255_021cdba4(Box2SysWork *syswk, int seq) {
    BOOL right = Box2Main_IsTrayScrollRight(syswk, syswk->tray, syswk->app->unkA55F);
    Box2VFunc func;

    syswk->tray = syswk->app->unkA55F;
    func_ov255_021cf9c8(syswk, syswk->tray);
    Box2Main_WallPaperSet(syswk, Box2Main_GetWallPaperNumber(syswk, syswk->tray), right);
    if (right == TRUE) {
        func = Box2Main_VFuncTrayScrollRight;
    } else {
        func = Box2Main_VFuncTrayScrollLeft;
    }
    syswk->nextSeq = seq;
    GFL_SndSEPlay(SEQ_SE_SELECT1);
    return func_ov255_021cbec8(syswk, func, syswk->nextSeq);
}

// Allocates the work of a drag of the box list
static void func_ov255_021cdc04(Box2SysWork *syswk) {
    Box2BoxListDrag *work = GFL_HeapAllocate(HEAPID_TAIL(HEAPID_BOX2_APP), sizeof(Box2BoxListDrag), FALSE,
                                             "box2_seq.c", 11290);

    work->cnt = 0;
    work->dir = 0;
    syswk->app->subWork = work;
}

// Moves the party's frame in or out, then goes to seq
static int func_ov255_021cdc38(Box2SysWork *syswk, int seq) {
    func_ov255_021bc0c0(syswk);
    return func_ov255_021cbec8(syswk, func_ov255_021c05e8, seq);
}

// Moves the party's frame, then goes to seq
static int func_ov255_021cdc54(Box2SysWork *syswk, int seq) {
    func_ov255_021d3b34(syswk->app->bgWinFrame);
    return func_ov255_021cbec8(syswk, func_ov255_021c0604, seq);
}

// Opens the item arrangement's menu, with or without a held item
void func_ov255_021cdc74(Box2SysWork *syswk, s16 item) {
    if (item != 0) {
        if (syswk->pos < BOX2_PARTY_POS) {
            CursorMove_EnablePos(syswk->app->cursorMove, 36);
        } else {
            CursorMove_EnablePos(syswk->app->cursorMove, 9);
        }
        func_ov255_021ceed0(syswk, sMenu70dc, 3);
    } else {
        if (syswk->pos < BOX2_PARTY_POS) {
            CursorMove_DisablePos(syswk->app->cursorMove, 36);
        } else {
            CursorMove_DisablePos(syswk->app->cursorMove, 9);
        }
        func_ov255_021ceed0(syswk, sMenu70d4, 2);
    }
}

// Mode 0: whether the party Pokémon can be deposited: 0 if so, 1 for the last battler, 2 for mail
static u32 func_ov255_021cdcc8(Box2SysWork *syswk) {
    // The party position, then the result
    u32 val = syswk->pos - BOX2_PARTY_POS;

    if (Box2Main_BattlePokeCheck(syswk, val) == FALSE) {
        return 1;
    }
    if (PML_ItemIsMail(PokeParty_GetParam(PokeParty_GetPkm(syswk->param->party, val), PKM_PARAM_ITEM, NULL))
        == TRUE) {
        val = 2;
    } else {
        val = 0;
    }
    return val;
}

// Toggles a mark
static void func_ov255_021cdd04(Box2SysWork *syswk, u32 mark) {
    syswk->app->unkA554 ^= 1 << mark;
    Box2Main_MarkingPutMain(syswk, syswk->app->unkA554);
}

// Picks a wallpaper of the theme's menu
static int func_ov255_021cdd24(Box2SysWork *syswk, u32 pos) {
    if (syswk->app->unkA55C <= 4
        || (syswk->app->unkA55C == 5 && func_02007da4(syswk->param->boxes, 2) == FALSE)) {
        syswk->app->wallpaperPos = pos + (syswk->app->unkA55C - 1) * 4 - 1;
    } else {
        syswk->app->wallpaperPos = pos + (syswk->app->unkA55C - 1) * 4;
    }
    return func_ov255_021cc3b0(syswk, pos, func_ov255_021cbe58(syswk, 114));
}

// Dims the actors behind the box's name entry
static void func_ov255_021cdd80(Box2SysWork *syswk) {
    switch (syswk->param->mode) {
    case 0:
        func_ov255_021d0310(syswk, 2, 1);
        break;
    case 1:
    case 3:
        func_ov255_021d1348(syswk->app, 0);
        func_ov255_021d0310(syswk, 1, 1);
        break;
    case 2:
    case 4:
    case 5:
        func_ov255_021d1348(syswk->app, 0);
        if (func_ov255_021d3858(syswk->app->bgWinFrame) == TRUE) {
            func_ov255_021d0310(syswk, 2, 1);
        } else {
            func_ov255_021d0310(syswk, 1, 1);
        }
        break;
    }
}

// Switches to the next way of moving Pokémon, by Select
static int func_ov255_021cddf0(Box2SysWork *syswk, int seq) {
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
    if (syswk->moveMode == 1 && func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
        syswk->unk1C_4 = 1;
    }
    syswk->unk1C_6 = 0;
    Box2Main_ShowCursor(syswk);
    return func_ov255_021cbe58(syswk, seq);
}

// Sets the way of moving Pokémon picked in the menu
static int func_ov255_021cde88(Box2SysWork *syswk, int seq) {
    GFL_SndSEPlay(SEQ_SE_DECIDE1);
    syswk->unk1C_4 = 0;
    if (syswk->moveMode == 1 && func_ov255_021d39c0(syswk->app->bgWinFrame) == TRUE) {
        syswk->unk1C_4 = 1;
    }
    syswk->unk1C_6 = 0;
    return func_ov255_021cbe58(syswk, seq);
}

// Cancels a range pick
static void func_ov255_021cded4(Box2SysWork *syswk) {
    func_ov255_021d1e2c(syswk, 0);
    syswk->pos = BOX2_GET_NONE;
    syswk->unk18 = 0;
    func_ov255_021d28c4(syswk, CursorMove_GetPos(syswk->app->cursorMove));
}

// Puts a range's ends in order
static void func_ov255_021cdef8(Box2SysWork *syswk) {
    if (syswk->app->rangeSelect.startPos > syswk->app->rangeSelect.endPos) {
        u8 end = syswk->app->rangeSelect.endPos;

        syswk->app->rangeSelect.endPos = syswk->app->rangeSelect.startPos;
        syswk->app->rangeSelect.startPos = end;
    }
}
