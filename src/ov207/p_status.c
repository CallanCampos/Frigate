#include "app/p_status.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/proc.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "gfl/ui.h"
#include "p_status_local.h"
#include "pml/poke_party.h"
#include "system/game_data.h"

// The summary screen's process. Started without a parameter, as from a debug menu, it makes one of its own: a party
// of five test Pokémon, or with X held a box of thirty, some of them empty

#define DEBUG_PARTY_COUNT 5
#define DEBUG_BOX_COUNT 30

static BOOL PStatus_ProcInit(GameProc *proc, u32 *state, void *data, void *work);
static BOOL PStatus_ProcExit(GameProc *proc, u32 *state, void *data, void *work);
static BOOL PStatus_ProcMain(GameProc *proc, u32 *state, void *data, void *work);

GameProcFunctions PSTATUS_PROC_FUNCTIONS = { PStatus_ProcInit, PStatus_ProcMain, PStatus_ProcExit };

static BOOL PStatus_ProcInit(GameProc *proc, u32 *state, void *data, void *work) {
    PStatusParam *param = data;
    PStatusWork *wk;
    PokeParty *party;
    PartyPkm *partyPkm;
    BoxPkm *boxPkm;
    u8 i;

    switch (*state) {
    case 0:
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_P_STATUS, 0x50000);
        wk = GFL_ProcInitSubsystem(proc, sizeof(PStatusWork), HEAPID_P_STATUS);
        MI_CpuFill8(wk, 0, sizeof(PStatusWork));
        wk->heapId = HEAPID_P_STATUS;
        if (param == NULL) {
            param = GFL_HeapAllocate(HEAPID_USER, sizeof(PStatusParam), FALSE, "p_status.c", 77);
            if (!(GCTX_HIDGetHeldKeys() & PAD_BUTTON_X)) {
                party = PokeParty_Create(HEAPID_USER);
                PokeParty_InitCore(party, 6);
                for (i = 0; i < DEBUG_PARTY_COUNT; i++) {
                    // The trainer ID 0xffffffff, not PKM_ID_RANDOM
                    partyPkm = PokeParty_NewTempPkm(i + 3, 10, 0xffffffff, HEAPID_USER);
                    {
                        // "ブラック", Black
                        u16 otName[5] = { 0x30d6, 0x30e9, 0x30c3, 0x30af, 0xffff };

                        PokeParty_SetParam(partyPkm, PKM_PARAM_OT_NAME_RAW, (u32)otName);
                    }
                    PokeParty_SetParam(partyPkm, PKM_PARAM_OT_GENDER, 0);
                    PokeParty_AddPkm(party, partyPkm);
                    GFL_HeapFree(partyPkm);
                }
                param->partyIndex = 0;
                param->party = party;
                param->dataType = PSTATUS_DATA_PARTY;
                param->partyCount = DEBUG_PARTY_COUNT;
            } else {
                param->party =
                    GFL_HeapAllocate(HEAPID_USER, PML_GetPkmRawSize() * DEBUG_BOX_COUNT, TRUE, "p_status.c", 129);
                param->dataType = PSTATUS_DATA_BOX;
                param->partyCount = DEBUG_BOX_COUNT;
                param->partyIndex = 0xff;
                for (i = 0; i < param->partyCount; i++) {
                    boxPkm = (BoxPkm *)((u8 *)param->party + PML_GetPkmRawSize() * i);
                    PML_PkmInit(boxPkm);
                    if (GFL_RandomLCAlt(3) != 0) {
                        PML_CreateTempPkm(boxPkm, i + 1, 50, PKM_ID_RANDOM);
                        {
                            // "ブラック", Black
                            u16 otName[5] = { 0x30d6, 0x30e9, 0x30c3, 0x30af, 0xffff };

                            PML_PkmSetParam(boxPkm, PKM_PARAM_OT_NAME_RAW, (u32)otName);
                        }
                        PML_PkmSetParam(boxPkm, PKM_PARAM_OT_GENDER, 0);
                        if (param->partyIndex == 0xff) {
                            param->partyIndex = i;
                        }
                    }
                }
            }
            param->mode = PSTATUS_MODE_NORMAL;
            param->page = PSTATUS_PAGE_INFO;
            param->fromFieldMenu = TRUE;
            param->isNationalDex = TRUE;
            param->gameData = GameData_Create(HEAPID_USER);
            if (GCTX_HIDGetHeldKeys() & PAD_BUTTON_X) {
                param->mode = PSTATUS_MODE_FORGET_MOVE;
                param->fromFieldMenu = FALSE;
                param->move = MOVE_SCRATCH;
            }
            if (GCTX_HIDGetHeldKeys() & PAD_BUTTON_R) {
                param->mode = PSTATUS_MODE_1;
                param->fromFieldMenu = FALSE;
            }
            GCTX_HIDSetUsingTouch(TRUE);
        }
        wk->param = param;
        *state = 1;
        break;
    case 1:
        if (PStatus_Init(work) == TRUE) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL PStatus_ProcExit(GameProc *proc, u32 *state, void *data, void *work) {
    PStatusWork *wk = work;

    if (PStatus_Exit(wk) == FALSE) {
        return FALSE;
    }
    if (data == NULL) {
        GameData_Free(wk->param->gameData);
        GFL_HeapFree(wk->param->party);
        GFL_HeapFree(wk->param);
    }
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_P_STATUS);
    return TRUE;
}

static BOOL PStatus_ProcMain(GameProc *proc, u32 *state, void *data, void *work) {
    int result = PStatus_Main(work);

    if (result == PSTATUS_RESULT_BACK) {
        return TRUE;
    }
    if (result == PSTATUS_RESULT_CLOSE) {
        return TRUE;
    }
    return FALSE;
}
