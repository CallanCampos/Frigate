#include "field/mus_comm_func.h"
#include "save/musical_savedata.h"
#include "types.h"
#include "app/musical/musical_system.h"
#include "constants/sound.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_system.h"

// Overlay 211's mus_comm_func.c, named after its string: the musical's communication between the players of a show,
// the callbacks of GAME_COMM_NO_MUSICAL. The players trade their Pokémon and trainer data, the parent decides their
// places on the stage and sends the program's data to the children, and during the show they pass the props the
// Pokémon use.

// The game's commands, from the high byte 0x11
enum {
    MUS_COMM_CMD_MSG = 0x1100,
    MUS_COMM_CMD_PLAYER_DATA,
    MUS_COMM_CMD_ALL_PLAYER_DATA,
    MUS_COMM_CMD_POKE,
    MUS_COMM_CMD_ALL_POKES,
    MUS_COMM_CMD_SOUND_DATA,
    MUS_COMM_CMD_PROGRAM_DATA,
    MUS_COMM_CMD_MSG_ARC_DATA,
    MUS_COMM_CMD_SCRIPT_DATA,
};

// The small messages of MUS_COMM_CMD_MSG
enum {
    MUS_MSG_UNK0,
    MUS_MSG_POSITIONS,
    MUS_MSG_PLAYER_VALUE,
    MUS_MSG_ALL_PLAYER_VALUES,
    MUS_MSG_PROGRAM_SIZE,
    MUS_MSG_MSG_ARC_SIZE,
    MUS_MSG_SCRIPT_SIZE,
    MUS_MSG_SOUND_SIZE,
    MUS_MSG_UNK144,
    MUS_MSG_UNK148,
    MUS_MSG_EQUIP_REQUEST,
    MUS_MSG_EQUIPS,
    MUS_MSG_EQUIP_RESULT,
};

// The numbers the machines synchronize on
#define MUS_COMM_TIMING_DATA 0x40
#define MUS_COMM_TIMING_START 0x41
#define MUS_COMM_TIMING_PROGRAM 0x42
#define MUS_COMM_TIMING_BOOT 0x46

// The communication's states
enum {
    MUS_COMM_STATE_IDLE,
    MUS_COMM_STATE_BOOT,
    MUS_COMM_STATE_BOOT_WAIT,
    MUS_COMM_STATE_SEND_POSITIONS,
    MUS_COMM_STATE_SEND_ALL_DATA,
    MUS_COMM_STATE_WAIT_REQUEST,
    MUS_COMM_STATE_DATA_TIMING,
    MUS_COMM_STATE_SEND_UNK144,
    MUS_COMM_STATE_SEND_UNK148,
    MUS_COMM_STATE_SEND_PROGRAM_SIZE,
    MUS_COMM_STATE_SEND_PROGRAM,
    MUS_COMM_STATE_WAIT_PROGRAM,
    MUS_COMM_STATE_PROGRAM_TIMING,
    MUS_COMM_STATE_SEND_MSG_ARC_SIZE,
    MUS_COMM_STATE_SEND_MSG_ARC,
    MUS_COMM_STATE_SEND_SCRIPT,
    MUS_COMM_STATE_WAIT_SCRIPT,
    MUS_COMM_STATE_SHOW,
    MUS_COMM_STATE_SEND_POKE,
    MUS_COMM_STATE_SEND_ALL_POKES,
    MUS_COMM_STATE_WAIT_POKES,
    MUS_COMM_STATE_POKES_SENT,
};

// The sound data goes in chunks of this size
#define MUS_COMM_SOUND_CHUNK_SIZE 0x3800

#define MUS_COMM_PLAYER_MAX 4
#define MUS_COMM_EQUIP_NONE 10
#define MUS_COMM_EQUIP_QUEUE_MAX 8
#define MUS_COMM_EQUIP_QUEUE_EMPTY 0xffff

// A small message
typedef struct {
    u8 cmd;
    u32 value;
} MusCommMsg;

// What a machine shares with the others when they look for each other
typedef struct {
    PlayerInfo playerInfo;
    u8 mac[6];
    u16 gameId;
} MusCommBeacon;

// A player of the show
typedef struct {
    // Whether it has joined, and whether its trainer data and its Pokémon have arrived
    BOOL joined;
    BOOL hasPoke;
    BOOL hasInfo;
    // Whether it asked to use a prop, and which
    BOOL equipRequested;
    u8 equipRequest;
    // The prop the Pokémon uses, or MUS_COMM_EQUIP_NONE
    u8 equipUsed;
    // Its place on the stage
    u8 pos;
    // The results of its props
    u8 equips[2];
    u32 unk18;
    // The Pokémon and the data it comes with, which is the id of the musical, the trainer data and the Pokémon
    MusicalPoke *poke;
    u16 *musicalId;
    PlayerInfo *info;
    BoxPkm *pkm;
    u8 *data;
} MusCommPlayer;

struct MusCommWork {
    HeapID heapId;
    // Whether the connection failed
    BOOL error;
    // Whether the show's data was given
    BOOL ready;
    // 1 if the connection already exists, as in the Union Room, 0 to make one
    s32 netMode;
    BOOL netEnded;
    u32 unk14;
    BOOL unk18;
    u8 unk1C;
    MusCommBeacon beacon;
    // 1 for the parent, 2 for a child
    s32 role;
    s32 state;
    GameCommSys *comm;
    GameSystem *gsys;
    GameData *gameData;
    PlayerInfo *playerInfo;
    MusCommPlayer players[MUS_COMM_PLAYER_MAX];
    // The Pokémon this machine sends, and a buffer holding it for sending
    MusicalPoke *poke;
    u8 *pokeData;
    // The Pokémon of every player, as the parent sends them
    u8 *allPokeData;
    u16 musicalId;
    PlayerInfo *info;
    BoxPkm *pkm;
    // This machine's data, and every player's data
    u8 *playerData;
    u8 *allPlayerData;
    Ov210Work *ov210;
    u32 unk144;
    u32 unk148;
    // The chunk of the sound data being sent, or the next to be received
    u8 soundChunk;
    BOOL soundStarted;
    BOOL soundChunkSent;
    BOOL allInfoReceived;
    BOOL allPokesReceived;
    BOOL unk160;
    BOOL programSizeReceived;
    BOOL msgArcSizeReceived;
    BOOL scriptSizeReceived;
    // Whether the program, its messages and its script have arrived
    BOOL programReceived;
    BOOL msgArcReceived;
    BOOL scriptReceived;
    // The sound data's state: 1 to start sending, 2 waiting, 3 to send a chunk, 4 waiting for the chunk to be sent
    s32 soundState;
    // The position of the Pokémon in the limelight, 4 or more for none
    u8 limelightPos;
    BOOL sendPlayerValues;
    BOOL equipQueued;
    u32 equipQueue[MUS_COMM_EQUIP_QUEUE_MAX];
};

static void MusComm_Free(MusCommWork *work);
static void MusComm_OnNetEnd(void *work);
static void MusComm_Main(MusCommWork *work);
static void *MusComm_GetBeacon(void *work);
static int MusComm_GetBeaconSize(void *work);
static BOOL MusComm_IsSameGame(int a, int b);
static BOOL MusComm_SendMsgSelf(MusCommWork *work, u8 cmd, u32 value, u8 sendId);
static BOOL MusComm_SendMsg(MusCommWork *work, u8 cmd, u32 value, u8 sendId);
static void MusComm_RecvMsg(int netId, int size, const void *data, void *work, NetHandle *handle);
static BOOL MusComm_SendPlayerData(MusCommWork *work);
static void MusComm_RecvPlayerData(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *MusComm_GetPlayerDataBuffer(int netId, void *work, int size);
static BOOL MusComm_SendAllPlayerData(MusCommWork *work);
static void MusComm_RecvAllPlayerData(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *MusComm_GetAllPlayerDataBuffer(int netId, void *work, int size);
static BOOL MusComm_SendPoke(MusCommWork *work, MusicalPoke *poke);
static void MusComm_RecvPoke(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *MusComm_GetPokeBuffer(int netId, void *work, int size);
static BOOL MusComm_SendAllPokes(MusCommWork *work);
static void MusComm_RecvAllPokes(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *MusComm_GetAllPokesBuffer(int netId, void *work, int size);
static void MusComm_SendSoundData(MusCommWork *work);
static BOOL MusComm_SendSoundChunk(MusCommWork *work, u8 chunk);
static void MusComm_RecvSoundChunk(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *MusComm_GetSoundBuffer(int netId, void *work, int size);
static BOOL MusComm_SendProgramSize(MusCommWork *work);
static BOOL MusComm_SendMsgArcSize(MusCommWork *work);
static BOOL MusComm_SendScriptSize(MusCommWork *work);
static BOOL MusComm_SendProgram(MusCommWork *work);
static void MusComm_RecvProgram(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *MusComm_GetProgramBuffer(int netId, void *work, int size);
static BOOL MusComm_SendMsgArc(MusCommWork *work);
static void MusComm_RecvMsgArc(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *MusComm_GetMsgArcBuffer(int netId, void *work, int size);
static BOOL MusComm_SendScript(MusCommWork *work);
static void MusComm_RecvScript(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *MusComm_GetScriptBuffer(int netId, void *work, int size);
static BOOL MusComm_SendPositions(MusCommWork *work);
static void MusComm_CheckData(void *data, u32 size, u32 kind);

static const NetCommand sMusCommCommands[] = {
    {MusComm_RecvMsg, NULL},
    {MusComm_RecvPlayerData, MusComm_GetPlayerDataBuffer},
    {MusComm_RecvAllPlayerData, MusComm_GetAllPlayerDataBuffer},
    {MusComm_RecvPoke, MusComm_GetPokeBuffer},
    {MusComm_RecvAllPokes, MusComm_GetAllPokesBuffer},
    {MusComm_RecvSoundChunk, MusComm_GetSoundBuffer},
    {MusComm_RecvProgram, MusComm_GetProgramBuffer},
    {MusComm_RecvMsgArc, MusComm_GetMsgArcBuffer},
    {MusComm_RecvScript, MusComm_GetScriptBuffer},
};

static GFLNetInitData sMusCommInitData = {
    sMusCommCommands,
    NELEMS(sMusCommCommands),
    NULL,
    NULL,
    NULL,
    NULL,
    MusComm_GetBeacon,
    MusComm_GetBeaconSize,
    MusComm_IsSameGame,
    NULL,
    NULL,
    NULL,
    NULL,
    {0},
    NULL,
    NULL,
    0,
    {1, 0, 0, 0, 0x80, 0x13, 0, 0},
    HEAPID_USER,
    0xd,
    0xf,
    0x10,
    0xf0,
    0,
    4,
    0x18,
    4,
    1,
    1,
    0,
    1,
    0x11,
    {0x2c, 1, 0, 0},
    0x1a0,
    0,
};

void func_ov211_021ef1e0(HeapID heapId, GameSystem *gsys, GameCommSys *comm, u16 value) {
    MusCommWork *work = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(MusCommWork), TRUE, "mus_comm_func.c", 332);

    work->netMode = value;
    work->gsys = gsys;
    work->gameData = GSYS_GetGameData(gsys);
    work->comm = comm;
    work->error = FALSE;
    work->ready = FALSE;
    GameCommSys_Boot(comm, GAME_COMM_NO_MUSICAL, work);
}

void func_ov211_021ef220(MusCommWork *comm) {
    MusComm_Free(comm);
    GameCommSys_ExitReq(comm->comm);
}

void *func_ov211_021ef230(u32 *seq, void *param) {
    MusCommWork *work = param;

    work->playerInfo = GetGameDataPlayerInfo(work->gameData);
    if (work->netMode == 1) {
        sMusCommInitData.bNetType = 4;
    } else {
        sMusCommInitData.bNetType = 0;
        func_020425ec(&sMusCommInitData, MusComm_OnNetEnd, work);
    }
    func_02008b34(work->playerInfo, &work->beacon.playerInfo);
    func_0207c33c(work->beacon.mac);
    work->beacon.gameId = 0x3a0b;
    return work;
}

BOOL func_ov211_021ef288(u32 *seq, void *param, void *work_) {
    MusCommWork *work = param;

    switch ((int)*seq) {
    case 0:
        if (func_02042a78() <= 1) {
            func_02042860(NULL);
            return TRUE;
        }
        if (work->netMode == 1) {
            func_02040c64(MUS_COMM_CMD_MSG);
        }
        if (work->ready == 1) {
            func_02042e9c(FALSE);
            func_ov211_021ef988(work, MUS_COMM_TIMING_START);
            *seq = 1;
        } else {
            *seq = 2;
        }
        break;
    case 1:
        if (func_ov211_021ef99c(work, MUS_COMM_TIMING_START) == TRUE) {
            *seq = 2;
        }
        if (func_02042a78() <= 1 || GFL_NetErrCheck()) {
            *seq = 10;
        }
        break;
    case 2:
        if (work->ready == 0) {
            func_02042860(NULL);
            return TRUE;
        }
        if (func_02042bc4() == 0) {
            if (func_02042be8(func_02040440(), 1, 0, NULL) == 1) {
                return TRUE;
            }
        } else if (func_02042a78() <= 1) {
            func_02042860(NULL);
            return TRUE;
        }
        if (GFL_NetErrCheck()) {
            *seq = 10;
        }
        break;
    case 10:
        if (GFL_NetErrCheck()) {
            func_02016b0c(work->gsys, 1);
        }
        func_02042860(NULL);
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov211_021ef378(u32 *seq, void *param, void *work) {
    if (func_020427a4() == 1) {
        GFL_HeapFree(work);
        return TRUE;
    }
    return FALSE;
}

void func_ov211_021ef394(u32 *seq, void *param, void *work) {
    MusComm_Main(work);
}

void func_ov211_021ef3a0(MusCommWork *comm) {
    if (func_02042bc4() == 1) {
        comm->role = 1;
    } else {
        comm->role = 2;
    }
}

void func_ov211_021ef3b8(MusCommWork *comm) {
    func_02040c20(MUS_COMM_CMD_MSG, sMusCommCommands, NELEMS(sMusCommCommands), comm);
    if (func_02042bc4() == 1) {
        comm->role = 1;
    } else {
        comm->role = 2;
    }
}

void func_ov211_021ef3e4(MusCommWork *comm, PlayerInfo *info, BoxPkm *pkm, GameCommSys *gameComm, Ov210Work *ov210,
                         HeapID heapId) {
    MusicalSave *musical = getMusicalInfoBlkAddress(comm->gameData);
    u8 i;
    u8 j;
    u32 pokeSize;
    u32 dataSize;

    comm->heapId = heapId;
    for (i = 0; i < MUS_COMM_PLAYER_MAX; i++) {
        comm->players[i].joined = FALSE;
        comm->players[i].pos = i;
        comm->players[i].unk18 = 0;
        comm->players[i].hasPoke = FALSE;
        comm->players[i].hasInfo = FALSE;
        comm->players[i].equipRequested = FALSE;
        comm->players[i].equipRequest = MUS_COMM_EQUIP_NONE;
        comm->players[i].equipUsed = MUS_COMM_EQUIP_NONE;
        comm->players[i].poke =
            GFL_HeapAllocate(comm->heapId, PokeParty_GetPkmRawSize() + sizeof(MusicalPoke), FALSE, "mus_comm_func.c", 526);
        comm->players[i].data = GFL_HeapAllocate(comm->heapId, PlayerInfo_GetSize() + 4 + PML_GetPkmRawSize(), FALSE,
                                        "mus_comm_func.c", 527);
        pokeSize = PokeParty_GetPkmRawSize() + sizeof(MusicalPoke);
        sys_memset(comm->players[i].poke, 0, pokeSize);
        dataSize = PlayerInfo_GetSize() + 4 + PML_GetPkmRawSize();
        sys_memset(comm->players[i].data, 0, dataSize);
        comm->players[i].musicalId = (u16 *)comm->players[i].data;
        comm->players[i].info = (PlayerInfo *)(comm->players[i].data + 4);
        comm->players[i].pkm = (BoxPkm *)(comm->players[i].data + 4 + PlayerInfo_GetSize());
        for (j = 0; j < 2; j++) {
            comm->players[i].equips[j] = 0xff;
        }
    }
    comm->musicalId = func_0200af38(musical);
    comm->info = info;
    comm->pkm = pkm;
    comm->playerData = NULL;
    comm->pokeData = NULL;
    comm->allPokeData = GFL_HeapAllocate(comm->heapId, (PokeParty_GetPkmRawSize() + sizeof(MusicalPoke)) * 4, FALSE,
                                         "mus_comm_func.c", 545);
    comm->allPlayerData = GFL_HeapAllocate(comm->heapId, (PlayerInfo_GetSize() + 4 + PML_GetPkmRawSize()) * 4, FALSE,
                                           "mus_comm_func.c", 546);
    comm->unk1C = 0;
    comm->unk14 = 0;
    comm->unk18 = FALSE;
    comm->soundChunk = 0;
    comm->soundStarted = FALSE;
    comm->allInfoReceived = FALSE;
    comm->allPokesReceived = FALSE;
    comm->unk160 = FALSE;
    comm->programSizeReceived = FALSE;
    comm->msgArcSizeReceived = FALSE;
    comm->programReceived = FALSE;
    comm->msgArcReceived = FALSE;
    comm->soundState = 0;
    comm->sendPlayerValues = FALSE;
    comm->equipQueued = FALSE;
    comm->limelightPos = 4;
    comm->ready = TRUE;
    for (i = 0; i < MUS_COMM_EQUIP_QUEUE_MAX; i++) {
        comm->equipQueue[i] = MUS_COMM_EQUIP_QUEUE_EMPTY;
    }
    comm->ov210 = ov210;
    comm->state = MUS_COMM_STATE_BOOT;
}

static void MusComm_Free(MusCommWork *work) {
    u8 i;

    for (i = 0; i < MUS_COMM_PLAYER_MAX; i++) {
        if (work->players[i].poke != NULL) {
            GFL_HeapFree(work->players[i].data);
            GFL_HeapFree(work->players[i].poke);
        }
    }
    if (work->pokeData != NULL) {
        GFL_HeapFree(work->pokeData);
    }
    if (work->playerData != NULL) {
        GFL_HeapFree(work->playerData);
    }
    if (work->allPokeData != NULL) {
        GFL_HeapFree(work->allPokeData);
    }
    if (work->allPlayerData != NULL) {
        GFL_HeapFree(work->allPlayerData);
    }
}

static void MusComm_OnNetEnd(void *work) {
    MusCommWork *comm = work;

    comm->netEnded = TRUE;
}

// A random number below range, which is not 0
static inline u32 MusComm_Random(u32 range) {
    u64 value = GFL_RandomMT();

    value *= range;
    value >>= 32;
    return value;
}

static void MusComm_Main(MusCommWork *work) {
    u8 count;
    u8 i;

    if (GFL_NetErrCheck()) {
        work->error = TRUE;
        if (!func_020427a4()) {
            func_02042860(NULL);
        }
    }
    if (work->error == 1) {
        return;
    }

    switch (work->state) {
    case MUS_COMM_STATE_IDLE:
        return;
    case MUS_COMM_STATE_BOOT:
        func_02042e9c(TRUE);
        func_ov211_021ef988(work, MUS_COMM_TIMING_BOOT);
        work->state = MUS_COMM_STATE_BOOT_WAIT;
        break;
    case MUS_COMM_STATE_BOOT_WAIT:
        if (func_ov211_021ef99c(work, MUS_COMM_TIMING_BOOT) == 1 && MusComm_SendPlayerData(work) == 1) {
            if (work->role == 1) {
                work->state = MUS_COMM_STATE_SEND_POSITIONS;
            } else {
                work->state = MUS_COMM_STATE_WAIT_REQUEST;
            }
        }
        break;
    case MUS_COMM_STATE_SEND_POSITIONS:
        if (MusComm_SendPositions(work) == 1) {
            work->state = MUS_COMM_STATE_SEND_ALL_DATA;
        }
        break;
    case MUS_COMM_STATE_SEND_ALL_DATA:
        count = 0;
        for (i = 0; i < MUS_COMM_PLAYER_MAX; i++) {
            if (work->players[i].hasInfo == 1) {
                count++;
            }
        }
        if (count == func_02042a78() && MusComm_SendAllPlayerData(work) == 1) {
            work->state = MUS_COMM_STATE_WAIT_REQUEST;
        }
        break;
    case MUS_COMM_STATE_WAIT_REQUEST:
        break;
    case MUS_COMM_STATE_DATA_TIMING:
        if (func_ov211_021ef99c(work, MUS_COMM_TIMING_DATA) == 1) {
            if (work->role == 1) {
                work->state = MUS_COMM_STATE_SEND_UNK144;
            } else {
                work->state = MUS_COMM_STATE_WAIT_PROGRAM;
            }
        }
        break;
    case MUS_COMM_STATE_SEND_UNK144:
        if (MusComm_SendMsg(work, MUS_MSG_UNK144, work->unk144, 0xff) == 1) {
            work->state = MUS_COMM_STATE_SEND_UNK148;
        }
        break;
    case MUS_COMM_STATE_SEND_UNK148:
        if (MusComm_SendMsg(work, MUS_MSG_UNK148, work->unk148, 0xff) == 1) {
            work->state = MUS_COMM_STATE_SEND_PROGRAM_SIZE;
        }
        break;
    case MUS_COMM_STATE_SEND_PROGRAM_SIZE:
        if (MusComm_SendProgramSize(work) == 1) {
            work->state = MUS_COMM_STATE_SEND_PROGRAM;
        }
        break;
    case MUS_COMM_STATE_SEND_PROGRAM:
        if (work->programSizeReceived == 1 && MusComm_SendProgram(work) == 1) {
            work->state = MUS_COMM_STATE_WAIT_PROGRAM;
        }
        break;
    case MUS_COMM_STATE_WAIT_PROGRAM:
        break;
    case MUS_COMM_STATE_PROGRAM_TIMING:
        if (func_ov211_021ef99c(work, MUS_COMM_TIMING_PROGRAM) == 1) {
            if (work->role == 1) {
                if (MusComm_SendMsgArcSize(work) == 1) {
                    work->state = MUS_COMM_STATE_SEND_MSG_ARC_SIZE;
                }
            } else {
                work->state = MUS_COMM_STATE_SHOW;
            }
        }
        break;
    case MUS_COMM_STATE_SEND_MSG_ARC_SIZE:
        if (work->msgArcSizeReceived == 1 && MusComm_SendScriptSize(work) == 1) {
            work->state = MUS_COMM_STATE_SEND_MSG_ARC;
        }
        break;
    case MUS_COMM_STATE_SEND_MSG_ARC:
        if (work->scriptSizeReceived == 1 && MusComm_SendMsgArc(work) == 1) {
            work->state = MUS_COMM_STATE_SEND_SCRIPT;
        }
        break;
    case MUS_COMM_STATE_SEND_SCRIPT:
        if (work->msgArcReceived == 1 && MusComm_SendScript(work) == 1) {
            work->state = MUS_COMM_STATE_WAIT_SCRIPT;
        }
        break;
    case MUS_COMM_STATE_WAIT_SCRIPT:
        work->state = MUS_COMM_STATE_SHOW;
        break;
    case MUS_COMM_STATE_SHOW:
        if (work->scriptReceived == 1) {
            func_ov211_021f0240(work);
        }
        break;
    case MUS_COMM_STATE_SEND_POKE:
        if (MusComm_SendPoke(work, work->poke) == 1) {
            if (work->role == 1) {
                work->state = MUS_COMM_STATE_SEND_ALL_POKES;
            } else {
                work->state = MUS_COMM_STATE_POKES_SENT;
            }
        }
        break;
    case MUS_COMM_STATE_SEND_ALL_POKES:
        count = 0;
        for (i = 0; i < MUS_COMM_PLAYER_MAX; i++) {
            if (work->players[i].hasPoke == 1) {
                count++;
            }
        }
        if (count == func_02042a78() && MusComm_SendAllPokes(work) == 1) {
            work->state = MUS_COMM_STATE_POKES_SENT;
        }
        break;
    case MUS_COMM_STATE_WAIT_POKES:
        break;
    case MUS_COMM_STATE_POKES_SENT:
        break;
    }

    if (work->sendPlayerValues == 1) {
        u32 values = 0;

        values += work->players[0].unk18;
        values += work->players[1].unk18 << 4;
        values += work->players[2].unk18 << 8;
        values += work->players[3].unk18 << 12;
        if (MusComm_SendMsg(work, MUS_MSG_ALL_PLAYER_VALUES, values, 0xff) == 1) {
            work->sendPlayerValues = FALSE;
        }
    }

    {
        u8 requesters[MUS_COMM_PLAYER_MAX];
        u32 requests = 0;

        count = 0;
        for (i = 0; i < MUS_COMM_PLAYER_MAX; i++) {
            if (work->players[i].equipRequested == 1) {
                requesters[count] = i;
                count++;
            }
            requests += work->players[i].equipRequest << (i * 4);
        }
        if (count != 0) {
            u8 chosen = requesters[MusComm_Random(count)];

            requests += chosen << 16;
            if (MusComm_SendMsg(work, MUS_MSG_EQUIPS, requests, 0xff) == 1) {
                for (i = 0; i < count; i++) {
                    work->players[requesters[i]].equipRequested = FALSE;
                    work->players[requesters[i]].equipRequest = MUS_COMM_EQUIP_NONE;
                }
            }
        }
    }

    if (work->equipQueued == 1) {
        BOOL allSent = TRUE;

        for (i = 0; i < MUS_COMM_EQUIP_QUEUE_MAX; i++) {
            if (work->equipQueue[i] != MUS_COMM_EQUIP_QUEUE_EMPTY) {
                if (MusComm_SendMsg(work, MUS_MSG_EQUIP_RESULT, work->equipQueue[i], 0xff) == 1) {
                    work->equipQueue[i] = MUS_COMM_EQUIP_QUEUE_EMPTY;
                }
                allSent = FALSE;
            }
        }
        if (allSent == 1) {
            work->equipQueued = FALSE;
        }
    }
    MusComm_SendSoundData(work);
}

void func_ov211_021ef988(MusCommWork *comm, u8 timing) {
    func_02040624(func_02040440(), timing, 0x11);
}

BOOL func_ov211_021ef99c(MusCommWork *comm, u8 timing) {
    if (func_02040664(func_02040440(), timing, 0x11) == 1) {
        return TRUE;
    }
    if (comm->error == 1) {
        return TRUE;
    }
    return FALSE;
}

PlayerInfo *func_ov211_021ef9c4(MusCommWork *comm, u8 index) {
    if (comm->state != 0 && comm->players[index].hasInfo == 1) {
        return comm->players[index].info;
    }
    return NULL;
}

BoxPkm *func_ov211_021ef9e0(MusCommWork *comm, u8 index) {
    if (comm->state != 0 && comm->players[index].hasInfo == 1) {
        return comm->players[index].pkm;
    }
    return NULL;
}

static void *MusComm_GetBeacon(void *work) {
    MusCommWork *comm = work;

    return &comm->beacon;
}

static int MusComm_GetBeaconSize(void *work) {
    return sizeof(MusCommBeacon);
}

static BOOL MusComm_IsSameGame(int a, int b) {
    if (a == b) {
        return TRUE;
    }
    return FALSE;
}

static BOOL MusComm_SendMsgSelf(MusCommWork *work, u8 cmd, u32 value, u8 sendId) {
    MusCommMsg msg;
    NetHandle *handle = func_02040440();

    msg.cmd = cmd;
    msg.value = value;
    return func_02042c18(handle, sendId, MUS_COMM_CMD_MSG, sizeof(MusCommMsg), &msg, 1, FALSE, FALSE);
}

static BOOL MusComm_SendMsg(MusCommWork *work, u8 cmd, u32 value, u8 sendId) {
    MusCommMsg msg;
    NetHandle *handle = func_02040414(0xff);

    msg.cmd = cmd;
    msg.value = value;
    return func_02042c18(handle, sendId, MUS_COMM_CMD_MSG, sizeof(MusCommMsg), &msg, 1, FALSE, FALSE);
}

static void MusComm_RecvMsg(int netId, int size, const void *data, void *work_, NetHandle *handle) {
    const MusCommMsg *msg = data;
    MusCommWork *work = work_;
    u8 i;
    u8 j;
    u32 mask;

    switch (msg->cmd) {
    case MUS_MSG_UNK0:
        work->unk18 = TRUE;
        break;
    case MUS_MSG_POSITIONS:
        mask = 0xf;
        for (i = 0; i < MUS_COMM_PLAYER_MAX; i++) {
            work->players[i].pos = (msg->value & mask) >> (i * 4);
            mask <<= 4;
        }
        break;
    case MUS_MSG_PLAYER_VALUE:
        work->players[netId].unk18 = msg->value;
        if (work->role == 1) {
            work->sendPlayerValues = TRUE;
        }
        break;
    case MUS_MSG_ALL_PLAYER_VALUES:
        if (work->role != 1) {
            work->players[0].unk18 = msg->value & 0xf;
            work->players[1].unk18 = (msg->value & 0xf0) >> 4;
            work->players[2].unk18 = (msg->value & 0xf00) >> 8;
            work->players[3].unk18 = (msg->value & 0xf000) >> 12;
        }
        break;
    case MUS_MSG_PROGRAM_SIZE:
        if (work->role == 2) {
            work->ov210->unk4 = GFL_HeapAllocate(HEAPID_GAMEEVENT, msg->value, FALSE, "mus_comm_func.c", 1220);
            work->ov210->size[0] = msg->value;
        }
        work->programSizeReceived = TRUE;
        break;
    case MUS_MSG_MSG_ARC_SIZE:
        if (work->role == 2) {
            work->ov210->msgArc = GFL_HeapAllocate(HEAPID_MUSICAL_EVENT, msg->value, FALSE, "mus_comm_func.c", 1229);
            work->ov210->size[1] = msg->value;
        }
        work->msgArcSizeReceived = TRUE;
        break;
    case MUS_MSG_SCRIPT_SIZE:
        if (work->role == 2) {
            work->ov210->script = GFL_HeapAllocate(HEAPID_MUSICAL_EVENT, msg->value, FALSE, "mus_comm_func.c", 1238);
            work->ov210->size[2] = msg->value;
        }
        work->scriptSizeReceived = TRUE;
        break;
    case MUS_MSG_SOUND_SIZE:
        if (work->role == 1) {
            work->soundState = 3;
        } else {
            work->ov210->soundData = GFL_HeapAllocate(HEAPID_MUSICAL_EVENT, msg->value, FALSE, "mus_comm_func.c", 1251);
            work->ov210->soundDataSize = msg->value;
        }
        break;
    case MUS_MSG_UNK144:
        work->unk144 = msg->value;
        break;
    case MUS_MSG_UNK148:
        work->unk148 = msg->value;
        break;
    case MUS_MSG_EQUIP_REQUEST:
        if (work->role == 1) {
            work->players[netId].equipRequested = TRUE;
            work->players[netId].equipRequest = msg->value;
        }
        break;
    case MUS_MSG_EQUIPS: {
        u8 k;
        u32 m;

        m = 0xf;
        for (k = 0; k < MUS_COMM_PLAYER_MAX; k++) {
            u8 equip = (msg->value & m) >> (k * 4);

            if (equip != MUS_COMM_EQUIP_NONE) {
                work->players[k].equipUsed = equip;
            }
            m <<= 4;
        }
        work->limelightPos = (msg->value & 0xf0000) >> 16;
        break;
    }
    case MUS_MSG_EQUIP_RESULT: {
        u8 result = (msg->value & 0xff0000) >> 16;
        u8 pos = (msg->value & 0xff00) >> 8;
        u8 equip = msg->value;

        for (j = 0; j < 2; j++) {
            if (work->players[pos].equips[j] == 0xff) {
                work->players[pos].equips[j] = equip;
                break;
            }
        }
        if (result == 1) {
            GFL_SndSEPlay(SEQ_SE_MSCL_11);
        } else if (result == 2) {
            GFL_SndSEPlay(SEQ_SE_MSCL_10);
        }
        break;
    }
    }
}

static BOOL MusComm_SendPlayerData(MusCommWork *work) {
    u8 *data;
    PlayerInfo *info;
    BoxPkm *pkm;

    if (work->playerData != NULL) {
        GFL_HeapFree(work->playerData);
    }
    data = GFL_HeapAllocate(work->heapId, PlayerInfo_GetSize() + 4 + PML_GetPkmRawSize(), FALSE, "mus_comm_func.c", 1331);
    work->playerData = data;
    info = (PlayerInfo *)(data + 4);
    pkm = (BoxPkm *)(data + 4 + PlayerInfo_GetSize());
    sys_memcpy(&work->musicalId, data, 4);
    sys_memcpy(work->info, info, PlayerInfo_GetSize());
    sys_memcpy(work->pkm, pkm, PML_GetPkmRawSize());
    return func_02042c18(func_02040440(), 0xff, MUS_COMM_CMD_PLAYER_DATA,
                         PlayerInfo_GetSize() + 4 + PML_GetPkmRawSize(), work->playerData, 1, FALSE, TRUE);
}

static void MusComm_RecvPlayerData(int netId, int size, const void *data, void *work, NetHandle *handle) {
    MusCommWork *comm = work;

    comm->players[netId].hasInfo = TRUE;
    comm->players[netId].joined = TRUE;
}

static void *MusComm_GetPlayerDataBuffer(int netId, void *work, int size) {
    MusCommWork *comm = work;

    comm->players[netId].hasInfo = FALSE;
    return comm->players[netId].data;
}

static BOOL MusComm_SendAllPlayerData(MusCommWork *work) {
    u8 i;

    for (i = 0; i < MUS_COMM_PLAYER_MAX; i++) {
        u8 *dest = work->allPlayerData + i * (PlayerInfo_GetSize() + 4 + PML_GetPkmRawSize());

        sys_memcpy(work->players[i].data, dest, PlayerInfo_GetSize() + 4 + PML_GetPkmRawSize());
    }
    return func_02042c18(func_02040414(0xff), 0xff, MUS_COMM_CMD_ALL_PLAYER_DATA,
                         (PlayerInfo_GetSize() + 4 + PML_GetPkmRawSize()) * 4, work->allPlayerData, 1, FALSE, TRUE);
}

static void MusComm_RecvAllPlayerData(int netId, int size, const void *data, void *work, NetHandle *handle) {
    MusCommWork *comm = work;
    u8 i;

    for (i = 0; i < MUS_COMM_PLAYER_MAX; i++) {
        u8 *src = comm->allPlayerData + i * (PlayerInfo_GetSize() + 4 + PML_GetPkmRawSize());
        u32 dataSize = PlayerInfo_GetSize() + 4 + PML_GetPkmRawSize();
        BOOL valid;

        sys_memcpy(src, comm->players[i].data, dataSize);
        valid = func_02008b5c(comm->players[i].info) == 0;
        comm->players[i].joined = valid;
        comm->players[i].hasInfo = valid;
    }
    comm->allInfoReceived = TRUE;
}

static void *MusComm_GetAllPlayerDataBuffer(int netId, void *work, int size) {
    MusCommWork *comm = work;
    u8 i;

    for (i = 0; i < MUS_COMM_PLAYER_MAX; i++) {
        comm->players[i].hasInfo = FALSE;
    }
    comm->allInfoReceived = FALSE;
    return comm->allPlayerData;
}

static BOOL MusComm_SendPoke(MusCommWork *work, MusicalPoke *poke) {
    u8 *data;

    if (work->pokeData != NULL) {
        GFL_HeapFree(work->pokeData);
    }
    data = GFL_HeapAllocate(work->heapId, PokeParty_GetPkmRawSize() + sizeof(MusicalPoke), FALSE, "mus_comm_func.c", 1470);
    work->pokeData = data;
    sys_memcpy(poke, data, sizeof(MusicalPoke));
    sys_memcpy(poke->pkm, data + sizeof(MusicalPoke), PokeParty_GetPkmRawSize());
    return func_02042c18(func_02040440(), 0xff, MUS_COMM_CMD_POKE, PokeParty_GetPkmRawSize() + sizeof(MusicalPoke),
                         work->pokeData, 1, FALSE, TRUE);
}

static void MusComm_RecvPoke(int netId, int size, const void *data, void *work, NetHandle *handle) {
    MusCommWork *comm = work;

    comm->players[netId].hasPoke = TRUE;
    comm->players[netId].poke->pkm = (PartyPkm *)(comm->players[netId].poke + 1);
}

static void *MusComm_GetPokeBuffer(int netId, void *work, int size) {
    MusCommWork *comm = work;

    comm->players[netId].hasPoke = FALSE;
    return comm->players[netId].poke;
}

static BOOL MusComm_SendAllPokes(MusCommWork *work) {
    u8 i;

    for (i = 0; i < MUS_COMM_PLAYER_MAX; i++) {
        u8 *dest = work->allPokeData + i * (PokeParty_GetPkmRawSize() + sizeof(MusicalPoke));
        u32 size = PokeParty_GetPkmRawSize() + sizeof(MusicalPoke);

        sys_memcpy(work->players[i].poke, dest, size);
    }
    return func_02042c18(func_02040414(0xff), 0xff, MUS_COMM_CMD_ALL_POKES,
                         (PokeParty_GetPkmRawSize() + sizeof(MusicalPoke)) * 4, work->allPokeData, 1, FALSE, TRUE);
}

static void MusComm_RecvAllPokes(int netId, int size, const void *data, void *work, NetHandle *handle) {
    MusCommWork *comm = work;
    u8 i;

    for (i = 0; i < MUS_COMM_PLAYER_MAX; i++) {

        if (func_02008b5c(comm->players[i].info) == 0) {
            u8 *src = comm->allPokeData + i * (PokeParty_GetPkmRawSize() + sizeof(MusicalPoke));
            u32 pokeSize = PokeParty_GetPkmRawSize() + sizeof(MusicalPoke);

            sys_memcpy(src, comm->players[i].poke, pokeSize);
            comm->players[i].poke->pkm = (PartyPkm *)(comm->players[i].poke + 1);
            comm->players[i].hasPoke = TRUE;
        } else {
            comm->players[i].hasPoke = FALSE;
        }
    }
    comm->allPokesReceived = TRUE;
}

static void *MusComm_GetAllPokesBuffer(int netId, void *work, int size) {
    MusCommWork *comm = work;
    u8 i;

    for (i = 0; i < MUS_COMM_PLAYER_MAX; i++) {
        comm->players[i].hasPoke = FALSE;
    }
    comm->allPokesReceived = FALSE;
    return comm->allPokeData;
}

MusicalPoke *func_ov211_021f0094(MusCommWork *comm, u8 index) {
    u8 i;

    for (i = 0; i < MUS_COMM_PLAYER_MAX; i++) {
        if (index == comm->players[i].pos && comm->players[i].hasPoke == 1) {
            return comm->players[i].poke;
        }
    }
    return NULL;
}

void func_ov211_021f00c8(MusCommWork *comm) {
    if (comm->soundStarted == 0) {
        comm->soundStarted = TRUE;
        comm->soundChunk = 0;
        comm->soundState = 1;
        comm->soundChunkSent = FALSE;
    }
}

static void MusComm_SendSoundData(MusCommWork *work) {
    if (work->soundStarted != 1) {
        return;
    }
    switch (work->soundState) {
    case 0:
        break;
    case 1:
        if (MusComm_SendMsg(work, MUS_MSG_SOUND_SIZE, work->ov210->soundDataSize, 0xff) == 1) {
            work->soundState = 2;
        }
        break;
    case 2:
        break;
    case 3:
        if (MusComm_SendSoundChunk(work, work->soundChunk) == 1) {
            work->soundChunkSent = TRUE;
            work->soundState = 4;
        }
        break;
    case 4:
        if (work->soundChunkSent == 0) {
            if (work->soundChunk * MUS_COMM_SOUND_CHUNK_SIZE >= work->ov210->soundDataSize) {
                work->soundStarted = FALSE;
            } else {
                work->soundState = 3;
            }
        }
        break;
    }
}

static BOOL MusComm_SendSoundChunk(MusCommWork *work, u8 chunk) {
    u8 *data = (u8 *)work->ov210->soundData + chunk * MUS_COMM_SOUND_CHUNK_SIZE;
    u16 size = MUS_COMM_SOUND_CHUNK_SIZE;

    if ((chunk + 1) * MUS_COMM_SOUND_CHUNK_SIZE > work->ov210->soundDataSize) {
        size = work->ov210->soundDataSize % MUS_COMM_SOUND_CHUNK_SIZE;
    }
    return func_02042c18(func_02040414(0xff), 0xff, MUS_COMM_CMD_SOUND_DATA, size, data, 0, TRUE, TRUE);
}

static void MusComm_RecvSoundChunk(int netId, int size, const void *data, void *work, NetHandle *handle) {
    MusCommWork *comm = work;

    comm->soundChunk++;
    comm->soundChunkSent = FALSE;
    if (comm->soundChunk * MUS_COMM_SOUND_CHUNK_SIZE >= comm->ov210->soundDataSize) {
        u32 *sizes = comm->ov210->soundData;

        // The three sizes, then the three parts
        comm->ov210->sound[0] = sizes + 3;
        comm->ov210->sound[1] = (void *)((u32)(comm->ov210->soundData + 3) + sizes[0]);
        comm->ov210->sound[2] = (void *)((u32)(comm->ov210->soundData + 3) + sizes[0] + sizes[1]);
    }
}

static void *MusComm_GetSoundBuffer(int netId, void *work, int size) {
    MusCommWork *comm = work;

    return (u8 *)comm->ov210->soundData + comm->soundChunk * MUS_COMM_SOUND_CHUNK_SIZE;
}

BOOL func_ov211_021f0240(MusCommWork *comm) {
    if (comm->ov210->soundDataSize / MUS_COMM_SOUND_CHUNK_SIZE < comm->soundChunk) {
        return TRUE;
    }
    return FALSE;
}

static BOOL MusComm_SendProgramSize(MusCommWork *work) {
    return MusComm_SendMsg(work, MUS_MSG_PROGRAM_SIZE, work->ov210->size[0], 0xff);
}

static BOOL MusComm_SendMsgArcSize(MusCommWork *work) {
    return MusComm_SendMsg(work, MUS_MSG_MSG_ARC_SIZE, work->ov210->size[1], 0xff);
}

static BOOL MusComm_SendScriptSize(MusCommWork *work) {
    return MusComm_SendMsg(work, MUS_MSG_SCRIPT_SIZE, work->ov210->size[2], 0xff);
}

static BOOL MusComm_SendProgram(MusCommWork *work) {
    NetHandle *handle;

    MusComm_CheckData(work->ov210->unk4, work->ov210->size[0], 2);
    handle = func_02040414(0xff);
    return func_02042c18(handle, 0xff, MUS_COMM_CMD_PROGRAM_DATA, work->ov210->size[0], work->ov210->unk4,
                         1, FALSE, TRUE);
}

static void MusComm_RecvProgram(int netId, int size, const void *data, void *work, NetHandle *handle) {
    MusCommWork *comm = work;

    comm->programReceived = TRUE;
    MusComm_CheckData(comm->ov210->unk4, comm->ov210->size[0], 0xc);
}

static void *MusComm_GetProgramBuffer(int netId, void *work, int size) {
    MusCommWork *comm = work;

    return comm->ov210->unk4;
}

static BOOL MusComm_SendMsgArc(MusCommWork *work) {
    NetHandle *handle;

    MusComm_CheckData(work->ov210->msgArc, work->ov210->size[1], 1);
    handle = func_02040414(0xff);
    return func_02042c18(handle, 0xff, MUS_COMM_CMD_MSG_ARC_DATA, work->ov210->size[1],
                         work->ov210->msgArc, 1, FALSE, TRUE);
}

static void MusComm_RecvMsgArc(int netId, int size, const void *data, void *work, NetHandle *handle) {
    MusCommWork *comm = work;

    comm->msgArcReceived = TRUE;
    MusComm_CheckData(comm->ov210->msgArc, comm->ov210->size[1], 0xb);
}

static void *MusComm_GetMsgArcBuffer(int netId, void *work, int size) {
    MusCommWork *comm = work;

    return comm->ov210->msgArc;
}

static BOOL MusComm_SendScript(MusCommWork *work) {
    NetHandle *handle;

    MusComm_CheckData(work->ov210->script, work->ov210->size[2], 0);
    handle = func_02040414(0xff);
    return func_02042c18(handle, 0xff, MUS_COMM_CMD_SCRIPT_DATA, work->ov210->size[2], work->ov210->script,
                         1, FALSE, TRUE);
}

static void MusComm_RecvScript(int netId, int size, const void *data, void *work, NetHandle *handle) {
    MusCommWork *comm = work;

    comm->scriptReceived = TRUE;
    MusComm_CheckData(comm->ov210->script, comm->ov210->size[2], 0xa);
}

static void *MusComm_GetScriptBuffer(int netId, void *work, int size) {
    MusCommWork *comm = work;

    return comm->ov210->script;
}

BOOL func_ov211_021f03d8(MusCommWork *comm) {
    return comm->programReceived;
}

BOOL func_ov211_021f03e0(MusCommWork *comm) {
    return comm->scriptReceived;
}

static BOOL MusComm_SendPositions(MusCommWork *work) {
    u8 order[MUS_COMM_PLAYER_MAX];
    u32 positions = 0;
    u8 i;
    u8 j;

    for (i = 0; i < MUS_COMM_PLAYER_MAX; i++) {
        order[i] = i;
    }
    for (i = 0; i < 10; i++) {
        for (j = 0; j < MUS_COMM_PLAYER_MAX; j++) {
            u8 other = GFL_RandomMTRange(MUS_COMM_PLAYER_MAX);
            u8 tmp = order[j];

            order[j] = order[other];
            order[other] = tmp;
        }
    }
    for (i = 0; i < MUS_COMM_PLAYER_MAX; i++) {
        positions += order[i] << (i * 4);
    }
    return MusComm_SendMsg(work, MUS_MSG_POSITIONS, positions, 0xff);
}

BOOL func_ov211_021f0460(MusCommWork *comm, u8 equip) {
    return MusComm_SendMsgSelf(comm, MUS_MSG_EQUIP_REQUEST, equip, 0xff);
}

u8 func_ov211_021f0470(MusCommWork *comm) {
    return comm->players[func_02042a6c(func_02040440())].pos;
}

u8 func_ov211_021f0488(MusCommWork *comm, u8 index) {
    return comm->players[index].pos;
}

u16 *func_ov211_021f0494(MusCommWork *comm, u8 index) {
    return comm->players[index].musicalId;
}

BOOL func_ov211_021f04a0(MusCommWork *comm) {
    return comm->role;
}

BOOL func_ov211_021f04a4(MusCommWork *comm) {
    return comm->allInfoReceived;
}

BOOL func_ov211_021f04ac(MusCommWork *comm) {
    return comm->allPokesReceived;
}

void func_ov211_021f04b4(MusCommWork *comm, u32 a1, u32 a2) {
    func_ov211_021ef988(comm, MUS_COMM_TIMING_DATA);
    comm->state = MUS_COMM_STATE_DATA_TIMING;
    comm->unk144 = a1;
    comm->unk148 = a2;
}

void func_ov211_021f04d4(MusCommWork *comm) {
    func_ov211_021ef988(comm, MUS_COMM_TIMING_PROGRAM);
    comm->state = MUS_COMM_STATE_PROGRAM_TIMING;
}

void func_ov211_021f04e4(MusCommWork *comm, MusicalPoke *poke) {
    comm->state = MUS_COMM_STATE_SEND_POKE;
    comm->poke = poke;
}

u32 func_ov211_021f04f0(MusCommWork *comm) {
    return comm->unk144;
}

u32 func_ov211_021f04f8(MusCommWork *comm) {
    return comm->unk148;
}

u8 func_ov211_021f0500(MusCommWork *comm, u8 index, u8 a2) {
    return comm->players[index].equips[a2];
}

void func_ov211_021f0510(MusCommWork *comm, u8 pos, u8 equip) {
    u8 i;

    for (i = 0; i < MUS_COMM_PLAYER_MAX; i++) {
        if (pos == comm->players[i].pos) {
            comm->players[i].equipRequested = TRUE;
            comm->players[i].equipRequest = equip;
        }
    }
}

u8 func_ov211_021f053c(MusCommWork *comm, u8 pos) {
    u8 i;

    for (i = 0; i < MUS_COMM_PLAYER_MAX; i++) {
        if (pos == comm->players[i].pos) {
            return comm->players[i].equipUsed;
        }
    }
    return MUS_COMM_EQUIP_NONE;
}

void func_ov211_021f056c(MusCommWork *comm, u8 pos) {
    u8 i;

    for (i = 0; i < MUS_COMM_PLAYER_MAX; i++) {
        if (pos == comm->players[i].pos) {
            comm->players[i].equipUsed = MUS_COMM_EQUIP_NONE;
        }
    }
}

u8 func_ov211_021f0598(MusCommWork *comm) {
    if (comm->limelightPos < MUS_COMM_PLAYER_MAX) {
        return comm->players[comm->limelightPos].pos;
    }
    return MUS_COMM_PLAYER_MAX;
}

void func_ov211_021f05b4(MusCommWork *comm) {
    comm->limelightPos = MUS_COMM_PLAYER_MAX;
}

void func_ov211_021f05c0(MusCommWork *comm, u8 pos, u8 equip, u32 result) {
    u8 i;

    for (i = 0; i < MUS_COMM_EQUIP_QUEUE_MAX; i++) {
        if (comm->equipQueue[i] == MUS_COMM_EQUIP_QUEUE_EMPTY) {
            comm->equipQueue[i] = (result << 16) + (pos << 8) + equip;
            comm->equipQueued = TRUE;
            return;
        }
    }
}

const GFLNetInitData *func_ov211_021f0608(GameData *gameData) {
    return &sMusCommInitData;
}

static void MusComm_CheckData(void *data, u32 size, u32 kind) {
}
