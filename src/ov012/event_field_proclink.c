#include "types.h"
#include "app/bag.h"
#include "app/battle_recorder.h"
#include "app/p_status.h"
#include "app/pokelist.h"
#include "app/wifi_note.h"
#include "app/xtransceiver.h"
#include "app/zukan.h"
#include "constants/pokemon.h"
#include "demo/shinka_demo.h"
#include "field/app_call.h"
#include "field/event_save.h"
#include "field/field.h"
#include "field/field_event.h"
#include "field/field_sound.h"
#include "field/itemuse_event.h"
#include "field/mailsys.h"
#include "field/mailtool.h"
#include "field/player_action.h"
#include "field/player_state.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "pml/evolution.h"
#include "pml/item.h"
#include "pml/mail.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/pokedex.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The params of the apps that no header describes yet

// The trainer card, which func_ov012_02169b04 allocates
typedef struct {
    u8 unk0[0x14];
    u32 result;
    s32 appParam;
} TrainerCardParam;

// The town map
typedef struct {
    u32 mode;
    u32 result;
    u16 zoneId;
    s16 escapeZoneId;
    u32 unk0C;
    u32 unk10;
    GameSystem *gsys;
} TownMapParam;

typedef struct {
    u32 result;
    GameSystem *gsys;
} Ov140Param;

typedef struct {
    GameData *gameData;
    u32 result;
} Ov204Param;

typedef struct {
    u32 unk0;
    u32 unk4;
    u16 unk8;
    GameData *gameData;
    GameSystem *gsys;
} Ov259Param;

typedef struct {
    GameData *gameData;
    u32 result;
} Ov145Param;

const FieldProcLink FIELD_PROC_LINK_LIST[15] = {
    { OVERLAY_ID(165), &POKELIST_PROC_FUNCTIONS, func_ov012_0215b7d8, func_ov012_0215b9cc, NULL, func_ov012_0215c594 },
    { OVERLAY_ID(302), &ZUKAN_PROC_FUNCTIONS, func_ov012_0215bad4, func_ov012_0215bb44, NULL, func_ov012_0215c594 },
    { OVERLAY_ID(142), &BAG_PROC_FUNCTIONS, func_ov012_0215bd48, func_ov012_0215bdd0, NULL, func_ov012_0215c594 },
    { OVERLAY_ID(186), &data_ov012_0216dd78, func_ov012_0215bef4, func_ov012_0215bf58, NULL, func_ov012_0215c594 },
    { 0, NULL, NULL, NULL, func_ov012_0215c0dc, func_ov012_0215c594 },
    { OVERLAY_ID(140), &CONFIG_PANEL_PROC_FUNCTIONS, func_ov012_0215c094, func_ov012_0215c0cc, NULL, func_ov012_0215c594 },
    { 0, NULL, NULL, NULL, func_ov012_0215c574, func_ov012_0215c594 },
    { OVERLAY_PSTATUS, &PSTATUS_PROC_FUNCTIONS, func_ov012_0215bb70, func_ov012_0215bcf0, NULL, func_ov012_0215c594 },
    { OVERLAY_ID(144), &TOWNMAP_PROC_FUNCTIONS, func_ov012_0215bf8c, func_ov012_0215bff8, NULL, func_ov012_0215c594 },
    { OVERLAY_ID(204), &WIFI_NOTE_PROC_FUNCTIONS, func_ov012_0215c10c, func_ov012_0215c138, NULL, func_ov012_0215c594 },
    { OVERLAY_NONE, &MAILSYS_PROC_FUNCTIONS, func_ov012_0215c160, func_ov012_0215c218, NULL, func_ov012_0215c2c8 },
    { OVERLAY_ID(284), &SHINKA_DEMO_PROC_FUNCTIONS, script_evo, func_ov012_0215c3a4, NULL, func_ov012_0215c594 },
    { OVERLAY_ID(272), &BR_MAIN_PROC_FUNCTIONS, func_ov012_0215c3d0, func_ov012_0215c3fc, NULL, func_ov012_0215c594 },
    { OVERLAY_ID(259), &XTRANSCEIVER_PROC_FUNCTIONS, func_ov012_0215c438, func_ov012_0215c474, NULL, func_ov012_0215c594 },
    { OVERLAY_ID(145), &MEDALBOX_PROC_FUNCTIONS, func_ov012_0215c488, func_ov012_0215c4b4, NULL, func_ov012_0215c594 },
};

GameEvent *EventFieldAppCall_Create(FieldAppCallInput *input, u16 code) {
    GameEvent *event = GameEvent_Create(input->gameSystem, input->parent, EventFieldAppCall_Callback, sizeof(FieldAppCallWork));
    FieldAppCallWork *work = GameEvent_GetData(event);

    sys_memset(work, 0, sizeof(FieldAppCallWork));
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
    func_ov012_0215b76c(&work->params, work->input, work->input->canRetry, work->input->callback1,
                          work->input->callback2, work->input->arg);
    PlayerActionPerms_Create(&work->perms, work->input->gameSystem, work->input->field);
    CalcPlayerActionPossibilities(work->input->field, &work->action);
    return event;
}

GameEventReturnCode EventFieldAppCall_Callback(GameEvent *event, u32 *state, void *data) {
    FieldAppCallWork *work = data;
    void *param;

    switch (*state) {
    case 0:
        *state = 1;
        break;
    case 1:
        if (work->result == FIELD_APP_RESULT_NEXT) {
            if (FIELD_PROC_LINK_LIST[work->appId].call == NULL) {
                *state = 6;
            } else {
                *state = 3;
            }
        } else {
            *state = 2;
        }
        break;
    case 2:
        return GAMEEVENT_DONE;
    case 3:
        if (func_ov012_0215b7a8(&work->params)) {
            FIELD_PROC_LINK_LIST[work->appId].call(work, work->input->appParam);
            *state = 4;
        }
        break;
    case 4:
        EventFieldAppCall_ConvAppResultToEventType(work->result, &work->input->eventType);
        *state = 5;
        break;
    case 5:
        if (work->result == 0) {
            if (FieldAppCallParam_CanRetry(&work->params)) {
                *state = 1;
            }
        } else {
            *state = 1;
        }
        break;
    case 6:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceOutTransitionDefault(work->input->gameSystem, work->input->field, 0, 0));
        *state = 7;
        break;
    case 7:
        if (func_ov012_0215b7a8(&work->params)) {
            GameEvent_ChainNext(event, CreateFieldCloseEvent(work->input->gameSystem, work->input->field));
            *state = 11;
        }
        break;
    case 8:
        func_ov012_0215b7c0(&work->params);
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(work->input->gameSystem));
        *state = 9;
        break;
    case 9:
        work->input->field = GSYS_GetField(work->input->gameSystem);
        FieldG2D_SetLCDConfig();
        FieldG2D_Prepare3DSurface(work->input->field);
        *state = 10;
        break;
    case 10:
        if (FieldAppCallParam_CanRetry(&work->params)) {
            GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(work->input->gameSystem, work->input->field, 0,
                                                                        0, 0, 0, 0));
            *state = 1;
        }
        break;
    case 11:
        param = FIELD_PROC_LINK_LIST[work->appId].createParam(work, work->input->appParam, work->prevAppId,
                                                              work->appParam);
        if (work->appParam != NULL) {
            FIELD_PROC_LINK_LIST[work->prevAppId].freeParam(work->appParam);
            work->appParam = NULL;
        }
        work->appParam = param;
        if (FIELD_PROC_LINK_LIST[work->appId].functions != NULL) {
            GSYS_QueueProc(work->input->gameSystem, FIELD_PROC_LINK_LIST[work->appId].overlayId,
                           FIELD_PROC_LINK_LIST[work->appId].functions, param);
            *state = 12;
        }
        break;
    case 12:
        if (GSYS_GetProcMgrState(work->input->gameSystem)) {
            break;
        }
        *state = 13;
        break;
    case 13:
        work->result = FIELD_PROC_LINK_LIST[work->appId].getResult(work, work->appParam);
        EventFieldAppCall_ConvAppResultToEventType(work->result, &work->input->eventType);
        if (work->result == FIELD_APP_RESULT_NEXT) {
            work->prevAppId = work->appId;
            work->appId = work->nextAppId;
            *state = 11;
        } else {
            func_ov012_0215b754(work);
            if (work->appParam != NULL) {
                GFL_HeapFree(work->appParam);
                work->appParam = NULL;
            }
            *state = 8;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

void EventFieldAppCall_ConvAppResultToEventType(u32 result, u32 *eventType) {
    switch (result) {
    case 0:
        *eventType = 0;
        break;
    case 1:
        *eventType = 1;
        break;
    case 3:
        *eventType = 3;
        break;
    case 2:
        *eventType = 2;
        break;
    case 5:
        *eventType = 5;
        break;
    default:
        break;
    }
}

void func_ov012_0215b754(FieldAppCallWork *work) {
    GameSystem *gsys = work->input->gameSystem;
    GameData *gameData = GSYS_GetGameData(gsys);
    void *data = func_0201734c(gameData);

    func_020088ec(data, 0);
}

void func_ov012_0215b76c(FieldAppCallParam *param, FieldAppCallInput *input, FieldAppCallPredicate canRetry,
                         FieldAppCallPredicate callback1, FieldAppCallPredicate callback2, void *arg) {
    sys_memset(param, 0, sizeof(FieldAppCallParam));
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

BOOL func_ov012_0215b7a8(FieldAppCallParam *param) {
    if (param->callback1 != NULL) {
        return param->callback1(param->input, param->arg);
    }
    return TRUE;
}

BOOL func_ov012_0215b7c0(FieldAppCallParam *param) {
    if (param->callback2 != NULL) {
        return param->callback2(param->input, param->arg);
    }
    return TRUE;
}

// The party screen
void *func_ov012_0215b7d8(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    GameData *gameData = GSYS_GetGameData(work->input->gameSystem);
    PokeListParam *param = PokeListParam_Create(gameData, 0, GameData_GetParty(gameData), HEAPID_GAMEEVENT);
    const BagProcessData *bag;
    const PStatusParam *status;
    BOOL mailResult;

    param->action = work->action;
    param->index = work->input->partySlot;
    param->move = 0;
    param->unk6E = 0xff;
    param->zoneId = GameData_GetPlayerState(gameData)->zoneId;
    if (GameData_IsShortcutRegistered(gameData, 0x1e) == TRUE) {
        param->keyItemRegistered = TRUE;
    } else {
        param->keyItemRegistered = FALSE;
    }
    if (appParam == 0x1d2 || appParam == 0x274 || appParam == 0x275 || appParam == 0x27e) {
        param->index = 0;
        param->mode = 5;
        param->item = appParam;
        work->unk74 = TRUE;
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
        case 0:
        case 1:
            if (status->mode == PSTATUS_MODE_FORGET_MOVE) {
                if (work->subMode == 6) {
                    param->mode = 8;
                    param->learnIndex = work->unk6C;
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
    } else if (prevAppId == FIELD_APP_SHINKA_DEMO) {
        param->mode = 5;
        param->item = work->item;
    }
    return param;
}

u32 func_ov012_0215b9cc(FieldAppCallWork *work, void *data) {
    PokeListParam *param = data;
    GameData *gameData;

    work->input->partySlot = param->index;
    work->partySlot = param->index;
    if (work->unk74 == TRUE) {
        if (param->index == 8) {
            return 1;
        }
        return 0;
    }
    gameData = GSYS_GetGameData(work->input->gameSystem);
    if (param->keyItemRegistered == TRUE) {
        GameData_SetKeyItemRegistration(gameData, 0x1e, TRUE);
    } else {
        GameData_SetKeyItemRegistration(gameData, 0x1e, FALSE);
    }
    switch (param->result) {
    case 0:
        if (param->mode == 6 || work->input->appId == FIELD_APP_BAG) {
            work->nextAppId = FIELD_APP_BAG;
            return FIELD_APP_RESULT_NEXT;
        }
        if (param->index == 8) {
            return 1;
        }
        return 0;
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
        work->unk6C = param->learnIndex;
        return FIELD_APP_RESULT_NEXT;
    case 6:
    case 7:
        work->nextAppId = FIELD_APP_MAIL;
        return FIELD_APP_RESULT_NEXT;
    case 8:
    case 9:
        work->nextAppId = FIELD_APP_SHINKA_DEMO;
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
        return 2;
    case 15:
        work->nextAppId = 8;
        return FIELD_APP_RESULT_NEXT;
    }
    return 1;
}

// The Pokédex
void *func_ov012_0215bad4(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    ZukanParam *param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(ZukanParam), FALSE, "event_field_proclink.c", 1295);
    FieldSound *sound;

    sys_memset(param, 0, sizeof(ZukanParam));
    param->gameData = GSYS_GetGameData(work->input->gameSystem);
    param->pokedex = GameData_GetPokedex(param->gameData);
    param->playerInfo = GetGameDataPlayerInfo(param->gameData);
    sound = GameData_GetFieldSoundSystem(param->gameData);
    FieldSnd_DuckVolume(sound, GameSystem_GetISS(work->input->gameSystem));
    if (appParam != -1) {
        param->start = appParam;
    } else {
        param->start = 0;
    }
    return param;
}

u32 func_ov012_0215bb44(FieldAppCallWork *work, void *data) {
    ZukanParam *param = data;
    FieldSound *sound = GameData_GetFieldSoundSystem(param->gameData);

    FieldSnd_RestoreVolume(sound, GameSystem_GetISS(work->input->gameSystem));
    switch (param->result) {
    case 0:
        return 0;
    default:
        return 1;
    }
}


// The summary screen
void *func_ov012_0215bb70(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
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
                    if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL) && PokeParty_CheckAnyRibbon(pkm)
                        && !PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
                        param->partyIndex = i;
                        break;
                    }
                }
            } else if (param->page == PSTATUS_PAGE_SKILL) {
                party = GameData_GetParty(gameData);
                for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
                    pkm = PokeParty_GetPkm(party, i);
                    if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)
                        && !PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
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

u32 func_ov012_0215bcf0(FieldAppCallWork *work, void *data) {
    PStatusParam *param = data;

    switch (param->result) {
    case 0:
    case 1:
        if (work->input->appId == FIELD_APP_POKESTATUS) {
            return 0;
        }
        work->nextAppId = FIELD_APP_POKELIST;
        return FIELD_APP_RESULT_NEXT;
    case 2:
        return 1;
    }
    return 1;
}

// The bag's mode where the player is
u32 func_ov012_0215bd1c(GameSystem *gsys) {
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
void *func_ov012_0215bd48(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    GameData *gameData = GSYS_GetGameData(work->input->gameSystem);
    BagProcessData *param;
    const PokeListParam *partyParam = prevParam;

    switch (prevAppId) {
    case FIELD_APP_BAG:
        param = BagParam_Create(gameData, &work->perms, func_ov012_0215bd1c(work->input->gameSystem), HEAPID_GAMEEVENT);
        if (appParam != -1) {
            func_020088a4(param->cursor, appParam);
        }
        break;
    case FIELD_APP_POKELIST:
        if (partyParam->result == 3) {
            param = BagParam_Create(gameData, &work->perms, 2, HEAPID_GAMEEVENT);
        } else {
            param = BagParam_Create(gameData, &work->perms, func_ov012_0215bd1c(work->input->gameSystem),
                                  HEAPID_GAMEEVENT);
        }
        break;
    case 8:
    case 9:
    default:
        param = BagParam_Create(gameData, &work->perms, func_ov012_0215bd1c(work->input->gameSystem), HEAPID_GAMEEVENT);
        break;
    }
    return param;
}

u32 func_ov012_0215bdd0(FieldAppCallWork *work, void *data) {
    BagProcessData *param = data;
    GameData *gameData;

    switch (param->result) {
    case 0:
        return 1;
    case 1:
        if (param->mode == 2) {
            work->nextAppId = FIELD_APP_POKELIST;
            return FIELD_APP_RESULT_NEXT;
        }
        return 0;
    case 12:
        work->nextAppId = FIELD_APP_POKELIST;
        return FIELD_APP_RESULT_NEXT;
    case 10:
    case 11:
        work->input->eventId = 0;
        return 3;
    case 13:
        work->input->eventId = 1;
        return 3;
    case 15:
        work->input->eventId = 3;
        return 3;
    case 16:
        work->input->eventId = 4;
        return 3;
    case 14:
        work->input->eventId = 2;
        return 3;
    case 8:
        work->nextAppId = 9;
        return FIELD_APP_RESULT_NEXT;
    case 7:
        work->nextAppId = 8;
        return FIELD_APP_RESULT_NEXT;
    case 22:
        work->nextAppId = 14;
        return FIELD_APP_RESULT_NEXT;
    case 17:
        work->nextAppId = 12;
        return FIELD_APP_RESULT_NEXT;
    case 21:
        work->nextAppId = 13;
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
        work->input->eventId = 5;
        return 3;
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
void *func_ov012_0215bef4(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    GameData *gameData = GSYS_GetGameData(work->input->gameSystem);
    u32 zoneId = GameData_GetPlayerState(gameData)->zoneId;
    BOOL canEdit = TRUE;
    FieldSound *sound = GameData_GetFieldSoundSystem(gameData);
    TrainerCardParam *param;

    FieldSnd_DuckVolume(sound, GameSystem_GetISS(work->input->gameSystem));
    if (GetZoneIsUnionRoom(zoneId) || IsZone150Or151(zoneId)) {
        canEdit = FALSE;
    }
    param = func_ov012_02169b04(gameData, HEAPID_GAMEEVENT, canEdit);
    if (appParam != -1) {
        param->appParam = appParam;
    }
    return param;
}

u32 func_ov012_0215bf58(FieldAppCallWork *work, void *data) {
    TrainerCardParam *param = data;
    FieldSound *sound = GameData_GetFieldSoundSystem(GSYS_GetGameData(work->input->gameSystem));

    FieldSnd_RestoreVolume(sound, GameSystem_GetISS(work->input->gameSystem));
    if (param->result == 1) {
        return 1;
    }
    return 0;
}

// The town map
void *func_ov012_0215bf8c(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    TownMapParam *param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(TownMapParam), TRUE, "event_field_proclink.c", 1797);
    PlayerState *playerState;
    GameData *gameData;

    param->mode = TRUE;
    param->gsys = work->input->gameSystem;
    playerState = GameData_GetPlayerState(GSYS_GetGameData(work->input->gameSystem));
    gameData = GSYS_GetGameData(work->input->gameSystem);
    param->zoneId = PlayerState_GetZoneID(playerState);
    param->escapeZoneId = GameData_GetEscapeRopeZone(gameData)->zoneId;
    if (work->input->appId == 0) {
        param->mode = FALSE;
    } else {
        param->mode = TRUE;
    }
    return param;
}

u32 func_ov012_0215bff8(FieldAppCallWork *work, void *data) {
    TownMapParam *param = data;

    if (work->input->appId == 8) {
        switch (param->result) {
        case 0:
            return 0;
        case 1:
            return 1;
        default:
            return 1;
        }
    } else if (work->input->appId == FIELD_APP_BAG) {
        switch (param->result) {
        case 0:
            work->nextAppId = FIELD_APP_BAG;
            return FIELD_APP_RESULT_NEXT;
        case 1:
            return 1;
        default:
            return 1;
        }
    } else if (work->input->appId == 0) {
        switch (param->result) {
        case 0:
            work->nextAppId = FIELD_APP_POKELIST;
            return FIELD_APP_RESULT_NEXT;
        case 1:
            return 1;
        case 2:
            work->input->eventId = 4;
            work->input->eventValue = param->zoneId;
            work->input->unk38 = param->unk0C;
            work->input->unk3C = param->unk10;
            work->input->unk40 = 0;
            RecordAddOne(GameData_GetRecords(GSYS_GetGameData(param->gsys)), 0x2a);
            return 2;
        default:
            return 1;
        }
    }
    return 1;
}

void *func_ov012_0215c094(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    Ov140Param *param;

    getTrainerDataBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(work->input->gameSystem)));
    param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(Ov140Param), TRUE, "event_field_proclink.c", 1926);
    param->gsys = work->input->gameSystem;
    return param;
}

u32 func_ov012_0215c0cc(FieldAppCallWork *work, void *data) {
    Ov140Param *param = data;

    if (param->result) {
        return 1;
    }
    return 0;
}

// The save, which has no proc
void func_ov012_0215c0dc(FieldAppCallWork *work, s32 appParam) {
    FieldAppCallInput *input = work->input;

    GameEvent_ChainNext(work->event, EventSave_Create(input->gameSystem, input->field, work->code,
                                                      Field_GetMsgBGSys(input->field), input->screenId, &work->result));
}

void *func_ov012_0215c10c(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    Ov204Param *param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(Ov204Param), TRUE, "event_field_proclink.c", 1997);

    param->gameData = GSYS_GetGameData(work->input->gameSystem);
    return param;
}

u32 func_ov012_0215c138(FieldAppCallWork *work, void *data) {
    Ov204Param *param = data;

    if (work->input->appId == 9) {
        switch (param->result) {
        case 0:
            return 0;
        default:
            return 1;
        }
    }
    if (param->result == 0) {
        work->nextAppId = FIELD_APP_BAG;
        return FIELD_APP_RESULT_NEXT;
    }
    return 1;
}


// The mail
void *func_ov012_0215c160(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
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

u32 func_ov012_0215c218(FieldAppCallWork *work, void *param) {
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

void func_ov012_0215c2c8(void *param) {
    GFL_OvlLoad(OVERLAY_ID(215));
    MailTool_FreeParam(param);
    GFL_OvlUnload(OVERLAY_ID(215));
}

// The evolution demo, for an item used from the bag or the party screen
void *script_evo(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    GameData *gameData = GSYS_GetGameData(work->input->gameSystem);
    ShinkaDemoParam *param = NULL;
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
        param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(ShinkaDemoParam), FALSE, "event_field_proclink.c", 2281);
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

u32 func_ov012_0215c3a4(FieldAppCallWork *work, void *param) {
    GSYS_GetGameData(work->input->gameSystem);
    if (work->subMode == 3) {
        work->nextAppId = FIELD_APP_BAG;
        return FIELD_APP_RESULT_NEXT;
    }
    if (work->subMode == 4) {
        work->nextAppId = FIELD_APP_POKELIST;
        return FIELD_APP_RESULT_NEXT;
    }
    return 0;
}

void *func_ov012_0215c3d0(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    BattleRecorderParam *param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(BattleRecorderParam), TRUE, "event_field_proclink.c", 2349);

    param->gameData = GSYS_GetGameData(work->input->gameSystem);
    return param;
}

u32 func_ov012_0215c3fc(FieldAppCallWork *work, void *data) {
    BattleRecorderParam *param = data;

    if (work->input->appId == 12) {
        switch (param->result) {
        case 0:
            return 0;
        case 1:
            return 1;
        default:
            return 0;
        }
    }
    switch (param->result) {
    case 0:
        work->nextAppId = FIELD_APP_BAG;
        return FIELD_APP_RESULT_NEXT;
    case 1:
        return 1;
    default:
        return 0;
    }
}

void *func_ov012_0215c438(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    Ov259Param *param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(Ov259Param), TRUE, "event_field_proclink.c", 2417);

    param->unk0 = 1;
    param->unk8 = 4;
    param->gameData = GSYS_GetGameData(work->input->gameSystem);
    param->gsys = work->input->gameSystem;
    return param;
}

u32 func_ov012_0215c474(FieldAppCallWork *work, void *param) {
    if (work->input->appId == 13) {
        return 0;
    }
    work->nextAppId = FIELD_APP_BAG;
    return FIELD_APP_RESULT_NEXT;
}

void *func_ov012_0215c488(FieldAppCallWork *work, s32 appParam, s32 prevAppId, const void *prevParam) {
    Ov145Param *param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(Ov145Param), TRUE, "event_field_proclink.c", 2496);

    param->gameData = GSYS_GetGameData(work->input->gameSystem);
    return param;
}

u32 func_ov012_0215c4b4(FieldAppCallWork *work, void *data) {
    Ov145Param *param = data;

    if (work->input->appId == 14) {
        switch (param->result) {
        case 0:
            return 0;
        case 1:
            return 1;
        default:
            return 1;
        }
    }
    if (work->input->appId == FIELD_APP_BAG) {
        switch (param->result) {
        case 0:
            work->nextAppId = FIELD_APP_BAG;
            return FIELD_APP_RESULT_NEXT;
        case 1:
            return 1;
        default:
            return 1;
        }
    }
    return 0;
}
