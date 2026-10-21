#include "types.h"
#include "battle/trainer_data.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/script_text_banks.h"
#include "constants/text_banks.h"
#include "field/fest_mission_data.h"
#include "field/wbt.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/poke_party.h"
#include "pml/species_names.h"
#include "save/box.h"
#include "save/high_link.h"
#include "save/player_info.h"
#include "system/country_region.h"
#include "system/fest_mission.h"
#include "system/pms_data.h"
#include "system/pms_word.h"
#include "system/printsys.h"
#include "system/str_tool.h"
#include "system/wordset.h"

// Word sets: buffers of words that a message's word set commands are replaced by, such as names of Pokémon, items
// and moves, and numbers. Each buffer has a few attributes besides its string, one of which adds a layout command

typedef struct {
    u8 unk0;
    u8 unk1;
    u8 unk2;
    // The grammatical gender of the word, and whether it is plural
    u8 gender : 7;
    u8 plural : 1;
    // Whether the word gets a layout command in front of it
    u8 addLayoutCmd;
} WordSetAttr;

typedef struct {
    WordSetAttr attr;
    StrBuf *strbuf;
} WordSetBuf;

struct WordSet {
    u32 count;
    u32 heapId;
    WordSetBuf *bufs;
    // A buffer that each word is loaded to before it is copied
    StrBuf *tmp;
};

static void GFL_WordSetClearBufFlags(WordSetAttr *attr);
static void GFL_WordSetClearBuf(WordSet *wordSet, u32 index);
static void GFL_WordSetCopyStrbuf(WordSet *wordSet, u32 index, const StrBuf *strbuf, const WordSetAttr *attr);
static void GFL_WordSetLoadMsg(WordSet *wordSet, u32 index, u32 fileId, u32 messageId);

WordSet *GFL_WordSetSystemCreateDefault(HeapID heapId) {
    return GFL_WordSetSystemCreate(8, 0x20, heapId);
}

WordSet *GFL_WordSetSystemCreate(u32 count, u32 length, HeapID heapId) {
    WordSet *wordSet = GFL_HeapAllocate(heapId, sizeof(WordSet), FALSE, "wordset.c", 189);
    u32 i;

    if (wordSet != NULL) {
        wordSet->count = count;
        wordSet->heapId = heapId;
        wordSet->tmp = GFL_StrBufCreate(length, heapId);
        if (wordSet->tmp != NULL) {
            wordSet->bufs = GFL_HeapAllocate(heapId, sizeof(WordSetBuf) * count, FALSE, "wordset.c", 198);
            if (wordSet->bufs != NULL) {
                for (i = 0; i < count; i++) {
                    GFL_WordSetClearBufFlags(&wordSet->bufs[i].attr);
                    wordSet->bufs[i].strbuf = GFL_StrBufCreate(length, heapId);
                    if (wordSet->bufs[i].strbuf == NULL) {
                        break;
                    }
                }
                if (i == count) {
                    return wordSet;
                }
            }
        }
    }
    return NULL;
}

void GFL_WordSetSystemFree(WordSet *wordSet) {
    u32 i;

    if (wordSet->bufs != NULL) {
        for (i = 0; i < wordSet->count; i++) {
            if (wordSet->bufs[i].strbuf == NULL) {
                break;
            }
            GFL_StrBufFree(wordSet->bufs[i].strbuf);
        }
        GFL_HeapFree(wordSet->bufs);
    }
    if (wordSet->tmp != NULL) {
        GFL_StrBufFree(wordSet->tmp);
    }
    wordSet->count = 0;
    GFL_HeapFree(wordSet);
}

static void GFL_WordSetClearBufFlags(WordSetAttr *attr) {
    MI_CpuFill8(attr, 0, sizeof(WordSetAttr));
    attr->addLayoutCmd = FALSE;
}

static void GFL_WordSetClearBuf(WordSet *wordSet, u32 index) {
    GFL_WordSetClearBufFlags(&wordSet->bufs[index].attr);
    GFL_StrBufClear(wordSet->bufs[index].strbuf);
}

static void GFL_WordSetCopyStrbuf(WordSet *wordSet, u32 index, const StrBuf *strbuf, const WordSetAttr *attr) {
    if (index < wordSet->count) {
        if (attr != NULL) {
            wordSet->bufs[index].attr = *attr;
        } else {
            GFL_WordSetClearBufFlags(&wordSet->bufs[index].attr);
        }
        GFL_StrBufCopy(wordSet->bufs[index].strbuf, strbuf);
    }
}

static void GFL_WordSetLoadMsg(WordSet *wordSet, u32 index, u32 fileId, u32 messageId) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, (u16)fileId, wordSet->heapId);

    if (msgData != NULL) {
        GFL_MsgDataLoadStrbuf(msgData, messageId, wordSet->tmp);
        GFL_WordSetCopyStrbuf(wordSet, index, wordSet->tmp, NULL);
        GFL_MsgDataFree(msgData);
    }
}

void GFL_WordSetSetStrbuf(WordSet *wordSet, u32 index, const StrBuf *strbuf, u32 gender, u32 singular, u32 language) {
    WordSetAttr attr;

    GFL_WordSetClearBufFlags(&attr);
    attr.gender = gender;
    attr.plural = singular == 0 ? TRUE : FALSE;
    GFL_WordSetCopyStrbuf(wordSet, index, strbuf, &attr);
}

void WordSet_LoadSpeciesName(WordSet *wordSet, u32 index, u32 species) {
    GFL_MsgDataLoadStrbuf(g_PMLSpeciesNamesResident, species, wordSet->tmp);
    GFL_WordSetCopyStrbuf(wordSet, index, wordSet->tmp, NULL);
}

void setPartyPokemonSpeciesNameToStrbuf(WordSet *wordSet, u32 index, PartyPkm *pkm) {
    WordSet_LoadSpeciesName(wordSet, index, (u16)PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL));
}

void setBoxPokemonSpeciesNameToStrbuf(WordSet *wordSet, u32 index, BoxPkm *pkm) {
    WordSet_LoadSpeciesName(wordSet, index, (u16)PML_PkmGetParam(pkm, 5, NULL));
}

void loadPokemonTextNameToStrbuf(WordSet *wordSet, u32 index, u32 species) {
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_SPECIES_NAMES_WITH_ARTICLE, species);
}

void loadPokemonSpeciesTextNameToStrbuf(WordSet *wordSet, u32 index, PartyPkm *pkm) {
    loadPokemonTextNameToStrbuf(wordSet, index, (u16)PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL));
}

void loadPokemonNicknameToStrbuf(WordSet *wordSet, u32 index, PartyPkm *pkm) {
    PokeParty_GetParam(pkm, PKM_PARAM_NICKNAME, wordSet->tmp);
    GFL_WordSetCopyStrbuf(wordSet, index, wordSet->tmp, NULL);
}

void loadBoxPokemonNameToStrbuf(WordSet *wordSet, u32 index, BoxPkm *pkm) {
    PML_PkmGetParam(pkm, PKM_PARAM_NICKNAME, wordSet->tmp);
    GFL_WordSetCopyStrbuf(wordSet, index, wordSet->tmp, NULL);
}

void loadMoveNameToStrbuf(WordSet *wordSet, u32 index, u32 move) {
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_MOVE_NAMES, move);
}

void loadItemNameToStrbuf(WordSet *wordSet, u32 index, u32 item) {
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_ITEM_NAMES, item);
}

void loadItemTextNameToStrbuf(WordSet *wordSet, u32 index, u32 item) {
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_ITEM_NAMES_WITH_ARTICLE, item);
}

void loadItemsNameToStrbuf(WordSet *wordSet, u32 index, u32 item) {
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_ITEM_NAMES_PLURAL, item);
}

void loadItemText(WordSet *wordSet, u32 index, u32 item, BOOL plural, BOOL a4) {
    if (plural) {
        loadItemsNameToStrbuf(wordSet, index, item);
    } else if (a4) {
        loadItemTextNameToStrbuf(wordSet, index, item);
    } else {
        loadItemNameToStrbuf(wordSet, index, item);
    }
}

void loadAbilityNameToStrbuf(WordSet *wordSet, u32 index, u32 ability) {
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_ABILITY_NAMES, ability);
}

void loadNatureToStrbuf(WordSet *wordSet, u32 index, u32 nature) {
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_NATURES, nature);
}

void WordSetNumber(WordSet *wordSet, u32 index, s32 number, u32 digits, u32 pad, BOOL ascii) {
    GFL_WordSetFormatNumber(wordSet->tmp, number, digits, pad, ascii);
    GFL_WordSetCopyStrbuf(wordSet, index, wordSet->tmp, NULL);
}

void loadSayingForDisplay(WordSet *wordSet, u32 index, u16 saying) {
    u16 copy = saying;

    PMSWord_Validate(&copy, FALSE, FALSE);
    loadSayingToString(copy, wordSet->tmp, wordSet->heapId);
    GFL_WordSetCopyStrbuf(wordSet, index, wordSet->tmp, NULL);
}

void func_02024574(WordSet *wordSet, u32 index, u32 value) {
    int copy = value;

    PMSNumber_Validate(&copy, FALSE);
    if (index < wordSet->count) {
        wordSet->bufs[index].attr.addLayoutCmd = copy;
    }
}

void loadTypeTextToStrbuf(WordSet *wordSet, u32 index, u32 type) {
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_TYPE_NAMES, type);
}

void copyVarForText(WordSet *wordSet, u32 index, PlayerInfo *playerInfo) {
    textCopy(playerInfo->name, wordSet->tmp);
    GFL_WordSetCopyStrbuf(wordSet, index, wordSet->tmp, NULL);
}

void GFL_WordSetLoadStr(WordSet *wordSet, u32 index, const u16 *str) {
    GFL_StrBufLoadString(wordSet->tmp, str);
    GFL_WordSetCopyStrbuf(wordSet, index, wordSet->tmp, NULL);
}

void loadCountryToStrbuf(WordSet *wordSet, u32 index, u32 country) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_MAPCHANGE_LOAD_COUNTRY_TO_STRBUF,
                                          wordSet->heapId);

    if (msgData != NULL) {
        if (country < GFL_MsgDataGetLineCount(msgData)) {
            GFL_MsgDataLoadStrbuf(msgData, country, wordSet->tmp);
            GFL_WordSetCopyStrbuf(wordSet, index, wordSet->tmp, NULL);
        } else {
            GFL_WordSetClearBuf(wordSet, index);
        }
        GFL_MsgDataFree(msgData);
    }
}

void loadCountryAreaToStrbuf(WordSet *wordSet, u32 index, u32 country, u32 area) {
    u32 fileId = fetchFileNumOfDividedCountry(country);

    if (fileId != 0 && area != 0) {
        MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, (u16)fileId, wordSet->heapId);

        if (msgData != NULL) {
            if (area < GFL_MsgDataGetLineCount(msgData)) {
                GFL_MsgDataLoadStrbuf(msgData, area, wordSet->tmp);
                GFL_WordSetCopyStrbuf(wordSet, index, wordSet->tmp, NULL);
                GFL_MsgDataFree(msgData);
                return;
            }
            GFL_MsgDataFree(msgData);
        }
    }
    GFL_WordSetClearBuf(wordSet, index);
}

void loadTrainerTypeText(WordSet *wordSet, u32 index, u8 trainerType) {
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_TRAINER_CLASS_NAMES, trainerType);
}

void loadTrainerTypeToStrbuf(WordSet *wordSet, u32 index, u32 trainerId) {
    loadTrainerTypeText(wordSet, index, TrainerData_GetParam(trainerId, 1));
}

void loadTrainerTypeWithArticleToStrbuf(WordSet *wordSet, u32 index, u8 trainerType) {
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_TRAINER_CLASS_NAMES_WITH_ARTICLE, trainerType);
}

void loadTrainerNamesToStrbuf(WordSet *wordSet, u32 index, u32 trainerId) {
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_TRAINER_NAMES, trainerId);
}

void loadStatNameToStrbuf(WordSet *wordSet, u32 index, u8 stat) {
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_STAT_NAMES, stat);
}

void loadBagPocketNameToStrbuf(WordSet *wordSet, u32 index, u32 pocket) {
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_BAG_POCKETS, pocket);
}

void loadLocationNameToStrbuf(WordSet *wordSet, u32 index, u32 placeNameId) {
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_PLACE_NAMES, placeNameId);
}

void loadBoxNameForDisplay(WordSet *wordSet, u32 index, void *boxData, u32 box) {
    loadBoxNameToStrbuf(boxData, box, wordSet->tmp);
    GFL_WordSetCopyStrbuf(wordSet, index, wordSet->tmp, NULL);
}

void loadPassPowerToStrbuf(WordSet *wordSet, u32 index, u32 passPower) {
    if (!PassPower_IsIDValid(passPower)) {
        passPower = 0;
    }
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_PASS_POWERS, passPower);
}

void loadHobbyNameToStrbuf(WordSet *wordSet, u32 index, u8 hobby) {
    if (hobby > 8) {
        GFL_WordSetClearBuf(wordSet, index);
        return;
    }
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_HOBBIES, hobby);
}

void loadQuestionnaireAnswerToStrbuf(WordSet *wordSet, u32 index, u8 answer) {
    if (answer > 0x91) {
        GFL_WordSetClearBuf(wordSet, index);
        return;
    }
    GFL_WordSetLoadMsg(wordSet, index, 0, answer);
}

void loadJobAnswerToStrbuf(WordSet *wordSet, u32 index, u8 job) {
    if (job > 8) {
        GFL_WordSetClearBuf(wordSet, index);
        return;
    }
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_JOB_ANSWERS, job);
}

void loadBattleInstituteMsgForDisplay(WordSet *wordSet, u32 index, u32 rank) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10440, wordSet->heapId);

    GFL_WordSetClearBuf(wordSet, index);
    // BUG: the message data is not freed when the rank is out of range
    if (rank <= 6 && msgData != NULL) {
        GFL_MsgDataLoadStrbuf(msgData, rank + 0x2b, wordSet->tmp);
        GFL_WordSetCopyStrbuf(wordSet, index, wordSet->tmp, NULL);
        GFL_MsgDataFree(msgData);
    }
#ifdef BUGFIX
    else if (msgData != NULL) {
        GFL_MsgDataFree(msgData);
    }
#endif
}

void loadMedalNameToStrbuf(WordSet *wordSet, u32 index, u8 medal) {
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_MEDAL_NAMES, medal);
}

void loadMedalRankToStrbuf(WordSet *wordSet, u32 index, u8 rank, u32 medalType) {
    if (rank == 4 && medalType == 0x16) {
        rank++;
    }
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_MEDAL_RANKS, rank);
}

void loadFromEmptyFile(WordSet *wordSet, u32 index, u8 messageId) {
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_EMPTY, messageId);
}

void loadPokewoodLineToStrbuf(WordSet *wordSet, u32 index, u32 line) {
    if (line >= 0x29) {
        GFL_WordSetClearBuf(wordSet, index);
        return;
    }
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_POKEWOOD_LINES, line + 0x2a);
}

void WordSet_LoadPWTTournamentType(WordSet *wordSet, u32 index, u32 tournament) {
    if (tournament >= 0x29) {
        GFL_WordSetClearBuf(wordSet, index);
        return;
    }
    LoadPWTTournamentTypeText(wordSet->heapId, tournament, wordSet->tmp);
    GFL_WordSetCopyStrbuf(wordSet, index, wordSet->tmp, NULL);
}

void WordSet_LoadFestivalMission(WordSet *wordSet, u32 index, u8 mission, s32 a3) {
    // The mission's config, whose first 0x14 bytes are a FestMission (system/fest_mission.h)
    FestMissionConfig *buffer;
    FestivalText *text;

    if (mission >= FEST_MISSION_COUNT) {
        GFL_WordSetClearBuf(wordSet, index);
        return;
    }
    if (a3 >= 4) {
        a3 = 0;
    }
    buffer = allocateForFestMission(HEAPID_TAIL((HeapID)wordSet->heapId));
    text = getTextFileForFestMissions(HEAPID_TAIL((HeapID)wordSet->heapId));
    FestMission_Load(text->archive, mission, a3, (FestMission *)buffer);
    FestMission_GetMessage(text, wordSet->tmp, (const FestMission *)buffer, HEAPID_TAIL((HeapID)wordSet->heapId));
    GFL_WordSetCopyStrbuf(wordSet, index, wordSet->tmp, NULL);
    FestivalText_Free(text);
    GFL_HeapFree(buffer);
}

void loadMonthToStrbuf(WordSet *wordSet, u32 index, u32 month) {
    if (month < 1) {
        month = 1;
    } else if (month > 12) {
        month = 12;
    }
    GFL_WordSetLoadMsg(wordSet, index, TEXT_BANK_MONTHS, month - 1);
}

void GFL_WordSetFormatStrbuf(WordSet *wordSet, StrBuf *dest, const StrBuf *src) {
    const u16 *str = GFL_StrBufGetStringPtr(src);
    u32 eom;
    u32 cmdChar;

    GFL_StrBufClear(dest);
    eom = GFL_StrBufGetTerminator();
    cmdChar = GFL_StrCmdGetIdentChar();
    while (*str != eom) {
        if (*str == cmdChar) {
            if (GFL_StrCmdIsWordSet(str)) {
                u32 i = GFL_WordSetGetCommandParameter(str, 0);
                WordSetBuf *buf = &wordSet->bufs[i];

                if (buf->attr.addLayoutCmd) {
                    u16 param = 0x54;

                    GFL_StrCmdBuild(buf->strbuf, 0xbd, 4, 1, &param);
                }
                GFL_StrBufUncompress(dest, wordSet->bufs[i].strbuf);
                str = GFL_StrCmdSkipCommand(str);
            } else {
                const u16 *cmd = str;

                str = GFL_StrCmdSkipCommand(str);
                while (cmd < str) {
                    GFL_StrBufAppend(dest, *cmd++);
                }
            }
        } else {
            GFL_StrBufAppend(dest, *str++);
        }
    }
}

void GFL_WordSetClearAll(WordSet *wordSet) {
    u32 i;

    for (i = 0; i < wordSet->count; i++) {
        GFL_StrBufClear(wordSet->bufs[i].strbuf);
    }
}
