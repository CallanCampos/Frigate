#include <stddef.h>
#include "types.h"
#include "battle/regulation.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/text_banks.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/poke_party.h"
#include "save/save_control.h"
#include "save/tournament_block.h"

// The rules of a battle, and the save block of the competitions' regulations and the teams entered in them, which
// swan calls the tournament block. Block 0x2f.

#define SAVE_BLOCK_TOURNAMENT 0x2f

#define REGULATION_NAME_STRBUF_SIZE 26

static void loadBattleTypeTextToStrbuf(Regulation *regulation, StrBuf *strbuf);
static void TournamentTeam_SetField(TournamentTeam *team, u32 field, u8 index, u32 value);

u32 getSizeofTourneyBlock(void) {
    return sizeof(TournamentBlock);
}

Regulation *Regulation_Create(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(Regulation), TRUE, "regulation.c", 115);
}

void Regulation_Copy(const Regulation *src, Regulation *dest) {
    sys_memcpy(src, dest, sizeof(Regulation));
}

Regulation *func_0200b530(void *entry) {
    return entry;
}

static void loadBattleTypeTextToStrbuf(Regulation *regulation, StrBuf *strbuf) {
    MsgData *msgData =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_LOAD_BATTLE_TYPE_TEXT_TO_STRBUF, HEAPID_USER);

    GFL_MsgDataLoadStrbuf(msgData, regulation->name, strbuf);
    GFL_MsgDataFree(msgData);
}

StrBuf *makeStrbufLoadBattleTypeText(Regulation *regulation, HeapID heapId) {
    StrBuf *strbuf = GFL_StrBufCreate(REGULATION_NAME_STRBUF_SIZE, heapId);

    loadBattleTypeTextToStrbuf(regulation, strbuf);
    return strbuf;
}

u32 Regulation_GetParam(Regulation *regulation, u32 param) {
    u32 value = 0;

    switch (param) {
    case REGULATION_PARAM_NAME:
        value = regulation->name;
        break;
    case REGULATION_PARAM_1:
        value = regulation->unk1;
        break;
    case REGULATION_PARAM_2:
        value = regulation->unk2;
        break;
    case REGULATION_PARAM_3:
        value = regulation->unk3;
        break;
    case REGULATION_PARAM_4:
        value = regulation->unk4;
        break;
    case REGULATION_PARAM_5:
        value = regulation->unk5;
        break;
    case REGULATION_PARAM_6:
        value = regulation->unk6;
        break;
    case REGULATION_PARAM_7:
        value = regulation->unk8;
        break;
    case REGULATION_PARAM_8:
        value = regulation->unk9;
        break;
    case REGULATION_PARAM_11:
        value = regulation->unkA8;
        break;
    case REGULATION_PARAM_12:
        value = regulation->unkAA;
        break;
    case REGULATION_PARAM_13:
        value = regulation->unkAB;
        break;
    case REGULATION_PARAM_15:
        value = regulation->unkB3;
        break;
    case REGULATION_PARAM_16:
        value = regulation->unkB4;
        break;
    case REGULATION_PARAM_17:
        value = regulation->unkB5;
        break;
    case REGULATION_PARAM_18:
        value = regulation->unkB6;
        break;
    case REGULATION_PARAM_SHOW_PARTNERS:
        value = regulation->showPartners;
        break;
    case REGULATION_PARAM_TIME_LIMIT:
        value = regulation->timeLimit;
        break;
    case REGULATION_PARAM_BATTLE_TYPE:
        value = regulation->unkBA;
        break;
    case REGULATION_PARAM_22:
        value = regulation->unkBB;
        break;
    }
    return value;
}

void Regulation_SetParam(Regulation *regulation, u32 param, int value) {
    switch (param) {
    case REGULATION_PARAM_NAME:
        if (value < 7) {
            regulation->name = value;
        }
        break;
    case REGULATION_PARAM_1:
        if (value < 6) {
            regulation->unk1 = value;
        }
        break;
    case REGULATION_PARAM_2:
        if (value < 6) {
            regulation->unk2 = value;
        }
        break;
    case REGULATION_PARAM_3:
        if (value < 6) {
            regulation->unk3 = value;
        }
        break;
    case REGULATION_PARAM_4:
        if (value <= 100) {
            regulation->unk4 = value;
        }
        break;
    case REGULATION_PARAM_5:
        if (value <= 6) {
            regulation->unk5 = value;
        }
        break;
    case REGULATION_PARAM_6:
        if (value <= 600) {
            regulation->unk6 = value;
        }
        break;
    case REGULATION_PARAM_7:
        if (value < 2) {
            regulation->unk8 = value;
        }
        break;
    case REGULATION_PARAM_8:
        if (value < 2) {
            regulation->unk9 = value;
        }
        break;
    case REGULATION_PARAM_11:
        if (value < 686) {
            regulation->unkA8 = value;
        }
        break;
    case REGULATION_PARAM_12:
        regulation->unkAA = value;
        break;
    case REGULATION_PARAM_13:
        if (value < 2) {
            regulation->unkAB = value;
        }
        break;
    case REGULATION_PARAM_15:
        regulation->unkB3 = value;
        break;
    case REGULATION_PARAM_16:
        regulation->unkB4 = value;
        break;
    case REGULATION_PARAM_17:
        if (value < 2) {
            regulation->unkB5 = value;
        }
        break;
    case REGULATION_PARAM_18:
        regulation->unkB6 = value;
        break;
    case REGULATION_PARAM_SHOW_PARTNERS:
        if (value < 2) {
            regulation->showPartners = value;
        }
        break;
    case REGULATION_PARAM_TIME_LIMIT:
        if (value <= 99) {
            regulation->timeLimit = value;
        }
        break;
    case REGULATION_PARAM_BATTLE_TYPE:
        if (value < 6) {
            regulation->unkBA = value;
        }
        break;
    case REGULATION_PARAM_22:
        regulation->unkBB = value;
        break;
    }
}

BOOL Regulation_CheckParamBit(Regulation *regulation, u32 param, int index) {
    int byte = index / 8;
    int bit = index % 8;

    switch (param) {
    case REGULATION_PARAM_BITS_A:
        if (regulation->unkA[byte] & (1 << bit)) {
            return TRUE;
        }
        break;
    case REGULATION_PARAM_BITS_5C:
        if (regulation->unk5C[byte] & (1 << bit)) {
            return TRUE;
        }
        break;
    case REGULATION_PARAM_BITS_AC:
        if (regulation->unkAC[byte] & (1 << bit)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void func_0200b760(Regulation *regulation, RegulationUnkAC *out) {
    sys_memset(out, 0, sizeof(RegulationUnkAC));
    out->unk7 = Regulation_GetParam(regulation, REGULATION_PARAM_13);
    if (out->unk7 >= 2) {
        out->unk7 = 1;
    }
    sys_memcpy(regulation->unkAC, out->unk0, sizeof(out->unk0));
}

void *func_0200b794(void *entry) {
    TournamentEntry *tournament = entry;

    return tournament->unkC0;
}

u32 TournamentEntry_GetField(void *entry, u32 field) {
    TournamentEntry *tournament = entry;
    u32 value;

    switch (field) {
    case TOURNAMENT_FIELD_UNKBC:
        value = tournament->unkBC;
        break;
    case TOURNAMENT_FIELD_UNK152:
        value = tournament->unk152;
        break;
    case TOURNAMENT_FIELD_UNK154:
        value = tournament->unk154;
        break;
    case TOURNAMENT_FIELD_UNK155:
        value = tournament->unk155;
        break;
    case TOURNAMENT_FIELD_UNK156:
        value = tournament->unk156;
        break;
    case TOURNAMENT_FIELD_UNK157:
        value = tournament->unk157;
        break;
    case TOURNAMENT_FIELD_UNK158:
        value = tournament->unk158;
        break;
    case TOURNAMENT_FIELD_UNK159:
        value = tournament->unk159;
        break;
    case TOURNAMENT_FIELD_UNK15A:
        value = tournament->unk15A;
        break;
    case TOURNAMENT_FIELD_UNK15B:
        value = tournament->unk15B;
        break;
    case TOURNAMENT_FIELD_UNK15C:
        value = tournament->unk15C;
        break;
    case TOURNAMENT_FIELD_UNK15D:
        value = tournament->unk15D;
        break;
    }
    return value;
}

void TournamentEntry_SetField(void *entry, u32 field, u32 value) {
    TournamentEntry *tournament = entry;

    switch (field) {
    case TOURNAMENT_FIELD_UNKBC:
        tournament->unkBC = value;
        break;
    case TOURNAMENT_FIELD_UNK152:
        tournament->unk152 = value;
        break;
    case TOURNAMENT_FIELD_UNK154:
        tournament->unk154 = value;
        break;
    case TOURNAMENT_FIELD_UNK155:
        tournament->unk155 = value;
        break;
    case TOURNAMENT_FIELD_UNK156:
        tournament->unk156 = value;
        break;
    case TOURNAMENT_FIELD_UNK157:
        tournament->unk157 = value;
        break;
    case TOURNAMENT_FIELD_UNK158:
        tournament->unk158 = value;
        break;
    case TOURNAMENT_FIELD_UNK159:
        tournament->unk159 = value;
        break;
    case TOURNAMENT_FIELD_UNK15A:
        tournament->unk15A = value;
        break;
    case TOURNAMENT_FIELD_UNK15B:
        tournament->unk15B = value;
        break;
    case TOURNAMENT_FIELD_UNK15C:
        tournament->unk15C = value;
        break;
    case TOURNAMENT_FIELD_UNK15D:
        tournament->unk15D = value;
        break;
    }
}

BOOL TournamentEntry_CheckCRC(void *entry) {
    TournamentEntry *tournament = entry;

    if (tournament->crc == CRC16_350bytes(tournament)) {
        return TRUE;
    }
    return FALSE;
}

u16 CRC16_350bytes(const void *entry) {
    return getCRC16(entry, offsetof(TournamentEntry, crc));
}

void *TournamentBlock_GetEntry(void *block, u32 index) {
    TournamentBlock *tournament = block;

    return &tournament->entries[index];
}

void TournamentBlock_SetEntry(void *block, u32 index, const void *entry) {
    TournamentBlock *tournament = block;

    tournament->entries[index] = *(const TournamentEntry *)entry;
}

void TournamentTeam_Clear(void *record) {
    sys_memset(record, 0, sizeof(TournamentTeam));
}

void *TournamentBlock_GetTeam(void *block, u32 index) {
    TournamentBlock *tournament = block;

    return &tournament->teams[index];
}

u32 TournamentTeam_GetField(void *record, u32 field, u8 index) {
    TournamentTeam *team = record;

    switch (field) {
    case TOURNAMENT_TEAM_SPECIES:
        return team->species[index];
    case TOURNAMENT_TEAM_FORM:
        return team->form[index];
    case TOURNAMENT_TEAM_HAS_ITEM:
        return (team->hasItem >> index) & 1;
    case TOURNAMENT_TEAM_GENDER:
        return (team->gender >> index) & 1;
    }
    return 0;
}

static void TournamentTeam_SetField(TournamentTeam *team, u32 field, u8 index, u32 value) {
    switch (field) {
    case TOURNAMENT_TEAM_SPECIES:
        team->species[index] = value;
        break;
    case TOURNAMENT_TEAM_FORM:
        team->form[index] = value;
        break;
    case TOURNAMENT_TEAM_HAS_ITEM:
        team->hasItem |= (value == 1) << index;
        break;
    case TOURNAMENT_TEAM_GENDER:
        team->gender |= (value == 1) << index;
        break;
    }
}

void TournamentTeam_SetFromParty(void *record, PokeParty *party) {
    s32 i;

    TournamentTeam_Clear(record);
    for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL)) {
            TournamentTeam_SetField(record, TOURNAMENT_TEAM_SPECIES, i, PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL));
            TournamentTeam_SetField(record, TOURNAMENT_TEAM_FORM, i, PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL));
            TournamentTeam_SetField(record, TOURNAMENT_TEAM_HAS_ITEM, i, PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL) != 0);
            TournamentTeam_SetField(record, TOURNAMENT_TEAM_GENDER, i, PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL));
        }
    }
}

void clearTournamentBlock(void *block) {
    sys_memset(block, 0, sizeof(TournamentBlock));
}

void *getTournamentBlockAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_TOURNAMENT);
}
