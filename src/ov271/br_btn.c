// The Battle Recorder's menu buttons: the buttons of the current menu, the stack of the buttons pressed to reach it,
// and the sequences that move them between menus. Also the button objects, a cell actor with its label in a bitmap
// OAM, that the screens use for their own buttons. The name is the ROM's string, from GFL_HeapAllocate's asserts

#include "types.h"
#include "app/battle_recorder/br_btn.h"
#include "app/battle_recorder/br_btn_data.h"
#include "app/battle_recorder/br_res.h"
#include "app/battle_recorder/br_util.h"
#include "gfl/bmp.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "system/bmp_oam.h"
#include "system/printsys.h"

// The parameters of a menu button, which BrBtnWork_GetParam reads from its data
enum {
    BR_BTN_PARAM_TYPE,
    BR_BTN_PARAM_DATA1,
    BR_BTN_PARAM_DATA2,
    BR_BTN_PARAM_VALID,
    BR_BTN_PARAM_MENUID,
    BR_BTN_PARAM_UNVALID_TYPE,
    BR_BTN_PARAM_UNVALID_DATA,
    BR_BTN_PARAM_VALID_TYPE,
    BR_BTN_PARAM_X,
    BR_BTN_PARAM_Y,
};

// How BrBtnWork_StartMove moves a button
enum {
    // Up or down to the target
    BR_BTN_MOVE_TARGET,
    // Down to its place in the menu
    BR_BTN_MOVE_RETURN,
    // Off the top of the sub screen, then from the top of the main screen to the target
    BR_BTN_MOVE_TO_MAIN,
    // Off the top of the main screen, then from the top of the sub screen to the target
    BR_BTN_MOVE_TO_SUB,
    // To the target, more slowly
    BR_BTN_MOVE_TARGET_SLOW,
};

// A button of the menus
typedef struct BrBtnWork {
    BOOL is_use;
    u32 display;
    BrBtn *btn;
    const BrBtnData *data;
    BOOL (*moveFunc)(struct BrBtnWork *p_wk);
    BrPoint target;
    u32 seq;
    s16 start;
    s16 end;
    s16 count;
    s16 max;
    u32 objID;
    // Kept at the button's position while it moves, for the ball effect
    BrPoint *follow;
    BrRes *res;
    HeapID heapId;
    ClActUnit *unit;
    BmpOamSys *bmpoam;
    StrBuf *str;
} BrBtnWork;

struct BrBtnSys {
    HeapID heapId;
    BrRes *res;
    ClActUnit *unit;
    BrSeq *seq;
    u32 objID;
    // FALSE while the menu waits for a button
    BOOL isBusy;
    u32 input;
    u32 btnType;
    BOOL isValid;
    u32 data1;
    u32 data2;
    u32 trgIdx;
    BrPoint trgPos;
    u8 btn_max;
    u8 btn_num;
    u8 btn_stack_max;
    u8 btn_stack_num;
    BrBtnWork *btn_stack;
    BrBtnWork *btn;
    BrBtnDataSys *btnData;
    BmpOamSys *bmpoam;
    PrintQueue *que;
    BrMsgWin *text;
    u32 unk54;
    BrBallEffect *ballEff;
    BrBtnRecovery *recovery;
};

// The buttons of the menus behind the current one, the top first
static const BrPoint sc_stack_pos[] = {
    { 42, 169 },
    { 42, 153 },
    { 42, 137 },
    { 42, 121 },
};

static void BrBtnSys_PushStack(BrBtnSys *p_wk, const BrBtnWork *cp_btn, u32 display);
static BOOL BrBtnSys_PopStack(BrBtnSys *p_wk, BrBtnWork *p_btn);
static void BrBtnSys_LoadMenu(BrBtnSys *p_wk, u32 menuID, const BrPoint *cp_pos);
static void BrBtnRecovery_Push(BrBtnRecovery *p_wk, u16 menuID, u16 btnID);
static void BrBtnRecovery_Pop(BrBtnRecovery *p_wk);
static void BrBtnSys_SetStackPalette(BrBtnSys *p_wk, u32 mode);
static void BrBtnSys_Seq_Start(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBtnSys_Seq_Main(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBtnSys_Seq_Hide(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBtnSys_Seq_Change(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBtnSys_Seq_Show(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBtnSys_Seq_Return(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBtnSys_Seq_Stack(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBtnSys_Seq_Unstack(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBtnSys_Seq_End(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBtnSys_Seq_Info(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBtnSys_Seq_Open(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBtnSys_Seq_Close(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs);
static void BrBtnWork_Init(BrBtnWork *p_wk, const BrBtnData *cp_data, ClActUnit *unit, BmpOamSys *bmpoam, BrRes *res,
                           const StrBuf *str, u32 objID, HeapID heapId);
static void BrBtnWork_Exit(BrBtnWork *p_wk);
static BOOL BrBtnWork_GetTrg(const BrBtnWork *cp_wk, u32 x, u32 y);
static void BrBtnWork_StartMove(BrBtnWork *p_wk, u32 type, const BrPoint *cp_pos);
static BOOL BrBtnWork_MainMove(BrBtnWork *p_wk);
static void BrBtnWork_ChangeDisplay(BrBtnWork *p_wk, u32 display);
static void BrBtnWork_Copy(const BrBtnSys *cp_sys, BrBtnWork *p_dst, const BrBtnWork *cp_src, u32 display);
static void BrBtnWork_SetStackPos(BrBtnWork *p_wk, u16 idx);
static void BrBtnWork_GetPos(const BrBtnWork *cp_wk, BrPoint *p_pos);
static void BrBtnWork_SetPosY(BrBtnWork *p_wk, s16 y);
static u32 BrBtnWork_GetParam(const BrBtnWork *cp_wk, u32 paramID);
static void BrBtnWork_SetSoftPriority(BrBtnWork *p_wk, u16 priority);
static void BrBtnWork_SetBgPriority(BrBtnWork *p_wk, u32 priority);
static void BrBtnWork_SetPalette(BrBtnWork *p_wk, u32 plt);
static void BrBtnWork_SetFollow(BrBtnWork *p_wk, BrPoint *p_pos);
static BOOL BrBtnWork_Move_Target(BrBtnWork *p_wk);
static BOOL BrBtnWork_Move_Return(BrBtnWork *p_wk);
static BOOL BrBtnWork_Move_ToMain(BrBtnWork *p_wk);
static BOOL BrBtnWork_Move_ToSub(BrBtnWork *p_wk);
static BOOL BrBtnWork_Move_TargetSlow(BrBtnWork *p_wk);
static void BrBtn_SetPos(BrBtn *p_wk, s16 x, s16 y);
static void BrBtn_GetPos(const BrBtn *cp_wk, s16 *p_x, s16 *p_y);
static void BrBtn_SetSoftPriority(BrBtn *p_wk, u8 priority);
static u8 BrBtn_GetSoftPriority(const BrBtn *cp_wk);
static void BrBtn_SetBgPriority(BrBtn *p_wk, u8 priority);
static u8 BrBtn_GetBgPriority(const BrBtn *cp_wk);
static void BrBtn_SetPalette(BrBtn *p_wk, u32 plt);

BrBtnSys *BrBtnSys_Init(int menuID, ClActUnit *unit, BrRes *res, BrRecordInfo *recordInfo, BrBtnRecovery *recovery,
                        BrBallEffect *ballEff, HeapID heapId) {
    BrBtnSys *p_wk;
    int i;

    GFL_ASSERT(menuID < BR_MENUID_MAX);
    p_wk = GFL_HeapAllocate(heapId, sizeof(BrBtnSys), FALSE, "br_btn.c", 318);
    MI_CpuFill8(p_wk, 0, sizeof(BrBtnSys));
    p_wk->res = res;
    p_wk->unit = unit;
    p_wk->heapId = heapId;
    p_wk->recovery = recovery;
    p_wk->bmpoam = BmpOam_Init(heapId, unit);
    p_wk->que = func_02021998(heapId);
    p_wk->ballEff = ballEff;
    p_wk->isBusy = TRUE;
    {
        BrBtnDataSetup setup;

        MI_CpuFill8(&setup, 0, sizeof(BrBtnDataSetup));
        setup.recordInfo = recordInfo;
        p_wk->btnData = BrBtnData_Init(&setup, heapId);
    }

    if (menuID >= BR_MENUID_MUSICAL_TOP) {
        p_wk->objID = BR_RES_OBJ_MUSICAL_BTN_M;
    } else {
        p_wk->objID = BR_RES_OBJ_BROWSE_BTN_M;
    }
    BrRes_LoadOBJ(p_wk->res, p_wk->objID, heapId);
    BrRes_LoadOBJ(p_wk->res, p_wk->objID + 1, heapId);

    p_wk->btn_max = BrBtnData_GetMaxNum(p_wk->btnData);
    {
        u32 size = sizeof(BrBtnWork) * p_wk->btn_max;

        p_wk->btn = GFL_HeapAllocate(heapId, size, FALSE, "br_btn.c", 358);
        MI_CpuFill8(p_wk->btn, 0, size);
    }
    p_wk->btn_stack_max = BR_BTN_SYS_STACK_MAX;
    p_wk->btn_stack = GFL_HeapAllocate(heapId, sizeof(BrBtnWork) * BR_BTN_SYS_STACK_MAX, FALSE, "br_btn.c", 368);
    MI_CpuFill8(p_wk->btn_stack, 0, sizeof(BrBtnWork) * BR_BTN_SYS_STACK_MAX);
    p_wk->seq = BrSeq_Init(p_wk, BrBtnSys_Seq_Start, heapId);

    if (p_wk->recovery->stack_num != 0) {
        MsgData *msg = BrRes_GetMsgData(res);

        for (i = 0; i < p_wk->recovery->stack_num; i++) {
            const BrBtnData *cp_data =
                BrBtnData_GetData(p_wk->btnData, p_wk->recovery->stack[i].menuID, p_wk->recovery->stack[i].btnID);
            StrBuf *str = BrBtnData_CreateStr(p_wk->btnData, cp_data, msg, HEAPID_TAIL(heapId));
            BrBtnWork btn;

            MI_CpuFill8(&btn, 0, sizeof(BrBtnWork));
            BrBtnWork_Init(&btn, cp_data, unit, p_wk->bmpoam, res, str, p_wk->objID, heapId);
            BrBtnSys_PushStack(p_wk, &btn, CLACT_SURFACE_MAIN);
            GFL_StrBufFree(str);
        }
        for (i = 0; i < p_wk->btn_stack_num; i++) {
            BrBtnWork_SetStackPos(&p_wk->btn_stack[p_wk->btn_stack_num - i - 1], i);
        }
        BrSeq_SetNext(p_wk->seq, BrBtnSys_Seq_Unstack);
        p_wk->btnType = BR_BTN_TYPE_RETURN;
    } else {
        MsgData *msg = BrRes_GetMsgData(res);

        p_wk->btn_num = BrBtnData_GetNum(p_wk->btnData, menuID);
        for (i = 0; i < p_wk->btn_num; i++) {
            const BrBtnData *cp_data = BrBtnData_GetData(p_wk->btnData, menuID, i);
            StrBuf *str = BrBtnData_CreateStr(p_wk->btnData, cp_data, msg, HEAPID_TAIL(heapId));

            BrBtnWork_Init(&p_wk->btn[i], cp_data, unit, p_wk->bmpoam, res, str, p_wk->objID, heapId);
            GFL_StrBufFree(str);
        }
        for (i = 0; i < p_wk->btn_num; i++) {
            BrBtnWork_SetPosY(&p_wk->btn[i], 224);
        }
        BrSeq_SetNext(p_wk->seq, BrBtnSys_Seq_Open);
    }
    return p_wk;
}

void BrBtnSys_Exit(BrBtnSys *p_wk) {
    int i;

    BrSeq_Exit(p_wk->seq);
    if (p_wk->text != NULL) {
        BrText_Exit(p_wk->text, p_wk->res);
        p_wk->text = NULL;
    }
    for (i = 0; i < p_wk->btn_stack_num; i++) {
        BrBtnWork_Exit(&p_wk->btn_stack[i]);
    }
    GFL_HeapFree(p_wk->btn_stack);
    for (i = 0; i < p_wk->btn_num; i++) {
        BrBtnWork_Exit(&p_wk->btn[i]);
    }
    GFL_HeapFree(p_wk->btn);
    BrRes_UnloadOBJ(p_wk->res, p_wk->objID);
    BrRes_UnloadOBJ(p_wk->res, p_wk->objID + 1);
    BrBtnData_Exit(p_wk->btnData);
    BmpOam_Exit(p_wk->bmpoam);
    func_02021a18(p_wk->que);
    GFL_HeapFree(p_wk);
}

void BrBtnSys_Main(BrBtnSys *p_wk) {
    BrSeq_Main(p_wk->seq);
    func_02021a3c(p_wk->que);
    if (p_wk->text != NULL) {
        BrText_Main(p_wk->text);
    }
}

u32 BrBtnSys_GetInput(const BrBtnSys *cp_wk, u32 *p_data1, u32 *p_data2) {
    if (p_data1 != NULL) {
        *p_data1 = cp_wk->data1;
    }
    if (p_data2 != NULL) {
        *p_data2 = cp_wk->data2;
    }
    return cp_wk->input;
}

BOOL BrBtnSys_IsBusy(const BrBtnSys *cp_wk) {
    return cp_wk->isBusy;
}

u32 BrBtnSys_GetUnk54(const BrBtnSys *cp_wk) {
    return cp_wk->unk54;
}

static void BrBtnSys_PushStack(BrBtnSys *p_wk, const BrBtnWork *cp_btn, u32 display) {
    int i;

    GFL_ASSERT(p_wk->btn_stack_num < p_wk->btn_stack_max);
    BrBtnWork_Copy(p_wk, &p_wk->btn_stack[p_wk->btn_stack_num], cp_btn, display);
    p_wk->btn_stack_num++;
    for (i = 0; i < p_wk->btn_stack_num; i++) {
        BrBtnWork_SetSoftPriority(&p_wk->btn_stack[i], (p_wk->btn_stack_num - i) * 2 + 1);
        BrBtnWork_SetBgPriority(&p_wk->btn_stack[i], 1);
    }
}

static BOOL BrBtnSys_PopStack(BrBtnSys *p_wk, BrBtnWork *p_btn) {
    if (p_wk->btn_stack_num != 0) {
        p_wk->btn_stack_num--;
        *p_btn = p_wk->btn_stack[p_wk->btn_stack_num];
        MI_CpuFill8(&p_wk->btn_stack[p_wk->btn_stack_num], 0, sizeof(BrBtnWork));
        return TRUE;
    }
    return FALSE;
}

static void BrBtnSys_LoadMenu(BrBtnSys *p_wk, u32 menuID, const BrPoint *cp_pos) {
    int i;
    MsgData *msg;

    for (i = 0; i < p_wk->btn_max; i++) {
        BrBtnWork_Exit(&p_wk->btn[i]);
    }

    msg = BrRes_GetMsgData(p_wk->res);
    p_wk->btn_num = BrBtnData_GetNum(p_wk->btnData, menuID);
    for (i = 0; i < p_wk->btn_num; i++) {
        const BrBtnData *cp_data = BrBtnData_GetData(p_wk->btnData, menuID, i);
        StrBuf *str = BrBtnData_CreateStr(p_wk->btnData, cp_data, msg, HEAPID_TAIL(p_wk->heapId));

        BrBtnWork_Init(&p_wk->btn[i], cp_data, p_wk->unit, p_wk->bmpoam, p_wk->res, str, p_wk->objID, p_wk->heapId);
        GFL_StrBufFree(str);
        BrBtnWork_SetSoftPriority(&p_wk->btn[i], 3);
        if (cp_pos != NULL) {
            BrBtn_SetPos(p_wk->btn[i].btn, cp_pos->x, cp_pos->y);
        }
    }
}

static void BrBtnRecovery_Push(BrBtnRecovery *p_wk, u16 menuID, u16 btnID) {
    GFL_ASSERT(p_wk->stack_num < BR_BTN_SYS_STACK_MAX);
    p_wk->stack[p_wk->stack_num].menuID = menuID;
    p_wk->stack[p_wk->stack_num].btnID = btnID;
    p_wk->stack_num++;
}

static void BrBtnRecovery_Pop(BrBtnRecovery *p_wk) {
    if (p_wk->stack_num != 0) {
        p_wk->stack_num--;
    }
}

// Lights the buttons of the stack that can be pressed to return: the top one, or after a return the top two
static void BrBtnSys_SetStackPalette(BrBtnSys *p_wk, u32 mode) {
    int i;
    BOOL isActive;

    for (i = 0; i < p_wk->btn_stack_num; i++) {
        switch (mode) {
        case 0:
            isActive = i == p_wk->btn_stack_num - 1;
            break;
        case 1:
            isActive = i >= p_wk->btn_stack_num - 2;
            break;
        }
        if (isActive) {
            BrBtnWork_SetPalette(&p_wk->btn_stack[i], 0);
        } else {
            BrBtnWork_SetPalette(&p_wk->btn_stack[i], 1);
        }
    }
}

static void BrBtnSys_Seq_Start(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_Main);
}

static void BrBtnSys_Seq_Main(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBtnSys *p_wk = p_wk_adrs;
    BOOL isTrg = FALSE;
    u32 x, y;
    int i;

    p_wk->input = BR_BTN_INPUT_NONE;
    p_wk->isBusy = FALSE;

    if (func_0203dac8(&x, &y)) {
        for (i = 0; i < p_wk->btn_max; i++) {
            if (BrBtnWork_GetTrg(&p_wk->btn[i], x, y)) {
                if (BrBtnWork_GetParam(&p_wk->btn[i], BR_BTN_PARAM_VALID)) {
                    if (p_wk->text != NULL) {
                        BrText_Exit(p_wk->text, p_wk->res);
                        p_wk->text = NULL;
                    }
                    GFL_SndSEPlay(0x703);
                    p_wk->trgIdx = i;
                    isTrg = TRUE;
                    p_wk->trgPos.x = x;
                    p_wk->trgPos.y = y;
                    BrBallEff_Start(p_wk->ballEff, BR_BALL_EFFECT_TOUCH, &p_wk->trgPos);
                    BrBtnWork_SetFollow(&p_wk->btn[p_wk->trgIdx], &p_wk->trgPos);
                    p_wk->unk54 = 0;
                } else {
                    u32 type = BrBtnWork_GetParam(&p_wk->btn[i], BR_BTN_PARAM_UNVALID_TYPE);
                    u32 data = BrBtnWork_GetParam(&p_wk->btn[i], BR_BTN_PARAM_UNVALID_DATA);

                    GFL_SndSEPlay(0x704);
                    if (type == 1) {
                        p_wk->trgIdx = i;
                        BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_Info);
                    }
                }
                break;
            }
        }
    }

    if (isTrg) {
        p_wk->isBusy = TRUE;
        p_wk->btnType = BrBtnWork_GetParam(&p_wk->btn[p_wk->trgIdx], BR_BTN_PARAM_TYPE);
        p_wk->isValid = BrBtnWork_GetParam(&p_wk->btn[p_wk->trgIdx], BR_BTN_PARAM_VALID);
        p_wk->data1 = BrBtnWork_GetParam(&p_wk->btn[p_wk->trgIdx], BR_BTN_PARAM_DATA1);
        p_wk->data2 = BrBtnWork_GetParam(&p_wk->btn[p_wk->trgIdx], BR_BTN_PARAM_DATA2);
        for (i = 0; i < p_wk->btn_num; i++) {
            if (i == p_wk->trgIdx) {
                BrBtnWork_SetSoftPriority(&p_wk->btn[i], 1);
            } else {
                BrBtnWork_SetSoftPriority(&p_wk->btn[i], 3);
            }
        }

        switch (p_wk->btnType) {
        case BR_BTN_TYPE_RETURN:
            BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_Return);
            break;
        case BR_BTN_TYPE_SELECT:
        case BR_BTN_TYPE_MENU:
        case BR_BTN_TYPE_CHANGE_DISPLAY:
            BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_Hide);
            break;
        case BR_BTN_TYPE_EXIT:
            BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_Close);
            break;
        default:
            BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_End);
            break;
        }
    }
}

static void BrBtnSys_Seq_Hide(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBtnSys *p_wk = p_wk_adrs;
    int i;

    switch (*p_seq) {
    case 0: {
        BrPoint pos;

        for (i = 0; i < p_wk->btn_num; i++) {
            if (p_wk->trgIdx != i) {
                BrBtnWork_GetPos(&p_wk->btn[p_wk->trgIdx], &pos);
                BrBtnWork_StartMove(&p_wk->btn[i], BR_BTN_MOVE_TARGET, &pos);
            }
        }
        *p_seq = 1;
        break;
    }
    case 1: {
        BOOL isEnd = TRUE;

        for (i = 0; i < p_wk->btn_num; i++) {
            isEnd &= BrBtnWork_MainMove(&p_wk->btn[i]);
        }
        if (isEnd) {
            *p_seq = 2;
        }
        break;
    }
    case 2:
        // Both branches start the same sequence
        if (p_wk->btnType == BR_BTN_TYPE_CHANGE_DISPLAY) {
            BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_Change);
            return;
        } else {
            BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_Change);
        }
        break;
    }
}

static void BrBtnSys_Seq_Change(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBtnSys *p_wk = p_wk_adrs;
    int i;

    if (p_wk->btnType <= BR_BTN_TYPE_MENU) {
        BrPoint pos;
        u32 nextMenuID = p_wk->data1;

        BrBtnWork_GetPos(&p_wk->btn[p_wk->trgIdx], &pos);
        BrBtnSys_PushStack(p_wk, &p_wk->btn[p_wk->trgIdx], CLACT_SURFACE_SUB);
        BrBtnRecovery_Push(p_wk->recovery, BrBtnWork_GetParam(&p_wk->btn[p_wk->trgIdx], BR_BTN_PARAM_MENUID),
                           p_wk->trgIdx);
        MI_CpuFill8(&p_wk->btn[p_wk->trgIdx], 0, sizeof(BrBtnWork));
        pos.y = -32;
        BrBtnSys_LoadMenu(p_wk, nextMenuID, &pos);
        BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_Stack);
    } else if (p_wk->btnType == BR_BTN_TYPE_RETURN) {
        BrBtnWork btn;

        BrBtnRecovery_Pop(p_wk->recovery);
        if (BrBtnSys_PopStack(p_wk, &btn)) {
            u32 menuID = BrBtnWork_GetParam(&btn, BR_BTN_PARAM_MENUID);
            BrPoint pos;
            u32 y;

            GFL_ASSERT(btn.is_use);
            pos.x = BrBtnData_GetParam(btn.data, BR_BTN_DATA_PARAM_X);
            pos.y = BrBtnData_GetParam(btn.data, BR_BTN_DATA_PARAM_Y);
            BrBtnSys_LoadMenu(p_wk, menuID, &pos);

            y = BrBtnData_GetParam(btn.data, BR_BTN_DATA_PARAM_Y);
            for (i = 0; i < p_wk->btn_num; i++) {
                if (y == BrBtnData_GetParam(p_wk->btn[i].data, BR_BTN_DATA_PARAM_Y)) {
                    BrBtnWork_SetSoftPriority(&p_wk->btn[i], 1);
                }
            }
            BrBtnWork_Exit(&btn);
            BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_Show);
        }
    } else {
        BrBtnSys_PushStack(p_wk, &p_wk->btn[p_wk->trgIdx], CLACT_SURFACE_SUB);
        BrBtnRecovery_Push(p_wk->recovery, BrBtnWork_GetParam(&p_wk->btn[p_wk->trgIdx], BR_BTN_PARAM_MENUID),
                           p_wk->trgIdx);
        MI_CpuFill8(&p_wk->btn[p_wk->trgIdx], 0, sizeof(BrBtnWork));
        for (i = 0; i < p_wk->btn_num; i++) {
            BrBtnWork_Exit(&p_wk->btn[i]);
        }
        BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_Stack);
    }
}

static void BrBtnSys_Seq_Show(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBtnSys *p_wk = p_wk_adrs;
    int i;

    switch (*p_seq) {
    case 0:
        for (i = 0; i < p_wk->btn_num; i++) {
            BrBtnWork_StartMove(&p_wk->btn[i], BR_BTN_MOVE_RETURN, NULL);
        }
        *p_seq = 1;
        break;
    case 1: {
        BOOL isEnd = TRUE;

        for (i = 0; i < p_wk->btn_num; i++) {
            isEnd &= BrBtnWork_MainMove(&p_wk->btn[i]);
        }
        if (isEnd) {
            *p_seq = 2;
        }
        break;
    }
    case 2:
        switch (p_wk->btnType) {
        case BR_BTN_TYPE_SELECT:
        case BR_BTN_TYPE_MENU:
            BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_End);
            break;
        case BR_BTN_TYPE_CHANGE_DISPLAY:
            BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_Stack);
            break;
        default:
            BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_End);
            break;
        }
        break;
    }
}

static void BrBtnSys_Seq_Return(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBtnSys *p_wk = p_wk_adrs;
    int i;

    switch (*p_seq) {
    case 0:
        for (i = 0; i < p_wk->btn_num; i++) {
            BrPoint pos;

            BrBtnWork_GetPos(&p_wk->btn[i], &pos);
            pos.y = 224;
            BrBtnWork_StartMove(&p_wk->btn[i], BR_BTN_MOVE_TARGET_SLOW, &pos);
        }
        *p_seq = 1;
        break;
    case 1: {
        BOOL isEnd = TRUE;

        for (i = 0; i < p_wk->btn_num; i++) {
            isEnd &= BrBtnWork_MainMove(&p_wk->btn[i]);
        }
        if (isEnd) {
            *p_seq = 2;
        }
        break;
    }
    case 2:
        BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_Unstack);
        break;
    }
}

static void BrBtnSys_Seq_Stack(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBtnSys *p_wk = p_wk_adrs;
    int i;

    switch (*p_seq) {
    case 0:
        for (i = 0; i < p_wk->btn_stack_num; i++) {
            if (i == p_wk->btn_stack_num - 1) {
                BrBtnWork_StartMove(&p_wk->btn_stack[i], BR_BTN_MOVE_TO_MAIN, &sc_stack_pos[0]);
            } else {
                BrBtnWork_StartMove(&p_wk->btn_stack[i], BR_BTN_MOVE_TARGET_SLOW,
                                    &sc_stack_pos[p_wk->btn_stack_num - i - 1]);
            }
        }
        *p_seq = 1;
        break;
    case 1: {
        BOOL isEnd = TRUE;

        for (i = 0; i < p_wk->btn_stack_num; i++) {
            isEnd &= BrBtnWork_MainMove(&p_wk->btn_stack[i]);
        }
        if (isEnd) {
            BrBtnSys_SetStackPalette(p_wk, 0);
            *p_seq = 2;
        }
        break;
    }
    case 2: {
        BrBtnWork *p_top = &p_wk->btn_stack[p_wk->btn_stack_num - 1];
        u32 type = BrBtnWork_GetParam(p_top, BR_BTN_PARAM_TYPE);
        u16 msgID = BrBtnWork_GetParam(p_top, BR_BTN_PARAM_DATA2);

        if (type == BR_BTN_TYPE_MENU) {
            if (p_wk->text == NULL) {
                p_wk->text = BrText_Init(p_wk->res, p_wk->que, p_wk->heapId);
            }
            BrText_Print(p_wk->text, p_wk->res, msgID);
        }
        *p_seq = 3;
        break;
    }
    case 3:
        switch (p_wk->btnType) {
        case BR_BTN_TYPE_SELECT:
        case BR_BTN_TYPE_MENU:
            BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_Show);
            break;
        default:
            BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_End);
        }
        break;
    }
}

static void BrBtnSys_Seq_Unstack(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBtnSys *p_wk = p_wk_adrs;
    int i;

    switch (*p_seq) {
    case 0:
        for (i = 0; i < p_wk->btn_stack_num; i++) {
            if (i == p_wk->btn_stack_num - 1) {
                BrPoint pos;

                pos.x = BrBtnData_GetParam(p_wk->btn_stack[i].data, BR_BTN_DATA_PARAM_X);
                pos.y = BrBtnData_GetParam(p_wk->btn_stack[i].data, BR_BTN_DATA_PARAM_Y);
                BrBtnWork_StartMove(&p_wk->btn_stack[i], BR_BTN_MOVE_TO_SUB, &pos);
                BrBtnWork_SetSoftPriority(&p_wk->btn_stack[i], 1);
            } else {
                BrBtnWork_StartMove(&p_wk->btn_stack[i], BR_BTN_MOVE_TARGET_SLOW,
                                    &sc_stack_pos[p_wk->btn_stack_num - i - 2]);
            }
        }
        for (i = 0; i < p_wk->btn_num; i++) {
            BrBtnWork_SetSoftPriority(&p_wk->btn[i], 3);
        }
        *p_seq = 1;
        break;
    case 1: {
        BOOL isEnd = TRUE;

        for (i = 0; i < p_wk->btn_stack_num; i++) {
            isEnd &= BrBtnWork_MainMove(&p_wk->btn_stack[i]);
        }
        if (isEnd) {
            BrBtnSys_SetStackPalette(p_wk, 1);
            *p_seq = 2;
        }
        break;
    }
    case 2:
        BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_Change);
        break;
    }
}

static void BrBtnSys_Seq_End(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBtnSys *p_wk = p_wk_adrs;

    switch (*p_seq) {
    case 0:
        if (p_wk->isValid) {
            switch (p_wk->btnType) {
            case BR_BTN_TYPE_SELECT:
            case BR_BTN_TYPE_MENU:
            case BR_BTN_TYPE_RETURN:
                break;
            case BR_BTN_TYPE_EXIT:
                p_wk->input = BR_BTN_INPUT_EXIT;
                break;
            case BR_BTN_TYPE_CHANGE_DISPLAY:
                p_wk->input = BR_BTN_INPUT_SELECT;
                break;
            }
        }
        BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_Main);
        break;
    }
}

static void BrBtnSys_Seq_Info(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBtnSys *p_wk = p_wk_adrs;

    switch (*p_seq) {
    case 0: {
        u32 msgID = BrBtnWork_GetParam(&p_wk->btn[p_wk->trgIdx], BR_BTN_PARAM_UNVALID_DATA);

        if (p_wk->text == NULL) {
            p_wk->text = BrText_Init(p_wk->res, p_wk->que, p_wk->heapId);
        }
        BrText_Print(p_wk->text, p_wk->res, msgID);
        *p_seq = 1;
        break;
    }
    case 1:
        *p_seq = 2;
        break;
    case 2:
        BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_Main);
        break;
    }
}

static void BrBtnSys_Seq_Open(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBtnSys *p_wk = p_wk_adrs;
    int i;

    switch (*p_seq) {
    case 0:
        for (i = 0; i < p_wk->btn_num; i++) {
            BrPoint pos;

            pos.x = BrBtnWork_GetParam(&p_wk->btn[i], BR_BTN_PARAM_X);
            pos.y = BrBtnWork_GetParam(&p_wk->btn[i], BR_BTN_PARAM_Y);
            BrBtnWork_StartMove(&p_wk->btn[i], BR_BTN_MOVE_TARGET_SLOW, &pos);
        }
        *p_seq = 1;
        break;
    case 1: {
        BOOL isEnd = TRUE;

        for (i = 0; i < p_wk->btn_num; i++) {
            isEnd &= BrBtnWork_MainMove(&p_wk->btn[i]);
        }
        if (isEnd) {
            *p_seq = 2;
        }
        break;
    }
    case 2:
        BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_Start);
        break;
    }
}

static void BrBtnSys_Seq_Close(BrSeq *p_seqwk, u32 *p_seq, void *p_wk_adrs) {
    BrBtnSys *p_wk = p_wk_adrs;
    int i;

    switch (*p_seq) {
    case 0:
        for (i = 0; i < p_wk->btn_num; i++) {
            BrPoint pos;

            BrBtnWork_GetPos(&p_wk->btn[i], &pos);
            pos.y = 224;
            BrBtnWork_StartMove(&p_wk->btn[i], BR_BTN_MOVE_TARGET_SLOW, &pos);
        }
        *p_seq = 1;
        break;
    case 1: {
        BOOL isEnd = TRUE;

        for (i = 0; i < p_wk->btn_num; i++) {
            isEnd &= BrBtnWork_MainMove(&p_wk->btn[i]);
        }
        if (isEnd) {
            *p_seq = 2;
        }
        break;
    }
    case 2:
        BrSeq_SetNext(p_seqwk, BrBtnSys_Seq_End);
        break;
    }
}

static void BrBtnWork_Init(BrBtnWork *p_wk, const BrBtnData *cp_data, ClActUnit *unit, BmpOamSys *bmpoam, BrRes *res,
                           const StrBuf *str, u32 objID, HeapID heapId) {
    GFL_ASSERT(p_wk->is_use == FALSE);
    MI_CpuFill8(p_wk, 0, sizeof(BrBtnWork));
    p_wk->is_use = TRUE;
    p_wk->display = CLACT_SURFACE_SUB;
    p_wk->data = cp_data;
    p_wk->objID = objID;
    p_wk->res = res;
    p_wk->heapId = heapId;
    p_wk->unit = unit;
    p_wk->bmpoam = bmpoam;
    p_wk->str = GFL_StrBufClone(str, heapId);
    {
        ClActorSetup setup;
        BrResObjData obj;

        MI_CpuFill8(&setup, 0, sizeof(ClActorSetup));
        setup.x = BrBtnData_GetParam(p_wk->data, BR_BTN_DATA_PARAM_X);
        setup.y = BrBtnData_GetParam(p_wk->data, BR_BTN_DATA_PARAM_Y);
        setup.sequence = BrBtnData_GetParam(p_wk->data, BR_BTN_DATA_PARAM_ANMSEQ);
        setup.priority = 1;
        BrRes_GetObjData(res, p_wk->objID + 1, &obj);
        p_wk->btn = BrBtn_Init(&setup, p_wk->str, 160, p_wk->display, unit, bmpoam, BrRes_GetFont(res), &obj,
                               HEAPID_TAIL(heapId));
    }
}

static void BrBtnWork_Exit(BrBtnWork *p_wk) {
    if (p_wk->is_use) {
        GFL_StrBufFree(p_wk->str);
        BrBtn_Exit(p_wk->btn);
        MI_CpuFill8(p_wk, 0, sizeof(BrBtnWork));
    }
}

static BOOL BrBtnWork_GetTrg(const BrBtnWork *cp_wk, u32 x, u32 y) {
    if (cp_wk->is_use) {
        return BrBtn_GetHit(cp_wk->btn, x, y);
    }
    return FALSE;
}

static void BrBtnWork_StartMove(BrBtnWork *p_wk, u32 type, const BrPoint *cp_pos) {
    p_wk->seq = 0;
    if (cp_pos != NULL) {
        p_wk->target = *cp_pos;
    }
    switch (type) {
    case BR_BTN_MOVE_TARGET:
        p_wk->moveFunc = BrBtnWork_Move_Target;
        break;
    case BR_BTN_MOVE_RETURN:
        p_wk->moveFunc = BrBtnWork_Move_Return;
        break;
    case BR_BTN_MOVE_TO_MAIN:
        p_wk->moveFunc = BrBtnWork_Move_ToMain;
        break;
    case BR_BTN_MOVE_TO_SUB:
        p_wk->moveFunc = BrBtnWork_Move_ToSub;
        break;
    case BR_BTN_MOVE_TARGET_SLOW:
        p_wk->moveFunc = BrBtnWork_Move_TargetSlow;
        break;
    }
}

static BOOL BrBtnWork_MainMove(BrBtnWork *p_wk) {
    if (p_wk->moveFunc != NULL) {
        BOOL ret = p_wk->moveFunc(p_wk);

        if (ret) {
            p_wk->moveFunc = NULL;
        }
        if (p_wk->follow != NULL) {
            s16 x, y;

            BrBtn_GetPos(p_wk->btn, &x, &y);
            p_wk->follow->x = x + 80;
            p_wk->follow->y = y;
        }
        return ret;
    }
    return TRUE;
}

static void BrBtnWork_ChangeDisplay(BrBtnWork *p_wk, u32 display) {
    if (p_wk->display != display) {
        s16 x, y;
        u8 softPriority, bgPriority;
        s32 ofs;
        ClActorSetup setup;
        BrResObjData obj;
        Font *font;

        p_wk->display = display;
        BrBtn_GetPos(p_wk->btn, &x, &y);
        softPriority = BrBtn_GetSoftPriority(p_wk->btn);
        bgPriority = BrBtn_GetBgPriority(p_wk->btn);
        BrBtn_Exit(p_wk->btn);
        p_wk->follow = NULL;

        if (display == CLACT_SURFACE_MAIN) {
            ofs = 512;
        } else {
            ofs = -512;
        }
        MI_CpuFill8(&setup, 0, sizeof(ClActorSetup));
        setup.x = x;
        setup.y = y + ofs;
        setup.sequence = BrBtnData_GetParam(p_wk->data, BR_BTN_DATA_PARAM_ANMSEQ);
        setup.priority = softPriority;
        setup.bgPriority = bgPriority;
        BrRes_GetObjData(p_wk->res, p_wk->objID + display, &obj);
        font = BrRes_GetFont(p_wk->res);
        // The message data and the label's message ID are read and left unused: the label is kept
        BrRes_GetMsgData(p_wk->res);
        BrBtnData_GetParam(p_wk->data, BR_BTN_DATA_PARAM_MSGID);
        p_wk->btn = BrBtn_Init(&setup, p_wk->str, 160, display, p_wk->unit, p_wk->bmpoam, font, &obj, p_wk->heapId);
    }
}

static void BrBtnWork_Copy(const BrBtnSys *cp_sys, BrBtnWork *p_dst, const BrBtnWork *cp_src, u32 display) {
    GFL_ASSERT(p_dst->is_use == FALSE);
    GFL_ASSERT(cp_src->is_use == TRUE);
    *p_dst = *cp_src;
    if (cp_src->display != display) {
        BrBtnWork_ChangeDisplay(p_dst, display);
    }
}

static void BrBtnWork_SetStackPos(BrBtnWork *p_wk, u16 idx) {
    s16 x = BrBtnData_GetParam(p_wk->data, BR_BTN_DATA_PARAM_X);

    BrBtn_SetPos(p_wk->btn, x, 169 - idx * 16);
}

static void BrBtnWork_GetPos(const BrBtnWork *cp_wk, BrPoint *p_pos) {
    s16 x, y;

    BrBtn_GetPos(cp_wk->btn, &x, &y);
    p_pos->x = x;
    p_pos->y = y;
}

static void BrBtnWork_SetPosY(BrBtnWork *p_wk, s16 y) {
    s16 x = BrBtnData_GetParam(p_wk->data, BR_BTN_DATA_PARAM_X);

    BrBtn_SetPos(p_wk->btn, x, y);
}

static u32 BrBtnWork_GetParam(const BrBtnWork *cp_wk, u32 paramID) {
    u32 ret;

    switch (paramID) {
    case BR_BTN_PARAM_TYPE:
        ret = BrBtnData_GetParam(cp_wk->data, BR_BTN_DATA_PARAM_TYPE);
        break;
    case BR_BTN_PARAM_DATA1:
        ret = BrBtnData_GetParam(cp_wk->data, BR_BTN_DATA_PARAM_DATA1);
        break;
    case BR_BTN_PARAM_DATA2:
        ret = BrBtnData_GetParam(cp_wk->data, BR_BTN_DATA_PARAM_DATA2);
        break;
    case BR_BTN_PARAM_UNVALID_TYPE:
        ret = BrBtnData_GetParam(cp_wk->data, BR_BTN_DATA_PARAM_UNVALID_TYPE);
        break;
    case BR_BTN_PARAM_UNVALID_DATA:
        ret = BrBtnData_GetParam(cp_wk->data, BR_BTN_DATA_PARAM_UNVALID_DATA);
        break;
    case BR_BTN_PARAM_VALID:
        ret = BrBtnData_GetParam(cp_wk->data, BR_BTN_DATA_PARAM_VALID);
        break;
    case BR_BTN_PARAM_MENUID:
        ret = BrBtnData_GetParam(cp_wk->data, BR_BTN_DATA_PARAM_MENUID);
        break;
    case BR_BTN_PARAM_VALID_TYPE:
        ret = BrBtnData_GetParam(cp_wk->data, BR_BTN_DATA_PARAM_VALID_TYPE);
        break;
    case BR_BTN_PARAM_X:
        ret = BrBtnData_GetParam(cp_wk->data, BR_BTN_DATA_PARAM_X);
        break;
    case BR_BTN_PARAM_Y:
        ret = BrBtnData_GetParam(cp_wk->data, BR_BTN_DATA_PARAM_Y);
        break;
    default:
        GFL_ASSERT(0);
        ret = 0;
    }
    return ret;
}

static void BrBtnWork_SetSoftPriority(BrBtnWork *p_wk, u16 priority) {
    BrBtn_SetSoftPriority(p_wk->btn, priority);
}

static void BrBtnWork_SetBgPriority(BrBtnWork *p_wk, u32 priority) {
    BrBtn_SetBgPriority(p_wk->btn, priority);
}

static void BrBtnWork_SetPalette(BrBtnWork *p_wk, u32 plt) {
    BrBtn_SetPalette(p_wk->btn, plt);
}

static void BrBtnWork_SetFollow(BrBtnWork *p_wk, BrPoint *p_pos) {
    p_wk->follow = p_pos;
}

static BOOL BrBtnWork_Move_Target(BrBtnWork *p_wk) {
    switch (p_wk->seq) {
    case 0: {
        BrPoint pos;

        BrBtnWork_GetPos(p_wk, &pos);
        p_wk->start = pos.y;
        p_wk->end = p_wk->target.y;
        p_wk->count = 0;
        p_wk->max = 10;
        p_wk->seq = 1;
        break;
    }
    case 1: {
        s16 y = p_wk->start + (p_wk->end - p_wk->start) * p_wk->count / p_wk->max;
        s16 x;

        BrBtn_GetPos(p_wk->btn, &x, NULL);
        BrBtn_SetPos(p_wk->btn, x, y);
        if (p_wk->count++ >= p_wk->max) {
            p_wk->seq = 2;
        }
        break;
    }
    case 2:
        return TRUE;
    }
    return FALSE;
}

static BOOL BrBtnWork_Move_Return(BrBtnWork *p_wk) {
    switch (p_wk->seq) {
    case 0: {
        BrPoint pos;
        s16 end;

        BrBtnWork_GetPos(p_wk, &pos);
        end = BrBtnData_GetParam(p_wk->data, BR_BTN_DATA_PARAM_Y);
        p_wk->start = pos.y;
        p_wk->end = end;
        p_wk->count = 0;
        p_wk->max = 16;
        p_wk->seq = 1;
        break;
    }
    case 1: {
        s16 y = p_wk->start + (p_wk->end - p_wk->start) * p_wk->count / p_wk->max;
        s16 x;

        BrBtn_GetPos(p_wk->btn, &x, NULL);
        BrBtn_SetPos(p_wk->btn, x, y);
        if (p_wk->count++ >= p_wk->max) {
            p_wk->seq = 2;
        }
        break;
    }
    case 2:
        return TRUE;
    }
    return FALSE;
}

static BOOL BrBtnWork_Move_ToMain(BrBtnWork *p_wk) {
    switch (p_wk->seq) {
    case 0: {
        BrPoint pos;

        BrBtnWork_GetPos(p_wk, &pos);
        p_wk->start = pos.y;
        p_wk->end = -32;
        p_wk->count = 0;
        p_wk->max = 14;
        p_wk->seq = 1;
        break;
    }
    case 1: {
        s16 y = p_wk->start + (p_wk->end - p_wk->start) * p_wk->count / p_wk->max;
        s16 x;

        BrBtn_GetPos(p_wk->btn, &x, NULL);
        BrBtn_SetPos(p_wk->btn, x, y);
        if (p_wk->count++ >= p_wk->max) {
            p_wk->seq = 2;
        }
        break;
    }
    case 2:
        BrBtnWork_ChangeDisplay(p_wk, CLACT_SURFACE_MAIN);
        p_wk->seq = 3;
        break;
    case 3: {
        BrPoint pos;

        BrBtnWork_GetPos(p_wk, &pos);
        p_wk->start = pos.y;
        p_wk->end = p_wk->target.y;
        p_wk->count = 0;
        p_wk->max = 14;
        p_wk->seq = 4;
        break;
    }
    case 4: {
        s16 y = p_wk->start + (p_wk->end - p_wk->start) * p_wk->count / p_wk->max;
        s16 x;

        BrBtn_GetPos(p_wk->btn, &x, NULL);
        BrBtn_SetPos(p_wk->btn, x, y);
        if (p_wk->count++ >= p_wk->max) {
            p_wk->seq = 5;
        }
        break;
    }
    case 5:
        return TRUE;
    }
    return FALSE;
}

static BOOL BrBtnWork_Move_ToSub(BrBtnWork *p_wk) {
    switch (p_wk->seq) {
    case 0: {
        BrPoint pos;

        BrBtnWork_GetPos(p_wk, &pos);
        p_wk->start = pos.y;
        p_wk->end = 224;
        p_wk->count = 0;
        p_wk->max = 14;
        p_wk->seq = 1;
        break;
    }
    case 1: {
        s16 y = p_wk->start + (p_wk->end - p_wk->start) * p_wk->count / p_wk->max;
        s16 x;

        BrBtn_GetPos(p_wk->btn, &x, NULL);
        BrBtn_SetPos(p_wk->btn, x, y);
        if (p_wk->count++ >= p_wk->max) {
            p_wk->seq = 2;
        }
        break;
    }
    case 2:
        BrBtnWork_ChangeDisplay(p_wk, CLACT_SURFACE_SUB);
        p_wk->seq = 3;
        break;
    case 3: {
        BrPoint pos;

        BrBtnWork_GetPos(p_wk, &pos);
        p_wk->start = pos.y;
        p_wk->end = p_wk->target.y;
        p_wk->count = 0;
        p_wk->max = 14;
        p_wk->seq = 4;
        break;
    }
    case 4: {
        s16 y = p_wk->start + (p_wk->end - p_wk->start) * p_wk->count / p_wk->max;
        s16 x;

        BrBtn_GetPos(p_wk->btn, &x, NULL);
        BrBtn_SetPos(p_wk->btn, x, y);
        if (p_wk->count++ >= p_wk->max) {
            p_wk->seq = 5;
        }
        break;
    }
    case 5:
        return TRUE;
    }
    return FALSE;
}

static BOOL BrBtnWork_Move_TargetSlow(BrBtnWork *p_wk) {
    switch (p_wk->seq) {
    case 0: {
        BrPoint pos;

        BrBtnWork_GetPos(p_wk, &pos);
        p_wk->start = pos.y;
        p_wk->end = p_wk->target.y;
        p_wk->count = 0;
        p_wk->max = 26;
        p_wk->seq = 1;
        break;
    }
    case 1: {
        s16 y = p_wk->start + (p_wk->end - p_wk->start) * p_wk->count / p_wk->max;
        s16 x;

        BrBtn_GetPos(p_wk->btn, &x, NULL);
        BrBtn_SetPos(p_wk->btn, x, y);
        if (p_wk->count++ >= p_wk->max) {
            p_wk->seq = 2;
        }
        break;
    }
    case 2:
        return TRUE;
    }
    return FALSE;
}

BrBtn *BrBtn_InitEx(const ClActorSetup *setup, u16 msgID, u16 width, u32 display, ClActUnit *unit, BmpOamSys *bmpoam,
                    Font *font, MsgData *msg, const BrResObjData *obj, HeapID heapId) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(msg, msgID);
    BrBtn *p_wk = BrBtn_Init(setup, str, width, display, unit, bmpoam, font, obj, heapId);

    GFL_StrBufFree(str);
    return p_wk;
}

BrBtn *BrBtn_Init(const ClActorSetup *setup, const StrBuf *str, u16 width, u32 display, ClActUnit *unit,
                  BmpOamSys *bmpoam, Font *font, const BrResObjData *obj, HeapID heapId) {
    BrBtn *p_wk = GFL_HeapAllocate(heapId, sizeof(BrBtn), FALSE, "br_btn.c", 2618);
    BmpOamActorSetup actorSetup;

    MI_CpuFill8(p_wk, 0, sizeof(BrBtn));
    p_wk->width = width;
    p_wk->display = display;
    p_wk->clwk = func_0204c040(unit, obj->chr, obj->plt, obj->cell, setup, display, heapId);
    func_0204c318(p_wk->clwk, TRUE);
    p_wk->bmp = GFL_BitmapCreate(p_wk->width / 8, 2, 0x20, heapId);
    GFL_TextRendererDrawToBitmapEx(p_wk->bmp, 1, 0, str, font, 0x3da0);

    MI_CpuFill8(&actorSetup, 0, sizeof(BmpOamActorSetup));
    actorSetup.bitmap = p_wk->bmp;
    actorSetup.x = setup->x + 32;
    actorSetup.y = setup->y - 8;
    actorSetup.palette = obj->plt;
    actorSetup.priority = setup->priority - 1;
    actorSetup.surface = display;
    actorSetup.vramType = display;
    actorSetup.bgPriority = setup->bgPriority;
    actorSetup.paletteOffset = 1;
    p_wk->oam = BmpOam_ActorAdd(bmpoam, &actorSetup);
    BmpOam_ActorBmpTrans(p_wk->oam);
    BmpOam_ActorSetObjMode(p_wk->oam, GX_OAM_MODE_XLU);
    return p_wk;
}

void BrBtn_Exit(BrBtn *p_wk) {
    BmpOam_ActorDel(p_wk->oam);
    GFL_BitmapFree(p_wk->bmp);
    func_0204c108(p_wk->clwk);
    GFL_HeapFree(p_wk);
}

BOOL BrBtn_GetTrg(const BrBtn *cp_wk, u32 x, u32 y) {
    BOOL ret = BrBtn_GetHit(cp_wk, x, y);

    if (ret) {
        GFL_SndSEPlay(0x703);
    }
    return ret;
}

// Whether the point is on the button, each range tested as one unsigned comparison
BOOL BrBtn_GetHit(const BrBtn *cp_wk, u32 x, u32 y) {
    ClActorPos pos;
    s32 left, right, top, bottom;

    func_0204c178(cp_wk->clwk, &pos, cp_wk->display);
    left = pos.x;
    right = pos.x + cp_wk->width;
    top = pos.y - 16;
    bottom = pos.y + 16;
    return ((u32)(x - left) <= (u32)(right - left)) & ((u32)(y - top) <= (u32)(bottom - top));
}

static void BrBtn_SetPos(BrBtn *p_wk, s16 x, s16 y) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    func_0204c140(p_wk->clwk, &pos, p_wk->display);
    BmpOam_ActorSetPos(p_wk->oam, pos.x + 32, pos.y - 8);
}

static void BrBtn_GetPos(const BrBtn *cp_wk, s16 *p_x, s16 *p_y) {
    ClActorPos pos;

    func_0204c178(cp_wk->clwk, &pos, cp_wk->display);
    if (p_x != NULL) {
        *p_x = pos.x;
    }
    if (p_y != NULL) {
        *p_y = pos.y;
    }
}

static void BrBtn_SetSoftPriority(BrBtn *p_wk, u8 priority) {
    func_0204c438(p_wk->clwk, priority);
    BmpOam_ActorSetPriority(p_wk->oam, priority - 1);
}

static u8 BrBtn_GetSoftPriority(const BrBtn *cp_wk) {
    return func_0204c45c(cp_wk->clwk);
}

static void BrBtn_SetBgPriority(BrBtn *p_wk, u8 priority) {
    func_0204c468(p_wk->clwk, priority);
    BmpOam_ActorSetBgPriority(p_wk->oam, priority);
}

static u8 BrBtn_GetBgPriority(const BrBtn *cp_wk) {
    return func_0204c47c(cp_wk->clwk);
}

static void BrBtn_SetPalette(BrBtn *p_wk, u32 plt) {
    func_0204c378(p_wk->clwk, (u8)plt, 0);
    BmpOam_ActorSetPaletteOffset(p_wk->oam, (u8)(plt + 1));
}
