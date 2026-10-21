#ifndef POKEBW2_SAVE_TRAINER_CARD_H
#define POKEBW2_SAVE_TRAINER_CARD_H

#include "types.h"
#include "struct_decls.h"

void setSecondsCurrentTimeInTrainerCard(TrainerCardSave *trainerCard, s64 seconds);
// Overlay 12's oneshot_dr.c: the gifts given once, kept in the trainer card's table keyed to the player's ID
void setOneShotDRObtained(TrainerCardSave *trainerCard, int flag, PlayerInfo *playerInfo);
BOOL isOneShotDRObtained(TrainerCardSave *trainerCard, int flag, PlayerInfo *playerInfo);
void setToTrainerCardDRTable(TrainerCardSave *trainerCard, int flag, u32 key);
u32 getOneShotDRKeyFromSave(TrainerCardSave *trainerCard, int flag);
TrainerCardSave *getTrainerCardData_wrapper(SaveControl *save);
u32 TrainerCard_GetPasserbyCount(TrainerCardSave *trainerCard);
u32 TrainerCard_GetThankedCount(TrainerCardSave *trainerCard);
u16 TrainerCard_GetHallOfFameMinutes(TrainerCardSave *trainerCard);
void TrainerCard_SetHallOfFameMinutes(TrainerCardSave *trainerCard, u16 value);
// The color of the Battle Recorder
u8 TrainerCard_GetBattleRecorderColor(TrainerCardSave *trainerCard);
void TrainerCard_SetBattleRecorderColor(TrainerCardSave *trainerCard, u8 color);
// When the survey started, in seconds since 2000
s64 getSecondsFromTrainerCardData(TrainerGameInfoSave *info);
// Adds to the play time
void TrainerGameInfo_SubHallOfFameMinutes(TrainerGameInfoSave *info, u16 minutes);
BOOL isBadgeObtained(TrainerCardSave *trainerCard, u32 badgeId);
void addBadge(TrainerCardSave *trainerCard, u32 badgeId);

#endif // POKEBW2_SAVE_TRAINER_CARD_H
