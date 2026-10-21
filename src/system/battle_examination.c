#include "types.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "save/battle_examination.h"
#include "save/save_control.h"
#include "save/trial_house.h"
#include "system/game_data.h"
#include <stddef.h>

// The downloaded battle examination that the Trial House plays: five Trainers, a signature and a checksum.
// Extra save block 5.

#define SAVE_EXTRA_BLOCK_BATTLE_EXAMINATION 5
#define BATTLE_EXAMINATION_LOAD_SIZE 0x800

u32 BattleExamination_GetSize(void) {
    return sizeof(BattleExaminationSave);
}

void BattleExamination_Init(BattleExaminationSave *examination) {
    MI_CpuFill8(examination, 0, sizeof(BattleExaminationSave));
}

BOOL BattleExamination_IsValid(void *saveBuffer) {
    BattleExaminationSave *examination = saveBuffer;

    if (examination->crc == getCRC16(examination, offsetof(BattleExaminationSave, crc)) &&
        examination->signature == BATTLE_EXAMINATION_SIGNATURE) {
        return TRUE;
    }
    return FALSE;
}

BOOL BattleExamination_IsSingle(void *extraSave) {
    BattleExaminationSave *examination = extraSave;

    if (!(examination->flags & BATTLE_EXAMINATION_DOUBLE)) {
        return TRUE;
    }
    return FALSE;
}

u32 BattleExamination_GetNumber(void *saveBuffer) {
    BattleExaminationSave *examination = saveBuffer;

    return (u16)(examination->flags & BATTLE_EXAMINATION_NUMBER_MASK);
}

BSubwayTrainer *BattleExamination_GetTrainer(void *save, u32 index) {
    BattleExaminationSave *examination = save;

    if (index < BATTLE_EXAMINATION_TRAINER_COUNT) {
        return &examination->trainers[index];
    }
    return examination->trainers;
}

void BattleExamination_Load(GameData *gameData, void *saveBuffer, HeapID heapId) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    void *buffer = GFL_HeapAllocate(heapId, BATTLE_EXAMINATION_LOAD_SIZE, FALSE, "battle_examination.c", 153);

    SaveControl_LoadExtraBlock(save, SAVE_EXTRA_BLOCK_BATTLE_EXAMINATION, heapId, buffer, BATTLE_EXAMINATION_LOAD_SIZE);
    MI_CpuCopy8(saveBuffer, buffer, sizeof(BattleExaminationSave));
    GameData_StartSaveBlock(gameData, SAVE_EXTRA_BLOCK_BATTLE_EXAMINATION);
    for (;;) {
        if (GameData_UpdateSaveBlock(gameData, SAVE_EXTRA_BLOCK_BATTLE_EXAMINATION) == 2) {
            break;
        }
        OS_WaitIrq(TRUE, 1);
    }
    freeIntermediateSaveExtraBlksAfterLoad2(save, SAVE_EXTRA_BLOCK_BATTLE_EXAMINATION);
    GFL_HeapFree(buffer);
}

void BattleExamination_SetUsed(void *extraSave) {
    BattleExaminationSave *examination = extraSave;

    if (examination->crc == getCRC16(examination, offsetof(BattleExaminationSave, crc))) {
        examination->signature = BATTLE_EXAMINATION_SIGNATURE_USED;
        examination->crc = getCRC16(examination, offsetof(BattleExaminationSave, crc));
    }
}
