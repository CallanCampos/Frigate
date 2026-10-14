#ifndef POKEBW2_SAVE_TRAINER_GAME_INFO_H
#define POKEBW2_SAVE_TRAINER_GAME_INFO_H

#include "types.h"
#include "struct_decls.h"

// Save block 0x34, which swan calls the trainer game info and, in the names of its later functions, the trainer card
// data (TrainerCardSave is the same struct). It keeps what belongs to no other block: the money, the badges, the
// favorite Pokémon, the greeting and thanks of the passing-by profile, the survey's answers, the parties that beat
// the Gyms and the table of the gifts given once

#define TRAINER_GAME_INFO_CASH_MAX 9999999
#define TRAINER_GAME_INFO_COUNT_MAX 99999
#define TRAINER_GAME_INFO_KEYBOARD_COUNT 12
#define TRAINER_GAME_INFO_MESSAGE_LENGTH 9
#define TRAINER_GAME_INFO_SURVEY_ANSWER_COUNT 3
#define TRAINER_GAME_INFO_GYM_COUNT 8
#define TRAINER_GAME_INFO_DR_COUNT 8

struct TrainerGameInfoSave {
    u32 cash;
    u32 badges;
    u16 faveSpecies;
    u8 faveForm : 6;
    u8 unk0a_6 : 1;
    u8 unk0a_7 : 1;
    u8 unk0b;
    // The keyboard that each mode of the name entry last used
    u8 keyboardTypes[TRAINER_GAME_INFO_KEYBOARD_COUNT];
    u32 palParkHighScore : 28;
    u32 palParkResult : 4;
    // Two counters that stop at TRAINER_GAME_INFO_COUNT_MAX
    u32 unk1c;
    u32 unk20;
    u16 greeting[TRAINER_GAME_INFO_MESSAGE_LENGTH];
    u16 thanks[TRAINER_GAME_INFO_MESSAGE_LENGTH];
    u8 surveyRank;
    // Two bits for each of the start menu's items that are added later
    u8 startMenuFlags;
    u8 surveyQuestion;
    u8 surveyAnswers[TRAINER_GAME_INFO_SURVEY_ANSWER_COUNT];
    u16 surveyCounts[TRAINER_GAME_INFO_SURVEY_ANSWER_COUNT];
    // When the survey started, in seconds since 2000
    s64 surveySeconds;
    u16 gymParties[TRAINER_GAME_INFO_GYM_COUNT][6];
    u32 drTable[TRAINER_GAME_INFO_DR_COUNT];
    u8 unkdc[4];
    // The color of the Battle Recorder
    u32 battleRecorderColor;
    u16 unke4;
    u8 unke6[0xa];
};

// The size of the block
u32 TrainerGameInfo_GetSize(void);
void TrainerGameInfo_Init(TrainerGameInfoSave *info);
void SetFavePokemonData(TrainerGameInfoSave *info, u32 species, u32 form, u32 a3, int a4);
void GetFavePokemonData(const TrainerGameInfoSave *info, int *species, int *form, int *a3, int *a4);
// Add one to the counters at 0x1c and 0x20, and return them
u32 func_0200c910(TrainerGameInfoSave *info);
u32 func_0200c928(TrainerGameInfoSave *info);
// The species of the party that beat a Gym
void SetGymDefeatPartyData(TrainerGameInfoSave *info, u32 gym, const u16 *species);
void GetGymDefeatPartyData(TrainerGameInfoSave *info, u32 gym, u16 *species);

#endif // POKEBW2_SAVE_TRAINER_GAME_INFO_H
