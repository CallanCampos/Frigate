#include "types.h"
#include "app/bag.h"
#include "battle/b_bag_main.h"
#include "battle/b_choice_main.h"
#include "battle/b_plist_main.h"
#include "battle/b_scenario_main.h"
#include "battle/btl_client.h"
#include "battle/btl_main.h"
#include "battle/btl_poke_select.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_string.h"
#include "battle/btl_tables.h"
#include "battle/btlv.h"
#include "battle/btlv_clact.h"
#include "battle/btlv_effect.h"
#include "battle/btlv_scd.h"
#include "battle/tr_ai.h"
#include "battle/trainer_data.h"
#include "constants/arc.h"
#include "constants/battle.h"
#include "constants/text_banks.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "pml/move.h"
#include "system/gf_font.h"
#include "text/system/btlv_core.h"

// The procedure each display command starts
typedef struct BtlvCmdEntry {
    u32 cmd;
    BtlvMainProc proc;
} BtlvCmdEntry;

typedef struct BtlvViewParam {
    u32 unk00;
    u32 unk04;
    u32 unk08;
    u32 unk0C;
} BtlvViewParam;

struct BtlvCore {
    BtlMainModule *mainModule;
    BtlClient *client;
    BtlPokeCon *pokeCon;
    u8 clientId;
    u32 cmd;
    BtlvMainProc mainProc;
    s32 seq;
    BtlvSubProc subProc;
    u8 work[0x80];
    StrBuf *strbuf;
    Font *font;
    Font *font2;
    BattleAction *unkB8;
    BattleMon *mon;
    u32 monId;
    u32 result;
    BBagParam ov286;
    BPlistParam ov287;
    BScenarioParam ov288;
    BChoiceParam ov289;
    BtlvPokeSelectParam *selectParam;
    BtlvViewParam unk1A0;
    BtlvSelectTargetParam *unk1B0;
    u8 subSeq;
    u8 unk1B5;
    u8 unk1B6[6];
    TCBExManager *tcbManager;
    BtlvScu *scu;
    BtlvScd *scd;
    HeapID heapId;
};

// The work of the capture demo's procedure
typedef struct BtlvCaptureDemoWork {
    MsgData *msgData;
    s32 wait;
    u8 savedKeys;
} BtlvCaptureDemoWork;

typedef struct BtlvPosEffectWork {
    u16 unk00;
    u32 type;
    u16 unk08;
    u8 pos;
} BtlvPosEffectWork;

typedef struct BtlvMultiEffectWork {
    u8 pos[6];
    u8 count;
    u16 seq;
    u32 type;
    u16 unk10;
} BtlvMultiEffectWork;

typedef struct BtlvMonEffectWork {
    u8 pos;
    u8 unk01;
    u8 unk02;
    u8 monId;
} BtlvMonEffectWork;

typedef struct BtlvSwapWork {
    u8 clientId;
    u8 unk01;
    u8 unk02;
    u8 slot1;
    u8 slot2;
    u8 pos1;
    u8 pos2;
} BtlvSwapWork;

typedef struct BtlvRotateWork {
    u8 clientId;
    u8 unk01;
    u8 viewPos[3];
    u8 pos[3];
} BtlvRotateWork;

typedef struct BtlvTwoStringWork {
    StrBuf *first;
    StrBuf *second;
    u32 unk08;
} BtlvTwoStringWork;

// The display commands that run a procedure of their own
const BtlvCmdEntry data_ov167_021da8c4[2] = {
    { 0, BtlvCore_CmdBattleStart },
    { 1, BtlvCore_CmdCaptureDemo },
};

const BGSysLCDConfig data_ov167_021da8d4 = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_3, GX_BG0_AS_3D };

const ClActSysSetup data_ov167_021da8e4 = { 0, 0, 0, 512, 4, 124, 4, 124, 0, 56, 48, 48, 48, 16, 16 };

const BGSysVRAMConfig data_ov167_021da900 = {
    GX_VRAM_BG_128_D,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E,  GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_01_AB, GX_VRAM_TEXPLTT_01_FG,   GX_OBJVRAMMODE_CHAR_1D_64K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static inline void BtlvCore_SetSubProc(BtlvCore *core, BtlvSubProcFn main) {
    core->subProc.init = NULL;
    core->subProc.main = main;
    core->subProc.arg = core;
    core->subProc.seq = 0;
}

static inline BOOL BtlvCore_RunSubProc(BtlvCore *core) {
    if (core->subProc.init != NULL) {
        if (core->subProc.init(&core->subProc.seq, core->subProc.arg)) {
            core->subProc.init = NULL;
            core->subProc.seq = 0;
        }
        return FALSE;
    }
    if (core->subProc.main != NULL) {
        if (core->subProc.main(&core->subProc.seq, core->subProc.arg)) {
            core->subProc.main = NULL;
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

void BtlvCore_InitSystem(HeapID heapId) {
    BtlvCore_InitGraphics(heapId);
    GFL_OvlLoad(OVERLAY_ID(168));
    GX_SetBankForLCDC(0x80);
    GFL_OvlLoad(OVERLAY_ID(169));
    if (!GFL_NetIsWifi()) {
        GFL_OvlLoad(OVERLAY_TR_AI);
    }
}

void BtlvCore_ExitSystem(void) {
    BtlvCore_ExitGraphics();
    GFL_OvlUnload(OVERLAY_ID(168));
    GFL_OvlUnload(OVERLAY_ID(169));
    if (!GFL_NetIsWifi()) {
        GFL_OvlUnload(OVERLAY_TR_AI);
    }
}

void BtlvCore_ResetGraphics(HeapID heapId) {
    BtlvCore_ExitGraphics();
    BtlvCore_InitGraphics(heapId);
}

void BtlvCore_InitGraphics(HeapID heapId) {
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    GFL_TextRndInitColorIndexLUT();
    GFL_BGSysSetVRAMBanks(&data_ov167_021da900);
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
    GFL_BGSysSetLCDConfig(&data_ov167_021da8d4);
    GFL_G3DSysCreate(FALSE, 2, 0, 1, 0, heapId, NULL);
    GFL_G3DSysSetSwapBufferParams(1, 0);
    G3X_AlphaBlend(TRUE);
    G3X_EdgeMarking(FALSE);
    G3X_AntiAlias(TRUE);
    G3X_SetFog(FALSE, 0, 0, 0);
    G2_SetBG0Priority(1);
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1,
                       GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 |
                           GX_BLEND_PLANEMASK_OBJ | GX_BLEND_PLANEMASK_BD,
                       0x1f, 3);
    ClActSys_Create(&data_ov167_021da8e4, &data_ov167_021da900, heapId);
}

void BtlvCore_ExitGraphics(void) {
    BtlvEffect_Exit();
    ClActSys_Delete();
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
    GFL_G3DSysFree();
}

BtlvCore *BtlvCore_Create(BtlMainModule *mainModule, BtlClient *client, BtlPokeCon *pokeCon, u32 arg3, HeapID heapId) {
    BtlvCore *core;

    core = GFL_HeapAllocate(heapId, sizeof(BtlvCore), FALSE, "btlv_core.c", 307);
    core->mainModule = mainModule;
    core->client = client;
    core->pokeCon = pokeCon;
    core->clientId = BattleClient_GetClientId(client);
    core->cmd = 4;
    core->heapId = heapId;
    core->strbuf = GFL_StrBufCreate(0x300, heapId);
    core->font = GFL_FontCreate(0x17, 0, 0, 0, heapId);
    core->font2 = GFL_FontCreate(0x17, 2, 0, 0, heapId);
    core->tcbManager = GFL_TCBExMgrCreate(heapId, heapId, 0x40, 0x80);
    core->scu = BtlvScu_Create(core, core->mainModule, pokeCon, core->tcbManager, core->font, core->font2,
                               core->clientId, heapId);
    core->scd =
        BtlvScd_Create(core, core->mainModule, pokeCon, core->tcbManager, core->font, core->client, arg3, heapId);
    core->mainProc = NULL;
    core->seq = 0;
    core->subSeq = 0;
    BtlString_Init(mainModule, core->clientId, pokeCon, heapId);
    return core;
}

void BtlvCore_Delete(BtlvCore *core) {
    BtlString_Exit();
    BtlvScd_Delete(core->scd);
    BtlvScu_Delete(core->scu);
    GFL_TCBExMgrFree(core->tcbManager);
    GFL_StrBufFree(core->strbuf);
    GFL_FontFree(core->font);
    GFL_FontFree(core->font2);
    GFL_HeapFree(core);
}

void BtlvCore_Update(BtlvCore *core) {
    GFL_TCBExMgrUpdate(core->tcbManager);
    BtlvEffect_Main();
}

void BtlvCore_StartCommand(BtlvCore *core, u32 cmd) {
    u32 i;

    for (i = 0; i < 2; i++) {
        if (cmd == data_ov167_021da8c4[i].cmd) {
            core->cmd = cmd;
            core->mainProc = data_ov167_021da8c4[i].proc;
            core->seq = 0;
            return;
        }
    }
}

BOOL BtlvCore_WaitCommand(BtlvCore *core) {
    if (core->cmd != 4) {
        if (core->mainProc(core, &core->seq, core->work)) {
            core->cmd = 4;
            core->mainProc = NULL;
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

void *BtlvCore_GetWork(BtlvCore *core, u32 size) {
    return core->work;
}

BOOL BtlvCore_CmdBattleStart(BtlvCore *core, s32 *seq, void *work) {
    void *data;

    switch (*seq) {
    case 0:
        data = BtlvEffect_CreateSetup(core->mainModule, core->scu, core->heapId);
        BtlvEffect_Init(data, core->font2, core->heapId);
        GFL_HeapFree(data);
        BtlvScu_Setup(core->scu);
        BtlvScd_CreateInput(core->scd);
        (*seq)++;
        break;
    case 1:
        BtlvScu_StartEncount(core->scu, BattleClient_IsChapterSkipping(core->client));
        (*seq)++;
        break;
    case 2:
        if (BtlvScu_WaitEncount(core->scu)) {
            if (BattleClient_IsRecPlay(core->client)) {
                core->unk1A0.unk00 = 1;
                core->unk1A0.unk04 = 1;
                core->unk1A0.unk08 = BattleClient_GetRecMaxChapter(core->client);
                core->unk1A0.unk0C = 0;
                if (!BattleClient_IsChapterSkipping(core->client)) {
                    BtlvScd_ShowRecorderScreen(core->scd, &core->unk1A0);
                }
            }
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL BtlvCore_CmdCaptureDemo(BtlvCore *core, s32 *seq, void *work) {
    BtlvCaptureDemoWork *wk = work;
    void *data;
    u8 *keys;
    BattleMon *mon;

    switch (*seq) {
    case 0:
        data = BtlvEffect_CreateSetup(core->mainModule, core->scu, core->heapId);
        BtlvEffect_Init(data, core->font2, core->heapId);
        GFL_HeapFree(data);
        keys = BtlvScd_GetKeyCursorFlag(core->scd);
        wk->savedKeys = *keys;
        *keys = 0;
        BtlvScu_Setup(core->scu);
        BtlvScd_CreateInput(core->scd);
        wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTLV_CORE, core->heapId);
        wk->wait = 0;
        (*seq)++;
        break;
    case 1:
        BtlvScu_StartEncount(core->scu, 0);
        (*seq)++;
        break;
    case 2:
        if (BtlvScu_WaitEncount(core->scu)) {
            BtlvStringParam param;

            mon = BtlvCore_GetMonAtViewPos(core, 0);
            Btlv_StringParam_Setup(&param, 1, 0x45);
            Btlv_StringParam_AddArg(&param, GetMonID(mon));
            Btlv_StringParam_SetWait(&param, 0);
            BtlvCore_PrintMsg(core, &param);
            (*seq)++;
        }
        break;
    case 3:
        core->mon = BtlvCore_GetMonAtViewPos(core, 0);
        core->monId = GetMonID(core->mon);
        core->unkB8 = 0;
        core->result = 0;
        core->unk1B5 = 0;
        BtlvScd_StartCommandDemo(core->scd, core->mon, core->unk1B5, 0);
        (*seq)++;
        break;
    case 4:
        if (BtlvScd_GetCommandResult(core->scd)) {
            BtlvScd_StartMoveDemo(core->scd, core->mon, core->unkB8);
            (*seq)++;
        }
        break;
    case 5:
        if (BtlvScd_GetCommandResult(core->scd)) {
            BtlvCore_StartMsgMove(core, GetMonID(BtlvCore_GetMonAtViewPos(core, 0)), 0x21);
            BtlvScd_ResetScreen(core->scd);
            (*seq)++;
        }
        break;
    case 6:
        if (BtlvCore_WaitMsg(core) && BtlvScd_IsIdle(core->scd)) {
            BtlvScu_StartMoveEffect(core->scu, 0, 1, 0x21, PML_MoveGetParam(0x21, MOVE_PARAM_TARGET), 0, 0);
            (*seq)++;
        }
        break;
    case 7:
        if (BtlvScu_WaitMoveEffect(core->scu)) {
            mon = BtlvCore_GetMonAtViewPos(core, 1);
            BtlvEffect_CalcGaugeHP(1, (s32)GetBattleMonStat(mon, BATTLEMON_MAX_HP) / 6);
            BtlvEffect_StartDamage(1, 0x21);
            BtlvCore_PlaySE(core, 0x564);
            (*seq)++;
        }
        break;
    case 8:
        if (BtlvScu_WaitDamage(core->scu)) {
            BtlvCore_StartMsgMove(core, GetMonID(BtlvCore_GetMonAtViewPos(core, 1)), 0x2d);
            (*seq)++;
        }
        break;
    case 9:
        if (BtlvCore_WaitMsg(core)) {
            BtlvScu_StartMoveEffect(core->scu, 1, 0, 0x2d, PML_MoveGetParam(0x2d, MOVE_PARAM_TARGET), 0, 0);
            (*seq)++;
        }
        break;
    case 10:
        if (BtlvScu_WaitMoveEffect(core->scu)) {
            BtlvCore_StartRankDownEffect(core, 0);
            (*seq)++;
        }
        break;
    case 11:
        if (BtlvCore_WaitRankEffect(core, 0)) {
            BtlvStringParam param;

            mon = BtlvCore_GetMonAtViewPos(core, 0);
            Btlv_StringParam_Setup(&param, 2, 0x5a);
            Btlv_StringParam_AddArg(&param, GetMonID(mon));
            Btlv_StringParam_AddArg(&param, 1);
            Btlv_StringParam_AddArg(&param, 0);
            Btlv_StringParam_SetWait(&param, 0);
            BtlvCore_StartMsg(core, &param);
            (*seq)++;
        }
        break;
    case 12:
        if (BtlvCore_WaitMsg(core)) {
            GFL_MsgDataLoadStrbuf(wk->msgData, BtlvCore_Text_JustHaveUsePoke, core->strbuf);
            BtlvCore_StartMsgStr(core, core->strbuf, 0, NULL);
            wk->wait = 60;
            (*seq)++;
        }
        break;
    case 13:
        if (BtlvCore_WaitMsg(core) && --wk->wait == 0) {
            BtlvScd_StartCommandDemo(core->scd, core->mon, core->unk1B5, core->unkB8);
            (*seq)++;
        }
        break;
    case 14:
        if (BtlvScd_GetCommandResult(core->scd)) {
            BattleClientCmd_StartItemSelect(core, 2, 0, 0, 1, 0);
            (*seq)++;
        }
        break;
    case 15:
        if (BtlvCore_WaitItemSelect(core)) {
            GFL_MsgDataLoadStrbuf(wk->msgData, BtlvCore_Text_BiancaUsedPokeBall, core->strbuf);
            BtlvCore_StartMsgStr(core, core->strbuf, 0, NULL);
            BtlvScd_ResetScreen(core->scd);
            (*seq)++;
        }
        break;
    case 16:
        if (BtlvCore_WaitMsg(core) && !BtlvEffect_IsBusy()) {
            BtlvEffect_StartEffect23A(1, 4, 3, 1, 0);
            (*seq)++;
        }
        break;
    case 17:
        if (!BtlvEffect_IsBusy()) {
            BtlvStringParam param;

            mon = BtlvCore_GetMonAtViewPos(core, 1);
            Btlv_StringParam_Setup(&param, 1, 0x41);
            Btlv_StringParam_AddArg(&param, GetMonID(mon));
            BtlvCore_StartMsg(core, &param);
            (*seq)++;
        }
        break;
    case 18:
        if (BtlvCore_IsMsgJustDone(core)) {
            GFL_SndBGMPlay(0x518, 0xffff);
        }
        if (BtlvCore_WaitMsg(core)) {
            (*seq)++;
        }
        break;
    case 19:
        if (!GFL_SndBGMIsPlaying()) {
            BtlvEffect_Resume();
            GFL_MsgDataFree(wk->msgData);
            (*seq)++;
        }
        break;
    case 20:
        *BtlvScd_GetKeyCursorFlag(core->scd) = wk->savedKeys;
        return TRUE;
    }
    return FALSE;
}

BattleMon *BtlvCore_GetMonAtViewPos(BtlvCore *core, u32 index) {
    return PokeCon_GetPosMonInner(core->pokeCon, ViewPosToBattlePos(core->mainModule, index));
}

BOOL BtlvCore_ProcCommandSelect(BtlvCore *core, s32 *seq, void *work) {
    switch (*seq) {
    case 0:
        BtlvScd_StartCommandSelect(core->scd, core->mon, core->unk1B5, BattleClient_GetShooterEnergy(core->client),
                                   core->unkB8);
        (*seq)++;
    case 1:
        core->result = BtlvScd_GetCommandResult(core->scd);
        if (core->result != 0) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL BtlvCore_ProcTargetSelect(BtlvCore *core, s32 *seq, void *work) {
    switch (*seq) {
    case 0:
        BtlvScd_StartTargetSelect(core->scd, core->mon, core->unkB8);
        (*seq)++;
        break;
    case 1:
        if (BtlvScd_IsTargetSelectDone(core->scd)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void BtlvCore_SetMainProc(BtlvCore *core, BtlvMainProc proc) {
    core->mainProc = proc;
    core->seq = 0;
}

void BtlvCore_ClearMainProc(BtlvCore *core) {
    core->mainProc = NULL;
    core->seq = 0;
}

BOOL BtlvCore_RunMainProc(BtlvCore *core) {
    if (core->mainProc != NULL) {
        if (core->mainProc(core, &core->seq, core->work)) {
            core->mainProc = NULL;
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

void BtlvCore_StartCommandSelect(BtlvCore *core, BattleMon *mon, u32 arg2, BattleAction *arg3) {
    core->mon = mon;
    core->monId = GetMonID(mon);
    core->unkB8 = arg3;
    core->result = 0;
    core->unk1B5 = arg2;
    BtlvCore_SetMainProc(core, BtlvCore_ProcCommandSelect);
}

void BtlvCore_StopCommandSelect(BtlvCore *core) {
    BtlvCore_CancelSelect(core);
}

u32 BtlvCore_WaitCommandSelect(BtlvCore *core) {
    if (BtlvCore_RunMainProc(core)) {
        return core->result;
    }
    return 0;
}

void BtlvCore_StartMoveSelect(BtlvCore *core, BattleMon *mon, BattleAction *arg2) {
    core->mon = mon;
    core->monId = GetMonID(mon);
    core->unkB8 = arg2;
    BtlvScd_StartMoveSelect(core->scd, core->mon, arg2);
    BtlvCore_SetMainProc(core, BtlvCore_ProcWaitSelect);
}

void BtlvCore_StartRotationSelect(BtlvCore *core, BtlvSelectTargetParam *param, BattleAction *arg2) {
    BattleMon *mon;

    mon = param->mons[0].mon;
    core->mon = mon;
    core->monId = GetMonID(mon);
    core->unk1B0 = param;
    core->unkB8 = arg2;
    BtlvScd_StartRotationSelect(core->scd, param, arg2);
    BtlvCore_SetMainProc(core, BtlvCore_ProcWaitSelect);
}

void BtlvCore_ResumeMoveSelect(BtlvCore *core) {
    BtlvScd_ResumeMoveSelect(core->scd);
    BtlvCore_SetMainProc(core, BtlvCore_ProcWaitSelect);
}

BOOL BtlvCore_ProcWaitSelect(BtlvCore *core, s32 *seq, void *work) {
    if (BtlvScd_IsDone(core->scd)) {
        return TRUE;
    }
    return FALSE;
}

void BtlvCore_StopMoveSelect(BtlvCore *core) {
    BtlvCore_CancelSelect(core);
}

BOOL BtlvCore_WaitMoveSelect(BtlvCore *core) {
    return BtlvCore_RunMainProc(core);
}

void BtlvCore_StartTargetSelect(BtlvCore *core, BattleMon *mon, BattleAction *arg2) {
    core->mon = mon;
    core->monId = GetMonID(mon);
    core->unkB8 = arg2;
    BtlvCore_SetMainProc(core, BtlvCore_ProcTargetSelect);
}

u32 BtlvCore_WaitTargetSelect(BtlvCore *core) {
    if (BtlvCore_RunMainProc(core)) {
        if (BtlvScd_IsTargetChosen(core->scd)) {
            return BTLV_TARGET_SELECT_CHOSEN;
        }
        return BTLV_TARGET_SELECT_BACK;
    }
    return BTLV_TARGET_SELECT_NONE;
}

void BtlvCore_StopTargetSelect(BtlvCore *core) {
    BtlvCore_CancelSelect(core);
}

void BtlvCore_CancelSelect(BtlvCore *core) {
    BtlvEffect_StopIdleEffect();
    BtlvScd_CancelProcs(core->scd);
    BtlvCore_ClearMainProc(core);
}

void BtlvCore_ResetScreenIfShown(BtlvCore *core) {
    BtlvScd_ResetScreenIfShown(core->scd);
}

void BtlvCore_ResetScreen(BtlvCore *core) {
    BtlvScd_ResetScreen(core->scd);
}

BOOL BtlvCore_IsScreenIdle(BtlvCore *core) {
    return BtlvScd_IsIdle(core->scd);
}

BOOL func_ov167_021cf210(BtlvCore *core) {
    if (BtlSetup_GetBattleType(core->mainModule) == 3) {
        if (GetMultiMode(core->mainModule) == 2) {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

void BtlvCore_SetupPokeListParam(BtlvCore *core, BPlistParam *param, u8 mode, u8 partyIndex, u32 move) {
    u8 clientId;
    u8 unk1C;
    u8 index;

    clientId = BattleClient_GetClientId(core->client);
    param->party = GetClientPokeParty(core->mainModule, clientId);
    param->unk18 = mode != 3 ? BtlMain_IsCommMulti(core->mainModule) : 0;
    if (param->unk18) {
        param->unk8 = GetAllyPokeParty(core->mainModule, clientId);
        unk1C = GetMultiTrainerIndex(core->mainModule, clientId);
    } else {
        param->unk8 = NULL;
        unk1C = 0;
    }
    param->unk1F = mode;
    param->unk1C = unk1C;
    if (BtlSetup_GetBattleStyle(core->mainModule) != 3) {
        param->unk22 = GetClientBattlerCount(core->mainModule, clientId);
    } else {
        param->unk22 = 3;
    }
    switch (mode) {
    case 4:
    case 5:
        index = partyIndex;
        break;
    default:
        index = 0;
        break;
    }
    param->partyIndex = index;
    param->unk21 = partyIndex;
    param->move = move;
    param->slot = 0;
    param->heapId = core->heapId;
    param->font = core->font;
    param->unk14 = BtlSetup_GetBattleStyle(core->mainModule);
    param->usingKeys = BtlvScd_GetKeyCursorFlag(core->scd);
    param->tcbManager = BtlvEffect_GetTCBManager();
    param->paletteFade = BtlvEffect_GetPaletteFade();
    param->gameData = GetGameData(core->mainModule);
    param->unk30 = 0;
    param->unk40 = func_ov167_021cf210(core);
    param->unk34 = 0;
    param->done = FALSE;
}

void BattleClientCmd_StartPokeList(BtlvCore *core, const BtlvPokeListCmd *cmd, s32 partyIndex, u16 move,
                                   BtlvPokeSelectParam *select) {
    u32 count;
    u32 i;

    if (partyIndex < 0) {
        partyIndex = 0;
    }
    BtlvCore_SetupPokeListParam(core, &core->ov287, cmd->mode, partyIndex, move);
    count = PokeSelect_GetCount(select);
    for (i = 0; i < count; i++) {
        if (i >= 2) {
            break;
        }
        core->ov287.unk1D[i] = PokeSelect_Get(select, i);
    }
    for (; i < 2; i++) {
        core->ov287.unk1D[i] = 6;
    }
    select->selecting = TRUE;
    core->selectParam = select;
    core->subSeq = 0;
}

void BattleClientCmd_QuitPokeSelect(BtlvCore *core) {
    core->ov287.unk30 = 1;
}

BOOL BattleClientCmd_WaitPokeSelect(BtlvCore *core) {
    u32 i;

    switch (core->subSeq) {
    case 0:
        if (core->ov287.unk1F == 1 || core->ov287.unk1F == 2) {
            BtlvScd_StartFadeOut(core->scd, 0);
        } else {
            BtlvScd_StartFadeOut(core->scd, 1);
        }
        GFL_OvlLoad(OVERLAY_ID(285));
        core->subSeq++;
        break;
    case 1:
        if (BtlvScd_IsFadeDone(core->scd)) {
            BtlvScd_OnFadeOutDone(core->scd);
            GFL_OvlLoad(OVERLAY_ID(287));
            BPlistMain_Start(&core->ov287);
            core->subSeq++;
        }
        break;
    case 2:
        if (core->ov287.done) {
            if (core->ov287.unk1F != 5 && core->ov287.unk30 == 0) {
                for (i = 0; i < 3; i++) {
                    if (core->ov287.unk48[i] != 0xff) {
                        PokeSelect_Push(core->selectParam, i, core->ov287.unk48[i]);
                        PokeSelect_GetCount(core->selectParam);
                        core->selectParam->selecting = FALSE;
                    }
                }
            }
            GFL_OvlUnload(OVERLAY_ID(287));
            BtlvScd_StartFadeIn(core->scd);
            BtlvScd_OnFadeInStart(core->scd);
            core->subSeq++;
        }
        break;
    case 3:
        if (BtlvScd_IsFadeDone(core->scd)) {
            core->subSeq++;
        }
        break;
    default:
        GFL_OvlUnload(OVERLAY_ID(285));
        core->subSeq = 0;
        return TRUE;
    }
    return FALSE;
}

void BattleClientCmd_StartMoveInfoView(BtlvCore *core, u8 partyIndex, u8 slot) {
    BtlvCore_SetupPokeListParam(core, &core->ov287, 5, partyIndex, 0);
    core->ov287.slot = slot;
    core->subSeq = 0;
}

void BattleClientCmd_StartItemSelect(BtlvCore *core, u32 mode, u8 arg2, u8 arg3, u8 canUseItems, u8 arg5) {
    u32 kind;
    BattleParty *party;
    u32 count;
    u32 i;

    if (core->subSeq == 0) {
        core->ov286.bag = GetBagSave(core->mainModule);
        core->ov286.bagCursor = GetBagCursor(core->mainModule);
        core->ov286.mode = mode;
        core->ov286.font = core->font;
        core->ov286.heapId = core->heapId;
        core->ov286.shooterEnergy = arg2;
        core->ov286.shooterSpent = arg3;
        core->ov286.item = 0;
        core->ov286.usingKeys = BtlvScd_GetKeyCursorFlag(core->scd);
        core->ov286.quit = 0;
        core->ov286.shooterDisabled = func_ov167_0219db00(core->mainModule);
        kind = 0;
        if (BtlSetup_GetBattleType(core->mainModule) != 3) {
            kind = 1;
        }
        core->ov286.playSound = kind;
        kind = 0;
        core->ov286.abort = 0;
        if (BtlSetup_GetBattleType(core->mainModule) == 0) {
            kind = 1;
        }
        core->ov286.isWild = kind;
        kind = 0;
        core->ov286.done = FALSE;
        if (!canUseItems) {
            kind = 3;
        } else if (BtlSetup_IsBattleType(core->mainModule, 0x40)) {
            kind = 1;
        } else {
            switch (BtlSetup_GetBattleType(core->mainModule)) {
            case 0:
                if (PokeCon_GetClientAliveCount(core->pokeCon, 1) > 1) {
                    kind = 2;
                    break;
                }
                if (arg5) {
                    kind = 4;
                    break;
                }
                // fallthrough
            default:
                if (BtlSetup_IsBattleType(core->mainModule, 0x10000)) {
                    kind = 5;
                }
                break;
            }
        }
        core->ov286.ballError = kind;
        BtlvCore_SetupPokeListParam(core, &core->ov287, 3, 0, 0);
        party = BattleClient_GetParty(core->client);
        count = GetNumMonsInParty(party);
        for (i = 0; i < count; i++) {
            core->ov287.unk38[i] = CheckCondition(GetBattleMonFromParty(party, i), 0x13);
        }
        for (; i < 6; i++) {
            core->ov287.unk38[i] = 0;
        }
        core->subSeq = 1;
    }
}

void BtlvCore_QuitItemSelect(BtlvCore *core) {
    core->ov286.quit = 1;
    core->ov287.unk30 = 1;
}

BOOL BtlvCore_WaitItemSelect(BtlvCore *core) {
    switch (core->subSeq) {
    case 0:
        break;
    case 1:
        BtlvScd_StartFadeOut(core->scd, 1);
        GFL_OvlLoad(OVERLAY_ID(285));
        core->subSeq = 2;
        break;
    case 2:
        if (BtlvScd_IsFadeDone(core->scd)) {
            BtlvScd_OnFadeOutDone(core->scd);
            GFL_OvlLoad(OVERLAY_ID(286));
            BBagMain_Start(&core->ov286);
            core->subSeq = 3;
        }
        break;
    case 3:
        if (core->ov286.done) {
            GFL_OvlUnload(OVERLAY_ID(286));
            if (core->ov286.quit == 0) {
                core->subSeq = 4;
            } else {
                core->subSeq = 6;
            }
        }
        break;
    case 4:
        if (core->ov286.item != 0 && core->ov286.pocket != 2 && !IsEscapeItem(core->ov286.item)) {
            core->ov287.item = core->ov286.item;
            GFL_OvlLoad(OVERLAY_ID(287));
            BPlistMain_Start(&core->ov287);
            core->subSeq = 5;
        } else {
            core->subSeq = 6;
        }
        break;
    case 5:
        if (core->ov287.done) {
            GFL_OvlUnload(OVERLAY_ID(287));
            core->subSeq = 6;
        }
        break;
    case 6:
        BtlvScd_StartFadeIn(core->scd);
        BtlvScd_OnFadeInStart(core->scd);
        core->subSeq = 7;
        break;
    case 7:
        if (BtlvScd_IsFadeDone(core->scd)) {
            GFL_OvlUnload(OVERLAY_ID(285));
            core->subSeq = 0;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

u16 BtlvCore_GetSelectedItem(BtlvCore *core) {
    if (core->ov286.done) {
        return core->ov286.item;
    }
    return 0;
}

u8 BtlvCore_GetItemCost(BtlvCore *core) {
    if (core->ov286.done) {
        return core->ov286.cost;
    }
    return 0;
}

u8 BtlvCore_GetSelectedPartyIndex(BtlvCore *core) {
    return core->ov287.partyIndex;
}

u8 BtlvCore_GetSelectedMoveSlot(BtlvCore *core) {
    return core->ov287.slot;
}

void BtlvCore_SaveBagCursor(BtlvCore *core) {
    if (core->ov286.item != 0 && core->ov287.partyIndex != 6 && core->ov286.mode != 1) {
        BagCursor_SetBattlePositions(core->ov286.bagCursor, core->ov286.rows, core->ov286.pages);
        BagCursor_SetBattleLastItem(core->ov286.bagCursor, core->ov286.item, core->ov286.pocket);
    }
}

void BtlvCore_StartScenario(BtlvCore *core, u32 arg1) {
    if (core->subSeq == 0) {
        core->ov288.done = FALSE;
        core->ov288.heapId = core->heapId;
        core->ov288.font = core->font;
        core->ov288.abort = 0;
        core->ov288.close = 0;
        core->ov288.party = GetClientPokeParty(core->mainModule, 1);
        core->ov288.scenario = GetStudioMovieId(core->mainModule);
        core->ov288.scene = arg1;
        core->subSeq = 1;
    }
}

void BtlvCore_QuitScenario(BtlvCore *core) {
    core->ov288.close = 1;
}

BOOL BtlvCore_WaitScenario(BtlvCore *core) {
    switch (core->subSeq) {
    case 0:
        break;
    case 1:
        BtlvScd_StartFadeOut(core->scd, 1);
        GFL_OvlLoad(OVERLAY_ID(285));
        core->subSeq = 2;
        break;
    case 2:
        if (BtlvScd_IsFadeDone(core->scd)) {
            BtlvScd_OnFadeOutDone(core->scd);
            GFL_OvlLoad(OVERLAY_ID(288));
            BScenarioMain_Start(&core->ov288);
            core->subSeq = 3;
        }
        break;
    case 3:
        if (core->ov288.done) {
            GFL_OvlUnload(OVERLAY_ID(288));
            core->subSeq = 4;
        }
        break;
    case 4:
        BtlvScd_StartFadeIn(core->scd);
        BtlvScd_OnFadeInStart(core->scd);
        core->subSeq = 5;
        break;
    case 5:
        if (BtlvScd_IsFadeDone(core->scd)) {
            GFL_OvlUnload(OVERLAY_ID(285));
            core->subSeq = 0;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void BtlvCore_StartChoice(BtlvCore *core) {
    if (core->subSeq == 0) {
        core->ov289.done = FALSE;
        core->ov289.heapId = core->heapId;
        core->ov289.font = core->font;
        core->ov289.skip = 0;
        core->subSeq = 1;
    }
}

void BtlvCore_SetupChoice(BtlvCore *core, u32 arg1, u32 *arg2) {
    core->ov289.count = arg1;
    core->ov289.result = arg2;
}

void BtlvCore_SetChoiceStr(BtlvCore *core, u32 index, StrBuf *value) {
    core->ov289.strs[index] = value;
}

BOOL BtlvCore_WaitChoice(BtlvCore *core) {
    switch (core->subSeq) {
    case 0:
        break;
    case 1:
        BtlvScd_StartFadeOut(core->scd, 0);
        GFL_OvlLoad(OVERLAY_ID(285));
        core->subSeq = 2;
        break;
    case 2:
        if (BtlvScd_IsFadeDone(core->scd)) {
            BtlvScd_OnFadeOutDone(core->scd);
            GFL_OvlLoad(OVERLAY_ID(289));
            BChoiceMain_Start(&core->ov289);
            core->subSeq = 3;
        }
        break;
    case 3:
        if (core->ov289.done) {
            GFL_OvlUnload(OVERLAY_ID(289));
            core->subSeq = 4;
        }
        break;
    case 4:
        BtlvScd_StartFadeIn(core->scd);
        BtlvScd_OnFadeInStart(core->scd);
        core->subSeq = 5;
        break;
    case 5:
        if (BtlvScd_IsFadeDone(core->scd)) {
            GFL_OvlUnload(OVERLAY_ID(285));
            core->subSeq = 0;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void BtlvCore_StartMoveEffect(BtlvCore *core, u8 attackerPos, u8 targetPos, u16 move, s32 arg4, u32 arg5, u8 arg6) {
    u8 attacker;
    u8 target;

    attacker = BattlePosToViewPos(core->mainModule, attackerPos);
    if (targetPos != 6) {
        target = BattlePosToViewPos(core->mainModule, targetPos);
    } else {
        target = 0xff;
    }
    BtlvScu_StartMoveEffect(core->scu, attacker, target, move, arg4, arg5, arg6);
}

BOOL BtlvCore_WaitMoveEffect(BtlvCore *core) {
    return BtlvScu_WaitMoveEffect(core->scu);
}

void BtlvCore_StartDamage(BtlvCore *core, u16 arg1, u8 pos, u8 type) {
    BtlvPosEffectWork *work;

    work = BtlvCore_GetWork(core, sizeof(BtlvPosEffectWork));
    work->type = type;
    work->pos = pos;
    work->unk08 = 0;
    work->unk00 = arg1;
    BtlvCore_SetSubProc(core, BtlvCore_ProcDamage);
}

BOOL BtlvCore_WaitDamage(BtlvCore *core) {
    return BtlvCore_RunSubProc(core);
}

BOOL BtlvCore_ProcDamage(s32 *seq, void *arg) {
    BtlvCore *core = arg;
    BtlvPosEffectWork *work;

    work = BtlvCore_GetWork(core, sizeof(BtlvPosEffectWork));
    switch (*seq) {
    case 0:
        BtlvScu_StartDamage(core->scu, work->pos, work->unk00, BattleClient_IsChapterSkipping(core->client));
        BtlvCore_PlayDamageSE(core, work->type);
        (*seq)++;
        break;
    case 1:
        if (BtlvScu_WaitDamage(core->scu)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void BtlvCore_StartDamageEffect(BtlvCore *core, u16 arg1, u8 pos, u32 type) {
    BtlvPosEffectWork *work;

    if (!BattleClient_IsChapterSkipping(core->client)) {
        work = BtlvCore_GetWork(core, sizeof(BtlvPosEffectWork));
        work->type = type;
        work->pos = pos;
        work->unk08 = 0;
        work->unk00 = arg1;
        BtlvCore_SetSubProc(core, BtlvCore_ProcDamageEffect);
    }
}

BOOL BtlvCore_WaitDamageEffect(BtlvCore *core) {
    return BtlvCore_RunSubProc(core);
}

BOOL BtlvCore_ProcDamageEffect(s32 *seq, void *arg) {
    BtlvCore *core = arg;
    BtlvPosEffectWork *work;

    work = BtlvCore_GetWork(core, sizeof(BtlvPosEffectWork));
    switch (*seq) {
    case 0:
        BtlvScu_StartDamageEffect(core->scu, work->pos, work->unk00);
        BtlvCore_PlayDamageSE(core, work->type);
        (*seq)++;
        break;
    case 1:
        if (BtlvScu_WaitDamageEffect(core->scu)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void BtlvCore_StartMultiDamage(BtlvCore *core, u16 count, u32 type, const u8 *monIds, u16 arg4) {
    BtlvMultiEffectWork *work;
    u32 i;

    work = BtlvCore_GetWork(core, sizeof(BtlvMultiEffectWork));
    work->type = type;
    work->count = count;
    work->unk10 = arg4;
    work->seq = 0;
    for (i = 0; i < count; i++) {
        work->pos[i] = MonIDToBattlePos(core->mainModule, core->pokeCon, monIds[i]);
    }
}

BOOL BtlvCore_WaitMultiDamage(BtlvCore *core) {
    BtlvMultiEffectWork *work;
    BOOL flag;
    u32 i;

    work = BtlvCore_GetWork(core, sizeof(BtlvMultiEffectWork));
    switch (work->seq) {
    case 0:
        flag = BattleClient_IsChapterSkipping(core->client);
        for (i = 0; i < work->count; i++) {
            BtlvScu_StartDamage(core->scu, work->pos[i], work->unk10, flag);
        }
        BtlvCore_PlayDamageSE(core, work->type);
        work->seq++;
        break;
    case 1:
        if (BtlvScu_WaitDamage(core->scu) && BtlvScu_WaitMsg(core->scu)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void BtlvCore_PlayDamageSE(BtlvCore *core, u32 type) {
    u32 se;

    switch (type) {
    case 2:
        se = 0x563;
        break;
    case 3:
        se = 0x565;
        break;
    default:
        se = 0x564;
        break;
    }
    BtlvCore_PlaySE(core, se);
}

void BtlvCore_SetMonVisible(BtlvCore *core, u8 pos, u32 visible) {
    if (BtlvEffect_CheckExist(pos) && visible != BtlvEffect_CheckExistPokemon(pos)) {
        if (visible == 1) {
            BtlvEffect_StartEffect285(pos);
        } else {
            BtlvEffect_StartEffect286(pos);
        }
    }
}

void BtlvCore_StartGaugeHP(BtlvCore *core, u8 pos) {
    BtlvScu_StartGaugeHP(core->scu, pos, BattleClient_IsChapterSkipping(core->client));
}

BOOL BtlvCore_WaitGaugeHP(BtlvCore *core) {
    if (BtlvScu_WaitGaugeHP(core->scu)) {
        return TRUE;
    }
    return FALSE;
}

void BtlvCore_StartPosEffect(BtlvCore *core, u32 arg1, u16 arg2) {
    if (!BattleClient_IsChapterSkipping(core->client)) {
        BtlvEffect_StartPos(arg1, arg2);
    }
}

BOOL BtlvCore_WaitPosEffect(BtlvCore *core) {
    if (!BtlvEffect_IsBusy()) {
        return TRUE;
    }
    return FALSE;
}

void BtlvCore_StartAtkDefEffect(BtlvCore *core, u32 arg1, u32 arg2, u16 arg3) {
    BtlvEffect_StartAtkDef(arg1, arg2, arg3);
}

BOOL BtlvCore_WaitAtkDefEffect(BtlvCore *core) {
    if (!BtlvEffect_IsBusy()) {
        return TRUE;
    }
    return FALSE;
}

void BtlvCore_StartFaint(BtlvCore *core, u8 pos) {
    BtlvScu_StartFaint(core->scu, pos, BattleClient_IsChapterSkipping(core->client));
}

BOOL BtlvCore_WaitFaint(BtlvCore *core) {
    return BtlvScu_WaitFaint(core->scu);
}

void BtlvCore_StartShowMon(BtlvCore *core, u8 pos) {
    BtlvScu_ShowMon(core->scu, pos);
}

BOOL BtlvCore_WaitShowMon(BtlvCore *core) {
    return BtlvScu_WaitShowMon(core->scu);
}

void BtlvCore_StartSwitchOut(BtlvCore *core, u8 pos, u16 arg2) {
    BtlvScu_StartMemberOut(core->scu, pos, arg2, BattleClient_IsChapterSkipping(core->client));
}

BOOL BtlvCore_WaitSwitchOut(BtlvCore *core) {
    return BtlvScu_WaitMemberOut(core->scu);
}

void BtlvCore_StartSwitchIn(BtlvCore *core, u8 pos, u8 arg2, u8 arg3) {
    BtlvMonEffectWork *work;
    BattleMon *mon;

    work = BtlvCore_GetWork(core, sizeof(BtlvMonEffectWork));
    mon = PokeCon_GetPosMonInner(core->pokeCon, pos);
    work->unk01 = arg2;
    work->unk02 = arg3;
    work->pos = pos;
    work->monId = GetMonID(mon);
    BtlvCore_SetSubProc(core, BtlvCore_ProcSwitchIn);
}

BOOL BtlvCore_WaitSwitchIn(BtlvCore *core) {
    return BtlvCore_RunSubProc(core);
}

BOOL BtlvCore_ProcSwitchIn(s32 *seq, void *arg) {
    BtlvCore *core = arg;
    BtlvMonEffectWork *work;

    work = BtlvCore_GetWork(core, sizeof(BtlvMonEffectWork));
    switch (*seq) {
    case 0:
        BtlvScu_StartMemberIn(core->scu, work->pos, work->unk01, work->unk02,
                              BattleClient_IsChapterSkipping(core->client));
        (*seq)++;
        break;
    case 1:
        if (BtlvScu_WaitMemberIn(core->scu)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void BtlvCore_MakeString(const BtlvStringParam *param, StrBuf *strbuf) {
    switch (param->type) {
    case 1:
        BtlString_FormatStdMsgArgs(strbuf, param->message, param->args);
        break;
    case 2:
        BtlString_FormatSetMsg(strbuf, param->message, param->args);
        break;
    case 3:
        BtlString_LoadUIMsg(strbuf, param->message);
        break;
    case 5:
        func_ov167_021d5770(strbuf, param->message, param->args);
        break;
    case 6:
        func_ov167_021d57b0(strbuf, param->message, param->args);
        break;
    case 4:
        BtlString_FormatMoveMsg(strbuf, param->args[0], param->args[1]);
        break;
    case 7:
        func_ov167_021d5700(strbuf, param->args[0], param->args[1]);
        break;
    case 8:
        BtlString_FormatMovieScriptMsg(strbuf, param->message);
        break;
    case 9:
        BtlString_FormatMovieMsg(strbuf, param->message);
        break;
    }
}

void BtlvCore_CopyMsgStr(BtlvCore *core, const StrBuf *src) {
    GFL_StrBufCopy(core->strbuf, src);
}

void BtlvCore_MakeMsg(BtlvCore *core, const BtlvStringParam *param) {
    BtlvCore_MakeString(param, core->strbuf);
}

void BtlvCore_StartMadeMsg(BtlvCore *core) {
    BtlvCore_StartMsgStr(core, core->strbuf, 0x50, NULL);
}

void BtlvCore_StartMsg(BtlvCore *core, const BtlvStringParam *param) {
    BtlvCore_MakeString(param, core->strbuf);
    BtlvCore_StartMsgStr(core, core->strbuf, param->mode, 0);
}

void BtlvCore_StartMsgCallback(BtlvCore *core, const BtlvStringParam *param, BtlvMsgCallback callback) {
    BtlvCore_MakeString(param, core->strbuf);
    BtlvCore_StartMsgStr(core, core->strbuf, param->mode, callback);
}

void BtlvCore_PrintMsg(BtlvCore *core, const BtlvStringParam *param) {
    BtlvCore_MakeString(param, core->strbuf);
    BtlvCore_PrintMsgStr(core, core->strbuf);
}

void BtlvCore_StartMsgStd(BtlvCore *core, u16 message, const u32 *args) {
    BtlString_FormatStdMsgArgs(core->strbuf, message, args);
    BtlvCore_StartMsgStr(core, core->strbuf, 0x50, NULL);
}

void BtlvCore_StartMsgSet(BtlvCore *core, u16 message, const u32 *args) {
    BtlString_FormatSetMsg(core->strbuf, message, args);
    BtlvCore_StartMsgStr(core, core->strbuf, 0x50, NULL);
}

BOOL BtlvCore_StartMsgTrainer(BtlvCore *core, u32 trainerId, u32 msgId) {
    if (TrainerMsg_CheckExists(trainerId, msgId, core->heapId)) {
        TrainerMsg_Load(trainerId, msgId, core->strbuf, core->heapId);
        BtlvCore_StartMsgStr(core, core->strbuf, 0x50, NULL);
    } else {
        return FALSE;
    }
    return TRUE;
}

void BtlvCore_StartMsgMove(BtlvCore *core, u8 monId, u16 move) {
    BtlString_FormatMoveMsg(core->strbuf, monId, move);
    BtlvCore_StartMsgStr(core, core->strbuf, 0x50, NULL);
}

BOOL BtlvCore_WaitMsg(BtlvCore *core) {
    return BtlvScu_WaitMsg(core->scu);
}

BOOL BtlvCore_IsMsgJustDone(BtlvCore *core) {
    return BtlvScu_IsMsgFinished(core->scu);
}

void BtlvCore_StartMsgStr(BtlvCore *core, const StrBuf *strbuf, u16 wait, BtlvMsgCallback callback) {
    if (!BattleClient_IsChapterSkipping(core->client)) {
        BtlvScu_StartMsg(core->scu, strbuf, wait, callback);
    }
}

void BtlvCore_PrintMsgStr(BtlvCore *core, const StrBuf *strbuf) {
    BtlvScu_PrintMsgAtOnce(core->scu, strbuf);
}

void BtlvCore_ShowAbilityWin(BtlvCore *core, u8 pos, BOOL flash) {
    BtlvScu_StartAbilityWinShow(core->scu, pos, flash);
}

BOOL BtlvCore_WaitShowAbilityWin(BtlvCore *core, u8 pos) {
    return BtlvScu_WaitAbilityWinShow(core->scu, pos);
}

void BtlvCore_HideAbilityWin(BtlvCore *core, u8 pos) {
    BtlvScu_StartAbilityWinHide(core->scu, pos);
}

BOOL BtlvCore_WaitHideAbilityWin(BtlvCore *core, u8 pos) {
    return BtlvScu_WaitAbilityWinHide(core->scu, pos);
}

void BtlvCore_UpdateAbilityWin(BtlvCore *core, u8 pos) {
    BtlvScu_StartAbilityWinChange(core->scu, pos);
}

BOOL BtlvCore_WaitUpdateAbilityWin(BtlvCore *core, u8 pos) {
    return BtlvScu_WaitAbilityWinChange(core->scu, pos);
}

void BtlvCore_StartRankDownEffect(BtlvCore *core, u32 arg1) {
    BtlvEffect_StartPos(arg1, 0x262);
}

void BtlvCore_StartRankUpEffect(BtlvCore *core, u32 arg1) {
    BtlvEffect_StartPos(arg1, 0x261);
}

BOOL BtlvCore_WaitRankEffect(BtlvCore *core, u32 arg1) {
    if (!BtlvEffect_IsBusy()) {
        return TRUE;
    }
    return FALSE;
}

void BtlvCore_StartWaitMsg(BtlvCore *core) {
    BtlvEffect_SetIdleEffectMode(BTLV_IDLE_EFFECT_WAIT_MSG);
    BtlvScu_StartWaitMsg(core->scu);
}

BOOL BtlvCore_IsWaitMsgDone(BtlvCore *core) {
    return BtlvScu_WaitWaitMsg(core->scu);
}

void BtlvCore_EndWaitMsg(BtlvCore *core) {
    BtlvEffect_StopIdleEffect();
    BtlvScu_HideWaitMsg(core->scu);
}

void BtlvCore_StartUseItemEffect(BtlvCore *core, u8 pos) {
    BtlvScu_StartPosCueEffect(core->scu, BattlePosToViewPos(core->mainModule, pos));
}

BOOL BtlvCore_WaitUseItemEffect(BtlvCore *core, u8 pos) {
    return BtlvScu_IsPosCueEffectDone(core->scu, BattlePosToViewPos(core->mainModule, pos));
}

void BtlvCore_StartIllusionBreak(BtlvCore *core, u8 pos, u32 arg2) {
    BtlvScu_StartChangeMon(core->scu, pos, arg2);
}

BOOL BtlvCore_WaitIllusionBreak(BtlvCore *core) {
    return BtlvScu_WaitChangeMon(core->scu);
}

void BtlvCore_StartFormChange(BtlvCore *core, u32 arg1) {
    BtlvScu_StartChangeForm(core->scu, arg1, BattleClient_IsChapterSkipping(core->client));
}

BOOL BtlvCore_WaitFormChange(BtlvCore *core) {
    return BtlvScu_WaitChangeSprite(core->scu);
}

void BtlvCore_StartTransform(BtlvCore *core, u32 arg1, u32 arg2) {
    BtlvScu_StartTransform(core->scu, arg1, arg2, BattleClient_IsChapterSkipping(core->client));
}

BOOL BtlvCore_WaitTransform(BtlvCore *core) {
    return BtlvScu_WaitChangeSprite(core->scu);
}

void BtlvCore_StartHideMsgWin(BtlvCore *core) {
    BtlvScu_StartMsgWinHide(core->scu);
}

BOOL BtlvCore_WaitHideMsgWin(BtlvCore *core) {
    return BtlvScu_WaitMsgWinHide(core->scu);
}

void BtlvCore_StartSwapPokemon(BtlvCore *core, u8 clientId, u8 arg2, u8 arg3, u8 slot1, u8 slot2) {
    BtlvSwapWork *work;

    work = BtlvCore_GetWork(core, sizeof(BtlvSwapWork));
    work->clientId = clientId;
    work->unk01 = arg2;
    work->unk02 = arg3;
    work->slot1 = slot1;
    work->slot2 = slot2;
    work->pos1 = GetClientBattlePos(core->mainModule, clientId, slot1);
    work->pos2 = GetClientBattlePos(core->mainModule, clientId, slot2);
    BtlvCore_SetSubProc(core, BtlvCore_ProcSwapPokemon);
}

BOOL BtlvCore_WaitSwapPokemon(BtlvCore *core) {
    return BtlvCore_RunSubProc(core);
}

BOOL BtlvCore_ProcSwapPokemon(s32 *seq, void *arg) {
    BtlvCore *core = arg;
    BtlvSwapWork *work;

    work = BtlvCore_GetWork(core, sizeof(BtlvSwapWork));
    switch (*seq) {
    case 0:
        BtlvEffect_SwapPokemon(work->unk01, work->unk02);
        BtlvCore_PlaySE(core, 0x57b);
        (*seq)++;
        break;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            BtlvScu_HideGaugePair(core->scu, work->pos1, work->pos2);
            (*seq)++;
        }
        break;
    case 2:
        if (BtlvScu_ShowGaugePair(core->scu, work->pos1, work->pos2)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void BtlvCore_ShowRotationScreen(BtlvCore *core, u8 arg1) {
    void *work;

    work = BtlvCore_GetWork(core, 0x3c);
    GetClientParty(core->pokeCon, core->clientId);
    BtlvScd_ShowRotationScreen(core->scd, work);
}

BOOL BtlvCore_PollRotationInput(BtlvCore *core, u8 *out) {
    if (BtlvScd_PollRotationInput(core->scd, out)) {
        return TRUE;
    }
    return FALSE;
}

void BtlvCore_HideRotationScreen(BtlvCore *core) {
    BtlvScd_HideRotationScreen(core->scd);
}

void BtlvCore_StartRotation(BtlvCore *core, u8 clientId, u8 arg2) {
    BtlvRotateWork *work;

    if (arg2 != 1) {
        work = BtlvCore_GetWork(core, sizeof(BtlvRotateWork));
        work->clientId = clientId;
        work->unk01 = arg2;
        work->pos[0] = GetClientBattlePos(core->mainModule, clientId, 0);
        work->pos[1] = GetClientBattlePos(core->mainModule, clientId, 1);
        work->pos[2] = GetClientBattlePos(core->mainModule, clientId, 2);
        work->viewPos[0] = BattlePosToViewPos(core->mainModule, work->pos[0]);
        work->viewPos[1] = BattlePosToViewPos(core->mainModule, work->pos[1]);
        work->viewPos[2] = BattlePosToViewPos(core->mainModule, work->pos[2]);
        BtlvCore_SetSubProc(core, BtlvCore_ProcRotation);
    }
}

BOOL BtlvCore_WaitRotation(BtlvCore *core) {
    return BtlvCore_RunSubProc(core);
}

BOOL BtlvCore_ProcRotation(s32 *seq, void *arg) {
    BtlvCore *core = arg;
    BtlvRotateWork *work;
    BOOL done;

    work = BtlvCore_GetWork(core, sizeof(BtlvRotateWork));
    switch (*seq) {
    case 0:
        BtlvEffect_StartRotation(work->unk01, work->viewPos[0] & 1, !BattleClient_IsChapterSkipping(core->client));
        (*seq)++;
        break;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            BtlvScu_HideGauge(core->scu, work->pos[0]);
            BtlvScu_HideGauge(core->scu, work->pos[1]);
            BtlvScu_HideGauge(core->scu, work->pos[2]);
            (*seq)++;
        }
        break;
    case 2:
        done = TRUE;
        if (!BtlvScu_ShowGauge(core->scu, work->pos[0])) {
            done = FALSE;
        }
        if (!BtlvScu_ShowGauge(core->scu, work->pos[1])) {
            done = FALSE;
        }
        if (!BtlvScu_ShowGauge(core->scu, work->pos[2])) {
            done = FALSE;
        }
        if (done) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void BtlvCore_ShowYesNo(BtlvCore *core, const BtlvStringParam *param1, const BtlvStringParam *param2, u32 arg3) {
    BtlvTwoStringWork *work;

    work = BtlvCore_GetWork(core, sizeof(BtlvTwoStringWork));
    work->first = GFL_StrBufCreate(0x20, HEAPID_TAIL(core->heapId));
    work->second = GFL_StrBufCreate(0x20, HEAPID_TAIL(core->heapId));
    work->unk08 = arg3;
    BtlvCore_MakeString(param1, work->first);
    BtlvCore_MakeString(param2, work->second);
    BtlvScd_ShowYesNo(core->scd, work);
    GFL_HeapFree(work->first);
    GFL_HeapFree(work->second);
}

BOOL BtlvCore_PollYesNo(BtlvCore *core, u32 *answer) {
    return BtlvScd_PollYesNo(core->scd, answer);
}

void BtlvCore_StartForgetMoveSelect(BtlvCore *core, u8 partyIndex, u16 move) {
    BtlvCore_SetupPokeListParam(core, &core->ov287, 4, partyIndex, move);
    core->subSeq = 0;
}

BOOL BtlvCore_WaitForgetMoveSelect(BtlvCore *core, u8 *slot) {
    switch (core->subSeq) {
    case 0:
        BtlvScd_StartFadeOut(core->scd, 0);
        GFL_OvlLoad(OVERLAY_ID(285));
        core->subSeq++;
        break;
    case 1:
        if (BtlvScd_IsFadeDone(core->scd)) {
            BtlvScd_OnFadeOutDone(core->scd);
            GFL_OvlLoad(OVERLAY_ID(287));
            BPlistMain_Start(&core->ov287);
            core->subSeq++;
        }
        break;
    case 2:
        if (core->ov287.done) {
            GFL_OvlUnload(OVERLAY_ID(287));
            BtlvScd_StartFadeIn(core->scd);
            BtlvScd_OnFadeInStart(core->scd);
            core->subSeq++;
        }
        break;
    case 3:
        if (BtlvScd_IsFadeDone(core->scd)) {
            core->subSeq++;
        }
        break;
    default:
        *slot = core->ov287.slot;
        GFL_OvlUnload(OVERLAY_ID(285));
        core->subSeq = 0;
        return TRUE;
    }
    return FALSE;
}

void BtlvCore_StartLevelUpWin(BtlvCore *core, BattleMon *mon, const BattleMonLevelUp *levelUp) {
    BtlvScu_StartLevelUpWinOpen(core->scu, mon, levelUp);
}

BOOL BtlvCore_WaitLevelUpWin(BtlvCore *core) {
    return BtlvScu_WaitLevelUpWinOpen(core->scu);
}

void BtlvCore_StartLevelUpStats(BtlvCore *core) {
    BtlvScu_StartLevelUpWinStats(core->scu);
}

BOOL BtlvCore_WaitLevelUpStats(BtlvCore *core) {
    return BtlvScu_WaitLevelUpWinStats(core->scu);
}

void BtlvCore_StartCloseLevelUpWin(BtlvCore *core) {
    BtlvScu_StartLevelUpWinClose(core->scu);
}

BOOL BtlvCore_WaitCloseLevelUpWin(BtlvCore *core) {
    return BtlvScu_WaitLevelUpWinClose(core->scu);
}

void BtlvCore_StartFadeOut(BtlvCore *core) {
    BtlvScu_StartFadeOut(core->scu);
}

BOOL BtlvCore_WaitFadeOut(BtlvCore *core) {
    return BtlvScu_IsFadeDone(core->scu);
}

void BtlvCore_StartFadeIn(BtlvCore *core) {
    BtlvScu_StartFadeIn(core->scu);
}

BOOL BtlvCore_WaitFadeIn(BtlvCore *core) {
    return BtlvScu_IsFadeDone(core->scu);
}

void BattleClientCmd_ForceQuitInputNotify(BtlvCore *core) {
    BtlvScd_ForceQuitInput(core->scd);
}

BOOL BattleClientCmd_ForceQuitInputWait(BtlvCore *core) {
    return BtlvScd_IsForceQuitInputDone(core->scd);
}

u32 BtlvCore_CheckRecorderInput(BtlvCore *core) {
    return BtlvScd_CheckRecorderInput(core->scd);
}

void BtlvCore_ShowRecorderChapter(BtlvCore *core, u16 arg1, u16 arg2) {
    core->unk1A0.unk00 = arg1;
    core->unk1A0.unk04 = arg2;
    core->unk1A0.unk0C = 0;
    BtlvScd_ShowRecorderScreen(core->scd, &core->unk1A0);
}

void func_ov167_021d0a7c(BtlvCore *core, u16 arg1) {
    core->unk1A0.unk00 = arg1;
    core->unk1A0.unk04 = arg1;
    core->unk1A0.unk0C = 3;
    BtlvScd_ShowRecorderScreen(core->scd, &core->unk1A0);
}

void BtlvCore_ShowRecorderMessage(BtlvCore *core, u32 arg1, u32 arg2) {
    core->unk1A0.unk00 = arg1;
    core->unk1A0.unk04 = arg1;
    core->unk1A0.unk0C = arg2;
    BtlvScd_ShowRecorderScreen(core->scd, &core->unk1A0);
}

void BtlvCore_PlaySE(BtlvCore *core, u32 se) {
    if (!BattleClient_IsChapterSkipping(core->client)) {
        GFL_SndSEPlay(se);
    }
}

// Function names from swan.
void Btlv_StringParam_Setup(BtlvStringParam *param, u32 type, u16 message) {
    u32 i;

    for (i = 0; i < 9; i++) {
        param->args[i] = 0;
    }
    param->count = 0;
    param->message = message;
    param->type = type;
    param->mode = 0x50;
}

void Btlv_StringParam_AddArg(BtlvStringParam *param, u32 arg) {
    u8 count;

    count = param->count;
    if (count < 9) {
        param->count = count + 1;
        param->args[count] = arg;
    }
}

void Btlv_StringParam_SetWait(BtlvStringParam *param, u8 mode) {
    param->mode = mode;
}

void BtlvCore_SetClactGauge(BtlvCore *core, u32 arg1, u32 arg2) {
    if (GetStudioMode(core->mainModule) == 1) {
        BtlvClact_SetGauge(BtlvEffect_GetClact(), arg1, arg2);
    }
}

void BtlvCore_ShowClactPopup(BtlvCore *core, u32 arg1) {
    if (GetStudioMode(core->mainModule) == 1) {
        BtlvClact_ShowPopup(BtlvEffect_GetClact(), arg1, core->font);
    }
}

BOOL BtlvCore_IsClactPopupShowing(BtlvCore *core) {
    if (GetStudioMode(core->mainModule) == 1) {
        return BtlvClact_IsPopupShowing(BtlvEffect_GetClact());
    }
    return FALSE;
}

void BtlvCore_SetTypeIconVisible(BtlvCore *core, u32 arg1, u32 arg2, u32 arg3) {
    if (GetStudioMode(core->mainModule) == 2) {
        BtlvScd_SetTypeIconVisible(core->scd, arg1, arg2, arg3);
    }
}

u32 BtlvCore_GetTypeMask(BtlvCore *core) {
    return BtlvScd_GetTypeMask(core->scd);
}

void BtlvCore_StartTypeIconAnim(BtlvCore *core, u32 arg1) {
    if (GetStudioMode(core->mainModule) == 2) {
        BtlvScd_StartTypeIconAnim(core->scd, arg1);
    }
}
