#include "battle/btl_move_rec.h"
#include "types.h"
#include "constants/moves.h"
#include "gfl/std.h"

void BtlMoveRec_Init(BtlMoveRec *rec) {
    u32 i;

    MI_CpuFill8(rec, 0, sizeof(BtlMoveRec));
    rec->head = 0;
    for (i = 0; i < BTL_MOVE_REC_MAX; i++) {
        rec->records[i].move = MOVE_NONE;
    }
}

void BtlMoveRec_Add(BtlMoveRec *rec, u16 move, u32 turn, u8 monId) {
    rec->records[rec->head].move = move;
    rec->records[rec->head].turn = turn;
    rec->records[rec->head].monId = monId;
    rec->records[rec->head].succeeded = FALSE;
    rec->head++;
    if (rec->head >= BTL_MOVE_REC_MAX) {
        rec->head = 0;
    }
}

void BtlMoveRec_MarkLastSucceeded(BtlMoveRec *rec) {
    int last = rec->head - 1;

    if (last < 0) {
        last = BTL_MOVE_REC_MAX - 1;
    }
    rec->records[last].succeeded = TRUE;
}

BOOL BtlMoveRec_IsUsed(BtlMoveRec *rec, u16 move, u32 turn) {
    int i = rec->head;
    int end = rec->head;

    while (TRUE) {
        i--;
        if (i < 0) {
            i = BTL_MOVE_REC_MAX - 1;
        }
        if (i == end || rec->records[i].move == MOVE_NONE) {
            break;
        }
        if (rec->records[i].move == move && rec->records[i].turn == turn) {
            return TRUE;
        }
    }
    return FALSE;
}

u32 GetUsedMoveCount(BtlMoveRec *rec, u16 move, u32 turn) {
    int i = rec->head;
    int end = rec->head;
    u32 count = 0;

    while (TRUE) {
        i--;
        if (i < 0) {
            i = BTL_MOVE_REC_MAX - 1;
        }
        if (i == end || rec->records[i].move == MOVE_NONE) {
            break;
        }
        if (rec->records[i].move == move && rec->records[i].turn == turn) {
            count++;
        }
    }
    return count;
}

u16 BtlMoveRec_GetSucceededMove(BtlMoveRec *rec, u32 turn) {
    int i = rec->head;
    int end = rec->head;

    while (TRUE) {
        i--;
        if (i < 0) {
            i = BTL_MOVE_REC_MAX - 1;
        }
        if (i == end || rec->records[i].move == MOVE_NONE) {
            break;
        }
        if (rec->records[i].succeeded && rec->records[i].turn == turn) {
            return rec->records[i].move;
        }
    }
    return MOVE_NONE;
}
