#include "types.h"
#include "field/fest_mission_hollow.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/rival_select.h"
#include "field/zone.h"
#include "gfl/random.h"
#include "save/save_control.h"
#include "system/area_data.h"
#include "system/game_data.h"

RivalEntry *func_ov073_021e8be0(RivalSelectContext *context, u32 id) {
    RivalEntry *entries = LinkFestival_GetEntries(context->entryOwner);
    return &entries[func_ov073_021e8d00(context, id)];
}

void func_ov073_021e8bfc(RivalSelectContext *context, u32 id) {
    u8 choices[10];
    int i;
    u8 current = func_ov073_021e8d00(context, id);
    u8 count = 0;
    RivalEntry *entries = LinkFestival_GetEntries(context->entryOwner);
    u8 choice;

    if (current == 10) {
        return;
    }
    for (i = 0; i < 10; i++) {
        if (current != i) {
            choices[count] = i;
            count++;
        }
    }
    entries[current].selected = 0;
    choice = choices[GFL_RandomLC(9)];
    entries[choice].selected = 1;
}

BOOL func_ov073_021e8c4c(RivalSelectContext *context) {
    if (FesMissionHollow_IsActive(context)) {
        func_ov073_021e8c64(context);
    }
    return FALSE;
}

BOOL func_ov073_021e8c64(RivalSelectContext *context) {
    u8 rival;
    FieldActor *actor;

    if (func_02018fa8(ZoneData_GetAreaID(Field_GetPlayerStateZoneID(context->field))) == TRUE) {
        rival = getHollowNum(getHollow_RivalData(GameData_GetSaveControl(context->gameData)));
        if (func_ov073_021e8cac(context, rival) == TRUE) {
            actor = FindFieldActor(context->actors, 0xe0);
            if (actor != NULL) {
                DeleteActor(actor);
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL func_ov073_021e8cac(RivalSelectContext *context, u32 id) {
    RivalEntry *entries = LinkFestival_GetEntries(context->entryOwner);
    int i;
    for (i = 0; i < 10; i++) {
        if (entries[i].id == id && entries[i].selected == 1 && entries[i].active != 0) {
            return TRUE;
        }
    }
    return FALSE;
}

void func_ov073_021e8cdc(RivalSelectContext *context, u32 id) {
    RivalEntry *entries = LinkFestival_GetEntries(context->entryOwner);
    int i;
    for (i = 0; i < 10; i++) {
        if (entries[i].id == id) {
            entries[i].active = 1;
        }
    }
}

u8 func_ov073_021e8d00(RivalSelectContext *context, u32 id) {
    RivalEntry *entries = LinkFestival_GetEntries(context->entryOwner);
    int i;
    for (i = 0; i < 10; i++) {
        if (entries[i].id == id) {
            return i;
        }
    }
    return 10;
}
