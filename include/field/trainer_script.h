#ifndef POKEBW2_FIELD_TRAINER_SCRIPT_H
#define POKEBW2_FIELD_TRAINER_SCRIPT_H

#include "types.h"
#include "struct_decls.h"

// A Trainer who has seen the player: how far away, facing which way, and in which kind of battle
typedef struct {
    FieldActor *actor;
    int range;
    u32 dir;
    u32 scrId;
    u32 trainerId;
    u32 kind;
} TrainerClashData;

// TrainerClashData's kind: a single Trainer, a double battle pair, two Trainers who saw the player at once, and one
// with the player's partner
enum {
    TRAINER_CLASH_SINGLE,
    TRAINER_CLASH_DOUBLE_PAIR,
    TRAINER_CLASH_TWO_TRAINERS,
    TRAINER_CLASH_WITH_PARTNER,
};

struct TrainerClashSlot {
    TrainerClashData data;
    // The work of the Trainer's walk up to the player
    struct EventTrainerEyeWork *eye;
};

void SetupTrainerClashSlot(GameEvent *event, int index, const TrainerClashData *data);
u16 GetNPCTrainerIDFromSCRID(u32 scriptId);
u16 GetNormalSCRIDFromTrainerID(u32 trainerId);
u16 GetPairMember2SCRIDFromTrainerID(u32 trainerId);
BOOL isTrainerScrIdDoublePair2(u32 scriptId);
BOOL isDoubleBattle(u32 trainerId);
u8 getBattleType(u16 trainerId);
BOOL TrainerFlagGet(EventWork *eventWork, u16 trainerId);
void setTrainerBattleFlag(EventWork *eventWork, u16 trainerId);
void clearTrainerBattleFlag(EventWork *eventWork, u16 trainerId);

#endif // POKEBW2_FIELD_TRAINER_SCRIPT_H
