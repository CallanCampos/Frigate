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
u32 func_ov028_021710a4(UnionSystem *sys);
u32 func_ov028_021710b0(UnionSystem *sys);
void func_ov028_021710bc(UnionSystem *sys, u8 value);
u8 func_ov028_021710c8(UnionSystem *sys);
// Whether the Union Room is up and idle
BOOL UnionMain_IsIdle(GameSystem *gsys);

// The net commands of union_comm.c's table: a command's handler, and for those with data in chunks, where it goes
void *UnionNet_GetRecvBuffer(int netId, void *work, int size);
void func_ov028_02171138(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_02171170(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_021711b4(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_021711f8(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_02171254(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_021712ac(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_02171308(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_021713e0(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_0217144c(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_02171478(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_021714bc(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_02171570(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_021715a4(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_021715d8(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_02171638(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_0217168c(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_021716ec(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_02171758(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_021717e8(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_02171848(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_021718e4(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_ov028_02171920(int netId, int size, const void *data, void *work, NetHandle *handle);

// The senders of those commands. The first goes to every machine, those with a mask go to the machines in it
BOOL func_ov028_0217115c(void);
BOOL func_ov028_02171194(u32 value);
BOOL func_ov028_021711d8(u32 value);
BOOL func_ov028_02171220(void);
BOOL func_ov028_0217128c(u32 value);
BOOL func_ov028_021712dc(UnionSystem *sys);
// The group's card, in two sizes
BOOL func_ov028_02171340(UnionSystem *sys);
BOOL func_ov028_02171390(UnionSystem *sys);
BOOL func_ov028_021714a4(void);
BOOL func_ov028_0217152c(UnionSystem *sys);
// The reply to an entry request: refused, and accepted
BOOL func_ov028_0217157c(u8 sendTo);
BOOL func_ov028_021715b0(u8 sendTo);
BOOL func_ov028_0217199c(UnionSystem *sys);

// Sends union_app.c's commands
BOOL func_ov028_02171664(u8 netId);
BOOL func_ov028_02171724(const UnionAppStatus *status, u8 sendTo);
BOOL func_ov028_0217181c(u8 sendTo, const UnionAppMember *member);
BOOL func_ov028_0217196c(u8 sendTo, u8 leftNetId);
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
extern const GameProcFunctions OEKAKI_PROC_FUNCTIONS;
extern const GameProcFunctions GURU2_PROC_FUNCTIONS;


// Overlay 36's, which puts a colosseum player's info into a table of the field's players
void func_ov036_021c3f34(void *players, u32 netId, const PlayerInfo *info, u8 flag, const u8 *mac);

#endif // POKEBW2_FIELD_UNION_MAIN_H
