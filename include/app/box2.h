#ifndef POKEBW2_APP_BOX2_H
#define POKEBW2_APP_BOX2_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The PC Box
#define OVERLAY_BOX2 OVERLAY_ID(255)

// What the box is opened for, which picks its title: the messages from Box2Bmp_Text_DepositPokemon on are in this
// order
enum {
    BOX2_MODE_DEPOSIT,
    BOX2_MODE_WITHDRAW,
    BOX2_MODE_MOVE_POKEMON,
    BOX2_MODE_MOVE_ITEMS,
    BOX2_MODE_BATTLE_BOX,
    // Picking the Pokémon to send to the Dream World
    BOX2_MODE_DREAM_WORLD,
};

typedef struct {
    GameData *gameData;
    BoxSaveAccessor *boxes;
    PokeParty *party;
    BagSave *bag;
    PlayerInfo *playerInfo;
    // The Battle Box's team is locked: its Pokémon can't be moved and their markings can't change
    u32 battleBoxLocked;
    TrainerDataSave *trainerData;
    // Passed on to the summary
    u32 isNationalDex;
    // The Game Sync's flags of the species that can be sent to the Dream World, which Box2Main_IsSpeciesSendable reads
    u8 *sendableSpecies;
    // A BOX2_MODE_*
    u32 mode;
    // Set when the player left with the close button, cleared when he backed out
    u16 closed;
    // Where the chosen Pokemon is, 0xff for both if none was chosen
    u8 tray;
    u8 position;
} Box2Param;

extern const GameProcFunctions BOX2_PROC_FUNCTIONS;

#endif // POKEBW2_APP_BOX2_H
