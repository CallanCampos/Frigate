#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "field/zone.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "nitro/rtc.h"
#include "pml/met_data.h"
#include "pml/poke_party.h"
#include "save/player_info.h"

// Where, when and by whom a Pokémon was met. The file's name is a guess, from swan's PML_PkmSetMetParams and
// PokeParty_SetupMetData: the ROM has no string for it

// The message file of the memo texts in ARCID_SYSTEM_MESSAGE, and N's name in it
#define MSG_FILE_MEMO TEXT_BANK_MEMO
#define MEMO_N_NAME 0x5c

// The Trainer ID of N's Pokémon
#define N_TRAINER_ID 2

// The index of the place name a location of a file uses when it is out of the file
#define PLACE_NAME_FALLBACK 2

static void PML_UtilSetNPokemonMetData(BoxPkm *pkm, u16 placeName, HeapID heapId);
static void PML_PkmClearMetParams(BoxPkm *pkm, BOOL isMet);
static void PML_PkmSetMetParams(BoxPkm *pkm, u32 location, BOOL isMet);
static void PML_PkmSetMetParamsEx(BoxPkm *pkm, u32 location, u32 year, u32 month, u32 day, BOOL isMet);
static void PML_UtilSyncHatchAndMetParams(BoxPkm *pkm, BOOL fromMet);
static void PML_PkmSetOT(BoxPkm *pkm, PlayerInfo *playerInfo, HeapID heapId);
static void PML_PkmSyncMetLevel(BoxPkm *pkm);
static void PML_PkmSyncOriginGame(BoxPkm *pkm);
static void PML_PkmSetSpecialTransferUsed(BoxPkm *pkm, u32 kind, PlayerInfo *playerInfo);

void PokeParty_SetupMetData(PartyPkm *pkm, u32 kind, PlayerInfo *playerInfo, u16 placeName, HeapID heapId) {
    PML_UtilSetupMetData(func_0201d620(pkm), kind, playerInfo, placeName, heapId);
}

void PML_UtilSetupMetData(BoxPkm *pkm, u32 kind, PlayerInfo *playerInfo, u16 placeName, HeapID heapId) {
    BOOL wasEncrypted = PML_PkmDecrypt(pkm);

    switch (kind) {
    case 0:
        PML_PkmClearMetParams(pkm, FALSE);
        PML_PkmSetMetParams(pkm, placeName, TRUE);
        PML_PkmSetOT(pkm, playerInfo, heapId);
        PML_PkmSyncMetLevel(pkm);
        PML_PkmSyncOriginGame(pkm);
        break;
    case 1:
        PML_PkmSetMetParams(pkm, LOCATION_IN_GAME_TRADE, TRUE);
        PML_PkmSyncMetLevel(pkm);
        PML_PkmSyncOriginGame(pkm);
        break;
    case 2:
        if (PML_UtilCheckForeignOT(pkm, playerInfo) == TRUE) {
            PML_PkmSetMetParams(pkm, placeName, TRUE);
            PML_PkmSetOT(pkm, playerInfo, heapId);
        } else {
            PML_UtilSyncHatchAndMetParams(pkm, TRUE);
            PML_PkmSetMetParams(pkm, placeName, TRUE);
            PML_PkmSetOT(pkm, playerInfo, heapId);
        }
        PML_PkmSyncOriginGame(pkm);
        break;
    case 5:
        PML_PkmSetMetParams(pkm, placeName, FALSE);
        PML_PkmSetOT(pkm, playerInfo, heapId);
        PML_PkmSyncOriginGame(pkm);
        break;
    case 6:
        PML_PkmSetMetParams(pkm, placeName, TRUE);
        break;
    case 7:
        PML_UtilSetNPokemonMetData(pkm, placeName, heapId);
        break;
    }
    PML_PkmReEncrypt(pkm, wasEncrypted);
}

void setMetCurrentDateTime(BoxPkm *pkm) {
    RTCDate date;

    RTC_GetCachedDate(&date);
    PML_PkmSetParam(pkm, PKM_PARAM_MET_YEAR, date.year);
    PML_PkmSetParam(pkm, PKM_PARAM_MET_MONTH, date.month);
    PML_PkmSetParam(pkm, PKM_PARAM_MET_DAY, date.day);
}

void setFatefulEncounterPkmData(BoxPkm *pkm, u16 location, u32 year, u32 month, u32 day) {
    // An egg gets the place and date as its egg's
    if (PML_PkmGetParam(pkm, 0xaa, NULL) == TRUE) {
        PML_PkmSetMetParamsEx(pkm, location, year, month, day, FALSE);
    } else {
        PML_PkmSetMetParamsEx(pkm, location, year, month, day, TRUE);
    }
    PML_PkmSyncMetLevel(pkm);
    PML_PkmSetParam(pkm, PKM_PARAM_FATEFUL_ENCOUNTER, TRUE);
}

void setDreamRadarPokeMetInfo(BoxPkm *pkm) {
    PML_PkmClearMetParams(pkm, FALSE);
    PML_PkmSetMetParams(pkm, LOCATION_DREAM_RADAR, TRUE);
    PML_PkmSyncMetLevel(pkm);
    PML_PkmSyncOriginGame(pkm);
}

static void PML_UtilSetNPokemonMetData(BoxPkm *pkm, u16 placeName, HeapID heapId) {
    MsgData *msgData;
    StrBuf *name;

    PML_PkmClearMetParams(pkm, FALSE);
    PML_PkmSetMetParams(pkm, placeName, TRUE);
    PML_PkmSyncMetLevel(pkm);
    PML_PkmSyncOriginGame(pkm);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, MSG_FILE_MEMO, heapId);
    name = GFL_MsgDataLoadStrbufNew(msgData, MEMO_N_NAME);
    PML_PkmSetParam(pkm, PKM_PARAM_OT_NAME, (u32)name);
    PML_PkmSetParam(pkm, PKM_PARAM_OT_GENDER, GENDER_MALE);
    PML_PkmSetParam(pkm, PKM_PARAM_ID, N_TRAINER_ID);
    GFL_StrBufFree(name);
    GFL_MsgDataFree(msgData);
}

static void PML_PkmClearMetParams(BoxPkm *pkm, BOOL isMet) {
    PML_PkmSetMetParamsEx(pkm, 0, 0, 0, 0, isMet);
}

static void PML_PkmSetMetParams(BoxPkm *pkm, u32 location, BOOL isMet) {
    RTCDate date;

    RTC_GetCachedDate(&date);
    if (IsMetLocationEntralink(location) == TRUE) {
        PML_PkmSetMetParamsEx(pkm, LOCATION_ENTRALINK, date.year, date.month, date.day, isMet);
    } else {
        PML_PkmSetMetParamsEx(pkm, location, date.year, date.month, date.day, isMet);
    }
}

// Sets the location and date of the meeting, or of the egg's
static void PML_PkmSetMetParamsEx(BoxPkm *pkm, u32 location, u32 year, u32 month, u32 day, BOOL isMet) {
    u8 offset = isMet == TRUE ? 1 : 0;

    PML_PkmSetParam(pkm, PKM_PARAM_EGG_LOCATION + offset, location);
    offset *= 3;
    PML_PkmSetParam(pkm, PKM_PARAM_EGG_YEAR + offset, year);
    PML_PkmSetParam(pkm, PKM_PARAM_EGG_MONTH + offset, month);
    PML_PkmSetParam(pkm, PKM_PARAM_EGG_DAY + offset, day);
}

// Copies the location and date of the meeting to the egg's, or the egg's to the meeting's
static void PML_UtilSyncHatchAndMetParams(BoxPkm *pkm, BOOL fromMet) {
    u8 offset = fromMet == TRUE ? 1 : 0;
    u32 location = PML_PkmGetParam(pkm, PKM_PARAM_EGG_LOCATION + offset, NULL);
    u32 year;
    u32 month;
    u32 day;

    offset *= 3;
    year = PML_PkmGetParam(pkm, PKM_PARAM_EGG_YEAR + offset, NULL);
    month = PML_PkmGetParam(pkm, PKM_PARAM_EGG_MONTH + offset, NULL);
    day = PML_PkmGetParam(pkm, PKM_PARAM_EGG_DAY + offset, NULL);
    PML_PkmSetMetParamsEx(pkm, location, year, month, day, fromMet == FALSE ? TRUE : FALSE);
}

static void PML_PkmSetOT(BoxPkm *pkm, PlayerInfo *playerInfo, HeapID heapId) {
    StrBuf *name = copyTrainerNameToNewStrbuf(playerInfo->name, heapId);
    u32 id = getIDAsUInt(playerInfo);
    u32 gender = getTrainerGender(playerInfo);

    PML_PkmSetParam(pkm, PKM_PARAM_OT_NAME, (u32)name);
    PML_PkmSetParam(pkm, PKM_PARAM_OT_GENDER, gender);
    PML_PkmSetParam(pkm, PKM_PARAM_ID, id);
    GFL_StrBufFree(name);
}

static void PML_PkmSyncMetLevel(BoxPkm *pkm) {
    PML_PkmSetParam(pkm, PKM_PARAM_MET_LEVEL, PML_PkmGetParam(pkm, PKM_PARAM_LEVEL, NULL));
}

static void PML_PkmSyncOriginGame(BoxPkm *pkm) {
    PML_PkmSetParam(pkm, PKM_PARAM_ORIGIN_GAME, GAME_VERSION);
}

BOOL PokeParty_IsSpecialTransfer(PartyPkm *pkm, u32 kind, PlayerInfo *playerInfo) {
    return special_transfers(func_0201d620(pkm), kind, playerInfo);
}

BOOL special_transfers(BoxPkm *pkm, u32 kind, PlayerInfo *playerInfo) {
    BOOL wasEncrypted = PML_PkmDecrypt(pkm);
    BOOL hatched = FALSE;
    u32 species = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u32 form = PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);
    u32 metLocation = PML_PkmGetParam(pkm, PKM_PARAM_MET_LOCATION, NULL);
    u32 fateful = PML_PkmGetParam(pkm, PKM_PARAM_FATEFUL_ENCOUNTER, NULL);
    BOOL isRare = PML_PkmIsRare(pkm);
    BOOL foreignOT = PML_UtilCheckForeignOT(pkm, playerInfo);

    if (PML_PkmGetParam(pkm, PKM_PARAM_EGG_YEAR, NULL) != 0 || PML_PkmGetParam(pkm, PKM_PARAM_EGG_MONTH, NULL) != 0 ||
        PML_PkmGetParam(pkm, PKM_PARAM_EGG_DAY, NULL) != 0 || PML_PkmGetParam(pkm, PKM_PARAM_EGG_LOCATION, NULL) != 0) {
        hatched = TRUE;
    }
    PML_PkmReEncrypt(pkm, wasEncrypted);

    switch (kind) {
    case 0:
        if (species == SPECIES_CELEBI && metLocation == LOCATION_EVENT_CELEBI && fateful == TRUE && hatched == FALSE &&
            isRare == FALSE) {
            return TRUE;
        }
        break;
    case 1:
        if (species == SPECIES_CELEBI && metLocation == LOCATION_EVENT_CELEBI_USED && fateful == TRUE &&
            hatched == FALSE && isRare == FALSE) {
            return TRUE;
        }
        break;
    case 2:
        if (species - SPECIES_RAIKOU <= SPECIES_SUICUNE - SPECIES_RAIKOU && metLocation == LOCATION_EVENT_BEASTS &&
            isRare == TRUE && fateful == TRUE && hatched == FALSE) {
            return TRUE;
        }
        break;
    case 3:
        if (species - SPECIES_RAIKOU <= SPECIES_SUICUNE - SPECIES_RAIKOU && metLocation == LOCATION_EVENT_BEASTS_USED &&
            isRare == TRUE && fateful == TRUE && hatched == FALSE) {
            return TRUE;
        }
        // BUG: There is no break, so kind 3 also takes an event Keldeo
#ifdef BUGFIX
        break;
#endif
    case 4:
        if (species == SPECIES_KELDEO && fateful == TRUE && hatched == FALSE) {
            return TRUE;
        }
        break;
    case 5:
        if (species == SPECIES_MELOETTA && fateful == TRUE && hatched == FALSE) {
            return TRUE;
        }
        break;
    case 6:
        if (species == SPECIES_GENESECT && fateful == TRUE && hatched == FALSE) {
            return TRUE;
        }
        break;
    case 7:
        if (species == SPECIES_SHAYMIN && fateful == TRUE && hatched == FALSE) {
            return TRUE;
        }
        break;
    case 8:
        if (species == SPECIES_LANDORUS && form == 1) {
            if (foreignOT == TRUE && metLocation == LOCATION_DREAM_RADAR && hatched == FALSE) {
                return TRUE;
            }
            if (hatched == FALSE && fateful == TRUE) {
                return TRUE;
            }
        }
        break;
    }
    return FALSE;
}

void PokeParty_SetSpecialTransferUsed(PartyPkm *pkm, u32 kind, PlayerInfo *playerInfo) {
    PML_PkmSetSpecialTransferUsed(func_0201d620(pkm), kind, playerInfo);
}

static void PML_PkmSetSpecialTransferUsed(BoxPkm *pkm, u32 kind, PlayerInfo *playerInfo) {
    BOOL wasEncrypted = PML_PkmDecrypt(pkm);

    if (special_transfers(pkm, kind, playerInfo) == TRUE) {
        switch (kind) {
        case 0:
            PML_PkmSetParam(pkm, PKM_PARAM_MET_LOCATION, LOCATION_EVENT_CELEBI_USED);
            break;
        case 2:
            PML_PkmSetParam(pkm, PKM_PARAM_MET_LOCATION, LOCATION_EVENT_BEASTS_USED);
            break;
        }
    }
    PML_PkmReEncrypt(pkm, wasEncrypted);
}

u32 MetLocation_GetNameFile(u32 location) {
    if (location < 30001) {
        return PLACE_FILE_UNOVA;
    }
    if (location < 40001) {
        if (location - 30001 >= 15) {
            return PLACE_FILE_UNOVA;
        }
        return PLACE_FILE_SPECIAL;
    }
    if (location < 60001) {
        if (location - 40001 >= 109) {
            return PLACE_FILE_UNOVA;
        }
        return PLACE_FILE_EVENT;
    }
    if (location <= 0xffff) {
        return PLACE_FILE_EXTERNAL;
    }
    return PLACE_FILE_UNOVA;
}

u32 MetLocation_GetNameIndex(u32 location) {
    if (location < 30001) {
        if (location < 154) {
            return location;
        }
        return PLACE_NAME_FALLBACK;
    }
    if (location < 40001) {
        if (location - 30001 < 15) {
            return location - 30001;
        }
        return PLACE_NAME_FALLBACK;
    }
    if (location < 60001) {
        if (location - 40001 < 109) {
            return location - 40001;
        }
        return PLACE_NAME_FALLBACK;
    }
    if (location <= 0xffff) {
        if (location - 60001 < 3) {
            return location - 60001;
        }
        return 0;
    }
    return PLACE_NAME_FALLBACK;
}
