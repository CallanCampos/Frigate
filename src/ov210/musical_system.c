#include "types.h"
#include "app/musical/musical_system.h"
#include "constants/pokemon.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/heap.h"
#include "pml/poke_party.h"
#include "save/save_control.h"

// Overlay 210's musical_system.c, named after its string: the musical's Pokémon and the program's data

// The archives of the four built-in programs, one each (our name)
#define ARCID_MUSICAL_PROGRAM 112
// The first program that is the downloaded one, read from the save data
#define MUSICAL_PROGRAM_DOWNLOAD 4

#define MUSICAL_ITEM_NONE 0xff

BOOL MusicalSystem_CanJoin(PartyPkm *pkm) {
    BOOL result = FALSE;

    // Whether it is an egg
    if (PokeParty_GetParam(pkm, 0xaa, NULL) == TRUE) {
        return result;
    }
    if (hasPokemonChangedForm(func_0201d620(pkm)) != TRUE) {
        result = TRUE;
    }
    return result;
}

MusicalPoke *MusicalSystem_InitPokeFromPkm(PartyPkm *pkm, HeapID heapId) {
    int i;
    MusicalPoke *poke = GFL_HeapAllocate(heapId, sizeof(MusicalPoke), TRUE, "musical_system.c", 147);

    poke->owner = 4;
    poke->pkm = pkm;
    poke->points = 0;
    for (i = 0; i < 9; i++) {
        poke->equips[i].itemId = MUSICAL_ITEM_NONE;
        poke->equips[i].unk2 = 0;
        poke->unk54[i] = FALSE;
    }
    poke->species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    poke->sex = PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL);
    poke->form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    poke->rare = PokeParty_IsRare(pkm);
    poke->personality = PokeParty_GetParam(pkm, PKM_PARAM_PID, NULL);
    return poke;
}

MusicalPoke *MusicalSystem_InitPoke(u16 species, u8 sex, u8 form, u8 rare, u32 personality, HeapID heapId) {
    int i;
    MusicalPoke *poke = GFL_HeapAllocate(heapId, sizeof(MusicalPoke), TRUE, "musical_system.c", 171);

    poke->owner = 4;
    poke->pkm = NULL;
    poke->points = 0;
    for (i = 0; i < 9; i++) {
        poke->equips[i].itemId = MUSICAL_ITEM_NONE;
        poke->equips[i].unk2 = 0;
        poke->unk54[i] = FALSE;
    }
    for (i = 0; i < 4; i++) {
        poke->unk4C[i] = 0;
    }
    poke->unk78 = 0;
    poke->species = species;
    poke->sex = sex;
    poke->form = form;
    poke->rare = rare;
    poke->personality = personality;
    return poke;
}

Ov210Work *MusicalSystem_InitProgramData(HeapID heapId) {
    Ov210Work *work = GFL_HeapAllocate(heapId, sizeof(Ov210Work), FALSE, "musical_system.c", 200);

    work->unk4 = NULL;
    work->msgArc = NULL;
    work->script = NULL;
    work->soundData = NULL;
    work->sound[0] = NULL;
    work->sound[1] = NULL;
    work->sound[2] = NULL;
    return work;
}

void MusicalSystem_FreeProgramData(Ov210Work *work) {
    if (work->unk4 != NULL) {
        GFL_HeapFree(work->unk4);
    }
    if (work->msgArc != NULL) {
        GFL_HeapFree(work->msgArc);
    }
    if (work->script != NULL) {
        GFL_HeapFree(work->script);
    }
    if (work->soundData != NULL) {
        GFL_HeapFree(work->soundData);
    }
    GFL_HeapFree(work);
}

void MusicalSystem_LoadProgramData(Ov210Work *work, SaveControl *save, GameData *gameData, u8 program, HeapID heapId) {
    void *download;
    ArcTool *arc;
    u32 size0;
    u32 size1;
    u32 size2;
    u32 *soundData;

    work->program = program;
    if (program >= MUSICAL_PROGRAM_DOWNLOAD) {
        download = func_0200cca0(gameData, heapId);
        arc = GFL_ArcSysCreateMemoryHandle(func_0200ce44(download), func_0200ce48(download), HEAPID_TAIL(heapId));
    } else {
        arc = GFL_ArcSysCreateFileHandle(ARCID_MUSICAL_PROGRAM + program, HEAPID_TAIL(heapId));
    }

    work->unk4 = GFL_ArcToolReadHeapNewLZGetLen(arc, 0, FALSE, heapId, &work->size[0]);
    work->msgArc = GFL_ArcToolReadHeapNewLZGetLen(arc, 1, FALSE, heapId, &work->size[1]);
    work->script = GFL_ArcToolReadHeapNewLZGetLen(arc, 2, FALSE, heapId, &work->size[2]);

    size0 = GFL_ArcToolGetDataLength(arc, 4);
    size1 = GFL_ArcToolGetDataLength(arc, 3);
    size2 = GFL_ArcToolGetDataLength(arc, 5);
    work->soundDataSize = size0 + 12 + size1 + size2;
    soundData = GFL_HeapAllocate(heapId, work->soundDataSize, TRUE, "musical_system.c", 266);
    work->soundData = soundData;
    work->sound[0] = soundData + 3;
    work->sound[1] = (void *)((u32)work->sound[0] + size0);
    work->sound[2] = (void *)((u32)work->sound[0] + size0 + size1);
    soundData[0] = size0;
    soundData[1] = size1;
    soundData[2] = size2;
    GFL_ArcToolRead(arc, 4, work->sound[0]);
    GFL_ArcToolRead(arc, 3, work->sound[1]);
    GFL_ArcToolRead(arc, 5, work->sound[2]);
    GFL_ArcToolFree(arc);

    if (program >= MUSICAL_PROGRAM_DOWNLOAD) {
        func_0200cd10(download);
    }
}
