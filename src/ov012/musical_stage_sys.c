#include "types.h"
#include "constants/species.h"
#include "field/musical.h"
#include "field/musical_program.h"
#include "field/musical_stage_sys.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"

typedef struct {
    MusicalStageParam *param;
    // Overlay 209's stage
    StaActing *stage;
    u32 unk8;
} MusicalStageWork;

static BOOL MusicalStageProc_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MusicalStageProc_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MusicalStageProc_Main(GameProc *proc, u32 *state, void *param, void *work);

GameProcFunctions data_ov012_0216dfe8 = {
    MusicalStageProc_Init,
    MusicalStageProc_Main,
    MusicalStageProc_Exit,
};

static BOOL MusicalStageProc_Init(GameProc *proc, u32 *state, void *param, void *work) {
    MusicalStageWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_MUSICAL, 0xa8000);
    GFL_OvlLoad(OVERLAY_ID(209));
    wk = GFL_ProcInitSubsystem(proc, sizeof(MusicalStageWork), HEAPID_MUSICAL);
    if (param == NULL) {
        // A test show of four Spinda
        wk->param = MusicalStageParam_Create(HEAPID_MUSICAL, NULL);
        MusicalStageParam_CreateNpcPoke(wk->param, 0, SPECIES_SPINDA, 0, 0xff000000, 60, HEAPID_MUSICAL);
        MusicalStageParam_CreateNpcPoke(wk->param, 1, SPECIES_SPINDA, 0, 0xff0000, 120, HEAPID_MUSICAL);
        MusicalStageParam_CreateNpcPoke(wk->param, 2, SPECIES_SPINDA, 0, 0xff00, 0xfffd, HEAPID_MUSICAL);
        MusicalStageParam_CreateNpcPoke(wk->param, 3, SPECIES_SPINDA, 0, 0xff, 0xffff, HEAPID_MUSICAL);
        MusicalStageParam_SetEquip(wk->param, 0, 7, 19, 0, 0);
        MusicalStageParam_SetEquip(wk->param, 0, 8, 19, 0, 1);
        MusicalStageParam_SetEquip(wk->param, 0, 2, 16, 0, 2);
        MusicalStageParam_SetEquip(wk->param, 1, 1, 7, 0, 0);
        MusicalStageParam_SetEquip(wk->param, 1, 5, 9, 0, 1);
        MusicalStageParam_SetEquip(wk->param, 1, 7, 24, 0, 2);
        MusicalStageParam_SetEquip(wk->param, 1, 8, 43, 0, 3);
        MusicalStageParam_SetEquip(wk->param, 2, 7, 31, 0, 0);
        MusicalStageParam_SetEquip(wk->param, 2, 8, 30, 0, 1);
        MusicalStageParam_SetEquip(wk->param, 2, 2, 15, 0, 2);
        MusicalStageParam_SetEquip(wk->param, 3, 7, 30, 0, 0);
        MusicalStageParam_SetEquip(wk->param, 3, 8, 30, 0, 1);
        MusicalStageParam_SetEquip(wk->param, 3, 2, 21, 0, 2);
        wk->param->pokes[1]->owner = 0;
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_TAIL(HEAPID_MUSICAL_EVENT), 0x80000);
        wk->param->ov210 = MusicalSystem_InitProgramData(HEAPID_USER);
        MusicalSystem_LoadProgramData(wk->param->ov210, NULL, NULL, 0, HEAPID_MUSICAL_EVENT);
        wk->param->program = MusicalProgram_Create(HEAPID_MUSICAL, wk->param->ov210, 0);
        MusicalProgram_CalcPoints(HEAPID_MUSICAL, wk->param->program, wk->param);
        wk->param->pokes[0]->points = 75;
        wk->param->pokes[1]->points = 45;
        wk->param->pokes[2]->points = 30;
        wk->param->pokes[3]->points = 0;
    } else {
        wk->param = param;
    }
    return TRUE;
}

static BOOL MusicalStageProc_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    MusicalStageWork *wk = work;

    if (param == NULL) {
        MusicalProgram_Free(wk->param->program);
        MusicalSystem_FreeProgramData(wk->param->ov210);
        GFL_HeapDelete(HEAPID_MUSICAL_EVENT);
        wk->param->pokes[1]->owner = 2;
        MusicalStageParam_Free(wk->param);
    }
    GFL_ProcReleaseSubsystem(proc);
    GFL_OvlUnload(OVERLAY_ID(209));
    GFL_HeapDelete(HEAPID_MUSICAL);
    return TRUE;
}

static BOOL MusicalStageProc_Main(GameProc *proc, u32 *state, void *param, void *work) {
    MusicalStageWork *wk = work;

    switch (*state) {
    case 0:
        wk->stage = StaActing_Init(wk->param, HEAPID_MUSICAL);
        *state = 1;
        break;
    case 1:
        if (StaActing_Main(wk->stage) == TRUE) {
            *state = 2;
        }
        break;
    case 2:
        StaActing_Term(wk->stage);
        return TRUE;
    }
    return FALSE;
}

MusicalStageParam *MusicalStageParam_Create(HeapID heapId, MusCommWork *comm) {
    MusicalStageParam *param;
    u8 i;

    param = GFL_HeapAllocate(heapId, sizeof(MusicalStageParam), FALSE, "musical_stage_sys.c", 193);
    param->comm = comm;
    for (i = 0; i < 4; i++) {
        param->pokes[i] = NULL;
    }
    return param;
}

void MusicalStageParam_Free(MusicalStageParam *param) {
    u8 i;

    for (i = 0; i < 4; i++) {
        if (param->pokes[i] != NULL && param->pokes[i]->owner == 2) {
            GFL_HeapFree(param->pokes[i]);
        }
    }
    GFL_HeapFree(param);
}

void MusicalStageParam_SetPlayerPoke(MusicalStageParam *param, u8 pos, MusicalPoke *poke) {
    param->pokes[pos] = poke;
    param->pokes[pos]->owner = 0;
}

void MusicalStageParam_CreateNpcPoke(MusicalStageParam *param, u8 pos, u16 species, u8 form, u32 personality, u16 a5,
                                     HeapID heapId) {
    param->pokes[pos] = MusicalSystem_InitPoke(species, form, 0, 0, personality, heapId);
    param->pokes[pos]->owner = 2;
    param->pokes[pos]->unk78 = a5;
}

void MusicalStageParam_SetCommPoke(MusicalStageParam *param, u8 pos, MusicalPoke *poke) {
    param->pokes[pos] = poke;
    param->pokes[pos]->owner = 1;
}

void MusicalStageParam_SetEquip(MusicalStageParam *param, u8 pos, u8 slot, u16 itemId, s16 a4, u8 a5) {
    MusicalPoke *poke = param->pokes[pos];

    poke->equips[slot].itemId = itemId;
    poke->equips[slot].unk2 = a4;
    poke->equips[slot].slot = a5;
}
