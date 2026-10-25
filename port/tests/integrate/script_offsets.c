// The field script's relative offsets, which its commands read as signed 32-bit numbers and add to the script's
// pointer: the jumps and calls of src/ov012/scrcmd_vm.c (s001E_VMJump, s0004_VMCall, s001F_VMJumpIf, s0020_VMCallIf)
// and the movement that s0064_ActorCmdExec (src/ov036/scrcmd_fldmmdl.c) hands a field actor. Each offset counts from
// the end of the command's arguments, and a script reaches what comes before the command with a negative one. On the
// DS the 32-bit sum wraps; read as a u32 and added to a 64-bit pointer, such an offset moves the pointer 4 GiB forward
// instead, which stopped Black 2 at the starter's nickname until the commands read it signed
// (port/docs/game-changes.md, the row of these files). Each command runs here alone, as VM_Run (src/system/vm.c) calls
// it once it has read the command's ID, over the bytes the starter scene's commands read, and the scenario checks where
// the script's pointer goes and which action the actor gets.
//
// No ROM: s0064_ActorCmdExec's actor, its actor system, the GFL task manager and heap that the actor's action task
// takes (src/ov036/fldmmdl_acmd.c) are made here with the game's own layouts (include/field/field_actor_internal.h,
// include/field/field_script.h). port/tests/integrate/test_integrate.c runs it as its script_offsets scenario.
#include "types.h"
#include "constants/field_script.h"
#include "field/field_acmd.h"
#include "field/field_actor_internal.h"
#include "field/field_script.h"
#include "field/scrcmd_fldmmdl.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "nitro/os.h"
#include "system/vm.h"
#include <stdlib.h>
#include <string.h>

// The bytes the commands read stand for the aspertia_city script (res/field/scripts/aspertia_city.s, where each label
// is named after its offset; Black 2's build/b2_us/res/field/scripts/aspertia_city.bin has them so) from offset 0x500
// on, at the offsets the script has them, each offset argument where its command reads it:
// - Movement_0528, an animation of Bianca's in place: one action, 100, then the end. Action 100's commands
//   (ACTOR_ACTION_COMMAND_TABLES[100], src/ov036/fldmmdl_movedata.c, run by src/ov036/fldmmdl_acmd.c) set her draw
//   status to 10 and wait for its animation to end, without moving her. The scene plays it twice: after "You're
//   ex-act-ly like what I heard!", from before it, and after the answer to the nickname;
// - the VMCall L_06C6 at 0x640, which the YES to the nickname takes, 128 bytes forward;
// - the ActorCmdExec 2, Movement_0528 at 0x65a, after L_0658's MsgWinCloseAll, which both answers reach, 314 bytes
//   back;
// - the VMJump L_06D2 at 0x752 (L_0752), which the nickname's loop takes once the name is confirmed, 134 bytes back.
// The commands' arguments start two bytes after them, past their IDs, and the offsets count from the arguments' end
#define SCRIPT_BASE 0x500
#define SCRIPT_SIZE 0x300
#define MOVEMENT 0x528
#define NICKNAME_CALL 0x642
#define NICKNAME_ROUTINE 0x6c6
#define BIANCA_ANIMATION 0x65c
#define NICKNAME_JUMP 0x754
#define NICKNAME_LOOP 0x6d2
// Bianca's actor in the scene, and her animation's action
#define BIANCA 2
#define ANIMATION_ACTION 100

// An actor's flag that its slot is in use (src/ov012/fldmmdl.c, NextActor)
#define ACTOR_FLAG_USED 0x1
// The actor system's task priority, of which an action task takes two less (FieldAcmdTCB_Create)
#define ACTOR_TASK_PRIORITY 2
// How many tasks the actor system's task manager holds here: the one action task
#define ACTOR_TASKS 1
// The GFL heap that the action task's work comes from (FieldAcmdTCB_Create takes the actor system's field heap)
#define FIELD_HEAP 0
#define FIELD_HEAP_SIZE 0x1000
// GameData's layout is game_data.c's own (src/system/game_data.c), 936 bytes on a 64-bit host when this was written.
// The actor command reads its actor's ID through ScriptReadAny, which reads the game data's event work pointer and uses
// it only for an ID of 0x4000 or more (script_work.c, ScriptWork_GetWkAddr), so zeroed memory larger than the struct
// stands in for it, in pointer-sized words so that it is aligned as that pointer needs
#define GAME_DATA_STAND_IN_SIZE 0x1000

static union {
    u32 words[SCRIPT_SIZE / 4];
    u8 bytes[SCRIPT_SIZE];
} sScript;
static uptr sStack[8];
static VM sVm;
static uptr sGameDataStandIn[GAME_DATA_STAND_IN_SIZE / sizeof(uptr)];

// Where the script's offset at is in the buffer
static u8 *At(u32 offset) {
    return &sScript.bytes[offset - SCRIPT_BASE];
}

static void Write16(u32 offset, u16 value) {
    At(offset)[0] = value & 0xff;
    At(offset)[1] = value >> 8;
}

// A command's offset argument at offset, to reach target: from the end of the argument, little-endian, as the
// script's assembler writes it (include/asm/field_script.inc, `.4byte \target - (. + 4)`) and VM_Read32 reads it
static void WriteOffset(u32 offset, u32 target) {
    u32 value = target - (offset + 4);

    At(offset)[0] = value & 0xff;
    At(offset)[1] = (value >> 8) & 0xff;
    At(offset)[2] = (value >> 16) & 0xff;
    At(offset)[3] = value >> 24;
}

// The VM as VM_Run leaves it for a command: the script running, the pointer on the command's arguments, the stack
// and the comparison result empty
static VM *VmAt(u32 offset) {
    memset(&sVm, 0, sizeof(sVm));
    memset(sStack, 0, sizeof(sStack));
    sVm.param.stackSize = sizeof(sStack) / sizeof(sStack[0]);
    sVm.stack = sStack;
    sVm.state = VM_STATE_RUNNING;
    sVm.pc = At(offset);
    return &sVm;
}

static void TestJumpsAndCalls(void (*check)(int ok, const char *what)) {
    VM *vm;

    WriteOffset(NICKNAME_JUMP, NICKNAME_LOOP);
    vm = VmAt(NICKNAME_JUMP);
    s001E_VMJump(vm, NULL);
    check(vm->pc == At(NICKNAME_LOOP), "s001E_VMJump goes 134 bytes back, to the nickname's loop");

    WriteOffset(NICKNAME_CALL, NICKNAME_ROUTINE);
    vm = VmAt(NICKNAME_CALL);
    s0004_VMCall(vm, NULL);
    check(vm->pc == At(NICKNAME_ROUTINE), "s0004_VMCall goes 128 bytes forward, to the nickname's routine");
    VM_Return(vm);
    check(vm->pc == At(NICKNAME_CALL + 4), "and returns after its offset");

    // The nickname's jump written as a conditional one: the condition's byte, then the offset
    At(NICKNAME_JUMP)[0] = CMP_EQ;
    WriteOffset(NICKNAME_JUMP + 1, NICKNAME_LOOP);
    vm = VmAt(NICKNAME_JUMP);
    vm->cmpResult = CMP_EQ;
    s001F_VMJumpIf(vm, NULL);
    check(vm->pc == At(NICKNAME_LOOP), "s001F_VMJumpIf, its condition met, goes back to the nickname's loop");
    vm = VmAt(NICKNAME_JUMP);
    vm->cmpResult = CMP_LT;
    s001F_VMJumpIf(vm, NULL);
    check(vm->pc == At(NICKNAME_JUMP + 5), "and, not met, goes on after its offset");
    At(NICKNAME_JUMP)[0] = CMP_STACK;
    vm = VmAt(NICKNAME_JUMP);
    VM_StackPush(vm, 0);
    s001F_VMJumpIf(vm, NULL);
    check(vm->pc == At(NICKNAME_LOOP), "and, testing the stack, goes back when the value popped isn't 1");

    At(NICKNAME_JUMP)[0] = CMP_EQ;
    vm = VmAt(NICKNAME_JUMP);
    vm->cmpResult = CMP_EQ;
    s0020_VMCallIf(vm, NULL);
    check(vm->pc == At(NICKNAME_LOOP), "s0020_VMCallIf, its condition met, calls back to the nickname's loop");
    VM_Return(vm);
    check(vm->pc == At(NICKNAME_JUMP + 5), "and returns after its offset");
}

static void TestActorMovement(void (*check)(int ok, const char *what)) {
    static const HeapDef heaps[] = { { FIELD_HEAP_SIZE, 0 } };
    FieldActor actor;
    MMSys actors;
    ScriptSubwork subwork;
    FieldScriptEnv env;
    void *taskMemory;
    VM *vm;

    // The GFL heap, from the main arena as the game's start-up makes its heaps (src/system/gfl_use.c)
    OS_InitArena();
    GFL_MemInit(heaps, 1, 1, 0);
    taskMemory = malloc(GFL_TCBMgrCalcAllocSize(ACTOR_TASKS));
    memset(&actors, 0, sizeof(actors));
    actors.actorCapacity = 1;
    actors.actorHeap = &actor;
    actors.fieldHeapId = FIELD_HEAP;
    actors.unkC = ACTOR_TASK_PRIORITY;
    actors.tcbMgr = GFL_TCBMgrCreate(ACTOR_TASKS, taskMemory);
    memset(&actor, 0, sizeof(actor));
    actor.flags = ACTOR_FLAG_USED;
    actor.uid = BIANCA;
    actor.actorSystem = &actors;
    memset(&subwork, 0, sizeof(subwork));
    subwork.gameData = (GameData *)sGameDataStandIn;
    subwork.mmSys = &actors;
    memset(&env, 0, sizeof(env));
    env.subwork = &subwork;

    sScript.words[(MOVEMENT - SCRIPT_BASE) / 4] = ACMD(ANIMATION_ACTION, 1);
    sScript.words[(MOVEMENT - SCRIPT_BASE) / 4 + 1] = ACMD_END;
    Write16(BIANCA_ANIMATION, BIANCA);
    WriteOffset(BIANCA_ANIMATION + 2, MOVEMENT);
    vm = VmAt(BIANCA_ANIMATION);
    s0064_ActorCmdExec(vm, &env);
    check(vm->pc == At(BIANCA_ANIMATION + 6), "s0064_ActorCmdExec goes on after its actor and offset");
    check(subwork.acmdTasks[0] != NULL, "and gives the actor an action task");
    // The task's first pass commits the movement's first action to the actor (FieldAcmdTCB_CommitAcmd): the movement
    // the command found, 314 bytes back. Read 4 GiB forward, it would fault here
    GFL_TCBMgrUpdate(actors.tcbMgr);
    check(actor.nextAcmd == ANIMATION_ACTION, "whose first pass gives Bianca her animation's action, 314 bytes back");
    GFL_TCBMgrDelete(actors.tcbMgr);
    free(taskMemory);
}

// The scenario: check is the integrate program's, which reports a check that fails
void port_test_script_offsets(void (*check)(int ok, const char *what)) {
    TestJumpsAndCalls(check);
    TestActorMovement(check);
}
