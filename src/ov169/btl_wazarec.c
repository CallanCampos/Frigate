#include "battle/btl_wazarec.h"
#include "types.h"
#include "constants/moves.h"
#include "gfl/std.h"

void BtlWazaRec_Init(BtlWazaRec *rec) {
    u32 i;

    sys_memset(rec, 0, sizeof(BtlWazaRec));
    rec->head = 0;
    for (i = 0; i < BTL_WAZAREC_MAX; i++) {
        rec->records[i].move = MOVE_NONE;
    }
}

void BtlWazaRec_Add(BtlWazaRec *rec, u16 move, u32 turn, u8 monId) {
    rec->records[rec->head].move = move;
    rec->records[rec->head].turn = turn;
    rec->records[rec->head].monId = monId;
    rec->records[rec->head].succeeded = FALSE;
    rec->head++;
    if (rec->head >= BTL_WAZAREC_MAX) {
        rec->head = 0;
    }
}

void BtlWazaRec_MarkLastSucceeded(BtlWazaRec *rec) {
    int last = rec->head - 1;

    if (last < 0) {
        last = BTL_WAZAREC_MAX - 1;
    }
    rec->records[last].succeeded = TRUE;
}

BOOL BtlWazaRec_IsUsed(BtlWazaRec *rec, u16 move, u32 turn) {
    int i = rec->head;
    int end = rec->head;

    while (TRUE) {
        i--;
        if (i < 0) {
            i = BTL_WAZAREC_MAX - 1;
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

u32 GetUsedMoveCount(BtlWazaRec *rec, u16 move, u32 turn) {
    int i = rec->head;
    int end = rec->head;
    u32 count = 0;

    while (TRUE) {
        i--;
        if (i < 0) {
            i = BTL_WAZAREC_MAX - 1;
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

u16 BtlWazaRec_GetSucceededMove(BtlWazaRec *rec, u32 turn) {
    int i = rec->head;
    int end = rec->head;

    while (TRUE) {
        i--;
        if (i < 0) {
            i = BTL_WAZAREC_MAX - 1;
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
