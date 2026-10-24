// The battle's message strings: loads a message, fills its word set placeholders with the names and numbers its
// arguments give, and formats it, for btlv_core.c and btlv_scu.c. The name is a guess from Black and White's
// BTL_STR module, which does the same.

#include "types.h"
#include "battle/btl_main.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_setup.h"
#include "battle/btl_string.h"
#include "battle/trainer_data.h"
#include "constants/arc.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "save/player_info.h"
#include "stdarg.h"
#include "system/printsys.h"
#include "system/wordset.h"

typedef void (*BtlStringFunc)(StrBuf *strbuf, u16 message, const u32 *args);

// A standard message that has its own way to pick its variant
typedef struct BtlStringHandler {
    u16 message;
    BtlStringFunc func;
} BtlStringHandler;

typedef struct BtlStringWork {
    BtlMainModule *mainModule;
    const BtlPokeCon *pokeCon;
    WordSet *wordSet;
    StrBuf *strbuf;
    MsgData *msgData[11];
    // The arguments of BtlString_FormatStdMsg
    u32 args[9];
    HeapID heapId;
    u32 clientId;
    u16 unk68;
    // Set while func_ov167_021d5778 formats, which names the Pokémon behind an Illusion
    u16 trueNames;
} BtlStringWork;

static void SetMonNickname(u8 monId, u32 index);
static void SetMonTrueNickname(u8 monId, u32 index);
static void SetMonSpeciesName(u8 monId, u8 index);
static void SetTrainerClassName(WordSet *wordSet, u8 index, u8 clientId);
static void SetTrainerName(WordSet *wordSet, u8 index, u8 clientId);
static void FormatStdMsgCore(StrBuf *strbuf, u16 message, const u32 *args);
static const u16 *FindNextWordSetCommand(const u16 *str, u32 *category, u16 *index, u8 *param);
static u8 CountMonNames(const StrBuf *strbuf);
static void RegisterArgWords(StrBuf *strbuf, const u32 *args, WordSet *wordSet);
static void FormatSetMsgDefault(StrBuf *strbuf, u16 message, const u32 *args);
static void FormatSetMsgOneMon(StrBuf *strbuf, u16 message, const u32 *args);
static void FormatSetMsgTwoMons(StrBuf *strbuf, u16 message, const u32 *args);
static void FormatItemStatRise(StrBuf *strbuf, u16 message, const u32 *args);
static void FormatStatChange(StrBuf *strbuf, u16 message, const u32 *args);
static void FormatStatLimit(StrBuf *strbuf, u16 message, const u32 *args);
static void FormatClientSideMsg(StrBuf *strbuf, u16 message, const u32 *args);
static void func_ov167_021d5778(StrBuf *strbuf, u16 message, const u32 *args);
static void BtlString_BuildTrainerStringBody(StrBuf *strbuf, u16 message, const u32 *args);

// The message files, in the order of BtlStringWork's msgData
static const u16 sMsgFiles[] = { 0x13, 0x12, 0x10, 0x14, 0x176, 0x191, 0x1b8, 0x178, 0x197 };

// The message files of the studio's movies
static const u32 sMovieMsgFiles[] = { 0x137, 0x138, 0x136, 0x135, 0x134 };

// The standard messages that have a variant for the other side, at the next ID
static const u16 sSideMessages[] = {
    0x94, 0x96, 0x98, 0x9a, 0x9c, 0x9e, 0xa0, 0xa2, 0xb0, 0x84, 0x86, 0x80, 0x82, 0x7c,
    0x7e, 0x88, 0x8a, 0x8c, 0x8e, 0x90, 0x92, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae,
};

// clang-format off
static const BtlStringHandler sHandlers[] = {
    { 0x1b, FormatStatChange },
    { 0x5a, FormatStatChange },
    { 0x99, FormatStatLimit },
    { 0xae, FormatStatLimit },
    { 0x3aa, FormatItemStatRise },
    { 0x21, FormatClientSideMsg },
    { 0x414, FormatSetMsgOneMon },
    { 0x417, FormatSetMsgOneMon },
    { 0x14d, FormatSetMsgOneMon },
    { 0x471, FormatSetMsgOneMon },
    { 0x9, FormatSetMsgOneMon },
    { 0xc, FormatSetMsgOneMon },
    { 0x12, FormatSetMsgOneMon },
    { 0x15, FormatSetMsgOneMon },
};

// clang-format on

static BtlStringWork sWork;

// Which variant of a message names a Pokémon: 0 for the player's side, 1 for a wild Pokémon, 2 for a foe's
static inline int GetMonVariant(u8 monId) {
    if (sWork.unk68 == 0 && AreClientsOnOppositeSides(sWork.mainModule, sWork.clientId, GetClientIDFromMonID(monId))) {
        if (BtlSetup_GetBattleType(sWork.mainModule) == 0 || BtlSetup_GetBattleType(sWork.mainModule) == 4) {
            return 1;
        }
        return 2;
    }
    return 0;
}

// The message file of a studio movie's script, one for each of the player's sexes
static inline u16 GetMovieScriptMsgFile(u8 movie, u8 male) {
    return 0xb5 + movie * 2 + male;
}

// The message files of the Pokéstar Studios movie being shot, in a movie battle (mode 1) or its practice (mode 2?)
static inline void LoadMovieMsgData(BtlMainModule *mainModule, u32 mode, HeapID heapId) {
    int male = FALSE;

    if (mode != 0) {
        BtlScriptedRules *rules = GetStudioRules(mainModule);
        if (getTrainerGender(GetPlayerInfo(mainModule)) == 0) {
            male = TRUE;
        }
        sWork.msgData[9] =
            GFL_MsgSysLoadData(FALSE, 2, GetMovieScriptMsgFile(GetStudioMovieId(mainModule), male), heapId);
        if (mode == 1) {
            sWork.msgData[10] = GFL_MsgSysLoadData(FALSE, 2, sMovieMsgFiles[rules->unk08], heapId);
        }
    }
}

void BtlString_Init(BtlMainModule *mainModule, u8 clientId, const BtlPokeCon *pokeCon, HeapID heapId) {
    u32 mode;
    int i;
    u32 fileId;

    mode = GetStudioMode(mainModule);
    sWork.mainModule = mainModule;
    sWork.pokeCon = pokeCon;
    sWork.heapId = heapId;
    sWork.clientId = clientId;
    sWork.unk68 = BtlSetup_IsBattleType(mainModule, 0x200);
    sWork.trueNames = FALSE;
    sWork.wordSet = GFL_WordSetSystemCreateDefault(heapId);
    sWork.strbuf = GFL_StrBufCreate(0x2f4, heapId);
    for (i = 0; i < 9; i++) {
        fileId = sMsgFiles[i];
        if (mode == 1 && fileId == 0x13) {
            fileId = 0x11;
        }
        sWork.msgData[i] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, fileId, heapId);
    }
    LoadMovieMsgData(mainModule, mode, heapId);
}

void BtlString_Exit(void) {
    int i;

    GFL_WordSetSystemFree(sWork.wordSet);
    GFL_StrBufFree(sWork.strbuf);
    for (i = 0; i < 11; i++) {
        if (sWork.msgData[i] != NULL) {
            GFL_MsgDataFree(sWork.msgData[i]);
        }
    }
}

// A Pokémon's nickname as the battle shows it, the Illusion's when it has one
static void SetMonNickname(u8 monId, u32 index) {
    loadPokemonNicknameToStrbuf(sWork.wordSet, index,
                                BattleMon_GetViewSrcData(GetPokeParamConst(sWork.pokeCon, monId)));
}

// A Pokémon's own nickname
static void SetMonTrueNickname(u8 monId, u32 index) {
    loadPokemonNicknameToStrbuf(sWork.wordSet, index, GetSrcData(GetPokeParamConst(sWork.pokeCon, monId)));
}

static void SetMonSpeciesName(u8 monId, u8 index) {
    setPartyPokemonSpeciesNameToStrbuf(sWork.wordSet, index,
                                       BattleMon_GetViewSrcData(GetPokeParamConst(sWork.pokeCon, monId)));
}

// A client's trainer class
static void SetTrainerClassName(WordSet *wordSet, u8 index, u8 clientId) {
    u32 trainerClass = GetClientTrainerClass(sWork.mainModule, clientId);
    StrBuf *name = GetBattleSetupParam(sWork.mainModule)->trainerNames[clientId];

    if (name != NULL && GFL_StrBufIsValid(name)) {
        GFL_WordSetSetStrbuf(wordSet, index, name, TrainerClass_GetSex(trainerClass), 1, 2);
    } else {
        loadTrainerTypeText(wordSet, index, (u8)trainerClass);
    }
}

// A client's trainer name
static void SetTrainerName(WordSet *wordSet, u8 index, u8 clientId) {
    u32 trainerClass;

    if (clientId != 4) {
        if (IsClientNPCTrainer(sWork.mainModule, clientId)) {
            StrBuf *name = GetClientTrainerName(sWork.mainModule, clientId, &trainerClass);
            GFL_WordSetSetStrbuf(wordSet, index, name, TrainerClass_GetSex(trainerClass), 1, 2);
        } else {
            copyVarForText(wordSet, index, GetClientPlayerInfo(sWork.mainModule, clientId));
        }
    } else {
        copyVarForText(wordSet, index, GetCommSupporterInfo(sWork.mainModule));
    }
}

void BtlString_FormatStdMsg(StrBuf *strbuf, u16 message, u32 count, ...) {
    va_list list;
    u32 i;

    va_start(list, count);
    for (i = 0; i < count; i++) {
        sWork.args[i] = va_arg(list, u32);
    }
    while (i < NELEMS(sWork.args)) {
        sWork.args[i++] = 0;
    }
    va_end(list);
    BtlString_FormatStdMsgArgs(strbuf, message, sWork.args);
}

void BtlString_FormatStdMsgArgs(StrBuf *strbuf, u16 message, const u32 *args) {
    u32 i;

    for (i = 0; i < NELEMS(sSideMessages); i++) {
        if (message == sSideMessages[i]) {
            if (!IsPlayerSide(sWork.mainModule, args[0])) {
                message++;
            }
            break;
        }
    }
    FormatStdMsgCore(strbuf, message, args);
}

static void FormatStdMsgCore(StrBuf *strbuf, u16 message, const u32 *args) {
    GFL_MsgDataLoadStrbuf(sWork.msgData[0], message, sWork.strbuf);
    RegisterArgWords(sWork.strbuf, args, sWork.wordSet);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

void BtlString_FormatSetMsg(StrBuf *strbuf, u16 message, const u32 *args) {
    u32 i;

    for (i = 0; i < NELEMS(sHandlers); i++) {
        if (message == sHandlers[i].message) {
            sHandlers[i].func(strbuf, message, args);
            return;
        }
    }
    FormatSetMsgDefault(strbuf, message, args);
}

// Finds the next word set command in a string, and returns the string after it, or NULL at its end
static const u16 *FindNextWordSetCommand(const u16 *str, u32 *category, u16 *index, u8 *param) {
    if (str != NULL) {
        u16 eom = GFL_StrBufGetTerminator();
        u16 tag = GFL_StrCmdGetIdentChar();

        while (*str != eom) {
            if (*str == tag) {
                if (GFL_StrCmdIsWordSet(str)) {
                    *param = GFL_WordSetGetCommandParameter(str, 0);
                    *category = GFL_StrCmdGetCommandCategory(str);
                    *index = GFL_StrCmdGetCommandIndex(str);
                    return GFL_StrCmdSkipCommand(str);
                }
                str = GFL_StrCmdSkipCommand(str);
            } else {
                str++;
            }
        }
        return NULL;
    }
    return NULL;
}

// Counts the Pokémon names in a message
static u8 CountMonNames(const StrBuf *strbuf) {
    u8 param;
    u16 index;
    u32 category;
    u8 count;
    const u16 *str;

    str = GFL_StrBufGetStringPtr(strbuf);
    count = 0;
    while (str != NULL) {
        str = FindNextWordSetCommand(str, &category, &index, &param);
        if (str != NULL && category == STRCMD_CATEGORY_WORD) {
            switch (index) {
            case 1:
            case 2:
            case 12:
                count++;
                break;
            }
        }
    }
    return count;
}

// Fills the word set from the arguments, by the commands in the string
static void RegisterArgWords(StrBuf *strbuf, const u32 *args, WordSet *wordSet) {
    u16 eom;
    u16 tag;
    u8 skip[8];
    u32 i;
    const u16 *str;

    eom = GFL_StrBufGetTerminator();
    tag = GFL_StrCmdGetIdentChar();
    for (i = 0; i < 8; i++) {
        skip[i] = 0;
    }

    // A trainer name command takes no argument of its own, so the words after it take earlier arguments
    str = GFL_StrBufGetStringPtr(strbuf);
    while (*str != eom) {
        if (*str == tag) {
            if (GFL_StrCmdIsWordSet(str)) {
                u16 param = GFL_WordSetGetCommandParameter(str, 0);
                u8 category = GFL_StrCmdGetCommandCategory(str);
                if (category == STRCMD_CATEGORY_WORD && GFL_StrCmdGetCommandIndex(str) == 14) {
                    for (i = param + 1; i < 8; i++) {
                        skip[i]++;
                    }
                }
            }
            str = GFL_StrCmdSkipCommand(str);
        } else {
            str++;
        }
    }

    str = GFL_StrBufGetStringPtr(strbuf);
    while (*str != eom) {
        if (*str == tag) {
            if (GFL_StrCmdIsWordSet(str)) {
                u32 param = GFL_WordSetGetCommandParameter(str, 0);
                u32 arg = param;
                u8 category = GFL_StrCmdGetCommandCategory(str);

                if (param >= skip[param]) {
                    arg = param - skip[param];
                }
                if (category == STRCMD_CATEGORY_NUMBER) {
                    // The command's index is the number's digits less one
                    u8 digits = GFL_StrCmdGetCommandIndex(str) + 1;
                    WordSetNumber(wordSet, param, args[arg], digits, 0, 1);
                } else {
                    switch (GFL_StrCmdGetCommandIndex(str)) {
                    case 14:
                        SetTrainerClassName(wordSet, param, args[arg]);
                        break;
                    case 0:
                        SetTrainerName(wordSet, param, args[arg]);
                        break;
                    case 2:
                        if (sWork.trueNames == FALSE) {
                            SetMonNickname(args[arg], param);
                        } else {
                            SetMonTrueNickname(args[arg], param);
                        }
                        break;
                    case 12:
                        SetMonTrueNickname(args[arg], param);
                        break;
                    case 1:
                        SetMonSpeciesName(args[arg], param);
                        break;
                    case 3:
                        loadTypeTextToStrbuf(wordSet, param, args[arg]);
                        break;
                    case 6:
                        loadAbilityNameToStrbuf(wordSet, param, args[arg]);
                        break;
                    case 7:
                        loadMoveNameToStrbuf(wordSet, param, args[arg]);
                        break;
                    case 9:
                        loadItemNameToStrbuf(wordSet, param, args[arg]);
                        break;
                    case 15:
                        loadTypeTextToStrbuf(wordSet, param, args[arg]);
                        break;
                    }
                }
            }
            str = GFL_StrCmdSkipCommand(str);
        } else {
            str++;
        }
    }
}

static void FormatSetMsgDefault(StrBuf *strbuf, u16 message, const u32 *args) {
    GFL_MsgDataLoadStrbuf(sWork.msgData[1], message, sWork.strbuf);
    if (CountMonNames(sWork.strbuf) == 2) {
        FormatSetMsgTwoMons(strbuf, message, args);
    } else {
        FormatSetMsgOneMon(strbuf, message, args);
    }
}

// A message about one Pokémon, args[0]
static void FormatSetMsgOneMon(StrBuf *strbuf, u16 message, const u32 *args) {
    GFL_MsgDataLoadStrbuf(sWork.msgData[1], (u16)(message + GetMonVariant(args[0])), sWork.strbuf);
    RegisterArgWords(sWork.strbuf, args, sWork.wordSet);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

// A message about two Pokémon, args[0] and args[1], in seven variants: three for the player's Pokémon by the other's
// variant, and two each for a wild or a foe's Pokémon, by whether the other is of the same kind
static void FormatSetMsgTwoMons(StrBuf *strbuf, u16 message, const u32 *args) {
    u8 target = args[1];
    int variant = GetMonVariant(args[0]);

    switch (variant) {
    case 0:
        message = message + GetMonVariant(target);
        // Only this layout matches: the original's case 0 jumps past the test of the other cases
        goto load;
    case 1:
        message += 3;
        break;
    case 2:
        message += 5;
        break;
    }
    if (variant == GetMonVariant(target)) {
        message += 1;
    }
load:
    GFL_MsgDataLoadStrbuf(sWork.msgData[1], message, sWork.strbuf);
    RegisterArgWords(sWork.strbuf, args, sWork.wordSet);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

// An item raised a stat: args are the Pokémon, the item, the stat and how many stages
static void FormatItemStatRise(StrBuf *strbuf, u16 message, const u32 *args) {
    u8 stat = args[2] - 1;
    int degree = 0;
    int variant;
    int base;

    if ((int)args[3] >= 3) {
        degree = 2;
    }
    if (args[3] == 2) {
        degree = 1;
    }
    message += degree * 21;
    variant = GetMonVariant(args[0]);
    SetMonNickname(args[0], 0);
    loadItemNameToStrbuf(sWork.wordSet, 1, args[1]);
    // The stat's message, before the variant is added: only a variable of its own puts the variant second in the add
    base = message + stat * 3;
    GFL_MsgDataLoadStrbuf(sWork.msgData[1], (u16)(base + variant), sWork.strbuf);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

// A stat rose or fell: args are the Pokémon, the stat and how many stages
static void FormatStatChange(StrBuf *strbuf, u16 message, const u32 *args) {
    u8 stat = args[1] - 1;
    int degree = 0;

    if ((int)args[2] >= 3) {
        degree = 2;
    }
    if (args[2] == 2) {
        degree = 1;
    }
    message += degree * 21;
    SetMonNickname(args[0], 0);
    GFL_MsgDataLoadStrbuf(sWork.msgData[1], (u16)(message + stat * 3 + GetMonVariant(args[0])), sWork.strbuf);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

// A stat can't go any higher or lower: args are the Pokémon and the stat
static void FormatStatLimit(StrBuf *strbuf, u16 message, const u32 *args) {
    u8 stat = args[1] - 1;

    SetMonNickname(args[0], 0);
    GFL_MsgDataLoadStrbuf(sWork.msgData[1], (u16)(message + stat * 3 + GetMonVariant(args[0])), sWork.strbuf);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

// Picks one of three messages by whose client args[0] is
static void FormatClientSideMsg(StrBuf *strbuf, u16 message, const u32 *args) {
    u8 clientId = args[0];

    if (clientId != GetPlayerClientID(sWork.mainModule)) {
        if (IsClientNPCTrainer(sWork.mainModule, clientId)) {
            message = 0x23;
        } else {
            message = 0x22;
        }
    } else {
        message = 0x21;
    }
    GFL_MsgDataLoadStrbuf(sWork.msgData[0], message, sWork.strbuf);
    RegisterArgWords(sWork.strbuf, args, sWork.wordSet);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

void BtlString_FormatMoveMsg(StrBuf *strbuf, u8 monId, u16 move) {
    int variant = GetMonVariant(monId);

    SetMonNickname(monId, 0);
    move *= 3;
    GFL_MsgDataLoadStrbuf(sWork.msgData[2], (u16)(move + variant), sWork.strbuf);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

void func_ov167_021d5700(StrBuf *strbuf, u16 arg1, u32 arg2) {
    if (arg1 >= 0x800) {
        BtlSetup *setup = GetBattleSetupParam(sWork.mainModule);
        if (arg2 == 0) {
            GFL_StrBufCopy(strbuf, setup->unk6C);
        } else {
            GFL_StrBufCopy(strbuf, setup->unk68);
        }
    } else if (arg1 != 0) {
        u16 message = (arg1 - 1) * 3;
        if (arg2 == 0) {
            message += 1;
        } else {
            message += 2;
        }
        GFL_MsgDataLoadStrbuf(sWork.msgData[7], message, strbuf);
    }
}

void BtlString_LoadUIMsg(StrBuf *strbuf, u16 message) {
    GFL_MsgDataLoadStrbuf(sWork.msgData[3], message, strbuf);
}

void func_ov167_021d5770(StrBuf *strbuf, u16 message, const u32 *args) {
    func_ov167_021d5778(strbuf, message, args);
}

static void func_ov167_021d5778(StrBuf *strbuf, u16 message, const u32 *args) {
    GFL_MsgDataLoadStrbuf(sWork.msgData[5], message, sWork.strbuf);
    sWork.trueNames = TRUE;
    RegisterArgWords(sWork.strbuf, args, sWork.wordSet);
    sWork.trueNames = FALSE;
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

void func_ov167_021d57b0(StrBuf *strbuf, u16 message, const u32 *args) {
    BtlString_BuildTrainerStringBody(strbuf, message, args);
}

static void BtlString_BuildTrainerStringBody(StrBuf *strbuf, u16 message, const u32 *args) {
    GFL_MsgDataLoadStrbuf(sWork.msgData[6], message, sWork.strbuf);
    RegisterArgWords(sWork.strbuf, args, sWork.wordSet);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

void BtlString_FormatLevelUpGains(StrBuf *strbuf, int hp, int attack, int defense, int spAttack, int spDefense,
                                  int speed) {
    GFL_MsgDataLoadStrbuf(sWork.msgData[3], 0x10, sWork.strbuf);
    WordSetNumber(sWork.wordSet, 0, hp, 2, 0, 1);
    WordSetNumber(sWork.wordSet, 1, attack, 2, 0, 1);
    WordSetNumber(sWork.wordSet, 2, defense, 2, 0, 1);
    WordSetNumber(sWork.wordSet, 3, spAttack, 2, 0, 1);
    WordSetNumber(sWork.wordSet, 4, spDefense, 2, 0, 1);
    WordSetNumber(sWork.wordSet, 5, speed, 2, 0, 1);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

void BtlString_FormatLevelUpStats(StrBuf *strbuf, int hp, int attack, int defense, int spAttack, int spDefense,
                                  int speed) {
    GFL_MsgDataLoadStrbuf(sWork.msgData[3], 0x11, sWork.strbuf);
    WordSetNumber(sWork.wordSet, 0, hp, 3, 0, 1);
    WordSetNumber(sWork.wordSet, 1, attack, 3, 0, 1);
    WordSetNumber(sWork.wordSet, 2, defense, 3, 0, 1);
    WordSetNumber(sWork.wordSet, 3, spAttack, 3, 0, 1);
    WordSetNumber(sWork.wordSet, 4, spDefense, 3, 0, 1);
    WordSetNumber(sWork.wordSet, 5, speed, 3, 0, 1);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

void BtlString_FormatMovieScriptMsg(StrBuf *strbuf, u32 message) {
    GFL_MsgDataLoadStrbuf(sWork.msgData[9], message, sWork.strbuf);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

void BtlString_FormatMovieMsg(StrBuf *strbuf, u16 message) {
    GFL_MsgDataLoadStrbuf(sWork.msgData[10], message, sWork.strbuf);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

void BtlString_SetStudioWords(void) {
    PlayerInfo *player = GetPlayerInfo(sWork.mainModule);

    SetMonSpeciesName(0, 0);
    copyVarForText(sWork.wordSet, 1, player);
}
