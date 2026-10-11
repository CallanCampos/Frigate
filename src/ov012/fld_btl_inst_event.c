// The battle facilities' events: picking the Pokémon to enter, and a Trainer's message in a balloon. The name is a
// guess after fld_btl_inst_tool.c, which follows it
#include "types.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "app/p_status.h"
#include "app/pokelist.h"
#include "battle/regulation.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_event.h"
#include "field/fld_btl_inst_event.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/poke_party.h"
#include "save/pokedex.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/pms.h"

typedef struct {
    GameSystem *gsys;
    Field *field;
    PStatusParam summaryParam;
    PokeListParam partyParam;
    Regulation regulation;
    u32 *result;
    u32 *choice;
    u8 *picked;
    PokeParty *party;
    PokeParty *entered;
} BtlInstPokeSelectWork;

typedef struct {
    GameSystem *gsys;
    VecFx32 pos;
    void *msgWin;
    StrBuf *strbuf;
    u32 unk18;
} BtlInstTrainerMsgWork;

static GameEventReturnCode func_ov012_02161d54(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode func_ov012_02161f18(GameEvent *event, u32 *state, void *data);

GameEvent *func_ov012_02161c88(GameSystem *gsys, u32 a1, u32 mode, u32 regulationId, PokeParty *party, u8 *picked,
                               u32 *choice, u32 *result, PokeParty *entered) {
    Field *field = GSYS_GetField(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov012_02161d54, sizeof(BtlInstPokeSelectWork));
    BtlInstPokeSelectWork *work = GameEvent_GetData(event);
    PokeListParam *partyParam;
    PStatusParam *summaryParam;
    GameData *gameData;
    PokeDexSave *pokedex;

    work->gsys = gsys;
    work->field = field;
    work->picked = picked;
    work->choice = choice;
    work->result = result;
    work->party = party;
    work->entered = entered;
    partyParam = &work->partyParam;
    PokeListParam_Setup(partyParam, GSYS_GetGameData(gsys), mode, party);
    func_0201f744(regulationId, &work->regulation);
    partyParam->regulation = &work->regulation;
    partyParam->unk48 = a1;
    summaryParam = &work->summaryParam;
    gameData = GSYS_GetGameData(gsys);
    pokedex = GameData_GetPokedex(gameData);
    sys_memset(summaryParam, 0, sizeof(PStatusParam));
    summaryParam->party = party;
    summaryParam->dataType = PSTATUS_DATA_PARTY;
    summaryParam->partyCount = PokeParty_GetPkmCount(party);
    summaryParam->mode = PSTATUS_MODE_NORMAL;
    summaryParam->page = PSTATUS_PAGE_INFO;
    summaryParam->gameData = gameData;
    summaryParam->isNationalDex = PokeDex_IsNationalObtained(pokedex);
    return event;
}

static GameEventReturnCode func_ov012_02161d54(GameEvent *event, u32 *state, void *data) {
    BtlInstPokeSelectWork *work = data;
    GameSystem *gsys = work->gsys;
    int i, count;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, CallFieldMapEntranceOutTransitionDefault(gsys, work->field, 0, 0));
        (*state)++;
        break;
    case 1:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, work->field));
        (*state)++;
        break;
    case 2:
        GameEvent_ChainNext(event, EventPokeList_Create(gsys, work->field, &work->partyParam, &work->summaryParam));
        (*state)++;
        break;
    case 3:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case 4:
        GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(gsys, work->field, 0, 0, 1, 0, 0));
        sys_memcpy(work->partyParam.picked, work->picked, sizeof(work->partyParam.picked));
        *work->choice = work->partyParam.index;
        *work->result = work->partyParam.result;
        if (*work->choice != 7 && *work->choice != 8 && work->entered != NULL) {
            count = PokeParty_GetCapacity(work->entered);
            for (i = 0; i < count; i++) {
                int slot = work->picked[i] - 1;

                if (slot >= 6) {
                    work->picked[i] = 1;
                }
                PokeParty_AddPkm(work->entered, PokeParty_GetPkm(work->party, slot));
            }
        }
        (*state)++;
        break;
    case 5:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *func_ov012_02161e6c(GameSystem *gsys, BSubwayTrainer *trainers, u32 index, u16 actorId) {
    GameEvent *event;
    GameData *gameData = GSYS_GetGameData(gsys);
    void *msgBGSys = Field_GetMsgBGSys(GSYS_GetField(gsys));
    BtlInstTrainerMsgWork *work;

    event = GameEvent_Create(gsys, NULL, func_ov012_02161f18, sizeof(BtlInstTrainerMsgWork));
    work = GameEvent_GetData(event);
    work->gsys = gsys;
    if (trainers->message.type == 0xffff) {
        MsgData *msgData;

        work->strbuf = GFL_StrBufCreate(0x5c, HEAPID_GAMEEVENT);
        msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BATTLE_SUBWAY_TRAINER_MESSAGES, HEAPID_GAMEEVENT);
        GFL_MsgDataLoadStrbuf(msgData, trainers->message.id, work->strbuf);
        GFL_MsgDataFree(msgData);
    } else {
        work->strbuf = PMSData_ToString(&trainers[index].message, HEAPID_GAMEEVENT);
    }
    CopyActorWPos(FindFieldActor(GameData_GetMMSys(gameData), actorId), &work->pos);
    work->msgWin = ActorMsgWin_CheckAndCreate(msgBGSys, 1, &work->pos, work->strbuf, 0, 0);
    return event;
}

static GameEventReturnCode func_ov012_02161f18(GameEvent *event, u32 *state, void *data) {
    BtlInstTrainerMsgWork *work = data;

    switch (*state) {
    case 0:
        if (func_ov036_02188884(work->msgWin) == TRUE) {
            (*state)++;
        }
        break;
    case 1:
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            func_ov036_021887d4(work->msgWin);
            (*state)++;
        }
        break;
    case 2:
        if (func_ov036_021887f4(work->msgWin) == TRUE) {
            GFL_StrBufFree(work->strbuf);
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}
