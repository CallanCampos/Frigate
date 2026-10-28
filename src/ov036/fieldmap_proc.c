// The field map's process, which makes the field's heaps and its Field (fieldmap.c) and runs the field's routines.
// The ROM doesn't name the file; the name is a guess. Its FIELDMAP_PROC_FUNCS lies ahead of fieldmap.c's smallest
// table, and no layout of one file gives that: a global that no code refers to gets a section of its own after the
// shared one, and the shared one starts with its smallest object. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field.h"
#include "field/field_internal.h"
#include "field/field_pokemon_form.h"
#include "field/player_state.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "system/game_comm.h"
#include "system/game_system.h"

// The field menu's heap, which the field map's heap leaves room for, and the cell actors' heap, in memory of its own
#define FIELD_MENU_HEAP_SIZE 0xf000
#define FIELD_CLACT_HEAP_SIZE 0x10000

// The steps of FieldmapProc_Init
enum {
    SEQ_CREATE_FIELD,
    SEQ_END_COMM,
};

u8 g_FieldHeapMemory[FIELD_CLACT_HEAP_SIZE];

const GameProcFunctions FIELDMAP_PROC_FUNCS = { FieldmapProc_Init, FieldmapProc_Update, FieldmapProc_End };

BOOL FieldmapProc_Init(GameProc *proc, u32 *seq, void *param, void *work) {
    GameSystem *gsys = param;

    switch (*seq) {
    case SEQ_CREATE_FIELD: {
        Field **procWork;
        u32 heapSize = GetFieldmapZoneHeapSize(PlayerState_GetZoneID(GSYS_GetPlayerState(gsys))) - FIELD_MENU_HEAP_SIZE;

        GFL_HeapCreateChild(HEAPID_USER, HEAPID_FIELDMAP, heapSize);
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_FIELD_MENU, FIELD_MENU_HEAP_SIZE);
        GFL_HeapCreateChild(HEAPID_FIELDMAP, HEAPID_FIELD_PARTICLE, 0xc000);
        GFL_HeapCreateRoot(g_FieldHeapMemory, FIELD_CLACT_HEAP_SIZE, HEAPID_FIELD_CLACT);
        GFL_HeapCreateChild(HEAPID_FIELD_CLACT, HEAPID_FIELD_WEATHER, 0x6400);
        GFL_HeapCreateChild(HEAPID_FIELD_CLACT, HEAPID_FIELD_PLACE_NAME, 0x7400);
        GFL_HeapCreateChild(HEAPID_FIELD_CLACT, HEAPID_FIELD_SCENEAREA, 0x500);
        procWork = GFL_ProcInitSubsystem(proc, sizeof(Field *), HEAPID_FIELDMAP);
        *procWork = Field_Create(gsys, HEAPID_FIELDMAP);
        GSYS_SetField(gsys, *procWork);
        (*seq)++;
        break;
    }
    case SEQ_END_COMM:
        // Without the C-Gear, the beacon search and Entralink end
        if (!GSYS_IsCGearOn(gsys)) {
            GameCommSys *comm = GSYS_GetGameCommSystem(gsys);

            switch (GameCommSys_BootCheck(comm)) {
            case GAME_COMM_NO_BEACON_SEARCH:
            case GAME_COMM_NO_INTRUDE:
                GameCommSys_ExitReq(comm);
                break;
            }
        }
        CheckResetKeldeoOrdinaryForme(gsys);
        return TRUE;
    }
    return FALSE;
}

BOOL FieldmapProc_Update(GameProc *proc, u32 *seq, void *param, void *work) {
    if (Field_CallRoutines(param, *(Field **)work) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

BOOL FieldmapProc_End(GameProc *proc, u32 *seq, void *param, void *work) {
    GameSystem *gsys = param;

    Field_Free(*(Field **)work);
    GSYS_SetField(gsys, NULL);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_FIELD_SCENEAREA);
    GFL_HeapDelete(HEAPID_FIELD_WEATHER);
    GFL_HeapDelete(HEAPID_FIELD_PLACE_NAME);
    GFL_HeapDelete(HEAPID_FIELD_CLACT);
    GFL_HeapDelete(HEAPID_FIELD_PARTICLE);
    GFL_HeapDelete(HEAPID_FIELD_MENU);
    GFL_HeapDelete(HEAPID_FIELDMAP);
    CheckResetKeldeoOrdinaryForme(gsys);
    return TRUE;
}
