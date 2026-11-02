#ifndef POKEBW2_BATTLE_BTLV_EFFVM_H
#define POKEBW2_BATTLE_BTLV_EFFVM_H

// Overlay 168's btlv_effvm.c (named by its string), the battle effect VM: it runs the move effect scripts, whose
// commands (swan's MOVE_SCRCMD) drive the camera, the Pokémon, the cell actors, the particles, the screen effects and
// the sounds of each move's animation. BtlvEffect_QueueCommands is swan's name; the others are ours

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "system/vm.h"

// What btlv_effect.c tells BtlvEffvm_Start of the effect, 0x10 bytes; BtlvEffvmParam_Clear clears it. The scripts
// read the fields as their variables 9 to 15
typedef struct {
    u8 target;    // 0x0  the move's MOVE_TARGET_*
    u8 variant;   // 0x1  which of the script's variants runs, the turn of a two-turn move
    u8 unk2;      // 0x2
    u8 shakes;    // 0x3  a thrown ball's, from SC_BALL_THROW
    u32 caught;   // 0x4  the same
    u32 critical; // 0x8  the same: a critical capture
    u16 itemNo;   // 0xc  the ball the ball effects show
} BtlvEffvmParam;

VM *BtlvEffvm_Create(TCBManager *tcbManager, HeapID heapId);
BOOL BtlvEffvm_Main(VM *vm);
void BtlvEffvm_Delete(VM *vm);
void BtlvEffvm_Start(VM *vm, u32 attacker, u32 defender, u16 move, BtlvEffvmParam *param);
void BtlvEffvm_Stop(VM *vm);
void BtlvEffvm_Resume(VM *vm);
s32 BtlvEffvm_GetScriptKind(VM *vm);
void BtlvEffvmParam_Clear(BtlvEffvmParam *param);
// PlaySE's arguments; the modulation's depth and speed are capped at 255
void BtlvEffvm_PlaySEAt(VM *vm, u32 se, u32 player, u32 pan, int pitch, int volume, int modDepth, int modSpeed,
                        int wait);
void BtlvEffvm_SlideSE(VM *vm, u32 player, u32 type, u32 param, int start, int end, int delay, int frames, int stepWait,
                       int count);
void BtlvEffvm_ReleaseVoices(VM *vm);
void BtlvEffvm_ClearLastEffect(VM *vm);
// A script command, global in the original: starts the task that opens or closes window 0 step by step
BOOL BtlvEffect_QueueCommands(VM *vm, void *env);

#endif // POKEBW2_BATTLE_BTLV_EFFVM_H
