#include "types.h"
#include "app/p_status.h"
#include "app/pokedex.h"
#include "constants/bag.h"
#include "constants/items.h"
#include "constants/pokemon.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_msgbg.h"
#include "field/field_script.h"
#include "field/field_script_event.h"
#include "field/hidden_event.h"
#include "field/item_use_block.h"
#include "field/itemuse_event.h"
#include "field/player_action.h"
#include "field/player_state.h"
#include "field/shortcut_menu.h"
#include "field/trcard_sys.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/std.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "pml/poke_party.h"
#include "save/shortcut.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

BOOL IsExistAnyYShortcut(GameSystem *gsys) {
    GameData *gameData = GSYS_GetGameData(gsys);
    SaveControl *save = GameData_GetSaveControl(gameData);
    ShortcutSave *shortcutSave = SaveControl_GetShortcutSave(save);

    return ShortcutSave_GetShortcutCount(shortcutSave) != 0;
}

GameEvent *CallYButtonShortcutMenu(GameSystem *gsys, Field *field, u16 code) {
    GameEvent *event;
    ShortcutMenuWork *work;
    ShortcutSave *shortcut = SaveControl_GetShortcutSave(GameData_GetSaveControl(GSYS_GetGameData(gsys)));

    if (ShortcutSave_GetShortcutCount(shortcut) == 1) {
        event = GameEvent_Create(gsys, NULL, EventShortcutCallDirect_Callback, sizeof(ShortcutMenuWork));
    } else {
        event = GameEvent_Create(gsys, NULL, EventShortcutChoicePopup_Callback, sizeof(ShortcutMenuWork));
    }
    work = GameEvent_GetData(event);
    MI_CpuFill8(work, 0, sizeof(ShortcutMenuWork));
    work->gameSystem = gsys;
    work->event = event;
    work->field = field;
    work->code = code;
    work->done = 0;
    work->input = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(FieldAppCallInput), FALSE, "event_shortcut_menu.c", 0xcb);
    MI_CpuFill8(work->input, 0, sizeof(FieldAppCallInput));
    work->input->gameSystem = work->gameSystem;
    work->input->field = work->field;
    work->input->parent = event;
    work->input->appParam = -1;
    if (ShortcutSave_GetShortcutCount(shortcut) != 1) {
        work->input->canRetry = ShortcutMenu_CanRetry;
        work->input->callback1 = ShortcutMenu_CanLeaveField;
        work->input->arg = work;
    }
    DisableAllActorsMovement(Field_GetActorSystem(work->field));
    GCTX_HIDSetUsingTouch(FALSE);
    return event;
}

GameEventReturnCode EventShortcutChoicePopup_Callback(GameEvent *event, u32 *state, void *data) {
    ShortcutMenuWork *work = data;
    u32 item;
    u32 action;
    u32 invalid;
    u32 result;
    BOOL checkPerms;
    u32 mode;
    BOOL noRibbon;
    u32 id;
    PlayerActionPerms perms;
    PlayerActionPossibilities possibilities;
    HiddenEventArgs args;
    GameEvent *next;

    switch (*state) {
    case 0:
        *state = 1;
        break;
    case 1:
        ShortcutMenu_Open(SHORTCUT_MENU_START_CLOSED, work);
        *state = 2;
        break;
    case 2:
        ShortcutMenu_Start(work->app);
        *state = 3;
        break;
    case 3:
        ShortcutMenu_Main(work->app);
        if (!ShortcutMenu_IsWaitingInput(work->app)) {
            *state = 4;
        }
        break;
    case 4:
        ShortcutMenu_Main(work->app);
        result = ShortcutMenu_GetResult(work->app, &item);
        if (result == 1) {
            noRibbon = FALSE;
            work->blocked = 0;
            checkPerms = ShortcutMenu_GetActionFromKeyItem(item, &action, &invalid);
            mode = ShortcutMenu_SetKeyItemID(work->input, item);
            if (invalid == 0 && GameData_IsForceSeasonSync(GSYS_GetGameData(work->gameSystem)) == TRUE) {
                work->blocked = -1;
            } else if (checkPerms == TRUE) {
                PlayerActionPerms_Create(&perms, work->gameSystem, work->field);
                work->blocked = PlayerActionPerms_IsActionBlocked(&perms, action);
            }
            if (item == 11 && !CheckAnyRibbon(work)) {
                noRibbon = TRUE;
            }
            if (work->blocked) {
                *state = 12;
            } else if (noRibbon == TRUE) {
                *state = 14;
            } else {
                switch (mode) {
                case 0:
                    *state = 9;
                    break;
                case 1:
                    *state = 11;
                    break;
                }
            }
        } else if (result == 2) {
            *state = 5;
        }
        break;
    case 5:
        ShortcutMenu_RequestClose(work->app);
        *state = 6;
        break;
    case 6:
        ShortcutMenu_Main(work->app);
        if (!ShortcutMenu_IsWaitingInput(work->app)) {
            *state = 7;
        }
        break;
    case 7:
        if (ShortcutMenu_Close(work)) {
            GFL_HeapFree(work->input);
            EnableAllActorsMovement(Field_GetActorSystem(work->field));
            return GAMEEVENT_DONE;
        }
        break;
    case 8:
        ShortcutMenu_Open(SHORTCUT_MENU_START_OPEN, work);
        *state = 4;
        break;
    case 9:
        GameEvent_ChainNext(event, EventFieldAppCall_Create(work->input, work->code));
        *state = 10;
        break;
    case 10:
        if (work->input->eventType == 0) {
            if (work->done) {
                *state = 7;
            } else {
                *state = 8;
            }
        } else if (work->input->eventType == 1) {
            *state = 7;
        } else if (work->input->eventType == 3) {
            *state = 11;
        } else if (work->input->eventType == 2) {
            *state = 16;
        }
        break;
    case 11:
        if (ShortcutMenu_Close(work)) {
            id = work->input->eventId;
            GFL_HeapFree(work->input);
            work->input = NULL;
            GameEvent_ChainNext(event, CallFieldCommonEventFunc(id, work->gameSystem, work->field));
            *state = 13;
        }
        break;
    case 12:
        if (ShortcutMenu_Close(work)) {
            EventFieldItemUseBlock_Call(event, work->gameSystem, work->input->eventId, work->blocked);
            GFL_HeapFree(work->input);
            work->input = NULL;
            *state = 13;
        }
        break;
    case 13:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        return GAMEEVENT_DONE;
    case 14:
        if (ShortcutMenu_Close(work)) {
            GFL_HeapFree(work->input);
            work->input = NULL;
            GameEvent_ChainNext(event, EventFieldItemUseBlock_Create(work->gameSystem, 2, 0));
            *state = 15;
        }
        break;
    case 15:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        return GAMEEVENT_DONE;
    case 16:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        CalcPlayerActionPossibilities(work->field, &possibilities);
        HiddenEventArgs_Set(&args, work->input->partySlot, work->input->eventId, work->input->eventValue);
        next = CreateHMEvent(work->input->eventId, &args, &possibilities);
        if (next != NULL) {
            GameEvent_ChainNext(event, next);
        }
        *state = 7;
        break;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventShortcutCallDirect_Callback(GameEvent *event, u32 *state, void *data) {
    ShortcutMenuWork *work = data;
    u32 item;
    u32 action;
    u32 invalid;
    BOOL checkPerms;
    u32 mode;
    BOOL noRibbon;
    u32 id;
    PlayerActionPerms perms;
    PlayerActionPossibilities possibilities;
    HiddenEventArgs args;
    GameEvent *next;

    switch (*state) {
    case 0:
        item = ShortcutSave_GetRegistItem(
            SaveControl_GetShortcutSave(GameData_GetSaveControl(GSYS_GetGameData(work->gameSystem))), 0);
        noRibbon = FALSE;
        mode = ShortcutMenu_SetKeyItemID(work->input, item);
        checkPerms = ShortcutMenu_GetActionFromKeyItem(item, &action, &invalid);
        work->blocked = 0;
        if (invalid == 0 && GameData_IsForceSeasonSync(GSYS_GetGameData(work->gameSystem)) == TRUE) {
            work->blocked = -1;
        } else if (checkPerms == TRUE) {
            PlayerActionPerms_Create(&perms, work->gameSystem, work->field);
            work->blocked = PlayerActionPerms_IsActionBlocked(&perms, action);
        }
        if (item == 11 && !CheckAnyRibbon(work)) {
            noRibbon = TRUE;
        }
        if (work->blocked) {
            *state = 5;
        } else if (noRibbon == TRUE) {
            *state = 7;
        } else {
            switch (mode) {
            case 0:
                *state = 2;
                break;
            case 1:
                *state = 4;
                break;
            }
        }
        break;
    case 1:
        GFL_HeapFree(work->input);
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        return GAMEEVENT_DONE;
    case 2:
        if (ShortcutMenu_Close(work)) {
            GameEvent_ChainNext(event, EventFieldAppCall_Create(work->input, work->code));
            *state = 3;
        }
        break;
    case 3:
        if (work->input->eventType == 0) {
            *state = 1;
        } else if (work->input->eventType == 1) {
            *state = 1;
        } else if (work->input->eventType == 3) {
            *state = 4;
        } else if (work->input->eventType == 2) {
            *state = 9;
        }
        break;
    case 4:
        id = work->input->eventId;
        GFL_HeapFree(work->input);
        work->input = NULL;
        GameEvent_ChainNext(event, CallFieldCommonEventFunc(id, work->gameSystem, work->field));
        *state = 6;
        break;
    case 5:
        EventFieldItemUseBlock_Call(event, work->gameSystem, work->input->eventId, work->blocked);
        GFL_HeapFree(work->input);
        work->input = NULL;
        *state = 6;
        break;
    case 6:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        return GAMEEVENT_DONE;
    case 7:
        GFL_HeapFree(work->input);
        work->input = NULL;
        GameEvent_ChainNext(event, EventFieldItemUseBlock_Create(work->gameSystem, 2, 0));
        *state = 8;
        break;
    case 8:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        return GAMEEVENT_DONE;
    case 9:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        CalcPlayerActionPossibilities(work->field, &possibilities);
        HiddenEventArgs_Set(&args, work->input->partySlot, work->input->eventId, work->input->eventValue);
        next = CreateHMEvent(work->input->eventId, &args, &possibilities);
        if (next != NULL) {
            GameEvent_ChainNext(event, next);
        }
        *state = 1;
        break;
    }
    return GAMEEVENT_CONTINUE;
}

u32 ShortcutMenu_SetKeyItemID(FieldAppCallInput *input, u32 item) {
    switch (item) {
    case SHORTCUT_BICYCLE:
        input->eventId = FIELD_COMMON_EVENT_CYCLING;
        return 1;
    case SHORTCUT_TOWN_MAP:
        input->appId = FIELD_APP_TOWNMAP;
        return 0;
    case SHORTCUT_VS_RECORDER:
        input->appId = FIELD_APP_BATTLE_RECORDER;
        return 0;
    case SHORTCUT_PAL_PAD:
        input->appId = FIELD_APP_WIFI_NOTE;
        return 0;
    case SHORTCUT_SUPER_ROD:
        input->eventId = FIELD_COMMON_EVENT_FISHING;
        return 1;
    case SHORTCUT_DOWSING_MCHN:
        input->eventId = FIELD_COMMON_EVENT_DOWSING;
        return 1;
    case SHORTCUT_GRACIDEA:
        input->appId = FIELD_APP_POKELIST;
        input->appParam = ITEM_GRACIDEA;
        return 0;
    case SHORTCUT_DNA_SPLICERS_FUSE:
        input->appId = FIELD_APP_POKELIST;
        input->appParam = ITEM_DNA_SPLICERS_FUSE;
        return 0;
    case SHORTCUT_DNA_SPLICERS_SEPARATE:
        input->appId = FIELD_APP_POKELIST;
        input->appParam = ITEM_DNA_SPLICERS_SEPARATE;
        return 0;
    case SHORTCUT_PSTATUS_INFO:
        input->appId = FIELD_APP_POKESTATUS;
        input->appParam = PSTATUS_PAGE_INFO;
        return 0;
    case SHORTCUT_PSTATUS_SKILL:
        input->appId = FIELD_APP_POKESTATUS;
        input->appParam = PSTATUS_PAGE_SKILL;
        return 0;
    case SHORTCUT_PSTATUS_RIBBON:
        input->appId = FIELD_APP_POKESTATUS;
        input->appParam = PSTATUS_PAGE_RIBBON;
        return 0;
    case SHORTCUT_BAG_ITEMS:
        input->appId = FIELD_APP_BAG;
        input->appParam = BAG_POCKET_ITEMS;
        return 0;
    case SHORTCUT_BAG_MEDICINE:
        input->appId = FIELD_APP_BAG;
        input->appParam = BAG_POCKET_MEDICINE;
        return 0;
    case SHORTCUT_BAG_TMS_HMS:
        input->appId = FIELD_APP_BAG;
        input->appParam = BAG_POCKET_TMS_HMS;
        return 0;
    case SHORTCUT_BAG_BERRIES:
        input->appId = FIELD_APP_BAG;
        input->appParam = BAG_POCKET_BERRIES;
        return 0;
    case SHORTCUT_BAG_KEY_ITEMS:
        input->appId = FIELD_APP_BAG;
        input->appParam = BAG_POCKET_KEY_ITEMS;
        return 0;
    case SHORTCUT_BAG_FREE_SPACE:
        input->appId = FIELD_APP_BAG;
        input->appParam = BAG_POCKET_FREE_SPACE;
        return 0;
    case SHORTCUT_POKEDEX_LIST:
        input->appId = FIELD_APP_POKEDEX;
        input->appParam = POKEDEX_START_LIST;
        return 0;
    case SHORTCUT_POKEDEX_SEARCH:
        input->appId = FIELD_APP_POKEDEX;
        input->appParam = POKEDEX_START_SEARCH;
        return 0;
    case SHORTCUT_POKEDEX_INFO:
        input->appId = FIELD_APP_POKEDEX;
        input->appParam = POKEDEX_START_INFO;
        return 0;
    case SHORTCUT_POKEDEX_MAP:
        input->appId = FIELD_APP_POKEDEX;
        input->appParam = POKEDEX_START_MAP;
        return 0;
    case SHORTCUT_POKEDEX_VOICE:
        input->appId = FIELD_APP_POKEDEX;
        input->appParam = POKEDEX_START_VOICE;
        return 0;
    case SHORTCUT_POKEDEX_FORM:
        input->appId = FIELD_APP_POKEDEX;
        input->appParam = POKEDEX_START_FORM;
        return 0;
    case SHORTCUT_POKEDEX_AWARDS:
        input->appId = FIELD_APP_POKEDEX;
        input->appParam = POKEDEX_START_AWARDS;
        return 0;
    case SHORTCUT_POKEDEX_HABITAT:
        input->appId = FIELD_APP_POKEDEX;
        input->appParam = POKEDEX_START_HABITATS;
        return 0;
    case SHORTCUT_TRAINER_CARD_FRONT:
        input->appId = FIELD_APP_TRAINER_CARD;
        input->appParam = TRCARD_START_FRONT;
        return 0;
    case SHORTCUT_TRAINER_CARD_BACK:
        input->appId = FIELD_APP_TRAINER_CARD;
        input->appParam = TRCARD_START_BACK;
        return 0;
    case SHORTCUT_TRAINER_CARD_BADGES:
        input->appId = FIELD_APP_TRAINER_CARD;
        input->appParam = TRCARD_START_BADGES;
        return 0;
    case SHORTCUT_CONFIG_PANEL:
        input->appId = FIELD_APP_CONFIG;
        return 0;
    case SHORTCUT_XTRANSCEIVER:
        input->appId = FIELD_APP_XTRANSCEIVER;
        return 0;
    case SHORTCUT_MEDAL_BOX:
        input->appId = FIELD_APP_MEDALBOX;
        return 0;
    case SHORTCUT_POKELIST:
        input->appId = FIELD_APP_POKELIST;
        input->appParam = -1;
        return 0;
    case SHORTCUT_REVEAL_GLASS:
        input->appId = FIELD_APP_POKELIST;
        input->appParam = ITEM_REVEAL_GLASS;
        return 0;
    default:
        input->appId = FIELD_APP_POKEDEX;
        return 0;
    }
}

BOOL ShortcutMenu_GetActionFromKeyItem(s32 item, u32 *action, u32 *invalid) {
    *invalid = 0;
    switch (item) {
    case SHORTCUT_BICYCLE:
        *action = 0;
        return TRUE;
    case SHORTCUT_TOWN_MAP:
        *action = 1;
        return TRUE;
    case SHORTCUT_VS_RECORDER:
        *action = 6;
        return TRUE;
    case SHORTCUT_PAL_PAD:
        *action = 2;
        return TRUE;
    case SHORTCUT_SUPER_ROD:
        *action = 5;
        return TRUE;
    case SHORTCUT_DOWSING_MCHN:
        *action = 9;
        return TRUE;
    case SHORTCUT_XTRANSCEIVER:
        *action = 10;
        return TRUE;
    case SHORTCUT_MEDAL_BOX:
        *action = 11;
        return TRUE;
    case SHORTCUT_GRACIDEA:
    case SHORTCUT_DNA_SPLICERS_FUSE:
    case SHORTCUT_DNA_SPLICERS_SEPARATE:
    case SHORTCUT_REVEAL_GLASS:
        return FALSE;
    default:
        *invalid = 1;
        return FALSE;
    }
}

void ShortcutMenu_Open(u32 arg0, ShortcutMenuWork *work) {
    GameData *gameData;
    ShortcutMenuPos *listPos;
    FieldMsgBG *msgBGSys;

    if (work->app == NULL && work->done == 0) {
        GFL_BGSysSetBGEnabled(1, 0);
        msgBGSys = Field_GetMsgBGSys(work->field);
        if (msgBGSys != NULL) {
            MsgBGPrinter_ReleaseBg(msgBGSys);
        }
        gameData = GSYS_GetGameData(work->gameSystem);
        listPos = GameData_GetShortcutMenuPos(gameData);
        work->app = ShortcutMenu_Create(gameData, arg0, listPos, HEAPID_FIELD_PARTICLE, HEAPID_FIELDMAP);
    }
}

BOOL ShortcutMenu_Close(ShortcutMenuWork *work) {
    if (work->done == 0) {
        switch (work->state) {
        case 0:
            if (work->app) {
                GFL_BGSysSetBGEnabled(1, 0);
                ShortcutMenu_Delete(work->app);
                work->app = NULL;
                work->state = 1;
            } else {
                work->state = 2;
            }
            break;
        case 1:
            MsgBGPrinter_SetupBgSingle(Field_GetMsgBGSys(work->field));
            work->state = 2;
            break;
        case 2:
            work->state = 0;
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

BOOL ShortcutMenu_CanRetry(FieldAppCallInput *input, void *arg) {
    ShortcutMenuWork *work = arg;
    GameData *gameData;
    SaveControl *save;
    ShortcutSave *shortcut;

    gameData = GSYS_GetGameData(work->gameSystem);
    save = GameData_GetSaveControl(gameData);
    shortcut = SaveControl_GetShortcutSave(save);
    switch (work->state) {
    case 0:
        if (ShortcutSave_GetShortcutCount(shortcut) == 0) {
            work->done = 1;
            work->state = 2;
        } else if (input->eventType == 1 || input->eventType == 3 || input->eventType == 2 || input->eventType == 5) {
            work->state = 2;
        } else {
            ShortcutMenu_Open(SHORTCUT_MENU_START_OPEN, work);
            work->state = 1;
        }
        break;
    case 1:
        if (ShortcutMenu_RunPrint(work->app)) {
            work->state = 2;
        }
        break;
    case 2:
        work->state = 0;
        return TRUE;
    }
    return FALSE;
}

BOOL ShortcutMenu_CanLeaveField(FieldAppCallInput *input, void *arg) {
    return ShortcutMenu_Close(arg);
}

void EventFieldItemUseBlock_Call(GameEvent *parent, GameSystem *gsys, u32 action, u32 kind) {
    u32 param = 0;
    u32 item = 0;
    GameEvent *event;

    if (kind == 1) {
        param = 3;
        switch (action) {
        case 0:
            item = 0x1c2;
            break;
        case 2:
            item = 0x4e;
            break;
        case 4:
            item = 0x1bf;
            break;
        }
    } else if (action == 0) {
        GameData *gameData = GSYS_GetGameData(gsys);
        PlayerState *state = GameData_GetPlayerState(gameData);

        if (FieldPlayerState_GetExState(state) == 1) {
            param = 1;
        }
    }
    event = EventFieldItemUseBlock_Create(gsys, param, item);
    GameEvent_ChainNext(parent, event);
}

GameEvent *EventFieldItemUseBlock_Create(GameSystem *gsys, u32 arg1, u32 arg2) {
    GameEvent *event = EventScriptCall_Create(gsys, 0x7d6, NULL, 0x15);
    ScriptWork *work = EventScriptCall_GetWork(event);

    ScriptWork_SetParams(work, arg1, arg2, 0, 0);
    return event;
}

BOOL CheckAnyRibbon(ShortcutMenuWork *work) {
    GameData *gameData = GSYS_GetGameData(work->gameSystem);
    PokeParty *party = GameData_GetParty(gameData);
    int i;

    for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL) != 0 && PokeParty_CheckAnyRibbon(pkm) != 0) {
            return TRUE;
        }
    }
    return FALSE;
}
