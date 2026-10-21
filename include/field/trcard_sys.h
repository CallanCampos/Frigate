#ifndef POKEBW2_FIELD_TRCARD_SYS_H
#define POKEBW2_FIELD_TRCARD_SYS_H

// Overlay 12's trcard_sys.c: the trainer card's data, gathered from the save, and the procs that run overlay 186's
// trainer card screen and the screens it calls (the greeting's phrase select, overlay 187's medals)

#include "types.h"
#include "gfl/heap.h"
#include "gfl/proc.h"
#include "struct_decls.h"
#include "system/pms.h"

// What the trainer card shows. TrainerCardData_Fill fills the front and the back, and TrainerCardData_FillExtras the
// medals, the Pokémon World Tournament and Join Avenue
typedef struct {
    u8 version;
    u8 unk01;
    u8 unk02;
    // Up to five, from TrainerCard_CountStars
    u8 stars;
    // Set when the card is the player's own, whose play time keeps running
    u8 isOwn : 1;
    u8 gender : 1;
    u8 unk04_2 : 1;
    u8 unk04_3 : 1;
    u8 unk04_4 : 1;
    u8 unk04_5 : 1;
    u8 canEdit : 1;
    u8 unk04_7 : 1;
    u8 unk05;
    u8 unk06;
    u8 unk07;
    // One bit for each badge
    u16 badges;
    u16 name[8];
    // The player's play time, while isOwn is set
    PlayTime *playTime;
    u32 money;
    u32 seenCount;
    u16 trainerId;
    u16 playHours;
    u16 clearHour;
    u8 playMinutes;
    // The date the adventure started on, and the date and time of the first Hall of Fame entry
    u8 startYear;
    u8 startMonth;
    u8 startDay;
    u8 clearYear;
    u8 clearMonth;
    u8 clearDay;
    u8 clearMinute;
    u8 unk36[2];
    // unk38 to unk44, unk4C to unk54, unk5C and unk60 are sums of records, from the rows of
    // TrainerCardData_SumRecords's table
    u32 unk38;
    u32 unk3C;
    u32 unk40;
    u32 unk44;
    u32 unk48;
    u32 unk4C;
    u32 unk50;
    u32 unk54;
    u32 unk58;
    u16 unk5C;
    u16 passPowerCount;
    u16 unk60;
    u16 palParkHighScore;
    u16 unk64;
    // The Battle Test rank
    u16 battleTestRank;
    u32 unk68_0 : 1;
    u32 unk68_1 : 1;
    u32 unk68_2 : 1;
    u32 unk68_3 : 1;
    u32 unk68_4 : 1;
    u32 unk68_5 : 27;
    // The phrase the player greets with, which is also the C-Gear's
    PMSData greeting;
    u8 unk74[0x600];
    u32 unk674;
    // The medal obtained last, or 0xff
    u32 lastMedal;
    u16 medalCount;
    u8 medalRank;
    u8 medalYear;
    u8 medalMonth;
    u8 medalDay;
    u8 unk682[2];
    u32 unk684;
    u16 unk688;
    u16 unk68A;
    // The tournaments won in the Pokémon World Tournament, up to 9999
    u32 pwtWins;
    u32 unk690_0 : 1;
    u32 unk690_1 : 1;
    u32 unk690_2 : 1;
    u32 unk690_3 : 1;
    u32 unk690_4 : 28;
    u8 unk694[2];
    // Join Avenue's name
    u16 avenueName[23];
} TrainerCardData;

typedef struct {
    TrainerCardData *data;
    // The card on the screen
    TrainerCardData *shownData;
    GameData *gameData;
    BOOL canEdit;
    // What the screen asks for next: 1 the greeting's phrase select, 2 overlay 186's other screen, 3 back to the
    // card, 4 the medals
    u32 next;
    // 1 when the medals screen asked to return to the field
    u32 result;
    s32 appParam;
} TrainerCardParam;

void TrainerCardData_FillExtras(TrainerCardData *data, GameData *gameData, HeapID heapId);
// isSnapshot is TRUE for a card that is sent away, with the play time copied
void TrainerCardData_Fill(TrainerCardData *data, GameData *gameData, BOOL isSnapshot, BOOL canEdit, HeapID heapId);
TrainerCardParam *TrainerCardParam_CreateView(GameData *gameData, TrainerCardData *data, HeapID heapId);
u16 TrainerCardData_GetMedalCount(TrainerCardData *data);
u8 TrainerCardData_GetMedalRank(TrainerCardData *data);

// The proc that shows a card it is given, which it frees
extern const GameProcFunctions data_ov012_0216dd6c;

// Overlay 186, the trainer card screen
BOOL TrainerCard_Init(GameProc *proc, u32 *state, void *param, void *work);
BOOL TrainerCard_Main(GameProc *proc, u32 *state, void *param, void *work);
BOOL TrainerCard_Exit(GameProc *proc, u32 *state, void *param, void *work);
extern const GameProcFunctions BADGE_VIEW_PROC_FUNCTIONS;

#endif // POKEBW2_FIELD_TRCARD_SYS_H
