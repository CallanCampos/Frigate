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

// The menu's event, for two or more shortcuts: the menu slides in and is chosen from, then the app, the common event
// or the hidden move it asks for runs, or a message says why it can't, and the menu comes back after an app
enum {
    SHORTCUT_MENU_EVENT_STATE_START,
    SHORTCUT_MENU_EVENT_STATE_OPEN,
    SHORTCUT_MENU_EVENT_STATE_SLIDE_IN,
    SHORTCUT_MENU_EVENT_STATE_WAIT_SLIDE_IN,
    SHORTCUT_MENU_EVENT_STATE_SELECT,
    SHORTCUT_MENU_EVENT_STATE_SLIDE_OUT,
    SHORTCUT_MENU_EVENT_STATE_WAIT_SLIDE_OUT,
    SHORTCUT_MENU_EVENT_STATE_CLOSE,
    SHORTCUT_MENU_EVENT_STATE_REOPEN,
    SHORTCUT_MENU_EVENT_STATE_APP_CALL,
    SHORTCUT_MENU_EVENT_STATE_APP_RESULT,
    SHORTCUT_MENU_EVENT_STATE_COMMON_EVENT,
    SHORTCUT_MENU_EVENT_STATE_ITEM_USE_BLOCKED,
    SHORTCUT_MENU_EVENT_STATE_END,
    SHORTCUT_MENU_EVENT_STATE_NO_RIBBON,
    SHORTCUT_MENU_EVENT_STATE_NO_RIBBON_END,
    SHORTCUT_MENU_EVENT_STATE_HM_EVENT,
};

// The event of a single shortcut, which calls it with no menu
enum {
    SHORTCUT_CALL_DIRECT_STATE_START,
    SHORTCUT_CALL_DIRECT_STATE_CLOSE,
    SHORTCUT_CALL_DIRECT_STATE_APP_CALL,
    SHORTCUT_CALL_DIRECT_STATE_APP_RESULT,
    SHORTCUT_CALL_DIRECT_STATE_COMMON_EVENT,
    SHORTCUT_CALL_DIRECT_STATE_ITEM_USE_BLOCKED,
    SHORTCUT_CALL_DIRECT_STATE_END,
    SHORTCUT_CALL_DIRECT_STATE_NO_RIBBON,
    SHORTCUT_CALL_DIRECT_STATE_NO_RIBBON_END,
    SHORTCUT_CALL_DIRECT_STATE_HM_EVENT,
};

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
    u32 callKind;
    BOOL noRibbon;
    u32 id;
    PlayerActionPerms perms;
    PlayerActionPossibilities possibilities;
    HiddenEventArgs args;
    GameEvent *next;

    switch (*state) {
    case SHORTCUT_MENU_EVENT_STATE_START:
        *state = SHORTCUT_MENU_EVENT_STATE_OPEN;
        break;
    case SHORTCUT_MENU_EVENT_STATE_OPEN:
        ShortcutMenu_Open(SHORTCUT_MENU_START_CLOSED, work);
        *state = SHORTCUT_MENU_EVENT_STATE_SLIDE_IN;
        break;
    case SHORTCUT_MENU_EVENT_STATE_SLIDE_IN:
        ShortcutMenu_Start(work->app);
        *state = SHORTCUT_MENU_EVENT_STATE_WAIT_SLIDE_IN;
        break;
    case SHORTCUT_MENU_EVENT_STATE_WAIT_SLIDE_IN:
        ShortcutMenu_Main(work->app);
        if (!ShortcutMenu_IsWaitingInput(work->app)) {
            *state = SHORTCUT_MENU_EVENT_STATE_SELECT;
        }
        break;
    case SHORTCUT_MENU_EVENT_STATE_SELECT:
        ShortcutMenu_Main(work->app);
        result = ShortcutMenu_GetResult(work->app, &item);
        if (result == SHORTCUT_MENU_RESULT_SELECTED) {
            noRibbon = FALSE;
            work->blocked = FIELD_ACTION_ALLOWED;
            checkPerms = ShortcutMenu_GetActionFromKeyItem(item, &action, &invalid);
            callKind = ShortcutMenu_SetKeyItemID(work->input, item);
            if (invalid == 0 && GameData_IsForceSeasonSync(GSYS_GetGameData(work->gameSystem)) == TRUE) {
                work->blocked = FIELD_ACTION_BLOCKED;
            } else if (checkPerms == TRUE) {
                PlayerActionPerms_Create(&perms, work->gameSystem, work->field);
                work->blocked = PlayerActionPerms_IsActionBlocked(&perms, action);
            }
            if (item == SHORTCUT_PSTATUS_RIBBON && !CheckAnyRibbon(work)) {
                noRibbon = TRUE;
            }
            if (work->blocked) {
                *state = SHORTCUT_MENU_EVENT_STATE_ITEM_USE_BLOCKED;
            } else if (noRibbon == TRUE) {
                *state = SHORTCUT_MENU_EVENT_STATE_NO_RIBBON;
            } else {
                switch (callKind) {
                case SHORTCUT_CALL_APP:
                    *state = SHORTCUT_MENU_EVENT_STATE_APP_CALL;
                    break;
                case SHORTCUT_CALL_COMMON_EVENT:
                    *state = SHORTCUT_MENU_EVENT_STATE_COMMON_EVENT;
                    break;
                }
            }
        } else if (result == SHORTCUT_MENU_RESULT_CANCELLED) {
            *state = SHORTCUT_MENU_EVENT_STATE_SLIDE_OUT;
        }
        break;
    case SHORTCUT_MENU_EVENT_STATE_SLIDE_OUT:
        ShortcutMenu_RequestClose(work->app);
        *state = SHORTCUT_MENU_EVENT_STATE_WAIT_SLIDE_OUT;
        break;
    case SHORTCUT_MENU_EVENT_STATE_WAIT_SLIDE_OUT:
        ShortcutMenu_Main(work->app);
        if (!ShortcutMenu_IsWaitingInput(work->app)) {
            *state = SHORTCUT_MENU_EVENT_STATE_CLOSE;
        }
        break;
    case SHORTCUT_MENU_EVENT_STATE_CLOSE:
        if (ShortcutMenu_Close(work)) {
            GFL_HeapFree(work->input);
            EnableAllActorsMovement(Field_GetActorSystem(work->field));
            return GAMEEVENT_DONE;
        }
        break;
    case SHORTCUT_MENU_EVENT_STATE_REOPEN:
        ShortcutMenu_Open(SHORTCUT_MENU_START_OPEN, work);
        *state = SHORTCUT_MENU_EVENT_STATE_SELECT;
        break;
    case SHORTCUT_MENU_EVENT_STATE_APP_CALL:
        GameEvent_ChainNext(event, EventFieldAppCall_Create(work->input, work->code));
        *state = SHORTCUT_MENU_EVENT_STATE_APP_RESULT;
        break;
    case SHORTCUT_MENU_EVENT_STATE_APP_RESULT:
        if (work->input->eventType == FIELD_APP_RESULT_RETURN) {
            if (work->done) {
                *state = SHORTCUT_MENU_EVENT_STATE_CLOSE;
            } else {
                *state = SHORTCUT_MENU_EVENT_STATE_REOPEN;
            }
        } else if (work->input->eventType == FIELD_APP_RESULT_EXIT) {
            *state = SHORTCUT_MENU_EVENT_STATE_CLOSE;
        } else if (work->input->eventType == FIELD_APP_RESULT_COMMON_EVENT) {
            *state = SHORTCUT_MENU_EVENT_STATE_COMMON_EVENT;
        } else if (work->input->eventType == FIELD_APP_RESULT_HM_EVENT) {
            *state = SHORTCUT_MENU_EVENT_STATE_HM_EVENT;
        }
        break;
    case SHORTCUT_MENU_EVENT_STATE_COMMON_EVENT:
        if (ShortcutMenu_Close(work)) {
            id = work->input->eventId;
            GFL_HeapFree(work->input);
            work->input = NULL;
            GameEvent_ChainNext(event, CallFieldCommonEventFunc(id, work->gameSystem, work->field));
            *state = SHORTCUT_MENU_EVENT_STATE_END;
        }
        break;
    case SHORTCUT_MENU_EVENT_STATE_ITEM_USE_BLOCKED:
        if (ShortcutMenu_Close(work)) {
            EventFieldItemUseBlock_Call(event, work->gameSystem, work->input->eventId, work->blocked);
            GFL_HeapFree(work->input);
            work->input = NULL;
            *state = SHORTCUT_MENU_EVENT_STATE_END;
        }
        break;
    case SHORTCUT_MENU_EVENT_STATE_END:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        return GAMEEVENT_DONE;
    case SHORTCUT_MENU_EVENT_STATE_NO_RIBBON:
        if (ShortcutMenu_Close(work)) {
            GFL_HeapFree(work->input);
            work->input = NULL;
            GameEvent_ChainNext(
                event, EventFieldItemUseBlock_Create(work->gameSystem, ITEM_USE_BLOCK_MSG_NO_RIBBON, ITEM_NONE));
            *state = SHORTCUT_MENU_EVENT_STATE_NO_RIBBON_END;
        }
        break;
    case SHORTCUT_MENU_EVENT_STATE_NO_RIBBON_END:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        return GAMEEVENT_DONE;
    case SHORTCUT_MENU_EVENT_STATE_HM_EVENT:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        CalcPlayerActionPossibilities(work->field, &possibilities);
        HiddenEventArgs_Set(&args, work->input->partySlot, work->input->eventId, work->input->eventValue);
        next = CreateHMEvent(work->input->eventId, &args, &possibilities);
        if (next != NULL) {
            GameEvent_ChainNext(event, next);
        }
        *state = SHORTCUT_MENU_EVENT_STATE_CLOSE;
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
    u32 callKind;
    BOOL noRibbon;
    u32 id;
    PlayerActionPerms perms;
    PlayerActionPossibilities possibilities;
    HiddenEventArgs args;
    GameEvent *next;

    switch (*state) {
    case SHORTCUT_CALL_DIRECT_STATE_START:
        item = ShortcutSave_GetRegistItem(
            SaveControl_GetShortcutSave(GameData_GetSaveControl(GSYS_GetGameData(work->gameSystem))), 0);
        noRibbon = FALSE;
        callKind = ShortcutMenu_SetKeyItemID(work->input, item);
        checkPerms = ShortcutMenu_GetActionFromKeyItem(item, &action, &invalid);
        work->blocked = FIELD_ACTION_ALLOWED;
        if (invalid == 0 && GameData_IsForceSeasonSync(GSYS_GetGameData(work->gameSystem)) == TRUE) {
            work->blocked = FIELD_ACTION_BLOCKED;
        } else if (checkPerms == TRUE) {
            PlayerActionPerms_Create(&perms, work->gameSystem, work->field);
            work->blocked = PlayerActionPerms_IsActionBlocked(&perms, action);
        }
        if (item == SHORTCUT_PSTATUS_RIBBON && !CheckAnyRibbon(work)) {
            noRibbon = TRUE;
        }
        if (work->blocked) {
            *state = SHORTCUT_CALL_DIRECT_STATE_ITEM_USE_BLOCKED;
        } else if (noRibbon == TRUE) {
            *state = SHORTCUT_CALL_DIRECT_STATE_NO_RIBBON;
        } else {
            switch (callKind) {
            case SHORTCUT_CALL_APP:
                *state = SHORTCUT_CALL_DIRECT_STATE_APP_CALL;
                break;
            case SHORTCUT_CALL_COMMON_EVENT:
                *state = SHORTCUT_CALL_DIRECT_STATE_COMMON_EVENT;
                break;
            }
        }
        break;
    case SHORTCUT_CALL_DIRECT_STATE_CLOSE:
        GFL_HeapFree(work->input);
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        return GAMEEVENT_DONE;
    case SHORTCUT_CALL_DIRECT_STATE_APP_CALL:
        if (ShortcutMenu_Close(work)) {
            GameEvent_ChainNext(event, EventFieldAppCall_Create(work->input, work->code));
            *state = SHORTCUT_CALL_DIRECT_STATE_APP_RESULT;
        }
        break;
    case SHORTCUT_CALL_DIRECT_STATE_APP_RESULT:
        if (work->input->eventType == FIELD_APP_RESULT_RETURN) {
            *state = SHORTCUT_CALL_DIRECT_STATE_CLOSE;
        } else if (work->input->eventType == FIELD_APP_RESULT_EXIT) {
            *state = SHORTCUT_CALL_DIRECT_STATE_CLOSE;
        } else if (work->input->eventType == FIELD_APP_RESULT_COMMON_EVENT) {
            *state = SHORTCUT_CALL_DIRECT_STATE_COMMON_EVENT;
        } else if (work->input->eventType == FIELD_APP_RESULT_HM_EVENT) {
            *state = SHORTCUT_CALL_DIRECT_STATE_HM_EVENT;
        }
        break;
    case SHORTCUT_CALL_DIRECT_STATE_COMMON_EVENT:
        id = work->input->eventId;
        GFL_HeapFree(work->input);
        work->input = NULL;
        GameEvent_ChainNext(event, CallFieldCommonEventFunc(id, work->gameSystem, work->field));
        *state = SHORTCUT_CALL_DIRECT_STATE_END;
        break;
    case SHORTCUT_CALL_DIRECT_STATE_ITEM_USE_BLOCKED:
        EventFieldItemUseBlock_Call(event, work->gameSystem, work->input->eventId, work->blocked);
        GFL_HeapFree(work->input);
        work->input = NULL;
        *state = SHORTCUT_CALL_DIRECT_STATE_END;
        break;
    case SHORTCUT_CALL_DIRECT_STATE_END:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        return GAMEEVENT_DONE;
    case SHORTCUT_CALL_DIRECT_STATE_NO_RIBBON:
        GFL_HeapFree(work->input);
        work->input = NULL;
        GameEvent_ChainNext(event,
                            EventFieldItemUseBlock_Create(work->gameSystem, ITEM_USE_BLOCK_MSG_NO_RIBBON, ITEM_NONE));
        *state = SHORTCUT_CALL_DIRECT_STATE_NO_RIBBON_END;
        break;
    case SHORTCUT_CALL_DIRECT_STATE_NO_RIBBON_END:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        return GAMEEVENT_DONE;
    case SHORTCUT_CALL_DIRECT_STATE_HM_EVENT:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        CalcPlayerActionPossibilities(work->field, &possibilities);
        HiddenEventArgs_Set(&args, work->input->partySlot, work->input->eventId, work->input->eventValue);
        next = CreateHMEvent(work->input->eventId, &args, &possibilities);
        if (next != NULL) {
            GameEvent_ChainNext(event, next);
        }
        *state = SHORTCUT_CALL_DIRECT_STATE_CLOSE;
        break;
    }
    return GAMEEVENT_CONTINUE;
}

u32 ShortcutMenu_SetKeyItemID(FieldAppCallInput *input, u32 item) {
    switch (item) {
    case SHORTCUT_BICYCLE:
        input->eventId = FIELD_COMMON_EVENT_CYCLING;
        return SHORTCUT_CALL_COMMON_EVENT;
    case SHORTCUT_TOWN_MAP:
        input->appId = FIELD_APP_TOWNMAP;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_VS_RECORDER:
        input->appId = FIELD_APP_BATTLE_RECORDER;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_PAL_PAD:
        input->appId = FIELD_APP_WIFI_NOTE;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_SUPER_ROD:
        input->eventId = FIELD_COMMON_EVENT_FISHING;
        return SHORTCUT_CALL_COMMON_EVENT;
    case SHORTCUT_DOWSING_MCHN:
        input->eventId = FIELD_COMMON_EVENT_DOWSING;
        return SHORTCUT_CALL_COMMON_EVENT;
    case SHORTCUT_GRACIDEA:
        input->appId = FIELD_APP_POKELIST;
        input->appParam = ITEM_GRACIDEA;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_DNA_SPLICERS_FUSE:
        input->appId = FIELD_APP_POKELIST;
        input->appParam = ITEM_DNA_SPLICERS_FUSE;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_DNA_SPLICERS_SEPARATE:
        input->appId = FIELD_APP_POKELIST;
        input->appParam = ITEM_DNA_SPLICERS_SEPARATE;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_PSTATUS_INFO:
        input->appId = FIELD_APP_POKESTATUS;
        input->appParam = PSTATUS_PAGE_INFO;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_PSTATUS_SKILL:
        input->appId = FIELD_APP_POKESTATUS;
        input->appParam = PSTATUS_PAGE_SKILL;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_PSTATUS_RIBBON:
        input->appId = FIELD_APP_POKESTATUS;
        input->appParam = PSTATUS_PAGE_RIBBON;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_BAG_ITEMS:
        input->appId = FIELD_APP_BAG;
        input->appParam = BAG_POCKET_ITEMS;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_BAG_MEDICINE:
        input->appId = FIELD_APP_BAG;
        input->appParam = BAG_POCKET_MEDICINE;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_BAG_TMS_HMS:
        input->appId = FIELD_APP_BAG;
        input->appParam = BAG_POCKET_TMS_HMS;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_BAG_BERRIES:
        input->appId = FIELD_APP_BAG;
        input->appParam = BAG_POCKET_BERRIES;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_BAG_KEY_ITEMS:
        input->appId = FIELD_APP_BAG;
        input->appParam = BAG_POCKET_KEY_ITEMS;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_BAG_FREE_SPACE:
        input->appId = FIELD_APP_BAG;
        input->appParam = BAG_POCKET_FREE_SPACE;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_POKEDEX_LIST:
        input->appId = FIELD_APP_POKEDEX;
        input->appParam = POKEDEX_START_LIST;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_POKEDEX_SEARCH:
        input->appId = FIELD_APP_POKEDEX;
        input->appParam = POKEDEX_START_SEARCH;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_POKEDEX_INFO:
        input->appId = FIELD_APP_POKEDEX;
        input->appParam = POKEDEX_START_INFO;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_POKEDEX_MAP:
        input->appId = FIELD_APP_POKEDEX;
        input->appParam = POKEDEX_START_MAP;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_POKEDEX_VOICE:
        input->appId = FIELD_APP_POKEDEX;
        input->appParam = POKEDEX_START_VOICE;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_POKEDEX_FORM:
        input->appId = FIELD_APP_POKEDEX;
        input->appParam = POKEDEX_START_FORM;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_POKEDEX_AWARDS:
        input->appId = FIELD_APP_POKEDEX;
        input->appParam = POKEDEX_START_AWARDS;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_POKEDEX_HABITAT:
        input->appId = FIELD_APP_POKEDEX;
        input->appParam = POKEDEX_START_HABITATS;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_TRAINER_CARD_FRONT:
        input->appId = FIELD_APP_TRAINER_CARD;
        input->appParam = TRCARD_START_FRONT;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_TRAINER_CARD_BACK:
        input->appId = FIELD_APP_TRAINER_CARD;
        input->appParam = TRCARD_START_BACK;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_TRAINER_CARD_BADGES:
        input->appId = FIELD_APP_TRAINER_CARD;
        input->appParam = TRCARD_START_BADGES;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_CONFIG_PANEL:
        input->appId = FIELD_APP_CONFIG;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_XTRANSCEIVER:
        input->appId = FIELD_APP_XTRANSCEIVER;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_MEDAL_BOX:
        input->appId = FIELD_APP_MEDALBOX;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_POKELIST:
        input->appId = FIELD_APP_POKELIST;
        input->appParam = -1;
        return SHORTCUT_CALL_APP;
    case SHORTCUT_REVEAL_GLASS:
        input->appId = FIELD_APP_POKELIST;
        input->appParam = ITEM_REVEAL_GLASS;
        return SHORTCUT_CALL_APP;
    default:
        input->appId = FIELD_APP_POKEDEX;
        return SHORTCUT_CALL_APP;
    }
}

BOOL ShortcutMenu_GetActionFromKeyItem(s32 item, u32 *action, u32 *invalid) {
    *invalid = 0;
    switch (item) {
    case SHORTCUT_BICYCLE:
        *action = PLAYER_ACTION_BICYCLE;
        return TRUE;
    case SHORTCUT_TOWN_MAP:
        *action = PLAYER_ACTION_TOWN_MAP;
        return TRUE;
    case SHORTCUT_VS_RECORDER:
        *action = PLAYER_ACTION_VS_RECORDER;
        return TRUE;
    case SHORTCUT_PAL_PAD:
        *action = PLAYER_ACTION_PAL_PAD;
        return TRUE;
    case SHORTCUT_SUPER_ROD:
        *action = PLAYER_ACTION_FISHING;
        return TRUE;
    case SHORTCUT_DOWSING_MCHN:
        *action = PLAYER_ACTION_DOWSING;
        return TRUE;
    case SHORTCUT_XTRANSCEIVER:
        *action = PLAYER_ACTION_XTRANSCEIVER;
        return TRUE;
    case SHORTCUT_MEDAL_BOX:
        *action = PLAYER_ACTION_MEDAL_BOX;
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

void ShortcutMenu_Open(u32 mode, ShortcutMenuWork *work) {
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
        work->app = ShortcutMenu_Create(gameData, mode, listPos, HEAPID_FIELD_PARTICLE, HEAPID_FIELDMAP);
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
        } else if (input->eventType == FIELD_APP_RESULT_EXIT || input->eventType == FIELD_APP_RESULT_COMMON_EVENT ||
                   input->eventType == FIELD_APP_RESULT_HM_EVENT || input->eventType == FIELD_APP_RESULT_SCRIPT) {
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

void EventFieldItemUseBlock_Call(GameEvent *parent, GameSystem *gsys, u32 eventId, u32 blocked) {
    u32 message = ITEM_USE_BLOCK_MSG_CANT_USE;
    u32 item = ITEM_NONE;
    GameEvent *event;

    if (blocked == FIELD_ACTION_ALLOWED_PAIRED) {
        message = ITEM_USE_BLOCK_MSG_PAIRED;
        switch (eventId) {
        case FIELD_COMMON_EVENT_CYCLING:
            item = ITEM_BICYCLE;
            break;
        case FIELD_COMMON_EVENT_ESCAPE_ROPE:
            item = ITEM_ESCAPE_ROPE;
            break;
        case FIELD_COMMON_EVENT_FISHING:
            item = ITEM_SUPER_ROD;
            break;
        }
    } else if (eventId == FIELD_COMMON_EVENT_CYCLING) {
        GameData *gameData = GSYS_GetGameData(gsys);
        PlayerState *state = GameData_GetPlayerState(gameData);

        if (FieldPlayerState_GetExState(state) == FLD_PLAYER_EXSTATE_CYCLING) {
            message = ITEM_USE_BLOCK_MSG_CANT_DISMOUNT;
        }
    }
    event = EventFieldItemUseBlock_Create(gsys, message, item);
    GameEvent_ChainNext(parent, event);
}

GameEvent *EventFieldItemUseBlock_Create(GameSystem *gsys, u32 message, u32 item) {
    GameEvent *event = EventScriptCall_Create(gsys, 0x7d6, NULL, HEAPID_FIELDMAP);
    ScriptWork *work = EventScriptCall_GetWork(event);

    ScriptWork_SetParams(work, message, item, 0, 0);
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
