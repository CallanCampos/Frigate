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
    // Whether the card shows the Pokédex's window, from event flag 0x962
    u8 hasPokedex : 1;
    // TimeSigSave_HasSignature's
    u8 hasSignature : 1;
    // Whether the signature blinks (TimeSigSave_GetSignBlinks), which the pen button toggles
    u8 signBlinks : 1;
    // Set on a card received from another player: the pen and the back's buttons don't work, and the signature isn't
    // saved (TrainerCard_IsLocked)
    u8 isLocked : 1;
    u8 canEdit : 1;
    // Set on a card that TRCARD_SYS_VIEW_PROC_FUNCTIONS shows, with the Union Room appearance and the other buttons
    u8 isOtherCard : 1;
    u8 unk05;
    // The player's Union Room appearance (PlayerInfo_GetUnionView)
    u8 unionView;
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
    // The counts on the back, named after its labels. timesLinked to linkBattleLosses, linkTrades to trainersBattled,
    // feelingChecks and musicalPerformances are sums of records, from the rows of TrainerCardData_SumRecords's table
    u32 timesLinked;
    u32 linkBattles;
    u32 linkBattleWins;
    u32 linkBattleLosses;
    // The nature on the front (TimeSigSave_GetPersonality), which the player can change
    u32 personality;
    u32 linkTrades;
    u32 wildEncounters;
    u32 trainersBattled;
    u32 passerbyCount;
    u16 feelingChecks;
    u16 passPowerCount;
    u16 musicalPerformances;
    u16 palParkHighScore;
    u16 battleTestHighScore;
    // The Battle Test rank
    u16 battleTestRank;
    // Which of the back's rows show (TrCardBmp_InitRows): the C-Gear's two when it is on, and the others once the
    // player has something to show there
    u32 cgearOn : 1;
    u32 showPassPowers : 1;
    u32 showMusicals : 1;
    u32 showPalPark : 1;
    u32 showBattleTest : 1;
    u32 unk68_5 : 27;
    // The phrase the player greets with, which is also the C-Gear's
    PMSData greeting;
    // The signature, as TimeSigSave_GetSignature keeps it
    u8 signature[0x600];
    u32 unk674;
    // The medal obtained last, or 0xff
    u32 lastMedal;
    u16 medalCount;
    u8 medalRank;
    u8 medalYear;
    u8 medalMonth;
    u8 medalDay;
    u8 unk682[2];
    // The cleared Funfest Missions, the movie shoots, and the customers recommended to Join Avenue
    u32 funfestMissions;
    u16 movieShoots;
    u16 recommendedCustomers;
    // The tournaments won in the Pokémon World Tournament, up to 9999
    u32 pwtWins;
    // Which of the back's rows that only the player's own card has show
    u32 showFunfestMissions : 1;
    u32 showMovieShoots : 1;
    u32 showPwtWins : 1;
    u32 showAvenue : 1;
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
    // What the screen asks for next, a TRCARD_NEXT_*
    u32 next;
    // 1 when the medals screen asked to return to the field
    u32 result;
    // A TRCARD_START_*, or -1
    s32 appParam;
} TrainerCardParam;

// What the trainer card's screens ask for next: the end, the greeting's phrase select, overlay 186's badges, back to
// the card, or the medals
enum {
    TRCARD_NEXT_END,
    TRCARD_NEXT_GREETING,
    TRCARD_NEXT_BADGES,
    TRCARD_NEXT_CARD,
    TRCARD_NEXT_MEDALS,
};

// What the trainer card starts on: the card as it was last left, its front, its back, or the badges
enum {
    TRCARD_START_DEFAULT,
    TRCARD_START_FRONT,
    TRCARD_START_BACK,
    TRCARD_START_BADGES,
};

void TrainerCardData_FillExtras(TrainerCardData *data, GameData *gameData, HeapID heapId);
// isSnapshot is TRUE for a card that is sent away, with the play time copied
void TrainerCardData_Fill(TrainerCardData *data, GameData *gameData, BOOL isSnapshot, BOOL canEdit, HeapID heapId);
// The param of the card's own proc, whose canEdit is FALSE in the Union Room
TrainerCardParam *TrainerCardParam_Create(GameData *gameData, HeapID heapId, BOOL canEdit);
TrainerCardParam *TrainerCardParam_CreateView(GameData *gameData, TrainerCardData *data, HeapID heapId);
u16 TrainerCardData_GetMedalCount(TrainerCardData *data);
u8 TrainerCardData_GetMedalRank(TrainerCardData *data);

// The proc of the player's own card, which FIELD_PROC_LINK_LIST opens
extern const GameProcFunctions TRCARD_SYS_PROC_FUNCTIONS;
// The proc that shows a card it is given, which it frees
extern const GameProcFunctions TRCARD_SYS_VIEW_PROC_FUNCTIONS;

// Overlay 186, the trainer card screen
BOOL TrainerCard_Init(GameProc *proc, u32 *state, void *param, void *work);
BOOL TrainerCard_Main(GameProc *proc, u32 *state, void *param, void *work);
BOOL TrainerCard_Exit(GameProc *proc, u32 *state, void *param, void *work);
extern const GameProcFunctions BADGE_VIEW_PROC_FUNCTIONS;

#endif // POKEBW2_FIELD_TRCARD_SYS_H
