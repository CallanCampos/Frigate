#include "types.h"
#include "app/name_entry.h"
#include "constants/pokemon.h"
#include "field/battle_facility.h"
#include "field/field_effect.h"
#include "field/fld_btl_inst_event.h"
#include "field/trial_house.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "gfl/net.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/trial_house.h"
#include "struct_decls.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/version.h"

struct TrialHouseWork *CreateTrialHouseWk(GameSystem *gsys) {
    u32 saveSize = BattleExamination_GetSize();
    GameData *gameData = GSYS_GetGameData(gsys);
    struct TrialHouseWork *work;

    GameData_GetSaveControl(gameData);
    work = GFL_HeapAllocate(HEAPID_TRIAL_HOUSE, sizeof(struct TrialHouseWork), TRUE, "trial_house.c", 0x5d);
    work->heapId = HEAPID_TRIAL_HOUSE;
    work->saveBuffer = GFL_HeapAllocate(HEAPID_TRIAL_HOUSE, saveSize, TRUE, "trial_house.c", 0x5f);
    func_ov033_0217acd4(gsys, work);
    work->party = PokeParty_Create(work->heapId);
    return work;
}

void func_ov033_0217acd4(GameSystem *gsys, TrialHouseWork *work) {
    u32 state;
    SaveControl *save;
    void *buffer;
    void *extraSave;

    save = GameData_GetSaveControl(GSYS_GetGameData(gsys));
    buffer = GFL_HeapAllocate(0x8004, 0x800, TRUE, "trial_house.c", 0x79);
    func_02007560(save, 5, 0x8004, buffer, 0x800);
    extraSave = getAddressOfExtraSaveBlk(save, 5, 0);
    state = 0;
    if (BattleExamination_IsValid(extraSave)) {
        if (BattleExamination_IsSingle(extraSave)) {
            state = 1;
        } else {
            state = 2;
        }
    }
    freeIntermediateSaveExtraBlksAfterLoad2(save, 5);
    GFL_HeapFree(buffer);
    work->initState = state;
}

void TrialHouseWorkDelete(void *unused, struct TrialHouseWork **workPtr) {
    if (*workPtr != NULL) {
        GFL_HeapFree((*workPtr)->saveBuffer);
        GFL_HeapFree((*workPtr)->party);
        GFL_HeapFree(*workPtr);
        *workPtr = NULL;
    }
}

void func_ov033_0217ad78(TrialHouseWork *work, u32 mode) {
    u32 battleType;

    if (work != NULL) {
        if (mode <= 1) {
            if (mode == 0) {
                battleType = 0;
                work->capacity = 3;
            } else {
                battleType = 1;
                work->capacity = 4;
            }
            work->battleType = battleType;
        } else {
            work->battleType = 0;
            work->capacity = 3;
        }
        PokeParty_InitCore(work->party, work->capacity);
    }
}

void func_ov033_0217adbc(TrialHouseWork *work, u32 selectionFlag) {
    work->selectionFlag = selectionFlag;
}

u32 func_ov033_0217adc4(GameSystem *gsys, TrialHouseWork *work, u32 mode) {
    if (work->selectionFlag != 0) {
        func_ov033_0217ae5c(gsys, work, mode);
    } else {
        func_ov033_0217ade8(work, mode);
    }
    return func_ov033_0217aed0(work);
}

void func_ov033_0217ade8(TrialHouseWork *work, u32 mode) {
    u32 base;
    u32 range;
    u32 value;

    switch (mode) {
    case 0:
        base = 0x32;
        range = 0x14;
        break;
    case 1:
        base = 0x6e;
        range = 0x32;
        break;
    case 2:
        base = 0xa0;
        range = 0x14;
        break;
    case 3:
        base = 0xf0;
        range = 0x1e;
        break;
    case 4:
        base = 0x10e;
        range = 0x1e;
        break;
    default:
        base = 0x32;
        range = 0x14;
        break;
    }
    value = base + GFL_RandomLC(range);
    func_ov012_02162864(&work->trainer, value, work->capacity, NULL, NULL, NULL, HEAPID_TAIL(work->heapId));
}

void func_ov033_0217ae5c(GameSystem *gsys, TrialHouseWork *work, u32 mode) {
    SaveControl *save;
    void *buffer;
    void *extra;

    save = GameData_GetSaveControl(GSYS_GetGameData(gsys));
    buffer = GFL_HeapAllocate(0x8004, 0x800, TRUE, "trial_house.c", 0x14c);
    if (func_02007560(save, 5, 0x8004, buffer, 0x800) == 1) {
        extra = getAddressOfExtraSaveBlk(save, 5, 0);
        work->trainer = *BattleExamination_GetTrainer(extra, mode);
    }
    freeIntermediateSaveExtraBlksAfterLoad2(save, 5);
    GFL_HeapFree(buffer);
}

u32 func_ov033_0217aed0(TrialHouseWork *work) {
    return func_ov012_02162b38(work->trainer.trainerId);
}

GameEvent *func_ov033_0217aedc(GameSystem *gsys, TrialHouseWork *work, u32 index, u32 actorId) {
    return func_ov012_02161e6c(gsys, &work->trainer, index, (u16)actorId);
}

GameEvent *func_ov033_0217aee8(GameSystem *gsys, TrialHouseWork *work, u16 *result) {
    GameEvent *event;
    struct TrialHouseEventData *data;

    event = GameEvent_Create(gsys, NULL, func_ov033_0217af5c, sizeof(struct TrialHouseEventData));
    data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->result = result;
    data->work = work;
    data->timeout = 0;
    data->init.code = 0x2e;
    data->init.data[0].datasize = BattleExamination_GetSize();
    data->init.data[0].pData = work->saveBuffer;
    data->init.data[0].region = region;
#ifdef BLACK2
    data->init.data[0].mask = 0x800000;
#else
    data->init.data[0].mask = 0x400000;
#endif
    data->init.dataNum = 1;
    data->init.flag4 = 0;
    data->init.heapId = HEAPID_TAIL(HEAPID_FIELDMAP);
    return event;
}

GameEventReturnCode func_ov033_0217af5c(GameEvent *event, u32 *state, void *arg) {
    TrialHouseEventData *data;
    GameSystem *gsys;
    GameData *gameData;
    TrialHouseSave *save;
    void *saveBuffer;
    u32 a;
    u32 b;
    u32 c;

    data = arg;
    gsys = data->gsys;
    switch (*state) {
    case 0:
        data->subwork = func_ov012_02152990(&data->init);
        if (func_ov012_02152b64(data->subwork) == 0) {
            *data->result = 0;
            *state = 4;
        } else {
            *state = 1;
        }
        break;
    case 1:
        if (GCTX_HIDGetPressedKeys() == 2) {
            *data->result = 3;
            *state = 4;
            break;
        }
        func_ov012_02152bec(data->subwork);
        data->timeout++;
        if (data->timeout > 120) {
            *state = 2;
        }
        break;
    case 2:
        if (GCTX_HIDGetPressedKeys() == 2) {
            *data->result = 3;
            *state = 4;
            break;
        }
        func_ov012_02152bec(data->subwork);
        if (func_ov012_02152bb4(data->subwork)) {
            *state = 3;
        } else {
            *data->result = 0;
            *state = 4;
        }
        break;
    case 3:
        if (GCTX_HIDGetPressedKeys() == 2) {
            *data->result = 3;
            *state = 4;
            break;
        }
        func_ov012_02152bec(data->subwork);
        if (!func_ov012_02152bd4(data->subwork)) {
            break;
        }
        gameData = GSYS_GetGameData(gsys);
        save = func_0200f1b8(GameData_GetSaveControl(gameData));
        saveBuffer = data->work->saveBuffer;
        a = BattleExamination_GetNumber(saveBuffer);
        b = BattleExamination_IsValid(saveBuffer);
        c = func_ov033_0217b35c(save, a);
        if (b != 0 && (c == 0 || a == 0)) {
            func_ov033_0217b384(save, a);
            BattleExamination_Load(gameData, data->work->saveBuffer, 0x8004);
            *data->result = 1;
        } else if (c != 0) {
            *data->result = 2;
        } else {
            *data->result = 0;
        }
        *state = 4;
        break;
    case 4:
        func_ov012_02152bfc(data->subwork);
        *state = 5;
        break;
    case 5:
        if (func_02042ab8()) {
            *state = 6;
        }
        break;
    case 6:
        func_ov033_0217acd4(gsys, data->work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

void TrialHouseCalcPointScore(GameSystem *gsys, TrialHouseWork *work, u16 *rankOut, u16 *pointsOut) {
    u16 *stats = work->stats;
    s32 total;
    SaveControl *save;
    u32 points;
    TrialHouseSave *thSave;
    u32 rank;
    TrialHouseRecord *record;
    s32 i;
    PartyPkm *pkm;
    TrialHousePokemon *pokemon;
    SaveControl *extraSaveControl;
    void *buffer;
    GameRecords *records;

    total = 0;
    total += stats[7] * 1000;
    total += stats[8] * 80;
    total += stats[1] * 5;
    total += stats[3];
    total += stats[5] * 5;
    total += stats[6] * 2;
    total += stats[11] * 15;
    total -= stats[0] * 10;
    total -= stats[2] * 10;
    total -= stats[4] * 2;
    total -= stats[9] * 80;
    total -= 500 - stats[10];
    if (total < 0) {
        total = 0;
    } else if (total > 9999) {
        total = 9999;
    }
    points = total;
    if (points >= 6000) {
        rank = 6;
    } else if (points >= 5000) {
        rank = 5;
    } else if (points >= 4000) {
        rank = 4;
    } else if (points >= 3000) {
        rank = 3;
    } else if (points >= 2000) {
        rank = 2;
    } else if (points >= 1000) {
        rank = 1;
    } else {
        rank = 0;
    }
    *rankOut = rank;
    *pointsOut = points;
    save = GameData_GetSaveControl(GSYS_GetGameData(gsys));
    thSave = func_0200f1b8(save);
    record = &thSave->records[(u8)(work->initState != 0 ? 1 : 0)];
    record->valid = TRUE;
    if (work->capacity == 3) {
        record->isDouble = FALSE;
    } else if (work->capacity == 4) {
        record->isDouble = TRUE;
    } else {
        record->isDouble = FALSE;
    }
    record->points = points;
    sys_memset(record->pokemon, 0, sizeof(record->pokemon));
    for (i = 0; i < (s32)work->capacity; i++) {
        pokemon = &record->pokemon[i];
        pkm = PokeParty_GetPkm(work->party, i);
        pokemon->species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        pokemon->form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
        pokemon->sex = PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL);
    }
    if (work->initState != 0) {
        extraSaveControl = GameData_GetSaveControl(GSYS_GetGameData(gsys));
        buffer = GFL_HeapAllocate(0x8004, 0x800, TRUE, "trial_house.c", 0x277);
        if (func_02007560(extraSaveControl, 5, 0x8004, buffer, 0x800) == 1) {
            sys_memcpy((u8 *)getAddressOfExtraSaveBlk(extraSaveControl, 5, 0) + 0x5a0, thSave->unk38, sizeof(thSave->unk38));
        }
        GFL_HeapFree(buffer);
        freeIntermediateSaveExtraBlksAfterLoad2(extraSaveControl, 5);
    }
    records = getTrainerCardInfoBlkAddress(save);
    func_02009638(records, points);
    func_02009618(records, rank);
}

u32 func_ov033_0217b2e4(GameSystem *gsys, TrialHouseWork *work) {
    return work->initState;
}

GameEvent *func_ov033_0217b2ec(GameSystem *gsys, TrialHouseWork *work, u32 mode) {
    GameEvent *event;
    TrialHouseEffectEvent *data;

    event = GameEvent_Create(gsys, NULL, func_ov033_0217b3ac, sizeof(TrialHouseEffectEvent));
    data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->buffer = NULL;
    data->mode = mode;
    data->effect = func_ov036_021c6cc8(0x8015, GSYS_GetField(gsys));
    func_ov036_021c6d14(data->effect);
    return event;
}

u32 func_ov033_0217b32c(GameSystem *gsys) {
    TrialHouseSave *save;

    save = func_0200f1b8(GameData_GetSaveControl(GSYS_GetGameData(gsys)));
    if (save->records[0].valid) {
        if (save->records[1].valid) {
            return 3;
        }
        return 1;
    }
    if (save->records[1].valid) {
        return 2;
    }
    return 0;
}

u8 func_ov033_0217b35c(TrialHouseSave *save, u32 index) {
    u8 byte;
    u8 bit;

    if (index < 128) {
        byte = index / 8;
        bit = index % 8;
        return (save->bits[byte] >> bit) & 1;
    }
    return TRUE;
}

void func_ov033_0217b384(TrialHouseSave *save, u32 index) {
    u8 byte;
    u8 bit;

    if (index < 128) {
        byte = index / 8;
        bit = index % 8;
        save->bits[byte] |= 1 << bit;
    }
}

GameEventReturnCode func_ov033_0217b3ac(GameEvent *event, u32 *state, void *arg) {
    TrialHouseEffectEvent *data;
    GameData *gameData;
    SaveControl *save;
    void *extraSave;

    data = arg;
    gameData = GSYS_GetGameData(data->gsys);
    save = GameData_GetSaveControl(gameData);
    switch (*state) {
    case 0:
        data->buffer = GFL_HeapAllocate(0x8004, 0x800, FALSE, "trial_house.c", 0x34c);
        func_02007560(save, 5, 0x8004, data->buffer, 0x800);
        extraSave = getAddressOfExtraSaveBlk(save, 5, 0);
        if (data->mode == 1) {
            BattleExamination_SetUsed(extraSave);
        } else {
            sys_memset(extraSave, 0, BattleExamination_GetSize());
        }
        func_020178c4(gameData, 5);
        (*state)++;
        break;
    case 1:
        if (func_020178f4(gameData, 5) == 2) {
            freeIntermediateSaveExtraBlksAfterLoad2(save, 5);
            GFL_HeapFree(data->buffer);
            func_ov036_021c6d3c(data->effect);
            func_ov036_021c6cf8(data->effect);
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}
