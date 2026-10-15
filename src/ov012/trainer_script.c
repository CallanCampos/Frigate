#include "types.h"
#include "battle/trainer_data.h"
#include "field/field_script.h"
#include "field/trainer_script.h"
#include "nitro/os.h"
#include "save/event_work.h"

// The Trainers whose battles resetRebattleTrainers resets, by Trainer ID less 0x5f0
const int REBATTLE_TRAINER_COUNT = 12;
static const u16 sRebattleTrainers[REBATTLE_TRAINER_COUNT] = { 0x5f,  0x60,  0xb7,  0xb8,  0xb9,  0xba,
                                                               0x128, 0x12b, 0x12e, 0x12f, 0x14d, 0x2f0 };

void SetupTrainerClashSlot(GameEvent *event, int index, const TrainerClashData *data) {
    ScriptWork *work = EventScriptCall_GetWork(event);
    TrainerClashSlot *dst = ScriptWork_GetTrainerState(work, index);

    dst->data = *data;
    dst->eye = NULL;
}

u16 GetNPCTrainerIDFromSCRID(u32 scriptId) {
    if (scriptId < 0x1388) {
        return scriptId - 0xbb8;
    }
    return scriptId - 0x1388;
}

u16 GetNormalSCRIDFromTrainerID(u32 trainerId) {
    return trainerId + 0xbb8;
}

u16 GetPairMember2SCRIDFromTrainerID(u32 trainerId) {
    return trainerId + 0x1388;
}

BOOL isTrainerScrIdDoublePair2(u32 scriptId) {
    return scriptId >= 0x1388;
}

BOOL isDoubleBattle(u32 trainerId) {
    return TrainerData_GetParam(trainerId, 2) == 1;
}

u8 getBattleType(u16 trainerId) {
    return TrainerData_GetParam(trainerId, 2);
}

BOOL TrainerFlagGet(EventWork *eventWork, u16 trainerId) {
    return EventWork_FlagGet(eventWork, (trainerId + 0x5f0));
}

void setTrainerBattleFlag(EventWork *eventWork, u16 trainerId) {
    EventWork_FlagSet(eventWork, (trainerId + 0x5f0));
}

void clearTrainerBattleFlag(EventWork *eventWork, u16 trainerId) {
    EventWork_FlagReset(eventWork, (trainerId + 0x5f0));
}

void resetRebattleTrainers(EventWork *eventWork) {
    int i;

    clock();
    for (i = 0; i < REBATTLE_TRAINER_COUNT; i++) {
        EventWork_FlagReset(eventWork, sRebattleTrainers[i] + 0x5f0);
    }
    clock();
}
