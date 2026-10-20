#include "types.h"
#include "constants/species.h"
#include "gfl/backup_system.h"
#include "gfl/heap.h"
#include "save/cygnus_save_control.h"
#include "save/dream_world.h"
#include "save/save_control.h"

// The data that Pokémon Dream Radar ("Cygnus") writes to the card outside the save: reading it, marking it read and
// erasing it, and the Pokémon that its words name. The file's name is the ROM's string; ReadDreamRadarSaveData and
// IsDreamRadarSaveValid are swan's names.

#define CYGNUS_DATA_CARD_OFFSET 0x7f000

typedef struct {
    // A word of the data
    u32 word;
    // Its bit in the Pokémon received
    u32 bit;
    u16 species;
    u8 form;
    u8 sex;
} CygnusPokemon;

static BOOL IsDreamRadarSaveValid(void *buffer);
static void updateCardData(SaveControl *save);

// clang-format off
static const CygnusPokemon sPokemon[] = {
    { 0x80808080, 0x01, SPECIES_TORNADUS, 1, 0 },
    { 0x92567284, 0x02, SPECIES_THUNDURUS, 1, 0 },
    { 0x87643567, 0x04, SPECIES_LANDORUS, 1, 0 },
    { 0x96436763, 0x08, SPECIES_DIALGA, 0, 0 },
    { 0x43867368, 0x10, SPECIES_PALKIA, 0, 0 },
    { 0x17693572, 0x20, SPECIES_GIRATINA, 0, 0 },
    { 0x44798367, 0x40, SPECIES_HO_OH, 0, 0 },
    { 0x96636983, 0x80, SPECIES_LUGIA, 0, 0 },
};

// clang-format on

static CygnusSaveData sCardData;

// The buffer is one word larger than the data, and nothing uses it: the data is read into sCardData
void *DreamRadar_AllocBuffer(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(CygnusSaveData) + 4, TRUE, "cygnus_save_control.c", 41);
}

void DreamRadar_FreeBuffer(void *buffer) {
    GFL_HeapFree(buffer);
}

void ReadDreamRadarSaveData(void *buffer) {
    BackupSystem_Read(CYGNUS_DATA_CARD_OFFSET, &sCardData, sizeof(CygnusSaveData));
}

void DreamRadar_WriteSaveData(void) {
    BackupSystem_WriteWait(CYGNUS_DATA_CARD_OFFSET, &sCardData, sizeof(CygnusSaveData));
}

// Erases the card's data
void DreamRadar_EraseSaveData(HeapID heapId) {
    CygnusSaveData *data = GFL_HeapAllocate(heapId, sizeof(CygnusSaveData), TRUE, "cygnus_save_control.c", 117);

    BackupSystem_WriteWait(CYGNUS_DATA_CARD_OFFSET, data, sizeof(CygnusSaveData));
    GFL_HeapFree(data);
}

void *DreamRadar_GetSaveData(void *buffer) {
    return &sCardData;
}

static BOOL IsDreamRadarSaveValid(void *buffer) {
    if (ValidateDreamRadarSave(&sCardData) != CYGNUS_DATA_INVALID) {
        return TRUE;
    }
    return FALSE;
}

BOOL DreamRadar_LoadAndCheckSaveData(HeapID heapId) {
    void *buffer = DreamRadar_AllocBuffer(heapId);
    BOOL valid;

    ReadDreamRadarSaveData(buffer);
    valid = IsDreamRadarSaveValid(buffer);
    DreamRadar_FreeBuffer(buffer);
    return valid;
}

static void updateCardData(SaveControl *save) {
    DreamRadarSave *radar = GetDreamRadarSaveBlock(save);
    u32 received = GetDreamRadarFlag(radar, DREAM_RADAR_FLAG_RECEIVED);

    CygnusSaveData_Store(&sCardData, GetDreamRadarFlag(radar, DREAM_RADAR_FLAG_SEED), received);
}

void DreamRadar_MarkRead(SaveControl *save) {
    updateCardData(save);
    DreamRadar_WriteSaveData();
}

BOOL DreamRadar_IsPokemonWord(u32 word) {
    u32 i;

    for (i = 0; i < NELEMS(sPokemon); i++) {
        if (word == sPokemon[i].word) {
            return TRUE;
        }
    }
    return FALSE;
}

void DreamRadar_GetPokemonByWord(u32 word, u16 *species, u8 *form, u8 *sex) {
    u32 i;

    for (i = 0; i < NELEMS(sPokemon); i++) {
        if (word == sPokemon[i].word) {
            if (species != NULL) {
                *species = sPokemon[i].species;
            }
            if (form != NULL) {
                *form = sPokemon[i].form;
            }
            if (sex != NULL) {
                *sex = sPokemon[i].sex;
            }
            return;
        }
    }
}

u32 DreamRadar_GetPokemonBit(u32 word) {
    u32 i;

    for (i = 0; i < NELEMS(sPokemon); i++) {
        if (word == sPokemon[i].word) {
            return sPokemon[i].bit;
        }
    }
    return 0;
}
