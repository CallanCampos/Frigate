#include "types.h"
#include "app/bag.h"
#include "app/battle_recorder.h"
#include "app/config_panel.h"
#include "app/medalbox.h"
#include "app/p_status.h"
#include "app/pokedex.h"
#include "app/pokelist.h"
#include "app/townmap.h"
#include "app/wifi_note.h"
#include "app/xtransceiver.h"
#include "constants/items.h"
#include "constants/pokemon.h"
#include "demo/evolution_demo.h"
#include "field/app_call.h"
#include "field/event_save.h"
#include "field/field.h"
#include "field/field_event.h"
#include "field/field_sound.h"
#include "field/hidden_event.h"
#include "field/itemuse_event.h"
#include "field/mailsys.h"
#include "field/mailtool.h"
#include "field/player_action.h"
#include "field/player_state.h"
#include "field/trcard_sys.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "pml/evolution.h"
#include "pml/item.h"
#include "pml/mail.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/pokedex_savedata.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The app call's states: an app with a proc closes the field, runs and opens the next app until one gives another
// result, then the field opens again; an app with a call function (the save) runs over the field
enum {
    FIELD_APP_CALL_STATE_START,
    FIELD_APP_CALL_STATE_NEXT_APP,
    FIELD_APP_CALL_STATE_END,
    FIELD_APP_CALL_STATE_CALL,
    FIELD_APP_CALL_STATE_CALL_RESULT,
    FIELD_APP_CALL_STATE_CALL_RETRY,
    FIELD_APP_CALL_STATE_FADE_OUT,
    FIELD_APP_CALL_STATE_CLOSE_FIELD,
    FIELD_APP_CALL_STATE_OPEN_FIELD,
    FIELD_APP_CALL_STATE_SETUP_FIELD,
    FIELD_APP_CALL_STATE_FADE_IN,
    FIELD_APP_CALL_STATE_START_PROC,
    FIELD_APP_CALL_STATE_WAIT_PROC,
    FIELD_APP_CALL_STATE_PROC_RESULT,
};

// clang-format off
const FieldProcLink FIELD_PROC_LINK_LIST[FIELD_APP_MAX] = {
    { OVERLAY_ID(165), &POKELIST_PROC_FUNCTIONS, FieldAppCall_CreatePokeListParam, FieldAppCall_GetPokeListResult, NULL, FieldProcLink_FreeParam },
    { OVERLAY_ID(302), &POKEDEX_PROC_FUNCTIONS, FieldAppCall_CreatePokedexParam, FieldAppCall_GetPokedexResult, NULL, FieldProcLink_FreeParam },
    { OVERLAY_ID(142), &BAG_PROC_FUNCTIONS, FieldAppCall_CreateBagParam, FieldAppCall_GetBagResult, NULL, FieldProcLink_FreeParam },
    { OVERLAY_ID(186), &TRCARD_SYS_PROC_FUNCTIONS, FieldAppCall_CreateTrainerCardParam, FieldAppCall_GetTrainerCardResult, NULL, FieldProcLink_FreeParam },
    { 0, NULL, NULL, NULL, FieldAppCall_CallSave, FieldProcLink_FreeParam },
    { OVERLAY_ID(140), &CONFIG_PANEL_PROC_FUNCTIONS, FieldAppCall_CreateConfigParam, FieldAppCall_GetConfigResult, NULL, FieldProcLink_FreeParam },
    { 0, NULL, NULL, NULL, FieldProcLink_CallInfDungeonRetire, FieldProcLink_FreeParam },
    { OVERLAY_PSTATUS, &PSTATUS_PROC_FUNCTIONS, FieldAppCall_CreatePStatusParam, FieldAppCall_GetPStatusResult, NULL, FieldProcLink_FreeParam },
    { OVERLAY_ID(144), &TOWNMAP_PROC_FUNCTIONS, FieldAppCall_CreateTownMapParam, FieldAppCall_GetTownMapResult, NULL, FieldProcLink_FreeParam },
    { OVERLAY_ID(204), &WIFI_NOTE_PROC_FUNCTIONS, FieldAppCall_CreateWifiNoteParam, FieldAppCall_GetWifiNoteResult, NULL, FieldProcLink_FreeParam },
    { OVERLAY_NONE, &MAILSYS_PROC_FUNCTIONS, FieldAppCall_CreateMailParam, FieldAppCall_GetMailResult, NULL, FieldAppCall_FreeMailParam },
    { OVERLAY_ID(284), &EVOLUTION_DEMO_PROC_FUNCTIONS, script_evo, FieldAppCall_GetEvolutionDemoResult, NULL, FieldProcLink_FreeParam },
    { OVERLAY_ID(272), &BR_MAIN_PROC_FUNCTIONS, FieldAppCall_CreateBattleRecorderParam, FieldAppCall_GetBattleRecorderResult, NULL, FieldProcLink_FreeParam },
    { OVERLAY_ID(259), &XTRANSCEIVER_PROC_FUNCTIONS, FieldAppCall_CreateXtransceiverParam, FieldAppCall_GetXtransceiverResult, NULL, FieldProcLink_FreeParam },
    { OVERLAY_ID(145), &MEDALBOX_PROC_FUNCTIONS, FieldAppCall_CreateMedalBoxParam, FieldAppCall_GetMedalBoxResult, NULL, FieldProcLink_FreeParam },
};

// clang-format on

GameEvent *EventFieldAppCall_Create(FieldAppCallInput *input, u16 code) {
    GameEvent *event =
        GameEvent_Create(input->gameSystem, input->parent, EventFieldAppCall_Callback, sizeof(FieldAppCallWork));
    FieldAppCallWork *work = GameEvent_GetData(event);

    MI_CpuFill8(work, 0, sizeof(FieldAppCallWork));
    work->code = code;
    work->input = input;
    work->result = FIELD_APP_RESULT_NEXT;
    work->event = event;
    work->appId = input->appId;
    work->prevAppId = input->appId;
    work->nextAppId = input->appId;
    work->partySlot = 0;
    work->item = 0;
    work->subMode = 0;
    FieldAppCallParam_Init(&work->params, work->input, work->input->canRetry, work->input->callback1,
                           work->input->callback2, work->input->arg);
    PlayerActionPerms_Create(&work->perms, work->input->gameSystem, work->input->field);
    CalcPlayerActionPossibilities(work->input->field, &work->action);
    return event;
}

GameEventReturnCode EventFieldAppCall_Callback(GameEvent *event, u32 *state, void *data) {
    FieldAppCallWork *work = data;
    void *param;

    switch (*state) {
    case FIELD_APP_CALL_STATE_START:
        *state = FIELD_APP_CALL_STATE_NEXT_APP;
        break;
    case FIELD_APP_CALL_STATE_NEXT_APP:
        if (work->result == FIELD_APP_RESULT_NEXT) {
            if (FIELD_PROC_LINK_LIST[work->appId].call == NULL) {
                *state = FIELD_APP_CALL_STATE_FADE_OUT;
            } else {
                *state = FIELD_APP_CALL_STATE_CALL;
            }
        } else {
            *state = FIELD_APP_CALL_STATE_END;
        }
        break;
    case FIELD_APP_CALL_STATE_END:
        return GAMEEVENT_DONE;
    case FIELD_APP_CALL_STATE_CALL:
        if (FieldAppCallParam_RunCallback1(&work->params)) {
            FIELD_PROC_LINK_LIST[work->appId].call(work, work->input->appParam);
            *state = FIELD_APP_CALL_STATE_CALL_RESULT;
        }
        break;
    case FIELD_APP_CALL_STATE_CALL_RESULT:
        EventFieldAppCall_ConvAppResultToEventType(work->result, &work->input->eventType);
        *state = FIELD_APP_CALL_STATE_CALL_RETRY;
        break;
    case FIELD_APP_CALL_STATE_CALL_RETRY:
        if (work->result == FIELD_APP_RESULT_RETURN) {
            if (FieldAppCallParam_CanRetry(&work->params)) {
                *state = FIELD_APP_CALL_STATE_NEXT_APP;
            }
        } else {
            *state = FIELD_APP_CALL_STATE_NEXT_APP;
        }
        break;
    case FIELD_APP_CALL_STATE_FADE_OUT:
        GameEvent_ChainNext(event, CallFieldMapEntranceOutTransitionDefault(work->input->gameSystem, work->input->field,
                                                                            FIELD_TRANSITION_FADE_BLACK, 0));
        *state = FIELD_APP_CALL_STATE_CLOSE_FIELD;
        break;
    case FIELD_APP_CALL_STATE_CLOSE_FIELD:
        if (FieldAppCallParam_RunCallback1(&work->params)) {
            GameEvent_ChainNext(event, CreateFieldCloseEvent(work->input->gameSystem, work->input->field));
            *state = FIELD_APP_CALL_STATE_START_PROC;
        }
        break;
    case FIELD_APP_CALL_STATE_OPEN_FIELD:
        FieldAppCallParam_RunCallback2(&work->params);
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(work->input->gameSystem));
        *state = FIELD_APP_CALL_STATE_SETUP_FIELD;
        break;
    case FIELD_APP_CALL_STATE_SETUP_FIELD:
        work->input->field = GSYS_GetField(work->input->gameSystem);
        FieldG2D_SetLCDConfig();
        FieldG2D_Prepare3DSurface(work->input->field);
        *state = FIELD_APP_CALL_STATE_FADE_IN;
        break;
    case FIELD_APP_CALL_STATE_FADE_IN:
        if (FieldAppCallParam_CanRetry(&work->params)) {
            GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(work->input->gameSystem, work->input->field,
                                                                        FIELD_TRANSITION_FADE_BLACK, 0, 0, 0, 0));
            *state = FIELD_APP_CALL_STATE_NEXT_APP;
        }
        break;
    case FIELD_APP_CALL_STATE_START_PROC:
        param =
            FIELD_PROC_LINK_LIST[work->appId].createParam(work, work->input->appParam, work->prevAppId, work->appParam);
        if (work->appParam != NULL) {
            FIELD_PROC_LINK_LIST[work->prevAppId].freeParam(work->appParam);
            work->appParam = NULL;
        }
        work->appParam = param;
        if (FIELD_PROC_LINK_LIST[work->appId].functions != NULL) {
            GSYS_QueueProc(work->input->gameSystem, FIELD_PROC_LINK_LIST[work->appId].overlayId,
                           FIELD_PROC_LINK_LIST[work->appId].functions, param);
            *state = FIELD_APP_CALL_STATE_WAIT_PROC;
        }
        break;
    case FIELD_APP_CALL_STATE_WAIT_PROC:
        if (GSYS_GetProcMgrState(work->input->gameSystem)) {
            break;
        }
        *state = FIELD_APP_CALL_STATE_PROC_RESULT;
        break;
    case FIELD_APP_CALL_STATE_PROC_RESULT:
        work->result = FIELD_PROC_LINK_LIST[work->appId].getResult(work, work->appParam);
        EventFieldAppCall_ConvAppResultToEventType(work->result, &work->input->eventType);
        if (work->result == FIELD_APP_RESULT_NEXT) {
            work->prevAppId = work->appId;
            work->appId = work->nextAppId;
            *state = FIELD_APP_CALL_STATE_START_PROC;
        } else {
            FieldAppCall_ClearBagFreeSpaceFilter(work);
            if (work->appParam != NULL) {
                GFL_HeapFree(work->appParam);
                work->appParam = NULL;
            }
            *state = FIELD_APP_CALL_STATE_OPEN_FIELD;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

void EventFieldAppCall_ConvAppResultToEventType(u32 result, u32 *eventType) {
    switch (result) {
    case FIELD_APP_RESULT_RETURN:
        *eventType = FIELD_APP_RESULT_RETURN;
        break;
    case FIELD_APP_RESULT_EXIT:
        *eventType = FIELD_APP_RESULT_EXIT;
        break;
    case FIELD_APP_RESULT_COMMON_EVENT:
        *eventType = FIELD_APP_RESULT_COMMON_EVENT;
        break;
    case FIELD_APP_RESULT_HM_EVENT:
        *eventType = FIELD_APP_RESULT_HM_EVENT;
        break;
    case FIELD_APP_RESULT_SCRIPT:
        *eventType = FIELD_APP_RESULT_SCRIPT;
        break;
    default:
        break;
    }
}

void FieldAppCall_ClearBagFreeSpaceFilter(FieldAppCallWork *work) {
    GameSystem *gsys = work->input->gameSystem;
    GameData *gameData = GSYS_GetGameData(gsys);
    BagCursor *data = GameData_GetBagCursor(gameData);

    BagCursor_SetFreeSpaceFilter(data, 0);
}

void FieldAppCallParam_Init(FieldAppCallParam *param, FieldAppCallInput *input, FieldAppCallPredicate canRetry,
                            FieldAppCallPredicate callback1, FieldAppCallPredicate callback2, void *arg) {
    MI_CpuFill8(param, 0, sizeof(FieldAppCallParam));
    param->canRetry = canRetry;
    param->callback1 = callback1;
    param->callback2 = callback2;
    param->arg = arg;
    param->input = input;
}

BOOL FieldAppCallParam_CanRetry(FieldAppCallParam *param) {
    if (param->canRetry != NULL) {
        return param->canRetry(param->input, param->arg);
    }
    return TRUE;
}

BOOL FieldAppCallParam_RunCallback1(FieldAppCallParam *param) {
    if (param->callback1 != NULL) {
        return param->callback1(param->input, param->arg);
    }
    return TRUE;
}

BOOL FieldAppCallParam_RunCallback2(FieldAppCallParam *param) {
    if (param->callback2 != NULL) {
        return param->callback2(param->input, param->arg);
    }
    return TRUE;
}

// The party screen
void *FieldAppCall_CreatePokeListParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    GameData *gameData = GSYS_GetGameData(work->input->gameSystem);
    PokeListParam *param = PokeListParam_Create(gameData, 0, GameData_GetParty(gameData), HEAPID_GAMEEVENT);
    const BagProcessData *bag;
    const PStatusParam *status;
    BOOL mailResult;

    param->action = work->action;
    param->index = work->input->partySlot;
    param->move = 0;
    param->learnable = 0xff;
    param->zoneId = GameData_GetPlayerState(gameData)->zoneId;
    if (GameData_IsShortcutRegistered(gameData, SHORTCUT_POKELIST) == TRUE) {
        param->keyItemRegistered = TRUE;
    } else {
        param->keyItemRegistered = FALSE;
    }
    if (appParam == ITEM_GRACIDEA || appParam == ITEM_DNA_SPLICERS_FUSE || appParam == ITEM_DNA_SPLICERS_SEPARATE ||
        appParam == ITEM_REVEAL_GLASS) {
        param->index = 0;
        param->mode = 5;
        param->item = appParam;
        work->formItemUse = TRUE;
    } else if (prevAppId == FIELD_APP_POKELIST) {
        param->index = 0;
    } else if (prevAppId == FIELD_APP_BAG) {
        bag = prevParam;
        param->item = bag->item;
        if (bag->result != 3 && bag->result != 4 && bag->result != 1 && bag->result != 18) {
            param->index = 0;
        }
        switch (bag->result) {
        case 1:
            if (bag->mode == 2) {
                param->mode = 0;
            }
            break;
        case 12:
            param->mode = 16;
            break;
        case 2:
            param->mode = 9;
            break;
        case 3:
        case 18:
            param->mode = 10;
            break;
        case 4:
            param->mode = 10;
            break;
        case 5:
            param->mode = 5;
            break;
        case 6:
            param->mode = 6;
            break;
        default:
            param->mode = 9;
            break;
        }
    } else if (prevAppId == FIELD_APP_POKESTATUS) {
        status = prevParam;
        switch (status->result) {
        case PSTATUS_RESULT_FORGET:
        case PSTATUS_RESULT_BACK:
            if (status->mode == PSTATUS_MODE_FORGET_MOVE) {
                if (work->subMode == 6) {
                    param->mode = 8;
                    param->learnIndex = work->learnIndex;
                } else {
                    param->mode = 7;
                }
                param->item = work->item;
                param->move = status->move;
                if (status->result == PSTATUS_RESULT_FORGET) {
                    param->moveSlot = status->slot;
                } else {
                    param->moveSlot = 0xff;
                }
            } else {
                param->mode = 0;
                param->index = status->partyIndex;
            }
            break;
        }
    } else if (prevAppId == FIELD_APP_MAIL) {
        GFL_OvlLoad(OVERLAY_ID(215));
        mailResult = MailTool_IsWritten(prevParam);
        GFL_OvlUnload(OVERLAY_ID(215));
        if (work->subMode == 0) {
            param->mode = 11;
            param->index = work->partySlot;
            param->item = work->item;
        } else if (work->subMode == 2) {
            if (mailResult == TRUE) {
                param->mode = 12;
                param->index = work->partySlot;
                param->item = work->item;
            } else {
                param->mode = 0;
                param->index = work->partySlot;
            }
        }
    } else if (prevAppId == FIELD_APP_EVOLUTION_DEMO) {
        param->mode = 5;
        param->item = work->item;
    }
    return param;
}

u32 FieldAppCall_GetPokeListResult(FieldAppCallWork *work, void *data) {
    PokeListParam *param = data;
    GameData *gameData;

    work->input->partySlot = param->index;
    work->partySlot = param->index;
    if (work->formItemUse == TRUE) {
        if (param->index == 8) {
            return FIELD_APP_RESULT_EXIT;
        }
        return FIELD_APP_RESULT_RETURN;
    }
    gameData = GSYS_GetGameData(work->input->gameSystem);
    if (param->keyItemRegistered == TRUE) {
        GameData_SetKeyItemRegistration(gameData, SHORTCUT_POKELIST, TRUE);
    } else {
        GameData_SetKeyItemRegistration(gameData, SHORTCUT_POKELIST, FALSE);
    }
    switch (param->result) {
    case 0:
        if (param->mode == 6 || work->input->appId == FIELD_APP_BAG) {
            work->nextAppId = FIELD_APP_BAG;
            return FIELD_APP_RESULT_NEXT;
        }
        if (param->index == 8) {
            return FIELD_APP_RESULT_EXIT;
        }
        return FIELD_APP_RESULT_RETURN;
    case 1:
        work->nextAppId = FIELD_APP_POKESTATUS;
        return FIELD_APP_RESULT_NEXT;
    case 3:
        work->nextAppId = FIELD_APP_BAG;
        return FIELD_APP_RESULT_NEXT;
    case 10:
        work->nextAppId = FIELD_APP_BAG;
        return FIELD_APP_RESULT_NEXT;
    case 4:
    case 5:
        work->nextAppId = FIELD_APP_POKESTATUS;
        work->item = param->item;
        work->learnIndex = param->learnIndex;
        return FIELD_APP_RESULT_NEXT;
    case 6:
    case 7:
        work->nextAppId = FIELD_APP_MAIL;
        return FIELD_APP_RESULT_NEXT;
    case 8:
    case 9:
        work->nextAppId = FIELD_APP_EVOLUTION_DEMO;
        work->item = param->item;
        return FIELD_APP_RESULT_NEXT;
    case 11:
    case 12:
    case 13:
    case 14:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
        work->input->eventId = param->result - 11;
        return FIELD_APP_RESULT_HM_EVENT;
    case 15:
        work->nextAppId = FIELD_APP_TOWNMAP;
        return FIELD_APP_RESULT_NEXT;
    }
    return FIELD_APP_RESULT_EXIT;
}

// The Pokédex
void *FieldAppCall_CreatePokedexParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    PokedexParam *param =
        GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(PokedexParam), FALSE, "event_field_proclink.c", 1295);
    FieldSound *sound;

    MI_CpuFill8(param, 0, sizeof(PokedexParam));
    param->gameData = GSYS_GetGameData(work->input->gameSystem);
    param->pokedex = GameData_GetPokedex(param->gameData);
    param->playerInfo = GetGameDataPlayerInfo(param->gameData);
    sound = GameData_GetFieldSoundSystem(param->gameData);
    FieldSnd_DuckVolume(sound, GameSystem_GetISS(work->input->gameSystem));
    if (appParam != -1) {
        param->start = appParam;
    } else {
        param->start = POKEDEX_START_OPEN;
    }
    return param;
}

u32 FieldAppCall_GetPokedexResult(FieldAppCallWork *work, void *data) {
    PokedexParam *param = data;
    FieldSound *sound = GameData_GetFieldSoundSystem(param->gameData);

    FieldSnd_RestoreVolume(sound, GameSystem_GetISS(work->input->gameSystem));
    switch (param->result) {
    case 0:
        return FIELD_APP_RESULT_RETURN;
    default:
        return FIELD_APP_RESULT_EXIT;
    }
}

// The summary screen
void *FieldAppCall_CreatePStatusParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    PStatusParam *param =
        GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(PStatusParam), TRUE, "event_field_proclink.c", 1362);
    GameData *gameData = GSYS_GetGameData(work->input->gameSystem);
    SaveControl *save = GameData_GetSaveControl(gameData);
    PokeDexSave *pokedex = GameData_GetPokedex(gameData);
    const PokeListParam *partyParam;
    s32 i;
    PokeParty *party;
    PartyPkm *pkm;

    param->dataType = PSTATUS_DATA_PARTY;
    param->gameData = gameData;
    param->forceExit = FALSE;
    param->isNationalDex = PokeDex_IsNationalObtained(pokedex);
    if (prevAppId == FIELD_APP_POKELIST) {
        PokeParty *listParty;
        TrainerDataSave *trainerData;

        partyParam = prevParam;
        listParty = partyParam->party;
        trainerData = partyParam->trainerData;
        param->party = listParty;
        param->trainerData = trainerData;
        param->partyCount = PokeParty_GetPkmCount(listParty);
        param->partyIndex = partyParam->index;
        switch (partyParam->result) {
        case 4:
            param->move = partyParam->move;
            param->mode = PSTATUS_MODE_FORGET_MOVE;
            param->page = PSTATUS_PAGE_SKILL;
            param->fromFieldMenu = FALSE;
            work->subMode = 5;
            break;
        case 5:
            param->move = partyParam->move;
            param->mode = PSTATUS_MODE_FORGET_MOVE;
            param->page = PSTATUS_PAGE_SKILL;
            param->fromFieldMenu = FALSE;
            work->subMode = 6;
            break;
        default:
            param->page = PSTATUS_PAGE_INFO;
            param->mode = PSTATUS_MODE_NORMAL;
            param->fromFieldMenu = TRUE;
            break;
        }
    } else if (prevAppId == FIELD_APP_POKESTATUS) {
        param->trainerData = getTrainerDataBlkAddress(save);
        param->party = GameData_GetParty(gameData);
        param->partyCount = PokeParty_GetPkmCount(GameData_GetParty(gameData));
        param->partyIndex = 0;
        param->mode = PSTATUS_MODE_NORMAL;
        param->fromFieldMenu = TRUE;
        if (appParam != -1) {
            param->page = appParam;
            if (param->page == PSTATUS_PAGE_RIBBON) {
                party = GameData_GetParty(gameData);
                for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
                    pkm = PokeParty_GetPkm(party, i);
                    if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL) && PokeParty_CheckAnyRibbon(pkm) &&
                        !PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
                        param->partyIndex = i;
                        break;
                    }
                }
            } else if (param->page == PSTATUS_PAGE_SKILL) {
                party = GameData_GetParty(gameData);
                for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
                    pkm = PokeParty_GetPkm(party, i);
                    if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL) &&
                        !PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
                        param->partyIndex = i;
                        break;
                    }
                }
            }
        } else {
            param->page = PSTATUS_PAGE_INFO;
        }
    }
    return param;
}

u32 FieldAppCall_GetPStatusResult(FieldAppCallWork *work, void *data) {
    PStatusParam *param = data;

    switch (param->result) {
    case PSTATUS_RESULT_FORGET:
    case PSTATUS_RESULT_BACK:
        if (work->input->appId == FIELD_APP_POKESTATUS) {
            return FIELD_APP_RESULT_RETURN;
        }
        work->nextAppId = FIELD_APP_POKELIST;
        return FIELD_APP_RESULT_NEXT;
    case PSTATUS_RESULT_CLOSE:
        return FIELD_APP_RESULT_EXIT;
    }
    return FIELD_APP_RESULT_EXIT;
}

// The bag's mode where the player is
u32 FieldAppCall_GetBagMode(GameSystem *gsys) {
    u16 zoneId = PlayerState_GetZoneID(GSYS_GetPlayerState(gsys));

    if (GetZoneIsUnionRoom(zoneId) == TRUE) {
        return 1;
    }
    if (IsZone150Or151(zoneId) == TRUE) {
        return 3;
    }
    return 0;
}

// The bag
void *FieldAppCall_CreateBagParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    GameData *gameData = GSYS_GetGameData(work->input->gameSystem);
    BagProcessData *param;
    const PokeListParam *partyParam = prevParam;

    switch (prevAppId) {
    case FIELD_APP_BAG:
        param =
            BagParam_Create(gameData, &work->perms, FieldAppCall_GetBagMode(work->input->gameSystem), HEAPID_GAMEEVENT);
        if (appParam != -1) {
            BagCursor_SetPocket(param->cursor, appParam);
        }
        break;
    case FIELD_APP_POKELIST:
        if (partyParam->result == 3) {
            param = BagParam_Create(gameData, &work->perms, 2, HEAPID_GAMEEVENT);
        } else {
            param = BagParam_Create(gameData, &work->perms, FieldAppCall_GetBagMode(work->input->gameSystem),
                                    HEAPID_GAMEEVENT);
        }
        break;
    case 8:
    case 9:
    default:
        param =
            BagParam_Create(gameData, &work->perms, FieldAppCall_GetBagMode(work->input->gameSystem), HEAPID_GAMEEVENT);
        break;
    }
    return param;
}

u32 FieldAppCall_GetBagResult(FieldAppCallWork *work, void *data) {
    BagProcessData *param = data;
    GameData *gameData;

    switch (param->result) {
    case 0:
        return FIELD_APP_RESULT_EXIT;
    case 1:
        if (param->mode == 2) {
            work->nextAppId = FIELD_APP_POKELIST;
            return FIELD_APP_RESULT_NEXT;
        }
        return FIELD_APP_RESULT_RETURN;
    case 12:
        work->nextAppId = FIELD_APP_POKELIST;
        return FIELD_APP_RESULT_NEXT;
    case 10:
    case 11:
        work->input->eventId = FIELD_COMMON_EVENT_CYCLING;
        return FIELD_APP_RESULT_COMMON_EVENT;
    case 13:
        work->input->eventId = FIELD_COMMON_EVENT_ENTRALINK;
        return FIELD_APP_RESULT_COMMON_EVENT;
    case 15:
        work->input->eventId = FIELD_COMMON_EVENT_SWEET_SCENT;
        return FIELD_APP_RESULT_COMMON_EVENT;
    case 16:
        work->input->eventId = FIELD_COMMON_EVENT_FISHING;
        return FIELD_APP_RESULT_COMMON_EVENT;
    case 14:
        work->input->eventId = FIELD_COMMON_EVENT_ESCAPE_ROPE;
        return FIELD_APP_RESULT_COMMON_EVENT;
    case 8:
        work->nextAppId = FIELD_APP_WIFI_NOTE;
        return FIELD_APP_RESULT_NEXT;
    case 7:
        work->nextAppId = FIELD_APP_TOWNMAP;
        return FIELD_APP_RESULT_NEXT;
    case 22:
        work->nextAppId = FIELD_APP_MEDALBOX;
        return FIELD_APP_RESULT_NEXT;
    case 17:
        work->nextAppId = FIELD_APP_BATTLE_RECORDER;
        return FIELD_APP_RESULT_NEXT;
    case 21:
        work->nextAppId = FIELD_APP_XTRANSCEIVER;
        return FIELD_APP_RESULT_NEXT;
    case 19:
        work->nextAppId = FIELD_APP_MAIL;
        return FIELD_APP_RESULT_NEXT;
    case 18:
        gameData = GSYS_GetGameData(work->input->gameSystem);
        if (PokeParty_GetParam(PokeParty_GetPkm(GameData_GetParty(gameData), work->input->partySlot), PKM_PARAM_ITEM,
                               NULL)) {
            work->nextAppId = FIELD_APP_POKELIST;
        } else {
            work->nextAppId = FIELD_APP_MAIL;
        }
        work->subMode = 2;
        return FIELD_APP_RESULT_NEXT;
    case 20:
        work->input->eventId = FIELD_COMMON_EVENT_DOWSING;
        return FIELD_APP_RESULT_COMMON_EVENT;
    case 2:
        if (PML_ItemIsMail(param->item) == TRUE) {
            work->subMode = 0;
        }
        work->nextAppId = FIELD_APP_POKELIST;
        return FIELD_APP_RESULT_NEXT;
    }
    work->nextAppId = FIELD_APP_POKELIST;
    return FIELD_APP_RESULT_NEXT;
}

// The trainer card
void *FieldAppCall_CreateTrainerCardParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    GameData *gameData = GSYS_GetGameData(work->input->gameSystem);
    u32 zoneId = GameData_GetPlayerState(gameData)->zoneId;
    BOOL canEdit = TRUE;
    FieldSound *sound = GameData_GetFieldSoundSystem(gameData);
    TrainerCardParam *param;

    FieldSnd_DuckVolume(sound, GameSystem_GetISS(work->input->gameSystem));
    if (GetZoneIsUnionRoom(zoneId) || IsZone150Or151(zoneId)) {
        canEdit = FALSE;
    }
    param = TrainerCardParam_Create(gameData, HEAPID_GAMEEVENT, canEdit);
    if (appParam != -1) {
        param->appParam = appParam;
    }
    return param;
}

u32 FieldAppCall_GetTrainerCardResult(FieldAppCallWork *work, void *data) {
    TrainerCardParam *param = data;
    FieldSound *sound = GameData_GetFieldSoundSystem(GSYS_GetGameData(work->input->gameSystem));

    FieldSnd_RestoreVolume(sound, GameSystem_GetISS(work->input->gameSystem));
    if (param->result == 1) {
        return FIELD_APP_RESULT_EXIT;
    }
    return FIELD_APP_RESULT_RETURN;
}

// The town map
void *FieldAppCall_CreateTownMapParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    TownMapParam *param =
        GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(TownMapParam), TRUE, "event_field_proclink.c", 1797);
    PlayerState *playerState;
    GameData *gameData;

    param->mode = TOWNMAP_MODE_VIEW;
    param->gsys = work->input->gameSystem;
    playerState = GameData_GetPlayerState(GSYS_GetGameData(work->input->gameSystem));
    gameData = GSYS_GetGameData(work->input->gameSystem);
    param->zoneId = PlayerState_GetZoneID(playerState);
    param->escapeZoneId = GameData_GetEscapeRopeZone(gameData)->zoneId;
    if (work->input->appId == FIELD_APP_POKELIST) {
        param->mode = TOWNMAP_MODE_FLY;
    } else {
        param->mode = TOWNMAP_MODE_VIEW;
    }
    return param;
}

u32 FieldAppCall_GetTownMapResult(FieldAppCallWork *work, void *data) {
    TownMapParam *param = data;

    if (work->input->appId == FIELD_APP_TOWNMAP) {
        switch (param->result) {
        case TOWNMAP_RESULT_RETURN:
            return FIELD_APP_RESULT_RETURN;
        case TOWNMAP_RESULT_CLOSE:
            return FIELD_APP_RESULT_EXIT;
        default:
            return FIELD_APP_RESULT_EXIT;
        }
    } else if (work->input->appId == FIELD_APP_BAG) {
        switch (param->result) {
        case TOWNMAP_RESULT_RETURN:
            work->nextAppId = FIELD_APP_BAG;
            return FIELD_APP_RESULT_NEXT;
        case TOWNMAP_RESULT_CLOSE:
            return FIELD_APP_RESULT_EXIT;
        default:
            return FIELD_APP_RESULT_EXIT;
        }
    } else if (work->input->appId == FIELD_APP_POKELIST) {
        switch (param->result) {
        case TOWNMAP_RESULT_RETURN:
            work->nextAppId = FIELD_APP_POKELIST;
            return FIELD_APP_RESULT_NEXT;
        case TOWNMAP_RESULT_CLOSE:
            return FIELD_APP_RESULT_EXIT;
        case TOWNMAP_RESULT_FLY:
            work->input->eventId = HM_EVENT_FLY;
            work->input->eventValue = param->zoneId;
            work->input->warpX = param->warpX;
            work->input->warpZ = param->warpZ;
            work->input->unk40 = 0;
            RecordAddOne(GameData_GetRecords(GSYS_GetGameData(param->gsys)), 0x2a);
            return FIELD_APP_RESULT_HM_EVENT;
        default:
            return FIELD_APP_RESULT_EXIT;
        }
    }
    return FIELD_APP_RESULT_EXIT;
}

void *FieldAppCall_CreateConfigParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    ConfigPanelParam *param;

    getTrainerDataBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(work->input->gameSystem)));
    param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(ConfigPanelParam), TRUE, "event_field_proclink.c", 1926);
    param->gsys = work->input->gameSystem;
    return param;
}

u32 FieldAppCall_GetConfigResult(FieldAppCallWork *work, void *data) {
    ConfigPanelParam *param = data;

    if (param->result) {
        return FIELD_APP_RESULT_EXIT;
    }
    return FIELD_APP_RESULT_RETURN;
}

// The save, which has no proc
void FieldAppCall_CallSave(FieldAppCallWork *work, s32 appParam) {
    FieldAppCallInput *input = work->input;

    GameEvent_ChainNext(work->event, EventSave_Create(input->gameSystem, input->field, work->code,
                                                      Field_GetMsgBGSys(input->field), input->screenId, &work->result));
}

void *FieldAppCall_CreateWifiNoteParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    WifiNoteParam *param =
        GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(WifiNoteParam), TRUE, "event_field_proclink.c", 1997);

    param->gameData = GSYS_GetGameData(work->input->gameSystem);
    return param;
}

u32 FieldAppCall_GetWifiNoteResult(FieldAppCallWork *work, void *data) {
    WifiNoteParam *param = data;

    if (work->input->appId == FIELD_APP_WIFI_NOTE) {
        switch (param->result) {
        case WIFI_NOTE_RESULT_RETURN:
            return FIELD_APP_RESULT_RETURN;
        default:
            return FIELD_APP_RESULT_EXIT;
        }
    }
    if (param->result == WIFI_NOTE_RESULT_RETURN) {
        work->nextAppId = FIELD_APP_BAG;
        return FIELD_APP_RESULT_NEXT;
    }
    return FIELD_APP_RESULT_EXIT;
}

// The mail
void *FieldAppCall_CreateMailParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    GameData *gameData = GSYS_GetGameData(work->input->gameSystem);
    const PokeListParam *partyParam;
    const BagProcessData *bag;
    PokeParty *party;
    void *param;
    u32 slot;
    u32 item;

    GFL_OvlLoad(OVERLAY_ID(215));
    if (prevAppId == FIELD_APP_POKELIST) {
        partyParam = prevParam;
        party = GameData_GetParty(gameData);
        slot = partyParam->index;
        work->partySlot = slot;
        item = partyParam->item;
        work->item = item;
        if (partyParam->result == 6) {
            return MailTool_CreateWriteParam(gameData, 2, partyParam->index, PML_ItemGetMailID(item), HEAPID_GAMEEVENT);
        }
        work->subMode = 1;
        return MailTool_CreatePokemonViewParam(gameData, PokeParty_GetPkm(party, slot), HEAPID_GAMEEVENT);
    }
    bag = prevParam;
    if (bag->mode == 2) {
        param = MailTool_CreateWriteParam(gameData, 2, bag->item, PML_ItemGetMailID(bag->item), HEAPID_GAMEEVENT);
        work->item = bag->item;
        return param;
    }
    work->subMode = 1;
    return MailTool_CreateItemViewParam(gameData, PML_ItemGetMailID(bag->item), HEAPID_GAMEEVENT);
}

u32 FieldAppCall_GetMailResult(FieldAppCallWork *work, void *param) {
    GameData *gameData = GSYS_GetGameData(work->input->gameSystem);

    if (work->prevAppId == FIELD_APP_POKELIST) {
        if (work->subMode == 0 || work->subMode == 2) {
            if (MailTool_IsWritten(param) == TRUE) {
                MailTool_GiveMail(param, PokeParty_GetPkm(GameData_GetParty(gameData), work->partySlot));
                work->nextAppId = FIELD_APP_POKELIST;
            } else if (work->subMode == 0) {
                work->nextAppId = FIELD_APP_BAG;
            } else {
                work->nextAppId = FIELD_APP_POKELIST;
            }
        } else {
            work->nextAppId = FIELD_APP_POKELIST;
        }
        GFL_OvlUnload(OVERLAY_ID(215));
        return FIELD_APP_RESULT_NEXT;
    } else if (work->prevAppId == FIELD_APP_BAG) {
        if (work->subMode == 2) {
            if (MailTool_IsWritten(param) == TRUE) {
                MailTool_GiveMail(param, PokeParty_GetPkm(GameData_GetParty(gameData), work->partySlot));
                work->nextAppId = FIELD_APP_POKELIST;
            } else {
                work->nextAppId = FIELD_APP_POKELIST;
            }
        } else {
            work->nextAppId = FIELD_APP_BAG;
        }
        GFL_OvlUnload(OVERLAY_ID(215));
        return FIELD_APP_RESULT_NEXT;
    }
    work->nextAppId = FIELD_APP_BAG;
    GFL_OvlUnload(OVERLAY_ID(215));
    return FIELD_APP_RESULT_NEXT;
}

void FieldAppCall_FreeMailParam(void *param) {
    GFL_OvlLoad(OVERLAY_ID(215));
    MailTool_FreeParam(param);
    GFL_OvlUnload(OVERLAY_ID(215));
}

// The evolution demo, for an item used from the bag or the party screen
void *script_evo(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    GameData *gameData = GSYS_GetGameData(work->input->gameSystem);
    EvolutionDemoParam *param = NULL;
    const PokeListParam *partyParam = prevParam;
    PokeParty *party;
    PartyPkm *pkm;
    PlayerState *playerState;
    u32 species;
    u32 method;

    if (prevAppId == FIELD_APP_POKELIST) {
        party = GameData_GetParty(gameData);
        if (partyParam->result == 8) {
            work->subMode = 3;
            pkm = PokeParty_GetPkm(party, partyParam->index);
            species = CheckEvolveSpecies(party, pkm, 3, partyParam->item, GameData_GetSeason(gameData), &method,
                                         HEAPID_GAMEEVENT);
        } else {
            work->subMode = 4;
            pkm = PokeParty_GetPkm(party, partyParam->index);
            playerState = GameData_GetPlayerState(gameData);
            species = CheckEvolveSpecies(party, pkm, 0, playerState->zoneId, GameData_GetSeason(gameData), &method,
                                         HEAPID_GAMEEVENT);
        }
        param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(EvolutionDemoParam), FALSE, "event_field_proclink.c", 2281);
        param->gameData = gameData;
        param->party = party;
        param->species = species;
        param->partyIndex = partyParam->index;
        param->method = method;
        param->unkC = 1;
        param->canCancel = TRUE;
    }
    return param;
}

u32 FieldAppCall_GetEvolutionDemoResult(FieldAppCallWork *work, void *param) {
    GSYS_GetGameData(work->input->gameSystem);
    if (work->subMode == 3) {
        work->nextAppId = FIELD_APP_BAG;
        return FIELD_APP_RESULT_NEXT;
    }
    if (work->subMode == 4) {
        work->nextAppId = FIELD_APP_POKELIST;
        return FIELD_APP_RESULT_NEXT;
    }
    return FIELD_APP_RESULT_RETURN;
}

void *FieldAppCall_CreateBattleRecorderParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId,
                                             const void *prevParam) {
    BattleRecorderParam *param =
        GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(BattleRecorderParam), TRUE, "event_field_proclink.c", 2349);

    param->gameData = GSYS_GetGameData(work->input->gameSystem);
    return param;
}

u32 FieldAppCall_GetBattleRecorderResult(FieldAppCallWork *work, void *data) {
    BattleRecorderParam *param = data;

    if (work->input->appId == FIELD_APP_BATTLE_RECORDER) {
        switch (param->result) {
        case 0:
            return FIELD_APP_RESULT_RETURN;
        case 1:
            return FIELD_APP_RESULT_EXIT;
        default:
            return FIELD_APP_RESULT_RETURN;
        }
    }
    switch (param->result) {
    case 0:
        work->nextAppId = FIELD_APP_BAG;
        return FIELD_APP_RESULT_NEXT;
    case 1:
        return FIELD_APP_RESULT_EXIT;
    default:
        return FIELD_APP_RESULT_RETURN;
    }
}

void *FieldAppCall_CreateXtransceiverParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    LocalTvtParam *param =
        GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(LocalTvtParam), TRUE, "event_field_proclink.c", 2417);

    param->mode = LOCAL_TVT_MODE_CALL;
    param->scriptId = 4;
    param->gameData = GSYS_GetGameData(work->input->gameSystem);
    param->gsys = work->input->gameSystem;
    return param;
}

u32 FieldAppCall_GetXtransceiverResult(FieldAppCallWork *work, void *param) {
    if (work->input->appId == FIELD_APP_XTRANSCEIVER) {
        return FIELD_APP_RESULT_RETURN;
    }
    work->nextAppId = FIELD_APP_BAG;
    return FIELD_APP_RESULT_NEXT;
}

void *FieldAppCall_CreateMedalBoxParam(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    MedalBoxParam *param =
        GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(MedalBoxParam), TRUE, "event_field_proclink.c", 2496);

    param->gameData = GSYS_GetGameData(work->input->gameSystem);
    return param;
}

u32 FieldAppCall_GetMedalBoxResult(FieldAppCallWork *work, void *data) {
    MedalBoxParam *param = data;

    if (work->input->appId == FIELD_APP_MEDALBOX) {
        switch (param->result) {
        case MEDALBOX_RESULT_RETURN:
            return FIELD_APP_RESULT_RETURN;
        case MEDALBOX_RESULT_EXIT:
            return FIELD_APP_RESULT_EXIT;
        default:
            return FIELD_APP_RESULT_EXIT;
        }
    }
    if (work->input->appId == FIELD_APP_BAG) {
        switch (param->result) {
        case MEDALBOX_RESULT_RETURN:
            work->nextAppId = FIELD_APP_BAG;
            return FIELD_APP_RESULT_NEXT;
        case MEDALBOX_RESULT_EXIT:
            return FIELD_APP_RESULT_EXIT;
        default:
            return FIELD_APP_RESULT_EXIT;
        }
    }
    return FIELD_APP_RESULT_RETURN;
}
