#include "types.h"
#include "battle/btl_setup.h"
#include "constants/pokemon.h"
#include "field/battle_facility.h"
#include "field/bsubway_scr.h"
#include "field/player_state.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/bsubway_save.h"
#include "save/player_info.h"
#include "save/records.h"
#include "save/save_control.h"
#include "struct_decls.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/version.h"

// Battle Subway trainer selection and team construction. The file name is descriptive; the ROM gives none here.

static inline u16 BSubwayScr_PickTrainerInRanges(BSubwayScrWork *bsw, const BSubwayTrainerRange *ranges, u16 level);

BtlSetup *func_ov033_0217c094(BSubwayScrWork *bsw, GameSystem *gsys) {
    PokeParty *party;
    PokeParty *source;
    PartyPkm *pkm;
    BtlSetup *result;
    s32 i;

    party = PokeParty_Create(0x8004);
    source = func_ov033_0217bd60(bsw);
    PokeParty_InitCore(party, bsw->memberCount);
    for (i = 0; i < bsw->memberCount; i++) {
        pkm = PokeParty_GetPkm(source, bsw->memberSlots[i]);
        PokeParty_AddPkm(party, pkm);
    }
    result =
        SetupTrialHouseBattle(gsys, party, bsw->playMode, bsw->trainers, &bsw->unk2C8[bsw->unkC_5], bsw->memberCount);
    GFL_HeapFree(party);
    return result;
}

void *func_ov033_0217c110(BSubwayScrWork *bsw) {
    return bsw->unk74C;
}

// The trainers of the super trains from the fifth streak on
static const BSubwayTrainerRange sSuperTopTrainerRange[1] = {
    { 200, 299 },
};

// The trainers of a normal train's last car, by streak
static const BSubwayTrainerRange sNormalLastTrainerRanges[2] = {
    { 50, 69 },
    { 70, 89 },
};

// The trainers of a normal train's first six cars, by streak
static const BSubwayTrainerRange sNormalTrainerRanges[3] = {
    { 0, 49 },
    { 30, 69 },
    { 50, 109 },
};

// The trainers of a super train's first six cars, by streak
static const BSubwayTrainerRange sSuperTrainerRanges[4] = {
    { 110, 159 },
    { 140, 179 },
    { 160, 199 },
    { 180, 219 },
};

// The trainers of a super train's last car, by streak
static const BSubwayTrainerRange sSuperLastTrainerRanges[4] = {
    { 160, 179 },
    { 180, 199 },
    { 200, 219 },
    { 220, 239 },
};

static inline u16 BSubwayScr_PickTrainerInRanges(BSubwayScrWork *bsw, const BSubwayTrainerRange *ranges, u16 level) {
    const BSubwayTrainerRange *range = ranges + level;
    u16 count = range->max - range->min + 1;

    return range->min + randFFFFFFFFdivFFFF(bsw) % count;
}

u16 func_ov033_0217c11c(BSubwayScrWork *bsw, u16 level, u8 index, u32 mode, u8 side) {
    BOOL super;
    s32 boss;
    u16 trainerId;

    trainerId = 0;
    super = FALSE;
    boss = -1;
    switch (mode) {
    case 5:
    case 6:
    case 7:
    case 8:
        super = TRUE;
        break;
    }
    if (index == 6) {
        if (!super) {
            if (level == 2) {
                boss = 0;
            }
        } else if (level == 6) {
            boss = 1;
        }
    }
    if (boss != -1) {
        switch (mode) {
        case 0:
        case 5:
            if (boss == 0) {
                trainerId = 0x132;
            } else if (boss == 1) {
                trainerId = 0x133;
            }
            break;
        case 1:
        case 6:
            if (boss == 0) {
                trainerId = 0x134;
            } else if (boss == 1) {
                trainerId = 0x135;
            }
            break;
        case 2:
        case 3:
        case 7:
        case 8:
            if (boss == 0) {
                if (side == 0) {
                    trainerId = 0x137;
                } else {
                    trainerId = 0x136;
                }
            } else if (boss == 1) {
                if (side == 0) {
                    trainerId = 0x139;
                } else {
                    trainerId = 0x138;
                }
            }
            break;
        }
    } else if (!super) {
        if (level < 3) {
            if (index < 6) {
                trainerId = BSubwayScr_PickTrainerInRanges(bsw, sNormalTrainerRanges, level);
            } else if (level < 2) {
                trainerId = BSubwayScr_PickTrainerInRanges(bsw, sNormalLastTrainerRanges, level);
            }
        }
    } else if (level < 4) {
        if (index < 6) {
            trainerId = BSubwayScr_PickTrainerInRanges(bsw, sSuperTrainerRanges, level);
        } else {
            trainerId = BSubwayScr_PickTrainerInRanges(bsw, sSuperLastTrainerRanges, level);
        }
    } else {
        trainerId = sSuperTopTrainerRange[0].min +
                    randFFFFFFFFdivFFFF(bsw) % (sSuperTopTrainerRange[0].max - sSuperTopTrainerRange[0].min + 1);
    }
    return trainerId;
}

BOOL func_ov033_0217c264(BSubwayScrWork *bsw, BSubwayTrainer *trainer, u16 trainerId, u32 count, const u16 *species,
                         const u16 *items, BSubwayTeamConfig *config, HeapID heapId) {
    return BtlInst_MakeRomTrainerAndTeam(trainer, trainerId, count, species, items, config, heapId);
}

u8 func_ov033_0217c288(u32 value) {
    if (value < 100) {
        return 3;
    }
    if (value < 120) {
        return 6;
    }
    if (value < 140) {
        return 9;
    }
    if (value < 160) {
        return 12;
    }
    if (value < 180) {
        return 15;
    }
    if (value < 200) {
        return 18;
    }
    if (value < 220) {
        return 21;
    }
    return 31;
}

void func_ov033_0217c2c4(BSubwayScrWork *bsw, BSubwayTrainer *trainer, u16 trainerId, u32 count,
                         const BSubwayTeamConfig *config, HeapID heapId) {
    u8 iv;
    void *temp;
    s32 i;

    temp = BtlInst_MakeRomTrainer(trainer, 0xd4, trainerId, 15, heapId);
    iv = func_ov033_0217c288(trainerId);
    for (i = 0; i < 2; i++) {
        BtlInst_MakeRomPokemon(&trainer->pokemon[i], 0xd3, config->files[i], config->id, config->pids[i], iv, i, count,
                               heapId);
    }
    GFL_HeapFree(temp);
}

u16 randFFFFFFFFdivFFFF(BSubwayScrWork *bsw) {
    return GFL_RandomLC(0xffffffff) / 0xffff;
}
