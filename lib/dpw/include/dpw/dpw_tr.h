#ifndef POKEBW2_DPW_DPW_TR_H
#define POKEBW2_DPW_DPW_TR_H

#include "types.h"
#include "struct_decls.h"

// The Global Trade Station's server library in overlay 189. The ROM has no strings of this library's own (only the
// server's pages, "/web/worldexchange/*.asp"), so the DpwTr_ names of its functions, the header's name and the type and
// field names are guesses, the functions' after the pages they ask for

// What a player looks for, or wants for the Pokémon they deposit
typedef struct {
    s16 characterNo;
    s8 gender;
    s8 level_min;
    s8 level_max;
    s8 unused;
} Dpw_Tr_PokemonSearchData;

// A search with a country, and how many results to return
typedef struct {
    s16 characterNo;
    s8 gender;
    s8 level_min;
    s8 level_max;
    s8 unused;
    s8 maxNum;
    u8 countryCode;
} Dpw_Tr_PokemonSearchDataEx;

// What a deposited Pokémon is, for searches
typedef struct {
    s16 characterNo;
    s8 gender;
    s8 level;
} Dpw_Tr_PokemonDataSimple;

// A deposited Pokémon on the server
typedef struct {
    // The Pokémon, a party Pokémon
    u8 postData[0xec];
    Dpw_Tr_PokemonDataSimple postSimple;
    Dpw_Tr_PokemonSearchData wantSimple;
    // The trainer's
    u8 gender;
    u8 unkF7;
    u8 postDate[8];
    u8 tradeDate[8];
    s32 id;
    u32 trainerID;
    u16 name[8];
    u8 countryCode;
    u8 localCode;
    u8 trainerType;
    // Whether the Pokémon was traded
    s8 isTrade;
    u8 versionCode;
    u8 langCode;
    u8 unk126;
    u8 unk127;
} Dpw_Tr_Data;

// The player's profile, which the server keeps with their trades
typedef struct {
    u8 version;
    u8 language;
    u8 country;
    u8 region;
    u32 playerId;
    u16 playerName[8];
    u32 unk18;
    u8 unk1C[8];
    // An e-mail address, which the server confirms (see Dpw_Common_ProfileResult)
    char mailAddr[56];
    u32 unk5C;
    u8 unk60[4];
} Dpw_Common_Profile;

typedef struct {
    int code;
    int mailAddrAuthResult;
} Dpw_Common_ProfileResult;

// Starts and ends the library, for the player's ID and friend key
void DpwTr_Init(s32 pid, u64 friendKey, int a2);
void DpwTr_End(void);
// Runs the library's requests, every frame
void DpwTr_Process(void);
// Whether the last request has ended, and its result
BOOL DpwTr_IsFinished(void);
s32 DpwTr_GetResult(void);
// Requests the server's state, and sends the player's profile
void DpwTr_GetServerState(void);
void DpwTr_SetProfile(Dpw_Common_Profile *profile, Dpw_Common_ProfileResult *result);
// Searches the server for up to maxNum Pokémon, without and with a country
void DpwTr_Search(const Dpw_Tr_PokemonSearchData *search, s32 maxNum, Dpw_Tr_Data *result);
void DpwTr_SearchEx(const Dpw_Tr_PokemonSearchDataEx *search, Dpw_Tr_Data *result);
// Deposits a Pokémon with its signature, and ends the deposit
void DpwTr_Upload(Dpw_Tr_Data *data, const void *sign, u32 size);
void DpwTr_UploadFinish(void);
// Asks for the player's deposited Pokémon, and whether it was traded
void DpwTr_Download(Dpw_Tr_Data *result);
void DpwTr_GetTradeResult(Dpw_Tr_Data *result);
// End the taking back of a traded Pokémon, and of a deposited one
void DpwTr_DeleteFinish(void);
void DpwTr_ReturnFinish(void);
// Cancels the request
void DpwTr_Cancel(void);
// Trades for the Pokémon of the given ID, and ends the trade
void DpwTr_Exchange(s32 id, Dpw_Tr_Data *upload, Dpw_Tr_Data *result, const void *sign, u32 size);
void DpwTr_ExchangeFinish(void);

#endif // POKEBW2_DPW_DPW_TR_H
