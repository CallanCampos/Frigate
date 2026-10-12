#include "types.h"
#include "save/cygnus_save_control.h"
#include "constants/species.h"
#include "gfl/backup_system.h"
#include "gfl/heap.h"
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

static CygnusSaveData sCardData;

// The buffer is one word larger than the data, and nothing uses it: the data is read into sCardData
void *func_02011510(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(CygnusSaveData) + 4, TRUE, "cygnus_save_control.c", 41);
}

void func_02011528(void *buffer) {
    GFL_HeapFree(buffer);
}

void ReadDreamRadarSaveData(void *buffer) {
    func_0203bf30(CYGNUS_DATA_CARD_OFFSET, &sCardData, sizeof(CygnusSaveData));
}

void func_02011544(void) {
    func_0203bee8(CYGNUS_DATA_CARD_OFFSET, &sCardData, sizeof(CygnusSaveData));
}

// Erases the card's data
void func_02011558(HeapID heapId) {
    CygnusSaveData *data = GFL_HeapAllocate(heapId, sizeof(CygnusSaveData), TRUE, "cygnus_save_control.c", 117);

    func_0203bee8(CYGNUS_DATA_CARD_OFFSET, data, sizeof(CygnusSaveData));
    GFL_HeapFree(data);
}

void *func_02011588(void *buffer) {
    return &sCardData;
}

static BOOL IsDreamRadarSaveValid(void *buffer) {
    if (ValidateDreamRadarSave(&sCardData) != CYGNUS_DATA_INVALID) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_020115a8(HeapID heapId) {
    void *buffer = func_02011510(heapId);
    BOOL valid;

    ReadDreamRadarSaveData(buffer);
    valid = IsDreamRadarSaveValid(buffer);
    func_02011528(buffer);
    return valid;
}

static void updateCardData(SaveControl *save) {
    DreamRadarSave *radar = GetDreamRadarSaveBlock(save);
    u32 received = GetDreamRadarFlag(radar, DREAM_RADAR_FLAG_RECEIVED);

    func_020113c4(&sCardData, GetDreamRadarFlag(radar, DREAM_RADAR_FLAG_SEED), received);
}

void func_020115f0(SaveControl *save) {
    updateCardData(save);
    func_02011544();
}

BOOL func_020115fc(u32 word) {
    u32 i;

    for (i = 0; i < NELEMS(sPokemon); i++) {
        if (word == sPokemon[i].word) {
            return TRUE;
        }
    }
    return FALSE;
}

void func_02011624(u32 word, u16 *species, u8 *form, u8 *sex) {
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

u32 func_0201167c(u32 word) {
    u32 i;

    for (i = 0; i < NELEMS(sPokemon); i++) {
        if (word == sPokemon[i].word) {
            return sPokemon[i].bit;
        }
    }
    return 0;
}
