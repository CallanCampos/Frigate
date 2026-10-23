#ifndef POKEBW2_SAVE_TRIAL_HOUSE_H
#define POKEBW2_SAVE_TRIAL_HOUSE_H

#include "types.h"
#include "gfl/heap.h"
#include "save/wbt_save.h"
#include "struct_decls.h"

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
    // The Pokémon World Tournament's part (wbt_savedata.c)
    WbtSave wbt;
}; // size: 0xa8

u32 BattleExamination_GetSize(void);
BOOL BattleExamination_IsValid(BattleExaminationSave *saveBuffer);
BOOL BattleExamination_IsSingle(BattleExaminationSave *extraSave);
u32 BattleExamination_GetNumber(BattleExaminationSave *saveBuffer);
// The Trainer of a recorded battle, 0 to 4
BSubwayTrainer *BattleExamination_GetTrainer(BattleExaminationSave *save, u32 index);
void BattleExamination_Load(GameData *gameData, void *saveBuffer, HeapID heapId);
// The block's size and its initial state (trial_house_savedata.c)
u32 TrialHouseSave_GetSize(void);
void TrialHouseSave_Init(TrialHouseSave *save);
TrialHouseSave *TrialHouseSave_Get(SaveControl *save);
// Of wbt_savedata.c: clears the Pokémon World Tournament's part
void WbtSave_Init(WbtSave *wbt);
void BattleExamination_SetUsed(BattleExaminationSave *extraSave);

#endif // POKEBW2_SAVE_TRIAL_HOUSE_H
