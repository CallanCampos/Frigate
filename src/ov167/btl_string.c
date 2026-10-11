// The battle's message strings: loads a message, fills its word set placeholders with the names and numbers its
// arguments give, and formats it, for btlv_core.c and btlv_scu.c. The name is a guess from Black and White's
// BTL_STR module, which does the same.

#include "types.h"
#include "constants/arc.h"
#include "battle/btl_main.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_setup.h"
#include "battle/btl_string.h"
#include "battle/trainer_data.h"
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
    // The arguments of func_ov167_021d4ec0
    u32 args[9];
    HeapID heapId;
    u32 clientId;
    u16 unk68;
    // Set while func_ov167_021d5778 formats, which names the Pokémon behind an Illusion
    u16 trueNames;
} BtlStringWork;

static void func_ov167_021d4d7c(u8 monId, u32 index);
static void func_ov167_021d4da0(u8 monId, u32 index);
static void func_ov167_021d4dc4(u8 monId, u8 index);
static void func_ov167_021d4de8(WordSet *wordSet, u8 index, u8 clientId);
static void func_ov167_021d4e4c(WordSet *wordSet, u8 index, u8 clientId);
static void func_ov167_021d4f64(StrBuf *strbuf, u16 message, const u32 *args);
static const u16 *func_ov167_021d4fbc(const u16 *str, u32 *category, u16 *index, u8 *param);
static u8 func_ov167_021d5034(const StrBuf *strbuf);
static void func_ov167_021d507c(StrBuf *strbuf, const u32 *args, WordSet *wordSet);
static void func_ov167_021d5268(StrBuf *strbuf, u16 message, const u32 *args);
static void func_ov167_021d52a0(StrBuf *strbuf, u16 message, const u32 *args);
static void func_ov167_021d531c(StrBuf *strbuf, u16 message, const u32 *args);
static void func_ov167_021d5440(StrBuf *strbuf, u16 message, const u32 *args);
static void func_ov167_021d54fc(StrBuf *strbuf, u16 message, const u32 *args);
static void func_ov167_021d55a4(StrBuf *strbuf, u16 message, const u32 *args);
static void func_ov167_021d5630(StrBuf *strbuf, u16 message, const u32 *args);
static void func_ov167_021d5778(StrBuf *strbuf, u16 message, const u32 *args);
static void func_ov167_021d57b8(StrBuf *strbuf, u16 message, const u32 *args);

// The message files, in the order of BtlStringWork's msgData
static const u16 sMsgFiles[] = { 0x13, 0x12, 0x10, 0x14, 0x176, 0x191, 0x1b8, 0x178, 0x197 };

// The message files of the studio's movies
static const u32 sMovieMsgFiles[] = { 0x137, 0x138, 0x136, 0x135, 0x134 };

// The standard messages that have a variant for the other side, at the next ID
static const u16 sSideMessages[] = {
    0x94, 0x96, 0x98, 0x9a, 0x9c, 0x9e, 0xa0, 0xa2, 0xb0, 0x84, 0x86, 0x80, 0x82, 0x7c,
    0x7e, 0x88, 0x8a, 0x8c, 0x8e, 0x90, 0x92, 0xa4, 0xa6, 0xa8, 0xaa, 0xac, 0xae,
};

static const BtlStringHandler sHandlers[] = {
    { 0x1b, func_ov167_021d54fc },
    { 0x5a, func_ov167_021d54fc },
    { 0x99, func_ov167_021d55a4 },
    { 0xae, func_ov167_021d55a4 },
    { 0x3aa, func_ov167_021d5440 },
    { 0x21, func_ov167_021d5630 },
    { 0x414, func_ov167_021d52a0 },
    { 0x417, func_ov167_021d52a0 },
    { 0x14d, func_ov167_021d52a0 },
    { 0x471, func_ov167_021d52a0 },
    { 0x9, func_ov167_021d52a0 },
    { 0xc, func_ov167_021d52a0 },
    { 0x12, func_ov167_021d52a0 },
    { 0x15, func_ov167_021d52a0 },
};

static BtlStringWork sWork;

// Which variant of a message names a Pokémon: 0 for the player's side, 1 for a wild Pokémon, 2 for a foe's
static inline int GetMonVariant(u8 monId) {
    if (sWork.unk68 == 0
        && AreClientsOnOppositeSides(sWork.mainModule, sWork.clientId, func_ov167_0219c648(monId))) {
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
        BtlScriptedRules *rules = func_ov167_0219e39c(mainModule);
        if (getTrainerGender(func_ov167_0219bf68(mainModule)) == 0) {
            male = TRUE;
        }
        sWork.msgData[9] =
            GFL_MsgSysLoadData(FALSE, 2, GetMovieScriptMsgFile(func_ov167_0219c9b0(mainModule), male), heapId);
        if (mode == 1) {
            sWork.msgData[10] = GFL_MsgSysLoadData(FALSE, 2, sMovieMsgFiles[rules->unk08], heapId);
        }
    }
}

void func_ov167_021d4c64(BtlMainModule *mainModule, u8 clientId, const BtlPokeCon *pokeCon, HeapID heapId) {
    u32 mode;
    int i;
    u32 fileId;

    mode = func_ov167_0219c988(mainModule);
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

void func_ov167_021d4d50(void) {
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
static void func_ov167_021d4d7c(u8 monId, u32 index) {
    loadPokemonNicknameToStrbuf(sWork.wordSet, index,
                                func_ov167_021bb064(GetPokeParamConst(sWork.pokeCon, monId)));
}

// A Pokémon's own nickname
static void func_ov167_021d4da0(u8 monId, u32 index) {
    loadPokemonNicknameToStrbuf(sWork.wordSet, index, GetSrcData(GetPokeParamConst(sWork.pokeCon, monId)));
}

static void func_ov167_021d4dc4(u8 monId, u8 index) {
    setPartyPokemonSpeciesNameToStrbuf(sWork.wordSet, index,
                                       func_ov167_021bb064(GetPokeParamConst(sWork.pokeCon, monId)));
}

// A client's trainer class
static void func_ov167_021d4de8(WordSet *wordSet, u8 index, u8 clientId) {
    u32 trainerClass = func_ov167_0219d938(sWork.mainModule, clientId);
    StrBuf *name = func_ov167_0219e310(sWork.mainModule)->trainerNames[clientId];

    if (name != NULL && GFL_StrBufIsValid(name)) {
        func_0202437c(wordSet, index, name, TrainerClass_GetSex(trainerClass), 1, 2);
    } else {
        loadTrainerTypeText(wordSet, index, (u8)trainerClass);
    }
}

// A client's trainer name
static void func_ov167_021d4e4c(WordSet *wordSet, u8 index, u8 clientId) {
    u32 trainerClass;

    if (clientId != 4) {
        if (func_ov167_0219d888(sWork.mainModule, clientId)) {
            StrBuf *name = func_ov167_0219d8c4(sWork.mainModule, clientId, &trainerClass);
            func_0202437c(wordSet, index, name, TrainerClass_GetSex(trainerClass), 1, 2);
        } else {
            copyVarForText(wordSet, index, func_ov167_0219d97c(sWork.mainModule, clientId));
        }
    } else {
        copyVarForText(wordSet, index, func_ov167_0219d998(sWork.mainModule));
    }
}

void func_ov167_021d4ec0(StrBuf *strbuf, u16 message, u32 count, ...) {
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
    func_ov167_021d4f1c(strbuf, message, sWork.args);
}

void func_ov167_021d4f1c(StrBuf *strbuf, u16 message, const u32 *args) {
    u32 i;

    for (i = 0; i < NELEMS(sSideMessages); i++) {
        if (message == sSideMessages[i]) {
            if (!func_ov167_0219c43c(sWork.mainModule, args[0])) {
                message++;
            }
            break;
        }
    }
    func_ov167_021d4f64(strbuf, message, args);
}

static void func_ov167_021d4f64(StrBuf *strbuf, u16 message, const u32 *args) {
    GFL_MsgDataLoadStrbuf(sWork.msgData[0], message, sWork.strbuf);
    func_ov167_021d507c(sWork.strbuf, args, sWork.wordSet);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

void func_ov167_021d4f90(StrBuf *strbuf, u16 message, const u32 *args) {
    u32 i;

    for (i = 0; i < NELEMS(sHandlers); i++) {
        if (message == sHandlers[i].message) {
            sHandlers[i].func(strbuf, message, args);
            return;
        }
    }
    func_ov167_021d5268(strbuf, message, args);
}

// Finds the next word set command in a string, and returns the string after it, or NULL at its end
static const u16 *func_ov167_021d4fbc(const u16 *str, u32 *category, u16 *index, u8 *param) {
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
static u8 func_ov167_021d5034(const StrBuf *strbuf) {
    u8 param;
    u16 index;
    u32 category;
    u8 count;
    const u16 *str;

    str = GFL_StrBufGetStringPtr(strbuf);
    count = 0;
    while (str != NULL) {
        str = func_ov167_021d4fbc(str, &category, &index, &param);
        if (str != NULL && category == 1) {
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
static void func_ov167_021d507c(StrBuf *strbuf, const u32 *args, WordSet *wordSet) {
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
                if (category == 1 && GFL_StrCmdGetCommandIndex(str) == 14) {
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
                if (category == 2) {
                    // The command's index is the number's digits less one
                    u8 digits = GFL_StrCmdGetCommandIndex(str) + 1;
                    WordSetNumber(wordSet, param, args[arg], digits, 0, 1);
                } else {
                    switch (GFL_StrCmdGetCommandIndex(str)) {
                    case 14:
                        func_ov167_021d4de8(wordSet, param, args[arg]);
                        break;
                    case 0:
                        func_ov167_021d4e4c(wordSet, param, args[arg]);
                        break;
                    case 2:
                        if (sWork.trueNames == FALSE) {
                            func_ov167_021d4d7c(args[arg], param);
                        } else {
                            func_ov167_021d4da0(args[arg], param);
                        }
                        break;
                    case 12:
                        func_ov167_021d4da0(args[arg], param);
                        break;
                    case 1:
                        func_ov167_021d4dc4(args[arg], param);
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

static void func_ov167_021d5268(StrBuf *strbuf, u16 message, const u32 *args) {
    GFL_MsgDataLoadStrbuf(sWork.msgData[1], message, sWork.strbuf);
    if (func_ov167_021d5034(sWork.strbuf) == 2) {
        func_ov167_021d531c(strbuf, message, args);
    } else {
        func_ov167_021d52a0(strbuf, message, args);
    }
}

// A message about one Pokémon, args[0]
static void func_ov167_021d52a0(StrBuf *strbuf, u16 message, const u32 *args) {
    GFL_MsgDataLoadStrbuf(sWork.msgData[1], (u16)(message + GetMonVariant(args[0])), sWork.strbuf);
    func_ov167_021d507c(sWork.strbuf, args, sWork.wordSet);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

// A message about two Pokémon, args[0] and args[1], in seven variants: three for the player's Pokémon by the other's
// variant, and two each for a wild or a foe's Pokémon, by whether the other is of the same kind
static void func_ov167_021d531c(StrBuf *strbuf, u16 message, const u32 *args) {
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
    func_ov167_021d507c(sWork.strbuf, args, sWork.wordSet);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

// An item raised a stat: args are the Pokémon, the item, the stat and how many stages
static void func_ov167_021d5440(StrBuf *strbuf, u16 message, const u32 *args) {
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
    func_ov167_021d4d7c(args[0], 0);
    loadItemNameToStrbuf(sWork.wordSet, 1, args[1]);
    // The stat's message, before the variant is added: only a variable of its own puts the variant second in the add
    base = message + stat * 3;
    GFL_MsgDataLoadStrbuf(sWork.msgData[1], (u16)(base + variant), sWork.strbuf);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

// A stat rose or fell: args are the Pokémon, the stat and how many stages
static void func_ov167_021d54fc(StrBuf *strbuf, u16 message, const u32 *args) {
    u8 stat = args[1] - 1;
    int degree = 0;

    if ((int)args[2] >= 3) {
        degree = 2;
    }
    if (args[2] == 2) {
        degree = 1;
    }
    message += degree * 21;
    func_ov167_021d4d7c(args[0], 0);
    GFL_MsgDataLoadStrbuf(sWork.msgData[1], (u16)(message + stat * 3 + GetMonVariant(args[0])), sWork.strbuf);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

// A stat can't go any higher or lower: args are the Pokémon and the stat
static void func_ov167_021d55a4(StrBuf *strbuf, u16 message, const u32 *args) {
    u8 stat = args[1] - 1;

    func_ov167_021d4d7c(args[0], 0);
    GFL_MsgDataLoadStrbuf(sWork.msgData[1], (u16)(message + stat * 3 + GetMonVariant(args[0])), sWork.strbuf);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

// Picks one of three messages by whose client args[0] is
static void func_ov167_021d5630(StrBuf *strbuf, u16 message, const u32 *args) {
    u8 clientId = args[0];

    if (clientId != GetPlayerClientID(sWork.mainModule)) {
        if (func_ov167_0219d888(sWork.mainModule, clientId)) {
            message = 0x23;
        } else {
            message = 0x22;
        }
    } else {
        message = 0x21;
    }
    GFL_MsgDataLoadStrbuf(sWork.msgData[0], message, sWork.strbuf);
    func_ov167_021d507c(sWork.strbuf, args, sWork.wordSet);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

void func_ov167_021d5684(StrBuf *strbuf, u8 monId, u16 move) {
    int variant = GetMonVariant(monId);

    func_ov167_021d4d7c(monId, 0);
    move *= 3;
    GFL_MsgDataLoadStrbuf(sWork.msgData[2], (u16)(move + variant), sWork.strbuf);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

void func_ov167_021d5700(StrBuf *strbuf, u16 arg1, u32 arg2) {
    if (arg1 >= 0x800) {
        BtlSetup *setup = func_ov167_0219e310(sWork.mainModule);
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

void func_ov167_021d575c(StrBuf *strbuf, u16 message) {
    GFL_MsgDataLoadStrbuf(sWork.msgData[3], message, strbuf);
}

void func_ov167_021d5770(StrBuf *strbuf, u16 message, const u32 *args) {
    func_ov167_021d5778(strbuf, message, args);
}

static void func_ov167_021d5778(StrBuf *strbuf, u16 message, const u32 *args) {
    GFL_MsgDataLoadStrbuf(sWork.msgData[5], message, sWork.strbuf);
    sWork.trueNames = TRUE;
    func_ov167_021d507c(sWork.strbuf, args, sWork.wordSet);
    sWork.trueNames = FALSE;
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

void func_ov167_021d57b0(StrBuf *strbuf, u16 message, const u32 *args) {
    func_ov167_021d57b8(strbuf, message, args);
}

static void func_ov167_021d57b8(StrBuf *strbuf, u16 message, const u32 *args) {
    GFL_MsgDataLoadStrbuf(sWork.msgData[6], message, sWork.strbuf);
    func_ov167_021d507c(sWork.strbuf, args, sWork.wordSet);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

void func_ov167_021d57e4(StrBuf *strbuf, int hp, int attack, int defense, int spAttack, int spDefense, int speed) {
    GFL_MsgDataLoadStrbuf(sWork.msgData[3], 0x10, sWork.strbuf);
    WordSetNumber(sWork.wordSet, 0, hp, 2, 0, 1);
    WordSetNumber(sWork.wordSet, 1, attack, 2, 0, 1);
    WordSetNumber(sWork.wordSet, 2, defense, 2, 0, 1);
    WordSetNumber(sWork.wordSet, 3, spAttack, 2, 0, 1);
    WordSetNumber(sWork.wordSet, 4, spDefense, 2, 0, 1);
    WordSetNumber(sWork.wordSet, 5, speed, 2, 0, 1);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

void func_ov167_021d5874(StrBuf *strbuf, int hp, int attack, int defense, int spAttack, int spDefense, int speed) {
    GFL_MsgDataLoadStrbuf(sWork.msgData[3], 0x11, sWork.strbuf);
    WordSetNumber(sWork.wordSet, 0, hp, 3, 0, 1);
    WordSetNumber(sWork.wordSet, 1, attack, 3, 0, 1);
    WordSetNumber(sWork.wordSet, 2, defense, 3, 0, 1);
    WordSetNumber(sWork.wordSet, 3, spAttack, 3, 0, 1);
    WordSetNumber(sWork.wordSet, 4, spDefense, 3, 0, 1);
    WordSetNumber(sWork.wordSet, 5, speed, 3, 0, 1);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

void func_ov167_021d5904(StrBuf *strbuf, u32 message) {
    GFL_MsgDataLoadStrbuf(sWork.msgData[9], message, sWork.strbuf);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

void func_ov167_021d5924(StrBuf *strbuf, u16 message) {
    GFL_MsgDataLoadStrbuf(sWork.msgData[10], message, sWork.strbuf);
    GFL_WordSetFormatStrbuf(sWork.wordSet, strbuf, sWork.strbuf);
}

void func_ov167_021d5944(void) {
    PlayerInfo *player = func_ov167_0219bf68(sWork.mainModule);

    func_ov167_021d4dc4(0, 0);
    copyVarForText(sWork.wordSet, 1, player);
}
