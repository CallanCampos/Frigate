// The Trial House's events: picking the Pokémon to enter, the battle, its statistics and the results screen. Function
// names from swan; the file's name is descriptive
#include "types.h"
#include "app/th_award.h"
#include "battle/btl_setup.h"
#include "constants/pokemon.h"
#include "field/battle_facility.h"
#include "field/event_battle.h"
#include "field/fld_btl_inst_event.h"
#include "field/trial_house.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/player_info.h"
#include "save/trial_house.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

typedef struct {
    GameSystem *gsys;
    Field *field;
    u32 unk08;
    // Set to whether Pokémon were picked
    u16 *result;
    u32 unk10;
    u32 regulationId;
    PokeParty *party;
    // The Battle Box's party, made for the screen
    PokeParty *battleBoxParty;
    u32 choice;
    u32 screenResult;
    u8 picked[6];
    TrialHouseWork *work;
} TrialHousePokeSelectData;

typedef struct {
    GameSystem *gsys;
    Field *field;
    ThAwardParam param;
} TrialHouseResultsData;

static GameEventReturnCode func_ov012_02162ccc(GameEvent *event, u32 *state, void *work);
static GameEventReturnCode func_ov012_02162f08(GameEvent *event, u32 *state, void *work);

GameEvent *func_ov012_02162c48(GameSystem *gsys, TrialHouseWork *work, u32 mode, BOOL battleBox, u16 *result) {
    GameEvent *event;
    GameData *gameData = GSYS_GetGameData(gsys);
    TrialHousePokeSelectData *data;

    event = GameEvent_Create(gsys, NULL, func_ov012_02162ccc, sizeof(TrialHousePokeSelectData));
    data = GameEvent_GetData(event);

    data->gsys = gsys;
    data->field = GSYS_GetField(gsys);
    data->result = result;
    data->work = work;
    if (mode == 20 || mode == 21) {
        if (mode == 20) {
            data->unk10 = 0;
            data->regulationId = 20;
        } else {
            data->unk10 = 1;
            data->regulationId = 21;
        }
    } else {
        data->unk10 = 0;
        data->regulationId = 20;
    }
    if (battleBox == FALSE) {
        data->battleBoxParty = NULL;
        data->party = GameData_GetParty(gameData);
    } else {
        data->party = data->battleBoxParty =
            convertBoxedPokeSetToParty(getBattleBox(GameData_GetSaveControl(gameData)), HEAPID_TRIAL_HOUSE);
    }
    return event;
}

static GameEventReturnCode func_ov012_02162ccc(GameEvent *event, u32 *state, void *work) {
    TrialHousePokeSelectData *data = work;
    GameSystem *gsys = data->gsys;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, func_ov012_02161c88(gsys, data->unk10, 0x17, data->regulationId, data->party,
                                                       data->picked, &data->choice, &data->screenResult,
                                                       data->work->party));
        *state = 1;
        break;
    case 1:
        if (data->battleBoxParty != NULL) {
            GFL_HeapFree(data->battleBoxParty);
        }
        if (data->screenResult != 0 || data->choice == 7 || data->choice == 8) {
            *data->result = FALSE;
        } else {
            *data->result = TRUE;
        }
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// The battle type of the work, which MWCC loads before the other arguments only when it goes through a variable of an
// enum type
typedef enum {
    TRIAL_HOUSE_BATTLE_SINGLE,
    TRIAL_HOUSE_BATTLE_DOUBLE,
} TrialHouseBattleType;

GameEvent *CallTrialHouseBattle(GameSystem *gsys, TrialHouseWork *work) {
    Field *field = GSYS_GetField(gsys);
    TrialHouseBattleType type = work->battleType;
    BtlSetup *setup = SetupTrialHouseBattle(gsys, work->party, type, &work->trainer, NULL, work->capacity);

    return CreateTrialHouseBattleEvent(gsys, field, setup);
}

void SyncTrialHouseWkStatsFromBattle(TrialHouseWork *work, BtlSetup *setup) {
    PokeParty *party;
    int count;
    int totalHp;
    int i;

    work->stats[0] += (u16)(setup->unkD2 + 1);
    work->stats[1] += setup->unkD3;
    work->stats[2] += setup->unkD4;
    work->stats[3] += setup->unkD5;
    work->stats[4] += setup->unkD6;
    work->stats[5] += setup->unkD7;
    work->stats[6] += setup->unkD8;
    work->stats[7] += setup->unkD9;
    work->stats[8] += setup->unkDA;
    work->stats[9] += setup->unkDB;
    work->stats[11] += setup->unkDC;
    party = setup->party[0];
    count = PokeParty_GetPkmCount(party);
    totalHp = 0;
    for (i = 0; i < count; i++) {
        totalHp += PokeParty_GetParam(PokeParty_GetPkm(party, i), PKM_PARAM_MAX_HP, NULL);
    }
    work->stats[10] += (u16)(setup->unkD0 * 100 / totalHp);
}

GameEvent *func_ov012_02162eb4(GameSystem *gsys, u32 a1, u32 a2) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov012_02162f08, sizeof(TrialHouseResultsData));
    TrialHouseResultsData *data = GameEvent_GetData(event);

    data->gsys = gsys;
    data->field = GSYS_GetField(gsys);
    data->param.gender = getTrainerGender(GetGameDataPlayerInfo(gameData));
    data->param.save = func_0200f1b8(GameData_GetSaveControl(gameData));
    data->param.downloaded = a1;
    data->param.playJingle = a2;
    return event;
}

static GameEventReturnCode func_ov012_02162f08(GameEvent *event, u32 *state, void *work) {
    TrialHouseResultsData *data = GameEvent_GetData(event);

    switch (*state) {
    case 0:
        GSYS_QueueProcAsEvent(event, OVERLAY_TH_AWARD, &TH_AWARD_PROC_FUNCTIONS, &data->param);
        (*state)++;
        break;
    case 1:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
