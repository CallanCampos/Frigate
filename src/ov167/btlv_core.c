#include "types.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "app/bag.h"
#include "battle/b_bag_main.h"
#include "battle/b_plist_main.h"
#include "battle/btl_client.h"
#include "battle/btl_main.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_string.h"
#include "battle/btlv.h"
#include "battle/btlv_clact.h"
#include "battle/btlv_effect.h"
#include "battle/tr_ai.h"
#include "battle/trainer_data.h"
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
#include "pml/waza.h"
#include "system/gf_font.h"
#include "text/system/btlv_core.h"

// The procedure each display command starts
typedef struct BtlvCmdEntry {
    u32 cmd;
    BtlvMainProc proc;
} BtlvCmdEntry;

// Overlay 288's parameters
typedef struct BtlvOv288Param {
    Font *font;
    HeapID heapId;
    u8 done;
    u8 unk07;
    PokeParty *unk08;
    u32 unk0C;
    u32 unk10;
    u32 unk14;
    u32 unk18;
} BtlvOv288Param;

// Overlay 289's parameters
typedef struct BtlvOv289Param {
    Font *font;
    HeapID heapId;
    u8 done;
    u8 unk07;
    u32 unk08;
    StrBuf *unk0C[4];
    u32 *unk1C;
    u32 unk20;
} BtlvOv289Param;

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
    void *unkB8;
    BattleMon *mon;
    u32 monId;
    u32 result;
    BBagParam ov286;
    BPlistParam ov287;
    BtlvOv288Param ov288;
    BtlvOv289Param ov289;
    BtlvPokeSelectParam *selectParam;
    BtlvViewParam unk1A0;
    BtlvSelectTargetParam *unk1B0;
    u8 subSeq;
    u8 unk1B5;
    u8 unk1B6[6];
    TCBExManager *tcbManager;
    BtlvScu *scu;
    void *unk1C4;
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

extern const BtlvCmdEntry data_ov167_021da8c4[2];

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

void func_ov167_021ce604(HeapID heapId) {
    func_ov167_021ce678(heapId);
    GFL_OvlLoad(OVERLAY_ID(168));
    gfxSetLCDCBanks(0x80);
    GFL_OvlLoad(OVERLAY_ID(169));
    if (!func_02042b20()) {
        GFL_OvlLoad(OVERLAY_TR_AI);
    }
}

void func_ov167_021ce638(void) {
    func_ov167_021ce748();
    GFL_OvlUnload(OVERLAY_ID(168));
    GFL_OvlUnload(OVERLAY_ID(169));
    if (!func_02042b20()) {
        GFL_OvlUnload(OVERLAY_TR_AI);
    }
}

void func_ov167_021ce668(HeapID heapId) {
    func_ov167_021ce748();
    func_ov167_021ce678(heapId);
}

void func_ov167_021ce678(HeapID heapId) {
    GFL_BGSysCreate(heapId);
    BmpWin_InitAllocator(heapId);
    func_020232d0();
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
    gfxSetFog(FALSE, 0, 0, 0);
    G2_SetBG0Priority(1);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1,
                        GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 |
                            GX_BLEND_PLANEMASK_OBJ | GX_BLEND_PLANEMASK_BD,
                        0x1f, 3);
    ClActSys_Create(&data_ov167_021da8e4, &data_ov167_021da900, heapId);
}

void func_ov167_021ce748(void) {
    BtlvEffect_Exit();
    func_0204b758();
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
    core->scu = func_ov167_021d0c24(core, core->mainModule, pokeCon, core->tcbManager, core->font, core->font2,
                                    core->clientId, heapId);
    core->unk1C4 =
        func_ov169_06899af0(core, core->mainModule, pokeCon, core->tcbManager, core->font, core->client, arg3, heapId);
    core->mainProc = NULL;
    core->seq = 0;
    core->subSeq = 0;
    func_ov167_021d4c64(mainModule, core->clientId, pokeCon, heapId);
    return core;
}

void func_ov167_021ce870(BtlvCore *core) {
    func_ov167_021d4d50();
    func_ov169_06899ed0(core->unk1C4);
    func_ov167_021d0f84(core->scu);
    GFL_TCBExMgrFree(core->tcbManager);
    GFL_StrBufFree(core->strbuf);
    GFL_FontFree(core->font);
    GFL_FontFree(core->font2);
    GFL_HeapFree(core);
}

void func_ov167_021ce8c8(BtlvCore *core) {
    GFL_TCBExMgrUpdate(core->tcbManager);
    BtlvEffect_Main();
}

void func_ov167_021ce8dc(BtlvCore *core, u32 cmd) {
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

BOOL func_ov167_021ce90c(BtlvCore *core) {
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

void *func_ov167_021ce93c(BtlvCore *core, u32 size) {
    return core->work;
}

BOOL func_ov167_021ce940(BtlvCore *core, s32 *seq, void *work) {
    void *data;

    switch (*seq) {
    case 0:
        data = BtlvEffect_CreateSetup(core->mainModule, core->scu, core->heapId);
        BtlvEffect_Init(data, core->font2, core->heapId);
        GFL_HeapFree(data);
        func_ov167_021d0cd4(core->scu);
        func_ov169_06899c7c(core->unk1C4);
        (*seq)++;
        break;
    case 1:
        func_ov167_021d1084(core->scu, func_ov167_021b1990(core->client));
        (*seq)++;
        break;
    case 2:
        if (func_ov167_021d12a0(core->scu)) {
            if (func_ov167_021b1978(core->client)) {
                core->unk1A0.unk00 = 1;
                core->unk1A0.unk04 = 1;
                core->unk1A0.unk08 = func_ov167_021b198c(core->client);
                core->unk1A0.unk0C = 0;
                if (!func_ov167_021b1990(core->client)) {
                    func_ov169_0689b8dc(core->unk1C4, &core->unk1A0);
                }
            }
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL func_ov167_021cea24(BtlvCore *core, s32 *seq, void *work) {
    BtlvCaptureDemoWork *wk = work;
    void *data;
    u8 *keys;
    BattleMon *mon;

    switch (*seq) {
    case 0:
        data = BtlvEffect_CreateSetup(core->mainModule, core->scu, core->heapId);
        BtlvEffect_Init(data, core->font2, core->heapId);
        GFL_HeapFree(data);
        keys = func_ov169_0689b7c8(core->unk1C4);
        wk->savedKeys = *keys;
        *keys = 0;
        func_ov167_021d0cd4(core->scu);
        func_ov169_06899c7c(core->unk1C4);
        wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTLV_CORE, core->heapId);
        wk->wait = 0;
        (*seq)++;
        break;
    case 1:
        func_ov167_021d1084(core->scu, 0);
        (*seq)++;
        break;
    case 2:
        if (func_ov167_021d12a0(core->scu)) {
            BtlvStringParam param;

            mon = func_ov167_021ceec0(core, 0);
            Btlv_StringParam_Setup(&param, 1, 0x45);
            Btlv_StringParam_AddArg(&param, GetMonID(mon));
            func_ov167_021d0b4c(&param, 0);
            func_ov167_021d0234(core, &param);
            (*seq)++;
        }
        break;
    case 3:
        core->mon = func_ov167_021ceec0(core, 0);
        core->monId = GetMonID(core->mon);
        core->unkB8 = 0;
        core->result = 0;
        core->unk1B5 = 0;
        func_ov169_0689a038(core->unk1C4, core->mon, core->unk1B5, 0);
        (*seq)++;
        break;
    case 4:
        if (func_ov169_0689a060(core->unk1C4)) {
            func_ov169_0689a120(core->unk1C4, core->mon, core->unkB8);
            (*seq)++;
        }
        break;
    case 5:
        if (func_ov169_0689a060(core->unk1C4)) {
            func_ov167_021d02cc(core, GetMonID(func_ov167_021ceec0(core, 0)), 0x21);
            func_ov169_06899e5c(core->unk1C4);
            (*seq)++;
        }
        break;
    case 6:
        if (func_ov167_021d02e8(core) && func_ov169_06899ea8(core->unk1C4)) {
            func_ov167_021d3094(core->scu, 0, 1, 0x21, PML_MoveGetParam(0x21, 0x1b), 0, 0);
            (*seq)++;
        }
        break;
    case 7:
        if (func_ov167_021d3130(core->scu)) {
            mon = func_ov167_021ceec0(core, 1);
            BtlvEffect_CalcGaugeHP(1, (s32)GetBattleMonStat(mon, 0xe) / 6);
            BtlvEffect_StartDamage(1, 0x21);
            func_ov167_021d0ad4(core, 0x564);
            (*seq)++;
        }
        break;
    case 8:
        if (func_ov167_021d31d0(core->scu)) {
            func_ov167_021d02cc(core, GetMonID(func_ov167_021ceec0(core, 1)), 0x2d);
            (*seq)++;
        }
        break;
    case 9:
        if (func_ov167_021d02e8(core)) {
            func_ov167_021d3094(core->scu, 1, 0, 0x2d, PML_MoveGetParam(0x2d, 0x1b), 0, 0);
            (*seq)++;
        }
        break;
    case 10:
        if (func_ov167_021d3130(core->scu)) {
            func_ov167_021d03a0(core, 0);
            (*seq)++;
        }
        break;
    case 11:
        if (func_ov167_021d03c0(core, 0)) {
            BtlvStringParam param;

            mon = func_ov167_021ceec0(core, 0);
            Btlv_StringParam_Setup(&param, 2, 0x5a);
            Btlv_StringParam_AddArg(&param, GetMonID(mon));
            Btlv_StringParam_AddArg(&param, 1);
            Btlv_StringParam_AddArg(&param, 0);
            func_ov167_021d0b4c(&param, 0);
            func_ov167_021d01ec(core, &param);
            (*seq)++;
        }
        break;
    case 12:
        if (func_ov167_021d02e8(core)) {
            GFL_MsgDataLoadStrbuf(wk->msgData, BtlvCore_Text_JustHaveUsePoke, core->strbuf);
            func_ov167_021d0308(core, core->strbuf, 0, NULL);
            wk->wait = 60;
            (*seq)++;
        }
        break;
    case 13:
        if (func_ov167_021d02e8(core) && --wk->wait == 0) {
            func_ov169_0689a038(core->unk1C4, core->mon, core->unk1B5, core->unkB8);
            (*seq)++;
        }
        break;
    case 14:
        if (func_ov169_0689a060(core->unk1C4)) {
            BattleClientCmd_StartItemSelect(core, 2, 0, 0, 1, 0);
            (*seq)++;
        }
        break;
    case 15:
        if (func_ov167_021cf73c(core)) {
            GFL_MsgDataLoadStrbuf(wk->msgData, BtlvCore_Text_BiancaUsedPokeBall, core->strbuf);
            func_ov167_021d0308(core, core->strbuf, 0, NULL);
            func_ov169_06899e5c(core->unk1C4);
            (*seq)++;
        }
        break;
    case 16:
        if (func_ov167_021d02e8(core) && !BtlvEffect_IsBusy()) {
            BtlvEffect_StartEffect23A(1, 4, 3, 1, 0);
            (*seq)++;
        }
        break;
    case 17:
        if (!BtlvEffect_IsBusy()) {
            BtlvStringParam param;

            mon = func_ov167_021ceec0(core, 1);
            Btlv_StringParam_Setup(&param, 1, 0x41);
            Btlv_StringParam_AddArg(&param, GetMonID(mon));
            func_ov167_021d01ec(core, &param);
            (*seq)++;
        }
        break;
    case 18:
        if (func_ov167_021d02f8(core)) {
            GFL_SndBGMPlay(0x518, 0xffff);
        }
        if (func_ov167_021d02e8(core)) {
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
        *func_ov169_0689b7c8(core->unk1C4) = wk->savedKeys;
        return TRUE;
    }
    return FALSE;
}

BattleMon *func_ov167_021ceec0(BtlvCore *core, u32 index) {
    return func_ov167_0219d188(core->pokeCon, func_ov167_0219c744(core->mainModule, index));
}

BOOL func_ov167_021ceed8(BtlvCore *core, s32 *seq, void *work) {
    switch (*seq) {
    case 0:
        func_ov169_0689a008(core->unk1C4, core->mon, core->unk1B5, BattleClient_GetShooterEnergy(core->client),
                            core->unkB8);
        (*seq)++;
    case 1:
        core->result = func_ov169_0689a060(core->unk1C4);
        if (core->result != 0) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL func_ov167_021cef50(BtlvCore *core, s32 *seq, void *work) {
    switch (*seq) {
    case 0:
        func_ov169_0689a140(core->unk1C4, core->mon, core->unkB8);
        (*seq)++;
        break;
    case 1:
        if (func_ov169_0689a158(core->unk1C4)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void func_ov167_021cefb4(BtlvCore *core, BtlvMainProc proc) {
    core->mainProc = proc;
    core->seq = 0;
}

void func_ov167_021cefbc(BtlvCore *core) {
    core->mainProc = NULL;
    core->seq = 0;
}

BOOL func_ov167_021cefc4(BtlvCore *core) {
    if (core->mainProc != NULL) {
        if (core->mainProc(core, &core->seq, core->work)) {
            core->mainProc = NULL;
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

void func_ov167_021cefec(BtlvCore *core, BattleMon *mon, u32 arg2, void *arg3) {
    core->mon = mon;
    core->monId = GetMonID(mon);
    core->unkB8 = arg3;
    core->result = 0;
    core->unk1B5 = arg2;
    func_ov167_021cefb4(core, func_ov167_021ceed8);
}

void func_ov167_021cf028(BtlvCore *core) {
    func_ov167_021cf1b4(core);
}

u32 func_ov167_021cf030(BtlvCore *core) {
    if (func_ov167_021cefc4(core)) {
        return core->result;
    }
    return 0;
}

void func_ov167_021cf048(BtlvCore *core, BattleMon *mon, void *arg2) {
    core->mon = mon;
    core->monId = GetMonID(mon);
    core->unkB8 = arg2;
    func_ov169_0689a078(core->unk1C4, core->mon, arg2);
    func_ov167_021cefb4(core, func_ov167_021cf110);
}

void func_ov167_021cf094(BtlvCore *core, BtlvSelectTargetParam *param, void *arg2) {
    BattleMon *mon;

    mon = param->mons[0].mon;
    core->mon = mon;
    core->monId = GetMonID(mon);
    core->unk1B0 = param;
    core->unkB8 = arg2;
    func_ov169_0689a09c(core->unk1C4, param, arg2);
    func_ov167_021cefb4(core, func_ov167_021cf110);
}

void func_ov167_021cf0e4(BtlvCore *core) {
    func_ov169_0689a114(core->unk1C4);
    func_ov167_021cefb4(core, func_ov167_021cf110);
}

BOOL func_ov167_021cf110(BtlvCore *core, s32 *seq, void *work) {
    if (func_ov169_0689a10c(core->unk1C4)) {
        return TRUE;
    }
    return FALSE;
}

void func_ov167_021cf138(BtlvCore *core) {
    func_ov167_021cf1b4(core);
}

BOOL func_ov167_021cf140(BtlvCore *core) {
    return func_ov167_021cefc4(core);
}

void func_ov167_021cf148(BtlvCore *core, BattleMon *mon, void *arg2) {
    core->mon = mon;
    core->monId = GetMonID(mon);
    core->unkB8 = arg2;
    func_ov167_021cefb4(core, func_ov167_021cef50);
}

u32 func_ov167_021cf174(BtlvCore *core) {
    if (func_ov167_021cefc4(core)) {
        if (func_ov169_0689b228(core->unk1C4)) {
            return 1;
        }
        return 2;
    }
    return 0;
}

void func_ov167_021cf1ac(BtlvCore *core) {
    func_ov167_021cf1b4(core);
}

void func_ov167_021cf1b4(BtlvCore *core) {
    BtlvEffect_StopIdleEffect();
    func_ov169_0689a160(core->unk1C4);
    func_ov167_021cefbc(core);
}

void func_ov167_021cf1e0(BtlvCore *core) {
    func_ov169_06899e34(core->unk1C4);
}

void func_ov167_021cf1f0(BtlvCore *core) {
    func_ov169_06899e5c(core->unk1C4);
}

BOOL func_ov167_021cf200(BtlvCore *core) {
    return func_ov169_06899ea8(core->unk1C4);
}

BOOL func_ov167_021cf210(BtlvCore *core) {
    if (BtlSetup_GetBattleType(core->mainModule) == 3) {
        if (func_ov167_0219bedc(core->mainModule) == 2) {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

void func_ov167_021cf234(BtlvCore *core, BPlistParam *param, u8 mode, u8 partyIndex, u32 move) {
    u8 clientId;
    u8 unk1C;
    u8 index;

    clientId = BattleClient_GetClientId(core->client);
    param->party = func_ov167_0219c7c8(core->mainModule, clientId);
    param->unk18 = mode != 3 ? func_ov167_0219c7a4(core->mainModule) : 0;
    if (param->unk18) {
        param->unk8 = func_ov167_0219c7e8(core->mainModule, clientId);
        unk1C = func_ov167_0219c82c(core->mainModule, clientId);
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
    param->usingKeys = func_ov169_0689b7c8(core->unk1C4);
    param->tcbManager = BtlvEffect_GetTCBManager();
    param->paletteFade = BtlvEffect_GetPaletteFade();
    param->gameData = func_ov167_0219bf98(core->mainModule);
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
    func_ov167_021cf234(core, &core->ov287, cmd->mode, partyIndex, move);
    count = func_ov169_0689cca0(select);
    for (i = 0; i < count; i++) {
        if (i >= 2) {
            break;
        }
        core->ov287.unk1D[i] = func_ov169_0689ccb4(select, i);
    }
    for (; i < 2; i++) {
        core->ov287.unk1D[i] = 6;
    }
    select->unk08 = 1;
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
            func_ov169_06899d9c(core->unk1C4, 0);
        } else {
            func_ov169_06899d9c(core->unk1C4, 1);
        }
        GFL_OvlLoad(OVERLAY_ID(285));
        core->subSeq++;
        break;
    case 1:
        if (func_ov169_06899dfc(core->unk1C4)) {
            func_ov169_06899e24(core->unk1C4);
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
                        func_ov169_0689cc74(core->selectParam, i, core->ov287.unk48[i]);
                        func_ov169_0689cca0(core->selectParam);
                        core->selectParam->unk08 = 0;
                    }
                }
            }
            GFL_OvlUnload(OVERLAY_ID(287));
            func_ov169_06899dd4(core->unk1C4);
            func_ov169_06899e28(core->unk1C4);
            core->subSeq++;
        }
        break;
    case 3:
        if (func_ov169_06899dfc(core->unk1C4)) {
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
    func_ov167_021cf234(core, &core->ov287, 5, partyIndex, 0);
    core->ov287.slot = slot;
    core->subSeq = 0;
}

static inline u8 BtlvCore_GetBagKind(BtlvCore *core, u8 canUseItems, u8 arg5) {
    if (!canUseItems) {
        return 3;
    }
    if (BtlSetup_IsBattleType(core->mainModule, 0x40)) {
        return 1;
    }
    if (BtlSetup_GetBattleType(core->mainModule) == 0) {
        if (func_ov167_0219d3f8(core->pokeCon, 1) > 1) {
            return 2;
        }
        if (arg5) {
            return 4;
        }
    }
    if (BtlSetup_IsBattleType(core->mainModule, 0x10000)) {
        return 5;
    }
    return 0;
}

void BattleClientCmd_StartItemSelect(BtlvCore *core, u32 mode, u8 arg2, u8 arg3, u8 canUseItems, u8 arg5) {
    u32 kind;
    BattleParty *party;
    u32 count;
    u32 i;

    if (core->subSeq == 0) {
        core->ov286.bag = func_ov167_0219c954(core->mainModule);
        core->ov286.bagCursor = func_ov167_0219c95c(core->mainModule);
        core->ov286.mode = mode;
        core->ov286.font = core->font;
        core->ov286.heapId = core->heapId;
        core->ov286.shooterEnergy = arg2;
        core->ov286.shooterSpent = arg3;
        core->ov286.item = 0;
        core->ov286.usingKeys = func_ov169_0689b7c8(core->unk1C4);
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
        core->ov286.done = FALSE;
        core->ov286.ballError = BtlvCore_GetBagKind(core, canUseItems, arg5);
        func_ov167_021cf234(core, &core->ov287, 3, 0, 0);
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

void func_ov167_021cf72c(BtlvCore *core) {
    core->ov286.quit = 1;
    core->ov287.unk30 = 1;
}

BOOL func_ov167_021cf73c(BtlvCore *core) {
    switch (core->subSeq) {
    case 0:
        break;
    case 1:
        func_ov169_06899d9c(core->unk1C4, 1);
        GFL_OvlLoad(OVERLAY_ID(285));
        core->subSeq = 2;
        break;
    case 2:
        if (func_ov169_06899dfc(core->unk1C4)) {
            func_ov169_06899e24(core->unk1C4);
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
        if (core->ov286.item != 0 && core->ov286.pocket != 2 && !func_ov169_0689ca94(core->ov286.item)) {
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
        func_ov169_06899dd4(core->unk1C4);
        func_ov169_06899e28(core->unk1C4);
        core->subSeq = 7;
        break;
    case 7:
        if (func_ov169_06899dfc(core->unk1C4)) {
            GFL_OvlUnload(OVERLAY_ID(285));
            core->subSeq = 0;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

u16 func_ov167_021cf8d8(BtlvCore *core) {
    if (core->ov286.done) {
        return core->ov286.item;
    }
    return 0;
}

u8 func_ov167_021cf8ec(BtlvCore *core) {
    if (core->ov286.done) {
        return core->ov286.cost;
    }
    return 0;
}

u8 func_ov167_021cf900(BtlvCore *core) {
    return core->ov287.partyIndex;
}

u8 func_ov167_021cf908(BtlvCore *core) {
    return core->ov287.slot;
}

void func_ov167_021cf914(BtlvCore *core) {
    if (core->ov286.item != 0 && core->ov287.partyIndex != 6 && core->ov286.mode != 1) {
        func_020088c4(core->ov286.bagCursor, core->ov286.rows, core->ov286.pages);
        func_020088e0(core->ov286.bagCursor, core->ov286.item, core->ov286.pocket);
    }
}

void func_ov167_021cf95c(BtlvCore *core, u32 arg1) {
    if (core->subSeq == 0) {
        core->ov288.done = FALSE;
        core->ov288.heapId = core->heapId;
        core->ov288.font = core->font;
        core->ov288.unk18 = 0;
        core->ov288.unk14 = 0;
        core->ov288.unk08 = func_ov167_0219c7c8(core->mainModule, 1);
        core->ov288.unk0C = func_ov167_0219c9b0(core->mainModule);
        core->ov288.unk10 = arg1;
        core->subSeq = 1;
    }
}

void func_ov167_021cf9c0(BtlvCore *core) {
    core->ov288.unk14 = 1;
}

BOOL func_ov167_021cf9cc(BtlvCore *core) {
    switch (core->subSeq) {
    case 0:
        break;
    case 1:
        func_ov169_06899d9c(core->unk1C4, 1);
        GFL_OvlLoad(OVERLAY_ID(285));
        core->subSeq = 2;
        break;
    case 2:
        if (func_ov169_06899dfc(core->unk1C4)) {
            func_ov169_06899e24(core->unk1C4);
            GFL_OvlLoad(OVERLAY_ID(288));
            func_ov288_021f4440(&core->ov288);
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
        func_ov169_06899dd4(core->unk1C4);
        func_ov169_06899e28(core->unk1C4);
        core->subSeq = 5;
        break;
    case 5:
        if (func_ov169_06899dfc(core->unk1C4)) {
            GFL_OvlUnload(OVERLAY_ID(285));
            core->subSeq = 0;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void func_ov167_021cfaec(BtlvCore *core) {
    if (core->subSeq == 0) {
        core->ov289.done = FALSE;
        core->ov289.heapId = core->heapId;
        core->ov289.font = core->font;
        core->ov289.unk20 = 0;
        core->subSeq = 1;
    }
}

void func_ov167_021cfb28(BtlvCore *core, u32 arg1, u32 *arg2) {
    core->ov289.unk08 = arg1;
    core->ov289.unk1C = arg2;
}

void func_ov167_021cfb34(BtlvCore *core, u32 index, StrBuf *value) {
    core->ov289.unk0C[index] = value;
}

BOOL func_ov167_021cfb40(BtlvCore *core) {
    switch (core->subSeq) {
    case 0:
        break;
    case 1:
        func_ov169_06899d9c(core->unk1C4, 0);
        GFL_OvlLoad(OVERLAY_ID(285));
        core->subSeq = 2;
        break;
    case 2:
        if (func_ov169_06899dfc(core->unk1C4)) {
            func_ov169_06899e24(core->unk1C4);
            GFL_OvlLoad(OVERLAY_ID(289));
            func_ov289_021f4440(&core->ov289);
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
        func_ov169_06899dd4(core->unk1C4);
        func_ov169_06899e28(core->unk1C4);
        core->subSeq = 5;
        break;
    case 5:
        if (func_ov169_06899dfc(core->unk1C4)) {
            GFL_OvlUnload(OVERLAY_ID(285));
            core->subSeq = 0;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void func_ov167_021cfc60(BtlvCore *core, u8 attackerPos, u8 targetPos, u16 move, s32 arg4, u32 arg5, u8 arg6) {
    u8 attacker;
    u8 target;

    attacker = func_ov167_0219c6dc(core->mainModule, attackerPos);
    if (targetPos != 6) {
        target = func_ov167_0219c6dc(core->mainModule, targetPos);
    } else {
        target = 0xff;
    }
    func_ov167_021d3094(core->scu, attacker, target, move, arg4, arg5, arg6);
}

BOOL func_ov167_021cfca4(BtlvCore *core) {
    return func_ov167_021d3130(core->scu);
}

void func_ov167_021cfcb4(BtlvCore *core, u16 arg1, u8 pos, u8 type) {
    BtlvPosEffectWork *work;

    work = func_ov167_021ce93c(core, sizeof(BtlvPosEffectWork));
    work->type = type;
    work->pos = pos;
    work->unk08 = 0;
    work->unk00 = arg1;
    BtlvCore_SetSubProc(core, func_ov167_021cfd20);
}

BOOL func_ov167_021cfce0(BtlvCore *core) {
    return BtlvCore_RunSubProc(core);
}

BOOL func_ov167_021cfd20(s32 *seq, void *arg) {
    BtlvCore *core = arg;
    BtlvPosEffectWork *work;

    work = func_ov167_021ce93c(core, sizeof(BtlvPosEffectWork));
    switch (*seq) {
    case 0:
        func_ov167_021d3188(core->scu, work->pos, work->unk00, func_ov167_021b1990(core->client));
        func_ov167_021cfeec(core, work->type);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d31d0(core->scu)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void func_ov167_021cfd78(BtlvCore *core, u16 arg1, u8 pos, u32 type) {
    BtlvPosEffectWork *work;

    if (!func_ov167_021b1990(core->client)) {
        work = func_ov167_021ce93c(core, sizeof(BtlvPosEffectWork));
        work->type = type;
        work->pos = pos;
        work->unk08 = 0;
        work->unk00 = arg1;
        BtlvCore_SetSubProc(core, func_ov167_021cfdf0);
    }
}

BOOL func_ov167_021cfdb0(BtlvCore *core) {
    return BtlvCore_RunSubProc(core);
}

BOOL func_ov167_021cfdf0(s32 *seq, void *arg) {
    BtlvCore *core = arg;
    BtlvPosEffectWork *work;

    work = func_ov167_021ce93c(core, sizeof(BtlvPosEffectWork));
    switch (*seq) {
    case 0:
        func_ov167_021d31e8(core->scu, work->pos, work->unk00);
        func_ov167_021cfeec(core, work->type);
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d3200(core->scu)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void func_ov167_021cfe40(BtlvCore *core, u16 count, u32 type, const u8 *monIds, u16 arg4) {
    BtlvMultiEffectWork *work;
    u32 i;

    work = func_ov167_021ce93c(core, sizeof(BtlvMultiEffectWork));
    work->type = type;
    work->count = count;
    work->unk10 = arg4;
    work->seq = 0;
    for (i = 0; i < count; i++) {
        work->pos[i] = MonIDToBattlePos(core->mainModule, core->pokeCon, monIds[i]);
    }
}

BOOL func_ov167_021cfe7c(BtlvCore *core) {
    BtlvMultiEffectWork *work;
    BOOL flag;
    u32 i;

    work = func_ov167_021ce93c(core, sizeof(BtlvMultiEffectWork));
    switch (work->seq) {
    case 0:
        flag = func_ov167_021b1990(core->client);
        for (i = 0; i < work->count; i++) {
            func_ov167_021d3188(core->scu, work->pos[i], work->unk10, flag);
        }
        func_ov167_021cfeec(core, work->type);
        work->seq++;
        break;
    case 1:
        if (func_ov167_021d31d0(core->scu) && func_ov167_021d2edc(core->scu)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void func_ov167_021cfeec(BtlvCore *core, u32 type) {
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
    func_ov167_021d0ad4(core, se);
}

void func_ov167_021cff14(BtlvCore *core, u8 pos, u32 visible) {
    if (BtlvEffect_CheckExist(pos) && visible != BtlvEffect_CheckExistPokemon(pos)) {
        if (visible == 1) {
            BtlvEffect_StartEffect285(pos);
        } else {
            BtlvEffect_StartEffect286(pos);
        }
    }
}

void func_ov167_021cff44(BtlvCore *core, u8 pos) {
    func_ov167_021d3414(core->scu, pos, func_ov167_021b1990(core->client));
}

BOOL func_ov167_021cff60(BtlvCore *core) {
    if (func_ov167_021d3450(core->scu)) {
        return TRUE;
    }
    return FALSE;
}

void func_ov167_021cff78(BtlvCore *core, u32 arg1, u16 arg2) {
    if (!func_ov167_021b1990(core->client)) {
        BtlvEffect_StartPos(arg1, arg2);
    }
}

BOOL func_ov167_021cff94(BtlvCore *core) {
    if (!BtlvEffect_IsBusy()) {
        return TRUE;
    }
    return FALSE;
}

void func_ov167_021cffa8(BtlvCore *core, u32 arg1, u32 arg2, u16 arg3) {
    BtlvEffect_StartAtkDef(arg1, arg2, arg3);
}

BOOL func_ov167_021cffb8(BtlvCore *core) {
    if (!BtlvEffect_IsBusy()) {
        return TRUE;
    }
    return FALSE;
}

void func_ov167_021cffcc(BtlvCore *core, u8 pos) {
    func_ov167_021d3214(core->scu, pos, func_ov167_021b1990(core->client));
}

BOOL func_ov167_021cffe8(BtlvCore *core) {
    return func_ov167_021d323c(core->scu);
}

void func_ov167_021cfff8(BtlvCore *core, u8 pos) {
    func_ov167_021d3250(core->scu, pos);
}

BOOL func_ov167_021d0008(BtlvCore *core) {
    return func_ov167_021d3284(core->scu);
}

void func_ov167_021d0018(BtlvCore *core, u8 pos, u16 arg2) {
    func_ov167_021d3298(core->scu, pos, arg2, func_ov167_021b1990(core->client));
}

BOOL func_ov167_021d0038(BtlvCore *core) {
    return func_ov167_021d32f4(core->scu);
}

void func_ov167_021d0048(BtlvCore *core, u8 pos, u8 arg2, u8 arg3) {
    BtlvMonEffectWork *work;
    BattleMon *mon;

    work = func_ov167_021ce93c(core, sizeof(BtlvMonEffectWork));
    mon = func_ov167_0219d188(core->pokeCon, pos);
    work->unk01 = arg2;
    work->unk02 = arg3;
    work->pos = pos;
    work->monId = GetMonID(mon);
    BtlvCore_SetSubProc(core, func_ov167_021d00c4);
}

BOOL func_ov167_021d0084(BtlvCore *core) {
    return BtlvCore_RunSubProc(core);
}

BOOL func_ov167_021d00c4(s32 *seq, void *arg) {
    BtlvCore *core = arg;
    BtlvMonEffectWork *work;

    work = func_ov167_021ce93c(core, sizeof(BtlvMonEffectWork));
    switch (*seq) {
    case 0:
        func_ov167_021d3354(core->scu, work->pos, work->unk01, work->unk02, func_ov167_021b1990(core->client));
        (*seq)++;
        break;
    case 1:
        if (func_ov167_021d33d0(core->scu)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void func_ov167_021d011c(const BtlvStringParam *param, StrBuf *strbuf) {
    switch (param->type) {
    case 1:
        func_ov167_021d4f1c(strbuf, param->message, param->args);
        break;
    case 2:
        func_ov167_021d4f90(strbuf, param->message, param->args);
        break;
    case 3:
        func_ov167_021d575c(strbuf, param->message);
        break;
    case 5:
        func_ov167_021d5770(strbuf, param->message, param->args);
        break;
    case 6:
        func_ov167_021d57b0(strbuf, param->message, param->args);
        break;
    case 4:
        func_ov167_021d5684(strbuf, param->args[0], param->args[1]);
        break;
    case 7:
        func_ov167_021d5700(strbuf, param->args[0], param->args[1]);
        break;
    case 8:
        func_ov167_021d5904(strbuf, param->message);
        break;
    case 9:
        func_ov167_021d5924(strbuf, param->message);
        break;
    }
}

void func_ov167_021d01bc(BtlvCore *core, const StrBuf *src) {
    GFL_StrBufCopy(core->strbuf, src);
}

void func_ov167_021d01c8(BtlvCore *core, const BtlvStringParam *param) {
    func_ov167_021d011c(param, core->strbuf);
}

void func_ov167_021d01d8(BtlvCore *core) {
    func_ov167_021d0308(core, core->strbuf, 0x50, NULL);
}

void func_ov167_021d01ec(BtlvCore *core, const BtlvStringParam *param) {
    func_ov167_021d011c(param, core->strbuf);
    func_ov167_021d0308(core, core->strbuf, param->mode, 0);
}

void func_ov167_021d0210(BtlvCore *core, const BtlvStringParam *param, BtlvMsgCallback callback) {
    func_ov167_021d011c(param, core->strbuf);
    func_ov167_021d0308(core, core->strbuf, param->mode, callback);
}

void func_ov167_021d0234(BtlvCore *core, const BtlvStringParam *param) {
    func_ov167_021d011c(param, core->strbuf);
    func_ov167_021d0330(core, core->strbuf);
}

void func_ov167_021d0250(BtlvCore *core, u16 message, const u32 *args) {
    func_ov167_021d4f1c(core->strbuf, message, args);
    func_ov167_021d0308(core, core->strbuf, 0x50, NULL);
}

void func_ov167_021d026c(BtlvCore *core, u16 message, const u32 *args) {
    func_ov167_021d4f90(core->strbuf, message, args);
    func_ov167_021d0308(core, core->strbuf, 0x50, NULL);
}

BOOL func_ov167_021d0288(BtlvCore *core, u32 trainerId, u32 msgId) {
    if (TrainerMsg_CheckExists(trainerId, msgId, core->heapId)) {
        TrainerMsg_Load(trainerId, msgId, core->strbuf, core->heapId);
        func_ov167_021d0308(core, core->strbuf, 0x50, NULL);
    } else {
        return FALSE;
    }
    return TRUE;
}

void func_ov167_021d02cc(BtlvCore *core, u8 monId, u16 move) {
    func_ov167_021d5684(core->strbuf, monId, move);
    func_ov167_021d0308(core, core->strbuf, 0x50, NULL);
}

BOOL func_ov167_021d02e8(BtlvCore *core) {
    return func_ov167_021d2edc(core->scu);
}

BOOL func_ov167_021d02f8(BtlvCore *core) {
    return func_ov167_021d2ec4(core->scu);
}

void func_ov167_021d0308(BtlvCore *core, const StrBuf *strbuf, u16 wait, BtlvMsgCallback callback) {
    if (!func_ov167_021b1990(core->client)) {
        func_ov167_021d2e20(core->scu, strbuf, wait, callback);
    }
}

void func_ov167_021d0330(BtlvCore *core, const StrBuf *strbuf) {
    func_ov167_021d2dbc(core->scu, strbuf);
}

void func_ov167_021d0340(BtlvCore *core, u8 pos, BOOL flash) {
    func_ov167_021d39e4(core->scu, pos, flash);
}

BOOL func_ov167_021d0350(BtlvCore *core, u8 pos) {
    return func_ov167_021d3a1c(core->scu, pos);
}

void func_ov167_021d0360(BtlvCore *core, u8 pos) {
    func_ov167_021d3a38(core->scu, pos);
}

BOOL func_ov167_021d0370(BtlvCore *core, u8 pos) {
    return func_ov167_021d3a68(core->scu, pos);
}

void func_ov167_021d0380(BtlvCore *core, u8 pos) {
    func_ov167_021d3a88(core->scu, pos);
}

BOOL func_ov167_021d0390(BtlvCore *core, u8 pos) {
    return func_ov167_021d3aa8(core->scu, pos);
}

void func_ov167_021d03a0(BtlvCore *core, u32 arg1) {
    BtlvEffect_StartPos(arg1, 0x262);
}

void func_ov167_021d03b0(BtlvCore *core, u32 arg1) {
    BtlvEffect_StartPos(arg1, 0x261);
}

BOOL func_ov167_021d03c0(BtlvCore *core, u32 arg1) {
    if (!BtlvEffect_IsBusy()) {
        return TRUE;
    }
    return FALSE;
}

void func_ov167_021d03d4(BtlvCore *core) {
    BtlvEffect_SetIdleEffectMode(3);
    func_ov167_021d4194(core->scu);
}

BOOL func_ov167_021d03ec(BtlvCore *core) {
    return func_ov167_021d41b0(core->scu);
}

void func_ov167_021d03fc(BtlvCore *core) {
    BtlvEffect_StopIdleEffect();
    func_ov167_021d41b8(core->scu);
}

void func_ov167_021d0410(BtlvCore *core, u8 pos) {
    func_ov167_021d34ec(core->scu, func_ov167_0219c6dc(core->mainModule, pos));
}

BOOL func_ov167_021d0428(BtlvCore *core, u8 pos) {
    return func_ov167_021d34fc(core->scu, func_ov167_0219c6dc(core->mainModule, pos));
}

void func_ov167_021d0440(BtlvCore *core, u8 pos, u32 arg2) {
    func_ov167_021d3510(core->scu, pos, arg2);
}

BOOL func_ov167_021d0450(BtlvCore *core) {
    return func_ov167_021d3558(core->scu);
}

void func_ov167_021d0460(BtlvCore *core, u32 arg1) {
    func_ov167_021d363c(core->scu, arg1, func_ov167_021b1990(core->client));
}

BOOL func_ov167_021d047c(BtlvCore *core) {
    return func_ov167_021d3694(core->scu);
}

void func_ov167_021d048c(BtlvCore *core, u32 arg1, u32 arg2) {
    func_ov167_021d35e0(core->scu, arg1, arg2, func_ov167_021b1990(core->client));
}

BOOL func_ov167_021d04ac(BtlvCore *core) {
    return func_ov167_021d3694(core->scu);
}

void func_ov167_021d04bc(BtlvCore *core) {
    func_ov167_021d3830(core->scu);
}

BOOL func_ov167_021d04cc(BtlvCore *core) {
    return func_ov167_021d385c(core->scu);
}

void func_ov167_021d04dc(BtlvCore *core, u8 clientId, u8 arg2, u8 arg3, u8 slot1, u8 slot2) {
    BtlvSwapWork *work;

    work = func_ov167_021ce93c(core, sizeof(BtlvSwapWork));
    work->clientId = clientId;
    work->unk01 = arg2;
    work->unk02 = arg3;
    work->slot1 = slot1;
    work->slot2 = slot2;
    work->pos1 = func_ov167_0219c458(core->mainModule, clientId, slot1);
    work->pos2 = func_ov167_0219c458(core->mainModule, clientId, slot2);
    BtlvCore_SetSubProc(core, func_ov167_021d0568);
}

BOOL func_ov167_021d0528(BtlvCore *core) {
    return BtlvCore_RunSubProc(core);
}

BOOL func_ov167_021d0568(s32 *seq, void *arg) {
    BtlvCore *core = arg;
    BtlvSwapWork *work;

    work = func_ov167_021ce93c(core, sizeof(BtlvSwapWork));
    switch (*seq) {
    case 0:
        BtlvEffect_SwapPokemon(work->unk01, work->unk02);
        func_ov167_021d0ad4(core, 0x57b);
        (*seq)++;
        break;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            func_ov167_021d3464(core->scu, work->pos1, work->pos2);
            (*seq)++;
        }
        break;
    case 2:
        if (func_ov167_021d3490(core->scu, work->pos1, work->pos2)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void func_ov167_021d05d4(BtlvCore *core, u8 arg1) {
    void *work;

    work = func_ov167_021ce93c(core, 0x3c);
    GetClientParty(core->pokeCon, core->clientId);
    func_ov169_0689b670(core->unk1C4, work);
}

BOOL func_ov167_021d0608(BtlvCore *core, u8 *out) {
    if (func_ov169_0689b680(core->unk1C4, out)) {
        return TRUE;
    }
    return FALSE;
}

void func_ov167_021d0630(BtlvCore *core) {
    func_ov169_0689b6b4(core->unk1C4);
}

void func_ov167_021d0640(BtlvCore *core, u8 clientId, u8 arg2) {
    BtlvRotateWork *work;

    if (arg2 != 1) {
        work = func_ov167_021ce93c(core, sizeof(BtlvRotateWork));
        work->clientId = clientId;
        work->unk01 = arg2;
        work->pos[0] = func_ov167_0219c458(core->mainModule, clientId, 0);
        work->pos[1] = func_ov167_0219c458(core->mainModule, clientId, 1);
        work->pos[2] = func_ov167_0219c458(core->mainModule, clientId, 2);
        work->viewPos[0] = func_ov167_0219c6dc(core->mainModule, work->pos[0]);
        work->viewPos[1] = func_ov167_0219c6dc(core->mainModule, work->pos[1]);
        work->viewPos[2] = func_ov167_0219c6dc(core->mainModule, work->pos[2]);
        BtlvCore_SetSubProc(core, func_ov167_021d06ec);
    }
}

BOOL func_ov167_021d06ac(BtlvCore *core) {
    return BtlvCore_RunSubProc(core);
}

BOOL func_ov167_021d06ec(s32 *seq, void *arg) {
    BtlvCore *core = arg;
    BtlvRotateWork *work;
    BOOL done;

    work = func_ov167_021ce93c(core, sizeof(BtlvRotateWork));
    switch (*seq) {
    case 0:
        BtlvEffect_StartRotation(work->unk01, work->viewPos[0] & 1, !func_ov167_021b1990(core->client));
        (*seq)++;
        break;
    case 1:
        if (!BtlvEffect_IsBusy()) {
            func_ov167_021d34bc(core->scu, work->pos[0]);
            func_ov167_021d34bc(core->scu, work->pos[1]);
            func_ov167_021d34bc(core->scu, work->pos[2]);
            (*seq)++;
        }
        break;
    case 2:
        done = TRUE;
        if (!func_ov167_021d34d4(core->scu, work->pos[0])) {
            done = FALSE;
        }
        if (!func_ov167_021d34d4(core->scu, work->pos[1])) {
            done = FALSE;
        }
        if (!func_ov167_021d34d4(core->scu, work->pos[2])) {
            done = FALSE;
        }
        if (done) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void func_ov167_021d0798(BtlvCore *core, const BtlvStringParam *param1, const BtlvStringParam *param2, u32 arg3) {
    BtlvTwoStringWork *work;

    work = func_ov167_021ce93c(core, sizeof(BtlvTwoStringWork));
    work->first = GFL_StrBufCreate(0x20, HEAPID_TAIL(core->heapId));
    work->second = GFL_StrBufCreate(0x20, HEAPID_TAIL(core->heapId));
    work->unk08 = arg3;
    func_ov167_021d011c(param1, work->first);
    func_ov167_021d011c(param2, work->second);
    func_ov169_0689b700(core->unk1C4, work);
    GFL_HeapFree(work->first);
    GFL_HeapFree(work->second);
}

BOOL func_ov167_021d0828(BtlvCore *core, u32 *answer) {
    return func_ov169_0689b728(core->unk1C4, answer);
}

void func_ov167_021d0838(BtlvCore *core, u8 partyIndex, u16 move) {
    func_ov167_021cf234(core, &core->ov287, 4, partyIndex, move);
    core->subSeq = 0;
}

BOOL func_ov167_021d0854(BtlvCore *core, u8 *slot) {
    switch (core->subSeq) {
    case 0:
        func_ov169_06899d9c(core->unk1C4, 0);
        GFL_OvlLoad(OVERLAY_ID(285));
        core->subSeq++;
        break;
    case 1:
        if (func_ov169_06899dfc(core->unk1C4)) {
            func_ov169_06899e24(core->unk1C4);
            GFL_OvlLoad(OVERLAY_ID(287));
            BPlistMain_Start(&core->ov287);
            core->subSeq++;
        }
        break;
    case 2:
        if (core->ov287.done) {
            GFL_OvlUnload(OVERLAY_ID(287));
            func_ov169_06899dd4(core->unk1C4);
            func_ov169_06899e28(core->unk1C4);
            core->subSeq++;
        }
        break;
    case 3:
        if (func_ov169_06899dfc(core->unk1C4)) {
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

void func_ov167_021d0978(BtlvCore *core, BattleMon *mon, const BattleMonLevelUp *levelUp) {
    func_ov167_021d41f8(core->scu, mon, levelUp);
}

BOOL func_ov167_021d0988(BtlvCore *core) {
    return func_ov167_021d4234(core->scu);
}

void func_ov167_021d0998(BtlvCore *core) {
    func_ov167_021d428c(core->scu);
}

BOOL func_ov167_021d09a8(BtlvCore *core) {
    return func_ov167_021d42ac(core->scu);
}

void func_ov167_021d09b8(BtlvCore *core) {
    func_ov167_021d4304(core->scu);
}

BOOL func_ov167_021d09c8(BtlvCore *core) {
    return func_ov167_021d4324(core->scu);
}

void func_ov167_021d09d8(BtlvCore *core) {
    func_ov167_021d41bc(core->scu);
}

BOOL func_ov167_021d09e8(BtlvCore *core) {
    return func_ov167_021d41f0(core->scu);
}

void func_ov167_021d09f8(BtlvCore *core) {
    func_ov167_021d41d0(core->scu);
}

BOOL func_ov167_021d0a08(BtlvCore *core) {
    return func_ov167_021d41f0(core->scu);
}

void BattleClientCmd_ForceQuitInputNotify(BtlvCore *core) {
    func_ov169_0689b7cc(core->unk1C4);
}

BOOL BattleClientCmd_ForceQuitInputWait(BtlvCore *core) {
    return func_ov169_0689b7f8(core->unk1C4);
}

u32 func_ov167_021d0a38(BtlvCore *core) {
    return func_ov169_0689b8ec(core->unk1C4);
}

void func_ov167_021d0a48(BtlvCore *core, u16 arg1, u16 arg2) {
    core->unk1A0.unk00 = arg1;
    core->unk1A0.unk04 = arg2;
    core->unk1A0.unk0C = 0;
    func_ov169_0689b8dc(core->unk1C4, &core->unk1A0);
}

void func_ov167_021d0a7c(BtlvCore *core, u16 arg1) {
    core->unk1A0.unk00 = arg1;
    core->unk1A0.unk04 = arg1;
    core->unk1A0.unk0C = 3;
    func_ov169_0689b8dc(core->unk1C4, &core->unk1A0);
}

void func_ov167_021d0aa0(BtlvCore *core, u32 arg1, u32 arg2) {
    core->unk1A0.unk00 = arg1;
    core->unk1A0.unk04 = arg1;
    core->unk1A0.unk0C = arg2;
    func_ov169_0689b8dc(core->unk1C4, &core->unk1A0);
}

void func_ov167_021d0ad4(BtlvCore *core, u32 se) {
    if (!func_ov167_021b1990(core->client)) {
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

void func_ov167_021d0b4c(BtlvStringParam *param, u8 mode) {
    param->mode = mode;
}

void func_ov167_021d0b50(BtlvCore *core, u32 arg1, u32 arg2) {
    if (func_ov167_0219c988(core->mainModule) == 1) {
        BtlvClact_SetGauge(BtlvEffect_GetClact(), arg1, arg2);
    }
}

void func_ov167_021d0b70(BtlvCore *core, u32 arg1) {
    if (func_ov167_0219c988(core->mainModule) == 1) {
        BtlvClact_ShowPopup(BtlvEffect_GetClact(), arg1, core->font);
    }
}

BOOL func_ov167_021d0b90(BtlvCore *core) {
    if (func_ov167_0219c988(core->mainModule) == 1) {
        return BtlvClact_IsPopupShowing(BtlvEffect_GetClact());
    }
    return FALSE;
}

void func_ov167_021d0bac(BtlvCore *core, u32 arg1, u32 arg2, u32 arg3) {
    if (func_ov167_0219c988(core->mainModule) == 2) {
        func_ov169_0689b904(core->unk1C4, arg1, arg2, arg3);
    }
}

u32 func_ov167_021d0be4(BtlvCore *core) {
    return func_ov169_0689b920(core->unk1C4);
}

void func_ov167_021d0bf4(BtlvCore *core, u32 arg1) {
    if (func_ov167_0219c988(core->mainModule) == 2) {
        func_ov169_0689b92c(core->unk1C4, arg1);
    }
}
