#include "types.h"
#include "constants/pokemon.h"
#include "field/field.h"
#include "field/field_menu.h"
#include "field/field_script.h"
#include "field/field_script_event.h"
#include "field/field_actor.h"
#include "field/hidden_event.h"
#include "field/itemuse_event.h"
#include "field/player_action.h"
#include "field/item_use_block.h"
#include "field/player_state.h"
#include "field/shortcut_menu.h"
#include "field/subscreen.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "save/shortcut.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

u32 func_ov012_0215aa68(u32 index) {
    return data_ov012_0216cb74[index];
}

BOOL func_ov012_0215aa74(FieldAppCallInput *input, void *arg) {
    FieldMenuWork *menu = arg;
    FieldSubscreen *subscreen = Field_GetSubscreen(menu->field);

    if (input->eventType == 0) {
        func_ov036_0219886c(subscreen, menu->unk10);
    }
    return TRUE;
}

BOOL func_ov012_0215aa90(FieldAppCallInput *input, void *arg) {
    return TRUE;
}

BOOL func_ov012_0215aa94(FieldAppCallInput *input, void *arg) {
    FieldMenuWork *menu = arg;
    GameData *gameData = GSYS_GetGameData(menu->gameSystem);

    switch (input->eventType) {
    case 4:
        GameData_SetLastSubscreen(gameData, 6);
        break;
    case 0:
        GameData_SetLastSubscreen(gameData, 1);
        break;
    case 5:
        GameData_SetLastSubscreen(gameData, 0);
        break;
    case 1:
    case 2:
    case 3:
        GameData_SetLastSubscreen(gameData, menu->screenId);
        break;
    }
    return TRUE;
}

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
    sys_memset(work, 0, sizeof(ShortcutMenuWork));
    work->gameSystem = gsys;
    work->event = event;
    work->field = field;
    work->code = code;
    work->done = 0;
    work->input = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(FieldAppCallInput), FALSE, "event_shortcut_menu.c", 0xcb);
    sys_memset(work->input, 0, sizeof(FieldAppCallInput));
    work->input->gameSystem = work->gameSystem;
    work->input->field = work->field;
    work->input->parent = event;
    work->input->appParam = -1;
    if (ShortcutSave_GetShortcutCount(shortcut) != 1) {
        work->input->canRetry = func_ov012_0215b32c;
        work->input->callback1 = func_ov012_0215b39c;
        work->input->arg = work;
    }
    DisableAllActorsMovement(Field_GetActorSystem(work->field));
    func_0203d564(FALSE);
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
        func_ov012_0215b284(0, work);
        *state = 2;
        break;
    case 2:
        func_ov036_021bf1ac(work->app);
        *state = 3;
        break;
    case 3:
        func_ov036_021bf088(work->app);
        if (!func_ov036_021bf1e4(work->app)) {
            *state = 4;
        }
        break;
    case 4:
        func_ov036_021bf088(work->app);
        result = func_ov036_021bf1f8(work->app, &item);
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
        func_ov036_021bf1d0(work->app);
        *state = 6;
        break;
    case 6:
        func_ov036_021bf088(work->app);
        if (!func_ov036_021bf1e4(work->app)) {
            *state = 7;
        }
        break;
    case 7:
        if (func_ov012_0215b2d0(work)) {
            GFL_HeapFree(work->input);
            EnableAllActorsMovement(Field_GetActorSystem(work->field));
            return GAMEEVENT_DONE;
        }
        break;
    case 8:
        func_ov012_0215b284(1, work);
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
        if (func_ov012_0215b2d0(work)) {
            id = work->input->eventId;
            GFL_HeapFree(work->input);
            work->input = NULL;
            GameEvent_ChainNext(event, CallFieldCommonEventFunc(id, work->gameSystem, work->field));
            *state = 13;
        }
        break;
    case 12:
        if (func_ov012_0215b2d0(work)) {
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
        if (func_ov012_0215b2d0(work)) {
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
        func_ov012_02159418(&args, work->input->partySlot, work->input->eventId, work->input->eventValue);
        next = CreateHidenEvent(work->input->eventId, &args, &possibilities);
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
        item = ShortcutSave_GetRegistItem(SaveControl_GetShortcutSave(GameData_GetSaveControl(GSYS_GetGameData(work->gameSystem))), 0);
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
        if (func_ov012_0215b2d0(work)) {
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
        func_ov012_02159418(&args, work->input->partySlot, work->input->eventId, work->input->eventValue);
        next = CreateHidenEvent(work->input->eventId, &args, &possibilities);
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
    case 0:
        input->eventId = 0;
        return 1;
    case 1:
        input->appId = 8;
        return 0;
    case 2:
        input->appId = 12;
        return 0;
    case 3:
        input->appId = 9;
        return 0;
    case 4:
        input->eventId = 4;
        return 1;
    case 5:
        input->eventId = 5;
        return 1;
    case 6:
        input->appId = 0;
        input->appParam = 466;
        return 0;
    case 7:
        input->appId = 0;
        input->appParam = 628;
        return 0;
    case 8:
        input->appId = 0;
        input->appParam = 629;
        return 0;
    case 9:
        input->appId = 7;
        input->appParam = 0;
        return 0;
    case 10:
        input->appId = 7;
        input->appParam = 1;
        return 0;
    case 11:
        input->appId = 7;
        input->appParam = 2;
        return 0;
    case 12:
        input->appId = 2;
        input->appParam = 0;
        return 0;
    case 13:
        input->appId = 2;
        input->appParam = 1;
        return 0;
    case 14:
        input->appId = 2;
        input->appParam = 2;
        return 0;
    case 15:
        input->appId = 2;
        input->appParam = 3;
        return 0;
    case 16:
        input->appId = 2;
        input->appParam = 4;
        return 0;
    case 17:
        input->appId = 2;
        input->appParam = 5;
        return 0;
    case 18:
        input->appId = 1;
        input->appParam = 1;
        return 0;
    case 19:
        input->appId = 1;
        input->appParam = 6;
        return 0;
    case 20:
        input->appId = 1;
        input->appParam = 2;
        return 0;
    case 21:
        input->appId = 1;
        input->appParam = 3;
        return 0;
    case 22:
        input->appId = 1;
        input->appParam = 4;
        return 0;
    case 23:
        input->appId = 1;
        input->appParam = 5;
        return 0;
    case 31:
        input->appId = 1;
        input->appParam = 7;
        return 0;
    case 32:
        input->appId = 1;
        input->appParam = 8;
        return 0;
    case 24:
        input->appId = 3;
        input->appParam = 1;
        return 0;
    case 25:
        input->appId = 3;
        input->appParam = 2;
        return 0;
    case 26:
        input->appId = 3;
        input->appParam = 3;
        return 0;
    case 27:
        input->appId = 5;
        return 0;
    case 28:
        input->appId = 13;
        return 0;
    case 29:
        input->appId = 14;
        return 0;
    case 30:
        input->appId = 0;
        input->appParam = -1;
        return 0;
    case 33:
        input->appId = 0;
        input->appParam = 638;
        return 0;
    default:
        input->appId = 1;
        return 0;
    }
}

BOOL ShortcutMenu_GetActionFromKeyItem(s32 item, u32 *action, u32 *invalid) {
    *invalid = 0;
    switch (item) {
    case 0:
        *action = 0;
        return TRUE;
    case 1:
        *action = 1;
        return TRUE;
    case 2:
        *action = 6;
        return TRUE;
    case 3:
        *action = 2;
        return TRUE;
    case 4:
        *action = 5;
        return TRUE;
    case 5:
        *action = 9;
        return TRUE;
    case 28:
        *action = 10;
        return TRUE;
    case 29:
        *action = 11;
        return TRUE;
    case 6:
    case 7:
    case 8:
    case 33:
        return FALSE;
    default:
        *invalid = 1;
        return FALSE;
    }
}

void func_ov012_0215b284(u32 arg0, ShortcutMenuWork *work) {
    GameData *gameData;
    u16 *param;

    if (work->app == NULL && work->done == 0) {
        GFL_BGSysSetBGEnabled(1, 0);
        if (Field_GetMsgBGSys(work->field)) {
            func_ov036_02187834();
        }
        gameData = GSYS_GetGameData(work->gameSystem);
        param = func_02017670(gameData);
        work->app = func_ov036_021bedd8(gameData, arg0, param, 0x50, 0x15);
    }
}

BOOL func_ov012_0215b2d0(ShortcutMenuWork *work) {
    if (work->done == 0) {
        switch (work->state) {
        case 0:
            if (work->app) {
                GFL_BGSysSetBGEnabled(1, 0);
                func_ov036_021bf004(work->app);
                work->app = NULL;
                work->state = 1;
            } else {
                work->state = 2;
            }
            break;
        case 1:
            Field_GetMsgBGSys(work->field);
            func_ov036_02187888();
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

BOOL func_ov012_0215b32c(FieldAppCallInput *input, void *arg) {
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
            func_ov012_0215b284(1, work);
            work->state = 1;
        }
        break;
    case 1:
        if (func_ov036_021bf198(work->app)) {
            work->state = 2;
        }
        break;
    case 2:
        work->state = 0;
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov012_0215b39c(FieldAppCallInput *input, void *arg) {
    return func_ov012_0215b2d0(arg);
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
