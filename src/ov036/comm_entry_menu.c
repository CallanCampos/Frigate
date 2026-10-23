// The field's menu that gathers a group over the wireless network for a Union Room multi battle, the Musical or the
// Battle Subway: the leader accepts or refuses the players who ask to join and proceeds once enough have, and the
// others pick a leader from the beacons they hear or join the one they are connected to. The name is the ROM's own,
// from GFL_HeapAllocate's file argument. The code was first written by mariano54 on their parallel-decomp branch
#include "field/comm_entry_menu.h"
#include "types.h"
#include "constants/text_banks.h"
#include "field/field.h"
#include "field/field_msgbg.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_system.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "save/player_info.h"
#include "system/bmp_menulist.h"
#include "system/wordset.h"
#include "text/system/comm_entry_menu.h"

#define COMM_ENTRY_MEMBER_MAX 4
#define COMM_ENTRY_LEADER_MAX 16
// The game ID that the activity's beacons carry while its leader gathers a group
#define COMM_ENTRY_BEACON_GAME_ID 0x3a0b
// The frames a leader's beacon stays in the list after it was last heard
#define COMM_ENTRY_LEADER_TIMEOUT 150
// The frames a player waits for the leader's answer before giving up
#define COMM_ENTRY_ANSWER_TIMEOUT 300
// No message waiting to be printed
#define COMM_ENTRY_NO_MESSAGE 0xffff
// The timing that the leader and the members synchronize on before the activity starts, and its class
#define COMM_ENTRY_SYNC_START 1
#define COMM_ENTRY_SYNC_CLASS 39

// The commands the machines of a group send each other, from GFL net command 0x2700
enum {
    // A player asks to join, with a CommEntryPacket
    COMM_ENTRY_CMD_ENTRY = 0x2700,
    // The leader accepts or refuses it, with its own PlayerInfo
    COMM_ENTRY_CMD_ACCEPT,
    COMM_ENTRY_CMD_REFUSE,
    // The leader asks the members whether they are still there, and each answers
    COMM_ENTRY_CMD_CHECK,
    COMM_ENTRY_CMD_CHECK_REPLY,
    // The leader starts the activity, or disbands the group
    COMM_ENTRY_CMD_START,
    COMM_ENTRY_CMD_CANCEL,
    // The leader's list of the members, a CommEntryMemberList
    COMM_ENTRY_CMD_MEMBER_LIST,
};

// What a player who asked to join is at
enum {
    COMM_ENTRY_MEMBER_NONE,
    COMM_ENTRY_MEMBER_NEW,
    // Waiting for the leader to be asked about it, and being asked
    COMM_ENTRY_MEMBER_WAITING,
    COMM_ENTRY_MEMBER_ASKING,
    COMM_ENTRY_MEMBER_ACCEPTED,
    COMM_ENTRY_MEMBER_REFUSED,
};

// What the questions to the leader return, CommEntryMenu_AskGroupOk and CommEntryMenu_AskCancel, and the answer that
// CommEntryYesNo keeps: still asking, yes, or no
enum {
    COMM_ENTRY_ASK_PENDING,
    COMM_ENTRY_ASK_YES,
    COMM_ENTRY_ASK_NO,
};

// The steps of the leader's menu: open the windows, start the server unless the Union Room's connection is up, wait
// for a player to connect and negotiate, add the leader itself, gather the players, ask whether the group is OK or
// whether to cancel, wait for the synchronization, check on the members and wait for their replies, cancel or start
// the activity or disband the group, print the error and wait, wait for the members to disconnect, print that there
// are no members and wait, and end
enum {
    COMM_ENTRY_LEADER_SEQ_INIT,
    COMM_ENTRY_LEADER_SEQ_START_SERVER,
    COMM_ENTRY_LEADER_SEQ_WAIT_CONNECT,
    COMM_ENTRY_LEADER_SEQ_WAIT_NEGOTIATE,
    COMM_ENTRY_LEADER_SEQ_SEND_SELF,
    COMM_ENTRY_LEADER_SEQ_GATHER,
    COMM_ENTRY_LEADER_SEQ_ASK_GROUP_OK,
    COMM_ENTRY_LEADER_SEQ_ASK_CANCEL,
    COMM_ENTRY_LEADER_SEQ_WAIT_SYNC,
    COMM_ENTRY_LEADER_SEQ_SEND_CHECK,
    COMM_ENTRY_LEADER_SEQ_WAIT_CHECK,
    COMM_ENTRY_LEADER_SEQ_SEND_CANCEL,
    COMM_ENTRY_LEADER_SEQ_SEND_START,
    COMM_ENTRY_LEADER_SEQ_SEND_DISBAND,
    COMM_ENTRY_LEADER_SEQ_PRINT_ERROR,
    COMM_ENTRY_LEADER_SEQ_WAIT_ERROR,
    COMM_ENTRY_LEADER_SEQ_WAIT_DISCONNECT,
    COMM_ENTRY_LEADER_SEQ_PRINT_NO_MEMBERS,
    COMM_ENTRY_LEADER_SEQ_WAIT_NO_MEMBERS,
    COMM_ENTRY_LEADER_SEQ_END,
};

// The steps of the menu that picks a leader: print the prompt and wait, open the list of the leaders and scan, run the
// list, print the leader's acceptance and wait, wait for the members, synchronize and wait, print the error and wait,
// start the network again after a refusal, and cancel and end
enum {
    COMM_ENTRY_SELECT_SEQ_PRINT_CHOOSE,
    COMM_ENTRY_SELECT_SEQ_WAIT_CHOOSE,
    COMM_ENTRY_SELECT_SEQ_START_SCAN,
    COMM_ENTRY_SELECT_SEQ_LIST,
    COMM_ENTRY_SELECT_SEQ_WAIT_REPLIED,
    COMM_ENTRY_SELECT_SEQ_WAIT_MEMBERS,
    COMM_ENTRY_SELECT_SEQ_SYNC,
    COMM_ENTRY_SELECT_SEQ_WAIT_SYNC,
    COMM_ENTRY_SELECT_SEQ_PRINT_ERROR,
    COMM_ENTRY_SELECT_SEQ_WAIT_ERROR,
    COMM_ENTRY_SELECT_SEQ_REFUSED,
    COMM_ENTRY_SELECT_SEQ_WAIT_NET_EXIT,
    COMM_ENTRY_SELECT_SEQ_NET_INIT,
    COMM_ENTRY_SELECT_SEQ_WAIT_NET_INIT,
    COMM_ENTRY_SELECT_SEQ_CANCEL,
    COMM_ENTRY_SELECT_SEQ_END,
};

// The steps of the menu of a player that the leader accepts without asking: print the prompt, ask to join, open the
// windows, wait for the members, synchronize and wait, print the error and wait, print the refusal and wait (which
// nothing sets), and cancel and end
enum {
    COMM_ENTRY_JOIN_SEQ_PRINT_WAIT,
    COMM_ENTRY_JOIN_SEQ_SEND_ENTRY,
    COMM_ENTRY_JOIN_SEQ_CREATE_WINDOWS,
    COMM_ENTRY_JOIN_SEQ_WAIT_MEMBERS,
    COMM_ENTRY_JOIN_SEQ_SYNC,
    COMM_ENTRY_JOIN_SEQ_WAIT_SYNC,
    COMM_ENTRY_JOIN_SEQ_PRINT_ERROR,
    COMM_ENTRY_JOIN_SEQ_WAIT_ERROR,
    COMM_ENTRY_JOIN_SEQ_REFUSED,
    COMM_ENTRY_JOIN_SEQ_WAIT_REFUSED,
    COMM_ENTRY_JOIN_SEQ_CANCEL,
    COMM_ENTRY_JOIN_SEQ_END,
};

// The steps of the menu of a player who asks the leader: print the prompt, ask to join, wait for the leader's reply,
// open the windows, wait for the members, synchronize and wait, print the error and wait, print the refusal and wait,
// and cancel and end
enum {
    COMM_ENTRY_ASK_TO_JOIN_SEQ_PRINT_WAIT,
    COMM_ENTRY_ASK_TO_JOIN_SEQ_SEND_ENTRY,
    COMM_ENTRY_ASK_TO_JOIN_SEQ_WAIT_REPLY,
    COMM_ENTRY_ASK_TO_JOIN_SEQ_CREATE_WINDOWS,
    COMM_ENTRY_ASK_TO_JOIN_SEQ_WAIT_MEMBERS,
    COMM_ENTRY_ASK_TO_JOIN_SEQ_SYNC,
    COMM_ENTRY_ASK_TO_JOIN_SEQ_WAIT_SYNC,
    COMM_ENTRY_ASK_TO_JOIN_SEQ_PRINT_ERROR,
    COMM_ENTRY_ASK_TO_JOIN_SEQ_WAIT_ERROR,
    COMM_ENTRY_ASK_TO_JOIN_SEQ_REFUSED,
    COMM_ENTRY_ASK_TO_JOIN_SEQ_WAIT_REFUSED,
    COMM_ENTRY_ASK_TO_JOIN_SEQ_CANCEL,
    COMM_ENTRY_ASK_TO_JOIN_SEQ_END,
};

// The steps of CommEntryMenu_UpdateAsk and CommEntryMenu_AskGroupOk: print the question, open the yes/no menu, wait
// for the answer, and finish
enum {
    COMM_ENTRY_ASK_SEQ_PRINT,
    COMM_ENTRY_ASK_SEQ_OPEN,
    COMM_ENTRY_ASK_SEQ_ANSWER,
    COMM_ENTRY_ASK_SEQ_DONE,
};

// The steps of CommEntryMenu_AskCancel: print the question, open the yes/no menu and wait for the answer, ask again
// when members have joined, and cancel; after the answer, the leader keeps the group, and the answer is returned
enum {
    COMM_ENTRY_CANCEL_SEQ_PRINT,
    COMM_ENTRY_CANCEL_SEQ_OPEN,
    COMM_ENTRY_CANCEL_SEQ_ANSWER,
    COMM_ENTRY_CANCEL_SEQ_OPEN_AGAIN,
    COMM_ENTRY_CANCEL_SEQ_ANSWER_AGAIN,
    COMM_ENTRY_CANCEL_SEQ_CANCEL,
    COMM_ENTRY_CANCEL_SEQ_KEEP = 100,
    COMM_ENTRY_CANCEL_SEQ_END = 0xff,
};

// The steps of the list of the leaders: pick a leader, ask to join once connected, wait for the reply, return the
// result, ask whether to leave and wait for the answer, and wait on the message of a leader that doesn't answer
enum {
    COMM_ENTRY_LIST_SEQ_PICK,
    COMM_ENTRY_LIST_SEQ_SEND_ENTRY,
    COMM_ENTRY_LIST_SEQ_WAIT_REPLY,
    COMM_ENTRY_LIST_SEQ_END,
    COMM_ENTRY_LIST_SEQ_ASK_LEAVE,
    COMM_ENTRY_LIST_SEQ_ANSWER_LEAVE,
    COMM_ENTRY_LIST_SEQ_WAIT_PROBLEM,
};

// The steps of CommEntryMenu_AskLeave: wait for B, and wait for the answer
enum {
    COMM_ENTRY_LEAVE_SEQ_WAIT_B,
    COMM_ENTRY_LEAVE_SEQ_ANSWER,
};

// What CommEntryMenu_AskLeave returns: B wasn't pressed, the question is open, or the player leaves
enum {
    COMM_ENTRY_LEAVE_NONE,
    COMM_ENTRY_LEAVE_ASKING,
    COMM_ENTRY_LEAVE_YES,
};

// The leader's reply to a player who asked to join
enum {
    COMM_ENTRY_REPLY_NONE,
    COMM_ENTRY_REPLY_ACCEPTED,
    COMM_ENTRY_REPLY_REFUSED,
};

// What CommEntryMenu_UpdateLeaderList returns: still picking or waiting, the leader accepted the player or started or
// disbanded the group, the leader refused, or the player left
enum {
    COMM_ENTRY_LEADER_LIST_RUNNING,
    COMM_ENTRY_LEADER_LIST_JOINED,
    COMM_ENTRY_LEADER_LIST_REFUSED,
    COMM_ENTRY_LEADER_LIST_LEFT,
};

// The beacon of the activity's network, which its own setup data fill in
typedef struct {
    PlayerInfo info;
    u8 mac[6];
    u16 gameId;
} CommEntryBeacon;

// What a player sends to ask to join
typedef struct {
    PlayerInfo info;
    u8 mac[6];
    u8 autoAccept;
} CommEntryPacket;

// A player who asked to join the leader, by network ID
typedef struct {
    PlayerInfo info;
    u8 state;
    // Whether to accept it without asking the leader
    u8 autoAccept;
    u8 unk22;
    u8 mac[6];
} CommEntryMember;

// A leader whose beacon was heard
typedef struct {
    PlayerInfo info;
    u8 mac[6];
    u8 valid;
    // The frames until it leaves the list
    s16 timer;
} CommEntryLeader;

// The yes/no menu and the questions that use it
typedef struct {
    ListMenuUI *window;
    u32 result;
    u8 seq;
} CommEntryYesNo;

// The list of the leaders heard, to pick one to ask
typedef struct {
    ListMenuOption *options;
    ListMenuUI *menu;
    CommEntryLeader leaders[COMM_ENTRY_LEADER_MAX];
    CommEntryLeader *selected;
    u32 result;
    u8 seq;
    // Whether a leader came or went since the options were built
    u8 changed;
    u8 optionsBuilt;
    // The step to go back to when the player doesn't leave after all
    u8 prevSeq;
    u16 timer;
} CommEntryLeaderList;

// The members as the leader sends them, by network ID
typedef struct {
    PlayerInfo players[COMM_ENTRY_MEMBER_MAX];
    u8 mask;
} CommEntryMemberList;

// The messages that depend on the activity
typedef struct {
    // Seeking participants
    u16 title;
    // Will you cancel?
    u16 cancelAsk;
    // It will be cancelled
    u16 cancelled;
    // There are no members, which CommEntryMenu_UpdateLeader prints without the table
    u16 noMembers;
    // There appears to be a problem
    u16 error;
    // Choose the leader you would like to join
    u16 chooseLeader;
} CommEntryMessages;

struct CommEntryMenu {
    Field *field;
    WordSet *wordSet;
    MsgData *msgData;
    // The members' names and the title above them
    MoneyWin *membersWin;
    MoneyWin *titleWin;
    FieldMsgWin *msgWin;
    StrBuf *msgBuf;
    StrBuf *memberFmt;
    StrBuf *memberBuf;
    StrBuf *countFmt;
    StrBuf *countBuf;
    u32 result;
    PlayerInfo self;
    CommEntryMember members[COMM_ENTRY_MEMBER_MAX];
    // The player the leader is asked about
    BOOL asking;
    CommEntryMember asked;
    CommEntryYesNo yesNo;
    CommEntryLeaderList list;
    // The network's setup, kept to start it again after a leader refuses
    GFLNetInitData netInit;
    void *netWork;
    void *callbackWork;
    CommEntryMenuCallback onJoin;
    CommEntryMenuCallback onLeave;
    CommEntryMemberList memberList;
    u8 memberListChanged;
    // The leader started the activity, or disbanded the group
    u8 started;
    u8 cancelled;
    // Whether the commands go to every machine, for the Musical
    u8 sendToAll;
    u8 minCount;
    u8 maxCount;
    u8 seq;
    u8 kind;
    u8 mode;
    // The members to send an answer to, as bits
    u8 acceptMask;
    u8 refuseMask;
    // Whether to send the member list, whether it changed again while it was being sent, and whether it was sent
    u8 sendMemberList;
    u8 memberListPending;
    u8 memberListSent;
    u8 checkMask;
    HeapID heapId;
    // The leader's answer, and its info
    u32 reply;
    PlayerInfo replyInfo;
    u8 savedStates[COMM_ENTRY_MEMBER_MAX];
    // A MAC address that the creator gives, which nothing reads
    u8 mac[6];
    // The MAC address of the member each row shows
    u8 shownMacs[COMM_ENTRY_MEMBER_MAX][6];
    s8 shownCount;
    u8 readyCount;
    u16 nextMsg;
    // Whether the leader checked on the members, and whether to answer it
    u8 checked;
    u8 sendCheckReply;
    u8 readyMask;
    u8 showWaitMsg;
    u16 lastMsg;
    u8 unk57C[4];
};

static const u8 sNoMac[6] = { 0 };

// The list of the leaders heard, five rows at a time
static const ListMenuRequest sLeaderListRequest = {
    .count = COMM_ENTRY_LEADER_MAX,
    .maxShown = 5,
    .itemX = 13,
    .fgColor = 1,
    .bgColor = 15,
    .shadowColor = 2,
    .fontSizeX = 12,
    .rowHeight = 16,
    .x = 1,
    .y = 1,
    .width = 22,
    .height = 10,
};

static const CommEntryMessages sCommEntryMessages[] = {
    [COMM_ENTRY_MENU_MULTI_BATTLE] = {
        CommEntryMenu_Text_SeekingMultiBattleParticipants,
        CommEntryMenu_Text_WillCancelMultiBattle,
        CommEntryMenu_Text_MultiBattleWillCanceled,
        CommEntryMenu_Text_ThereNoMembersMulti,
        CommEntryMenu_Text_ThereAppearsProblemMulti,
        // The Musical's, which nothing shows: a multi battle's leader is never picked from the list
        CommEntryMenu_Text_ChooseMusicalWouldLike,
    },
    [COMM_ENTRY_MENU_MUSICAL] = {
        CommEntryMenu_Text_SeekingMusicalParticipants,
        CommEntryMenu_Text_WillCancelMusical,
        CommEntryMenu_Text_MusicalWillCanceled,
        CommEntryMenu_Text_ThereNoMembersMulti,
        CommEntryMenu_Text_ThereAppearsProblemMusical,
        CommEntryMenu_Text_ChooseMusicalWouldLike,
    },
    [COMM_ENTRY_MENU_BATTLE_SUBWAY] = {
        CommEntryMenu_Text_SeekingBattleSubwayParticipants,
        CommEntryMenu_Text_WillCancelBattleSubway,
        CommEntryMenu_Text_BattleSubwayWillCanceled,
        CommEntryMenu_Text_ThereNoMembersMulti,
        CommEntryMenu_Text_ThereAppearsProblemBattle,
        CommEntryMenu_Text_ChooseBattleSubwayWould,
    },
};

static BOOL CommEntryMenu_UpdateLeader(CommEntryMenu *menu);
static BOOL CommEntryMenu_UpdateSelectLeader(CommEntryMenu *menu);
static BOOL CommEntryMenu_UpdateJoin(CommEntryMenu *menu);
static BOOL CommEntryMenu_UpdateAskToJoin(CommEntryMenu *menu);
static void CommEntryMenu_SendAnswers(CommEntryMenu *menu);
static u8 CommEntryMenu_UpdateMemberList(CommEntryMenu *menu);
static void CommEntryMenu_CheckLeft(CommEntryMenu *menu);
static void CommEntryMenu_DrawMembers(CommEntryMenu *menu);
static void CommEntryMenu_UpdateEntries(CommEntryMenu *menu);
static void CommEntryMenu_RemoveMember(CommEntryMenu *menu, int netId);
static BOOL CommEntryMenu_QueueAsk(CommEntryMenu *menu, CommEntryMember *member);
static void CommEntryMenu_UpdateAsk(CommEntryMenu *menu);
static u32 CommEntryMenu_AskGroupOk(CommEntryMenu *menu);
static u32 CommEntryMenu_AskCancel(CommEntryMenu *menu);
static void CommEntryMenu_SetReady(CommEntryMenu *menu, u8 netId);
static void CommEntryMenu_SetStarted(CommEntryMenu *menu);
static void CommEntryMenu_SetCancelled(CommEntryMenu *menu);
static void CommEntryMenu_SetChecked(CommEntryMenu *menu);
static int CommEntryMenu_CountAccepted(CommEntryMenu *menu);
static u32 CommEntryMenu_GetAcceptedMask(CommEntryMenu *menu);
static BOOL CommEntryMenu_IsIdle(CommEntryMenu *menu);
static void CommEntryMenu_CreateWindows(CommEntryMenu *menu);
static void CommEntryMenu_FreeWindows(CommEntryMenu *menu);
static BOOL CommEntryMenu_PrintCount(CommEntryMenu *menu, int count);
static void CommEntryMenu_DrawMember(CommEntryMenu *menu, CommEntryMember *member, u32 netId);
static void CommEntryMenu_ClearMember(CommEntryMenu *menu, CommEntryMember *member, u32 netId);
static void CommEntryMenu_PrintMember(CommEntryMenu *menu, PlayerInfo *info, u8 row);
static void CommEntryMenu_ClearRow(CommEntryMenu *menu, int row);
static void CommEntryMenu_DrawMemberList(CommEntryMenu *menu);
static void CommEntryMenu_PrintMessage(CommEntryMenu *menu, int msgId);
static void CommEntryMenu_SaveStates(CommEntryMenu *menu);
static BOOL CommEntryMenu_StatesUnchanged(CommEntryMenu *menu);
static void CommEntryMenu_ClearAnswer(CommEntryMenu *menu, u8 netId);
static void CommEntryMenu_RequestMemberList(CommEntryMenu *menu);
static void CommEntryMenu_OnMemberListSent(CommEntryMenu *menu);
static CommEntryMemberList *CommEntryMenu_GetMemberListBuffer(CommEntryMenu *menu);
static void CommEntryMenu_SetMemberList(CommEntryMenu *menu, const CommEntryMemberList *list);
static void CommEntryMenu_CreateLeaderList(CommEntryMenu *menu);
static void CommEntryMenu_FreeLeaderList(CommEntryMenu *menu);
static u32 CommEntryMenu_UpdateLeaderList(CommEntryMenu *menu);
static BOOL CommEntryMenu_UpdateLeaderListInput(CommEntryMenu *menu);
static void CommEntryMenu_AgeLeaders(CommEntryMenu *menu);
static void CommEntryMenu_ScanBeacons(CommEntryMenu *menu);
static void CommEntryMenu_AddLeader(CommEntryMenu *menu, const u8 *mac, const PlayerInfo *info);
static void CommEntryMenu_RemoveLeader(CommEntryMenu *menu, int index);
static void CommEntryMenu_BuildLeaderOptions(CommEntryMenu *menu);
static u32 CommEntryMenu_AskLeave(CommEntryMenu *menu);
static void CommEntryMenu_CloseYesNo(CommEntryMenu *menu);
static void CommEntryMenu_SetReply(CommEntryMenu *menu, u32 reply, const PlayerInfo *info);
static int CommEntryMenu_GetMemberListSize(void);
static BOOL CommEntryMenu_AnyConnected(u32 mask);
static void CommEntryMenu_AddCommands(CommEntryMenu *menu);
static void CommEntryMenu_RemoveCommands(void);
static void *CommEntryMenu_GetMemberListRecvBuffer(int netId, void *work, int size);
static void CommEntryMenu_RecvEntry(int netId, int size, const void *data, void *work, NetHandle *handle);
static BOOL CommEntryMenu_SendEntry(const PlayerInfo *info, u8 autoAccept, BOOL toAll);
static void CommEntryMenu_RecvAccept(int netId, int size, const void *data, void *work, NetHandle *handle);
static BOOL CommEntryMenu_SendAccept(u8 netId, BOOL toAll, PlayerInfo *info);
static void CommEntryMenu_RecvRefuse(int netId, int size, const void *data, void *work, NetHandle *handle);
static BOOL CommEntryMenu_SendRefuse(u8 netId, BOOL toAll, PlayerInfo *info);
static void CommEntryMenu_RecvCheck(int netId, int size, const void *data, void *work, NetHandle *handle);
static BOOL CommEntryMenu_SendCheck(BOOL toAll);
static void CommEntryMenu_RecvCheckReply(int netId, int size, const void *data, void *work, NetHandle *handle);
static BOOL CommEntryMenu_SendCheckReply(BOOL toAll);
static void CommEntryMenu_RecvStart(int netId, int size, const void *data, void *work, NetHandle *handle);
static BOOL CommEntryMenu_SendStart(BOOL toAll);
static void CommEntryMenu_RecvCancel(int netId, int size, const void *data, void *work, NetHandle *handle);
static BOOL CommEntryMenu_SendCancel(BOOL toAll);
static void CommEntryMenu_RecvMemberList(int netId, int size, const void *data, void *work, NetHandle *handle);
static BOOL CommEntryMenu_SendMemberList(CommEntryMemberList *list, u32 netIds, BOOL toAll);

// The messages of the menu's activity
static inline const CommEntryMessages *CommEntryMenu_GetMessages(CommEntryMenu *menu) {
    return &sCommEntryMessages[menu->mode];
}

// The MAC address of the member that a row shows
static inline u8 *CommEntryMenu_GetShownMac(CommEntryMenu *menu, int row) {
    return menu->shownMacs[row];
}

// This machine's beacon, which the activity's network setup gives
static inline CommEntryBeacon *CommEntryMenu_GetBeacon(void) {
    return GFL_NetGetInitData()->getBeaconData(GFL_NetGetWork());
}

// Whether the member of a network ID is one the leader checks on before the activity starts
static inline BOOL CommEntryMenu_IsChecked(CommEntryMenu *menu, u32 netId) {
    return menu->checkMask & 1 << netId;
}

static const NetCommand sCommEntryNetCommands[] = {
    [COMM_ENTRY_CMD_ENTRY - COMM_ENTRY_CMD_ENTRY] = { CommEntryMenu_RecvEntry, NULL },
    [COMM_ENTRY_CMD_ACCEPT - COMM_ENTRY_CMD_ENTRY] = { CommEntryMenu_RecvAccept, NULL },
    [COMM_ENTRY_CMD_REFUSE - COMM_ENTRY_CMD_ENTRY] = { CommEntryMenu_RecvRefuse, NULL },
    [COMM_ENTRY_CMD_CHECK - COMM_ENTRY_CMD_ENTRY] = { CommEntryMenu_RecvCheck, NULL },
    [COMM_ENTRY_CMD_CHECK_REPLY - COMM_ENTRY_CMD_ENTRY] = { CommEntryMenu_RecvCheckReply, NULL },
    [COMM_ENTRY_CMD_START - COMM_ENTRY_CMD_ENTRY] = { CommEntryMenu_RecvStart, NULL },
    [COMM_ENTRY_CMD_CANCEL - COMM_ENTRY_CMD_ENTRY] = { CommEntryMenu_RecvCancel, NULL },
    [COMM_ENTRY_CMD_MEMBER_LIST - COMM_ENTRY_CMD_ENTRY] = { CommEntryMenu_RecvMemberList,
                                                            CommEntryMenu_GetMemberListRecvBuffer },
};

CommEntryMenu *CommEntryMenu_Create(const PlayerInfo *info, Field *field, u32 minCount, u32 maxCount, HeapID heapId,
                                    u32 kind, u32 mode, const u8 *mac) {
    FieldMsgBG *msgBGSys = Field_GetMsgBGSys(field);
    CommEntryMenu *menu = GFL_HeapAllocate(heapId, sizeof(CommEntryMenu), TRUE, "comm_entry_menu.c", 377);

    menu->wordSet = GFL_WordSetSystemCreateDefault(heapId);
    menu->msgData = func_ov036_021879a0(msgBGSys, TEXT_BANK_COMM_ENTRY_MENU);
    menu->msgWin = func_ov036_0218845c(msgBGSys, menu->msgData);
    menu->msgBuf = GFL_StrBufCreate(100, heapId);
    menu->memberFmt = GFL_MsgDataLoadStrbufNew(menu->msgData, CommEntryMenu_Text_Id);
    menu->memberBuf = GFL_StrBufCreate(64, heapId);
    if (mode == COMM_ENTRY_MENU_MUSICAL) {
        menu->countFmt = GFL_MsgDataLoadStrbufNew(menu->msgData, CommEntryMenu_Text_UpMoreAllowed);
    } else {
        menu->countFmt = GFL_MsgDataLoadStrbufNew(menu->msgData, CommEntryMenu_Text_MoreNeeded);
    }
    menu->countBuf = GFL_StrBufCreate(32, heapId);
    menu->minCount = minCount;
    menu->maxCount = maxCount;
    menu->field = field;
    menu->kind = kind;
    menu->mode = mode;
    menu->heapId = heapId;
    menu->shownCount = -1;
    menu->nextMsg = COMM_ENTRY_NO_MESSAGE;
    PlayerInfo_Copy(info, &menu->self);
    if (mac != NULL) {
        MI_CpuCopy8(mac, menu->mac, sizeof(menu->mac));
    }
    GFL_NetSetDisconnectError(FALSE);
    CommEntryMenu_AddCommands(menu);
    if (mode == COMM_ENTRY_MENU_MUSICAL) {
        menu->sendToAll = TRUE;
    }
    CommEntryMenu_GetBeacon()->gameId = COMM_ENTRY_BEACON_GAME_ID;
    return menu;
}

u32 CommEntryMenu_Free(CommEntryMenu *menu) {
    u32 joined = CommEntryMenu_GetAcceptedMask(menu);

    CommEntryMenu_FreeWindows(menu);
    if (menu->yesNo.window != NULL) {
        func_ov036_02187ea0(menu->yesNo.window);
        menu->yesNo.window = NULL;
    }
    GFL_StrBufFree(menu->countBuf);
    GFL_StrBufFree(menu->countFmt);
    GFL_StrBufFree(menu->memberBuf);
    GFL_StrBufFree(menu->memberFmt);
    GFL_StrBufFree(menu->msgBuf);
    func_ov036_02188338(menu->msgWin);
    func_ov036_021879b8(menu->msgData);
    GFL_WordSetSystemFree(menu->wordSet);
    if (GFL_NetIsInitialized() == TRUE) {
        CommEntryMenu_RemoveCommands();
    }
    GFL_HeapFree(menu);
    return joined;
}

void CommEntryMenu_SetCallbacks(CommEntryMenu *menu, void *work, CommEntryMenuCallback onJoin,
                                CommEntryMenuCallback onLeave) {
    menu->callbackWork = work;
    menu->onJoin = onJoin;
    menu->onLeave = onLeave;
}

void CommEntryMenu_AddEntry(CommEntryMenu *menu, u32 netId, const PlayerInfo *info, u8 autoAccept, const u8 *mac) {
    CommEntryMember *member = &menu->members[netId];

    PlayerInfo_Copy(info, &member->info);
    member->state = COMM_ENTRY_MEMBER_NEW;
    member->autoAccept = autoAccept;
    MI_CpuCopy8(mac, member->mac, sizeof(member->mac));
    CommEntryMenu_ClearAnswer(menu, netId);
    CommEntryMenu_RequestMemberList(menu);
    if (menu->onJoin != NULL) {
        menu->onJoin(menu->callbackWork, info, mac);
    }
}

u32 CommEntryMenu_Update(CommEntryMenu *menu) {
    BOOL done;

    switch (menu->kind) {
    case COMM_ENTRY_MENU_LEADER:
        done = CommEntryMenu_UpdateLeader(menu);
        break;
    case COMM_ENTRY_MENU_SELECT_LEADER:
        done = CommEntryMenu_UpdateSelectLeader(menu);
        break;
    case COMM_ENTRY_MENU_JOIN:
        done = CommEntryMenu_UpdateJoin(menu);
        break;
    case COMM_ENTRY_MENU_ASK_TO_JOIN:
        done = CommEntryMenu_UpdateAskToJoin(menu);
        break;
    }
    if (done == TRUE) {
        return menu->result;
    }
    return COMM_ENTRY_MENU_RUNNING;
}

// The leader's menu
static BOOL CommEntryMenu_UpdateLeader(CommEntryMenu *menu) {
    u8 mask;
    u8 i;

    if (menu->seq != COMM_ENTRY_LEADER_SEQ_INIT) {
        CommEntryMenu_CheckLeft(menu);
        CommEntryMenu_DrawMembers(menu);
        CommEntryMenu_SendAnswers(menu);
    }
    switch (menu->seq) {
    case COMM_ENTRY_LEADER_SEQ_INIT:
        CommEntryMenu_CreateWindows(menu);
        CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_AwaitingParticipatingMembersB);
        menu->seq++;
        break;
    case COMM_ENTRY_LEADER_SEQ_START_SERVER:
        if (menu->mode == COMM_ENTRY_MENU_MULTI_BATTLE) {
            menu->seq = COMM_ENTRY_LEADER_SEQ_SEND_SELF;
        } else {
            GFL_NetStartServer();
            menu->seq++;
        }
        break;
    case COMM_ENTRY_LEADER_SEQ_WAIT_CONNECT:
        if (NetSystem_CountConnected() > 1) {
            if (NetHandle_SendNegotiation() == TRUE) {
                menu->seq++;
            }
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            // The cancel question's step is set and replaced at once: the menu ends without asking
            menu->seq = COMM_ENTRY_LEADER_SEQ_ASK_CANCEL;
            menu->result = COMM_ENTRY_MENU_CANCELLED;
            menu->seq = COMM_ENTRY_LEADER_SEQ_END;
        }
        break;
    case COMM_ENTRY_LEADER_SEQ_WAIT_NEGOTIATE:
        if (NetHandle_IsNegotiated(NetHandle_GetOwn()) == TRUE) {
            menu->seq = COMM_ENTRY_LEADER_SEQ_SEND_SELF;
        }
        break;
    case COMM_ENTRY_LEADER_SEQ_SEND_SELF:
        if (CommEntryMenu_SendEntry(&menu->self, TRUE, menu->sendToAll) == TRUE) {
            menu->seq++;
        }
        break;
    case COMM_ENTRY_LEADER_SEQ_GATHER: {
        int count;
        u16 msg;

        CommEntryMenu_UpdateEntries(menu);
        CommEntryMenu_UpdateAsk(menu);
        if (CommEntryMenu_IsIdle(menu) != TRUE) {
            break;
        }
        count = CommEntryMenu_CountAccepted(menu);
        msg = menu->nextMsg;
        menu->nextMsg = COMM_ENTRY_NO_MESSAGE;
        if (count >= menu->maxCount || (count >= menu->minCount && (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A))) {
            CommEntryMenu_SaveStates(menu);
            menu->seq = COMM_ENTRY_LEADER_SEQ_ASK_GROUP_OK;
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            menu->seq = COMM_ENTRY_LEADER_SEQ_ASK_CANCEL;
        } else if (menu->mode == COMM_ENTRY_MENU_MULTI_BATTLE && GFL_NetGetNegotiatedCount() <= 1) {
            menu->seq = COMM_ENTRY_LEADER_SEQ_PRINT_NO_MEMBERS;
        } else if (msg != COMM_ENTRY_NO_MESSAGE) {
            CommEntryMenu_PrintMessage(menu, msg);
        }
        break;
    }
    case COMM_ENTRY_LEADER_SEQ_ASK_GROUP_OK: {
        u32 answer = CommEntryMenu_AskGroupOk(menu);

        if (answer == COMM_ENTRY_ASK_YES) {
            GFL_NetSetAcceptConnect(NetHandle_GetOwn(), 0);
            menu->seq = COMM_ENTRY_LEADER_SEQ_SEND_CHECK;
        } else if (answer == COMM_ENTRY_ASK_NO) {
            if (CommEntryMenu_CountAccepted(menu) >= menu->maxCount) {
                menu->seq = COMM_ENTRY_LEADER_SEQ_ASK_CANCEL;
            } else {
                menu->seq = COMM_ENTRY_LEADER_SEQ_GATHER;
                if (CommEntryMenu_CountAccepted(menu) > 1 && menu->mode == COMM_ENTRY_MENU_MUSICAL) {
                    menu->nextMsg = CommEntryMenu_Text_WhenGroupReadyButton;
                } else {
                    menu->nextMsg = CommEntryMenu_Text_AwaitingParticipatingMembersB;
                }
            }
        }
        break;
    }
    case COMM_ENTRY_LEADER_SEQ_ASK_CANCEL: {
        u32 answer = CommEntryMenu_AskCancel(menu);

        if (answer == COMM_ENTRY_ASK_YES) {
            GFL_NetSetAcceptConnect(NetHandle_GetOwn(), 0);
            menu->seq = COMM_ENTRY_LEADER_SEQ_SEND_DISBAND;
        } else if (answer == COMM_ENTRY_ASK_NO) {
            menu->seq = COMM_ENTRY_LEADER_SEQ_GATHER;
        }
        break;
    }
    case COMM_ENTRY_LEADER_SEQ_SEND_CHECK:
        if (CommEntryMenu_SendCheck(menu->sendToAll) == TRUE) {
            CommEntryMenu_SetReady(menu, GFL_NetGetHandleNetId(NetHandle_GetOwn()));
            menu->checkMask = menu->memberList.mask;
            GFL_NetSetDisconnectError(TRUE);
            menu->seq = COMM_ENTRY_LEADER_SEQ_WAIT_CHECK;
        }
        break;
    case COMM_ENTRY_LEADER_SEQ_WAIT_CHECK:
        mask = 1 << GFL_NetGetHandleNetId(NetHandle_GetOwn());

        for (i = 0; (i < COMM_ENTRY_MEMBER_MAX) ? TRUE : FALSE; i++) {
            if (i != GFL_NetGetHandleNetId(NetHandle_GetOwn()) && CommEntryMenu_IsChecked(menu, i)) {
                if (GFL_NetIsNegotiated(i) == TRUE) {
                    if (menu->readyMask & 1 << i) {
                        mask |= 1U << i;
                    }
                } else {
                    menu->seq = COMM_ENTRY_LEADER_SEQ_SEND_CANCEL;
                    break;
                }
            }
        }
        if (menu->checkMask == mask) {
            if (MATH_CountPopulation(menu->readyMask) < menu->minCount) {
                menu->seq = COMM_ENTRY_LEADER_SEQ_SEND_CANCEL;
            } else {
                menu->seq = COMM_ENTRY_LEADER_SEQ_SEND_START;
            }
        }
        break;
    case COMM_ENTRY_LEADER_SEQ_SEND_CANCEL:
        if (CommEntryMenu_SendCancel(menu->sendToAll) == TRUE) {
            menu->checkMask = menu->memberList.mask;
            menu->result = COMM_ENTRY_MENU_CANCELLED;
            menu->seq = COMM_ENTRY_LEADER_SEQ_PRINT_ERROR;
        }
        break;
    case COMM_ENTRY_LEADER_SEQ_SEND_START:
        if (CommEntryMenu_SendStart(menu->sendToAll) == TRUE) {
            menu->result = COMM_ENTRY_MENU_DONE;
            menu->seq = COMM_ENTRY_LEADER_SEQ_WAIT_SYNC;
            menu->readyCount = MATH_CountPopulation(menu->readyMask);
            NetHandle_SendTimingPair(NetHandle_GetOwn(), COMM_ENTRY_SYNC_START, COMM_ENTRY_SYNC_CLASS);
        }
        break;
    case COMM_ENTRY_LEADER_SEQ_WAIT_SYNC:
        if (menu->readyCount != NetSystem_CountConnected() || menu->minCount > NetSystem_CountConnected()) {
            if (CommEntryMenu_SendCancel(menu->sendToAll) == TRUE) {
                menu->checkMask = menu->memberList.mask;
                menu->result = COMM_ENTRY_MENU_CANCELLED;
                menu->seq = COMM_ENTRY_LEADER_SEQ_PRINT_ERROR;
            }
        } else if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), COMM_ENTRY_SYNC_START, COMM_ENTRY_SYNC_CLASS) ==
                   TRUE) {
            menu->result = COMM_ENTRY_MENU_DONE;
            menu->seq = COMM_ENTRY_LEADER_SEQ_END;
            // Stop the players who look for a leader from finding this one
            CommEntryMenu_GetBeacon()->gameId = 0;
        }
        break;
    case COMM_ENTRY_LEADER_SEQ_SEND_DISBAND:
        if (CommEntryMenu_SendCancel(menu->sendToAll) == TRUE) {
            menu->checkMask = menu->memberList.mask;
            menu->result = COMM_ENTRY_MENU_CANCELLED;
            menu->seq = COMM_ENTRY_LEADER_SEQ_WAIT_DISCONNECT;
        }
        break;
    case COMM_ENTRY_LEADER_SEQ_PRINT_ERROR:
        CommEntryMenu_PrintMessage(menu, CommEntryMenu_GetMessages(menu)->error);
        menu->seq = COMM_ENTRY_LEADER_SEQ_WAIT_ERROR;
        break;
    case COMM_ENTRY_LEADER_SEQ_WAIT_ERROR:
        if (func_ov036_021883e8(menu->msgWin) == TRUE) {
            menu->seq = COMM_ENTRY_LEADER_SEQ_WAIT_DISCONNECT;
        }
        break;
    case COMM_ENTRY_LEADER_SEQ_WAIT_DISCONNECT:
        // Wait for the members to disconnect
        if (CommEntryMenu_AnyConnected(menu->checkMask) == FALSE) {
            menu->seq = COMM_ENTRY_LEADER_SEQ_END;
        }
        break;
    case COMM_ENTRY_LEADER_SEQ_PRINT_NO_MEMBERS:
        CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_ThereNoMembersMulti);
        menu->seq = COMM_ENTRY_LEADER_SEQ_WAIT_NO_MEMBERS;
        break;
    case COMM_ENTRY_LEADER_SEQ_WAIT_NO_MEMBERS:
        if (func_ov036_021883e8(menu->msgWin) == TRUE) {
            menu->result = COMM_ENTRY_MENU_CANCELLED;
            menu->seq = COMM_ENTRY_LEADER_SEQ_END;
        }
        break;
    case COMM_ENTRY_LEADER_SEQ_END:
        CommEntryMenu_FreeWindows(menu);
        return TRUE;
    }
    if (GFL_NetErrCheck()) {
        menu->result = COMM_ENTRY_MENU_ERROR;
        return TRUE;
    }
    return FALSE;
}

// The menu that picks a leader from the beacons heard and asks to join it
static BOOL CommEntryMenu_UpdateSelectLeader(CommEntryMenu *menu) {
    switch (menu->seq) {
    case COMM_ENTRY_SELECT_SEQ_PRINT_CHOOSE:
        CommEntryMenu_PrintMessage(menu, CommEntryMenu_GetMessages(menu)->chooseLeader);
        menu->seq++;
        break;
    case COMM_ENTRY_SELECT_SEQ_WAIT_CHOOSE:
        if (func_ov036_021883e8(menu->msgWin) == TRUE) {
            menu->seq++;
        }
        break;
    case COMM_ENTRY_SELECT_SEQ_START_SCAN:
        CommEntryMenu_FreeLeaderList(menu);
        CommEntryMenu_FreeWindows(menu);
        menu->list.selected = NULL;
        CommEntryMenu_CreateLeaderList(menu);
        GFL_NetStartScan();
        menu->seq++;
        break;
    case COMM_ENTRY_SELECT_SEQ_LIST:
        switch (CommEntryMenu_UpdateLeaderList(menu)) {
        case COMM_ENTRY_LEADER_LIST_JOINED:
            CommEntryMenu_FreeLeaderList(menu);
            if (menu->started == TRUE) {
                menu->seq = COMM_ENTRY_SELECT_SEQ_SYNC;
            } else if (menu->cancelled == TRUE) {
                menu->seq = COMM_ENTRY_SELECT_SEQ_PRINT_ERROR;
            } else {
                copyVarForText(menu->wordSet, 0, &menu->list.selected->info);
                CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_RepliedOk);
                menu->seq = COMM_ENTRY_SELECT_SEQ_WAIT_REPLIED;
            }
            break;
        case COMM_ENTRY_LEADER_LIST_REFUSED:
            CommEntryMenu_FreeLeaderList(menu);
            menu->seq = COMM_ENTRY_SELECT_SEQ_REFUSED;
            break;
        case COMM_ENTRY_LEADER_LIST_LEFT:
            CommEntryMenu_FreeLeaderList(menu);
            menu->seq = COMM_ENTRY_SELECT_SEQ_CANCEL;
            break;
        }
        break;
    case COMM_ENTRY_SELECT_SEQ_WAIT_REPLIED:
        if (func_ov036_021883e8(menu->msgWin) == TRUE) {
            CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_PleaseWaitBButton_2);
            CommEntryMenu_CreateWindows(menu);
            menu->seq++;
        }
        break;
    case COMM_ENTRY_SELECT_SEQ_WAIT_MEMBERS:
        if (CommEntryMenu_AskLeave(menu) == COMM_ENTRY_LEAVE_YES) {
            CommEntryMenu_FreeWindows(menu);
            CommEntryMenu_CloseYesNo(menu);
            menu->seq = COMM_ENTRY_SELECT_SEQ_CANCEL;
        } else {
            if (menu->sendCheckReply == TRUE && CommEntryMenu_SendCheckReply(menu->sendToAll) == TRUE) {
                menu->sendCheckReply = FALSE;
            }
            CommEntryMenu_DrawMemberList(menu);
            if (menu->started == TRUE) {
                CommEntryMenu_FreeWindows(menu);
                CommEntryMenu_CloseYesNo(menu);
                menu->seq = COMM_ENTRY_SELECT_SEQ_SYNC;
            } else if (menu->cancelled == TRUE) {
                CommEntryMenu_FreeWindows(menu);
                CommEntryMenu_CloseYesNo(menu);
                menu->seq = COMM_ENTRY_SELECT_SEQ_PRINT_ERROR;
            }
        }
        break;
    case COMM_ENTRY_SELECT_SEQ_SYNC:
        if (menu->mode == COMM_ENTRY_MENU_MULTI_BATTLE) {
            CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_MembersHaveGatheredPlease);
        }
        menu->seq = COMM_ENTRY_SELECT_SEQ_WAIT_SYNC;
        NetHandle_SendTimingPair(NetHandle_GetOwn(), COMM_ENTRY_SYNC_START, COMM_ENTRY_SYNC_CLASS);
        break;
    case COMM_ENTRY_SELECT_SEQ_WAIT_SYNC:
        if (menu->mode == COMM_ENTRY_MENU_MULTI_BATTLE && func_ov036_021883e8(menu->msgWin) != TRUE) {
            break;
        }
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), COMM_ENTRY_SYNC_START, COMM_ENTRY_SYNC_CLASS) == TRUE) {
            menu->result = COMM_ENTRY_MENU_DONE;
            menu->seq = COMM_ENTRY_SELECT_SEQ_END;
        } else if (menu->cancelled == TRUE) {
            menu->seq = COMM_ENTRY_SELECT_SEQ_PRINT_ERROR;
        }
        break;
    case COMM_ENTRY_SELECT_SEQ_PRINT_ERROR:
        CommEntryMenu_PrintMessage(menu, CommEntryMenu_GetMessages(menu)->error);
        menu->seq = COMM_ENTRY_SELECT_SEQ_WAIT_ERROR;
        break;
    case COMM_ENTRY_SELECT_SEQ_WAIT_ERROR:
        if (func_ov036_021883e8(menu->msgWin) == TRUE) {
            menu->seq = COMM_ENTRY_SELECT_SEQ_CANCEL;
        }
        break;
    case COMM_ENTRY_SELECT_SEQ_REFUSED:
        // The leader refused: start the network again, to pick another
        menu->netWork = GFL_NetGetWork();
        MI_CpuCopy8(GFL_NetGetInitData(), &menu->netInit, sizeof(GFLNetInitData));
        CommEntryMenu_RemoveCommands();
        GFL_NetExit(NULL);
        copyVarForText(menu->wordSet, 0, &menu->replyInfo);
        CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_Refused);
        menu->seq = COMM_ENTRY_SELECT_SEQ_WAIT_NET_EXIT;
        break;
    case COMM_ENTRY_SELECT_SEQ_WAIT_NET_EXIT:
        if (GFL_NetIsEnded() == TRUE && func_ov036_021883e8(menu->msgWin) == TRUE) {
            menu->seq = COMM_ENTRY_SELECT_SEQ_NET_INIT;
        }
        break;
    case COMM_ENTRY_SELECT_SEQ_NET_INIT:
        GFL_NetInit(&menu->netInit, NULL, menu->netWork);
        menu->seq = COMM_ENTRY_SELECT_SEQ_WAIT_NET_INIT;
        break;
    case COMM_ENTRY_SELECT_SEQ_WAIT_NET_INIT:
        if (GFL_NetIsInitialized() == TRUE) {
            CommEntryMenu_AddCommands(menu);
            menu->seq = COMM_ENTRY_SELECT_SEQ_PRINT_CHOOSE;
        }
        break;
    case COMM_ENTRY_SELECT_SEQ_CANCEL:
        menu->result = COMM_ENTRY_MENU_CANCELLED;
        menu->seq = COMM_ENTRY_SELECT_SEQ_END;
        break;
    case COMM_ENTRY_SELECT_SEQ_END:
        CommEntryMenu_FreeLeaderList(menu);
        CommEntryMenu_FreeWindows(menu);
        return TRUE;
    }
    if (GFL_NetErrCheck()) {
        CommEntryMenu_CloseYesNo(menu);
        CommEntryMenu_FreeLeaderList(menu);
        menu->result = COMM_ENTRY_MENU_ERROR;
        return TRUE;
    }
    return FALSE;
}

// The menu of a player the leader accepts without asking
static BOOL CommEntryMenu_UpdateJoin(CommEntryMenu *menu) {
    switch (menu->seq) {
    case COMM_ENTRY_JOIN_SEQ_PRINT_WAIT:
        CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_PleaseWaitBButton);
        menu->seq++;
        break;
    case COMM_ENTRY_JOIN_SEQ_SEND_ENTRY:
        if (CommEntryMenu_SendEntry(&menu->self, TRUE, menu->sendToAll) == TRUE) {
            menu->seq++;
        }
        break;
    case COMM_ENTRY_JOIN_SEQ_CREATE_WINDOWS:
        CommEntryMenu_CreateWindows(menu);
        menu->seq++;
        break;
    case COMM_ENTRY_JOIN_SEQ_WAIT_MEMBERS:
        if (CommEntryMenu_AskLeave(menu) == COMM_ENTRY_LEAVE_YES) {
            CommEntryMenu_FreeWindows(menu);
            CommEntryMenu_CloseYesNo(menu);
            menu->seq = COMM_ENTRY_JOIN_SEQ_CANCEL;
        } else {
            if (menu->sendCheckReply == TRUE && CommEntryMenu_SendCheckReply(menu->sendToAll) == TRUE) {
                menu->sendCheckReply = FALSE;
            }
            CommEntryMenu_DrawMemberList(menu);
            if (menu->started == TRUE) {
                CommEntryMenu_FreeWindows(menu);
                CommEntryMenu_CloseYesNo(menu);
                menu->seq = COMM_ENTRY_JOIN_SEQ_SYNC;
            } else if (menu->cancelled == TRUE) {
                CommEntryMenu_FreeWindows(menu);
                CommEntryMenu_CloseYesNo(menu);
                menu->seq = COMM_ENTRY_JOIN_SEQ_PRINT_ERROR;
            }
        }
        break;
    case COMM_ENTRY_JOIN_SEQ_SYNC:
        if (menu->mode == COMM_ENTRY_MENU_MULTI_BATTLE) {
            CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_MembersHaveGatheredPlease);
        }
        menu->seq = COMM_ENTRY_JOIN_SEQ_WAIT_SYNC;
        NetHandle_SendTimingPair(NetHandle_GetOwn(), COMM_ENTRY_SYNC_START, COMM_ENTRY_SYNC_CLASS);
        break;
    case COMM_ENTRY_JOIN_SEQ_WAIT_SYNC:
        if (menu->mode == COMM_ENTRY_MENU_MULTI_BATTLE && func_ov036_021883e8(menu->msgWin) != TRUE) {
            break;
        }
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), COMM_ENTRY_SYNC_START, COMM_ENTRY_SYNC_CLASS) == TRUE) {
            menu->result = COMM_ENTRY_MENU_DONE;
            menu->seq = COMM_ENTRY_JOIN_SEQ_END;
        } else if (menu->cancelled == TRUE) {
            menu->seq = COMM_ENTRY_JOIN_SEQ_PRINT_ERROR;
        }
        break;
    case COMM_ENTRY_JOIN_SEQ_PRINT_ERROR:
        CommEntryMenu_PrintMessage(menu, CommEntryMenu_GetMessages(menu)->error);
        menu->seq = COMM_ENTRY_JOIN_SEQ_WAIT_ERROR;
        break;
    case COMM_ENTRY_JOIN_SEQ_WAIT_ERROR:
        if (func_ov036_021883e8(menu->msgWin) == TRUE) {
            menu->result = COMM_ENTRY_MENU_CANCELLED;
            menu->seq = COMM_ENTRY_JOIN_SEQ_END;
        }
        break;
    case COMM_ENTRY_JOIN_SEQ_REFUSED:
        // Nothing sets steps 8 and 9, the leader's refusal as CommEntryMenu_UpdateAskToJoin has it. The leader
        // list's selected leader they print is NULL in this menu, which never creates the list
        GFL_NetExit(NULL);
        copyVarForText(menu->wordSet, 0, &menu->list.selected->info);
        CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_Refused);
        menu->seq = COMM_ENTRY_JOIN_SEQ_WAIT_REFUSED;
        break;
    case COMM_ENTRY_JOIN_SEQ_WAIT_REFUSED:
        if (GFL_NetIsEnded() == TRUE && func_ov036_021883e8(menu->msgWin) == TRUE) {
            menu->result = COMM_ENTRY_MENU_REFUSED;
            menu->seq = COMM_ENTRY_JOIN_SEQ_END;
        }
        break;
    case COMM_ENTRY_JOIN_SEQ_CANCEL:
        menu->result = COMM_ENTRY_MENU_CANCELLED;
        menu->seq = COMM_ENTRY_JOIN_SEQ_END;
        break;
    case COMM_ENTRY_JOIN_SEQ_END:
        CommEntryMenu_FreeWindows(menu);
        return TRUE;
    }
    if (GFL_NetErrCheck()) {
        CommEntryMenu_CloseYesNo(menu);
        menu->result = COMM_ENTRY_MENU_ERROR;
        return TRUE;
    }
    return FALSE;
}

// The menu of a player who asks the leader to join
static BOOL CommEntryMenu_UpdateAskToJoin(CommEntryMenu *menu) {
    switch (menu->seq) {
    case COMM_ENTRY_ASK_TO_JOIN_SEQ_PRINT_WAIT:
        CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_PleaseWaitBButton);
        menu->seq++;
        break;
    case COMM_ENTRY_ASK_TO_JOIN_SEQ_SEND_ENTRY:
        menu->reply = COMM_ENTRY_REPLY_NONE;
        if (CommEntryMenu_SendEntry(&menu->self, FALSE, menu->sendToAll) == TRUE) {
            menu->seq = COMM_ENTRY_ASK_TO_JOIN_SEQ_WAIT_REPLY;
        }
        break;
    case COMM_ENTRY_ASK_TO_JOIN_SEQ_WAIT_REPLY:
        if (CommEntryMenu_AskLeave(menu) == COMM_ENTRY_LEAVE_YES) {
            CommEntryMenu_FreeWindows(menu);
            // The window is closed twice, and the second call does nothing
            CommEntryMenu_CloseYesNo(menu);
            CommEntryMenu_CloseYesNo(menu);
            menu->seq = COMM_ENTRY_ASK_TO_JOIN_SEQ_CANCEL;
        } else if (menu->reply == COMM_ENTRY_REPLY_ACCEPTED) {
            menu->seq = COMM_ENTRY_ASK_TO_JOIN_SEQ_CREATE_WINDOWS;
            CommEntryMenu_CloseYesNo(menu);
            CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_AwaitingParticipatingMembersB);
        } else if (menu->reply == COMM_ENTRY_REPLY_REFUSED) {
            menu->seq = COMM_ENTRY_ASK_TO_JOIN_SEQ_REFUSED;
            CommEntryMenu_CloseYesNo(menu);
        } else if (menu->started == TRUE) {
            menu->seq = COMM_ENTRY_ASK_TO_JOIN_SEQ_SYNC;
            CommEntryMenu_CloseYesNo(menu);
        } else if (menu->cancelled == TRUE) {
            menu->seq = COMM_ENTRY_ASK_TO_JOIN_SEQ_CANCEL;
            CommEntryMenu_CloseYesNo(menu);
        }
        break;
    case COMM_ENTRY_ASK_TO_JOIN_SEQ_CREATE_WINDOWS:
        CommEntryMenu_CreateWindows(menu);
        menu->seq++;
        break;
    case COMM_ENTRY_ASK_TO_JOIN_SEQ_WAIT_MEMBERS:
        if (CommEntryMenu_AskLeave(menu) == COMM_ENTRY_LEAVE_YES) {
            CommEntryMenu_FreeWindows(menu);
            CommEntryMenu_CloseYesNo(menu);
            menu->seq = COMM_ENTRY_ASK_TO_JOIN_SEQ_CANCEL;
        } else {
            if (menu->sendCheckReply == TRUE && CommEntryMenu_SendCheckReply(menu->sendToAll) == TRUE) {
                menu->sendCheckReply = FALSE;
            }
            CommEntryMenu_DrawMemberList(menu);
            if (menu->started == TRUE) {
                CommEntryMenu_FreeWindows(menu);
                CommEntryMenu_CloseYesNo(menu);
                menu->seq = COMM_ENTRY_ASK_TO_JOIN_SEQ_SYNC;
            } else if (menu->cancelled == TRUE) {
                CommEntryMenu_FreeWindows(menu);
                CommEntryMenu_CloseYesNo(menu);
                menu->seq = COMM_ENTRY_ASK_TO_JOIN_SEQ_PRINT_ERROR;
            }
        }
        break;
    case COMM_ENTRY_ASK_TO_JOIN_SEQ_SYNC:
        if (menu->mode == COMM_ENTRY_MENU_MULTI_BATTLE) {
            CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_MembersHaveGatheredPlease);
        }
        menu->seq = COMM_ENTRY_ASK_TO_JOIN_SEQ_WAIT_SYNC;
        NetHandle_SendTimingPair(NetHandle_GetOwn(), COMM_ENTRY_SYNC_START, COMM_ENTRY_SYNC_CLASS);
        break;
    case COMM_ENTRY_ASK_TO_JOIN_SEQ_WAIT_SYNC:
        if (menu->mode == COMM_ENTRY_MENU_MULTI_BATTLE && func_ov036_021883e8(menu->msgWin) != TRUE) {
            break;
        }
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), COMM_ENTRY_SYNC_START, COMM_ENTRY_SYNC_CLASS) == TRUE) {
            menu->result = COMM_ENTRY_MENU_DONE;
            menu->seq = COMM_ENTRY_ASK_TO_JOIN_SEQ_END;
        } else if (menu->cancelled == TRUE) {
            menu->seq = COMM_ENTRY_ASK_TO_JOIN_SEQ_PRINT_ERROR;
        }
        break;
    case COMM_ENTRY_ASK_TO_JOIN_SEQ_PRINT_ERROR:
        CommEntryMenu_PrintMessage(menu, CommEntryMenu_GetMessages(menu)->error);
        menu->seq = COMM_ENTRY_ASK_TO_JOIN_SEQ_WAIT_ERROR;
        break;
    case COMM_ENTRY_ASK_TO_JOIN_SEQ_WAIT_ERROR:
        if (func_ov036_021883e8(menu->msgWin) == TRUE) {
            menu->result = COMM_ENTRY_MENU_CANCELLED;
            menu->seq = COMM_ENTRY_ASK_TO_JOIN_SEQ_END;
        }
        break;
    case COMM_ENTRY_ASK_TO_JOIN_SEQ_REFUSED:
        GFL_NetExit(NULL);
        copyVarForText(menu->wordSet, 0, &menu->replyInfo);
        CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_Refused);
        menu->seq = COMM_ENTRY_ASK_TO_JOIN_SEQ_WAIT_REFUSED;
        break;
    case COMM_ENTRY_ASK_TO_JOIN_SEQ_WAIT_REFUSED:
        if (GFL_NetIsEnded() == TRUE && func_ov036_021883e8(menu->msgWin) == TRUE) {
            menu->result = COMM_ENTRY_MENU_REFUSED;
            menu->seq = COMM_ENTRY_ASK_TO_JOIN_SEQ_END;
        }
        break;
    case COMM_ENTRY_ASK_TO_JOIN_SEQ_CANCEL:
        menu->result = COMM_ENTRY_MENU_CANCELLED;
        menu->seq = COMM_ENTRY_ASK_TO_JOIN_SEQ_END;
        break;
    case COMM_ENTRY_ASK_TO_JOIN_SEQ_END:
        CommEntryMenu_FreeWindows(menu);
        return TRUE;
    }
    if (GFL_NetErrCheck()) {
        CommEntryMenu_CloseYesNo(menu);
        menu->result = COMM_ENTRY_MENU_ERROR;
        return TRUE;
    }
    return FALSE;
}

// Sends the answers of the leader, and the member list when it changed
static void CommEntryMenu_SendAnswers(CommEntryMenu *menu) {
    u8 i;

    for (i = 0; i < COMM_ENTRY_MEMBER_MAX; i++) {
        u32 bit = 1 << i;

        if (menu->acceptMask & bit) {
            if (CommEntryMenu_SendAccept(i, menu->sendToAll, &menu->self) == TRUE) {
                menu->acceptMask ^= bit;
            }
        } else if (menu->refuseMask & bit) {
            if (CommEntryMenu_SendRefuse(i, menu->sendToAll, &menu->self) == TRUE) {
                menu->refuseMask ^= bit;
            }
        }
    }
    if (menu->sendMemberList == TRUE && menu->memberListSent == FALSE) {
        u32 netIds = CommEntryMenu_UpdateMemberList(menu);

        if (netIds > 1 && CommEntryMenu_SendMemberList(&menu->memberList, netIds, menu->sendToAll) == TRUE) {
            menu->sendMemberList = FALSE;
            menu->memberListSent = TRUE;
        }
    }
}

// Fills the member list in, and returns the machines to send it to, the leader's included
static u8 CommEntryMenu_UpdateMemberList(CommEntryMenu *menu) {
    u8 i;
    u8 mask = 0;

    for (i = 0; i < COMM_ENTRY_MEMBER_MAX; i++) {
        if (menu->members[i].state != COMM_ENTRY_MEMBER_NONE) {
            PlayerInfo_Copy(&menu->members[i].info, &menu->memberList.players[i]);
            mask |= 1 << i;
        }
    }
    menu->memberList.mask = mask;
    return mask | 1;
}

// Removes the members who disconnected
static void CommEntryMenu_CheckLeft(CommEntryMenu *menu) {
    u8 i;

    for (i = 0; i < COMM_ENTRY_MEMBER_MAX; i++) {
        if (menu->members[i].state != COMM_ENTRY_MEMBER_NONE && GFL_NetIsNegotiated(i) == FALSE) {
            if (menu->onLeave != NULL) {
                menu->onLeave(menu->callbackWork, &menu->members[i].info, menu->members[i].mac);
            }
            CommEntryMenu_RemoveMember(menu, i);
            CommEntryMenu_ClearAnswer(menu, i);
            CommEntryMenu_RequestMemberList(menu);
        }
    }
}

// Shows the members, and how many more can join or are needed
static void CommEntryMenu_DrawMembers(CommEntryMenu *menu) {
    int count = 0;
    int i;

    for (i = 0; i < COMM_ENTRY_MEMBER_MAX; i++) {
        if (menu->members[i].state != COMM_ENTRY_MEMBER_NONE) {
            CommEntryMenu_DrawMember(menu, &menu->members[i], i);
            count++;
        } else {
            CommEntryMenu_ClearMember(menu, &menu->members[i], i);
        }
    }
    if (count == 0) {
        CommEntryMenu_PrintMember(menu, &menu->self, 0);
        count++;
    }
    if (menu->shownCount != count && CommEntryMenu_PrintCount(menu, count) == TRUE) {
        menu->shownCount = count;
        if (menu->mode == COMM_ENTRY_MENU_MUSICAL && count == 1) {
            menu->showWaitMsg = TRUE;
        }
    }
    if (menu->showWaitMsg == TRUE && CommEntryMenu_IsIdle(menu) == TRUE && menu->yesNo.window == NULL) {
        if (count == 1) {
            CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_AwaitingParticipatingMembersB);
        }
        menu->showWaitMsg = FALSE;
    }
}

// Accepts the new members who need no asking, and queues the others to ask the leader about
static void CommEntryMenu_UpdateEntries(CommEntryMenu *menu) {
    CommEntryMember *member = menu->members;
    u8 i;

    for (i = 0; i < COMM_ENTRY_MEMBER_MAX; i++, member++) {
        switch (member->state) {
        case COMM_ENTRY_MEMBER_NEW:
            if (member->autoAccept == TRUE) {
                member->state = COMM_ENTRY_MEMBER_ACCEPTED;
                menu->acceptMask |= 1 << i;
            } else {
                member->state = COMM_ENTRY_MEMBER_WAITING;
            }
            break;
        case COMM_ENTRY_MEMBER_WAITING:
            if (CommEntryMenu_QueueAsk(menu, member) == TRUE) {
                member->state = COMM_ENTRY_MEMBER_ASKING;
            }
            break;
        }
    }
}

static void CommEntryMenu_RemoveMember(CommEntryMenu *menu, int netId) {
    menu->members[netId].state = COMM_ENTRY_MEMBER_NONE;
}

static BOOL CommEntryMenu_QueueAsk(CommEntryMenu *menu, CommEntryMember *member) {
    if (menu->asking == FALSE) {
        menu->asked = *member;
        menu->asking = TRUE;
        return TRUE;
    }
    return FALSE;
}

// Asks the leader whether to add the player who contacted it
static void CommEntryMenu_UpdateAsk(CommEntryMenu *menu) {
    CommEntryMember *asked = &menu->asked;
    CommEntryYesNo *yesNo = &menu->yesNo;
    u8 i;

    if (menu->asking != FALSE && func_ov036_021883e8(menu->msgWin) != FALSE) {
        switch (yesNo->seq) {
        case COMM_ENTRY_ASK_SEQ_PRINT:
            copyVarForText(menu->wordSet, 0, &asked->info);
            CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_HasContactedAddGroup);
            yesNo->seq++;
            return;
        case COMM_ENTRY_ASK_SEQ_OPEN:
            yesNo->window = func_ov036_021880d4(Field_GetMsgBGSys(menu->field), 0);
            yesNo->seq++;
            return;
        case COMM_ENTRY_ASK_SEQ_ANSWER: {
            u32 answer = func_ov036_0218816c(yesNo->window);

            if (answer != MSGBG_YES_NO_WAITING) {
                u8 state = COMM_ENTRY_MEMBER_ACCEPTED;

                if (answer != MSGBG_YES_NO_YES) {
                    state = COMM_ENTRY_MEMBER_REFUSED;
                }
                for (i = 0; i < COMM_ENTRY_MEMBER_MAX; i++) {
                    if (menu->members[i].state == COMM_ENTRY_MEMBER_ASKING) {
                        menu->members[i].state = state;
                        if (state == COMM_ENTRY_MEMBER_ACCEPTED) {
                            menu->acceptMask |= 1 << i;
                        } else {
                            menu->refuseMask |= 1 << i;
                        }
                        break;
                    }
                }
                func_ov036_02187ea0(yesNo->window);
                yesNo->window = NULL;
                yesNo->seq++;
                return;
            }
            break;
        }
        case COMM_ENTRY_ASK_SEQ_DONE:
            menu->asking = FALSE;
            yesNo->seq = COMM_ENTRY_ASK_SEQ_PRINT;
            CommEntryMenu_UpdateEntries(menu);
            if (menu->asking == FALSE) {
                if (CommEntryMenu_CountAccepted(menu) > 1 && menu->mode == COMM_ENTRY_MENU_MUSICAL) {
                    menu->nextMsg = CommEntryMenu_Text_WhenGroupReadyButton;
                    return;
                }
                menu->nextMsg = CommEntryMenu_Text_AwaitingParticipatingMembersB;
            }
            break;
        }
    }
}

// Asks the leader whether the group is OK
static u32 CommEntryMenu_AskGroupOk(CommEntryMenu *menu) {
    CommEntryYesNo *yesNo = &menu->yesNo;
    u32 answer;

    if (func_ov036_021883e8(menu->msgWin) == FALSE) {
        return COMM_ENTRY_ASK_PENDING;
    }
    switch (yesNo->seq) {
    case COMM_ENTRY_ASK_SEQ_PRINT:
        CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_GroupOk);
        yesNo->seq++;
        break;
    case COMM_ENTRY_ASK_SEQ_OPEN:
        yesNo->window = func_ov036_021880d4(Field_GetMsgBGSys(menu->field), 0);
        yesNo->seq++;
        break;
    case COMM_ENTRY_ASK_SEQ_ANSWER:
        // A member who joined or left while asking makes it a no
        if (CommEntryMenu_StatesUnchanged(menu) == FALSE) {
            answer = MSGBG_YES_NO_NO;
        } else {
            answer = func_ov036_0218816c(yesNo->window);
            if (answer == MSGBG_YES_NO_WAITING) {
                break;
            }
        }
        func_ov036_02187ea0(yesNo->window);
        yesNo->window = NULL;
        yesNo->seq = COMM_ENTRY_ASK_SEQ_PRINT;
        if (answer == MSGBG_YES_NO_YES) {
            return COMM_ENTRY_ASK_YES;
        }
        return COMM_ENTRY_ASK_NO;
    }
    return COMM_ENTRY_ASK_PENDING;
}

// Asks the leader whether to cancel
static u32 CommEntryMenu_AskCancel(CommEntryMenu *menu) {
    CommEntryYesNo *yesNo = &menu->yesNo;
    u32 answer;

    if (func_ov036_021883e8(menu->msgWin) == FALSE) {
        return COMM_ENTRY_ASK_PENDING;
    }
    switch (yesNo->seq) {
    case COMM_ENTRY_CANCEL_SEQ_PRINT:
        CommEntryMenu_PrintMessage(menu, CommEntryMenu_GetMessages(menu)->cancelAsk);
        yesNo->seq++;
        break;
    case COMM_ENTRY_CANCEL_SEQ_OPEN:
        yesNo->window = func_ov036_021880d4(Field_GetMsgBGSys(menu->field), 0);
        yesNo->seq++;
        break;
    case COMM_ENTRY_CANCEL_SEQ_ANSWER:
        answer = func_ov036_0218816c(yesNo->window);
        if (answer == MSGBG_YES_NO_WAITING) {
            break;
        }
        func_ov036_02187ea0(yesNo->window);
        yesNo->window = NULL;
        if (answer == MSGBG_YES_NO_NO) {
            yesNo->result = COMM_ENTRY_ASK_NO;
            yesNo->seq = COMM_ENTRY_CANCEL_SEQ_KEEP;
        } else if (CommEntryMenu_CountAccepted(menu) <= 1) {
            CommEntryMenu_PrintMessage(menu, CommEntryMenu_GetMessages(menu)->cancelled);
            yesNo->seq = COMM_ENTRY_CANCEL_SEQ_CANCEL;
        } else {
            // Members have joined: ask again, since they will be disbanded
            CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_IfLeaderDropsOut);
            yesNo->seq++;
        }
        break;
    case COMM_ENTRY_CANCEL_SEQ_OPEN_AGAIN:
        yesNo->window = func_ov036_021880d4(Field_GetMsgBGSys(menu->field), 0);
        yesNo->seq++;
        break;
    case COMM_ENTRY_CANCEL_SEQ_ANSWER_AGAIN:
        answer = func_ov036_0218816c(yesNo->window);
        if (answer == MSGBG_YES_NO_WAITING) {
            break;
        }
        func_ov036_02187ea0(yesNo->window);
        yesNo->window = NULL;
        if (answer == MSGBG_YES_NO_YES) {
            CommEntryMenu_PrintMessage(menu, CommEntryMenu_GetMessages(menu)->cancelled);
            yesNo->seq++;
        } else {
            yesNo->result = COMM_ENTRY_ASK_NO;
            yesNo->seq = COMM_ENTRY_CANCEL_SEQ_KEEP;
        }
        break;
    case COMM_ENTRY_CANCEL_SEQ_CANCEL:
        yesNo->seq = COMM_ENTRY_CANCEL_SEQ_END;
        yesNo->result = COMM_ENTRY_ASK_YES;
        break;
    case COMM_ENTRY_CANCEL_SEQ_KEEP:
        if (CommEntryMenu_CountAccepted(menu) > 1 && menu->mode == COMM_ENTRY_MENU_MUSICAL) {
            menu->nextMsg = CommEntryMenu_Text_WhenGroupReadyButton;
        } else {
            menu->nextMsg = CommEntryMenu_Text_AwaitingParticipatingMembersB;
        }
        yesNo->seq = COMM_ENTRY_CANCEL_SEQ_END;
        break;
    case COMM_ENTRY_CANCEL_SEQ_END:
        if (yesNo->window != NULL) {
            func_ov036_02187ea0(yesNo->window);
            yesNo->window = NULL;
        }
        yesNo->seq = COMM_ENTRY_CANCEL_SEQ_PRINT;
        return yesNo->result;
    }
    return COMM_ENTRY_ASK_PENDING;
}

static void CommEntryMenu_SetReady(CommEntryMenu *menu, u8 netId) {
    menu->readyMask |= 1 << netId;
}

static void CommEntryMenu_SetStarted(CommEntryMenu *menu) {
    menu->started = TRUE;
}

static void CommEntryMenu_SetCancelled(CommEntryMenu *menu) {
    menu->cancelled = TRUE;
}

static void CommEntryMenu_SetChecked(CommEntryMenu *menu) {
    menu->checked = TRUE;
    menu->sendCheckReply = TRUE;
    GFL_NetSetDisconnectError(TRUE);
}

// The number of members accepted
static int CommEntryMenu_CountAccepted(CommEntryMenu *menu) {
    int i;
    int count = 0;

    for (i = 0; i < COMM_ENTRY_MEMBER_MAX; i++) {
        if (menu->members[i].state == COMM_ENTRY_MEMBER_ACCEPTED) {
            count++;
        }
    }
    return count;
}

// The members accepted, as bits by network ID
static u32 CommEntryMenu_GetAcceptedMask(CommEntryMenu *menu) {
    u32 mask = 0;
    int i;

    for (i = 0; i < COMM_ENTRY_MEMBER_MAX; i++) {
        if (menu->members[i].state == COMM_ENTRY_MEMBER_ACCEPTED) {
            mask |= 1 << i;
        }
    }
    return mask;
}

// Whether the leader is asked about nothing and the message has been printed
static BOOL CommEntryMenu_IsIdle(CommEntryMenu *menu) {
    if (menu->asking == FALSE && func_ov036_021883e8(menu->msgWin) == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static void CommEntryMenu_CreateWindows(CommEntryMenu *menu) {
    FieldMsgBG *msgBGSys = Field_GetMsgBGSys(menu->field);
    s16 bottom = 16;

    if (menu->kind != COMM_ENTRY_MENU_LEADER) {
        bottom = 14;
    }
    menu->membersWin = FieldMsgBG_CreateMoneyWin(msgBGSys, menu->msgData, 2, 6, 18, bottom - 6);
    menu->titleWin = FieldMsgBG_CreateMoneyWin(msgBGSys, menu->msgData, 2, 1, 28, 2);
    func_ov036_02187c34(menu->titleWin, 0, 0, CommEntryMenu_GetMessages(menu)->title);
}

static void CommEntryMenu_FreeWindows(CommEntryMenu *menu) {
    FieldMsgBG *msgBGSys = Field_GetMsgBGSys(menu->field);

    if (menu->titleWin != NULL) {
        func_ov036_02187c1c(menu->titleWin);
        menu->titleWin = NULL;
    }
    if (menu->membersWin != NULL) {
        func_ov036_02187c1c(menu->membersWin);
        menu->membersWin = NULL;
    }
    func_ov036_02187980(msgBGSys);
}

// Prints how many more players can join or are needed
static BOOL CommEntryMenu_PrintCount(CommEntryMenu *menu, int count) {
    if (menu->membersWin != NULL && func_ov036_02187c70(menu->membersWin) == TRUE) {
        func_ov036_02187c88(menu->membersWin, 0, 64, 144, 80);
        WordSetNumber(menu->wordSet, 0, menu->maxCount - count, 1, 0, TRUE);
        GFL_WordSetFormatStrbuf(menu->wordSet, menu->countBuf, menu->countFmt);
        func_ov036_02187c4c(menu->membersWin, 0, 64, menu->countBuf);
        return TRUE;
    }
    return FALSE;
}

// Prints a member's row again when another member took it
static void CommEntryMenu_DrawMember(CommEntryMenu *menu, CommEntryMember *member, u32 netId) {
    if (GFL_STD_MemCmp(member->mac, CommEntryMenu_GetShownMac(menu, netId), sizeof(member->mac)) != 0) {
        CommEntryMenu_PrintMember(menu, &member->info, netId);
        MI_CpuCopy8(member->mac, CommEntryMenu_GetShownMac(menu, netId), sizeof(member->mac));
    }
}

// Clears a row whose member left
static void CommEntryMenu_ClearMember(CommEntryMenu *menu, CommEntryMember *member, u32 netId) {
    if (GFL_STD_MemCmp(CommEntryMenu_GetShownMac(menu, netId), sNoMac, sizeof(sNoMac)) != 0) {
        CommEntryMenu_ClearRow(menu, netId);
        MI_CpuCopy8(sNoMac, menu->shownMacs[netId], sizeof(sNoMac));
    }
}

static void CommEntryMenu_PrintMember(CommEntryMenu *menu, PlayerInfo *info, u8 row) {
    if (menu->membersWin != NULL) {
        copyVarForText(menu->wordSet, 0, info);
        WordSetNumber(menu->wordSet, 1, getTrainerID(info), 5, 2, TRUE);
        GFL_WordSetFormatStrbuf(menu->wordSet, menu->memberBuf, menu->memberFmt);
        func_ov036_02187c4c(menu->membersWin, 0, row << 4, menu->memberBuf);
    }
}

static void CommEntryMenu_ClearRow(CommEntryMenu *menu, int row) {
    if (menu->membersWin != NULL) {
        func_ov036_02187c88(menu->membersWin, 0, row << 4, 144, 16);
    }
}

// Shows the member list the leader sent
static void CommEntryMenu_DrawMemberList(CommEntryMenu *menu) {
    int i;

    if (menu->memberListChanged) {
        menu->memberListChanged = FALSE;
        for (i = 0; i < COMM_ENTRY_MEMBER_MAX; i++) {
            CommEntryMenu_ClearRow(menu, i);
            if (menu->memberList.mask & 1 << i) {
                CommEntryMenu_PrintMember(menu, &menu->memberList.players[i], i);
            }
        }
    }
}

static void CommEntryMenu_PrintMessage(CommEntryMenu *menu, int msgId) {
    if (msgId != CommEntryMenu_Text_AwaitingParticipatingMembersB ||
        menu->lastMsg != CommEntryMenu_Text_AwaitingParticipatingMembersB) {
        StrBuf *str;

        menu->lastMsg = msgId;
        str = GFL_MsgDataLoadStrbufNew(menu->msgData, msgId);
        GFL_WordSetFormatStrbuf(menu->wordSet, menu->msgBuf, str);
        GFL_StrBufFree(str);
        func_ov036_02188474(menu->msgWin);
        func_ov036_021883b0(menu->msgWin, 0, 0, menu->msgBuf);
    }
}

// Saves the members' states, to see whether any joined or left
static void CommEntryMenu_SaveStates(CommEntryMenu *menu) {
    u8 i;

    for (i = 0; i < COMM_ENTRY_MEMBER_MAX; i++) {
        menu->savedStates[i] = menu->members[i].state;
    }
}

static BOOL CommEntryMenu_StatesUnchanged(CommEntryMenu *menu) {
    u8 i;

    for (i = 0; i < COMM_ENTRY_MEMBER_MAX; i++) {
        if (menu->savedStates[i] != menu->members[i].state) {
            return FALSE;
        }
    }
    return TRUE;
}

static void CommEntryMenu_ClearAnswer(CommEntryMenu *menu, u8 netId) {
    menu->acceptMask &= 0xff ^ (1 << netId);
    menu->refuseMask &= 0xff ^ (1 << netId);
}

// Sends the member list again, once the one being sent arrives
static void CommEntryMenu_RequestMemberList(CommEntryMenu *menu) {
    if (menu->sendMemberList == TRUE || menu->memberListSent == TRUE) {
        menu->memberListPending = TRUE;
    } else {
        menu->sendMemberList = TRUE;
        menu->memberListPending = FALSE;
    }
}

static void CommEntryMenu_OnMemberListSent(CommEntryMenu *menu) {
    menu->memberListSent = FALSE;
    menu->sendMemberList = menu->memberListPending;
    menu->memberListPending = FALSE;
}

static CommEntryMemberList *CommEntryMenu_GetMemberListBuffer(CommEntryMenu *menu) {
    return &menu->memberList;
}

static void CommEntryMenu_SetMemberList(CommEntryMenu *menu, const CommEntryMemberList *list) {
    MI_CpuCopy8(list, &menu->memberList, sizeof(CommEntryMemberList));
    menu->memberListChanged = TRUE;
}

static void CommEntryMenu_CreateLeaderList(CommEntryMenu *menu) {
    FieldMsgBG *msgBGSys = Field_GetMsgBGSys(menu->field);

    menu->list.options = InitListMenuOptionHeap(COMM_ENTRY_LEADER_MAX, menu->heapId);
    CommEntryMenu_BuildLeaderOptions(menu);
    menu->list.menu = func_ov036_02187e60(msgBGSys, &sLeaderListRequest, menu->list.options);
    menu->list.seq = COMM_ENTRY_LIST_SEQ_PICK;
}

static void CommEntryMenu_FreeLeaderList(CommEntryMenu *menu) {
    if (menu->list.menu != NULL) {
        func_ov036_02187ea0(menu->list.menu);
        menu->list.menu = NULL;
        menu->list.optionsBuilt = FALSE;
    }
}

// Lets the player pick a leader and asks it
static u32 CommEntryMenu_UpdateLeaderList(CommEntryMenu *menu) {
    CommEntryYesNo *yesNo = &menu->yesNo;
    CommEntryLeaderList *list = &menu->list;

    switch (list->seq) {
    case COMM_ENTRY_LIST_SEQ_PICK:
        if (list->selected == NULL) {
            CommEntryMenu_AgeLeaders(menu);
            CommEntryMenu_ScanBeacons(menu);
            if (CommEntryMenu_UpdateLeaderListInput(menu) == FALSE) {
                list->result = COMM_ENTRY_LEADER_LIST_LEFT;
                list->seq = COMM_ENTRY_LIST_SEQ_END;
            } else if (menu->list.changed == TRUE) {
                CommEntryMenu_BuildLeaderOptions(menu);
                func_ov036_02187ef8(menu->list.menu);
                menu->list.changed = FALSE;
            }
        } else if (NetSystem_CountConnected() > 1) {
            if (NetHandle_SendNegotiation() == TRUE) {
                list->seq++;
            }
        } else {
            if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
                list->prevSeq = list->seq;
                list->seq = COMM_ENTRY_LIST_SEQ_ASK_LEAVE;
            }
            list->timer++;
            if (list->timer < COMM_ENTRY_ANSWER_TIMEOUT) {
                break;
            }
            list->seq = COMM_ENTRY_LIST_SEQ_WAIT_PROBLEM;
            CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_ThereAppearsProblem);
        }
        break;
    case COMM_ENTRY_LIST_SEQ_SEND_ENTRY:
        if (NetHandle_IsNegotiated(NetHandle_GetOwn()) == TRUE) {
            menu->reply = COMM_ENTRY_REPLY_NONE;
            if (CommEntryMenu_SendEntry(&menu->self, FALSE, menu->sendToAll) == TRUE) {
                list->seq = COMM_ENTRY_LIST_SEQ_WAIT_REPLY;
            }
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            list->prevSeq = list->seq;
            list->seq = COMM_ENTRY_LIST_SEQ_ASK_LEAVE;
        }
        if (menu->cancelled != TRUE) {
            break;
        }
        list->result = COMM_ENTRY_LEADER_LIST_JOINED;
        list->seq = COMM_ENTRY_LIST_SEQ_END;
        break;
    case COMM_ENTRY_LIST_SEQ_WAIT_REPLY:
        if (menu->reply == COMM_ENTRY_REPLY_ACCEPTED) {
            list->result = COMM_ENTRY_LEADER_LIST_JOINED;
            list->seq = COMM_ENTRY_LIST_SEQ_END;
        } else if (menu->reply == COMM_ENTRY_REPLY_REFUSED) {
            list->result = COMM_ENTRY_LEADER_LIST_REFUSED;
            list->seq = COMM_ENTRY_LIST_SEQ_END;
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            list->prevSeq = list->seq;
            list->seq = COMM_ENTRY_LIST_SEQ_ASK_LEAVE;
        }
        if (menu->cancelled != TRUE) {
            break;
        }
        list->result = COMM_ENTRY_LEADER_LIST_JOINED;
        list->seq = COMM_ENTRY_LIST_SEQ_END;
        break;
    case COMM_ENTRY_LIST_SEQ_END:
        return list->result;
    case COMM_ENTRY_LIST_SEQ_ASK_LEAVE:
        CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_WantLeaveGroup);
        yesNo->window = func_ov036_021880d4(Field_GetMsgBGSys(menu->field), 0);
        list->seq = COMM_ENTRY_LIST_SEQ_ANSWER_LEAVE;
        break;
    case COMM_ENTRY_LIST_SEQ_ANSWER_LEAVE: {
        u32 answer = func_ov036_0218816c(yesNo->window);

        // The leader checked on the members: stay
        if (func_ov036_0218798c(Field_GetMsgBGSys(menu->field)) == TRUE && menu->checked == TRUE) {
            answer = MSGBG_YES_NO_NO;
        }
        if (answer == MSGBG_YES_NO_WAITING && func_ov036_0218798c(Field_GetMsgBGSys(menu->field)) == TRUE) {
            if (menu->started != TRUE && menu->cancelled != TRUE) {
                break;
            }
            func_ov036_02187ea0(yesNo->window);
            yesNo->window = NULL;
            list->result = COMM_ENTRY_LEADER_LIST_JOINED;
            list->seq = COMM_ENTRY_LIST_SEQ_END;
            break;
        }
        func_ov036_02187ea0(yesNo->window);
        yesNo->window = NULL;
        if (answer == MSGBG_YES_NO_NO) {
            list->seq = list->prevSeq;
            CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_AwaitingSResponseB);
            if (menu->list.menu != NULL) {
                func_ov036_02188008(menu->list.menu);
            }
            break;
        }
        list->result = COMM_ENTRY_LEADER_LIST_LEFT;
        list->seq = COMM_ENTRY_LIST_SEQ_END;
        break;
    }
    case COMM_ENTRY_LIST_SEQ_WAIT_PROBLEM:
        if (func_ov036_021883e8(menu->msgWin) != TRUE) {
            break;
        }
        list->result = COMM_ENTRY_LEADER_LIST_LEFT;
        list->seq = COMM_ENTRY_LIST_SEQ_END;
        break;
    }
    return COMM_ENTRY_LEADER_LIST_RUNNING;
}

// Updates the list menu: FALSE when the player leaves it
static BOOL CommEntryMenu_UpdateLeaderListInput(CommEntryMenu *menu) {
    CommEntryLeaderList *list = &menu->list;
    s32 index = ListMenuUI_Update(list->menu);

    switch (index) {
    case BMPMENULIST_CANCEL:
        return FALSE;
    case BMPMENULIST_NULL:
        break;
    default:
        if (list->leaders[index].valid == TRUE) {
            list->selected = &list->leaders[index];
            GFL_NetStartAsChild(list->selected->mac);
            copyVarForText(menu->wordSet, 0, &list->selected->info);
            CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_AwaitingSResponseB);
            list->timer = 0;
        }
        break;
    }
    return TRUE;
}

// Removes the leaders not heard for a while
static void CommEntryMenu_AgeLeaders(CommEntryMenu *menu) {
    CommEntryLeaderList *list = &menu->list;
    int i;

    for (i = 0; i < COMM_ENTRY_LEADER_MAX; i++) {
        if (list->leaders[i].valid == TRUE && --list->leaders[i].timer <= 0) {
            CommEntryMenu_RemoveLeader(menu, i);
            list->changed = TRUE;
        }
    }
}

// Adds the leaders whose beacons are heard
static void CommEntryMenu_ScanBeacons(CommEntryMenu *menu) {
    int i;

    for (i = 0; i < COMM_ENTRY_LEADER_MAX; i++) {
        CommEntryBeacon *beacon = GFL_NetGetBeaconData(i);

        if (beacon != NULL && beacon->gameId == COMM_ENTRY_BEACON_GAME_ID) {
            CommEntryMenu_AddLeader(menu, beacon->mac, &beacon->info);
        }
    }
}

static void CommEntryMenu_AddLeader(CommEntryMenu *menu, const u8 *mac, const PlayerInfo *info) {
    CommEntryLeaderList *list = &menu->list;
    int i;

    for (i = 0; i < COMM_ENTRY_LEADER_MAX; i++) {
        if (list->leaders[i].valid == TRUE &&
            GFL_STD_MemCmp(list->leaders[i].mac, mac, sizeof(list->leaders[i].mac)) == 0) {
            list->leaders[i].timer = COMM_ENTRY_LEADER_TIMEOUT;
            return;
        }
    }
    for (i = 0; i < COMM_ENTRY_LEADER_MAX; i++) {
        if (list->leaders[i].valid == FALSE) {
            MI_CpuCopy8(mac, list->leaders[i].mac, sizeof(list->leaders[i].mac));
            PlayerInfo_Copy(info, &list->leaders[i].info);
            list->leaders[i].timer = COMM_ENTRY_LEADER_TIMEOUT;
            list->leaders[i].valid = TRUE;
            list->changed = TRUE;
            return;
        }
    }
}

static void CommEntryMenu_RemoveLeader(CommEntryMenu *menu, int index) {
    menu->list.leaders[index].valid = FALSE;
}

// Builds the list's options: a numbered row for each leader, with its name and ID, or empty
static void CommEntryMenu_BuildLeaderOptions(CommEntryMenu *menu) {
    u16 eom = GFL_StrBufGetTerminator();
    StrBuf *line;
    StrBuf *emptyFmt;
    StrBuf *leaderFmt;
    StrBuf *empty;
    int i;

    if (menu->list.optionsBuilt == TRUE) {
        func_ov036_02187f50(menu->list.options);
    }
    emptyFmt = GFL_StrBufCreate(64, menu->heapId);
    leaderFmt = GFL_StrBufCreate(64, menu->heapId);
    line = GFL_StrBufCreate(64, menu->heapId);
    empty = GFL_StrBufCreate(4, menu->heapId);
    GFL_MsgDataLoadStrbuf(menu->msgData, CommEntryMenu_Text_Empty_5, emptyFmt);
    GFL_MsgDataLoadStrbuf(menu->msgData, CommEntryMenu_Text_Id_2, leaderFmt);
    for (i = 0; i < COMM_ENTRY_LEADER_MAX; i++) {
        WordSetNumber(menu->wordSet, 0, i + 1, 2, 2, TRUE);
        if (menu->list.leaders[i].valid == TRUE) {
            copyVarForText(menu->wordSet, 1, &menu->list.leaders[i].info);
            WordSetNumber(menu->wordSet, 2, getTrainerID(&menu->list.leaders[i].info), 5, 2, TRUE);
            GFL_WordSetFormatStrbuf(menu->wordSet, line, leaderFmt);
        } else {
            GFL_StrBufLoadString(empty, &eom);
            GFL_WordSetSetStrbuf(menu->wordSet, 1, empty, 0, 1, 2);
            GFL_WordSetSetStrbuf(menu->wordSet, 2, empty, 0, 1, 2);
            GFL_WordSetFormatStrbuf(menu->wordSet, line, emptyFmt);
        }
        AppendListMenuOption(menu->list.options, line, i, menu->heapId);
    }
    GFL_StrBufFree(leaderFmt);
    GFL_StrBufFree(emptyFmt);
    GFL_StrBufFree(line);
    GFL_StrBufFree(empty);
    menu->list.optionsBuilt = TRUE;
}

// Asks whether to leave the group when the player presses B
static u32 CommEntryMenu_AskLeave(CommEntryMenu *menu) {
    CommEntryYesNo *yesNo = &menu->yesNo;

    switch (yesNo->seq) {
    case COMM_ENTRY_LEAVE_SEQ_WAIT_B:
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            yesNo->window = func_ov036_021880d4(Field_GetMsgBGSys(menu->field), 0);
            CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_WantLeaveGroup);
            yesNo->seq = COMM_ENTRY_LEAVE_SEQ_ANSWER;
        }
        break;
    case COMM_ENTRY_LEAVE_SEQ_ANSWER: {
        u32 answer = func_ov036_0218816c(yesNo->window);

        if (func_ov036_0218798c(Field_GetMsgBGSys(menu->field)) == TRUE && menu->checked == TRUE) {
            answer = MSGBG_YES_NO_NO;
        }
        if (answer != MSGBG_YES_NO_WAITING) {
            func_ov036_02187ea0(yesNo->window);
            yesNo->window = NULL;
            if (answer == MSGBG_YES_NO_NO) {
                yesNo->seq = COMM_ENTRY_LEAVE_SEQ_WAIT_B;
                CommEntryMenu_PrintMessage(menu, CommEntryMenu_Text_PleaseWaitBButton_2);
            } else {
                return COMM_ENTRY_LEAVE_YES;
            }
        }
        return COMM_ENTRY_LEAVE_ASKING;
    }
    }
    return COMM_ENTRY_LEAVE_NONE;
}

static void CommEntryMenu_CloseYesNo(CommEntryMenu *menu) {
    CommEntryYesNo *yesNo = &menu->yesNo;

    if (yesNo->window != NULL) {
        func_ov036_02187ea0(yesNo->window);
        yesNo->window = NULL;
    }
    yesNo->seq = COMM_ENTRY_ASK_SEQ_PRINT;
}

static void CommEntryMenu_SetReply(CommEntryMenu *menu, u32 reply, const PlayerInfo *info) {
    menu->reply = reply;
    PlayerInfo_Copy(info, &menu->replyInfo);
}

static int CommEntryMenu_GetMemberListSize(void) {
    return sizeof(CommEntryMemberList);
}

// Whether any of these members is still connected
static BOOL CommEntryMenu_AnyConnected(u32 mask) {
    u8 i;

    for (i = 0; i < COMM_ENTRY_MEMBER_MAX; i++) {
        if (i != GFL_NetGetHandleNetId(NetHandle_GetOwn()) && (1 << i & mask) && GFL_NetIsNegotiated(i) == TRUE) {
            return TRUE;
        }
    }
    return FALSE;
}

static void CommEntryMenu_AddCommands(CommEntryMenu *menu) {
    NetCommand_AddTable(COMM_ENTRY_CMD_ENTRY, sCommEntryNetCommands, NELEMS(sCommEntryNetCommands), menu);
}

static void CommEntryMenu_RemoveCommands(void) {
    NetCommand_RemoveTable(COMM_ENTRY_CMD_ENTRY);
}

static void *CommEntryMenu_GetMemberListRecvBuffer(int netId, void *work, int size) {
    return CommEntryMenu_GetMemberListBuffer(work);
}

static void CommEntryMenu_RecvEntry(int netId, int size, const void *data, void *work, NetHandle *handle) {
    const CommEntryPacket *packet = data;

    CommEntryMenu_AddEntry(work, netId, &packet->info, packet->autoAccept, packet->mac);
}

static BOOL CommEntryMenu_SendEntry(const PlayerInfo *info, u8 autoAccept, BOOL toAll) {
    CommEntryPacket packet;

    MI_CpuFill8(&packet, 0, sizeof(packet));
    PlayerInfo_Copy(info, &packet.info);
    OS_GetMacAddress(packet.mac);
    packet.autoAccept = autoAccept;
    if (toAll == TRUE) {
        return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), GFL_NET_NETID_SERVER, COMM_ENTRY_CMD_ENTRY,
                                           sizeof(packet), &packet, 0, FALSE, FALSE);
    }
    return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), 0, COMM_ENTRY_CMD_ENTRY, sizeof(packet), &packet, 0, FALSE,
                                       FALSE);
}

static void CommEntryMenu_RecvAccept(int netId, int size, const void *data, void *work, NetHandle *handle) {
    CommEntryMenu_SetReply(work, COMM_ENTRY_REPLY_ACCEPTED, data);
}

static BOOL CommEntryMenu_SendAccept(u8 netId, BOOL toAll, PlayerInfo *info) {
    if (toAll == TRUE) {
        return GFL_NetSendCommandToMachine(NetHandle_Get(GFL_NET_NETID_SERVER), netId, COMM_ENTRY_CMD_ACCEPT,
                                           PlayerInfo_GetSize(), info, 0, FALSE, FALSE);
    }
    return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), netId, COMM_ENTRY_CMD_ACCEPT, PlayerInfo_GetSize(), info, 0,
                                       FALSE, FALSE);
}

static void CommEntryMenu_RecvRefuse(int netId, int size, const void *data, void *work, NetHandle *handle) {
    CommEntryMenu_SetReply(work, COMM_ENTRY_REPLY_REFUSED, data);
}

static BOOL CommEntryMenu_SendRefuse(u8 netId, BOOL toAll, PlayerInfo *info) {
    if (toAll == TRUE) {
        return GFL_NetSendCommandToMachine(NetHandle_Get(GFL_NET_NETID_SERVER), netId, COMM_ENTRY_CMD_REFUSE,
                                           PlayerInfo_GetSize(), info, 0, FALSE, FALSE);
    }
    return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), netId, COMM_ENTRY_CMD_REFUSE, PlayerInfo_GetSize(), info, 0,
                                       FALSE, FALSE);
}

static void CommEntryMenu_RecvCheck(int netId, int size, const void *data, void *work, NetHandle *handle) {
    CommEntryMenu_SetChecked(work);
}

static BOOL CommEntryMenu_SendCheck(BOOL toAll) {
    if (toAll == TRUE) {
        return GFL_NetSendCommand(NetHandle_Get(GFL_NET_NETID_SERVER), COMM_ENTRY_CMD_CHECK, 0, NULL);
    }
    return GFL_NetSendCommand(NetHandle_GetOwn(), COMM_ENTRY_CMD_CHECK, 0, NULL);
}

static void CommEntryMenu_RecvCheckReply(int netId, int size, const void *data, void *work, NetHandle *handle) {
    CommEntryMenu_SetReady(work, netId);
}

static BOOL CommEntryMenu_SendCheckReply(BOOL toAll) {
    if (toAll == TRUE) {
        return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), GFL_NET_NETID_SERVER, COMM_ENTRY_CMD_CHECK_REPLY, 0,
                                           NULL, 0, FALSE, FALSE);
    }
    return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), 0, COMM_ENTRY_CMD_CHECK_REPLY, 0, NULL, 0, FALSE, FALSE);
}

static void CommEntryMenu_RecvStart(int netId, int size, const void *data, void *work, NetHandle *handle) {
    CommEntryMenu_SetStarted(work);
}

static BOOL CommEntryMenu_SendStart(BOOL toAll) {
    if (toAll == TRUE) {
        return GFL_NetSendCommand(NetHandle_Get(GFL_NET_NETID_SERVER), COMM_ENTRY_CMD_START, 0, NULL);
    }
    return GFL_NetSendCommand(NetHandle_GetOwn(), COMM_ENTRY_CMD_START, 0, NULL);
}

static void CommEntryMenu_RecvCancel(int netId, int size, const void *data, void *work, NetHandle *handle) {
    CommEntryMenu_SetCancelled(work);
}

static BOOL CommEntryMenu_SendCancel(BOOL toAll) {
    if (toAll == TRUE) {
        return GFL_NetSendCommand(NetHandle_Get(GFL_NET_NETID_SERVER), COMM_ENTRY_CMD_CANCEL, 0, NULL);
    }
    return GFL_NetSendCommand(NetHandle_GetOwn(), COMM_ENTRY_CMD_CANCEL, 0, NULL);
}

static void CommEntryMenu_RecvMemberList(int netId, int size, const void *data, void *work, NetHandle *handle) {
    // The leader's own copy tells it that the list was sent
    if (GFL_NetIsParent() == TRUE) {
        CommEntryMenu_OnMemberListSent(work);
    } else {
        CommEntryMenu_SetMemberList(work, data);
    }
}

static BOOL CommEntryMenu_SendMemberList(CommEntryMemberList *list, u32 netIds, BOOL toAll) {
    if (toAll == TRUE) {
        return GFL_NetSendCommandToBits(NetHandle_Get(GFL_NET_NETID_SERVER), netIds, COMM_ENTRY_CMD_MEMBER_LIST,
                                        CommEntryMenu_GetMemberListSize(), list, 0, FALSE, TRUE);
    }
    return GFL_NetSendCommandToBits(NetHandle_GetOwn(), netIds, COMM_ENTRY_CMD_MEMBER_LIST,
                                    CommEntryMenu_GetMemberListSize(), list, 0, FALSE, TRUE);
}
