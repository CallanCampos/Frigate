#ifndef POKEBW2_BATTLE_BTL_MAIN_H
#define POKEBW2_BATTLE_BTL_MAIN_H

#include "types.h"
#include "battle/btl_calc.h"
#include "battle/btl_rec.h"
#include "battle/btl_setup.h"
#include "constants/battle.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// What IsAdjacentOpponent looks up for a position
struct AdjacentOpponentData {
    u16 count1;
    u16 count2;
    u8 list1[3];
    u8 list2[3];
};

// The battle positions, 0 to 5. Even positions are on one side and odd positions on the other
#define BTL_POS_MAX 6

// Layout reconstructed from the party accessors in the game code.
struct BattleParty {
    BattleMon *mons[6];
    u8 count;
    u8 unk19[3];
};

// The parties of the four clients and every BattleMon by mon ID, 0xe8 bytes as the main module holds two
struct BtlPokeCon {
    BtlMainModule *mainModule;
    BattleParty parties[4];
    // The parties the clients' Pokémon come from
    PokeParty *srcParties[4];
    BattleMon *mons[24];
    u32 unkE4;
};

// A buffer of the main module that BtlMain_LoadStudioData allocates
typedef struct {
    u8 unk00[8];
    s8 unk08;
    u8 unk09[0xb];
} BtlMainUnk478;

// A step of the main module's setup or cleanup, run until it returns TRUE
typedef BOOL (*BtlMainSeqFunc)(u32 *state, BtlMainModule *mainModule);

typedef struct {
    BtlMainSeqFunc func;
    BtlMainSeqFunc nextFunc;
    BtlMainModule *mainModule;
    u32 state;
} BtlMainSeq;

// What the clients of a link battle exchange before it starts
struct BtlMainSyncData {
    MATHRandContext32 rand;
    u16 unk18;
    u16 unk1A;
    u16 unk1C;
    u8 unk1E;
    u8 unk1F_0 : 4;
    u8 unk1F_4 : 4;
};

// A client's trainer, 0x28 bytes
typedef struct {
    PlayerInfo *playerInfo;
    StrBuf *name;
    u16 unk08;
    u16 unk0A;
    u32 unk0C;
    u16 unk10[4];
    PMSData unk18;
    PMSData unk20;
} BtlTrainerData;

// A choice the player is offered in a Pokestar Studios scene
typedef struct {
    // -1 for none
    s16 msgId;
    s16 msgId2;
    s16 unk04;
    s16 unk06;
    s16 points;
    s16 unk0A;
} BtlStudioChoice;

// One entry of a Pokestar Studios movie's script
typedef struct {
    s16 unk00;
    s16 unk02;
    s16 unk04;
    s16 unk06;
    s16 unk08;
    s16 unk0A;
    // The messages of two kinds, by step, which the cut-ins' message command picks between
    s16 messages[2][10];
    // The script event each step waits for, -1 for none
    s16 events[10];
    BtlStudioChoice choices[4];
} BtlStudioScene;

typedef struct {
    s16 turn;
    s16 value;
} BtlStudioTurnEvent;

// The rules of a scripted battle, which seem to be the Pokestar Studios movies
typedef struct {
    s16 movie;
    u8 unk02[2];
    s16 unk04;
    s16 unk06;
    // Which of the studio's message files the movie uses
    s16 unk08;
    s16 turnLimit;
    s16 unk0C;
    s16 unk0E;
    s16 rule;
    s16 species1;
    s16 species2;
    s16 unk16;
    u8 unk18[0xc];
    // btlv_effect.c passes it to the field
    s16 unk24;
    u8 unk26[0xa];
    BtlStudioTurnEvent turnEvents[3];
    u8 unk3C[0x6c];
    // Messages by turn
    s16 unkA8[20];
    s16 unkD0[20];
    BtlStudioScene scenes[];
} BtlScriptedRules;

struct BtlMainModule {
    BtlSetup *setup;
    BtlvCore *viewCore;
    BtlServer *server;
    BtlServer *unk0C;
    BtlClient *clients[4];
    BtlTrainerData trainers[4];
    PlayerInfo *unkC0;
    u8 unkC4[4];
    BtlPokeCon pokeCons[2];
    PokeParty *unk298[4];
    PokeParty *unk2A8[4];
    PokeParty *unk2B8;
    BtlField *field;
    PartyPkm *unk2C0;
    void *unk2C4;
    BtlRecReader recReader;
    void *unk3E0[4];
    MATHRandContext32 rand;
    // What the clients of a link battle agree on before it starts
    BtlMainSyncData syncData;
    u8 posClientIds[6];
    u8 unk42e[2];
    // The prize money, which FixPrizeMoney doubles and finalizes
    u32 prizeMoney;
    u32 money;
    u32 unk438;
    u32 unk43C;
    u16 unk440;
    u16 unk442;
    u32 result;
    BtlClientIDList unk448;
    BtlMainSeq seq;
    s32 unk460;
    BOOL (*mainFunc)(BtlMainModule *mainModule);
    u16 heapId;
    u8 unk46A;
    u8 clientCount;
    u8 playerClientId;
    u8 unk46D;
    u8 unk46E;
    u8 unk46F;
    u8 unk470;
    u8 unk471;
    u8 unk472;
    u8 unk473_0 : 1;
    u8 unk473_1 : 1;
    u8 unk473_2 : 1;
    u8 unk473_3 : 1;
    u8 unk473_4 : 1;
    u8 unk473_5 : 1;
    u8 unk473_6 : 1;
    u8 unk473_7 : 1;
    BtlScriptedRules *unk474;
    BtlMainUnk478 *unk478;
    PokewoodCutin *cutin;
};

// Swan's names for these two take the main module, whose first field points to the BtlSetup
u32 BtlSetup_GetBattleStyle(BtlMainModule *mainModule);
u32 IsBattleSceneOff(BtlMainModule *mainModule);
u8 GetBadgeCount(BtlMainModule *mainModule);
u32 BtlSetup_GetBattleType(BtlMainModule *mainModule);
u8 GetMultiMode(BtlMainModule *mainModule);
u8 GetCommMode(BtlMainModule *mainModule);
BOOL IsMultiMode(BtlMainModule *mainModule);
u16 GetWinBGM(BtlMainModule *mainModule);
u16 func_ov167_0219bf08(BtlMainModule *mainModule);
u16 func_ov167_0219bf14(BtlMainModule *mainModule);
u32 BtlSetup_IsBattleType(BtlMainModule *mainModule, u32 flag);
u8 IsRecPlayMode(BtlMainModule *mainModule);
void func_ov167_0219c9fc(BtlMainModule *mainModule, u8 pos);
void SetBattleResult(BtlMainModule *mainModule, u32 result);
u32 GetBattleResult(BtlMainModule *mainModule);
void AddBonusMoney(BtlMainModule *mainModule, u32 money);
void SetMoneyDoubled(BtlMainModule *mainModule);
void IncRecord(BtlMainModule *mainModule, u32 record);
BOOL IsGameTimeUp(BtlMainModule *mainModule);
BOOL func_ov167_0219df50(BtlMainModule *mainModule);
u32 GetStudioMode(BtlMainModule *mainModule);
BOOL IsSwitchMode(BtlMainModule *mainModule);
u32 GetValidPosMax(BtlMainModule *mainModule);
u32 GetActivePosCount(BtlMainModule *mainModule);
BOOL IsActivePos(BtlMainModule *mainModule, u32 pos);
u32 GetRunMode(BtlMainModule *mainModule);
BtlFieldSituation *GetFieldEffectData(BtlMainModule *mainModule);

// The position at index on the same side as pos
u8 GetPosOnSameSide(u8 pos, u8 index);
BOOL DoesClientExist(BtlMainModule *mainModule, u8 clientId);
u8 GetClientSide(BtlMainModule *mainModule, u8 clientId);
BOOL AreClientsOnOppositeSides(BtlMainModule *mainModule, u8 clientId1, u8 clientId2);
u8 BattlePosToClientID(BtlMainModule *mainModule, u8 pos);
u8 MonIDToClientID(u8 monId);
u8 GetClientBattlePos(BtlMainModule *mainModule, u8 clientId, u8 slot);
s32 PokeCon_FindClientMonIndex(BtlPokeCon *pokeCon, u8 clientId, u8 monId);
u8 MonIDToBattlePos(BtlMainModule *mainModule, BtlPokeCon *pokeCon, u8 monId);
u8 ExpandPosFlags(BtlMainModule *mainModule, u16 posFlags, u8 *out);
u8 GetOpposingPos(BtlMainModule *mainModule, u8 pos, u8 index);
u8 ExpandPosFlagsToMonIDs(BtlMainModule *mainModule, BtlPokeCon *pokeCon, u16 posFlags, u8 *monIds);
u8 GetClientIDFromMonID(u8 monId);
u8 GetPosIndexInClient(BtlMainModule *mainModule, u8 pos);
u8 GetPlayerClientID(BtlMainModule *mainModule);
u8 GetPlayerAllyClientID(BtlMainModule *mainModule);
u8 GetAllyClientID(BtlMainModule *mainModule, u8 clientId);
BOOL IsAllyClientID(u8 clientId1, u8 clientId2);
u8 GetPlayerOpponentClientID(BtlMainModule *mainModule, u8 other);
u32 GetOpponentClientID(BtlMainModule *mainModule, u8 clientId, u8 other);
void BattleClient_SubItem(BtlMainModule *mainModule, u8 clientId, u16 item);
void BattleClient_AddItem(BtlMainModule *mainModule, u8 clientId, u16 item);
void ChangeFriendshipWhenFainted(BtlMainModule *mainModule, BattleMon *mon, BOOL reason);
void ChangeFriendship(BtlMainModule *mainModule, BattleMon *mon, u32 reason);
u8 GetClientIDFromPos(BtlMainModule *mainModule, u8 pos);
BattleMon *PokeCon_GetPosMonInner(const BtlPokeCon *pokeCon, u8 pos);
BattleMon *GetClientMonData(BtlPokeCon *pokeCon, u8 clientId, u8 monId);
BattleMon *GetPokeParam(BtlPokeCon *pokeCon, u8 monId);
const BattleMon *GetPokeParamConst(const BtlPokeCon *pokeCon, u8 monId);
// Underlying battle-type-dependent count used by GetClientBattlerCount.
u8 CalcClientBattlerCount(BtlMainModule *mainModule, u8 clientId);
// How many of the client's Pokemon are in battle at once. They come first in the party, so this is also the index of
// the first Pokemon waiting to switch in
u8 GetClientBattlerCount(BtlMainModule *mainModule, u8 clientId);
BOOL IsAllyMonID(u8 monId1, u8 monId2);
// The side of the battle a mon ID is on: IDs from 12 are the second side's
typedef enum { BTL_SIDE_1ST, BTL_SIDE_2ND } BtlSide;
u8 GetSideFromMonID(u8 monId);
u8 GetSideFromOpposingMonID(u8 monId);
u8 GetOpposingSide(u8 side);
// Returns the opponent-position lookup entry for a battle position.
const AdjacentOpponentData *GetAdjacentOpponentData(u8 pos);
// Whether pos2 is an opponent next to pos1 in a triple battle
BOOL IsAdjacentOpponent(u8 pos1, u8 pos2);
BattleParty *GetClientParty(BtlPokeCon *pokeCon, u32 clientId);
BattleParty *GetPartyData(BtlPokeCon *pokeCon, u32 clientId);

s32 FindPartyMon(const BattleParty *party, BattleMon *mon);
s32 GetPartyPkmnEligibleForBattle(PokeParty *party);
void AddBattleMonToParty(BattleParty *party, BattleMon *mon);
void BattleParty_Clear(BattleParty *party);
void BattleParty_MoveFaintedToEnd(BattleParty *party);
u8 BattleParty_CountAliveFrom(BattleParty *party, s32 index);
BattleMon *BattleParty_GetMon(BattleParty *party, u8 index);
void BattleParty_Swap(BattleParty *party, u32 first, u32 second);
void BattleParty_MoveToEnd(BattleParty *party, u8 index);
void BattleParty_Rotate(BattleParty *party, u32 pos, BattleMon **oldOut, BattleMon **newOut);
s32 BattleParty_FindMonID(const BattleParty *party, u32 monId);
BattleMon *BattleParty_GetFirstAlive(const BattleParty *party);
u8 GetRotationInSlot(u32 pos);
u32 GetRotationOutSlot(u32 pos);
BattleMon *GetBattleMonFromParty(const BattleParty *party, u8 index);
u8 GetNumMonsInParty(const BattleParty *party);
u8 GetAlivePartyCount(BattleParty *party);

u8 func_ov167_0219a004(BtlSetup *setup);
void BtlMain_SetupPlayerAndParties(BtlMainModule *mainModule, BtlSetup *setup);
void BtlMain_LoadSetupParties(BtlMainModule *mainModule, BtlSetup *setup);
void BtlMain_ForceQuitClients(BtlMainModule *mainModule);
BOOL BtlMain_AreAllClientsDone(BtlMainModule *mainModule);
s32 GetTextWait(BtlMainModule *mainModule);
void func_ov167_0219bdf0(BtlMainModule *mainModule);
BOOL func_ov167_0219bdfc(BtlMainModule *mainModule);
void *GetCommPlayerSupport(BtlMainModule *mainModule);
PlayerInfo *GetPlayerInfo(BtlMainModule *mainModule);
BOOL IsMonCaught(BtlMainModule *mainModule, BattleMon *mon);
u32 GetDexCaughtCount(BtlMainModule *mainModule);
GameData *GetGameData(BtlMainModule *mainModule);
void RegisterDexMon(BtlMainModule *mainModule, u8 clientId, BattleMon *mon);
u8 ExpandPosFlagsSingle(BtlMainModule *mainModule, u8 type, u8 pos, u8 *out);
u8 ExpandPosFlagsDouble(BtlMainModule *mainModule, u8 type, u8 pos, u8 *out);
u8 ExpandPosFlagsTriple(BtlMainModule *mainModule, u8 type, u8 pos, u8 *out);
u32 GetClientPosCount(BtlMainModule *mainModule, u8 clientId);
BOOL IsPlayerSide(BtlMainModule *mainModule, u8 side);
u8 GetOpposingPosForStyle(u32 battleStyle, u8 pos, u8 index);
u8 GetFacingPos(u32 battleStyle, u8 pos);
u8 GetPartnerPos(BtlMainModule *mainModule, u8 pos);
u8 MonIDToViewPos(BtlMainModule *mainModule, BtlPokeCon *pokeCon, u8 monId);
void GetPosClientAndIndex(BtlMainModule *mainModule, u8 pos, u8 *clientId, u8 *index);
u8 BattlePosToViewPos(BtlMainModule *mainModule, u8 pos);
u8 ViewPosToBattlePos(BtlMainModule *mainModule, u8 index);
BOOL func_ov167_0219c7a4(BtlMainModule *mainModule);
PokeParty *GetClientPokeParty(BtlMainModule *mainModule, u8 clientId);
PokeParty *GetAllyPokeParty(BtlMainModule *mainModule, u8 clientId);
u8 GetMultiTrainerIndex(BtlMainModule *mainModule, u8 clientId);
u8 GetPlayerMultiTrainerIndex(BtlMainModule *mainModule);
BagSave *GetBagSave(BtlMainModule *mainModule);
void *GetBagCursor(BtlMainModule *mainModule);
BOOL IsRecordingEnabled(BtlMainModule *mainModule);
u32 func_ov167_0219c99c(BtlMainModule *mainModule);
u8 GetStudioMovieId(BtlMainModule *mainModule);
u32 func_ov167_0219c9c0(BtlMainModule *mainModule);
void func_ov167_0219c9d4(BtlMainModule *mainModule);
u32 func_ov167_0219c9e0(BtlMainModule *mainModule);
u32 ReturnZero(BtlMainModule *mainModule, u32 flag);
void SetCmdCheckError(BtlMainModule *mainModule);
u32 FixPrizeMoney(BtlMainModule *mainModule);
u32 GetBonusMoney(BtlMainModule *mainModule);
u32 CalcLossMoney(BtlMainModule *mainModule);
void SkipExitBGMFadeOut(BtlMainModule *mainModule);
void SyncServerMonMoves(BtlMainModule *mainModule, u8 monId);
BattleMon *GetIllusionDisguise(BtlMainModule *mainModule, BtlPokeCon *pokeCon, BattleMon *mon);
void PokeCon_Init(BtlPokeCon *pokeCon, BtlMainModule *mainModule, u32 unkE4);
void PokeCon_Clear(BtlPokeCon *pokeCon);
void PokeCon_CreateClientParty(BtlPokeCon *pokeCon, BtlMainModule *mainModule, u8 clientId);
void PokeCon_FreeMons(BtlPokeCon *pokeCon);
void PokeCon_StoreClientSrcParty(BtlPokeCon *pokeCon, BtlMainModule *mainModule, u8 clientId, BOOL keepBaseForm);
void PokeCon_StoreClientSrcPartyInOrder(BtlPokeCon *pokeCon, BtlMainModule *mainModule, u8 clientId);
PokeParty *PokeCon_GetSrcParty(BtlPokeCon *pokeCon, u8 clientId);
BattleMon *PokeCon_GetPosMon(BtlPokeCon *pokeCon, u8 pos);
BattleMon *PokeCon_GetClientPartyMon(BtlPokeCon *pokeCon, u8 clientId, u8 index);
BOOL GetViewPosChatter(BtlMainModule *mainModule, u32 index, void **out);
u8 GetClientChatterEffect(BtlMainModule *mainModule, u8 clientId);
u8 GetClientBenchStart(BtlMainModule *mainModule, u8 clientId);
BOOL IsTripleCenterPos(u8 pos);
BOOL GetNonAdjacentOpponentPos(u8 pos, u8 *out);
u8 GetPosSide(u8 pos);
u32 GetServerTurnCount(BtlMainModule *mainModule);
u8 PokeCon_GetClientAliveCount(BtlPokeCon *pokeCon, u8 clientId);
void AddClientShooterEnergy(BtlMainModule *mainModule, u8 clientId, u8 value);
void UpdatePartyIllusionDisguises(BtlMainModule *mainModule, BattleParty *party, u8 clientId);
void BtlMain_InitTrainers(BtlMainModule *mainModule);
void BtlMain_FreeTrainers(BtlMainModule *mainModule);
void TrainerData_SetFromPlayer(BtlTrainerData *trainer, HeapID heapId, PlayerInfo *src);
BOOL IsClientNPCTrainer(BtlMainModule *mainModule, u8 clientId);
u16 GetClientTrainerItem(BtlMainModule *mainModule, u8 clientId, u8 index);
StrBuf *GetClientTrainerName(BtlMainModule *mainModule, u8 clientId, u32 *trainerClass);
u32 GetClientAIFlags(BtlMainModule *mainModule, u8 clientId);
u16 GetClientTrainerID(BtlMainModule *mainModule, u8 clientId);
u32 GetClientTrainerClass(BtlMainModule *mainModule, u8 clientId);
PMSData *GetClientTrainerMsg(BtlMainModule *mainModule, u8 clientId, u32 which);
PlayerInfo *GetClientPlayerInfo(BtlMainModule *mainModule, u8 clientId);
PlayerInfo *GetCommSupporterInfo(BtlMainModule *mainModule);
BtlField *GetBtlField(BtlMainModule *mainModule);
void BtlMain_CreateSrcParties(BtlMainModule *mainModule);
void BtlMain_FreeSrcParties(BtlMainModule *mainModule);
void SetClientSrcParty(BtlMainModule *mainModule, u8 clientId, const PokeParty *party);
PokeParty *GetClientSrcParty(BtlMainModule *mainModule, u8 clientId, u32 useSecond);
void AddRecord(BtlMainModule *mainModule, u32 record, u32 value);
void *func_ov167_0219db00(BtlMainModule *mainModule);
BOOL func_ov167_0219db08(BtlMainModule *mainModule);
BOOL func_ov167_0219db28(BtlMainModule *mainModule);
void NotifyMonLevelUp(BtlMainModule *mainModule, BattleMon *mon);
void ChangeFriendshipByItem(BtlMainModule *mainModule, BattleMon *mon, u16 item);
void NotifyMonCaught(BtlMainModule *mainModule, BattleMon *mon);
void BtlMain_StorePartiesToSetup(BtlMainModule *mainModule);
BOOL func_ov167_0219de6c(BtlMainModule *mainModule);
u32 BtlMain_DetermineResult(BtlMainModule *mainModule);
u16 GetCommandTimeLimit(BtlMainModule *mainModule);
u16 GetGameTimeLimit(BtlMainModule *mainModule);
void SetSetupParty(BtlMainModule *mainModule, u8 clientId, const PokeParty *party);
void SetSetupPlayerInfo(BtlMainModule *mainModule, u8 clientId, const PlayerInfo *src);
void SetSetupChatterEffect(BtlMainModule *mainModule, u8 clientId);
void BtlMain_StoreRecData(BtlMainModule *mainModule);
u8 ClientIDToSetupIndex(u8 playerClientId, u8 clientId);
void BtlMain_StartRecSkip(BtlMainModule *mainModule, u32 arg1);
void BtlMain_EndRecSkip(BtlMainModule *mainModule);
BtlServerFlow *GetServerFlow(BtlMainModule *mainModule);
void BtlSetup_ClearResults(BtlSetup *setup);
void BtlMain_StoreResultStats(BtlMainModule *mainModule);
u32 func_ov167_0219e300(BtlMainModule *mainModule);
BtlSetup *GetBattleSetup(BtlMainModule *mainModule);
BtlSetup *GetBattleSetupParam(BtlMainModule *mainModule);
void BtlMain_LoadStudioData(BtlMainModule *mainModule, u8 arg1);
void BtlMain_FreeStudioData(BtlMainModule *mainModule);
BtlScriptedRules *GetStudioRules(BtlMainModule *mainModule);
void *func_ov167_0219e3ac(BtlMainModule *mainModule);
PokewoodCutin *GetPokewoodCutin(BtlMainModule *mainModule);
void func_ov167_0219e3c8(void *data);

BOOL BtlMain_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
BOOL BtlMain_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
BOOL BtlMain_IsRecReadComplete(BtlMainModule *mainModule);
BOOL BtlMain_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
void BtlMain_SetSetupSeq(BtlMainSeq *seq, BtlMainModule *mainModule, BtlSetup *setup);
void BtlMain_SetCleanupSeq(BtlMainSeq *seq, BtlMainModule *mainModule, BtlSetup *setup);
void BtlMain_SetupClientTrainer(BtlMainModule *mainModule, u8 clientId);
void BtlMain_SetupTrainers(BtlMainModule *mainModule, BtlSetup *setup);
void BtlMain_SetupRecReader(BtlMainModule *mainModule, BtlSetup *setup);
void BtlMain_CreateViewCore(BtlMainModule *mainModule, BtlSetup *setup, u32 arg2);
BOOL BtlMain_SetupAloneSingle(u32 *state, BtlMainModule *mainModule);
BOOL BtlMain_Cleanup(u32 *state, BtlMainModule *mainModule);
BOOL BtlMain_SetupAloneDouble(u32 *state, BtlMainModule *mainModule);
BOOL BtlMain_SetupAloneMulti(u32 *state, BtlMainModule *mainModule);
BOOL BtlMain_SetupAloneTriple(u32 *state, BtlMainModule *mainModule);
BOOL BtlMain_SetupAloneRotation(u32 *state, BtlMainModule *mainModule);
BOOL BtlMain_SetupCommSingle(u32 *state, BtlMainModule *mainModule);
BOOL BtlMain_SetupCommDouble(u32 *state, BtlMainModule *mainModule);
BOOL BtlMain_SetupCommRotation(u32 *state, BtlMainModule *mainModule);
BOOL BtlMain_SetupCommTriple(u32 *state, BtlMainModule *mainModule);
BOOL BtlMain_CommExchangeSyncData(BtlMainModule *mainModule, s32 *state);
BOOL BtlMain_CommExchangeParties(BtlMainModule *mainModule, s32 *state);
BOOL BtlMain_CommExchangeTrainers(BtlMainModule *mainModule, s32 *state);
BOOL BtlMain_CommExchangeChatter(BtlMainModule *mainModule, s32 *state);
BOOL BtlMain_CommCreateSingle(BtlMainModule *mainModule, s32 *state);
BOOL BtlMain_CommCreateDouble(BtlMainModule *mainModule, s32 *state);
BOOL BtlMain_CommCreateTriple(BtlMainModule *mainModule, s32 *state);
BOOL BtlMain_CommCreateRotation(BtlMainModule *mainModule, s32 *state);
BOOL BtlMain_CommCreateViewCore(BtlMainModule *mainModule, s32 *state);
BOOL BtlMain_MainLoopAlone(BtlMainModule *mainModule);
BOOL BtlMain_MainLoopCommServer(BtlMainModule *mainModule);
BOOL BtlMain_MainLoopCommClient(BtlMainModule *mainModule);
void TrainerData_SetFromSetup(BtlTrainerData *trainer, const BtlSetupTrainer *src);
void TrainerData_SetFromComm(BtlTrainerData *trainer, const BtlCommTrainerData *src);

#endif // POKEBW2_BATTLE_BTL_MAIN_H
