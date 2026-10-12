#ifndef POKEBW2_SAVE_TRIAL_HOUSE_H
#define POKEBW2_SAVE_TRIAL_HOUSE_H

#include "types.h"
#include "struct_decls.h"
#include "gfl/heap.h"

// Save block 0x3f
// A Pokémon of a recorded team
typedef struct {
    u16 species;
    u8 form;
    u8 sex;
} TrialHousePokemon;

// The result of a challenge
typedef struct {
    u8 valid;
    u8 isDouble;
    u16 points;
    TrialHousePokemon pokemon[4];
} TrialHouseRecord;

struct TrialHouseSave {
    // The challenges against the Trainers of the game and of a downloaded challenge
    TrialHouseRecord records[2];
    u8 bits[16];
    // Copied from the downloaded challenge's save
    u8 unk38[0x22];
    u8 padding5A[2];
    // The data of the file at func_0200fe80, which that file initializes
    u8 unk5C[0x4c];
}; // size: 0xa8

u32 BattleExamination_GetSize(void);
BOOL BattleExamination_IsValid(void *saveBuffer);
BOOL BattleExamination_IsSingle(void *extraSave);
u32 BattleExamination_GetNumber(void *saveBuffer);
// The Trainer of a recorded battle, 0 to 4
BSubwayTrainer *BattleExamination_GetTrainer(void *save, u32 index);
void BattleExamination_Load(GameData *gameData, void *saveBuffer, HeapID heapId);
// The block's size and its initial state (trial_house_savedata.c)
u32 func_0200f19c(void);
void func_0200f1a0(TrialHouseSave *save);
TrialHouseSave *func_0200f1b8(SaveControl *save);
// Of a file that is not written yet: clears the data at unk5C
void func_0200fe80(void *data);
void BattleExamination_SetUsed(void *extraSave);

#endif // POKEBW2_SAVE_TRIAL_HOUSE_H
