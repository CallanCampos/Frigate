// The battle facilities' events: picking the Pokémon to enter, and a Trainer's message in a balloon. The name is a
// guess after fld_btl_inst_tool.c, which follows it
#include "types.h"
#include "app/p_status.h"
#include "app/pokelist.h"
#include "battle/regulation.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_event.h"
#include "field/field_msgbg.h"
#include "field/fld_btl_inst_event.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/poke_party.h"
#include "save/pokedex_savedata.h"
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
    ActorMsgWin *msgWin;
    StrBuf *strbuf;
    u32 unk18;
} BtlInstTrainerMsgWork;

static GameEventReturnCode EventBtlInstPokeSelect_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventBtlInstTrainerMsg_Callback(GameEvent *event, u32 *state, void *data);

GameEvent *EventBtlInstPokeSelect_Create(GameSystem *gsys, u32 a1, u32 mode, u32 regulationId, PokeParty *party,
                                         u8 *picked, u32 *choice, u32 *result, PokeParty *entered) {
    Field *field = GSYS_GetField(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBtlInstPokeSelect_Callback, sizeof(BtlInstPokeSelectWork));
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
    PokeRegulation_Load(regulationId, &work->regulation);
    partyParam->regulation = &work->regulation;
    partyParam->unk48 = a1;
    summaryParam = &work->summaryParam;
    gameData = GSYS_GetGameData(gsys);
    pokedex = GameData_GetPokedex(gameData);
    MI_CpuFill8(summaryParam, 0, sizeof(PStatusParam));
    summaryParam->party = party;
    summaryParam->dataType = PSTATUS_DATA_PARTY;
    summaryParam->partyCount = PokeParty_GetPkmCount(party);
    summaryParam->mode = PSTATUS_MODE_NORMAL;
    summaryParam->page = PSTATUS_PAGE_INFO;
    summaryParam->gameData = gameData;
    summaryParam->isNationalDex = PokeDex_IsNationalObtained(pokedex);
    return event;
}

static GameEventReturnCode EventBtlInstPokeSelect_Callback(GameEvent *event, u32 *state, void *data) {
    BtlInstPokeSelectWork *work = data;
    GameSystem *gsys = work->gsys;
    int i, count;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(
            event, CallFieldMapEntranceOutTransitionDefault(gsys, work->field, FIELD_TRANSITION_FADE_BLACK, 0));
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
        GameEvent_ChainNext(
            event, CallFieldMapEntranceInTransition(gsys, work->field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        MI_CpuCopy8(work->partyParam.picked, work->picked, sizeof(work->partyParam.picked));
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

GameEvent *EventBtlInstTrainerMsg_Create(GameSystem *gsys, BSubwayTrainer *trainers, u32 index, u16 actorId) {
    GameEvent *event;
    GameData *gameData = GSYS_GetGameData(gsys);
    FieldMsgBG *msgBGSys = Field_GetMsgBGSys(GSYS_GetField(gsys));
    BtlInstTrainerMsgWork *work;

    event = GameEvent_Create(gsys, NULL, EventBtlInstTrainerMsg_Callback, sizeof(BtlInstTrainerMsgWork));
    work = GameEvent_GetData(event);
    work->gsys = gsys;
    if (trainers->message.type == 0xffff) {
        MsgData *msgData;

        work->strbuf = GFL_StrBufCreate(0x5c, HEAPID_GAMEEVENT);
        msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BATTLE_SUBWAY_TRAINER_MESSAGES, HEAPID_GAMEEVENT);
        GFL_MsgDataLoadStrbuf(msgData, trainers->message.id, work->strbuf);
        GFL_MsgDataFree(msgData);
    } else {
        work->strbuf = PMSData_ToString(&trainers[index].message, HEAPID_GAMEEVENT);
    }
    CopyActorWPos(FindFieldActor(GameData_GetMMSys(gameData), actorId), &work->pos);
    work->msgWin = ActorMsgWin_CheckAndCreate(msgBGSys, 1, &work->pos, work->strbuf, 0, 0);
    return event;
}

static GameEventReturnCode EventBtlInstTrainerMsg_Callback(GameEvent *event, u32 *state, void *data) {
    BtlInstTrainerMsgWork *work = data;

    switch (*state) {
    case 0:
        if (MsgBGPrinter_TalkWinMain(work->msgWin) == TRUE) {
            (*state)++;
        }
        break;
    case 1:
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            MsgBGPrinter_DeleteTalkWin(work->msgWin);
            (*state)++;
        }
        break;
    case 2:
        if (MsgBGPrinter_WaitTalkWinClose(work->msgWin) == TRUE) {
            GFL_StrBufFree(work->strbuf);
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}
