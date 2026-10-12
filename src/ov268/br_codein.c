// The Battle Recorder's code input, where the player types a battle video's number on the lower screen: its work,
// and its parameter. The ROM names this file. The states and the input are in br_codein_pv.c and the graphics in
// br_codein_disp.c, as in overlay 205's code input screen, which this is a copy of. Our names

#include "types.h"
#include "app/battle_recorder/br_codein.h"
#include "app/battle_recorder/br_codein_pv.h"
#include "app/battle_recorder/br_res.h"
#include "app/battle_recorder/br_util.h"
#include "gfl/arc.h"
#include "gfl/button_man.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"

// The archive of the code input screen's graphics, which nothing is loaded from here
#define ARCID_CODEIN 58

static void BrCodeIn_VBlank(TCB *tcb, void *data);

BrCodeInWork *BrCodeIn_Init(const BrCodeInParam *param, HeapID heapId) {
    BrCodeInWork *wk = GFL_HeapAllocate(heapId, sizeof(BrCodeInWork), FALSE, "br_codein.c", 56);

    sys_memset(wk, 0, sizeof(BrCodeInWork));
    wk->param = *param;
    wk->heapId = heapId;
    wk->unit = param->unit;
    wk->font = BrRes_GetFont(param->res);
    wk->arc = GFL_ArcSysCreateFileHandle(ARCID_CODEIN, wk->heapId);
    BrCodeIn_InitParam(wk);
    BrCodeIn_InitButtons(wk);
    wk->vblankTask = GFL_VBlankTCBAdd(BrCodeIn_VBlank, wk, 16);
    return wk;
}

void BrCodeIn_Main(BrCodeInWork *wk) {
    BrCodeIn_MainPv(wk);
}

int BrCodeIn_GetResult(const BrCodeInWork *wk) {
    return wk->result;
}

u64 BrCodeIn_GetNumber(const BrCodeInWork *wk) {
    return wk->number;
}

void BrCodeIn_Exit(BrCodeInWork *wk) {
    GFL_TCBRemove(wk->vblankTask);
    BrCodeInDisp_Free(wk);
    GFL_ArcToolFree(wk->arc);
    GFL_BMN_Delete(wk->buttons);
    GFL_HeapFree(wk);
}

BrCodeInParam *BrCodeInParam_Create(u32 heapId, u32 length, ClActUnit *unit, BrRes *res, BrBallEffect *ballEff,
                                    const int *blocks) {
    int i = 0;
    BrCodeInParam *param = GFL_HeapAllocate(heapId, sizeof(BrCodeInParam), FALSE, "br_codein.c", 192);

    param->unit = unit;
    param->res = res;
    param->length = length;
    param->strbuf = GFL_StrBufCreate(length + 1, heapId);
    param->ballEff = ballEff;
    for (; i < BR_CODEIN_BLOCK_COUNT; i++) {
        param->blocks[i] = blocks[i];
    }
    param->blocks[i] = blocks[i - 1];
    return param;
}

// The parameter and its string keep the original's names, which the assertions' texts hold
void BrCodeInParam_Free(BrCodeInParam *codein_param) {
    GFL_ASSERT(codein_param->strbuf != NULL);
    GFL_ASSERT(codein_param != NULL);
    GFL_StrBufFree(codein_param->strbuf);
    GFL_HeapFree(codein_param);
}

static void BrCodeIn_VBlank(TCB *tcb, void *data) {
}
