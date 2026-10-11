#ifndef POKEBW2_OV214_WORLDTRADE_LOCAL_H
#define POKEBW2_OV214_WORLDTRADE_LOCAL_H

#include "types.h"
#include "app/worldtrade.h"
#include "dpw/dpw_tr.h"
#include "gfl/nhttp_rap.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/msg.h"
#include "gfl/proc.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "struct_decls.h"
#include "system/app_keycursor.h"
#include "system/app_taskmenu.h"
#include "system/bmp_menulist.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/time_icon.h"
#include "system/wordset.h"

// The Global Trade Station's work and the functions its files share. The header's name is a guess, as are the
// names of the sequences, screens, fields and functions, except task_res and task_work, which the overlay's asserts
// print. The ROM names eight of the files; worldtrade_title.c, worldtrade_mypoke.c, worldtrade_partner.c and
// worldtrade_upload.c are guessed names for the screens of the same names

// What the proc's main function does
enum {
    WT_SEQ_INIT,
    WT_SEQ_FADEIN,
    WT_SEQ_MAIN,
    WT_SEQ_FADEOUT,
    WT_SEQ_OUT,
    WT_SEQ_END,
};

// The screens, each with an init, main and end function
enum {
    WORLDTRADE_ENTER,
    WORLDTRADE_TITLE,
    WORLDTRADE_MYPOKE,
    WORLDTRADE_PARTNER,
    WORLDTRADE_SEARCH,
    WORLDTRADE_MYBOX,
    WORLDTRADE_DEPOSIT,
    WORLDTRADE_UPLOAD,
    WORLDTRADE_STATUS,
    WORLDTRADE_DEMO,
};

// The cell actor resources: the main screen's, the lower screen's, the player's and the main screen's second set
enum {
    WT_CLACT_RES_MAIN,
    WT_CLACT_RES_SUB,
    WT_CLACT_RES_HERO,
    WT_CLACT_RES_MAIN2,
    WT_CLACT_RES_SETS,
};

enum {
    WT_CLACT_RES_CHAR,
    WT_CLACT_RES_PLTT,
    WT_CLACT_RES_CELL,
    WT_CLACT_RES_KINDS,
};

// A window whose text worldtrade_adapter.c prints through the print queue
typedef struct {
    PrintWindow printWin;
    BOOL active;
} WorldTradePrintEntry;

// worldtrade_adapter.c's text printing: the font, the print queue and the message stream
typedef struct {
    Font *font;
    TCBExManager *tcbManager;
    TrainerDataSave *config;
    PrintQueue *printQueue;
    PrintStream *stream;
    BmpWin *streamWin;
    WorldTradePrintEntry entries[24];
    KeyCursor *keyCursor;
} WorldTradePrint;

// A Pokémon icon of the box screen, whose characters and palette worldtrade_box.c uploads at the next VBlank
typedef struct {
    u32 charOffset;
    u32 palette;
    ClActor *icon;
    u8 chars[0x200];
} WorldTradePokeBuf;

// The boxes and the party have 30 slots a page
#define BOX_POKE_NUM 30
// How many Pokémon a search returns
#define SEARCH_POKE_MAX 7

// A search's gender that takes either
#define SEARCH_GENDER_ANY 3

// How many levels a search can ask for
#define SEARCH_LEVEL_SELECT_NUM 11

// Which table of levels a wanted level is from
#define LEVEL_PRINT_TBL_DEPOSIT 0
#define LEVEL_PRINT_TBL_SEARCH 1

typedef struct {
    int boxNo;
    int pos;
} WorldTradeEvoPokeInfo;

// The check of the trainer's name for bad words, which replaces a bad name
typedef struct {
    u16 name[12];
    const u16 *words;
    char result[4];
    int badWordCount;
    u16 unk24;
} WorldTradeNameCheck;

// The deposit and search screens' work
typedef struct {
    ListMenuOption *pokename;
    u16 headwordPos;
    u16 headwordListPos;
    u16 namePos;
    u16 nameListPos;
    int sexPos;
    int levelPos;
    // Whether each species is of the regional Pokédex
    u8 *sinouTable;
    // The species in the order of their names
    u16 *nameSortTable;
    int nameSortNum;
    // The gender ratio of the Pokémon chosen
    int sexSelection;
    int cursorSide;
    int leftCursorPos;
    int rightCursorPos;
} WorldTradeDepositWork;

// The cursor of each list of the name input, kept between the deposit and search screens
typedef struct {
    u16 headList;
    u16 headPos;
    u16 nameList[10];
    u16 namePos[10];
} WorldTradeSelectListPos;

typedef struct WorldTradeInputWork WorldTradeInputWork;

// What worldtrade_input.c's input asks for; the modes from INPUT_MODE_HEADWORD_1 on are its own steps
enum {
    INPUT_MODE_POKEMON_NAME,
    INPUT_MODE_SEX,
    INPUT_MODE_LEVEL,
    INPUT_MODE_NATION,
    INPUT_MODE_HEADWORD_1,
    INPUT_MODE_HEADWORD_2,
    INPUT_MODE_NATION_HEAD1,
    INPUT_MODE_NATION_HEAD2,
};

// The screen that uses the input
enum {
    INPUT_SITUATION_DEPOSIT,
    INPUT_SITUATION_SEARCH,
};

// worldtrade_adapter.c's numbers, printed with a font of digits
typedef struct WorldTradeNumFont WorldTradeNumFont;

// What worldtrade_input.c's input draws with
typedef struct {
    BmpWin **menuWin;
    BmpWin **backWin;
    ClActor *cursorAct;
    ClActor *arrowAct[2];
    // Only for the search screen
    ClActor *searchCursorAct;
    MsgData *msgManager;
    MsgData *monsNameManager;
    MsgData *countryNameManager;
    PokeDexSave *zukan;
    u8 *sinouTable;
    TrainerDataSave *config;
} WorldTradeInputHeader;

typedef struct WorldTradeWork WorldTradeWork;

typedef void (*WorldTradeVBlankFunc)(WorldTradeWork *wk);

struct WorldTradeWork {
    WorldTradeParam *param;
    u8 unk4[0xc];
    int subProcess;
    int subNextProcess;
    // The screen that a screen started from the title returns to
    int subReturnProcess;
    int oldSubProcess;
    int subProcessMode;
    // The message that the save after a server check ends with
    int errorMesNo;
    int subprocessSeq;
    int subprocessNextSeq;
    // Set once the player has seen the title's opening walk
    u16 openingFlag;
    // Set while the player has a Pokémon deposited on the server
    u16 depositFlag;
    u16 unk34;
    // Counts down the frames before the server may be checked again
    u16 serverWaitTime;
    // The server's or the library's error, which the error screens show
    int connectErrorNo;
    u8 unk3C[0x7c];
    // Set while the status screen or the trade demo runs, which takes the cell actors
    int subprocFlag;
    // The cursor of the list being shown, to play a sound when it moves
    u16 listpos;
    u16 unkBE;
    u16 titleCursorPos;
    u16 unkC2;
    u16 boxTrayNo;
    u16 boxCursorPos;
    // How many boxes the player has
    u32 boxCount;
    // The Pokémon chosen to deposit or offer
    BoxPkm *depositPkm;
    // How many Pokémon a search found
    int searchResult;
    // Which of them was chosen
    int touchTrainerPos;
    // The trade partner, made up for the trade demo
    PlayerInfo *partnerStatus;
    // Where the traded Pokémon goes, for its evolution: a box and slot, or 0xff and a party slot
    WorldTradeEvoPokeInfo evoPokeInfo;
    Dpw_Tr_Data uploadPokemonData;
    Dpw_Tr_Data downloadPokemonData[SEARCH_POKE_MAX];
    Dpw_Tr_Data exchangePokemonData;
    Dpw_Tr_PokemonDataSimple post;
    Dpw_Tr_PokemonSearchData want;
    Dpw_Tr_PokemonSearchData search;
    // The last search, which can't be made again
    Dpw_Tr_PokemonSearchData searchBackup;
    u8 unkB62[0x2];
    int searchBackupCountryCode;
    WordSet *wordSet;
    MsgData *msgManager;
    MsgData *monsNameManager;
    MsgData *lobbyMsgManager;
    MsgData *systemMsgManager;
    MsgData *countryNameManager;
    StrBuf *boxTrayNameString;
    // "Quit"
    StrBuf *endString;
    StrBuf *talkString;
    StrBuf *titleString;
    StrBuf *infoString[10];
    u8 unkBB8[0x8];
    ClActUnit *clactUnit;
    u32 clactRes[WT_CLACT_RES_SETS][WT_CLACT_RES_KINDS];
    ClActor *cursorAct;
    ClActor *subCursorAct;
    // The finger that points at the Quit button
    ClActor *fingerAct;
    ClActor *pokeIconAct[BOX_POKE_NUM];
    ClActor *itemIconAct[BOX_POKE_NUM];
    ClActor *cballAct[6];
    // The Pokémon's picture on the main screen
    ClActor *pokemonAct;
    ClActor *subAct[8];
    // The arrows by the box name
    ClActor *boxArrowAct[2];
    // The cursor on the trade partner of the lower screen
    ClActor *partnerCursorAct;
    // The icon that says to look at the lower screen
    ClActor *promptDsAct;
    // How far the screens slide, between the search and partner screens
    int drawOffset;
    BmpWin *msgWin;
    u8 unkD44[0x4];
    // The title of the screen, on the main screen's top
    BmpWin *titleWin;
    BmpWin *subWin;
    BmpWin *menuWin[16];
    BmpWin *infoWin[16];
    BmpWin *talkWin;
    // "Back"
    BmpWin *backWin;
    // The search's country, its label and its value
    BmpWin *countryWin[2];
    // Explains the screen on the lower screen
    BmpWin *explainWin;
    // worldtrade_input.c's input of a search or of the wanted Pokémon
    WorldTradeInputWork *inputWork;
    ListMenuOption *menuList;
    u8 unkDEC[0x8];
    BmpMenuList *bmpListWork;
    WaitIcon *timeWaitWork;
    int wait;
    // The deposit and search screens' work
    WorldTradeDepositWork *dw;
    void *task_res;
    void *task_work;
    // worldtrade_upload.c's steps after the two halves of a save, and after the name check
    u16 saveNextSeq1st;
    u16 saveNextSeq2nd;
    int nameCheckNextSeq;
    // The task that walks the player in or out of the lower screen's trade room
    TCB *demoTask;
    // Set once the player's walk ends
    u16 demoEnd;
    u16 subLcdTouchOK;
    // The characters and palettes of the people a search found, one per trainer type
    void *fieldObjCharaBuf[16];
    NNSG2dCharacterData *fieldObjCharaData[16];
    void *fieldObjPalBuf;
    NNSG2dPaletteData *fieldObjPalData;
    // A copy of the Pokémon the player trades away, which an evolution by trade checks
    PartyPkm *sentPokemon;
    // The Pokémon of the trade demo
    PartyPkm *demoPokemon;
    // The box page's Pokémon, as the server describes them
    Dpw_Tr_PokemonDataSimple *boxWork;
    u16 boxPokeNum;
    u16 boxSearchFlag;
    u32 subOutFlag;
    WorldTradePokeBuf *boxIcon;
    // Called once at the next VBlank
    WorldTradeVBlankFunc vfunc;
    // Called at every VBlank
    WorldTradeVBlankFunc vfunc2;
    // The first y of each person on the lower screen
    s16 subActY[10][2];
    WorldTradeSelectListPos selectListPos;
    // The player's profile on the server, and the server's answer
    Dpw_Common_Profile dcProfile;
    Dpw_Common_ProfileResult dcProfileResult;
    int countryCode;
    // The steps and frames of a server error's message
    s16 localSeq;
    s16 localWait;
    // Frames spent waiting for the server
    s32 timeoutCount;
    TCB *vblankTask;
    TCBManager *tcbManager;
    // The player's walk's work
    void *heroDemoWork;
    void *tcbBuffer;
    WorldTradePrint print;
    // The parameter of the proc that a screen runs, the trade demo or the evolution demo
    void *subProcParam;
    // The server's check of the Pokémon to send, through nhttp_rap.c: the request, its result and the signature
    // that the upload or the trade sends with the Pokémon
    NHttpRap *evilCheck;
    u8 evilCheckStatus;
    u32 evilCheckResult;
    u8 evilCheckSign[0x80];
    // The Wi-Fi login proc's work
    u8 wifiLoginBuffer[0x174];
    // Set once the lower screen's BGs are set up, and while a screen keeps them for the next one
    int subLcdBgInit;
    int subLcdBgKeep;
    int unk12E8;
    // The step of the login, which the Wi-Fi login proc calls back for
    int loginSeq;
    GameProcManager *procManager;
    BOOL procResult;
    // Set when the traded Pokémon may evolve
    int checkEvolution;
    // Set when the partner screen comes back for another partner, which keeps the lower screen's windows
    int partnerChange;
    WorldTradeNameCheck nameCheck;
    // How many times the server's check was made again, after the name was replaced
    int evilCheckRetry;
};

// worldtrade.c
void WorldTrade_TouchWinYesNoMake(WorldTradeWork *wk, int y, int cgx, int palette, u8 passive);
void WorldTrade_TouchWinYesNoMakeEx(WorldTradeWork *wk, int y, int cgx, int palette, int frame, u8 passive);
void WorldTrade_TouchWinYesNoDel(WorldTradeWork *wk);
u32 WorldTrade_TouchSwMain(WorldTradeWork *wk);
void WorldTrade_SelBoxInit(WorldTradeWork *wk, u8 frame, int count, int y);
int WorldTrade_SelBoxMain(WorldTradeWork *wk);
void WorldTrade_SelBoxEnd(WorldTradeWork *wk);
void WorldTrade_SetNextSeq(WorldTradeWork *wk, int toSeq, int nextSeq);
void WorldTrade_ActPos(ClActor *act, int x, int y);
void WorldTrade_SubProcessChange(WorldTradeWork *wk, int subProcess, int mode);
void WorldTrade_SubProcessUpdate(WorldTradeWork *wk);
int WorldTrade_GetTalkSpeed(WorldTradeWork *wk);
void WorldTrade_BoxPokeNumGetStart(WorldTradeWork *wk);
void WorldTrade_TimeIconAdd(WorldTradeWork *wk);
void WorldTrade_TimeIconDel(WorldTradeWork *wk);
void WorldTrade_CLACT_PosChange(ClActor *act, int x, int y);
void WorldTrade_SetPassiveKeepBG2(BOOL main);
void WorldTrade_ClearPassive(void);
void WorldTrade_InitGraphics(WorldTradeWork *wk);
void WorldTrade_ExitGraphics(WorldTradeWork *wk);
void WorldTrade_ShowFatalError(WorldTradeWork *wk);

// worldtrade_box.c
int WorldTrade_Box_Init(WorldTradeWork *wk, int seq);
int WorldTrade_Box_Main(WorldTradeWork *wk, int seq);
int WorldTrade_Box_End(WorldTradeWork *wk, int seq);
BOOL WorldTrade_GetPPorPPP(int tray);
BoxPkm *WorldTrade_GetPokePtr(PokeParty *party, BoxSaveAccessor *box, int tray, int pos);
int WorldTrade_GetBoxPokeNum(PokeParty *party, BoxSaveAccessor *box, int tray);
BOOL WorldTrade_PokemonMailCheck(PartyPkm *pkm);

// worldtrade_demo.c
int WorldTrade_Demo_Init(WorldTradeWork *wk, int seq);
int WorldTrade_Demo_Main(WorldTradeWork *wk, int seq);
int WorldTrade_Demo_End(WorldTradeWork *wk, int seq);
PlayerInfo *WorldTrade_MakePartnerStatus(Dpw_Tr_Data *dtd);

// worldtrade_deposit.c
int WorldTrade_Deposit_Init(WorldTradeWork *wk, int seq);
int WorldTrade_Deposit_Main(WorldTradeWork *wk, int seq);
int WorldTrade_Deposit_End(WorldTradeWork *wk, int seq);
BOOL WorldTrade_SexSelectionCheck(Dpw_Tr_PokemonSearchData *dtps, int sexSelection);
void WorldTrade_PokeNamePrint(BmpWin *win, MsgData *nameManager, int monsno, int flag, int y, u16 color,
                              WorldTradePrint *print);
void WorldTrade_PokeNamePrintNoPut(BmpWin *win, MsgData *nameManager, int monsno, int y, u16 color,
                                   WorldTradePrint *print);
void WorldTrade_CountryPrint(BmpWin *win, MsgData *nameManager, MsgData *msgManager, int countryCode, int flag, int y,
                             u16 color, WorldTradePrint *print);
void WorldTrade_SexPrint(BmpWin *win, MsgData *msgManager, int sex, int flag, int y, int printFlag, u16 color,
                         WorldTradePrint *print);
void WorldTrade_SexPrintNoPut(BmpWin *win, MsgData *msgManager, int sex, int flag, int x, int y, u16 color,
                              WorldTradePrint *print);
void WorldTrade_WantLevelPrint(BmpWin *win, MsgData *msgManager, int level, int flag, int y, u16 color, int tblSelect,
                               WorldTradePrint *print);
void WorldTrade_WantLevelPrint_XY(BmpWin *win, MsgData *msgManager, int level, int flag, int x, int y, u16 color,
                                  int tblSelect, WorldTradePrint *print);
void WorldTrade_PokeWantPrint(MsgData *msgManager, MsgData *monsNameManager, WordSet *wordSet, BmpWin **win, int monsno,
                              int sex, int level, WorldTradePrint *print);
void WorldTrade_MyPokeWantPrint(MsgData *msgManager, MsgData *monsNameManager, WordSet *wordSet, BmpWin **win,
                                int monsno, int sex, int level, WorldTradePrint *print);
void WorldTrade_PokeInfoPrint(MsgData *msgManager, WordSet *wordSet, BmpWin **win, BoxPkm *pkm,
                              Dpw_Tr_PokemonDataSimple *post, WorldTradePrint *print);
u16 *WorldTrade_ZukanSortDataGet(int heapId, int idx, int *num);
void WorldTrade_HeadwordRangeGet(int select, int *start, int *end);
u8 *WorldTrade_SinouZukanDataGet(int heapId);
void WorldTrade_PostPokemonBaseDataMake(Dpw_Tr_Data *dtd, WorldTradeWork *wk);
BmpMenuList *WorldTrade_PokeNameListMake(WorldTradeWork *wk, ListMenuOption **menulist, BmpWin *win,
                                         MsgData *msgManager, MsgData *monsNameManager, WorldTradeDepositWork *dw,
                                         PokeDexSave *zukan);
int WorldTrade_LevelListAdd(ListMenuOption **menulist, MsgData *msgManager, int tblSelect);
void WorldTrade_LevelMinMaxSet(Dpw_Tr_PokemonSearchData *dtps, int index, int tblSelect);
int WorldTrade_LevelTermGet(int min, int max, int tblSelect);
void WorldTrade_CountryCodeSet(WorldTradeWork *wk, int countryCode);
int WorldTrade_NationSortListNumGet(int start, int *number);
int WorldTrade_NationSortListMake(ListMenuOption **menulist, MsgData *countryNameManager, int start);
void WorldTrade_SelectListPosInit(WorldTradeSelectListPos *slp);
void WorldTrade_SelectNameListBackup(WorldTradeSelectListPos *slp, int head, int list, int pos);
// The number of countries in the list
extern const u32 WorldTrade_CountryListNum;
extern const u32 WorldTrade_SexStringTable[];

// worldtrade_enter.c
int WorldTrade_Enter_Init(WorldTradeWork *wk, int seq);
int WorldTrade_Enter_Main(WorldTradeWork *wk, int seq);
int WorldTrade_Enter_End(WorldTradeWork *wk, int seq);
void Enter_MessagePrint(WorldTradeWork *wk, MsgData *msgManager, int msgNo, int wait, u16 dat);
void Enter_MessagePrintNoStream(WorldTradeWork *wk, MsgData *msgManager, int msgNo, int wait, u16 dat);
// Print a string at x, or centered for flag 1 or right-aligned for flag 2
void WorldTrade_SysPrint(BmpWin *win, StrBuf *str, int x, int y, int flag, u16 color, WorldTradePrint *print);
void WorldTrade_TouchPrint(BmpWin *win, StrBuf *str, int x, int y, int flag, u16 color, WorldTradePrint *print);
void WorldTrade_ExplainPrint(BmpWin *win, MsgData *msgManager, int no, WorldTradePrint *print);
void WorldTrade_WifiIconAdd(WorldTradeWork *wk);

// worldtrade_input.c
WorldTradeInputWork *WorldTrade_Input_Init(WorldTradeInputHeader *header, int frame, int situation);
void WorldTrade_Input_Start(WorldTradeInputWork *wk, int type);
void WorldTrade_Input_Exit(WorldTradeInputWork *wk);
// The value chosen, or -1 while the input runs or -2 when it was cancelled
u32 WorldTrade_Input_Main(WorldTradeInputWork *wk);

// worldtrade_mypoke.c
int WorldTrade_MyPoke_Init(WorldTradeWork *wk, int seq);
int WorldTrade_MyPoke_Main(WorldTradeWork *wk, int seq);
int WorldTrade_MyPoke_End(WorldTradeWork *wk, int seq);
// Prints a Pokémon's nickname, gender, species, level and item into seven windows
void WorldTrade_MyPokeInfoPrint(MsgData *msgManager, MsgData *monsNameManager, WordSet *wordSet, BmpWin **win,
                                BoxPkm *pkm, Dpw_Tr_PokemonDataSimple *post, WorldTradePrint *print);
// Prints the Pokémon's owner and its original trainer
void WorldTrade_PokeInfoPrint2(MsgData *msgManager, BmpWin **win, u16 *name, PartyPkm *pkm, BmpWin **oyaWin,
                               WorldTradePrint *print);
// Uploads the Pokémon's front sprite to the main screen's OBJ characters
void WorldTrade_TransPokeGraphic(PartyPkm *pkm);

// worldtrade_partner.c
int WorldTrade_Partner_Init(WorldTradeWork *wk, int seq);
int WorldTrade_Partner_Main(WorldTradeWork *wk, int seq);
int WorldTrade_Partner_End(WorldTradeWork *wk, int seq);

// worldtrade_search.c
int WorldTrade_Search_Init(WorldTradeWork *wk, int seq);
int WorldTrade_Search_Main(WorldTradeWork *wk, int seq);
int WorldTrade_Search_End(WorldTradeWork *wk, int seq);

// worldtrade_status.c
int WorldTrade_Status_Init(WorldTradeWork *wk, int seq);
int WorldTrade_Status_Main(WorldTradeWork *wk, int seq);
int WorldTrade_Status_End(WorldTradeWork *wk, int seq);

// worldtrade_sublcd.c
void WorldTrade_SubLcdActorAdd(WorldTradeWork *wk);
// Walks the player into the trade room, or out of it
void WorldTrade_HeroDemo(WorldTradeWork *wk);
void WorldTrade_ReturnHeroDemo(WorldTradeWork *wk);
// The person of the lower screen that is touched, of the first count, or -1
int WorldTrade_SubLcdObjHitCheck(int count);
// Shows the first count people of the lower screen as the trainers a search found, appearing when appear is set
void WorldTrade_SubLcdMatchObjAppear(WorldTradeWork *wk, int count, int appear);
// Hides the people of the lower screen that a search found
void WorldTrade_SubLcdMatchObjHide(WorldTradeWork *wk);
void WorldTrade_FreeFieldObjData(WorldTradeWork *wk);
// Puts the partner cursor on a person of the lower screen, offset by y
void WorldTrade_SetPartnerCursorPos(WorldTradeWork *wk, int index, int offsetY);
// Puts the people back where they stand, or 32 pixels lower
void WorldTrade_SetPartnerExchangePos(WorldTradeWork *wk);
void WorldTrade_SetPartnerExchangePosIsReturns(WorldTradeWork *wk);

// worldtrade_title.c
int WorldTrade_Title_Init(WorldTradeWork *wk, int seq);
int WorldTrade_Title_Main(WorldTradeWork *wk, int seq);
int WorldTrade_Title_End(WorldTradeWork *wk, int seq);
// Sets up the lower screen's BGs, the trade room, with the room's BG moved down by bg1YOffset
void WorldTrade_SubLcdBgInit(WorldTradeWork *wk, int bg1YOffset, BOOL bg2NoClear);
void WorldTrade_SubLcdBgExit(WorldTradeWork *wk);
void WorldTrade_SubLcdBgGraphicSet(WorldTradeWork *wk);
void WorldTrade_SubLcdWinGraphicSet(WorldTradeWork *wk);
// Shows a text that explains the screen on the lower screen
void WorldTrade_SubLcdExplainPut(WorldTradeWork *wk, int explain);

// worldtrade_upload.c
int WorldTrade_Upload_Init(WorldTradeWork *wk, int seq);
int WorldTrade_Upload_Main(WorldTradeWork *wk, int seq);
int WorldTrade_Upload_End(WorldTradeWork *wk, int seq);

// worldtrade_adapter.c
void WorldTrade_SetBoxPkmNickname(WordSet *wordSet, u32 index, BoxPkm *pkm);
PartyPkm *WorldTrade_AllocPartyPkm(HeapID heapId);
void WorldTrade_CopyPartyPkm(PartyPkm *src, PartyPkm *dest);
// Clears a window from the screen, now or at the next VBlank
void WorldTrade_ClearWindow(BmpWin *win, int mode);
// A message with the word set's words put in
StrBuf *WorldTrade_ExpandMessage(WordSet *wordSet, MsgData *msgData, u32 msgNo, HeapID heapId);
void WorldTrade_BoxPkmToPartyPkm(BoxPkm *pkm, PartyPkm *dest);
BoxPkm *WorldTrade_GetBoxPkm(PartyPkm *pkm);
// The width of a string in the print's font
int WorldTrade_GetStrWidth(WorldTradePrint *print, u8 font, StrBuf *str, int spacing);
void WorldTrade_PrintInit(WorldTradePrint *print, TrainerDataSave *config);
void WorldTrade_PrintExit(WorldTradePrint *print);
void WorldTrade_PrintMain(WorldTradePrint *print);
// Whether the message stream still prints
BOOL WorldTrade_PrintIsBusy(WorldTradePrint *print);
void WorldTrade_Print(BmpWin *win, u8 font, StrBuf *str, int x, int y, WorldTradePrint *print);
// The same through the message stream, at the player's text speed
void WorldTrade_StreamPrint(BmpWin *win, u8 font, StrBuf *str, int x, int y, WorldTradePrint *print);
void WorldTrade_PrintColor(BmpWin *win, u8 font, StrBuf *str, int x, int y, int unused, u16 color,
                           WorldTradePrint *print);
// Forgets the windows that still print, and ends the stream
void WorldTrade_PrintClear(WorldTradePrint *print);
WorldTradeNumFont *WorldTrade_NumFontCreate(u32 unused0, u32 unused1, u32 unused2, HeapID heapId);
void WorldTrade_NumFontDelete(WorldTradeNumFont *numFont);
void WorldTrade_NumFontMain(WorldTradeNumFont *numFont);
// Prints a number of digits at x and y of a window
void WorldTrade_NumFontPrintNumber(WorldTradeNumFont *numFont, int num, int digits, int dispType, BmpWin *win, int x,
                                   int y);
// Prints the slash between two numbers
void WorldTrade_NumFontPrintSlash(WorldTradeNumFont *numFont, int unused, BmpWin *win, int x, int y);

#endif // POKEBW2_OV214_WORLDTRADE_LOCAL_H
