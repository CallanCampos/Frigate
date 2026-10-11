#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/text_banks.h"
#include "demo/shinka_demo.h"
#include "field/event_field_trade.h"
#include "field/field_event.h"
#include "field/fld_trade.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/str.h"
#include "pml/evolution.h"
#include "pml/met_data.h"
#include "pml/poke_party.h"
#include "save/player_info.h"
#include "save/pokedex.h"
#include "struct_decls.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

FieldTradeInput *FieldTradeInput_Create(u32 heapId, u32 offerIndex) {
    u16 name[0x80];
    FieldTradeInput *input;
    StrBuf *nameBuf;

    input = GFL_HeapAllocate(heapId, sizeof(FieldTradeInput), TRUE, "fld_trade.c", 0x5f);
    input->heapId = heapId;
    input->offerIndex = offerIndex;
    input->offerData = GFL_ArcSysReadHeapNewRange(0xa3, offerIndex, heapId, 0, sizeof(FieldTradeOfferData));
    input->tradeData = GFL_HeapAllocate(heapId, sizeof(PartyPkm), FALSE, "fld_trade.c", 0x67);
    input->trainer = func_02008b0c(heapId);
    func_02008b40(input->trainer);
    nameBuf = FieldTradeInput_LoadName(heapId, input->offerData->nameMessageId);
    GFL_StrBufStoreString(nameBuf, name, 0x80);
    GFL_StrBufFree(nameBuf);
    copyTrainerName(input->trainer, name);
    setTrainerGender(input->trainer, input->offerData->trainerGender);
    return input;
}

void FieldTradeInput_Free(FieldTradeInput *input) {
    GFL_HeapFree(input->offerData);
    GFL_HeapFree(input->tradeData);
    GFL_HeapFree(input->trainer);
    GFL_HeapFree(input);
}

u32 FieldTradeInput_GetSpecies(FieldTradeInput *input) {
    return input->offerData->species;
}

u32 FieldTradeInput_GetWantedSpecies(FieldTradeInput *input) {
    return input->offerData->wantedSpecies;
}

u32 FieldTradeInput_GetWantedSex(FieldTradeInput *input) {
    return input->offerData->wantedSex;
}

StrBuf *FieldTradeInput_LoadName(u32 heapId, u32 messageId) {
    MsgData *msgData;
    StrBuf *name;

    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TRADE_NAMES, heapId);
    name = GFL_MsgDataLoadStrbufNew(msgData, messageId);
    GFL_MsgDataFree(msgData);
    return name;
}

void EventFieldTrade_CreatePkm(GameData *gameData, HeapID heapId, PartyPkm *pkm, const FieldTradeOfferData *offer,
                               u32 offerIndex) {
    u32 sexMode = offer->sex;
    u32 pid;
    StrBuf *name;

    if (sexMode == 0xff) {
        sexMode = 2;
    }
    pid = PML_GenPID(offer->trainerId, (u16)offer->species, (u16)offer->form, sexMode, offer->abilitySlot, 0);
    PokeParty_CreatePkm(pkm, (u16)offer->species, (u16)offer->level, offer->trainerId, PKM_IVS_RANDOM, pid);
    PokeParty_SetParam(pkm, PKM_PARAM_FORM, offer->form);
    EventFieldTrade_DebugLogPkm(pkm);

    name = FieldTradeInput_LoadName(heapId, offer->nicknameMessageId);
    PokeParty_SetParam(pkm, PKM_PARAM_NICKNAME, (u32)name);
    GFL_StrBufFree(name);

    if (offer->ivs[0] != 0xff) {
        PokeParty_SetParam(pkm, PKM_PARAM_IV_HP, offer->ivs[0]);
    }
    if (offer->ivs[1] != 0xff) {
        PokeParty_SetParam(pkm, PKM_PARAM_IV_ATTACK, offer->ivs[1]);
    }
    if (offer->ivs[2] != 0xff) {
        PokeParty_SetParam(pkm, PKM_PARAM_IV_DEFENSE, offer->ivs[2]);
    }
    if (offer->ivs[3] != 0xff) {
        PokeParty_SetParam(pkm, PKM_PARAM_IV_SPEED, offer->ivs[3]);
    }
    if (offer->ivs[4] != 0xff) {
        PokeParty_SetParam(pkm, PKM_PARAM_IV_SP_ATTACK, offer->ivs[4]);
    }
    if (offer->ivs[5] != 0xff) {
        PokeParty_SetParam(pkm, PKM_PARAM_IV_SP_DEFENSE, offer->ivs[5]);
    }
    if (offer->abilitySlot == 2) {
        PokeParty_SetHiddenAbil(pkm, offer->species, offer->form);
    }
    if (offer->nature != 0xff) {
        PokeParty_SetParam(pkm, PKM_PARAM_NATURE, offer->nature);
    }

    PokeParty_SetParam(pkm, PKM_PARAM_CONTEST_COOL, offer->contest[0]);
    PokeParty_SetParam(pkm, PKM_PARAM_CONTEST_BEAUTY, offer->contest[1]);
    PokeParty_SetParam(pkm, PKM_PARAM_CONTEST_CUTE, offer->contest[2]);
    PokeParty_SetParam(pkm, PKM_PARAM_CONTEST_SMART, offer->contest[3]);
    PokeParty_SetParam(pkm, PKM_PARAM_CONTEST_TOUGH, offer->contest[4]);
    PokeParty_SetParam(pkm, PKM_PARAM_ITEM, offer->heldItem);

    name = FieldTradeInput_LoadName(heapId, offer->nameMessageId);
    PokeParty_SetParam(pkm, PKM_PARAM_OT_NAME, (u32)name);
    GFL_StrBufFree(name);
    PokeParty_SetParam(pkm, PKM_PARAM_OT_GENDER, offer->trainerGender);
    PokeParty_SetParam(pkm, PKM_PARAM_REGION, offer->region);
    PokeParty_SetupMetData(pkm, 1, GetGameDataPlayerInfo(gameData), 0x7532, heapId);
    PokeParty_SetParam(pkm, 9, 0x46);
    EventFieldTrade_DebugLogPkm(pkm);
    PokeParty_RecalcStats(pkm);
}

void EventFieldTrade_DebugLogPkm(PartyPkm *pkm) {
    PokeParty_GetParam(pkm, 0, NULL);
    PokeParty_GetParam(pkm, 5, NULL);
    PokeParty_GetParam(pkm, 0x6f, NULL);
    PokeParty_GetParam(pkm, 6, NULL);
    PokeParty_GetParam(pkm, 7, NULL);
    PokeParty_GetParam(pkm, 8, NULL);
    PokeParty_GetParam(pkm, 9, NULL);
    PokeParty_GetParam(pkm, 10, NULL);
    PokeParty_GetParam(pkm, 0x6e, NULL);
    PokeParty_GetParam(pkm, 0x70, NULL);
    PokeParty_GetParam(pkm, 11, NULL);
    PokeParty_GetParam(pkm, 12, NULL);
    PokeParty_GetParam(pkm, 13, NULL);
    PokeParty_GetParam(pkm, 14, NULL);
    PokeParty_GetParam(pkm, 15, NULL);
    PokeParty_GetParam(pkm, 16, NULL);
    PokeParty_GetParam(pkm, 17, NULL);
    PokeParty_GetParam(pkm, 18, NULL);
    PokeParty_GetParam(pkm, 19, NULL);
    PokeParty_GetParam(pkm, 20, NULL);
    PokeParty_GetParam(pkm, 21, NULL);
    PokeParty_GetParam(pkm, 22, NULL);
    PokeParty_GetParam(pkm, 23, NULL);
    PokeParty_GetParam(pkm, 24, NULL);
    PokeParty_GetParam(pkm, 0x46, NULL);
    PokeParty_GetParam(pkm, 0x47, NULL);
    PokeParty_GetParam(pkm, 0x48, NULL);
    PokeParty_GetParam(pkm, 0x49, NULL);
    PokeParty_GetParam(pkm, 0x4a, NULL);
    PokeParty_GetParam(pkm, 0x4b, NULL);
    PokeParty_GetParam(pkm, 0x9a, NULL);
    PokeParty_GetParam(pkm, 0x9e, NULL);
    PokeParty_GetParam(pkm, 0x95, NULL);
}

void func_ov033_0217a864(void *param) {
}

GameEventReturnCode EventFieldTrade_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldTradeWork *work;
    GameSystem *gsys;
    GameData *gameData;
    PokeParty *party;
    Field *field;
    PartyPkm *pkm;
    PokeDexSave *pokedex;
    ShinkaDemoParam *evolutionParam;
    u32 species;
    u32 method;

    work = data;
    gsys = work->gameSystem;
    gameData = work->gameData;
    party = work->party;
    field = GSYS_GetField(gsys);
    pkm = PokeParty_GetPkm(party, work->partyIndex);

    switch (*state) {
    case 0:
        work->input = FieldTradeInput_Create(HEAPID_GAMEEVENT, work->offerIndex);
        EventFieldTrade_CreatePkm(gameData, HEAPID_GAMEEVENT, work->input->tradeData, work->input->offerData,
                                  work->offerIndex);
        EventFieldTrade_DebugLogPkm(work->input->tradeData);
        func_ov033_0217a864(work->input->offerData);
        *state = 1;
        break;
    case 1:
        field = GSYS_GetField(gsys);
        work->transitionGameData = gameData;
        work->playerInfo = GetGameDataPlayerInfo(gameData);
        work->partyPkm = pkm;
        work->tradeTrainer = work->input->trainer;
        work->tradePkm = work->input->tradeData;
        GameEvent_ChainNext(event, EventFieldSubprocessTransition_Create(gsys, field, OVERLAY_ID(194),
                                                                         &data_ov194_021c63ac, work->subprocessData));
        *state = 2;
        break;
    case 2:
        copyPkmIntoPartyBlk(party, work->partyIndex, work->input->tradeData);
        pokedex = GameData_GetPokedex(gameData);
        PokeDex_RegistPkm(pokedex, work->input->tradeData);
        addPkmToDex(pokedex, work->input->tradeData);
        *state = 4;
        break;
    case 3:
        species = CheckEvolveSpecies(party, pkm, 1, 0, GameData_GetSeason(gameData), &method, HEAPID_GAMEEVENT);
        if (species != 0) {
            evolutionParam = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(ShinkaDemoParam), FALSE, "fld_trade.c", 0x1ff);
            evolutionParam->gameData = gameData;
            evolutionParam->party = party;
            evolutionParam->species = species;
            evolutionParam->partyIndex = work->partyIndex;
            evolutionParam->method = method;
            evolutionParam->unkC = 1;
            evolutionParam->canCancel = FALSE;
            work->evolutionParam = evolutionParam;
            GameEvent_ChainNext(event, EventFieldSubprocessTransition_Create(
                                           gsys, field, OVERLAY_ID(284), &SHINKA_DEMO_PROC_FUNCTIONS, evolutionParam));
        }
        *state = 4;
        break;
    case 4:
        if (work->evolutionParam != NULL) {
            GFL_HeapFree(work->evolutionParam);
        }
        FieldTradeInput_Free(work->input);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventFieldTrade_Create(GameSystem *gsys, u8 offerIndex, u8 partyIndex) {
    GameData *gameData = GSYS_GetGameData(gsys);
    PokeParty *party = GameData_GetParty(gameData);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFieldTrade_Callback, sizeof(EventFieldTradeWork));
    EventFieldTradeWork *work = GameEvent_GetData(event);

    work->gameSystem = gsys;
    work->gameData = gameData;
    work->party = party;
    work->offerIndex = offerIndex;
    work->partyIndex = partyIndex;
    work->evolutionParam = NULL;
    return event;
}
