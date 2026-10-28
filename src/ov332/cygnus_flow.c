#include "types.h"
#include "app/unova_link.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/backup_system.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/math.h"
#include "nitro/os.h"
#include "pml/item.h"
#include "pml/met_data.h"
#include "pml/personal.h"
#include "pml/poke_graphic.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/box.h"
#include "save/cygnus_save_control.h"
#include "save/dream_world.h"
#include "save/event_work.h"
#include "save/player_info.h"
#include "save/pokedex_savedata.h"
#include "save/save_control.h"
#include "save/trainer_card.h"
#include "system/bmp_menulist.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/poke_icon.h"
#include "system/version.h"
#include "system/wordset.h"
#include "text/system/ability_handlers_2.h"

// Pokémon Dream Radar: receiving the Pokémon and items that the 3DS game writes to the card. A legendary Pokémon
// appears in a flash of light before the rest show up as icons. Named after the ROM's assertion string

// What an entry of the data is
typedef enum {
    CYGNUS_ENTRY_NONE,
    CYGNUS_ENTRY_POKEMON,
    CYGNUS_ENTRY_ITEM,
} CygnusEntryType;

// What CygnusData_GetState says about the data
enum {
    CYGNUS_STATE_NONE,
    CYGNUS_STATE_RECEIVED,
    CYGNUS_STATE_NEW,
    CYGNUS_STATE_INVALID,
};

// What CygnusFlow_GetState says to do
enum {
    CYGNUS_FLOW_CANT_RECEIVE,
    CYGNUS_FLOW_NOTHING,
    CYGNUS_FLOW_RECEIVE,
};

// The steps of CygnusFlow_SeqMenu
enum {
    MENU_SEQ_START_SCENE,
    MENU_SEQ_PRINT,
    MENU_SEQ_WAIT_PRINT,
    MENU_SEQ_WAIT_MENU,
    MENU_SEQ_SELECT,
    MENU_SEQ_END_MENU_SCENE,
    MENU_SEQ_RECEIVE,
    MENU_SEQ_END_SCENE,
    MENU_SEQ_END,
};

// The steps of CygnusFlow_SeqCantReceive
enum {
    CANT_RECEIVE_SEQ_START_SCENE,
    CANT_RECEIVE_SEQ_PRINT,
    CANT_RECEIVE_SEQ_WAIT_PRINT,
    CANT_RECEIVE_SEQ_END_SCENE,
    CANT_RECEIVE_SEQ_END,
};

// The steps of CygnusFlow_SeqNothing
enum {
    NOTHING_SEQ_START_SCENE,
    NOTHING_SEQ_PRINT,
    NOTHING_SEQ_WAIT_PRINT,
    NOTHING_SEQ_PRINT_INITIALIZING,
    NOTHING_SEQ_WAIT_INITIALIZING,
    NOTHING_SEQ_COMMIT,
    NOTHING_SEQ_SAVE,
    NOTHING_SEQ_PRINT_INITIALIZED,
    NOTHING_SEQ_WAIT_INITIALIZED,
    NOTHING_SEQ_END_SCENE,
    NOTHING_SEQ_END,
};

// The steps of CygnusFlow_SeqReceive
enum {
    RECEIVE_SEQ_READ,
    RECEIVE_SEQ_FADE_OUT,
    RECEIVE_SEQ_SHOW_LEGENDS,
    RECEIVE_SEQ_RECEIVE_ALL,
    RECEIVE_SEQ_END,
};

// The steps of CygnusFlow_SeqReading
enum {
    READING_SEQ_START_SCENE,
    READING_SEQ_PRINT,
    READING_SEQ_WAIT,
    READING_SEQ_END_SCENE,
    READING_SEQ_END,
};

// The steps of CygnusFlow_SeqShowLegends
enum {
    LEGENDS_SEQ_CREATE,
    LEGENDS_SEQ_FADE_IN,
    LEGENDS_SEQ_SHOW,
    LEGENDS_SEQ_NEXT,
    LEGENDS_SEQ_FADE_OUT,
    LEGENDS_SEQ_END,
};

// The steps of CygnusFlow_SeqShowLegend
enum {
    LEGEND_SEQ_LOAD,
    LEGEND_SEQ_APPEAR,
    LEGEND_SEQ_PRINT,
    LEGEND_SEQ_WAIT_SE,
    LEGEND_SEQ_WAIT_PRINT,
    LEGEND_SEQ_END,
};

// The steps of CygnusFlow_SeqReceiveAll
enum {
    RECEIVE_ALL_SEQ_RECEIVE,
    RECEIVE_ALL_SEQ_FADE_IN,
    RECEIVE_ALL_SEQ_PRINT,
    RECEIVE_ALL_SEQ_LOAD_ICONS,
    RECEIVE_ALL_SEQ_START,
    RECEIVE_ALL_SEQ_SAVE,
    RECEIVE_ALL_SEQ_PRINT_SAVED,
    RECEIVE_ALL_SEQ_WAIT_SE,
    RECEIVE_ALL_SEQ_WAIT_SAVED,
    RECEIVE_ALL_SEQ_PRINT_POKEMON,
    RECEIVE_ALL_SEQ_WAIT_POKEMON,
    RECEIVE_ALL_SEQ_PRINT_ITEMS,
    RECEIVE_ALL_SEQ_WAIT_ITEMS,
    RECEIVE_ALL_SEQ_END,
};

// The steps of the sequences that start a scene
enum {
    SCENE_SEQ_START,
    SCENE_SEQ_WAIT,
    SCENE_SEQ_END,
};

// What CygnusAppear_Run does: load the particles, flash, wait, show the sprite, fade it in, hold it, pause, whiten it,
// wait, and end
enum {
    APPEAR_STATE_LOAD,
    APPEAR_STATE_FLASH,
    APPEAR_STATE_SHOW,
    APPEAR_STATE_FADE_IN,
    APPEAR_STATE_WAIT_FLASH,
    APPEAR_STATE_HOLD,
    APPEAR_STATE_PAUSE,
    APPEAR_STATE_WHITEN,
    APPEAR_STATE_WAIT_END,
    APPEAR_STATE_END,
};

// The steps of CygnusData_Save
enum {
    SAVE_STATE_START,
    SAVE_STATE_WAIT,
    SAVE_STATE_DONE,
};

// The steps of CygnusIcons_Update: start an icon falling, wait for it to land, go on to the next
enum {
    ICONS_STATE_START,
    ICONS_STATE_FALL,
    ICONS_STATE_NEXT,
};

#define CYGNUS_ENTRY_MAX 12
// The Pokémon that the words of the data can name, which the player hasn't received yet
#define CYGNUS_WORD_POKEMON_END 8
// Pokémon named by species, form and sex, of which there are at most 6
#define CYGNUS_WORD_FREE_POKEMON_END 14
#define CYGNUS_FREE_POKEMON_MAX 6
// Items, by item and quantity
#define CYGNUS_WORD_ITEM_END 20

#define CYGNUS_LEGEND_COUNT 3
// The flag that the player gets the Dream Radar's Pokémon after
#define CYGNUS_FLAG 0x961
// The OBJ palette that the legendary Pokémon uses
#define CYGNUS_APPEAR_PALETTE 4
// How long an icon takes to fall into place, in milliseconds
#define CYGNUS_ICON_TIME 160

typedef struct {
    CygnusEntryType type;
    // The species or the item
    u16 id;
    u8 form;
    u8 sex;
    u16 quantity;
    // Whether it is one of the legendary Pokémon, which appear one by one
    u16 legend;
} CygnusEntry;

struct CygnusData {
    void *buffer;
    GameData *gameData;
    CygnusSaveData *radarData;
    DreamRadarSave *save;
    // What CygnusSaveData_Load says of the data
    u32 status;
    u32 saveState;
    CygnusEntry entries[CYGNUS_ENTRY_MAX];
    u32 count;
    // The Pokémon received, a bit each
    u32 received;
};

// A legendary Pokémon appearing
struct CygnusAppear {
    u16 heapId;
    ClActUnit *unit;
    ClActor *actor;
    BOOL done;
    u32 palette;
    u32 chars;
    u32 cellAnims;
    u32 timer;
    u32 fade;
    u32 state;
    u16 blend;
    u16 unk2A[64];
    u16 blendedPalette[16];
    u16 whitePalette[16];
    u16 spritePalette[16];
    // How far the sprite's lowest pixels are below the usual
    s16 offset;
    u16 unk10C;
    KeySystemParticle *particle;
    void (*func)(CygnusAppear *appear);
};

// A Pokémon's or an item's icon, which falls into place
typedef struct {
    u32 palette;
    u32 chars;
    u32 cellAnims;
    ClActor *actor;
    ClActorPos pos;
    u64 startTime;
    BOOL falling;
} CygnusIcon;

struct CygnusIcons {
    u16 heapId;
    ClActUnit *unit;
    CygnusIcon icons[CYGNUS_ENTRY_MAX];
    // The icon falling
    u32 current;
    BOOL done;
    BOOL active;
    u32 state;
    CygnusData *data;
};

static void CygnusFlow_SeqCantReceive(KeySystemSeq *seq, int *state, void *work);
static void CygnusFlow_SeqNothing(KeySystemSeq *seq, int *state, void *work);
static void CygnusFlow_SeqReceive(KeySystemSeq *seq, int *state, void *work);
static void CygnusFlow_SeqReading(KeySystemSeq *seq, int *state, void *work);
static void CygnusFlow_SeqShowLegends(KeySystemSeq *seq, int *state, void *work);
static void CygnusFlow_SeqShowLegend(KeySystemSeq *seq, int *state, void *work);
static void CygnusFlow_SeqReceiveAll(KeySystemSeq *seq, int *state, void *work);
static void CygnusFlow_SeqStartMenuScene(KeySystemSeq *seq, int *state, void *work);
static void CygnusFlow_SeqStartMsgScene(KeySystemSeq *seq, int *state, void *work);
static void CygnusFlow_SeqStartReceiveScene(KeySystemSeq *seq, int *state, void *work);
static void CygnusFlow_MenuSceneInit(void *work, HeapID heapId);
static BOOL CygnusFlow_MenuSceneMain(void *work);
static void CygnusFlow_MenuSceneExit(void *work);
static void CygnusFlow_MsgSceneInit(void *work, HeapID heapId);
static BOOL CygnusFlow_MsgSceneMain(void *work);
static void CygnusFlow_MsgSceneExit(void *work);
static void CygnusFlow_ReceiveSceneInit(void *work, HeapID heapId);
static BOOL CygnusFlow_ReceiveSceneMain(void *work);
static void CygnusFlow_ReceiveSceneExit(void *work);
static CygnusAppear *CygnusAppear_Create(KeySystemParticle *particle, KeySystemGraphic *graphic, HeapID heapId);
static void CygnusAppear_Start(CygnusAppear *appear);
static void CygnusAppear_Free(CygnusAppear *appear);
static void CygnusAppear_Update(CygnusAppear *appear);
static BOOL CygnusAppear_IsDone(CygnusAppear *appear);
static void CygnusAppear_Load(CygnusAppear *appear, PartyPkm *pkm);
static void CygnusAppear_Unload(CygnusAppear *appear);
static void CygnusAppear_Run(CygnusAppear *appear);
static CygnusData *CygnusData_Create(GameData *gameData, HeapID heapId);
static void CygnusData_Free(CygnusData *data);
static void CygnusData_Load(CygnusData *data, HeapID heapId);
static u32 CygnusData_GetState(CygnusData *data);
static BOOL CygnusData_HasLegend(const CygnusData *data);
static int CygnusData_FindLegend(const CygnusData *data, u32 n);
static void CygnusData_Commit(CygnusData *data);
static BOOL CygnusData_Save(CygnusData *data);
static BOOL CygnusData_GetEntry(const CygnusData *data, u32 index, CygnusEntryType *type, u32 *id);
static PartyPkm *CygnusData_CreatePokemon(const CygnusData *data, u32 index, HeapID heapId);
static BOOL CygnusData_GetItem(const CygnusData *data, u32 index, u16 *item, u8 *quantity);
static u32 CygnusData_GetItemNumber(const CygnusData *data, u32 n);
static u32 CygnusData_CountPokemon(const CygnusData *data);
static u32 CygnusData_CountItems(const CygnusData *data);
static u32 CygnusData_GetCount(const CygnusData *data);
static void CygnusData_Receive(CygnusData *data, HeapID heapId);
static void CygnusData_ReadEntries(CygnusData *data, const u32 *words, HeapID heapId);
static BOOL CygnusData_IsPokemonValid(u16 species, u8 form, u8 sex, HeapID heapId);
static u16 CygnusData_IsLegend(u16 species);
static CygnusIcons *CygnusIcons_Create(KeySystemGraphic *graphic, CygnusData *data, HeapID heapId);
static void CygnusIcons_Free(CygnusIcons *icons);
static void CygnusIcons_Update(CygnusIcons *icons);
static void CygnusIcons_Start(CygnusIcons *icons);
static BOOL CygnusIcons_IsDone(CygnusIcons *icons);
static void CygnusIcons_Load(CygnusIcons *icons);
static void CygnusIcons_Unload(CygnusIcons *icons);
static void CygnusIcon_Load(CygnusIcon *icon, ClActUnit *unit, CygnusData *data, u16 index, HeapID heapId);
static void CygnusIcon_Unload(CygnusIcon *icon);
static void CygnusIcon_Start(CygnusIcon *icon);
static BOOL CygnusIcon_Update(CygnusIcon *icon);
static void CygnusFlow_CreateAppear(KeySystemWork *wk, HeapID heapId);
static void CygnusFlow_FreeAppear(KeySystemWork *wk);
static void CygnusFlow_HideScreen(KeySystemWork *wk, HeapID heapId);
static void CygnusFlow_ShowScreen(KeySystemWork *wk);
static u32 CygnusFlow_GetState(KeySystemWork *wk, u32 *msgId);
static int CygnusAppear_GetSpriteBottom(BoxPkm *pkm, HeapID heapId);

// Sets the blend's weights of the two layers
static inline void CygnusFlow_SetBlendAlpha(s16 alpha1, s16 alpha2) {
    reg_G2_BLDALPHA = alpha1 | (alpha2 << 8);
}

// Thundurus, Tornadus and Landorus
static const u32 sLegends[CYGNUS_LEGEND_COUNT] = { SPECIES_THUNDURUS, SPECIES_TORNADUS, SPECIES_LANDORUS };

static const KeySystemSceneFuncs sCygnusMsgSceneFuncs = {
    CygnusFlow_MsgSceneInit,
    CygnusFlow_MsgSceneMain,
    CygnusFlow_MsgSceneExit,
    NULL,
};

static const KeySystemSceneFuncs sReceiveSceneFuncs = {
    CygnusFlow_ReceiveSceneInit,
    CygnusFlow_ReceiveSceneMain,
    CygnusFlow_ReceiveSceneExit,
    NULL,
};

static const KeySystemSceneFuncs sCygnusMenuSceneFuncs = {
    CygnusFlow_MenuSceneInit,
    CygnusFlow_MenuSceneMain,
    CygnusFlow_MenuSceneExit,
    NULL,
};

// The level of the Pokémon by the number of badges
static const u32 sLevelsByBadges[9] = { 5, 10, 10, 20, 20, 30, 30, 40, 40 };

void CygnusFlow_Init(KeySystemWork *wk, HeapID heapId) {
    wk->cygnusData = CygnusData_Create(wk->param->gameData, heapId);
    wk->cygnusIcons = CygnusIcons_Create(wk->graphic, wk->cygnusData, heapId);
}

void CygnusFlow_Exit(KeySystemWork *wk) {
    CygnusIcons_Free(wk->cygnusIcons);
    CygnusData_Free(wk->cygnusData);
}

void CygnusFlow_SeqMenu(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    s32 result;

    switch (*state) {
    case MENU_SEQ_START_SCENE:
        CygnusData_Load(wk->cygnusData, HEAPID_KEY_SYSTEM);
        KeySystem_FreeTitleWin(wk);
        KeySystem_CreateTitleWin(wk, 21, HEAPID_KEY_SYSTEM);
        KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
        KeySystemSeq_Push(seq, CygnusFlow_SeqStartMenuScene);
        (*state)++;
        break;
    case MENU_SEQ_PRINT:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_ReceiveResearchDataFrom_2,
                                 KEY_SYSTEM_MSG_STREAM);
        (*state)++;
        break;
    case MENU_SEQ_WAIT_PRINT:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            KeySystem_CreateYesNoMenu(wk, HEAPID_KEY_SYSTEM);
            (*state)++;
        }
        break;
    case MENU_SEQ_WAIT_MENU:
        if (KeySystemMenu_UpdatePrint(wk->menu)) {
            GFL_BGSysQueueScrLoad(0);
            (*state)++;
        }
        break;
    case MENU_SEQ_SELECT:
        result = KeySystemMenu_Update(wk->menu);
        if (result == BMPMENULIST_NULL) {
            break;
        }
        KeySystem_FreeMenu(wk);
        switch (result) {
        case 0:
            *state = MENU_SEQ_END_MENU_SCENE;
            break;
        case 1:
        default:
            *state = MENU_SEQ_END_SCENE;
            break;
        }
        break;
    case MENU_SEQ_END_MENU_SCENE:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case MENU_SEQ_RECEIVE:
        switch (CygnusFlow_GetState(wk, NULL)) {
        case CYGNUS_FLOW_RECEIVE:
            KeySystemSeq_Push(seq, CygnusFlow_SeqReceive);
            break;
        case CYGNUS_FLOW_CANT_RECEIVE:
            KeySystemSeq_Push(seq, CygnusFlow_SeqCantReceive);
            break;
        case CYGNUS_FLOW_NOTHING:
            KeySystemSeq_Push(seq, CygnusFlow_SeqNothing);
            break;
        }
        (*state)++;
        break;
    case MENU_SEQ_END_SCENE:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case MENU_SEQ_END:
        KeySystem_FreeMsgWin(wk);
        if (wk->titleWin != NULL) {
            KeySystemMsgWin_Free(wk->titleWin);
            wk->titleWin = NULL;
        }
        KeySystemBG_StartFade(wk->bg, KEY_SYSTEM_BG_FADE_0_TO_3, 30);
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void CygnusFlow_SeqCantReceive(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    u32 msgId;

    switch (*state) {
    case CANT_RECEIVE_SEQ_START_SCENE:
        KeySystemSeq_Push(seq, CygnusFlow_SeqStartMsgScene);
        (*state)++;
        break;
    case CANT_RECEIVE_SEQ_PRINT:
        CygnusFlow_GetState(wk, &msgId);
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, msgId, KEY_SYSTEM_MSG_STREAM);
        (*state)++;
        break;
    case CANT_RECEIVE_SEQ_WAIT_PRINT:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case CANT_RECEIVE_SEQ_END_SCENE:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case CANT_RECEIVE_SEQ_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

// The data has nothing to receive, and is marked read anyway
static void CygnusFlow_SeqNothing(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case NOTHING_SEQ_START_SCENE:
        KeySystemSeq_Push(seq, CygnusFlow_SeqStartMsgScene);
        (*state)++;
        break;
    case NOTHING_SEQ_PRINT:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_PokemonDreamRadarsResearch,
                                 KEY_SYSTEM_MSG_STREAM);
        (*state)++;
        break;
    case NOTHING_SEQ_WAIT_PRINT:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case NOTHING_SEQ_PRINT_INITIALIZING:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_InitializingResearchData,
                                 KEY_SYSTEM_MSG_PRINT_WAIT_ICON);
        (*state)++;
        break;
    case NOTHING_SEQ_WAIT_INITIALIZING:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case NOTHING_SEQ_COMMIT:
        CygnusData_Commit(wk->cygnusData);
        (*state)++;
        break;
    case NOTHING_SEQ_SAVE:
        if (CygnusData_Save(wk->cygnusData)) {
            (*state)++;
        }
        break;
    case NOTHING_SEQ_PRINT_INITIALIZED:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_ResearchDataInitializedStart,
                                 KEY_SYSTEM_MSG_STREAM);
        (*state)++;
        break;
    case NOTHING_SEQ_WAIT_INITIALIZED:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case NOTHING_SEQ_END_SCENE:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case NOTHING_SEQ_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void CygnusFlow_SeqReceive(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case RECEIVE_SEQ_READ:
        KeySystemSeq_Push(seq, CygnusFlow_SeqReading);
        (*state)++;
        break;
    case RECEIVE_SEQ_FADE_OUT:
        if (CygnusData_HasLegend(wk->cygnusData)) {
            KeySystemSeq_Push(seq, KeySystem_SeqFadeOut);
            *state = RECEIVE_SEQ_SHOW_LEGENDS;
        } else {
            KeySystemSeq_Push(seq, KeySystem_SeqFadeOutWhite);
            *state = RECEIVE_SEQ_RECEIVE_ALL;
        }
        break;
    case RECEIVE_SEQ_SHOW_LEGENDS:
        KeySystemSeq_Push(seq, CygnusFlow_SeqShowLegends);
        (*state)++;
        break;
    case RECEIVE_SEQ_RECEIVE_ALL:
        KeySystemSeq_Push(seq, CygnusFlow_SeqReceiveAll);
        (*state)++;
        break;
    case RECEIVE_SEQ_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void CygnusFlow_SeqReading(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case READING_SEQ_START_SCENE:
        KeySystemSeq_Push(seq, CygnusFlow_SeqStartReceiveScene);
        (*state)++;
        break;
    case READING_SEQ_PRINT:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_ConnectingPokemonDreamRadar,
                                 KEY_SYSTEM_MSG_PRINT_WAIT_ICON);
        (*state)++;
        break;
    case READING_SEQ_WAIT:
        if (wk->timer++ > 180) {
            wk->timer = 0;
            (*state)++;
        }
        break;
    case READING_SEQ_END_SCENE:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case READING_SEQ_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void CygnusFlow_SeqShowLegends(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case LEGENDS_SEQ_CREATE:
        CygnusFlow_CreateAppear(wk, HEAPID_KEY_SYSTEM);
        CygnusFlow_HideScreen(wk, HEAPID_KEY_SYSTEM);
        (*state)++;
        break;
    case LEGENDS_SEQ_FADE_IN:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeIn);
        (*state)++;
        break;
    case LEGENDS_SEQ_SHOW:
        KeySystemSeq_Push(seq, CygnusFlow_SeqShowLegend);
        (*state)++;
        break;
    case LEGENDS_SEQ_NEXT:
        if (CygnusData_FindLegend(wk->cygnusData, ++wk->timer) != -1) {
            *state = LEGENDS_SEQ_SHOW;
        } else {
            wk->timer = 0;
            (*state)++;
        }
        break;
    case LEGENDS_SEQ_FADE_OUT:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeOutWhite);
        (*state)++;
        break;
    case LEGENDS_SEQ_END:
        CygnusFlow_FreeAppear(wk);
        CygnusFlow_ShowScreen(wk);
        KeySystemSeq_Pop(seq);
        break;
    }
}

// Shows the legendary Pokémon that wk->timer counts
static void CygnusFlow_SeqShowLegend(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    PartyPkm *pkm;
    u32 species;
    CygnusEntryType type;
    StrBuf *str;

    switch (*state) {
    case LEGEND_SEQ_LOAD:
        pkm = CygnusData_CreatePokemon(wk->cygnusData, CygnusData_FindLegend(wk->cygnusData, wk->timer),
                                       HEAPID_KEY_SYSTEM);
        CygnusAppear_Load(wk->cygnusAppear, pkm);
        GFL_HeapFree(pkm);
        CygnusAppear_Start(wk->cygnusAppear);
        (*state)++;
        // fallthrough
    case LEGEND_SEQ_APPEAR:
        CygnusAppear_Update(wk->cygnusAppear);
        if (CygnusAppear_IsDone(wk->cygnusAppear)) {
            (*state)++;
        }
        break;
    case LEGEND_SEQ_PRINT:
        GFL_SndSEPlay(SEQ_SE_SYS_50);
        KeySystem_CreateMsgWinOn(wk, 2, HEAPID_KEY_SYSTEM);
        GFL_BGSysQueueScrLoad(2);
        CygnusData_GetEntry(wk->cygnusData, CygnusData_FindLegend(wk->cygnusData, wk->timer), &type, &species);
        WordSet_LoadSpeciesName(wk->wordSet, 0, (u16)species);
        str = KeySystem_LoadFormattedStr(wk->wordSet, wk->msgData, AbilityHandlers2_Text_Arrived, HEAPID_KEY_SYSTEM);
        KeySystemMsgWin_PrintStr(wk->msgWin, str, KEY_SYSTEM_MSG_STREAM_NO_CURSOR);
        GFL_StrBufFree(str);
        (*state)++;
        break;
    case LEGEND_SEQ_WAIT_SE:
        if (!GFL_SndPlayerIsActiveAny()) {
            KeySystemMsgWin_CreateCursor(wk->msgWin);
            (*state)++;
        }
        // fallthrough
    case LEGEND_SEQ_WAIT_PRINT:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case LEGEND_SEQ_END:
        CygnusAppear_Unload(wk->cygnusAppear);
        KeySystem_FreeMsgWinOn(wk, 2);
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void CygnusFlow_SeqReceiveAll(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    BOOL saved;
    BOOL done;

    switch (*state) {
    case RECEIVE_ALL_SEQ_RECEIVE:
        CygnusData_Receive(wk->cygnusData, HEAPID_KEY_SYSTEM);
        KeySystemBG_LoadScreen(wk->bg, 2, 3);
        (*state)++;
        break;
    case RECEIVE_ALL_SEQ_FADE_IN:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeInUnused);
        (*state)++;
        break;
    case RECEIVE_ALL_SEQ_PRINT:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_ReceivingResearchDataFrom,
                                 KEY_SYSTEM_MSG_PRINT_WAIT_ICON);
        (*state)++;
        break;
    case RECEIVE_ALL_SEQ_LOAD_ICONS:
        CygnusIcons_Load(wk->cygnusIcons);
        (*state)++;
        break;
    case RECEIVE_ALL_SEQ_START:
        CygnusData_Commit(wk->cygnusData);
        CygnusIcons_Start(wk->cygnusIcons);
        (*state)++;
        break;
    case RECEIVE_ALL_SEQ_SAVE:
        saved = CygnusData_Save(wk->cygnusData);
        done = CygnusIcons_IsDone(wk->cygnusIcons);
        CygnusIcons_Update(wk->cygnusIcons);
        if (saved && done) {
            (*state)++;
        }
        break;
    case RECEIVE_ALL_SEQ_PRINT_SAVED:
        GFL_SndSEPlay(SEQ_SE_SAVE);
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_ResearchDataWrittenSaved,
                                 KEY_SYSTEM_MSG_STREAM_NO_CURSOR);
        (*state)++;
        break;
    case RECEIVE_ALL_SEQ_WAIT_SE:
        if (!GFL_SndPlayerIsActiveAny()) {
            KeySystemMsgWin_CreateCursor(wk->msgWin);
            (*state)++;
        }
        // fallthrough
    case RECEIVE_ALL_SEQ_WAIT_SAVED:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case RECEIVE_ALL_SEQ_PRINT_POKEMON:
        if (CygnusData_CountPokemon(wk->cygnusData)) {
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_TransferredReceivedPokemonPc,
                                     KEY_SYSTEM_MSG_STREAM);
            (*state)++;
        } else {
            *state = RECEIVE_ALL_SEQ_PRINT_ITEMS;
        }
        break;
    case RECEIVE_ALL_SEQ_WAIT_POKEMON:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case RECEIVE_ALL_SEQ_PRINT_ITEMS:
        if (CygnusData_CountItems(wk->cygnusData)) {
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_ReceivedItemSWere,
                                     KEY_SYSTEM_MSG_STREAM);
            (*state)++;
        } else {
            *state = RECEIVE_ALL_SEQ_END;
        }
        break;
    case RECEIVE_ALL_SEQ_WAIT_ITEMS:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case RECEIVE_ALL_SEQ_END:
        CygnusIcons_Unload(wk->cygnusIcons);
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void CygnusFlow_SeqStartMenuScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case SCENE_SEQ_START:
        KeySystemScene_Start(wk->scene, &sCygnusMenuSceneFuncs, HEAPID_KEY_SYSTEM);
        (*state)++;
        break;
    case SCENE_SEQ_WAIT:
        if (KeySystemScene_IsIdle(wk->scene)) {
            (*state)++;
        }
        break;
    case SCENE_SEQ_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void CygnusFlow_SeqStartMsgScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case SCENE_SEQ_START:
        KeySystemScene_Start(wk->scene, &sCygnusMsgSceneFuncs, HEAPID_KEY_SYSTEM);
        (*state)++;
        break;
    case SCENE_SEQ_WAIT:
        if (KeySystemScene_IsIdle(wk->scene)) {
            (*state)++;
        }
        break;
    case SCENE_SEQ_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void CygnusFlow_SeqStartReceiveScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case SCENE_SEQ_START:
        KeySystemScene_Start(wk->scene, &sReceiveSceneFuncs, HEAPID_KEY_SYSTEM);
        (*state)++;
        break;
    case SCENE_SEQ_WAIT:
        if (KeySystemScene_IsIdle(wk->scene)) {
            (*state)++;
        }
        break;
    case SCENE_SEQ_END:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void CygnusFlow_MenuSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    KeySystemBG_LoadScreen(wk->bg, 2, 4);
    KeySystemMsgWin_PrintMsg(wk->titleWin, wk->msgData, AbilityHandlers2_Text_Nintendo3dsLink, KEY_SYSTEM_MSG_PRINT);
    KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_Empty, KEY_SYSTEM_MSG_PRINT);
}

static BOOL CygnusFlow_MenuSceneMain(void *work) {
    KeySystemWork *wk = work;
    BOOL titleDone = KeySystemMsgWin_IsDone(wk->titleWin);
    BOOL msgDone = KeySystemMsgWin_IsDone(wk->msgWin);

    if (titleDone && msgDone) {
        GFL_BGSysQueueScrLoad(0);
        GFL_BGSysQueueScrLoad(2);
        return TRUE;
    }
    return FALSE;
}

static void CygnusFlow_MenuSceneExit(void *work) {
}

static void CygnusFlow_MsgSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_Empty, KEY_SYSTEM_MSG_PRINT);
}

static BOOL CygnusFlow_MsgSceneMain(void *work) {
    KeySystemWork *wk = work;

    if (KeySystemMsgWin_IsDone(wk->msgWin)) {
        GFL_BGSysQueueScrLoad(0);
        return TRUE;
    }
    return FALSE;
}

static void CygnusFlow_MsgSceneExit(void *work) {
}

static void CygnusFlow_ReceiveSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    ClActor_SetVisible(KeySystemClAct_Create(&wk->clact, 24, heapId), TRUE);
    KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_Empty, KEY_SYSTEM_MSG_PRINT);
}

static BOOL CygnusFlow_ReceiveSceneMain(void *work) {
    KeySystemWork *wk = work;

    if (KeySystemMsgWin_IsDone(wk->msgWin)) {
        GFL_BGSysQueueScrLoad(0);
        return TRUE;
    }
    return FALSE;
}

static void CygnusFlow_ReceiveSceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystemClAct_Delete(&wk->clact, 24);
}

static CygnusAppear *CygnusAppear_Create(KeySystemParticle *particle, KeySystemGraphic *graphic, HeapID heapId) {
    CygnusAppear *appear = GFL_HeapAllocate(heapId, sizeof(CygnusAppear), TRUE, "cygnus_flow.c", 1265);

    appear->particle = particle;
    appear->heapId = heapId;
    appear->offset = 0;
    appear->unk10C = 0;
    appear->unit = KeySystemGraphic_GetClActUnit(graphic);
    return appear;
}

static void CygnusAppear_Start(CygnusAppear *appear) {
    appear->done = FALSE;
    appear->state = APPEAR_STATE_LOAD;
    appear->timer = 0;
    appear->func = CygnusAppear_Run;
}

static void CygnusAppear_Free(CygnusAppear *appear) {
    GFL_HeapFree(appear);
}

static void CygnusAppear_Update(CygnusAppear *appear) {
    if (appear->func != NULL) {
        appear->func(appear);
        if (appear->done) {
            appear->func = NULL;
        }
    }
}

static BOOL CygnusAppear_IsDone(CygnusAppear *appear) {
    return appear->done;
}

static void CygnusAppear_Load(CygnusAppear *appear, PartyPkm *pkm) {
    ArcTool *arc = MakePokeGraArcHandle(appear->heapId);
    ClActorSetup setup;

    appear->palette =
        PokeGra_LoadClActPaletteByBoxData(arc, PokeParty_ToBoxPkm(pkm), POKEGRA_DIR_FRONT, 0, 0x80, appear->heapId);
    appear->cellAnims =
        PokeGra_LoadClActCellAnimsByBoxData(PokeParty_ToBoxPkm(pkm), POKEGRA_DIR_FRONT, 2, 0, appear->heapId);
    appear->chars = PokeGra_LoadClActCharsByBoxData(arc, PokeParty_ToBoxPkm(pkm), POKEGRA_DIR_FRONT, 0, appear->heapId);
    appear->offset = CygnusAppear_GetSpriteBottom(PokeParty_ToBoxPkm(pkm), appear->heapId) - 48;
    appear->offset = MATH_CLAMP(appear->offset, 0, 48);
    GFL_ArcToolFree(arc);
    MI_CpuCopy8((void *)(HW_OBJ_PLTT + CYGNUS_APPEAR_PALETTE * 0x20), appear->spritePalette,
                sizeof(appear->spritePalette));
    MI_CpuFill8(&setup, 0, sizeof(ClActorSetup));
    setup.x = 0;
    setup.y = 0;
    setup.bgPriority = 0;
    appear->actor =
        ClActor_Create(appear->unit, appear->chars, appear->palette, appear->cellAnims, &setup, 0, appear->heapId);
    ClActor_SetVisible(appear->actor, FALSE);
    ClActor_SetObjMode(appear->actor, GX_OAM_MODE_XLU);
}

static void CygnusAppear_Unload(CygnusAppear *appear) {
    if (appear->actor != NULL) {
        ClActor_Delete(appear->actor);
    }
    ClActSys_FreePltt(appear->palette);
    ClActSys_FreeChars(appear->chars);
    ClActSys_FreeCellAnims(appear->cellAnims);
}

static void CygnusAppear_Run(CygnusAppear *appear) {
    ClActorPos pos;
    u32 alpha;

    switch (appear->state) {
    case APPEAR_STATE_LOAD:
        KeySystemParticle_Load(appear->particle, ARCID_KEY_SYSTEM, 15, appear->heapId);
        appear->state = APPEAR_STATE_FLASH;
        break;
    case APPEAR_STATE_FLASH:
        appear->timer = 0;
        appear->state = APPEAR_STATE_WAIT_FLASH;
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0, 0x1f, 0, 16);
        MIi_CpuClear16(0x7fff, (void *)(HW_OBJ_PLTT + CYGNUS_APPEAR_PALETTE * 0x20), 0x20);
        MI_CpuCopy8((void *)(HW_OBJ_PLTT + CYGNUS_APPEAR_PALETTE * 0x20), appear->whitePalette,
                    sizeof(appear->whitePalette));
        KeySystemParticle_Emit(appear->particle, 0);
        KeySystemParticle_Emit(appear->particle, 1);
        KeySystemParticle_Emit(appear->particle, 2);
        GFL_SndSEPlay(SEQ_SE_SYS_88);
        break;
    case APPEAR_STATE_WAIT_FLASH:
        if (appear->timer++ > 120) {
            GFL_SndSEPlay(SEQ_SE_SYS_89);
            appear->timer = 0;
            appear->state = APPEAR_STATE_SHOW;
        }
        break;
    case APPEAR_STATE_SHOW:
        pos.x = 128;
        pos.y = 118 - appear->offset;
        ClActor_SetSurfacePos(appear->actor, &pos, 0);
        ClActor_SetVisible(appear->actor, TRUE);
        appear->fade = 0;
        appear->state = APPEAR_STATE_FADE_IN;
        break;
    case APPEAR_STATE_FADE_IN:
        alpha = (appear->fade * 16) / 10;
        CygnusFlow_SetBlendAlpha(alpha, 16 - alpha);
        appear->timer++;
        if (appear->fade++ > 10) {
            appear->fade = 0;
            appear->state = APPEAR_STATE_HOLD;
        }
        break;
    case APPEAR_STATE_HOLD:
        if (appear->timer++ > 90) {
            reg_G2_BLDCNT = 0;
            appear->timer = 0;
            appear->state = APPEAR_STATE_PAUSE;
        }
        break;
    case APPEAR_STATE_PAUSE:
        if (appear->timer++ > 10) {
            appear->timer = 0;
            appear->state = APPEAR_STATE_WHITEN;
        }
        break;
    case APPEAR_STATE_WHITEN:
        appear->blend = (appear->timer * 0x7fff) / 120;
        KeySystem_BlendPalette(14, appear->blendedPalette, appear->blend, CYGNUS_APPEAR_PALETTE, appear->spritePalette,
                               appear->whitePalette);
        if (appear->timer++ > 120) {
            appear->timer = 0;
            appear->state = APPEAR_STATE_WAIT_END;
        }
        break;
    case APPEAR_STATE_WAIT_END:
        if (appear->timer++ > 60) {
            appear->timer = 0;
            appear->state = APPEAR_STATE_END;
        }
        break;
    case APPEAR_STATE_END:
        reg_G2_BLDCNT = 0;
        appear->done = TRUE;
        break;
    }
}

static CygnusData *CygnusData_Create(GameData *gameData, HeapID heapId) {
    CygnusData *data = GFL_HeapAllocate(heapId, sizeof(CygnusData), TRUE, "cygnus_flow.c", 1768);

    data->gameData = gameData;
    data->buffer = DreamRadar_AllocBuffer(heapId);
    return data;
}

static void CygnusData_Free(CygnusData *data) {
    DreamRadar_FreeBuffer(data->buffer);
    GFL_HeapFree(data);
}

static void CygnusData_Load(CygnusData *data, HeapID heapId) {
    u32 words[CYGNUS_DATA_WORD_MAX];

    ReadDreamRadarSaveData(data->buffer);
    data->radarData = DreamRadar_GetSaveData(data->buffer);
    data->save = GetDreamRadarSaveBlock(GameData_GetSaveControl(data->gameData));
    data->received = GetDreamRadarFlag(data->save, DREAM_RADAR_FLAG_RECEIVED);
    data->status = CygnusSaveData_Load(data->radarData, GetDreamRadarFlag(data->save, DREAM_RADAR_FLAG_SEED), words);
    if (data->status == CYGNUS_DATA_VALID && words[CYGNUS_DATA_WORD_MAX - 1] == 0) {
        CygnusData_ReadEntries(data, words, heapId);
    }
}

static u32 CygnusData_GetState(CygnusData *data) {
    BOOL read = GetDreamRadarFlag(data->save, DREAM_RADAR_FLAG_READ);
    BOOL empty = FALSE;
    BOOL valid;

    if (data->count == 0) {
        empty = TRUE;
    }
    valid = data->status == CYGNUS_DATA_VALID;
    if (data->status == CYGNUS_DATA_INVALID || data->status == CYGNUS_DATA_BROKEN || (empty && valid)) {
        return CYGNUS_STATE_INVALID;
    }
    if (!empty && valid) {
        return CYGNUS_STATE_NEW;
    }
    if (read) {
        return CYGNUS_STATE_RECEIVED;
    }
    return CYGNUS_STATE_NONE;
}

static BOOL CygnusData_HasLegend(const CygnusData *data) {
    u32 i;

    for (i = 0; i < data->count; i++) {
        if (data->entries[i].legend) {
            return TRUE;
        }
    }
    return FALSE;
}

static int CygnusData_FindLegend(const CygnusData *data, u32 n) {
    u32 i;
    u32 found = 0;

    for (i = 0; i < data->count; i++) {
        if (data->entries[i].type == CYGNUS_ENTRY_POKEMON && data->entries[i].legend) {
            if (found++ == n) {
                return i;
            }
        }
    }
    return -1;
}

static void CygnusData_Commit(CygnusData *data) {
    u32 seed;

    seed = GFL_RandomLC(0);
    SetDreamRadarFlag(data->save, DREAM_RADAR_FLAG_SEED, seed);
    SetDreamRadarFlag(data->save, DREAM_RADAR_FLAG_READ, TRUE);
    CygnusSaveData_Store(data->radarData, seed, data->received);
    SetDreamRadarFlag(data->save, DREAM_RADAR_FLAG_RECEIVED, data->received);
    GCTX_HIDBlockSoftReset(8);
    data->saveState = SAVE_STATE_START;
}

static BOOL CygnusData_Save(CygnusData *data) {
    switch (data->saveState) {
    case SAVE_STATE_START:
        GameData_StartSave(data->gameData);
        data->saveState++;
        // fallthrough
    case SAVE_STATE_WAIT:
        if (GameData_UpdateSave(data->gameData) != SAVE_RESULT_OK) {
            break;
        }
        DreamRadar_WriteSaveData();
        data->saveState++;
        // fallthrough
    case SAVE_STATE_DONE:
        GCTX_HIDUnblockSoftReset(8);
        return TRUE;
    }
    return FALSE;
}

static BOOL CygnusData_GetEntry(const CygnusData *data, u32 index, CygnusEntryType *type, u32 *id) {
    if (data->entries[index].type != CYGNUS_ENTRY_NONE) {
        *type = data->entries[index].type;
        *id = data->entries[index].id;
        return TRUE;
    }
    return FALSE;
}

static PartyPkm *CygnusData_CreatePokemon(const CygnusData *data, u32 index, HeapID heapId) {
    if (data->entries[index].type == CYGNUS_ENTRY_POKEMON) {
        u8 sex = data->entries[index].sex;
        u16 species = data->entries[index].id;
        u8 form = data->entries[index].form;
        u32 id = getIDAsUInt(GetGameDataPlayerInfo(data->gameData));
        u32 pid = PML_GenPID(id, species, form, sex, 2, PID_RARE_NEVER);
        int badges = getBadgeCount(getTrainerCardDataBlkAddress(data->gameData));
        u8 level = sLevelsByBadges[MATH_CLAMP(badges, 0, NELEMS(sLevelsByBadges) - 1)];
        PartyPkm *pkm = PokeParty_NewTempPkm(species, level, id, heapId);
        PersonalData *personal;

        PokeParty_CreatePkm(pkm, species, level, id, -1, pid);
        PokeParty_ChangeForme(pkm, form);
        PokeParty_SetDefaultMoves(pkm);
        personal = PML_PersonalLoad(species, form, heapId);
        if (PML_PersonalGetParam(personal, PERSONAL_ABILITY_HIDDEN)) {
            PokeParty_SetHiddenAbil(pkm, species, form);
        }
        PML_PersonalFree(personal);
        PokeParty_SetParam(pkm, PKM_PARAM_OT_NAME_RAW, (u32)GetPlayerName(GetGameDataPlayerInfo(data->gameData)));
        PokeParty_SetParam(pkm, PKM_PARAM_OT_GENDER, getTrainerGender(GetGameDataPlayerInfo(data->gameData)));
        PokeParty_SetParam(pkm, PKM_PARAM_POKEBALL, 25);
        PokeParty_SetParam(pkm, PKM_PARAM_ORIGIN_GAME, game_version);
        PokeParty_SetParam(pkm, PKM_PARAM_REGION, region);
        setDreamRadarPokeMetInfo(PokeParty_GetBoxPkm(pkm));
        PokeParty_RecalcStats(pkm);
        if (PokeParty_GetParam(pkm, PKM_PARAM_BAD_EGG, NULL)) {
            GFL_HeapFree(pkm);
            return NULL;
        }
        return pkm;
    }
    return NULL;
}

static BOOL CygnusData_GetItem(const CygnusData *data, u32 index, u16 *item, u8 *quantity) {
    if (data->entries[index].type == CYGNUS_ENTRY_ITEM) {
        *item = data->entries[index].id;
        *quantity = data->entries[index].quantity;
        return TRUE;
    }
    return FALSE;
}

// Which of the items an entry is, counting from 1
static u32 CygnusData_GetItemNumber(const CygnusData *data, u32 n) {
    u32 i;
    u32 found = 0;

    for (i = 0; i < data->count; i++) {
        if (data->entries[i].type == CYGNUS_ENTRY_ITEM) {
            if (found++ == n) {
                return found;
            }
        }
    }
    return 0;
}

static u32 CygnusData_CountPokemon(const CygnusData *data) {
    u32 i;
    u32 count = 0;

    for (i = 0; i < data->count; i++) {
        if (data->entries[i].type == CYGNUS_ENTRY_POKEMON) {
            count++;
        }
    }
    return count;
}

static u32 CygnusData_CountItems(const CygnusData *data) {
    u32 i;
    u32 count = 0;

    for (i = 0; i < data->count; i++) {
        if (data->entries[i].type == CYGNUS_ENTRY_ITEM) {
            count++;
        }
    }
    return count;
}

static u32 CygnusData_GetCount(const CygnusData *data) {
    return data->count;
}

// Puts the Pokémon in the boxes and the items in the bag
static void CygnusData_Receive(CygnusData *data, HeapID heapId) {
    BoxSaveAccessor *boxes = GameData_GetBoxSaveAccessor(data->gameData);
    PokeDexSave *pokedex = GameData_GetPokedex(data->gameData);
    BagSave *bag;
    u32 i;
    u8 quantity;
    u16 item;

    for (i = 0; i < data->count; i++) {
        PartyPkm *pkm = CygnusData_CreatePokemon(data, i, heapId);

        if (pkm != NULL) {
            PlayerInfo *player = GetGameDataPlayerInfo(data->gameData);

            if (PokeParty_IsSpecialTransfer(pkm, 8, player)) {
                setOneShotDRObtained(getTrainerCardDataBlkAddress(data->gameData), 7, player);
            }
            addPkmToDex(pokedex, pkm);
            BoxSaveAccessor_InsertPkm(boxes, PokeParty_GetBoxPkm(pkm));
            GFL_HeapFree(pkm);
        }
    }
    bag = GameData_GetBag(data->gameData);
    for (i = 0; i < data->count; i++) {
        if (CygnusData_GetItem(data, i, &item, &quantity)) {
            BagSave_AddItem(bag, item, quantity, heapId);
        }
    }
}

static inline void CygnusData_SwapEntries(CygnusData *data, s16 a, s16 b) {
    CygnusEntry entry = data->entries[a];

    data->entries[a] = data->entries[b];
    data->entries[b] = entry;
}

static void CygnusData_ReadEntries(CygnusData *data, const u32 *words, HeapID heapId) {
    u16 species;
    u8 form;
    u8 sex;
    int i;
    int j;

    MI_CpuFill8(data->entries, 0, sizeof(data->entries));
    data->count = 0;
    for (i = 0; i <= CYGNUS_WORD_POKEMON_END - 1; i++) {
        if (DreamRadar_IsPokemonWord(words[i])) {
            u32 bit = DreamRadar_GetPokemonBit(words[i]);

            if (!(data->received & bit)) {
                u8 radarForm;
                u8 radarSex;
                u16 radarSpecies;

                DreamRadar_GetPokemonByWord(words[i], &radarSpecies, &radarForm, &radarSex);
                data->entries[data->count].type = CYGNUS_ENTRY_POKEMON;
                data->entries[data->count].id = radarSpecies;
                data->entries[data->count].form = radarForm;
                data->entries[data->count].sex = radarSex;
                data->entries[data->count].legend = CygnusData_IsLegend(radarSpecies);
                data->count++;
                data->received |= bit;
            }
        }
    }
    // The legendary Pokémon go first
    if (data->count != 0) {
        for (j = 0; j < data->count - 1; j++) {
            for (i = data->count - 1; i > j; i--) {
                s16 prev = i - 1;
                s16 cur = i;

                if (data->entries[prev].legend < data->entries[cur].legend) {
                    CygnusData_SwapEntries(data, prev, cur);
                }
            }
        }
    }
    for (i = CYGNUS_WORD_POKEMON_END; i <= CYGNUS_WORD_FREE_POKEMON_END - 1; i++) {
        if (data->count >= CYGNUS_FREE_POKEMON_MAX) {
            break;
        }
        species = words[i] >> 16;
        form = words[i] >> 8;
        sex = words[i];
        if (CygnusData_IsPokemonValid(species, form, sex, heapId)) {
            data->entries[data->count].type = CYGNUS_ENTRY_POKEMON;
            data->entries[data->count].id = species;
            data->entries[data->count].form = form;
            data->entries[data->count].sex = sex;
            data->entries[data->count].legend = FALSE;
            data->count++;
        }
    }
    for (i = CYGNUS_WORD_FREE_POKEMON_END; i <= CYGNUS_WORD_ITEM_END - 1; i++) {
        u16 item;
        u16 quantity;

        if (data->count >= CYGNUS_ENTRY_MAX) {
            break;
        }
        item = words[i] >> 16;
        quantity = words[i];
        if (item != 0 && item < 638 && quantity != 0) {
            data->entries[data->count].type = CYGNUS_ENTRY_ITEM;
            data->entries[data->count].id = item;
            data->entries[data->count].quantity = quantity;
            data->entries[data->count].form = 0;
            data->entries[data->count].sex = 0;
            data->entries[data->count].legend = FALSE;
            data->count++;
        }
    }
}

static BOOL CygnusData_IsPokemonValid(u16 species, u8 form, u8 sex, HeapID heapId) {
    PersonalData *personal;
    u32 formCount;
    u32 sexRatio;

    if (species > 649 || species == 0) {
        return FALSE;
    }
    personal = PML_PersonalLoad(species, 0, heapId);
    formCount = PML_PersonalGetParam(personal, PERSONAL_FORM_COUNT);
    sexRatio = PML_PersonalGetParam(personal, PERSONAL_SEX_RATIO);
    PML_PersonalFree(personal);
    if (form > formCount) {
        return FALSE;
    }
    switch (sexRatio) {
    case GENDER_RATIO_MALE_ONLY:
        if (sex != GENDER_MALE) {
            return FALSE;
        }
        break;
    case GENDER_RATIO_GENDERLESS:
        if (sex != GENDER_UNKNOWN) {
            return FALSE;
        }
        break;
    case GENDER_RATIO_FEMALE_ONLY:
        if (sex != GENDER_FEMALE) {
            return FALSE;
        }
        break;
    default:
        if (sex != GENDER_MALE && sex != GENDER_FEMALE) {
            return FALSE;
        }
        break;
    }
    return TRUE;
}

static u16 CygnusData_IsLegend(u16 species) {
    u32 i;

    for (i = 0; i < CYGNUS_LEGEND_COUNT; i++) {
        if (species == sLegends[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

static CygnusIcons *CygnusIcons_Create(KeySystemGraphic *graphic, CygnusData *data, HeapID heapId) {
    CygnusIcons *icons = GFL_HeapAllocate(heapId, sizeof(CygnusIcons), TRUE, "cygnus_flow.c", 2610);

    icons->heapId = heapId;
    icons->unit = KeySystemGraphic_GetClActUnit(graphic);
    icons->data = data;
    return icons;
}

static void CygnusIcons_Free(CygnusIcons *icons) {
    GFL_HeapFree(icons);
}

static void CygnusIcons_Update(CygnusIcons *icons) {
    u32 count = CygnusData_GetCount(icons->data);

    if (!icons->active) {
        return;
    }
    if (icons->current >= count) {
        icons->done = TRUE;
        return;
    }
    switch (icons->state) {
    case ICONS_STATE_START:
        CygnusIcon_Start(&icons->icons[icons->current]);
        icons->state++;
        // fallthrough
    case ICONS_STATE_FALL:
        if (CygnusIcon_Update(&icons->icons[icons->current])) {
            GFL_SndSEPlay(SEQ_SE_MSCL_04);
            icons->state++;
            icons->current++;
        }
        break;
    case ICONS_STATE_NEXT:
        icons->state = ICONS_STATE_START;
        break;
    }
}

static void CygnusIcons_Start(CygnusIcons *icons) {
    icons->active = TRUE;
    icons->done = FALSE;
    icons->state = ICONS_STATE_START;
    icons->current = 0;
}

static BOOL CygnusIcons_IsDone(CygnusIcons *icons) {
    return icons->done;
}

static void CygnusIcons_Load(CygnusIcons *icons) {
    u32 count = CygnusData_GetCount(icons->data);
    u32 i;

    for (i = 0; i < count; i++) {
        CygnusIcon_Load(&icons->icons[i], icons->unit, icons->data, i, icons->heapId);
    }
    icons->active = FALSE;
    icons->done = FALSE;
}

static void CygnusIcons_Unload(CygnusIcons *icons) {
    u32 count = CygnusData_GetCount(icons->data);
    u32 i;

    for (i = 0; i < count; i++) {
        CygnusIcon_Unload(&icons->icons[i]);
    }
}

static void CygnusIcon_Load(CygnusIcon *icon, ClActUnit *unit, CygnusData *data, u16 index, HeapID heapId) {
    CygnusEntryType type;
    u32 id;
    int x = 0;
    s16 y = 0;
    u32 iconPalette = 0;
    int rowGap;
    int row;
    int posY;
    ClActorSetup setup;

    MI_CpuFill8(icon, 0, sizeof(CygnusIcon));
    CygnusData_GetEntry(data, index, &type, &id);
    switch (type) {
    case CYGNUS_ENTRY_POKEMON:
    default: {
        PartyPkm *pkm = CygnusData_CreatePokemon(data, index, heapId);
        ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, heapId);
        u32 cells;

        icon->palette = ClActSys_LoadPlttEx(arc, PokeIcon_GetPaletteFile(), 0, 0x80, heapId);
        cells = PokeIcon_GetCellFile_Main();
        icon->cellAnims = ClActSys_LoadCellAnims(arc, cells, getOBJTileMapping_MainEng(), heapId);
        icon->chars = ClActSys_LoadChars(arc, PokeIcon_GetFileOfPkm(PokeParty_GetBoxPkm(pkm)), FALSE, 0, heapId);
        iconPalette = PokeIcon_GetPaletteOfPkm(PokeParty_GetBoxPkm(pkm));
        GFL_ArcToolFree(arc);
        GFL_HeapFree(pkm);
        break;
    }
    case CYGNUS_ENTRY_ITEM: {
        ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ITEMGRA, heapId);
        u32 number = CygnusData_GetItemNumber(data, index - CygnusData_CountPokemon(data));

        icon->palette = ClActSys_LoadPlttRange(arc, GetItemGraphicsDatID(id, ITEM_FILE_ICON_PLTT), x,
                                               (number + 7) * 0x20, 0, 1, heapId);
        icon->cellAnims = ClActSys_LoadCellAnims(arc, 1, x, heapId);
        icon->chars = ClActSys_LoadChars(arc, GetItemGraphicsDatID(id, ITEM_FILE_ICON_CHAR), x, x, heapId);
        GFL_ArcToolFree(arc);
        x = 4;
        y = 8;
        break;
    }
    }
    MI_CpuFill8(&setup, 0, sizeof(ClActorSetup));
    rowGap = 0;
    icon->pos.x = (index % 6) * 32 + 48 + x;
    row = index / 6;
    if (row != 0) {
        rowGap = 16;
    }
    posY = y + (row * 24 + 56);
    icon->pos.y = posY + rowGap;
    setup.x = icon->pos.x;
    setup.y = icon->pos.y - 16;
    setup.bgPriority = 0;
    icon->actor = ClActor_Create(unit, icon->chars, icon->palette, icon->cellAnims, &setup, 0, heapId);
    ClActor_SetPalette(icon->actor, (u8)iconPalette, 0);
    ClActor_SetVisible(icon->actor, FALSE);
    ClActor_SetObjMode(icon->actor, GX_OAM_MODE_XLU);
}

static void CygnusIcon_Unload(CygnusIcon *icon) {
    ClActor_Delete(icon->actor);
    ClActSys_FreePltt(icon->palette);
    ClActSys_FreeChars(icon->chars);
    ClActSys_FreeCellAnims(icon->cellAnims);
    MI_CpuFill8(icon, 0, sizeof(CygnusIcon));
}

static void CygnusIcon_Start(CygnusIcon *icon) {
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, 0, 0xf, 0, 16);
    ClActor_SetVisible(icon->actor, TRUE);
    icon->falling = TRUE;
    icon->startTime = OS_TicksToMilliSeconds(OS_GetTick());
}

static BOOL CygnusIcon_Update(CygnusIcon *icon) {
    u64 elapsed;
    ClActorPos pos;
    u64 alpha;

    if (icon->falling) {
        elapsed = OS_TicksToMilliSeconds(OS_GetTick()) - icon->startTime;
        if (elapsed > CYGNUS_ICON_TIME) {
            ClActor_SetSurfacePos(icon->actor, &icon->pos, 0);
            ClActor_SetObjMode(icon->actor, GX_OAM_MODE_NORMAL);
            reg_G2_BLDCNT = 0;
            return TRUE;
        }
        pos.x = icon->pos.x;
        alpha = (elapsed * 16) / CYGNUS_ICON_TIME;
        pos.y = icon->pos.y - 16 + alpha;
        ClActor_SetSurfacePos(icon->actor, &pos, 0);
        CygnusFlow_SetBlendAlpha(alpha, 16 - alpha);
    }
    return FALSE;
}

static void CygnusFlow_CreateAppear(KeySystemWork *wk, HeapID heapId) {
    KeySystemGraphic_Set3D(wk->graphic, KEY_SYSTEM_GRAPHIC_3D_ON);
    wk->particle = KeySystemParticle_Create(heapId);
    wk->cygnusAppear = CygnusAppear_Create(wk->particle, wk->graphic, heapId);
}

static void CygnusFlow_FreeAppear(KeySystemWork *wk) {
    CygnusAppear_Free(wk->cygnusAppear);
    wk->cygnusAppear = NULL;
    KeySystemParticle_Free(wk->particle);
    wk->particle = NULL;
    KeySystemGraphic_Set3D(wk->graphic, KEY_SYSTEM_GRAPHIC_3D_OFF);
}

static void CygnusFlow_HideScreen(KeySystemWork *wk, HeapID heapId) {
    GFL_BGSysClearScr(2);
    KeySystem_FreeTitleWin(wk);
    KeySystem_FreeMsgWin(wk);
    LoadSysMsgBox(2, 1, 15, 0, HEAPID_KEY_SYSTEM);
    KeySystemBG_StartFade(wk->bg, KEY_SYSTEM_BG_FADE_4_TO_3, 1);
}

static void CygnusFlow_ShowScreen(KeySystemWork *wk) {
    GFL_BGSysLoadNCGRStatic(ARCID_KEY_SYSTEM, 3, 2, 0, 0, FALSE, HEAPID_KEY_SYSTEM);
    KeySystemBG_LoadScreen(wk->bg, 2, 4);
    KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
    KeySystem_CreateTitleWin(wk, 21, HEAPID_KEY_SYSTEM);
    KeySystemMsgWin_PrintMsg(wk->titleWin, wk->msgData, AbilityHandlers2_Text_Nintendo3dsLink, KEY_SYSTEM_MSG_PRINT);
    KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, AbilityHandlers2_Text_Empty, KEY_SYSTEM_MSG_PRINT);
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(2);
    KeySystemBG_StartFade(wk->bg, KEY_SYSTEM_BG_FADE_3_TO_4, 1);
}

// Whether the Pokémon and items can be received, and if not, the message that says why
static u32 CygnusFlow_GetState(KeySystemWork *wk, u32 *msgId) {
    u32 unused;
    SaveControl *save;
    EventWork *eventWork;
    u32 dataState;

    if (msgId == NULL) {
        msgId = &unused;
    }
    save = GameData_GetSaveControl(wk->param->gameData);
    eventWork = GameData_GetEventWork(wk->param->gameData);
    if (SaveControl_IsDataAlreadyPresent(save)) {
        if (!EventWork_FlagGet(eventWork, CYGNUS_FLAG)) {
            *msgId = AbilityHandlers2_Text_ReceiveResearchDataPlay;
            return CYGNUS_FLOW_CANT_RECEIVE;
        }
    } else {
        *msgId = AbilityHandlers2_Text_ResearchDataCannotSent;
        return CYGNUS_FLOW_CANT_RECEIVE;
    }
    if (CygnusData_GetState(wk->cygnusData) == CYGNUS_STATE_INVALID) {
        return CYGNUS_FLOW_NOTHING;
    }
    dataState = CygnusData_GetState(wk->cygnusData);
    if (dataState == CYGNUS_STATE_NONE) {
        *msgId = AbilityHandlers2_Text_ThereNoResearchData;
        return CYGNUS_FLOW_CANT_RECEIVE;
    }
    if (dataState == CYGNUS_STATE_RECEIVED) {
        *msgId = AbilityHandlers2_Text_ThereNoResearchData_2;
        return CYGNUS_FLOW_CANT_RECEIVE;
    }
    if (BoxSaveAccessor_CountEmptySlots(GameData_GetBoxSaveAccessor(wk->param->gameData)) < 6) {
        *msgId = AbilityHandlers2_Text_ReceiveResearchDataMust;
        return CYGNUS_FLOW_CANT_RECEIVE;
    }
    return CYGNUS_FLOW_RECEIVE;
}

// The lowest row of the sprite that has a pixel, at least 48
static int CygnusAppear_GetSpriteBottom(BoxPkm *pkm, HeapID heapId) {
    NNSG2dCharacterData *charData;
    u32 bottom = 48;
    BOOL found = FALSE;
    void *buffer = LoadSingleCellSpindaGraphicsByBoxData(&charData, pkm, POKEGRA_DIR_FRONT, HEAPID_TAIL(heapId));
    int tileY;

    PokeGra_CellCharsToImage(charData, HEAPID_TAIL(heapId));
    for (tileY = 11; tileY >= 0; tileY--) {
        int tileX;

        for (tileX = 0; tileX < 12; tileX++) {
            int y;
            u32 *tile = &((u32 *)charData->pRawData)[(tileX + tileY * 12) * 8];

            for (y = 7; y >= 0; y--) {
                if (tile[y] != 0) {
                    if (bottom < y + tileY * 8) {
                        bottom = y + tileY * 8;
                    }
                    found = TRUE;
                }
            }
        }
        if (found) {
            break;
        }
    }
    GFL_HeapFree(buffer);
    return bottom;
}
