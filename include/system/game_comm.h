#ifndef POKEBW2_SYSTEM_GAME_COMM_H
#define POKEBW2_SYSTEM_GAME_COMM_H

#include "types.h"
#include "struct_decls.h"

// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except GameCommSys_Create, GameCommSys_Main,
// GameCommSys_FieldCreate, GameCommSys_FieldDelete, GameCommSys_Boot, GameCommSys_IsTransitioning,
// GameCommSys_GetWork, GameCommSys_GetLastCommNo and GameCommSys_ExitReq

// The game's communication system (game_comm.c): it runs one communication at a time, identified by a GameCommNo
enum GameCommNo {
    GAME_COMM_NO_NULL,
    // The field's beacon search (overlay 12's game_beacon_search.c)
    GAME_COMM_NO_BEACON_SEARCH,
    // Entralink (Intrude_GetCommWork), which has no callbacks of its own here
    GAME_COMM_NO_INTRUDE,
    // The Union Room (overlay 28)
    GAME_COMM_NO_UNION,
    // The musical (overlay 211)
    GAME_COMM_NO_MUSICAL,
    // Runs the beacon search's callbacks too
    GAME_COMM_NO_UNK5,
    GAME_COMM_NO_MAX,
};

// How many players GameCommSys keeps in its log of the last ones met
#define GAME_COMM_LOG_COUNT 6

// Called when a communication has ended. exitReqAgain is whether an exit was requested again while it was exiting
typedef void (*GameCommExitCallback)(void *arg, BOOL exitReqAgain);

GameCommSys *GameCommSys_Create(HeapID heapId, GameData *gameData);
void FreeGameComm(GameCommSys *comm);
// Runs the current communication's callbacks; called once a frame
void GameCommSys_Main(GameCommSys *comm);
// Call the current communication's callbacks for the field being created and deleted
void GameCommSys_FieldCreate(GameCommSys *comm, Field *field);
void GameCommSys_FieldDelete(GameCommSys *comm, Field *field);
// Starts a communication, which gets param in its callbacks
void GameCommSys_Boot(GameCommSys *comm, u8 commNo, void *param);
void GameCommSys_ExitReq(GameCommSys *comm);
// The running communication's GameCommNo, or GAME_COMM_NO_NULL
u8 GameCommSys_BootCheck(GameCommSys *comm);
// Whether a communication is running but booting or exiting
BOOL GameCommSys_IsTransitioning(GameCommSys *comm);
// The running communication's work
void *GameCommSys_GetWork(GameCommSys *comm);
GameData *getBasePlayerBlk(GameCommSys *comm);
// The GameCommNo of the communication booted last
u8 GameCommSys_GetLastCommNo(GameCommSys *comm);
void func_0202be00(GameCommSys *comm);
u32 func_0202be08(GameCommSys *comm);
void func_0202be14(GameCommSys *comm, BOOL flag);
void func_0202be28(GameCommSys *comm, BOOL flag);
// Whether either of the flags of func_0202be14 and func_0202be28 is set
BOOL func_0202be3c(GameCommSys *comm);
// Logs two players met over a communication, by net ID: as beacon type 0x27 if they are the same, else 0x26
void func_0202bf68(GameCommSys *comm, u32 netId1, u32 netId2);
// Logs the player, as beacon type 0x37
void func_0202bf7c(GameCommSys *comm);

#endif // POKEBW2_SYSTEM_GAME_COMM_H
