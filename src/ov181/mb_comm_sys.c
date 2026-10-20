#include "types.h"
#include "app/mb_parent/mb_comm_sys.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_lower_data.h"
#include "pml/poke_party.h"

// The connection between the DS Download Play parent and the child program once it has booted (mb_comm_sys.c): the
// parent sends the child a program, the child sends back Pokémon or, for the Memory Link, 0x500 bytes of its save
// data, and the two sides tell each other their progress with commands of one byte and a value. The names are ours,
// from how the parent uses them

// The game's commands, from 0x400 as the init data's gameCommandBase of 4 places them
enum {
    MB_COMM_NETCMD_PACKET = 0x400,
    MB_COMM_NETCMD_PARENT_INFO,
    MB_COMM_NETCMD_POKEMON,
    MB_COMM_NETCMD_DATA,
};

// The connection's states
enum {
    MB_COMM_SEQ_NONE,
    MB_COMM_SEQ_WAIT_CONNECT,
    MB_COMM_SEQ_WAIT_NEGOTIATION,
    MB_COMM_SEQ_WAIT_TIMING,
    MB_COMM_SEQ_WAIT_DISCONNECT_TIMING,
    MB_COMM_SEQ_DISCONNECTED,
    MB_COMM_SEQ_CONNECTED,
};

#define MB_COMM_POKEMON_MAX 6

// The Pokémon the child sends: how many, the score of the Poké Transfer game, then the Pokémon
typedef struct {
    u8 count;
    u16 score;
} MBCommPokemonHeader;

typedef struct {
    u32 value;
    u8 command;
} MBCommPacket;

struct MBCommSys {
    HeapID heapId;
    int seq;
    // The state to tell the other side, and the state last shared
    u32 pendingState;
    u32 state;
    MBCommPokemonHeader *pokemon;
    MBCommParentInfo *parentInfo;
    // Which commands have arrived
    u16 parentInfoReceived : 1;
    u16 pokemonReceived : 1;
    u16 ack : 1;
    u16 saveReady : 1;
    u16 saveStart : 1;
    u16 saveMid : 1;
    u16 saved : 1;
    u16 saveSync1 : 1;
    u16 saveSync2 : 1;
    u16 saveSync3 : 1;
    u16 saveSync4 : 1;
    u16 netStarted : 1;
    u16 boxSpaceReceived : 1;
    u16 answerReceived : 1;
    u16 resultReceived : 1;
    u16 more : 1;
    u16 itemReceived : 1;
    u16 hasItem : 1;
    u16 itemAnswerReceived : 1;
    u16 itemAnswer : 1;
    u16 finish : 1;
    u16 end : 1;
    u16 dataReceived : 1;
    u16 close : 1;
    u8 syncValue;
    u16 boxSpace;
    u8 answer;
    // The count of Pokémon to receive in the low 16 bits, then more counts and two flags
    u32 result;
    void *data;
};

static void MBComm_RecvPacket(int netId, int size, const void *data, void *work, NetHandle *handle);
static void MBComm_RecvParentInfo(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *MBComm_GetParentInfoBuffer(int netId, void *work, int size);
static void MBComm_RecvPokemon(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *MBComm_GetPokemonBuffer(int netId, void *work, int size);
static void MBComm_RecvData(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *MBComm_GetDataBuffer(int netId, void *work, int size);
static void *MBComm_GetBeaconData(void *work);
static int MBComm_GetBeaconDataSize(void *work);

static const NetCommand sMBCommCommands[] = {
    { MBComm_RecvPacket, NULL },
    { MBComm_RecvParentInfo, MBComm_GetParentInfoBuffer },
    { MBComm_RecvPokemon, MBComm_GetPokemonBuffer },
    { MBComm_RecvData, MBComm_GetDataBuffer },
};

static u16 sMBCommBeaconData;

MBCommSys *MBComm_Create(HeapID heapId) {
    MBCommSys *comm = GFL_HeapAllocate(heapId, sizeof(MBCommSys), TRUE, "mb_comm_sys.c", 169);

    comm->heapId = heapId;
    comm->seq = MB_COMM_SEQ_NONE;
    comm->parentInfo = NULL;
    comm->netStarted = FALSE;
    comm->state = 0;
    comm->pendingState = 0;
    comm->pokemon =
        GFL_HeapAllocate(comm->heapId, PML_GetPkmRawSize() * MB_COMM_POKEMON_MAX + sizeof(MBCommPokemonHeader), TRUE,
                         "mb_comm_sys.c", 179);
    return comm;
}

void MBComm_Delete(MBCommSys *comm) {
    if (comm->data != NULL) {
        GFL_HeapFree(comm->data);
    }
    if (comm->parentInfo != NULL) {
        GFL_HeapFree(comm->parentInfo);
    }
    if (comm->netStarted == TRUE) {
        NetLdata_Exit();
    }
    GFL_HeapFree(comm->pokemon);
    GFL_HeapFree(comm);
}

void MBComm_Update(MBCommSys *comm) {
    switch (comm->seq) {
    case MB_COMM_SEQ_WAIT_CONNECT:
        if (NetHandle_SendNegotiation() == TRUE) {
            comm->seq = MB_COMM_SEQ_WAIT_NEGOTIATION;
        }
        break;
    case MB_COMM_SEQ_WAIT_NEGOTIATION:
        if (NetHandle_IsNegotiated(NetHandle_GetOwn()) == TRUE) {
            comm->seq = MB_COMM_SEQ_WAIT_TIMING;
        }
        break;
    case MB_COMM_SEQ_WAIT_TIMING:
        if (GFL_NetIsHandleNegotiated(NetHandle_GetOwn()) == TRUE) {
            comm->seq = MB_COMM_SEQ_CONNECTED;
            func_02042e9c(TRUE);
            func_02042e94(TRUE);
        }
        break;
    case MB_COMM_SEQ_WAIT_DISCONNECT_TIMING:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x40, 4) == TRUE) {
            comm->seq = MB_COMM_SEQ_DISCONNECTED;
            func_02042e94(FALSE);
            func_02042e9c(FALSE);
        }
        break;
    case MB_COMM_SEQ_DISCONNECTED:
    case MB_COMM_SEQ_CONNECTED:
        break;
    }
    if (comm->state != comm->pendingState && comm->pendingState != 0) {
        if (MBComm_SendCommand(comm, MB_COMM_CMD_STATE, comm->pendingState) == TRUE) {
            comm->state = comm->pendingState;
        }
    }
}

void MBComm_StartNet(MBCommSys *comm) {
    GFLNetInitData init = {
        sMBCommCommands,
        NELEMS(sMBCommCommands),
        NULL,
        NULL,
        NULL,
        NULL,
        MBComm_GetBeaconData,
        MBComm_GetBeaconDataSize,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        { 0 },
        NULL,
        NULL,
        0,
        { 1, 0, 0, 0, 0x80, 0x13, 0, 0 },
        HEAPID_SYSTEM,
        0xd,
        0xf,
        0xf,
        0xf0,
        0,
        2,
        0x30,
        2,
        1,
        1,
        0,
        0,
        4,
        { 0x2c, 1, 0, 0 },
        0x1d0,
        0,
    };

    init.parentHeapId = comm->heapId;
    GFL_NetInit(&init, NULL, comm);
    comm->result = 0;
    comm->parentInfoReceived = FALSE;
    comm->boxSpace = 0;
    comm->saveReady = FALSE;
    comm->syncValue = 0;
    comm->saveStart = FALSE;
    comm->saveMid = FALSE;
    comm->saved = FALSE;
    comm->saveSync1 = FALSE;
    comm->saveSync2 = FALSE;
    comm->saveSync3 = FALSE;
    comm->saveSync4 = FALSE;
    comm->netStarted = TRUE;
    comm->resultReceived = FALSE;
    comm->answerReceived = FALSE;
    comm->more = FALSE;
    comm->itemReceived = FALSE;
    comm->hasItem = FALSE;
    comm->itemAnswerReceived = FALSE;
    comm->itemAnswer = FALSE;
    comm->finish = FALSE;
    comm->end = FALSE;
    comm->answer = 0;
    NetLdata_Init(comm->heapId, FALSE);
}

void MBComm_EndNet(MBCommSys *comm) {
    GFL_NetExit(NULL);
    NetLdata_Exit();
    comm->netStarted = FALSE;
    comm->seq = MB_COMM_SEQ_NONE;
}

BOOL MBComm_IsNetReady(MBCommSys *comm) {
    return GFL_NetIsInitialized();
}

BOOL MBComm_IsNetEnded(MBCommSys *comm) {
    return GFL_NetIsEnded();
}

void MBComm_Connect(MBCommSys *comm) {
    func_02042970();
    comm->seq = MB_COMM_SEQ_WAIT_CONNECT;
}

void MBComm_StartDisconnect(MBCommSys *comm) {
    NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x40, 4);
    comm->seq = MB_COMM_SEQ_WAIT_DISCONNECT_TIMING;
}

BOOL MBComm_IsDisconnected(MBCommSys *comm) {
    if (comm->seq == MB_COMM_SEQ_DISCONNECTED) {
        return TRUE;
    }
    return FALSE;
}

BOOL MBComm_IsConnected(MBCommSys *comm) {
    if (comm->seq >= MB_COMM_SEQ_CONNECTED) {
        return TRUE;
    }
    return FALSE;
}

int MBComm_GetState(MBCommSys *comm) {
    return comm->state;
}

void MBComm_ResetCommands(MBCommSys *comm) {
    comm->pokemonReceived = FALSE;
    comm->dataReceived = FALSE;
    comm->ack = FALSE;
    comm->close = FALSE;
    comm->saveReady = FALSE;
    comm->saveStart = FALSE;
    comm->saveMid = FALSE;
    comm->saved = FALSE;
    comm->saveSync1 = FALSE;
    comm->saveSync2 = FALSE;
    comm->saveSync3 = FALSE;
    comm->saveSync4 = FALSE;
    comm->boxSpaceReceived = FALSE;
    comm->syncValue = 0;
}

BOOL MBComm_IsSaveReady(MBCommSys *comm) {
    return comm->saveReady;
}

BOOL MBComm_IsSaveStarted(MBCommSys *comm) {
    return comm->saveStart;
}

BOOL MBComm_IsSaveMidReached(MBCommSys *comm) {
    // BUG: This reads the flag of MB_COMM_CMD_SAVE_START again, and nothing reads that of MB_COMM_CMD_SAVE_MID, so the
    // parent goes on to the next step of the save without waiting for the child to reach the middle
#ifdef BUGFIX
    return comm->saveMid;
#else
    return comm->saveStart;
#endif
}

BOOL MBComm_IsSaved(MBCommSys *comm) {
    return comm->saved;
}

BOOL MBComm_IsSaveSync1(MBCommSys *comm) {
    return comm->saveSync1;
}

BOOL MBComm_IsSaveSync4(MBCommSys *comm) {
    return comm->saveSync4;
}

BOOL MBComm_HasResult(MBCommSys *comm) {
    return comm->resultReceived;
}

u16 MBComm_GetResultCount(MBCommSys *comm) {
    return comm->result;
}

u16 MBComm_GetResultMoreCount(MBCommSys *comm) {
    return (comm->result & 0x3fff0000) >> 16;
}

BOOL MBComm_GetResultFlag1(MBCommSys *comm) {
    if (comm->result & (1 << 30)) {
        return TRUE;
    }
    return FALSE;
}

BOOL MBComm_GetResultFlag2(MBCommSys *comm) {
    if (comm->result & 0x80000000) {
        return TRUE;
    }
    return FALSE;
}

BOOL MBComm_HasMore(MBCommSys *comm) {
    return comm->more;
}

BOOL MBComm_HasItemInfo(MBCommSys *comm) {
    return comm->itemReceived;
}

BOOL MBComm_HasItem(MBCommSys *comm) {
    return comm->hasItem;
}

void MBComm_SendProgram(MBCommSys *comm, void *data, u32 size) {
    NetLdata_StartSend(data, size, 2, TRUE);
}

void MBComm_ClearPokemon(MBCommSys *comm) {
    u8 i;
    MBCommPokemonHeader *header = comm->pokemon;

    header->count = 0;
    header->score = 0;
    for (i = 0; i < MB_COMM_POKEMON_MAX; i++) {
        PML_PkmInit((BoxPkm *)((u8 *)comm->pokemon + PML_GetPkmRawSize() * i + sizeof(MBCommPokemonHeader)));
    }
    comm->pokemonReceived = FALSE;
    comm->ack = FALSE;
}

BOOL MBComm_IsPokemonReceived(MBCommSys *comm) {
    return comm->pokemonReceived;
}

BOOL MBComm_IsDataReceived(MBCommSys *comm) {
    return comm->dataReceived;
}

u8 MBComm_GetPokemonCount(MBCommSys *comm) {
    return comm->pokemon->count;
}

BoxPkm *MBComm_GetPokemon(MBCommSys *comm, u8 index) {
    return (BoxPkm *)((u8 *)comm->pokemon + PML_GetPkmRawSize() * index + sizeof(MBCommPokemonHeader));
}

u16 MBComm_GetScore(MBCommSys *comm) {
    return comm->pokemon->score;
}

BOOL MBComm_IsAcked(MBCommSys *comm) {
    return comm->ack;
}

BOOL MBComm_SendCommand(MBCommSys *comm, u8 command, u32 value) {
    NetHandle *handle;
    MBCommPacket packet;

    if (GFL_NetIsParent() == TRUE) {
        handle = NetHandle_Get(0xff);
    } else {
        handle = NetHandle_GetOwn();
    }
    packet.command = command;
    packet.value = value;
    return GFL_NetSendCommandToMachine(handle, 0xff, MB_COMM_NETCMD_PACKET, sizeof(MBCommPacket), &packet, TRUE, FALSE,
                                       FALSE);
}

static void MBComm_RecvPacket(int netId, int size, const void *data, void *work, NetHandle *handle) {
    const MBCommPacket *packet = data;
    MBCommSys *comm = work;

    switch (packet->command) {
    case MB_COMM_CMD_ACK:
        comm->ack = TRUE;
        break;
    case MB_COMM_CMD_SAVE_SYNC_1:
        comm->saveSync1 = TRUE;
        break;
    case MB_COMM_CMD_SAVE_SYNC_2:
        comm->saveSync2 = TRUE;
        comm->syncValue = packet->value;
        break;
    case MB_COMM_CMD_SAVE_SYNC_3:
        comm->saveSync3 = TRUE;
        comm->syncValue = packet->value;
        break;
    case MB_COMM_CMD_SAVE_SYNC_4:
        comm->saveSync4 = TRUE;
        break;
    case MB_COMM_CMD_CLOSE:
        comm->close = TRUE;
        break;
    case MB_COMM_CMD_BOX_SPACE:
        comm->boxSpace = packet->value;
        comm->boxSpaceReceived = TRUE;
        break;
    case MB_COMM_CMD_STATE:
        comm->state = packet->value;
        break;
    case MB_COMM_CMD_SAVE_READY:
        comm->saveReady = TRUE;
        break;
    case MB_COMM_CMD_SAVE_START:
        comm->saveStart = TRUE;
        break;
    case MB_COMM_CMD_SAVE_MID:
        comm->saveMid = TRUE;
        break;
    case MB_COMM_CMD_SAVED:
        comm->saved = TRUE;
        break;
    case MB_COMM_CMD_ANSWER:
        comm->answerReceived = TRUE;
        comm->answer = packet->value;
        break;
    case MB_COMM_CMD_ITEM_ANSWER:
        comm->itemAnswerReceived = TRUE;
        comm->itemAnswer = packet->value;
        break;
    case MB_COMM_CMD_FINISH:
        comm->finish = TRUE;
        break;
    case MB_COMM_CMD_END:
        comm->end = TRUE;
        break;
    case MB_COMM_CMD_RESULT:
        comm->resultReceived = TRUE;
        comm->result = packet->value;
        break;
    case MB_COMM_CMD_MORE:
        comm->more = TRUE;
        break;
    case MB_COMM_CMD_ITEM:
        comm->itemReceived = TRUE;
        comm->hasItem = packet->value;
        break;
    }
}

BOOL MBComm_SendParentInfo(MBCommSys *comm, MBCommParentInfo *info) {
    NetHandle *handle = NetHandle_Get(0xff);

    if (GFL_NetGetNegotiatedCount() < 2) {
        return FALSE;
    }
    return GFL_NetSendCommandToMachine(handle, 1, MB_COMM_NETCMD_PARENT_INFO, sizeof(MBCommParentInfo), info, TRUE,
                                       FALSE, TRUE);
}

static void MBComm_RecvParentInfo(int netId, int size, const void *data, void *work, NetHandle *handle) {
    MBCommSys *comm = work;

    comm->parentInfoReceived = TRUE;
}

static void *MBComm_GetParentInfoBuffer(int netId, void *work, int size) {
    MBCommSys *comm = work;

    if (comm->parentInfo == NULL) {
        comm->parentInfo = GFL_HeapAllocate(comm->heapId, sizeof(MBCommParentInfo), TRUE, "mb_comm_sys.c", 895);
    }
    return comm->parentInfo;
}

void *MBComm_GetData(MBCommSys *comm) {
    return comm->data;
}

static void MBComm_RecvPokemon(int netId, int size, const void *data, void *work, NetHandle *handle) {
    MBCommSys *comm = work;

    comm->pokemonReceived = TRUE;
}

static void *MBComm_GetPokemonBuffer(int netId, void *work, int size) {
    MBCommSys *comm = work;

    return comm->pokemon;
}

static void MBComm_RecvData(int netId, int size, const void *data, void *work, NetHandle *handle) {
    MBCommSys *comm = work;

    comm->dataReceived = TRUE;
}

static void *MBComm_GetDataBuffer(int netId, void *work, int size) {
    MBCommSys *comm = work;

    if (comm->data == NULL) {
        comm->data = GFL_HeapAllocate(comm->heapId, 0x500, TRUE, "mb_comm_sys.c", 991);
    }
    return comm->data;
}

static void *MBComm_GetBeaconData(void *work) {
    return &sMBCommBeaconData;
}

static int MBComm_GetBeaconDataSize(void *work) {
    return sizeof(sMBCommBeaconData);
}
