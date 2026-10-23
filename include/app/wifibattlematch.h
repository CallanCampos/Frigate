#ifndef POKEBW2_APP_WIFIBATTLEMATCH_H
#define POKEBW2_APP_WIFIBATTLEMATCH_H

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "save/player_info.h"
#include "struct_decls.h"
#include "system/pms_data.h"

// Overlay 290's wifibattlematch_sys.c (named by its embedded string): the Battle Competition, which chains the Wi-Fi
// login, the Wi-Fi (overlay 260) or infrared (overlay 261) link, the team selection and exchange of overlay 263, the
// battle and overlay 306's result screen. It runs from the title screen's menu or from the field

#define OVERLAY_WIFIBATTLEMATCH OVERLAY_ID(290)

// A player as the two machines exchange it, which overlay 290 keeps for each side and the Wi-Fi link (overlay 260) and
// the live competition (overlay 261) fill and send whole: the trainer info, the last results, what the records and the
// tournament say, and the team picked with the signature that the check of the Pokémon made
struct WifiBattleMatchPlayer {
    PlayerInfo info;
    // The console's address, which the live competition keeps of the players it met
    u8 mac[6];
    u8 unk26[2];
    // The wins, losses and battles played of the record: the live competition's LIVEMATCH_COUNT_WON,
    // LIVEMATCH_COUNT_LOST and LIVEMATCH_COUNT_PLAYED, or the random matches' RNDMATCH_FIELD_WIN and
    // RNDMATCH_FIELD_LOSE
    u32 record1;
    u32 record2;
    u32 record0;
    u32 unk34;
    PMSData sentence;
    // The cup ID of the player's entry in the tournament block (TOURNAMENT_FIELD_CUP_ID), which the two players compare
    u32 entryField2;
    // 100, which overlay 290 and the live competition check with BattleRec_IsVersionSupported
    u32 unk44;
    s32 gsid;
    // The player's record ID in the Sake database, which the Wi-Fi competition reads the other player's team by
    u32 unk4C;
    u32 checksum;
    u8 signature[0x80];
    // The team picked, as a PokeParty's save data
    u8 partyData[];
};

// What the battle left, for the steps that follow it
typedef struct {
    // How the battle ended, 0 to 4
    u32 result;
    u32 battleStyle;
    // The other player's Pokémon left standing and the percentage of their HP left, and the player's Pokémon left
    // standing, which the live competition keeps and shows
    u8 aliveOther;
    u8 otherHpPercent;
    u8 aliveOwn;
    // Set when the other player left or the link failed, so that the live competition keeps to the menus that need none
    BOOL disconnected;
    // The player's value of the report, which the two machines exchange, and the other player's
    u32 unk10;
    u32 unk14;
} WifiBattleMatchBattleResult;

// What a battle sends to the ranking: the other player's team, then the player's own
typedef struct {
    // The other player's
    s32 profileId;
    s32 gsid;
    u16 species[6];
    u8 forms[6];
    u8 levels[6];
    u8 genders[6];
    u16 ownSpecies[6];
    u8 ownForms[6];
    u8 ownLevels[6];
    u8 ownGenders[6];
    u32 year : 7;
    u32 month : 4;
    u32 day : 5;
    u32 hour : 5;
    u32 minute : 6;
    u32 unk : 4;
    u32 unkFlag : 1;
    u32 unk48;
    u8 result;
    u8 battleType;
    u8 aliveOwn : 4;
    u8 aliveOther : 4;
} WifiBattleMatchReport;

typedef struct {
    // The game data to play with; the title screen's menu has none, so the proc makes one and resets the game at the
    // end
    GameData *gameData;
    // The battle type, from 0 to 4 (single, double, triple, rotation, and another triple); picks the regulation
    u32 unk4;
    // 0 when started from the title screen's menu, 1 when started from the field
    u32 unk8;
    // Where the team comes from, 0 for the party and 1 for the battle box
    u32 unkC;
    // Set when the proc has to free the parameters itself
    u32 unk10;
} WifiBattleMatchParam;

extern const GameProcFunctions WIFIBATTLEMATCH_PROC_FUNCTIONS;

#endif // POKEBW2_APP_WIFIBATTLEMATCH_H
