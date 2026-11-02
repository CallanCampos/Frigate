// The save layout that port/tools/lab/savelab.py reads and writes, as the decomp's headers give it: a line for each
// offset of a field, size of a struct and number savelab uses, "NAME VALUE" in hex, printed with offsetof and sizeof as
// the compiler lays the headers out, and the offsets of the fields the lab leaves alone that the tests give the game's
// values. port/tests/lab/test_savelab.py holds savelab's LAYOUT to this output, and builds its synthetic saves' blocks
// field by field from it. The host build compiles the headers as the game's own files (port_decomp_env), and these
// save structs hold no pointers, so they are laid out as on the DS: the saves Frigate writes are melonDS's byte for
// byte. Game functionality: the saves (include/save/save_control.h's blocks: the player's place and spawns, the event
// work, the play time, the money and badges, the party); no game code runs.
#include "field/player_state.h"
#include "field/zone.h"
#include "pml/poke_party.h"
#include "save/event_work.h"
#include "save/player_savedata.h"
#include "save/playtime.h"
#include "save/save_control.h"
#include "save/trainer_data_savedata.h"
#include "save/trainer_game_info.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static void Print(const char *name, unsigned long value) {
    printf("%s 0x%lx\n", name, value);
}

#define FIELD(type, member) Print(#type "." #member, (unsigned long)offsetof(type, member))
#define SIZE(type) Print("sizeof(" #type ")", (unsigned long)sizeof(type))
#define VALUE(name) Print(#name, (unsigned long)(name))

// A one-bit field's byte and mask: the struct zeroed, the bit set, and the one byte that changed
#define BIT(type, member)                                                                                              \
    do {                                                                                                               \
        type object;                                                                                                   \
        memset(&object, 0, sizeof(object));                                                                            \
        object.member = 1;                                                                                             \
        PrintBit(#type "." #member, (const unsigned char *)&object, sizeof(object));                                   \
    } while (0)

static void PrintBit(const char *name, const unsigned char *bytes, size_t size) {
    char line[128];
    size_t i;

    for (i = 0; i < size; i++) {
        if (bytes[i] != 0) {
            snprintf(line, sizeof(line), "%s.byte", name);
            Print(line, (unsigned long)i);
            snprintf(line, sizeof(line), "%s.mask", name);
            Print(line, bytes[i]);
            return;
        }
    }
}

int main(void) {
    VALUE(SAVE_BLOCK_PARTY);
    VALUE(SAVE_BLOCK_TRAINER_DATA);
    VALUE(SAVE_BLOCK_PLAYER);
    VALUE(SAVE_BLOCK_EVENT_WORK);
    VALUE(SAVE_BLOCK_TRAINER_GAME_INFO);
    VALUE(SAVE_BLOCK_MAX);

    SIZE(PlayerSave);
    FIELD(PlayerSave, nowSpawn);
    FIELD(PlayerSave, nextSpawn);
    FIELD(PlayerSave, location);
    FIELD(SaveLocation, zoneId);
    FIELD(SaveLocation, pos);
    FIELD(SaveLocation, railPos);
    FIELD(SaveLocation, unk18);
    FIELD(SaveLocation, unk1A);
    FIELD(SaveLocation, exState);
    SIZE(ZoneSpawnInfo);
    FIELD(ZoneSpawnInfo, changeType);
    FIELD(ZoneSpawnInfo, zoneId);
    FIELD(ZoneSpawnInfo, warpId);
    FIELD(ZoneSpawnInfo, warpDir);
    FIELD(ZoneSpawnInfo, posWeightBits);
    FIELD(ZoneSpawnInfo, isRail);
    FIELD(ZoneSpawnInfo, pos);
    SIZE(RailPosition);
    FIELD(RailPosition, componentId);
    FIELD(RailPosition, componentIsLine);
    FIELD(RailPosition, railDirection);
    FIELD(RailPosition, posSide);
    FIELD(RailPosition, posFront);
    VALUE(ZONE_SPAWN_CHANGE_TYPE_WARP);
    VALUE(ZONE_SPAWN_CHANGE_TYPE_POSITION);
    VALUE(ZONE_SPAWN_CHANGE_TYPE_WARP_ARRIVAL);
    VALUE(ZONE_SPAWN_CHANGE_TYPE_3);
    VALUE(WARP_DIR_UP);
    VALUE(WARP_DIR_DOWN);
    VALUE(WARP_DIR_LEFT);
    VALUE(WARP_DIR_RIGHT);
    VALUE(FLD_PLAYER_EXSTATE_NONE);
    VALUE(FLD_PLAYER_EXSTATE_CYCLING);
    VALUE(FLD_PLAYER_EXSTATE_SURF);
    VALUE(FLD_PLAYER_EXSTATE_DIVE);

    SIZE(EventWork);
    FIELD(EventWork, works);
    FIELD(EventWork, flags);
    VALUE(EVENT_WORK_COUNT);
    VALUE(EVENT_FLAG_SAVED_COUNT);

    SIZE(TrainerDataSave);
    FIELD(TrainerDataSave, playTime);
    FIELD(PlayTime, hours);
    FIELD(PlayTime, minutes);
    FIELD(PlayTime, seconds);
    VALUE(PLAYTIME_MAX_HOURS);

    SIZE(TrainerGameInfoSave);
    FIELD(TrainerGameInfoSave, cash);
    FIELD(TrainerGameInfoSave, badges);
    VALUE(TRAINER_GAME_INFO_CASH_MAX);
    VALUE(TRAINER_GAME_INFO_GYM_COUNT);

    SIZE(PokeParty);
    FIELD(PokeParty, count);
    FIELD(PokeParty, pkms);
    VALUE(PARTY_SIZE);
    SIZE(PartyPkm);
    FIELD(BoxPkm, pid);
    BIT(BoxPkm, partyDecrypted);
    FIELD(BoxPkm, checksum);
    FIELD(BoxPkm, contentBuffer);
    SIZE(PkmBuffer);
    FIELD(PartyPkm, statusCond);
    FIELD(PartyPkm, level);
    FIELD(PartyPkm, nowHP);
    FIELD(PartyPkm, maxHP);
    FIELD(PkmBlockA, species);
    FIELD(PkmBlockA, item);
    FIELD(PkmBlockA, exp);
    FIELD(PkmBlockB, moves);

    // Fields the lab leaves alone, which the test's synthetic saves hold at the values a game leaves in them (no
    // follower, no step counter running) so that a write the lab shouldn't make shows
    FIELD(PlayerSave, followIndex);
    FIELD(PlayerSave, stepCounter);
    return 0;
}
