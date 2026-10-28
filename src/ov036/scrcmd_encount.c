// The script commands of wild encounters: static and scripted wild battles, the capture demo, the phenomena's items,
// roaming Pokémon, the fishing challenge's random Pokémon and the Repel's rearming. The name is the ROM's own, from
// GFL_HeapAllocate's file argument. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/sound.h"
#include "constants/zones.h"
#include "field/enc_pokeset.h"
#include "field/encounter.h"
#include "field/event_battle.h"
#include "field/field.h"
#include "field/field_event.h"
#include "field/field_script.h"
#include "field/scrcmd_encount.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "pml/item.h"
#include "save/bag.h"
#include "save/encounter.h"
#include "save/save_control.h"
#include "system/game_beacon.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

#define FISHING_CHALLENGE_ZONE_COUNT 6
#define FISHING_SLOT_COUNT 5

// The zones of the fishing challenge, whose fishing Pokémon it picks from
static const u16 CHALLENGE_FISHING_ZONES[FISHING_CHALLENGE_ZONE_COUNT] = {
    ZONE_VILLAGE_BRIDGE, ZONE_ROUTE_1, ZONE_ROUTE_3, ZONE_WELLSPRING_CAVE, ZONE_ROUTE_13, ZONE_ROUTE_17,
};

BOOL Scrcmd_Encount_CallWildBattle(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 species = ScriptReadAny(vm, env);
    u16 level = ScriptReadAny(vm, env);
    u16 flags = ScriptReadAny(vm, env);
    ScriptFieldWork *fieldWork;

    ScriptWork_GetGameSystem(work);
    fieldWork = ScriptWork_GetFieldWork(work);
    ScriptWork_CallEvent(
        work, EventWildBattleCall_CreateStatic(Field_GetEncountSystem(fieldWork->field), species, level, 0, flags));
    return TRUE;
}

BOOL Scrcmd_Encount_CallWildBattleEx(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    u16 species = ScriptReadAny(vm, env);
    u16 level = ScriptReadAny(vm, env);
    u16 form = ScriptReadAny(vm, env);
    u16 flags = ScriptReadAny(vm, env);
    ScriptFieldWork *fieldWork;

    ScriptWork_GetGameSystem(work);
    fieldWork = ScriptWork_GetFieldWork(work);
    ScriptWork_CallEvent(
        work, EventWildBattleCall_CreateStatic(Field_GetEncountSystem(fieldWork->field), species, level, form, flags));
    return TRUE;
}

BOOL Scrcmd_Encount_CallWildBattleEnd(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    ScriptWork_CallEvent(
        work, CallFieldMapEntranceInTransition(gsys, GSYS_GetField(gsys), FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
    return TRUE;
}

BOOL Scrcmd_Encount_CallCaptureDemo(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    ScriptWork_CallEvent(work, EventCaptureDemo_Create(gsys, GSYS_GetField(gsys), 4));
    return TRUE;
}

BOOL Scrcmd_Encount_PhenomenonGetItemID(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    u16 *result = ScriptReadVar(vm, env);

    *result = EffectEncount_GetItemId(Field_GetEncountSystem(field));
    return FALSE;
}

BOOL Scrcmd_Encount_SpawnMovePokemonIfInactive(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    EncountSave *save = SaveControl_GetEncountSave(GameData_GetSaveControl(gameData));
    u8 slot = ScriptReadAny(vm, env);

    if (EncountSave_GetRoamingPkmStatus(save, slot) == FALSE) {
        EncountSave_SpawnEntry(gameData, slot);
    }
    return FALSE;
}

// Add the species to the list unless it is in it already; the new count
static u16 LoadEventEncountPokemonToBuffer(u16 *list, u16 count, u16 species) {
    int i;

    for (i = 0; i < count; i++) {
        if (species == list[i]) {
            return count;
        }
    }
    list[count] = species;
    return count + 1;
}

// A random one of the Pokémon that can be fished in the fishing challenge's zones this season
BOOL Scrcmd_Encount_FishingChallengeGetRandomPkm(VM *vm, FieldScriptEnv *env) {
    EncData encData;
    u8 season;
    u16 i;
    u16 j;
    ArcTool *arc;
    u16 *result;
    u16 *species;
    u16 count = 0;
    GameData *gameData = FieldScriptEnv_GetGameData(env);

    result = ScriptReadVar(vm, env);
    season = GameData_GetSeason(gameData);
    arc = GFL_ArcSysCreateFileHandle(ARCID_ENCOUNTDATA, HEAPID_TAIL(HEAPID_FIELDMAP));
    species = GFL_HeapAllocate(HEAPID_TAIL(HEAPID_FIELDMAP), 120, TRUE, "scrcmd_encount.c", 261);
    for (i = 0; i < FISHING_CHALLENGE_ZONE_COUNT; i++) {
        if (EncData_Load(&encData, arc, CHALLENGE_FISHING_ZONES[i], season)) {
            if (encData.userData[ENCTYPE_FISHING] != 0) {
                for (j = 0; j < FISHING_SLOT_COUNT; j++) {
                    count = LoadEventEncountPokemonToBuffer(species, count, encData.fishing[j].species);
                }
            }
            if (encData.userData[ENCTYPE_FISHING_RARE] != 0) {
                for (j = 0; j < FISHING_SLOT_COUNT; j++) {
                    count = LoadEventEncountPokemonToBuffer(species, count, encData.fishingRare[j].species);
                }
            }
        }
    }
    *result = species[GFL_RandomLCAlt(count)];
    GFL_HeapFree(species);
    GFL_ArcToolFree(arc);
    return FALSE;
}

// Use another of the Repel just used up, if the player has one
BOOL Scrcmd_Encount_RepelRearm(VM *vm, FieldScriptEnv *env) {
    HeapID heapId = FieldScriptEnv_GetHeapID(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    EncountSave *save = SaveControl_GetEncountSave(GameData_GetSaveControl(gameData));
    BagSave *bag = GameData_GetBag(gameData);
    u16 repel = EncountSave_GetUsedRepelItemID(save);
    u16 *result = ScriptReadVar(vm, env);

    if (repel != ITEM_MAX_REPEL && repel != ITEM_SUPER_REPEL && repel != ITEM_REPEL) {
        return FALSE;
    }
    if (EncountSave_IsRepelDepleted(save)) {
        u8 steps = GetItemParam(repel, ITEM_PARAM_HOLD_PARAM, heapId);

        EncountSave_SetRepelSteps(save, steps);
        BagSave_SubItem(bag, repel, 1, heapId);
        GameBeaconSys_SendItemUse(repel);
        GFL_SndSEPlay(SEQ_SE_SYS_92);
        *result = repel;
    }
    return FALSE;
}
