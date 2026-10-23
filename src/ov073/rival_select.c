#include "types.h"
#include "field/fest_mission_gimmick.h"
#include "field/fest_mission_hollow.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/rival_select.h"
#include "field/zone.h"
#include "gfl/random.h"
#include "save/save_control.h"
#include "system/area_data.h"
#include "system/game_data.h"

RivalEntry *RivalSelect_GetEntry(FesGimmick *gimmick, u32 id) {
    RivalEntry *entries = LinkFestival_GetEntries(gimmick->festival);
    return &entries[RivalSelect_FindEntryIndex(gimmick, id)];
}

void RivalSelect_SelectOtherRandom(FesGimmick *gimmick, u32 id) {
    u8 choices[10];
    int i;
    u8 current = RivalSelect_FindEntryIndex(gimmick, id);
    u8 count = 0;
    RivalEntry *entries = LinkFestival_GetEntries(gimmick->festival);
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

BOOL RivalSelect_RemoveHollowActorIfMission(FesGimmick *gimmick) {
    if (FesMissionHollow_IsActive(gimmick)) {
        RivalSelect_RemoveHollowActor(gimmick);
    }
    return FALSE;
}

BOOL RivalSelect_RemoveHollowActor(FesGimmick *gimmick) {
    u8 rival;
    FieldActor *actor;

    if (func_02018fa8(ZoneData_GetAreaID(Field_GetPlayerStateZoneID(gimmick->field))) == TRUE) {
        rival = getHollowNum(getHollow_RivalData(GameData_GetSaveControl(gimmick->gameData)));
        if (RivalSelect_IsSelectedAndActive(gimmick, rival) == TRUE) {
            actor = FindFieldActor(gimmick->actors, 0xe0);
            if (actor != NULL) {
                DeleteActor(actor);
                return TRUE;
            }
        }
    }
    return FALSE;
}

BOOL RivalSelect_IsSelectedAndActive(FesGimmick *gimmick, u32 id) {
    RivalEntry *entries = LinkFestival_GetEntries(gimmick->festival);
    int i;
    for (i = 0; i < 10; i++) {
        if (entries[i].id == id && entries[i].selected == 1 && entries[i].active != 0) {
            return TRUE;
        }
    }
    return FALSE;
}

void RivalSelect_SetActive(FesGimmick *gimmick, u32 id) {
    RivalEntry *entries = LinkFestival_GetEntries(gimmick->festival);
    int i;
    for (i = 0; i < 10; i++) {
        if (entries[i].id == id) {
            entries[i].active = 1;
        }
    }
}

u8 RivalSelect_FindEntryIndex(FesGimmick *gimmick, u32 id) {
    RivalEntry *entries = LinkFestival_GetEntries(gimmick->festival);
    int i;
    for (i = 0; i < 10; i++) {
        if (entries[i].id == id) {
            return i;
        }
    }
    return 10;
}
