#ifndef POKEBW2_APP_SPIN_TRADE_H
#define POKEBW2_APP_SPIN_TRADE_H

#include "types.h"
#include "field/union_app.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// The Spin Trade of the Union Room, overlay 216, in which up to five players spin their Eggs on a disc and take the
// one that stops in front of them. spin_trade.c, spin_trade_comm.c and spin_trade_main.c are the ROM's
// guru2.c, guru2_comm.c and guru2_main.c; spin_trade_receipt.c, which allocates nothing by name, is named after
// pokeplatinum's guru2_receipt.c, of the same screen. Names are ours
#define OVERLAY_SPIN_TRADE OVERLAY_ID(216)

#define HEAPID_SPIN_TRADE 0x72

#define SPIN_TRADE_MEMBER_MAX UNION_APP_MEMBER_MAX

// The base of the net commands, and the commands. The receipt screen sends the first five
#define SPIN_TRADE_NET_CMD_BASE 0x2800
enum {
    // A member joined
    SPIN_TRADE_NET_CMD_JOIN = SPIN_TRADE_NET_CMD_BASE,
    // A SpinTradeCommSignal, to the parent or from it
    SPIN_TRADE_NET_CMD_SIGNAL,
    // The parent cancelled
    SPIN_TRADE_NET_CMD_CANCEL,
    SPIN_TRADE_NET_CMD_3,
    // The parent closed the group, and the trade starts
    SPIN_TRADE_NET_CMD_START,
    SPIN_TRADE_NET_CMD_5,
    SPIN_TRADE_NET_CMD_6,
    SPIN_TRADE_NET_CMD_7,
    SPIN_TRADE_NET_CMD_8,
    SPIN_TRADE_NET_CMD_9,
    SPIN_TRADE_NET_CMD_10,
    SPIN_TRADE_NET_CMD_11,
    SPIN_TRADE_NET_CMD_12,
    SPIN_TRADE_NET_CMD_13,
    SPIN_TRADE_NET_CMD_14,
    // A member's block of SPIN_TRADE_COMM_BLOCK_SIZE bytes, sent in chunks
    SPIN_TRADE_NET_CMD_BLOCK,
    SPIN_TRADE_NET_CMD_END,
};
#define SPIN_TRADE_NET_CMD_COUNT (SPIN_TRADE_NET_CMD_END - SPIN_TRADE_NET_CMD_BASE)

// The size of what each member sends with SPIN_TRADE_NET_CMD_BLOCK
#define SPIN_TRADE_COMM_BLOCK_SIZE 0x594

// The proc's param, which is union_main.c's UnionProcParam
typedef struct {
    UnionApp *app;
    GameData *gameData;
    u8 unk08[8];
    GameRecords *records;
    // Set by the main screen once the game has started
    BOOL started;
} SpinTradeParam;

typedef struct SpinTradeProcWork SpinTradeProcWork;
typedef struct SpinTradeCommWork SpinTradeCommWork;
typedef struct SpinTradeReceiptWork SpinTradeReceiptWork;
typedef struct SpinTradeMainWork SpinTradeMainWork;

// What spin_trade.c's SpinTradeProcWork.state says the Spin Trade's screen was left for
enum {
    SPIN_TRADE_PROC_STATE_POKE_LIST = 1,
    SPIN_TRADE_PROC_STATE_MAIN = 3,
};

// The work the three parts share, which spin_trade.c allocates and gives the receipt and main screens as their param
struct SpinTradeProcWork {
    int state;
    // The party slot picked on the party list
    int partyIndex;
    // The members when the receipt screen ended: how many, and their net ID bits
    u32 memberCount;
    u32 memberBits;
    // Whether the receipt screen ended with the group closed, rather than cancelled
    BOOL receiptDone;
    SpinTradeParam *param;
    SpinTradeCommWork *g2c;
    SpinTradeMainWork *g2m;
    SpinTradeReceiptWork *g2r;
};

// What SPIN_TRADE_NET_CMD_SIGNAL carries
typedef struct {
    u8 netId;
    // The parent's answer to a request to leave: how many members there are
    u8 count;
    // SPIN_TRADE_SIGNAL_*
    u8 type;
    // The parent's answer to a request to leave: whether the member may
    u8 ok;
} SpinTradeCommSignal;

enum {
    // A member asks the parent to leave, and the parent answers
    SPIN_TRADE_SIGNAL_LEAVE_REQUEST,
    // The member leaves, which the parent tells everyone
    SPIN_TRADE_SIGNAL_LEAVE,
};

// What SpinTradeComm_Send sends: the data follows four bytes that go out with it
typedef struct {
    u32 unk0;
    u8 data[0x18];
} SpinTradeCommSendBuffer;

struct SpinTradeCommWork {
    SpinTradeProcWork *g2p;
    SpinTradeCommSendBuffer send;
    // How many members there were when SPIN_TRADE_NET_CMD_START came
    int startCount;
    BOOL started;
    u32 unk28;
    // How many members the parent counts, and those it has let leave and that are still there (net ID bits)
    int joinCount;
    u32 leavingBits;
    u32 unk34;
    // The count of the parent's answer to this machine's request to leave
    u16 signalCount;
    u8 unk3A[2];
    // Tested by the main screen when it ends after an error
    u32 unk3C;
    // Net ID bits that the main screen's commands set
    u16 unk40;
    u16 unk42;
    u8 unk44[2];
    // The members whose block has arrived
    s16 blockBits;
    u16 unk48;
    u16 unk4A;
    // SPIN_TRADE_COMM_BLOCK_SIZE bytes per member: where the chunks arrive, and the blocks that have arrived whole
    u8 *blockRecvBuffer;
    u8 *blocks;
    u8 unk54[4];
};

// The receipt screen's sequences, which the net commands set
enum {
    SPIN_TRADE_RECEIPT_SEQ_INIT,
    SPIN_TRADE_RECEIPT_SEQ_1,
    SPIN_TRADE_RECEIPT_SEQ_2,
    SPIN_TRADE_RECEIPT_SEQ_MAIN,
    SPIN_TRADE_RECEIPT_SEQ_CANCEL_YESNO_INIT,
    SPIN_TRADE_RECEIPT_SEQ_CANCEL_YESNO,
    SPIN_TRADE_RECEIPT_SEQ_CANCEL_SIGNAL_WAIT,
    SPIN_TRADE_RECEIPT_SEQ_CANCEL_OK,
    SPIN_TRADE_RECEIPT_SEQ_CANCEL_REFUSED,
    SPIN_TRADE_RECEIPT_SEQ_END_MSG,
    SPIN_TRADE_RECEIPT_SEQ_END_WAIT,
    SPIN_TRADE_RECEIPT_SEQ_PARENT_CANCEL_YESNO_INIT,
    SPIN_TRADE_RECEIPT_SEQ_PARENT_CANCEL_YESNO,
    SPIN_TRADE_RECEIPT_SEQ_PARENT_CANCEL_MSG,
    SPIN_TRADE_RECEIPT_SEQ_PARENT_CANCEL_WAIT,
    SPIN_TRADE_RECEIPT_SEQ_PARENT_CANCEL_SYNC,
    SPIN_TRADE_RECEIPT_SEQ_PARENT_CANCEL_SYNC_WAIT,
    SPIN_TRADE_RECEIPT_SEQ_NO_MEMBERS_MSG_WAIT,
    SPIN_TRADE_RECEIPT_SEQ_NO_MEMBERS_END,
    SPIN_TRADE_RECEIPT_SEQ_MEMBER_LEFT,
    SPIN_TRADE_RECEIPT_SEQ_MEMBER_LEFT_2,
    SPIN_TRADE_RECEIPT_SEQ_MEMBER_LEFT_WAIT,
    SPIN_TRADE_RECEIPT_SEQ_START_YESNO_INIT,
    SPIN_TRADE_RECEIPT_SEQ_START_YESNO,
    SPIN_TRADE_RECEIPT_SEQ_START_SEND,
    SPIN_TRADE_RECEIPT_SEQ_START_WAIT,
    SPIN_TRADE_RECEIPT_SEQ_MSG_WAIT,
    SPIN_TRADE_RECEIPT_SEQ_START,
    SPIN_TRADE_RECEIPT_SEQ_COUNT,
};

extern const GameProcFunctions SPIN_TRADE_PROC_FUNCTIONS;

// spin_trade_comm.c
SpinTradeCommWork *SpinTradeComm_Create(SpinTradeProcWork *g2p, u32 heapId);
void SpinTradeComm_Free(SpinTradeCommWork *g2c);
// Adds the net commands
void SpinTradeComm_AddCommands(SpinTradeCommWork *g2c);
void SpinTradeComm_End(SpinTradeCommWork *g2c);
// Sends a SPIN_TRADE_NET_CMD_*, returning whether it could
BOOL SpinTradeComm_Send(SpinTradeCommWork *g2c, u32 command, const void *data, u32 size);
// A member's block
void *SpinTradeComm_GetBlock(SpinTradeCommWork *g2c, int netId);

// spin_trade_receipt.c, the screen where the group gathers
BOOL SpinTradeReceipt_Init(GameProc *proc, u32 *state, void *param, void *work);
BOOL SpinTradeReceipt_Main(GameProc *proc, u32 *state, void *param, void *work);
BOOL SpinTradeReceipt_Exit(GameProc *proc, u32 *state, void *param, void *work);
// Called by the net commands: sets the screen's sequence
void SpinTradeReceipt_SetSeq(SpinTradeReceiptWork *g2r, int seq, u8 netId);
void SpinTradeReceipt_SetSeqOnJoin(SpinTradeReceiptWork *g2r, int seq, u8 netId);

// SPIN_TRADE_NET_CMD_8: what the parent sends each frame of the game
typedef struct {
    fx32 discAngle;
    // How high the Eggs have jumped
    fx32 jumpY;
    // In degrees
    u16 eggAngles[SPIN_TRADE_MEMBER_MAX];
    u16 timeUp;
} SpinTradeGameData;

// SPIN_TRADE_NET_CMD_12: where everything stopped, in degrees
typedef struct {
    u16 discAngle;
    // The net IDs whose Egg stopped on a bonus area
    u16 bonusBits;
    // By position
    u16 eggAngles[6];
} SpinTradeResultData;

// SPIN_TRADE_NET_CMD_9: a member's position around the disc
typedef struct {
    u16 position;
    u16 netId;
} SpinTradePositionData;

// spin_trade_main.c, the screen with the disc
BOOL SpinTradeMain_Init(GameProc *proc, u32 *state, void *param, void *work);
BOOL SpinTradeMain_Main(GameProc *proc, u32 *state, void *param, void *work);
BOOL SpinTradeMain_Exit(GameProc *proc, u32 *state, void *param, void *work);
// What the net commands 7 to 12 call with their data
void SpinTradeMain_OnButton(SpinTradeMainWork *g2m, u32 value);
void SpinTradeMain_OnGameData(SpinTradeMainWork *g2m, const SpinTradeGameData *data);
void SpinTradeMain_OnPosition(SpinTradeMainWork *g2m, const SpinTradePositionData *position);
void SpinTradeMain_OnPlayerCount(SpinTradeMainWork *g2m, u32 value);
void SpinTradeMain_OnPartyIndex(SpinTradeMainWork *g2m, int netId, u32 value);
void SpinTradeMain_OnResult(SpinTradeMainWork *g2m, const SpinTradeResultData *data);

#endif // POKEBW2_APP_SPIN_TRADE_H
