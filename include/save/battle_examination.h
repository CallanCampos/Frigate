#ifndef POKEBW2_SAVE_BATTLE_EXAMINATION_H
#define POKEBW2_SAVE_BATTLE_EXAMINATION_H

#include "types.h"
#include "field/bsubway_scr.h"

// Extra save block 5, a downloaded battle examination, which the Trial House plays (battle_examination.c).
// save/trial_house.h declares the functions that the Trial House calls.

#define BATTLE_EXAMINATION_TRAINER_COUNT 5
#define BATTLE_EXAMINATION_SIGNATURE 0xd68
#define BATTLE_EXAMINATION_SIGNATURE_USED 0x7da
#define BATTLE_EXAMINATION_NUMBER_MASK 0x7f
#define BATTLE_EXAMINATION_DOUBLE 0x8000

typedef struct BattleExaminationSave {
    BSubwayTrainer trainers[BATTLE_EXAMINATION_TRAINER_COUNT];
    // What the Trial House copies into its own save block
    u8 unk5A0[0x22];
    // BATTLE_EXAMINATION_SIGNATURE, or BATTLE_EXAMINATION_SIGNATURE_USED once the examination was taken
    u16 signature;
    // The examination's number and BATTLE_EXAMINATION_DOUBLE
    u16 flags;
    // getCRC16 of everything before it
    u16 crc;
} BattleExaminationSave; // size: 0x5c8

void BattleExamination_Init(BattleExaminationSave *examination);

#endif // POKEBW2_SAVE_BATTLE_EXAMINATION_H
