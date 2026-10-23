#ifndef POKEBW2_APP_MB_PARENT_MB_COMM_SYS_H
#define POKEBW2_APP_MB_PARENT_MB_COMM_SYS_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The connection to the downloaded child once it has booted (mb_comm_sys.c), which the parent sends a program to, and
// receives Pokémon or, for the Memory Link, save data from. The names are ours

// The commands of the main packet, which carries a command and a value. Both sides send them; the names say what
// the child means by those the parent receives
enum {
    MB_COMM_CMD_ACK,
    MB_COMM_CMD_SAVE_SYNC_1,
    MB_COMM_CMD_SAVE_SYNC_2,
    MB_COMM_CMD_SAVE_SYNC_3,
    MB_COMM_CMD_SAVE_SYNC_4,
    MB_COMM_CMD_BOX_SPACE,
    MB_COMM_CMD_STATE = 10,
    MB_COMM_CMD_SAVE_READY,
    MB_COMM_CMD_SAVE_START,
    MB_COMM_CMD_SAVE_MID,
    MB_COMM_CMD_SAVED,
    MB_COMM_CMD_ANSWER,
    MB_COMM_CMD_ITEM_ANSWER,
    MB_COMM_CMD_FINISH,
    MB_COMM_CMD_END,
    MB_COMM_CMD_RESULT,
    MB_COMM_CMD_MORE,
    MB_COMM_CMD_ITEM,
    MB_COMM_CMD_CLOSE,
};

// The states the child tells the parent (MB_COMM_CMD_STATE, MBComm_GetState), named after what the parent does on
// them: the child asks for the program; runs it while the player picks Pokémon; starts the Poké Transfer game; has
// sent Pokémon; has the parent's ack and waits for the save; ended the game with none to store (a Pal Park result of
// PAL_PARK_RESULT_NONE_TRANSFERRED); is done; failed, in one of three ways that the Memory Link tells apart; or was
// quit while the program was sent (PAL_PARK_RESULT_CANCELED)
enum {
    MB_COMM_STATE_REQUEST_PROGRAM = 2,
    MB_COMM_STATE_SELECT_POKEMON,
    MB_COMM_STATE_TRANSFER,
    MB_COMM_STATE_POKEMON_SENT,
    MB_COMM_STATE_WAIT_SAVE,
    MB_COMM_STATE_NO_POKEMON = 8,
    MB_COMM_STATE_FINISHED,
    MB_COMM_STATE_ERROR_1,
    MB_COMM_STATE_ERROR_2,
    MB_COMM_STATE_ERROR_3,
    MB_COMM_STATE_CANCELED,
};

// The parent's answer to the Pokémon the child offers (MB_COMM_CMD_ANSWER): it takes them, declines them, or has no
// room in its boxes
enum {
    MB_COMM_ANSWER_ACCEPT,
    MB_COMM_ANSWER_DECLINE,
    MB_COMM_ANSWER_BOX_FULL,
};

// What the parent tells the child about itself
typedef struct {
    s32 textSpeed;
    u16 highScore;
    u8 language;
} MBCommParentInfo;

MBCommSys *MBComm_Create(HeapID heapId);
void MBComm_Delete(MBCommSys *comm);
void MBComm_Update(MBCommSys *comm);
void MBComm_StartNet(MBCommSys *comm);
void MBComm_EndNet(MBCommSys *comm);
BOOL MBComm_IsNetReady(MBCommSys *comm);
BOOL MBComm_IsNetEnded(MBCommSys *comm);
void MBComm_Connect(MBCommSys *comm);
void MBComm_StartDisconnect(MBCommSys *comm);
BOOL MBComm_IsDisconnected(MBCommSys *comm);
BOOL MBComm_IsConnected(MBCommSys *comm);
int MBComm_GetState(MBCommSys *comm);
void MBComm_ResetCommands(MBCommSys *comm);
BOOL MBComm_IsSaveReady(MBCommSys *comm);
BOOL MBComm_IsSaveStarted(MBCommSys *comm);
BOOL MBComm_IsSaveMidReached(MBCommSys *comm);
BOOL MBComm_IsSaved(MBCommSys *comm);
BOOL MBComm_IsSaveSync1(MBCommSys *comm);
BOOL MBComm_IsSaveSync4(MBCommSys *comm);
BOOL MBComm_HasResult(MBCommSys *comm);
u16 MBComm_GetResultCount(MBCommSys *comm);
u16 MBComm_GetResultMoreCount(MBCommSys *comm);
// Bits 30 and 31 of the result
BOOL MBComm_GetResultFlag1(MBCommSys *comm);
BOOL MBComm_GetResultFlag2(MBCommSys *comm);
BOOL MBComm_HasMore(MBCommSys *comm);
BOOL MBComm_HasItemInfo(MBCommSys *comm);
BOOL MBComm_HasItem(MBCommSys *comm);
void MBComm_SendProgram(MBCommSys *comm, void *data, u32 size);
void MBComm_ClearPokemon(MBCommSys *comm);
BOOL MBComm_IsPokemonReceived(MBCommSys *comm);
BOOL MBComm_IsDataReceived(MBCommSys *comm);
u8 MBComm_GetPokemonCount(MBCommSys *comm);
BoxPkm *MBComm_GetPokemon(MBCommSys *comm, u8 index);
u16 MBComm_GetScore(MBCommSys *comm);
BOOL MBComm_IsAcked(MBCommSys *comm);
BOOL MBComm_SendCommand(MBCommSys *comm, u8 command, u32 value);
BOOL MBComm_SendParentInfo(MBCommSys *comm, MBCommParentInfo *info);
void *MBComm_GetData(MBCommSys *comm);

#endif // POKEBW2_APP_MB_PARENT_MB_COMM_SYS_H
