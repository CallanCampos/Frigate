#include "constants/arc.h"
#include "constants/met_locations.h"
#include "constants/pokemon.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "p_status_local.h"
#include "pml/met_data.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "pml/species_names.h"
#include "system/app_menu_common.h"
#include "system/game_data.h"

// The summary screen's info page: the Pokédex number, species, original trainer, ID and experience, and the trainer
// memo, which says where and when the Pokémon was met and what its nature and best individual value say about it. The
// names of the fields and functions are guesses

#define INFO_WINDOW_COUNT 10
// What a regional Pokédex number is for a species that isn't in it
#define DEX_NUMBER_NONE 999
#define LEVEL_MAX 100
// The width in pixels of the experience bar
#define EXP_BAR_WIDTH 64

// The message files of the memo and of the place names
enum {
    INFO_MSG_MEMO,
    INFO_MSG_PLACES_UNOVA,
    INFO_MSG_PLACES_SPECIAL,
    INFO_MSG_PLACES_EVENT,
    INFO_MSG_PLACES_EXTERNAL,
    INFO_MSG_COUNT,
};

typedef struct {
    u8 x;
    u8 y;
    u8 width;
    u8 height;
} InfoWindowSetup;

struct PStaInfoWork {
    BOOL isPrinted;
    BOOL isShown;
    u16 *regionalDex;
    BmpWin *windows[INFO_WINDOW_COUNT];
    BmpWin *memoWindow;
    MsgData *msgData[INFO_MSG_COUNT];
    PStaScreen screens[4];
};

static void PStaInfo_PrintInfo(PStatusWork *wk, PStaInfoWork *info, BoxPkm *pkm);
static void PStaInfo_PrintMemo(PStatusWork *wk, PStaInfoWork *info, BoxPkm *pkm);
static StrBuf *PStaInfo_GetPlaceName(PStatusWork *wk, PStaInfoWork *info, u32 location);

static const PStaWindowSetup sWindows[INFO_WINDOW_COUNT] = {
    { 2, 1, 16, 2 },  { 2, 3, 16, 2 },  { 2, 5, 8, 2 },   { 2, 7, 16, 2 },  { 2, 9, 16, 2 },
    { 2, 11, 17, 2 }, { 10, 13, 8, 2 }, { 2, 15, 16, 2 }, { 6, 17, 12, 2 }, { 10, 19, 8, 1 },
};

PStaInfoWork *PStaInfo_Create(PStatusWork *wk) {
    PStaInfoWork *info = GFL_HeapAllocate(wk->heapId, sizeof(PStaInfoWork), FALSE, "p_sta_info.c", 197);

    info->isPrinted = FALSE;
    info->isShown = FALSE;
    info->regionalDex = PML_PersonalLoadRegionalDexTable(wk->heapId, 0);
    return info;
}

void PStaInfo_Free(PStatusWork *wk, PStaInfoWork *info) {
    GFL_HeapFree(info->regionalDex);
    GFL_HeapFree(info);
}

void PStaInfo_Main(PStatusWork *wk, PStaInfoWork *info) {
}

void PStaInfo_LoadResources(PStatusWork *wk, PStaInfoWork *info, ArcTool *arc) {
    info->screens[0].file = GFL_G2DIOReadNSCRArc(arc, 65, FALSE, &info->screens[0].screen, wk->heapId);
    info->screens[1].file = GFL_G2DIOReadNSCRArc(arc, 74, FALSE, &info->screens[1].screen, wk->heapId);
    info->screens[2].file = GFL_G2DIOReadNSCRArc(arc, 66, FALSE, &info->screens[2].screen, wk->heapId);
    info->screens[3].file = GFL_G2DIOReadNSCRArc(arc, 67, FALSE, &info->screens[3].screen, wk->heapId);
    info->msgData[INFO_MSG_MEMO] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MEMO, wk->heapId);
    info->msgData[INFO_MSG_PLACES_UNOVA] =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, PLACE_FILE_UNOVA, wk->heapId);
    info->msgData[INFO_MSG_PLACES_SPECIAL] =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, PLACE_FILE_SPECIAL, wk->heapId);
    info->msgData[INFO_MSG_PLACES_EVENT] =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, PLACE_FILE_EVENT, wk->heapId);
    info->msgData[INFO_MSG_PLACES_EXTERNAL] =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, PLACE_FILE_EXTERNAL, wk->heapId);
}

void PStaInfo_FreeResources(PStatusWork *wk, PStaInfoWork *info) {
    if (info->isShown == TRUE) {
        PStaInfo_Unload(wk, info);
    }
    GFL_MsgDataFree(info->msgData[INFO_MSG_MEMO]);
    GFL_MsgDataFree(info->msgData[INFO_MSG_PLACES_UNOVA]);
    GFL_MsgDataFree(info->msgData[INFO_MSG_PLACES_SPECIAL]);
    GFL_MsgDataFree(info->msgData[INFO_MSG_PLACES_EVENT]);
    GFL_MsgDataFree(info->msgData[INFO_MSG_PLACES_EXTERNAL]);
    GFL_HeapFree(info->screens[0].file);
    GFL_HeapFree(info->screens[1].file);
    GFL_HeapFree(info->screens[2].file);
    GFL_HeapFree(info->screens[3].file);
}

void PStaInfo_Load(PStatusWork *wk, PStaInfoWork *info) {
    BoxPkm *pkm = PStatus_GetBoxPkm(wk);
    u8 i;

    for (i = 0; i < INFO_WINDOW_COUNT; i++) {
        info->windows[i] =
            BmpWin_CreateDynamic(1, sWindows[i].x, sWindows[i].y, sWindows[i].width, sWindows[i].height, 14, 1);
    }
    info->memoWindow = BmpWin_CreateDynamic(4, 4, 5, 24, 20, 14, 1);
    PStaInfo_PrintInfo(wk, info, pkm);
    PStaInfo_PrintMemo(wk, info, pkm);
    info->isShown = TRUE;
}

void PStaInfo_Draw(PStatusWork *wk, PStaInfoWork *info) {
    BoxPkm *pkm = PStatus_GetBoxPkm(wk);
    u8 i;
    BmpWin *window;
    u32 type1;
    u32 type2;
    ClActorPos pos;
    NNSG2dImageProxy proxy1;
    NNSG2dImageProxy proxy2;

    if (wk->isEgg == FALSE) {
        GFL_BGSysLoadScrArea(2, 0, 0, 32, 24, info->screens[0].screen->rawData, 0, 0, 32, 32);
    } else {
        GFL_BGSysLoadScrArea(2, 0, 0, 32, 24, info->screens[1].screen->rawData, 0, 0, 32, 32);
    }
    GFL_BGSysQueueScrLoad(2);
    GFL_BGSysBufferScrDefault(6, info->screens[2].screen->rawData, info->screens[2].screen->size);
    GFL_BGSysQueueScrLoad(6);
    GFL_BGSysLoadScrArea(5, 0, 0, 32, 3, info->screens[3].screen->rawData, 0, 0, 32, 32);
    GFL_BGSysQueueScrLoad(5);
    for (i = 0; i < INFO_WINDOW_COUNT; i++) {
        window = info->windows[i];
        BmpWin_FlushChar(window);
        BmpWin_FlushMap(window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    }
    window = info->memoWindow;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));

    if (wk->isEgg == FALSE) {
        type1 = PML_PkmGetParam(pkm, PKM_PARAM_TYPE1, NULL);
        type2 = PML_PkmGetParam(pkm, PKM_PARAM_TYPE2, NULL);
        func_0204bb58(wk->typeIconChars[type1], &proxy1);
        func_0204c3e4(wk->typeIcons[0], &proxy1);
        func_0204c378(wk->typeIcons[0], func_0202d7e8(type1), 1);
        pos.x = 96;
        pos.y = 48;
        func_0204c140(wk->typeIcons[0], &pos, 0);
        func_0204c124(wk->typeIcons[0], TRUE);
        func_0204c468(wk->typeIcons[0], 1);
        if (type1 != type2) {
            func_0204bb58(wk->typeIconChars[type2], &proxy2);
            func_0204c3e4(wk->typeIcons[1], &proxy2);
            func_0204c378(wk->typeIcons[1], func_0202d7e8(type2), 1);
            pos.x = 130;
            pos.y = 48;
            func_0204c140(wk->typeIcons[1], &pos, 0);
            func_0204c124(wk->typeIcons[1], TRUE);
            func_0204c468(wk->typeIcons[1], 1);
        } else {
            func_0204c124(wk->typeIcons[1], FALSE);
        }
    } else {
        func_0204c124(wk->typeIcons[0], FALSE);
        func_0204c124(wk->typeIcons[1], FALSE);
    }
    func_0204c488(wk->buttons[PSTA_BUTTON_INFO], 3);
}

void PStaInfo_Unload(PStatusWork *wk, PStaInfoWork *info) {
    u8 i;

    for (i = 0; i < INFO_WINDOW_COUNT; i++) {
        BmpWin_Free(info->windows[i]);
    }
    BmpWin_Free(info->memoWindow);
    info->isShown = FALSE;
}

void PStaInfo_Clear(PStatusWork *wk, PStaInfoWork *info) {
    GFL_BGSysFillScrArea(1, 0, 0, 0, 19, 21, 16);
    GFL_BGSysQueueScrLoad(1);
    GFL_BGSysFillScrAsync(4, 0);
    GFL_BGSysQueueScrLoad(4);
    func_0204c124(wk->typeIcons[0], FALSE);
    func_0204c124(wk->typeIcons[1], FALSE);
    func_0204c488(wk->buttons[PSTA_BUTTON_INFO], 0);
}

static void PStaInfo_PrintInfo(PStatusWork *wk, PStaInfoWork *info, BoxPkm *pkm) {
    WordSet *wordSet;
    u32 dexNumber;
    u16 color;
    StrBuf *str;
    StrBuf *name;
    WordSet *nameWordSet;
    u32 sex;
    u16 id;
    u32 level;
    u32 exp;
    u32 nextExp;
    u32 levelExp;
    u32 toNext;
    u32 gained;
    u32 width;

    if (wk->isEgg == FALSE) {
        PStatus_PrintToWindow(wk, info->windows[0], 8, 1, 1, PRINT_COLOR(15, 2, 0));
        wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
        dexNumber = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
        color = PRINT_COLOR(1, 2, 0);
        if (wk->param->isNationalDex == FALSE) {
            dexNumber = info->regionalDex[dexNumber];
        }
        if (PML_PkmIsRare(pkm) == TRUE) {
            color = PRINT_COLOR(3, 4, 0);
        }
        if (dexNumber == DEX_NUMBER_NONE) {
            PStatus_PrintFormattedToWindow(wk, info->windows[0], wordSet, 22, 65, 1, color);
        } else {
            WordSetNumber(wordSet, 0, dexNumber, 3, 2, 1);
            PStatus_PrintFormattedToWindow(wk, info->windows[0], wordSet, 9, 65, 1, color);
        }
        GFL_WordSetSystemFree(wordSet);

        PStatus_PrintToWindow(wk, info->windows[1], 10, 1, 1, PRINT_COLOR(15, 2, 0));
        if (wk->isEgg == FALSE) {
            str = GFL_MsgDataLoadStrbufNew(g_PMLSpeciesNamesResident, PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL));
        } else {
            str = GFL_MsgDataLoadStrbufNew(g_PMLSpeciesNamesResident, SPECIES_EGG);
        }
        func_02021c7c(wk->printQueue, BmpWin_GetBitmap(info->windows[1]), 65, 1, str, wk->font, PRINT_COLOR(1, 2, 0));
        GFL_StrBufFree(str);

        PStatus_PrintToWindow(wk, info->windows[2], 12, 1, 1, PRINT_COLOR(15, 2, 0));
        PStatus_PrintToWindow(wk, info->windows[3], 13, 1, 1, PRINT_COLOR(15, 2, 0));
        name = GFL_StrBufCreate(32, wk->heapId);
        nameWordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
        color = PRINT_COLOR(1, 2, 0);
        sex = PML_PkmGetParam(pkm, PKM_PARAM_OT_GENDER, name);
        if (sex == GENDER_MALE) {
            color = PRINT_COLOR(5, 6, 0);
        } else if (sex == GENDER_FEMALE) {
            color = PRINT_COLOR(3, 4, 0);
        }
        PML_PkmGetParam(pkm, PKM_PARAM_OT_NAME, name);
        func_0202437c(nameWordSet, 0, name, 0, 1, 2);
        PStatus_PrintFormattedToWindow(wk, info->windows[3], nameWordSet, 14, 65, 1, color);
        GFL_WordSetSystemFree(nameWordSet);
        GFL_StrBufFree(name);

        PStatus_PrintToWindow(wk, info->windows[4], 15, 1, 1, PRINT_COLOR(15, 2, 0));
        wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
        id = PML_PkmGetParam(pkm, PKM_PARAM_ID, NULL);
        WordSetNumber(wordSet, 0, id, 5, 2, 1);
        PStatus_PrintFormattedToWindow(wk, info->windows[4], wordSet, 16, 65, 1, PRINT_COLOR(1, 2, 0));
        GFL_WordSetSystemFree(wordSet);

        level = PML_PkmGetParam(pkm, PKM_PARAM_LEVEL, NULL);
        exp = PML_PkmGetParam(pkm, PKM_PARAM_EXP, NULL);
        nextExp = PML_UtilGetPkmLvExp(PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL),
                                      PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL), level + 1);
        levelExp = PML_UtilGetPkmLvExp(PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL),
                                       PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL), level);
        toNext = nextExp - exp;
        if (level == LEVEL_MAX) {
            toNext = 0;
        }
        PStatus_PrintToWindow(wk, info->windows[5], 17, 1, 1, PRINT_COLOR(15, 2, 0));
        wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
        WordSetNumber(wordSet, 0, exp, 7, 0, 1);
        PStatus_PrintFormattedToWindow(wk, info->windows[6], wordSet, 18, 1, 1, PRINT_COLOR(1, 2, 0));
        GFL_WordSetSystemFree(wordSet);
        PStatus_PrintToWindow(wk, info->windows[7], 19, 1, 1, PRINT_COLOR(15, 2, 0));
        PStatus_PrintToWindow(wk, info->windows[8], 20, 1, 1, PRINT_COLOR(1, 2, 0));
        wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
        WordSetNumber(wordSet, 0, toNext, 6, 0, 1);
        PStatus_PrintFormattedToWindow(wk, info->windows[8], wordSet, 21, 33, 1, PRINT_COLOR(1, 2, 0));
        GFL_WordSetSystemFree(wordSet);
        if (level != LEVEL_MAX) {
            gained = nextExp - levelExp - toNext;
            width = gained * EXP_BAR_WIDTH / (nextExp - levelExp);
            if (gained != 0 && width == 0) {
                width = 1;
            }
            if (width == EXP_BAR_WIDTH) {
                width--;
            }
            GFL_BitmapFillArea(BmpWin_GetBitmap(info->windows[9]), 0, 3, width, 3, 5);
        }
    }
    info->isPrinted = TRUE;
}

static void PStaInfo_PrintMemo(PStatusWork *wk, PStaInfoWork *info, BoxPkm *pkm) {
    u32 y = 0;
    BOOL foreignOT;
    BOOL isEgg = PML_PkmGetParam(pkm, PKM_PARAM_IS_EGG, NULL);
    u32 nature = PML_PkmGetParam(pkm, PKM_PARAM_NATURE, NULL);
    u32 happiness = PML_PkmGetParam(pkm, PKM_PARAM_HAPPINESS, NULL);
    u32 eggYear = PML_PkmGetParam(pkm, PKM_PARAM_EGG_YEAR, NULL);
    u32 eggMonth = PML_PkmGetParam(pkm, PKM_PARAM_EGG_MONTH, NULL);
    u32 eggDay = PML_PkmGetParam(pkm, PKM_PARAM_EGG_DAY, NULL);
    u32 metYear = PML_PkmGetParam(pkm, PKM_PARAM_MET_YEAR, NULL);
    u32 metMonth = PML_PkmGetParam(pkm, PKM_PARAM_MET_MONTH, NULL);
    u32 metDay = PML_PkmGetParam(pkm, PKM_PARAM_MET_DAY, NULL);
    u32 metLevel = PML_PkmGetParam(pkm, PKM_PARAM_MET_LEVEL, NULL);
    u32 fateful = PML_PkmGetParam(pkm, PKM_PARAM_FATEFUL_ENCOUNTER, NULL);
    u32 originGame = PML_PkmGetParam(pkm, PKM_PARAM_ORIGIN_GAME, NULL);
    BOOL isN = PML_PkmGetParam(pkm, PKM_PARAM_N_POKEMON, NULL);
    u32 pid = PML_PkmGetParam(pkm, PKM_PARAM_PID, NULL);
    u32 eggLocation = PML_PkmGetParam(pkm, PKM_PARAM_EGG_LOCATION, NULL);
    u32 metLocation = PML_PkmGetParam(pkm, PKM_PARAM_MET_LOCATION, NULL);
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(wk->param->gameData);
    BOOL transfer0 = special_transfers(pkm, 0, playerInfo);
    BOOL transfer1 = special_transfers(pkm, 1, playerInfo);
    BOOL transfer2 = special_transfers(pkm, 2, playerInfo);
    BOOL transfer3 = special_transfers(pkm, 3, playerInfo);
    BOOL transferred = transfer0 | transfer1 | transfer2 | transfer3;
    StrBuf *str;
    StrBuf *buf;
    WordSet *wordSet;
    u16 msg;
    u16 originMsg;
    StrBuf *eggPlace;
    StrBuf *metPlace;

    foreignOT = PML_UtilCheckForeignOT(pkm, playerInfo);
    if (isEgg == FALSE) {
        str = GFL_MsgDataLoadStrbufNew(info->msgData[INFO_MSG_MEMO], nature);
        func_02021c7c(wk->printQueue, BmpWin_GetBitmap(info->memoWindow), 1, 1, str, wk->font, PRINT_COLOR(1, 2, 0));
        GFL_StrBufFree(str);
        y = 16;
    }
    buf = GFL_StrBufCreate(256, wk->heapId);
    wordSet = GFL_WordSetSystemCreate(10, 32, wk->heapId);
    if (isEgg == FALSE) {
        if (metLocation == LOCATION_POKE_TRANSFER) {
            originMsg = 0x29;
            msg = 0x2b;
            if (fateful == TRUE) {
                originMsg = 0x2d;
                msg = 0x2f;
            }
            switch (originGame) {
            case VERSION_SAPPHIRE:
            case VERSION_RUBY:
            case VERSION_EMERALD:
                metLocation = LOCATION_HOENN;
                break;
            case VERSION_FIRERED:
            case VERSION_LEAFGREEN:
                metLocation = LOCATION_KANTO;
                break;
            case VERSION_HEARTGOLD:
            case VERSION_SOULSILVER:
                metLocation = LOCATION_JOHTO;
                msg = originMsg;
                break;
            case VERSION_DIAMOND:
            case VERSION_PEARL:
            case VERSION_PLATINUM:
                metLocation = LOCATION_SINNOH;
                msg = originMsg;
                break;
            case VERSION_COLOSSEUM:
                metLocation = LOCATION_FARAWAY;
                break;
            default:
                // Sets the egg's place, though the memo names the met place. Both names are blank, so it shows the same
                eggLocation = LOCATION_UNKNOWN;
                msg = originMsg;
                break;
            }
        } else if (metLocation == LOCATION_DREAM_RADAR) {
            msg = 0x29;
        } else if (metLocation >= LOCATION_EVENT_CELEBI && metLocation <= LOCATION_EVENT_BEASTS_USED &&
                   transferred == TRUE) {
            switch (metLocation - LOCATION_EVENT_CELEBI) {
            case 0:
                msg = 0x31;
                break;
            case 1:
                msg = 0x32;
                break;
            case 2:
                msg = 0x33;
                break;
            case 3:
                msg = 0x34;
                break;
            }
        } else if (fateful == FALSE) {
            if (eggLocation == LOCATION_NONE) {
                if (isN) {
                    msg = 0x35;
                } else if (metLocation == LOCATION_IN_GAME_TRADE) {
                    msg = 0x1b;
                } else {
                    msg = 0x19;
                }
            } else if (eggLocation <= LOCATION_EXTERNAL_BASE) {
                msg = 0x1d;
            } else {
                msg = 0x1f;
            }
        } else {
            msg = 0x21;
            if (eggLocation != LOCATION_NONE) {
                msg = 0x23;
            }
        }
        if (foreignOT == FALSE && isN == FALSE &&
            (metLocation < LOCATION_EVENT_CELEBI || metLocation > LOCATION_EVENT_BEASTS_USED)) {
            msg++;
        }
    } else if (fateful == FALSE) {
        if (foreignOT == TRUE) {
            msg = 0x54;
        } else {
            msg = 0x55;
        }
    } else {
        msg = 0x56;
        if (foreignOT != TRUE) {
            msg = 0x57;
        }
    }
    eggPlace = PStaInfo_GetPlaceName(wk, info, eggLocation);
    metPlace = PStaInfo_GetPlaceName(wk, info, metLocation);
    {
        u8 best;

        WordSetNumber(wordSet, 0, metYear, 2, 2, 1);
        // The highest IV's zero is copied from best's stack slot, so best is cleared here and maxIV starts from it
        best = 0;
        WordSetNumber(wordSet, 1, metMonth, 2, 0, 1);
        WordSetNumber(wordSet, 2, metDay, 2, 0, 1);
        WordSetNumber(wordSet, 3, metLevel, 3, 0, 1);
        func_0202437c(wordSet, 4, metPlace, 0, 1, 2);
        WordSetNumber(wordSet, 5, eggYear, 2, 2, 1);
        WordSetNumber(wordSet, 6, eggMonth, 2, 0, 1);
        WordSetNumber(wordSet, 7, eggDay, 2, 0, 1);
        func_0202437c(wordSet, 8, eggPlace, 0, 1, 2);
        str = GFL_MsgDataLoadStrbufNew(info->msgData[INFO_MSG_MEMO], msg);
        GFL_WordSetFormatStrbuf(wordSet, buf, str);
        func_02021c7c(wk->printQueue, BmpWin_GetBitmap(info->memoWindow), 1, y + 1, buf, wk->font,
                      PRINT_COLOR(1, 2, 0));
        GFL_WordSetSystemFree(wordSet);
        y += func_0202284c(buf) * 16;
        GFL_StrBufFree(eggPlace);
        GFL_StrBufFree(metPlace);
        GFL_StrBufFree(str);
        GFL_StrBufFree(buf);
        if (isEgg == FALSE) {
            u8 maxIV = best;
            u32 params[6] = {
                PKM_PARAM_IV_HP,    PKM_PARAM_IV_ATTACK,    PKM_PARAM_IV_DEFENSE,
                PKM_PARAM_IV_SPEED, PKM_PARAM_IV_SP_ATTACK, PKM_PARAM_IV_SP_DEFENSE,
            };
            // The order in which the values are compared, by personality, so that a tie goes to a different stat
            u8 orders[6][6] = {
                { 0, 1, 2, 3, 4, 5 }, { 1, 2, 3, 4, 5, 0 }, { 2, 3, 4, 5, 0, 1 },
                { 3, 4, 5, 0, 1, 2 }, { 4, 5, 0, 1, 2, 3 }, { 5, 0, 1, 2, 3, 4 },
            };
            u8 i;
            u8 row = pid % 6;
            u32 iv;

            for (i = 0; i < 6; i++) {
                iv = PML_PkmGetParam(pkm, params[orders[row][i]], NULL);
                if (maxIV < iv) {
                    maxIV = iv;
                    best = orders[row][i];
                }
            }
            str = GFL_MsgDataLoadStrbufNew(info->msgData[INFO_MSG_MEMO], maxIV % 5 + (0x36 + best * 5));
            func_02021c7c(wk->printQueue, BmpWin_GetBitmap(info->memoWindow), 1, y + 1, str, wk->font,
                          PRINT_COLOR(1, 2, 0));
            GFL_StrBufFree(str);
        } else {
            u32 hatchMsg;

            if (happiness <= 5) {
                hatchMsg = 0x58;
            } else if (happiness <= 10) {
                hatchMsg = 0x59;
            } else if (happiness <= 40) {
                hatchMsg = 0x5a;
            } else {
                hatchMsg = 0x5b;
            }
            str = GFL_MsgDataLoadStrbufNew(info->msgData[INFO_MSG_MEMO], hatchMsg);
            func_02021c7c(wk->printQueue, BmpWin_GetBitmap(info->memoWindow), 1, y + 1, str, wk->font,
                          PRINT_COLOR(1, 2, 0));
            GFL_StrBufFree(str);
        }
    }
}

static StrBuf *PStaInfo_GetPlaceName(PStatusWork *wk, PStaInfoWork *info, u32 location) {
    u32 file = MetLocation_GetNameFile(location);
    u32 index = MetLocation_GetNameIndex(location);

    switch (file) {
    case PLACE_FILE_UNOVA:
        return GFL_MsgDataLoadStrbufNew(info->msgData[INFO_MSG_PLACES_UNOVA], index);
    case PLACE_FILE_SPECIAL:
        return GFL_MsgDataLoadStrbufNew(info->msgData[INFO_MSG_PLACES_SPECIAL], index);
    case PLACE_FILE_EVENT:
        return GFL_MsgDataLoadStrbufNew(info->msgData[INFO_MSG_PLACES_EVENT], index);
    case PLACE_FILE_EXTERNAL:
        return GFL_MsgDataLoadStrbufNew(info->msgData[INFO_MSG_PLACES_EXTERNAL], index);
    }
    return GFL_MsgDataLoadStrbufNew(info->msgData[INFO_MSG_PLACES_UNOVA], 0);
}
