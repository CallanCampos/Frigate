// Tests of the field script VM's jumps and calls as the host compiles the game's C (src/ov012/scrcmd_vm.c, over
// src/system/vm.c): the commands that move the script's position by an offset read from the script, VMJump, VMJumpIf,
// VMCall and VMCallIf, and VMReturn after a call. The game's field scripts (res/field/scripts) run on this VM: every
// talk, scene and menu of the field, such as the Pokemon Center's PC, whose menu loops back with a jump to an earlier
// command (res/field/scripts/global_10090.s, L_047B). The offset counts from the end of the command's own operands and
// is negative for a jump back. Read as a u32 and added to a 32-bit pointer it wraps on the DS; added to a 64-bit
// pointer it moved the position 4 GiB on, so the PC's menu crashed in VM_Run when the PC Box closed. The commands read
// it as an s32 now (port/docs/game-changes.md), and this test holds a jump back, a jump forward and both kinds of
// condition, and a call back and its return, to the positions the DS reaches.
//
//   port_game_script_vm_tests
#include "types.h"
#include "field/field_script.h"
#include "system/vm.h"
#include <stdio.h>
#include <string.h>

static int sFailures;

#define CHECK(condition, ...)                                                                                          \
    do {                                                                                                               \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__);                                                       \
            fprintf(stderr, __VA_ARGS__);                                                                              \
            fprintf(stderr, "\n");                                                                                     \
            sFailures++;                                                                                               \
        }                                                                                                              \
    } while (0)

// A script of 64 bytes, where the tests write each command's operands at offset 40: a jump back goes to the start
#define OPERANDS 40
#define SCRIPT_SIZE 64

// The comparison results VMCmp leaves and two of the conditions of VM_CMP_LUT (src/ov012/scrcmd_vm.c)
#define CMP_RESULT_LESS 0
#define CMP_RESULT_EQUAL 1
#define CONDITION_EQUAL 1
// VMJumpIf's condition that pops a value from the stack and jumps unless it is 1
#define CONDITION_STACK 0xff

static u8 sScript[SCRIPT_SIZE];
static uptr sStack[8];
static u32 sWork[4];

// A VM positioned at the operands at `at`, its stack empty, as VM_Run leaves it after reading a command's ID
static void Reset(VM *vm, const u8 *at) {
    memset(vm, 0, sizeof(*vm));
    vm->param.stackSize = (u16)(sizeof(sStack) / sizeof(sStack[0]));
    vm->param.workSize = (u16)(sizeof(sWork) / sizeof(sWork[0]));
    vm->stack = sStack;
    vm->work = sWork;
    vm->state = VM_STATE_RUNNING;
    vm->pc = at;
}

// Writes a little-endian 32-bit operand, as the script files hold them
static void Put32(u8 *at, s32 value) {
    u32 bits = (u32)value;

    at[0] = (u8)bits;
    at[1] = (u8)(bits >> 8);
    at[2] = (u8)(bits >> 16);
    at[3] = (u8)(bits >> 24);
}

static void TestJump(void) {
    VM vm;
    u8 *operands = sScript + OPERANDS;

    // Back to the script's start: the offset is from the end of the operand
    Put32(operands, -(OPERANDS + 4));
    Reset(&vm, operands);
    CHECK(s001E_VMJump(&vm, NULL) == FALSE, "VMJump goes on with the next command");
    CHECK(vm.pc == sScript, "a jump back lands at script+%td", vm.pc - sScript);

    // Forward, past the operand
    Put32(operands, 8);
    Reset(&vm, operands);
    s001E_VMJump(&vm, NULL);
    CHECK(vm.pc == operands + 4 + 8, "a jump forward lands at script+%td", vm.pc - sScript);
}

static void TestJumpIf(void) {
    VM vm;
    u8 *operands = sScript + OPERANDS;

    // A condition of the comparison table met: equal after an equal comparison
    operands[0] = CONDITION_EQUAL;
    Put32(operands + 1, -(OPERANDS + 5));
    Reset(&vm, operands);
    vm.cmpResult = CMP_RESULT_EQUAL;
    s001F_VMJumpIf(&vm, NULL);
    CHECK(vm.pc == sScript, "a conditional jump back taken lands at script+%td", vm.pc - sScript);

    // Not met: the script goes on after the operands
    Reset(&vm, operands);
    vm.cmpResult = CMP_RESULT_LESS;
    s001F_VMJumpIf(&vm, NULL);
    CHECK(vm.pc == operands + 5, "a conditional jump not taken goes on at script+%td", vm.pc - sScript);

    // The stack's condition: a value other than 1 popped jumps, as VMStackCmp's false does
    operands[0] = CONDITION_STACK;
    Reset(&vm, operands);
    VM_StackPush(&vm, 0);
    s001F_VMJumpIf(&vm, NULL);
    CHECK(vm.pc == sScript, "the stack's jump back taken lands at script+%td", vm.pc - sScript);
    CHECK(vm.stackPos == 0, "the stack's jump pops its value (%u left)", vm.stackPos);
}

static void TestCallAndReturn(void) {
    VM vm;
    u8 *operands = sScript + OPERANDS;

    // A call back pushes the position after its operand, and VMReturn comes back there
    Put32(operands, -(OPERANDS + 4));
    Reset(&vm, operands);
    s0004_VMCall(&vm, NULL);
    CHECK(vm.pc == sScript, "a call back lands at script+%td", vm.pc - sScript);
    CHECK(vm.stackPos == 1 && sStack[0] == (uptr)(operands + 4), "a call pushes its return position");
    s0005_VMReturn(&vm, NULL);
    CHECK(vm.pc == operands + 4, "VMReturn comes back to script+%td", vm.pc - sScript);

    // A conditional call back, its condition met
    operands[0] = CONDITION_EQUAL;
    Put32(operands + 1, -(OPERANDS + 5));
    Reset(&vm, operands);
    vm.cmpResult = CMP_RESULT_EQUAL;
    s0020_VMCallIf(&vm, NULL);
    CHECK(vm.pc == sScript, "a conditional call back lands at script+%td", vm.pc - sScript);
    CHECK(vm.stackPos == 1 && sStack[0] == (uptr)(operands + 5), "a conditional call pushes its return position");
}

int main(void) {
    TestJump();
    TestJumpIf();
    TestCallAndReturn();
    if (sFailures != 0) {
        fprintf(stderr, "%d failures\n", sFailures);
        return 1;
    }
    printf("the field script VM's jumps and calls: all passed\n");
    return 0;
}
