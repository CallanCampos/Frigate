// The ROM names this file zukan_detail_procsys.c.
#include "types.h"
#include "app/pokedex_detail.h"
#include "gfl/heap.h"

// Runs one of the Pokédex detail screens' pages (its info, map, cry and forms) as a small process: its init, main and
// exit functions in turn, plus a command and a draw function that the detail screen calls every frame

enum {
    PROCSYS_INIT,
    PROCSYS_MAIN,
    PROCSYS_EXIT,
};

PokedexDetailProcSys *PokedexDetailProcSys_Create(const PokedexDetailProcFuncs *funcs, void *param, HeapID heapId) {
    PokedexDetailProcSys *sys =
        GFL_HeapAllocate(heapId, sizeof(PokedexDetailProcSys), FALSE, "zukan_detail_procsys.c", 79);
    sys->funcs = funcs;
    sys->seq = PROCSYS_INIT;
    sys->subSeq = 0;
    sys->param = param;
    sys->work = NULL;
    return sys;
}

void PokedexDetailProcSys_Free(PokedexDetailProcSys *sys) {
    GFL_HeapFree(sys);
}

BOOL PokedexDetailProcSys_Main(PokedexDetailProcSys *sys, PokedexDetailCommon *common) {
    BOOL done = TRUE;

    switch (sys->seq) {
    case PROCSYS_INIT:
        if (sys->funcs->init != NULL) {
            done = sys->funcs->init(sys, &sys->subSeq, sys->param, sys->work, common);
        }
        if (done == TRUE) {
            sys->seq = PROCSYS_MAIN;
            sys->subSeq = 0;
        }
        break;
    case PROCSYS_MAIN:
        if (sys->funcs->main != NULL) {
            done = sys->funcs->main(sys, &sys->subSeq, sys->param, sys->work, common);
        }
        if (done == TRUE) {
            sys->seq = PROCSYS_EXIT;
            sys->subSeq = 0;
        }
        break;
    case PROCSYS_EXIT:
        if (sys->funcs->exit != NULL) {
            done = sys->funcs->exit(sys, &sys->subSeq, sys->param, sys->work, common);
        }
        if (done == TRUE) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void PokedexDetailProcSys_Command(PokedexDetailProcSys *sys, PokedexDetailCommon *common, int command) {
    if (sys->funcs->command != NULL) {
        sys->funcs->command(sys, &sys->subSeq, sys->param, sys->work, common, command);
    }
}

void PokedexDetailProcSys_Draw(PokedexDetailProcSys *sys, PokedexDetailCommon *common) {
    if (sys->funcs->draw != NULL) {
        sys->funcs->draw(sys, &sys->subSeq, sys->param, sys->work, common);
    }
}

void *PokedexDetailProcSys_AllocWork(PokedexDetailProcSys *sys, u32 size, HeapID heapId) {
    sys->work = GFL_HeapAllocate(heapId, size, FALSE, "zukan_detail_procsys.c", 200);
    return sys->work;
}

void PokedexDetailProcSys_FreeWork(PokedexDetailProcSys *sys) {
    GFL_HeapFree(sys->work);
    sys->work = NULL;
}
