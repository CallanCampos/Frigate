#include "battle/btl_faintrec.h"
#include "types.h"

static void BtlFaintRecTurn_Init(BtlFaintRecTurn *turn);

static void BtlFaintRecTurn_Init(BtlFaintRecTurn *turn) {
    u32 i;

    turn->count = 0;
    for (i = 0; i < BTL_FAINTREC_MAX_MONS; i++) {
        turn->monIds[i] = BTL_FAINTREC_NO_MON;
        turn->handled[i] = FALSE;
    }
}

void BtlFaintRec_Init(BtlFaintRec *rec) {
    u32 i;

    for (i = 0; i < BTL_FAINTREC_TURNS; i++) {
        BtlFaintRecTurn_Init(&rec->turns[i]);
    }
}

void BtlFaintRec_NextTurn(BtlFaintRec *rec) {
    u32 i;

    for (i = BTL_FAINTREC_TURNS - 1; i > 0; i--) {
        rec->turns[i] = rec->turns[i - 1];
    }
    BtlFaintRecTurn_Init(&rec->turns[0]);
}

void BtlFaintRec_Add(BtlFaintRec *rec, u8 monId) {
    u8 count = rec->turns[0].count;

    if (count < BTL_FAINTREC_MAX_MONS) {
        rec->turns[0].monIds[count] = monId;
        rec->turns[0].handled[count] = FALSE;
        rec->turns[0].count++;
    }
}

u32 BtlFaintRec_GetCount(BtlFaintRec *rec, u32 turn) {
    if (turn < BTL_FAINTREC_TURNS) {
        return rec->turns[turn].count;
    }
    return 0;
}

u8 BtlFaintRec_GetMonId(BtlFaintRec *rec, u32 turn, u8 index) {
    if (turn < BTL_FAINTREC_TURNS && index < rec->turns[turn].count) {
        return rec->turns[turn].monIds[index];
    }
    return BTL_FAINTREC_NO_MON;
}

BOOL BtlFaintRec_IsHandled(BtlFaintRec *rec, u32 turn, u8 index) {
    if (turn < BTL_FAINTREC_TURNS && index < rec->turns[turn].count) {
        return rec->turns[turn].handled[index];
    }
    return FALSE;
}

void BtlFaintRec_SetHandled(BtlFaintRec *rec, u32 turn, u8 index) {
    if (turn < BTL_FAINTREC_TURNS && index < rec->turns[turn].count) {
        rec->turns[turn].handled[index] = TRUE;
    }
}

u8 BtlFaintRec_GetLastFainted(BtlFaintRec *rec) {
    u32 i;

    for (i = 0; i < BTL_FAINTREC_TURNS; i++) {
        if (rec->turns[i].count != 0) {
            return rec->turns[i].monIds[rec->turns[i].count - 1];
        }
    }
    return BTL_FAINTREC_NO_MON;
}
