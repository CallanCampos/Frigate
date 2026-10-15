#include "types.h"
#include "field/wbt_makematch.h"
#include "field/wbt.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/math.h"
#include "pml/poke_party.h"

static void func_ov055_021e5d88(WbtEntrant *entrant, HeapID heapId);
static void func_ov055_021e5dfc(WbtEntrant *entrant);
static void func_ov055_021e614c(WbtMatch *match, MATHRandContext32 *rand, WbtEntrant *first, WbtEntrant *second);
static u8 func_ov055_021e6338(u32 attackType, u32 defenseType);

WbtSystem *WbtSystem_Create(HeapID heapId, BOOL a1) {
    int i;
    WbtSystem *sys = GFL_HeapAllocate(heapId, sizeof(WbtSystem), TRUE, "wbt_system.c", 51);

    sys->heapId = heapId;
    sys->unk4 = a1;
    sys->tournament = 0;
    sys->style = 4;
    sys->round = 0;
    sys->seed = ((u64)GFL_RandomMT() << 32) + GFL_RandomMT();
    MATH_InitRand32(&sys->rand, sys->seed);
    sys->unk1E = GFL_RandomLC(4);
    for (i = 0; i < 8; i++) {
        func_ov055_021e5d88(&sys->entrants[i], heapId);
    }
    sys->partyBC = PokeParty_Create(sys->heapId);
    sys->partyC0 = PokeParty_Create(sys->heapId);
    sys->partyC4 = PokeParty_Create(sys->heapId);
    sys->unk13AC = NULL;
    sys->trainers = func_ov055_021e63e0(sys->heapId);
    return sys;
}

void WbtSystem_Free(WbtSystem *sys) {
    int i;

    func_ov055_021e6438(sys->trainers);
    GFL_HeapFree(sys->partyC4);
    GFL_HeapFree(sys->partyBC);
    GFL_HeapFree(sys->partyC0);
    if (sys->unk13AC != NULL) {
        GFL_HeapFree(sys->unk13AC);
    }
    for (i = 0; i < 8; i++) {
        func_ov055_021e5dfc(&sys->entrants[i]);
    }
    GFL_HeapFree(sys);
}

void func_ov055_021e5ca0(WbtSystem *sys, u32 tournament) {
    sys->tournament = tournament;
}

u32 func_ov055_021e5ca4(WbtSystem *sys) {
    return sys->tournament;
}

void func_ov055_021e5ca8(WbtSystem *sys, u32 style) {
    sys->style = style;
}

u32 func_ov055_021e5cac(WbtSystem *sys) {
    return sys->style;
}

void func_ov055_021e5cb0(WbtSystem *sys, u8 type) {
    sys->type = type;
}

u8 func_ov055_021e5cb4(WbtSystem *sys) {
    return sys->type;
}

void func_ov055_021e5cb8(WbtSystem *sys, u32 value) {
    sys->unk14 = value;
}

u32 func_ov055_021e5cbc(WbtSystem *sys) {
    return sys->unk14;
}

void func_ov055_021e5cc0(WbtSystem *sys, u32 round) {
    sys->round = round;
}

u32 func_ov055_021e5cc4(WbtSystem *sys) {
    return sys->round;
}

u64 func_ov055_021e5cc8(WbtSystem *sys) {
    return sys->seed;
}

void func_ov055_021e5cd0(WbtSystem *sys, u8 value) {
    sys->unk1D = value;
}

u8 func_ov055_021e5cd4(WbtSystem *sys) {
    return sys->unk1D;
}

void func_ov055_021e5cd8(WbtSystem *sys, u16 *var) {
    sys->unk13E0 = var;
}

u16 *func_ov055_021e5ce4(WbtSystem *sys) {
    return sys->unk13E0;
}

void func_ov055_021e5cf0(WbtSystem *sys, u8 value) {
    sys->unk13A8 = value;
}

void func_ov055_021e5cfc(WbtSystem *sys, void *value) {
    sys->unk13CC = value;
}

void *func_ov055_021e5d08(WbtSystem *sys) {
    return sys->unk13CC;
}

void func_ov055_021e5d14(WbtSystem *sys, u32 value) {
    sys->unk13E4 = value;
}

WbtEntrant *func_ov055_021e5d20(WbtSystem *sys, u32 index) {
    return &sys->entrants[index];
}

// The player's entrant
WbtEntrant *func_ov055_021e5d28(WbtSystem *sys) {
    int i;

    for (i = 0; i < 8; i++) {
        if (sys->entrants[i].unk0_0 == 3) {
            return &sys->entrants[i];
        }
    }
    return sys->entrants;
}

void func_ov055_021e5d44(WbtSystem *sys, u32 tournament, StrBuf *strbuf) {
    if (tournament == 3) {
        GFL_StrBufLoadFixedString(strbuf, sys->download.name, NELEMS(sys->download.name));
    } else {
        LoadPWTTournamentTypeText(sys->heapId, tournament, strbuf);
    }
}

PokeParty *func_ov055_021e5d68(WbtSystem *sys) {
    return sys->partyBC;
}

PokeParty *func_ov055_021e5d74(WbtSystem *sys) {
    return sys->partyC0;
}

u32 func_ov055_021e5d7c(WbtSystem *sys) {
    return func_ov055_021e64c0(sys->style);
}

static void func_ov055_021e5d88(WbtEntrant *entrant, HeapID heapId) {
    entrant->unk0_0 = 0;
    entrant->unk0_3 = 0;
    entrant->unk0_4 = 1;
    entrant->unk0_7 = 0;
    entrant->type = 0;
    entrant->unk2 = 0;
    entrant->unk3_0 = 0;
    entrant->unk3_6 = 0;
    entrant->objCode = 0xe7;
    entrant->unk6_0 = 0;
    entrant->unk6_12 = 1;
    entrant->name = GFL_StrBufCreate(16, heapId);
    entrant->unkC = GFL_StrBufCreate(0x16, heapId);
}

static void func_ov055_021e5dfc(WbtEntrant *entrant) {
    GFL_StrBufFree(entrant->name);
    GFL_StrBufFree(entrant->unkC);
}

u32 func_ov055_021e5e10(WbtEntrant *entrant) {
    return entrant->unk0_4;
}

u8 func_ov055_021e5e18(WbtEntrant *entrant) {
    return entrant->unk3_0;
}

u32 func_ov055_021e5e20(WbtEntrant *entrant) {
    return entrant->unk3_6;
}

u8 func_ov055_021e5e28(WbtEntrant *entrant) {
    return entrant->unk2;
}

u16 func_ov055_021e5e2c(WbtEntrant *entrant) {
    return entrant->unk6_0;
}

u32 func_ov055_021e5e34(WbtEntrant *entrant) {
    return entrant->unk0_0;
}

void func_ov055_021e5e3c(const WbtEntrant *entrant, StrBuf *dest) {
    GFL_StrBufCopy(dest, entrant->name);
}

u32 func_ov055_021e5e4c(WbtEntrant *entrant) {
    return entrant->unk0_7;
}

// Copies the Pokémon picked for the tournament into its parties
void func_ov055_021e5e54(WbtSystem *sys, GameData *gameData) {
    PokeParty *party = PokeParty_Create(HEAPID_TAIL(sys->heapId));
    PokeParty *src;
    PartyPkm *copy;
    HeapID heapId;
    u32 count;
    u32 n;
    u32 i;
    u8 slot;

    PokeParty_Init(party);
    src = WbtMakeMatch_GetParty(sys, gameData);
    heapId = HEAPID_TAIL(sys->heapId);
    copy = GFL_HeapAllocate(heapId, PokeParty_GetPkmRawSize(), FALSE, "wbt_system.c", 468);
    n = 0;
    PokeParty_ClearPkm(copy);
    count = func_ov055_021e5d7c(sys);
    if (func_ov055_021e6794(sys->tournament) == 4) {
        for (i = 0; i < NELEMS(sys->castSlots); i++) {
            if (sys->castSlots[i] != 0) {
                n++;
            }
        }
        count = n;
    }
    for (i = 0; i < count; i++) {
        slot = sys->castSlots[i];
        if (slot == 0) {
            slot = 1;
        }
        copyPartyPkm(PokeParty_GetPkm(src, slot - 1), copy);
        PokeParty_AddPkm(party, copy);
    }
    PokeParty_Copy(party, sys->partyBC);
    PokeParty_Copy(party, sys->partyC4);
    GFL_HeapFree(party);
    GFL_HeapFree(copy);
}

static const u8 sRegulations[] = { 29, 30, 31, 32 };
static const u8 sDoubleRegulations[] = { 33, 34, 35, 36 };

// The tournament's regulation, in arc 106
u8 func_ov055_021e5f38(WbtSystem *sys) {
    const u8 *regulations;

    if (sys->tournament == 11) {
        return 37;
    }
    switch (func_ov055_021e6794(sys->tournament)) {
    case 0:
    case 3:
    case 4:
    default:
        regulations = sRegulations;
        break;
    case 1:
    case 2:
        regulations = sDoubleRegulations;
        break;
    }
    return regulations[sys->style];
}

Regulation *func_ov055_021e5f78(WbtSystem *sys) {
    if (sys->tournament == 3) {
        WbtDownloadedTournament *downloaded = &sys->download.tournament;
        int i;

        sys_memset(&sys->regulation, 0, sizeof(sys->regulation));
        sys->regulation.unk2 = downloaded->unk0;
        sys->regulation.unk3 = downloaded->unk1;
        sys->regulation.unk4 = downloaded->unk2;
        sys->regulation.unk5 = downloaded->unk3;
        sys->regulation.unk6 = downloaded->unk4;
        sys->regulation.unk8 = downloaded->unk6;
        sys->regulation.unk9 = downloaded->unk7;
        for (i = 0; i < 0x52; i++) {
            sys->regulation.unkA[i] = downloaded->unk8[i];
        }
        for (i = 0; i < 0x4c; i++) {
            sys->regulation.unk5C[i] = downloaded->unk5A[i];
        }
        sys->regulation.unkBA = downloaded->unkA7;
    } else {
        PokeRegulation_Load(func_ov055_021e5f38(sys), &sys->regulation);
        if (sys->tournament == 11 || sys->tournament == 4) {
            sys->regulation.unk4 = 25;
        }
    }
    return &sys->regulation;
}

WbtUnk18A *func_ov055_021e6024(WbtSystem *sys) {
    u8 type = 17;
    u16 value = 0;

    switch (func_ov055_021e6794(sys->tournament)) {
    case 0:
        break;
    case 1:
        value = 507;
        break;
    case 2:
        type = sys->type;
        value = 507;
        break;
    case 3:
        break;
    case 4:
        type = sys->download.tournament.unkA9;
        value = sys->download.tournament.unkAA;
        break;
    }
    sys->unk18A.type = type;
    sys->unk18A.unk1 = 0;
    sys->unk18A.unk2 = value;
    return &sys->unk18A;
}

// The score of a match, by how it was won and how the Trainers' types matched up
static const u8 sScores[5][3] = {
    { 3, 3, 4 }, { 0, 0, 0 }, { 1, 1, 1 }, { 2, 2, 4 }, { 3, 3, 4 },
};

// The opponent of each seat in the first round, and which match of the one before each seat's next opponent comes from
static const u32 sFirstOpponents[] = { 1, 0, 3, 2, 5, 4, 7, 6 };
static const u32 sSecondMatches[] = { 1, 1, 0, 0, 3, 3, 2, 2 };
static const u32 sThirdMatches[] = { 1, 1, 1, 1, 0, 0, 0, 0 };

// The seat of the player's opponent in the current round
u32 func_ov055_021e607c(WbtSystem *sys) {
    u8 firstWinners[4];
    u8 secondWinners[2];
    WbtMatch *matches;
    u32 player;
    u32 opponent;
    int i;
    int seat;

    matches = sys->matches;
    player = 0;
    opponent = 0;
    for (i = 0; i < 4; i++) {
        seat = i * 2 + (matches[i].firstWon == TRUE ? 0 : 1);
        firstWinners[i] = seat;
    }
    for (i = 0; i < 2; i++) {
        seat = i * 2 + (matches[i + 4].firstWon == TRUE ? 0 : 1);
        secondWinners[i] = firstWinners[seat];
    }
    for (i = 0; i < 8; i++) {
        if (sys->entrants[i].unk0_0 == 3) {
            player = i;
            break;
        }
    }
    switch (func_ov055_021e5cc4(sys)) {
    case 1:
    case 2:
        opponent = sFirstOpponents[player];
        break;
    case 3:
        opponent = firstWinners[sSecondMatches[player]];
        break;
    case 4:
        opponent = secondWinners[sThirdMatches[player]];
        break;
    case 5:
        break;
    }
    return opponent;
}

static void func_ov055_021e614c(WbtMatch *match, MATHRandContext32 *rand, WbtEntrant *first, WbtEntrant *second) {
    int firstRank = first->unk0_4;
    int secondRank = second->unk0_4;
    u8 firstEffect = func_ov055_021e6338(first->type, second->type);
    u8 secondEffect = func_ov055_021e6338(second->type, first->type);
    int kind;
    int sign;

    if (firstRank == secondRank) {
        if (firstEffect == secondEffect) {
            match->firstWon = MATH_Rand32(rand, 2) == 0;
        } else {
            match->firstWon = firstEffect > secondEffect;
            if (MATH_Rand32(rand, 10) >= 7) {
                match->firstWon = match->firstWon == FALSE;
            }
        }
    } else {
        match->firstWon = firstRank > secondRank;
    }
    if (first->unk0_0 == 3) {
        match->firstWon = TRUE;
    } else if (second->unk0_0 == 3) {
        match->firstWon = FALSE;
    }
    if (firstRank >= 4 && secondRank >= 4) {
        kind = 0;
    } else if ((firstRank - secondRank >= 2 && match->firstWon == TRUE) ||
               (secondRank - firstRank >= 2 && match->firstWon == FALSE)) {
        kind = 1;
    } else if ((firstRank - secondRank == 1 && match->firstWon == TRUE) ||
               (secondRank - firstRank == 1 && match->firstWon == FALSE)) {
        kind = 2;
    } else {
        kind = firstRank == secondRank ? 3 : 4;
    }
    if (firstEffect > secondEffect) {
        sign = -1;
    } else if (firstEffect < secondEffect) {
        sign = 1;
    } else {
        sign = 0;
    }
    if (match->firstWon == FALSE) {
        sign *= -1;
    }
    match->score = sScores[kind][sign + 1];
}

// Plays the first two rounds of the bracket
void func_ov055_021e62b0(WbtSystem *sys, MATHRandContext32 *rand) {
    WbtEntrant *entrants[2];
    WbtMatch *match;
    int i;
    int j;

    for (i = 0; i < 4; i++) {
        func_ov055_021e614c(&sys->matches[i], rand, &sys->entrants[i * 2], &sys->entrants[i * 2 + 1]);
    }
    for (i = 0; i < 2; i++) {
        match = &sys->matches[i + 4];
        for (j = 0; j < 2; j++) {
            if (sys->matches[i * 2 + j].firstWon) {
                entrants[j] = &sys->entrants[i * 4 + j * 2];
            } else {
                entrants[j] = &sys->entrants[i * 4 + j * 2 + 1];
            }
        }
        func_ov055_021e614c(match, rand, entrants[0], entrants[1]);
    }
}

// The type chart, in damage multiples of 4
static const u8 sTypeChart[17][17] = {
    { 4, 4, 4, 4, 4, 2, 4, 0, 2, 4, 4, 4, 4, 4, 4, 4, 4 },
    { 8, 4, 2, 2, 4, 8, 2, 0, 8, 4, 4, 4, 4, 2, 8, 4, 8 },
    { 4, 8, 4, 4, 4, 2, 8, 4, 2, 4, 4, 8, 2, 4, 4, 4, 4 },
    { 4, 4, 4, 2, 2, 2, 4, 2, 0, 4, 4, 8, 4, 4, 4, 4, 4 },
    { 4, 4, 0, 8, 4, 8, 2, 4, 8, 8, 4, 2, 8, 4, 4, 4, 4 },
    { 4, 2, 8, 4, 2, 4, 8, 4, 2, 8, 4, 4, 4, 4, 8, 4, 4 },
    { 4, 2, 2, 2, 4, 4, 4, 2, 2, 2, 4, 8, 4, 8, 4, 4, 8 },
    { 0, 4, 4, 4, 4, 4, 4, 8, 2, 4, 4, 4, 4, 8, 4, 4, 2 },
    { 4, 4, 4, 4, 4, 8, 4, 4, 2, 2, 2, 4, 2, 4, 8, 4, 4 },
    { 4, 4, 4, 4, 4, 2, 8, 4, 8, 2, 2, 8, 4, 4, 8, 2, 4 },
    { 4, 4, 4, 4, 8, 8, 4, 4, 4, 8, 2, 2, 4, 4, 4, 2, 4 },
    { 4, 4, 2, 2, 8, 8, 2, 4, 2, 2, 8, 2, 4, 4, 4, 2, 4 },
    { 4, 4, 8, 4, 0, 4, 4, 4, 4, 4, 8, 2, 2, 4, 4, 2, 4 },
    { 4, 8, 4, 8, 4, 4, 4, 4, 2, 4, 4, 4, 4, 2, 4, 4, 0 },
    { 4, 4, 8, 4, 8, 4, 4, 4, 2, 2, 2, 8, 4, 4, 2, 8, 4 },
    { 4, 4, 4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 4, 8, 4 },
    { 4, 2, 4, 4, 4, 4, 4, 8, 2, 4, 4, 4, 4, 8, 4, 4, 2 },
};

// How well one type attacks another: 0 not at all, 1 not very well, 2 normally and 3 super effectively. Type 17 is
// no type
static u8 func_ov055_021e6338(u32 attackType, u32 defenseType) {
    if (attackType == 17 || defenseType == 17) {
        return 2;
    }
    switch (sTypeChart[attackType][defenseType]) {
    case 0:
        return 0;
    case 2:
        return 1;
    case 4:
        return 2;
    case 8:
        return 3;
    }
    return 0;
}
