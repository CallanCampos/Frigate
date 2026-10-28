#ifndef POKEBW2_FIELD_UNION_MAIN_H
#define POKEBW2_FIELD_UNION_MAIN_H

#include "types.h"
#include "app/p_status.h"
#include "app/pokelist.h"
#include "field/event_colosseum_battle.h"
#include "field/union_app.h"
#include "field/union_comm.h"
#include "gfl/net_handle.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// union_main.c, overlay 28, which the ROM names: the field side of the Union Room, which boots its communication, runs
// its commands as field events, and handles the net commands from 0x1400 that its machines send each other

// What overlay 34 allocates as the argument of the commands it starts (UnionSystem's cmdArg)
// The argument of the commands that run the activity's own proc: it starts with the app
typedef struct {
    UnionApp *app;
    u8 unk04[0x10];
    u32 unk14;
} UnionProcParam;

// The argument of the colosseum's party select: the party list, then the summary of a Pokémon in it, and the battle
typedef struct {
    PokeListParam list;
    PStatusParam status;
    ColosseumBattleParam *battle;
} UnionColosseumParam;

// Boots the Union Room's communication
void UnionMain_Boot(GameSystem *gsys);
// Runs each frame from the field while the Union Room's communication is up
void UnionMain_Update(GameCommSys *comm, Field *field);
// Whether the field is up for the Union Room's update
BOOL UnionMain_IsFieldReady(UnionSystem *sys);
// The GameCommSys field callbacks for GAME_COMM_NO_UNION
void UnionMain_OnFieldIn(void *param, void *work, Field *field);
void UnionMain_OnFieldOut(void *param, void *work, Field *field);

// Accessors of the system that the field and overlay 82 use
u32 UnionMain_GetAtCounter(UnionSystem *sys);
u32 UnionMain_GetExiting(UnionSystem *sys);
void UnionMain_SetSituUnk02(UnionSystem *sys, u8 value);
u8 UnionMain_GetSituUnk02(UnionSystem *sys);
// Whether the Union Room is up and idle
BOOL UnionMain_IsIdle(GameSystem *gsys);

// The net commands of union_comm.c's table: a command's handler, and for those with data in chunks, where it goes
void *UnionNet_GetRecvBuffer(int netId, void *work, int size);
void UnionNet_RecvGroupDone(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvUnk19(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvUnk1A(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvMac(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvUnk46_2(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvPlayerInfo(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvCard(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvColosseumMember(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvColosseumUnkDE(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvColosseumUnkCA(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvEntryRequest(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvEntryReply2(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvEntryReply1(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvEnqueue(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvUnkE(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvRequestStatus(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvStatus(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvAppProfile(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvProfile(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvConfirmProfile(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvLeave(int netId, int size, const void *data, void *work, NetHandle *handle);
void UnionNet_RecvJoinAvenue(int netId, int size, const void *data, void *work, NetHandle *handle);

// The senders of those commands. The first goes to every machine, those with a mask go to the machines in it
BOOL UnionMain_SendGroupDone(void);
BOOL UnionMain_SendUnk19(u32 value);
BOOL UnionMain_SendUnk1A(u32 value);
BOOL UnionMain_SendMac(void);
BOOL UnionMain_SendUnk46_2(u32 value);
BOOL UnionMain_SendPlayerInfo(UnionSystem *sys);
// The group's card, in two sizes
BOOL UnionMain_SendCard(UnionSystem *sys);
BOOL UnionMain_SendCardHead(UnionSystem *sys);
BOOL UnionMain_SendColosseumUnkCA(void);
BOOL UnionMain_SendEntryRequest(UnionSystem *sys);
// The reply to an entry request: refused, and accepted
BOOL UnionMain_SendEntryReply2(u8 sendTo);
BOOL UnionMain_SendEntryReply1(u8 sendTo);
BOOL UnionMain_SendJoinAvenue(UnionSystem *sys);

// Sends union_app.c's commands
BOOL UnionMain_SendUnkE(u8 netId);
BOOL UnionMain_SendStatus(const UnionAppStatus *status, u8 sendTo);
BOOL UnionMain_SendProfile(u8 sendTo, const UnionAppMember *member);
BOOL UnionMain_SendLeave(u8 sendTo, u8 leftNetId);
// The senders that union_subproc.c's commands use
BOOL UnionMain_SendEnqueue(void);
BOOL UnionMain_SendRequestStatus(void);
BOOL UnionMain_SendAppProfile(u8 sendTo, const PlayerInfo *info);
BOOL UnionMain_SendConfirmProfile(u8 sendTo, const PlayerInfo *info);

// The event that runs the command that UnionCommand_Set set, and whether one is set or running. The ids are
// the rows of the table in union_main.c
GameEvent *UnionCommand_CreateEvent(GameSystem *gsys, Field *field, UnionSystem *sys);
void UnionCommand_Set(UnionSystem *sys, u32 id, void *arg);
BOOL UnionCommand_IsActive(UnionSystem *sys);

// The procs of the activities that run in the Union Room, in overlays 70 and 216
extern const GameProcFunctions DRAWING_PROC_FUNCTIONS;
extern const GameProcFunctions SPIN_TRADE_PROC_FUNCTIONS;

#endif // POKEBW2_FIELD_UNION_MAIN_H
