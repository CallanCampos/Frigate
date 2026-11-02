// The gifts given only once, which the trainer card's table keeps as keys made from the player's ID. Function names
// from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "save/player_info.h"
#include "save/trainer_card.h"

#define ONESHOT_DR_COUNT 8

// The key of each gift, which the table keeps XORed with the player's ID
static const u32 ONESHOT_DR_KEYS[ONESHOT_DR_COUNT] = {
    0x0132b4e5, 0x0132b536, 0x0132df06, 0x01cb7587, 0x009a5c10, 0x0132e2f1, 0x042e464f, 0x01708483,
};

void setOneShotDRObtained(TrainerCardSave *trainerCard, int flag, PlayerInfo *playerInfo) {
    if (flag < ONESHOT_DR_COUNT) {
        setToTrainerCardDRTable(trainerCard, flag, getIDAsUInt(playerInfo) ^ ONESHOT_DR_KEYS[flag]);
    }
}

BOOL isOneShotDRObtained(TrainerCardSave *trainerCard, int flag, PlayerInfo *playerInfo) {
    u32 id;
    u32 key;

    if (flag >= ONESHOT_DR_COUNT) {
        return FALSE;
    }
    id = getIDAsUInt(playerInfo);
    key = getOneShotDRKeyFromSave(trainerCard, flag);
    if ((key ^ id) == ONESHOT_DR_KEYS[flag]) {
        return TRUE;
    }
    return FALSE;
}
