// Trainer-AI table/list branches from actual src/ov170/tr_ai.c run through src/system/vm.c on owned synthetic
// scripts. Protect the post-operand PC base used by original DS AI scripts from compiler evaluation order; no ROM,
// replacement interpreter, AI timing model or fabricated game functions. Only move-effect data and unused heaps
// are fixture APIs. The real commands, argument reads, branches, callback returns and VM stopping are exercised.
#include "tr_ai_tables_source.inc"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char *sScript;
static s32 sEffect;
static VMCommand sCommands[120];
static void Check(int ok, const char *message) {
    if (!ok) {
        fprintf(stderr, "trainer AI tables: %s\n", message);
        exit(1);
    }
}
s32 PML_MoveGetParam(u16 move, u32 param) {
    Check(move == 33 && param == MOVE_PARAM_EFFECT, "actual move-effect API arguments");
    return sEffect;
}
void *GFL_HeapAllocate(HeapID heap, u32 size, BOOL clear, const char *file, u16 line) {
    (void)heap;
    (void)file;
    (void)line;
    return clear ? calloc(1, size) : malloc(size);
}
void GFL_HeapFree(void *memory) {
    free(memory);
}
void MI_CpuFill8(void *memory, u8 value, u32 size) {
    memset(memory, value, size);
}
static void Write16(unsigned offset, u16 value) {
    sScript[offset] = value;
    sScript[offset + 1] = value >> 8;
}
static void Write32(unsigned offset, u32 value) {
    Write16(offset, value);
    Write16(offset + 2, value >> 16);
}
static void Reset(VM *vm, TrAIContext *work, unsigned command, unsigned offset) {
    memset(sScript, 0xcc, 128);
    memset(vm, 0, sizeof(*vm));
    memset(work, 0, sizeof(*work));
    work->cmdReturn = TRUE;
    work->moveId = 33;
    vm->env = work;
    vm->param.commands = sCommands;
    vm->param.commandCount = 120;
    vm->state = VM_STATE_RUNNING;
    vm->pc = sScript + offset;
    Write16(offset, command);
}
static void EffectCase(unsigned start, s32 tableOffset, unsigned table, unsigned target, s32 effect) {
    VM vm;
    TrAIContext work;
    Reset(&vm, &work, 115, start);
    Write32(start + 2, 0);
    Write32(start + 6, 3);
    Write32(start + 10, tableOffset);
    Write32(table - 4, 0x7ffffff0);
    Write32(table + effect * 4, target - table);
    Write16(target, 77);
    sEffect = effect;
    Check(VM_Run(&vm), "table command yields through actual VM");
    Check(vm.pc == sScript + target, "effect table is relative to PC after all operands");
    Check(vm.state == VM_STATE_RUNNING && !(work.status & TRAI_STATUS_MOVE_DONE), "jump keeps script active");
    Check(VM_Run(&vm) && vm.state == VM_STATE_STOPPED && vm.pc == NULL && (work.status & TRAI_STATUS_MOVE_DONE),
          "actual AIEnd halts at selected destination");
}
static void EffectDefault(unsigned mode, unsigned maximum, s32 effect) {
    VM vm;
    TrAIContext work;
    Reset(&vm, &work, 115, 0);
    Write32(2, mode);
    Write32(6, maximum);
    Write32(10, 46);
    sEffect = effect;
    Check(VM_Run(&vm) && vm.pc == NULL && vm.state == VM_STATE_STOPPED && (work.status & TRAI_STATUS_MOVE_DONE),
          "unsupported mode or out-of-range effect takes actual AIEnd");
}
static void ListCase(unsigned command, s32 result, int jump, unsigned start, unsigned table, unsigned target) {
    VM vm;
    TrAIContext work;
    Reset(&vm, &work, command, start);
    Write32(start + 2, table - (start + 6));
    Write32(start + 6, target - (start + 10));
    Write32(table - 4, 0xffffffff);
    Write32(table, 42);
    Write32(table + 4, 0xffffffff);
    work.result = result;
    Check(VM_Run(&vm), "list command yields through actual VM");
    Check(vm.pc == sScript + (jump ? target : start + 10), "list uses post-read PC, sentinel and branch polarity");
}
int main(int argc, char **argv) {
    const char *group = argc == 1 ? "all" : argv[1];
    if (argc > 2 ||
        (strcmp(group, "all") && strcmp(group, "effect") && strcmp(group, "list_in") && strcmp(group, "list_not"))) {
        return 2;
    }
    sScript = malloc(128);
    Check(sScript != NULL, "owned script allocation");
#if UINTPTR_MAX > UINT32_MAX
    Check((uintptr_t)sScript > UINT32_MAX, "script exercises a full native pointer");
#endif
    sCommands[27] = AIIfResultInList;
    sCommands[28] = AIIfResultNotInList;
    sCommands[77] = AIEnd;
    sCommands[115] = AIJumpByMoveEffect;
    if (!strcmp(group, "all") || !strcmp(group, "effect")) {
        EffectCase(0, 46, 60, 100, 2);
        EffectCase(40, -34, 20, 80, 0);
        EffectCase(0, 46, 60, 24, 0);
        EffectDefault(1, 3, 0);
        EffectDefault(0, 1, 2);
    }
    if (!strcmp(group, "all") || !strcmp(group, "list_in")) {
        ListCase(27, 42, TRUE, 0, 60, 80);
        ListCase(27, 7, FALSE, 0, 60, 80);
        ListCase(27, 42, TRUE, 40, 20, 80);
        ListCase(27, 42, TRUE, 40, 20, 0);
    }
    if (!strcmp(group, "all") || !strcmp(group, "list_not")) {
        ListCase(28, 42, FALSE, 0, 60, 80);
        ListCase(28, 7, TRUE, 0, 60, 80);
        ListCase(28, 7, TRUE, 40, 20, 80);
        ListCase(28, 7, TRUE, 40, 20, 0);
    }
    free(sScript);
    printf("trainer AI table/list sequencing: %s passed\n", group);
    return 0;
}
