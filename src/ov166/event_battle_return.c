// Named after the string event_battle_return.c the overlay embeds
#include "types.h"
#include "app/event_battle_return.h"
#include "app/event_pdc_return.h"
#include "app/name_entry.h"
#include "battle/btl_setup.h"
#include "constants/abilities.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "demo/shinka_demo.h"
#include "field/burmy_form.h"
#include "field/player_state.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "pml/evolution.h"
#include "pml/met_data.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/event_work.h"
#include "save/player_info.h"
#include "save/pokedex.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/game_data.h"

// The battle types of the field's wild and trainer battles
#define BATTLE_TYPE_WILD 0
#define BATTLE_TYPE_TRAINER 1

// BtlSetup's unkA8: the player won, or caught the wild Pokémon
#define BATTLE_RESULT_WIN 1
#define BATTLE_RESULT_CAPTURE 5

// The battle backgrounds, by BtlFieldEnv's bgType, with the terrain Burmy takes its form from for each of its
// terrains
#define ARCID_BATTLE_BG_DATA 0x97

// Set once the player knows whose PC the boxes are
#define FLAG_BATTLE_RETURN_BOX_MESSAGE 0x96b

typedef struct {
    u8 unk00[0x16];
    u8 burmyTerrain[0x16];
} BattleBgData;

typedef struct {
    NameEntryParam *nameEntry;
    // The player's name, then the nickname
    StrBuf *name;
    // The caught Pokémon
    PartyPkm *pkm;
    ZukanTorokuParam zukan;
    // Where the Pokémon goes when the party is full
    StrBuf *boxMessage;
    u32 box;
    ShinkaDemoParam *shinkaDemo;
    // The party slot evolveMask's lowest bit stands for
    u16 evolveSlot;
    // The party slots whose Pokémon leveled up in the battle, which may evolve
    u16 evolveMask;
    u16 heapId;
    GameProcManager *procManager;
    // Set once the music is ready for the evolutions
    BOOL musicDone;
    BOOL musicFading;
} EventBattleReturnWork;

static BOOL EventBattleReturn_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL EventBattleReturn_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL EventBattleReturn_Main(GameProc *proc, u32 *state, void *param, void *work);
static void EventBattleReturn_CheckLevelUps(EventBattleReturnWork *wk, EventBattleReturnParam *prm);
static void EventBattleReturn_PickupItems(PokeParty *party);
static void EventBattleReturn_NaturalCure(PokeParty *party);
static void EventBattleReturn_BurmyForms(EventBattleReturnParam *prm, PokeParty *party, u32 terrain);
static u8 EventBattleReturn_GetBurmyTerrain(BtlFieldSituation *situation, HeapID heapId);
static void EventBattleReturn_ShayminForms(EventBattleReturnParam *prm, PokeParty *party);
static void EventBattleReturn_HealBall(PartyPkm *pkm);

// Pickup's chances, in percent, cumulative: an item of sPickupItems from the slot for the level onwards
static const u8 sPickupChances[9] = {30, 40, 50, 60, 70, 80, 90, 94, 98};

// Honey Gather's chance of Honey, in percent, by tens of levels
static const u8 sHoneyGatherChances[10] = {5, 10, 15, 20, 25, 30, 35, 40, 45, 50};

// Pickup's rare items, for the last two percent, from the slot for the level onwards
static const u16 sPickupRareItems[11] = {
    ITEM_HYPER_POTION, ITEM_NUGGET,      ITEM_KINGS_ROCK,  ITEM_FULL_RESTORE, ITEM_ETHER,       ITEM_IRON_BALL,
    ITEM_PRISM_SCALE,  ITEM_ELIXIR,      ITEM_PRISM_SCALE, ITEM_LEFTOVERS,    ITEM_PRISM_SCALE,
};

// Pickup's items, from the slot for the level onwards
static const u16 sPickupItems[18] = {
    ITEM_POTION,       ITEM_ANTIDOTE, ITEM_SUPER_POTION, ITEM_GREAT_BALL, ITEM_REPEL,       ITEM_ESCAPE_ROPE,
    ITEM_FULL_HEAL,    ITEM_HYPER_POTION, ITEM_ULTRA_BALL, ITEM_REVIVE,   ITEM_RARE_CANDY,  ITEM_SUN_STONE,
    ITEM_MOON_STONE,   ITEM_HEART_SCALE, ITEM_FULL_RESTORE, ITEM_MAX_REVIVE, ITEM_PP_UP,    ITEM_MAX_ELIXIR,
};

const GameProcFunctions EVENT_BATTLE_RETURN_PROC_FUNCTIONS = {
    EventBattleReturn_Init,
    EventBattleReturn_Main,
    EventBattleReturn_Exit,
};

static BOOL EventBattleReturn_Init(GameProc *proc, u32 *state, void *param, void *work) {
    EventBattleReturnWork *wk;

    GFL_HeapCreateChild(1, HEAPID_BATTLE_RETURN, 0x1000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(EventBattleReturnWork), HEAPID_BATTLE_RETURN);
    wk->name = GFL_StrBufCreate(32, HEAPID_BATTLE_RETURN);
    wk->nameEntry = NULL;
    wk->pkm = NULL;
    wk->boxMessage = NULL;
    wk->box = 0;
    wk->shinkaDemo = NULL;
    wk->heapId = HEAPID_BATTLE_RETURN;
    wk->procManager = CreateGameProcManager(wk->heapId);
    wk->musicDone = FALSE;
    wk->musicFading = FALSE;
    return TRUE;
}

static BOOL EventBattleReturn_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    EventBattleReturnWork *wk = work;

    FreeGameProcManager(wk->procManager);
    if (wk->boxMessage != NULL) {
        GFL_StrBufFree(wk->boxMessage);
    }
    GFL_StrBufFree(wk->name);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_BATTLE_RETURN);
    return TRUE;
}

static BOOL EventBattleReturn_Main(GameProc *proc, u32 *state, void *param, void *work) {
    EventBattleReturnWork *wk = work;
    EventBattleReturnParam *prm = param;
    BOOL procRunning;
    PokeParty *party;
    PlayerInfo *playerInfo;
    PokeDexSave *pokedex;
    BoxSaveAccessor *boxes;
    s32 money;
    MsgData *msgData;
    PlayerState *playerState;
    u32 metKind;
    BOOL nationalDex;
    BOOL firstCatch;
    int box;
    int slot;
    BOOL nickname;
    StrBuf *oldName;
    u32 species;
    u32 evolveSlot;
    PartyPkm *pkm;
    u32 method;
    ShinkaDemoParam *demo;

    procRunning = GFL_ProcMgrUpdate(wk->procManager);
    if (procRunning == TRUE) {
        return FALSE;
    }
    switch (*state) {
    case 0:
        party = GameData_GetParty(prm->gameData);
        playerInfo = GetGameDataPlayerInfo(prm->gameData);
        pokedex = GameData_GetPokedex(prm->gameData);
        boxes = GameData_GetBoxSaveAccessor(prm->gameData);
        wk->evolveSlot = 0;
        wk->evolveMask = 0;
        if (prm->setup->battleType <= BATTLE_TYPE_TRAINER) {
            EventBattleReturn_CheckLevelUps(wk, prm);
            PokeParty_Copy(prm->setup->party[0], party);
            pokerusHandler(party);
            pokerusSpread(party);
            if (prm->setup->unkA8 == BATTLE_RESULT_WIN) {
                EventBattleReturn_PickupItems(party);
            }
            EventBattleReturn_BurmyForms(prm, party, EventBattleReturn_GetBurmyTerrain(&prm->setup->fieldSituation, wk->heapId));
            EventBattleReturn_ShayminForms(prm, party);
            money = prm->setup->unkA4;
            if (money > 0) {
                addCashToTotal(getTrainerCardDataBlkAddress(prm->gameData), money);
            } else if (money < 0) {
                subCashFromTotal(getTrainerCardDataBlkAddress(prm->gameData), -money);
            }
        }
        EventBattleReturn_NaturalCure(party);
        if (prm->setup->unkA8 == BATTLE_RESULT_CAPTURE) {
            wk->pkm = PokeParty_GetPkm(prm->setup->party[1], prm->setup->unkAC);
            if (PokeParty_GetPkmCount(party) >= PokeParty_GetCapacity(party)) {
                box = BoxSaveAccessor_GetLastOpenedBox(boxes);
                slot = 0;
                BoxSaveAccessor_GetNextFreeBoxSlot(boxes, &box, &slot);
                wk->box = box;
                msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_SERVER_FLOW_TITLE, HEAPID_BATTLE_RETURN);
                wk->boxMessage = GFL_MsgDataLoadStrbufNew(
                    msgData,
                    EventWork_FlagGet(GameData_GetEventWork(prm->gameData), FLAG_BATTLE_RETURN_BOX_MESSAGE) ? 0xb2 : 0xb1);
                GFL_MsgDataFree(msgData);
            }
            playerState = GameData_GetPlayerState(prm->gameData);
            metKind = 0;
            if (PokeParty_GetParam(wk->pkm, PKM_PARAM_N_POKEMON, NULL)) {
                metKind = 7;
            }
            PokeParty_SetupMetData(wk->pkm, metKind, playerInfo,
                                   ZoneData_GetPlaceNameID(PlayerState_GetZoneID(playerState)), wk->heapId);
            nationalDex = PokeDex_IsNationalObtained(pokedex);
            firstCatch = FALSE;
            if (!PokeDex_IsCaught(pokedex, PokeParty_GetParam(wk->pkm, PKM_PARAM_SPECIES, NULL))) {
                firstCatch = TRUE;
            }
            PokeDex_RegistPkm(pokedex, wk->pkm);
            addPkmToDex(pokedex, wk->pkm);
            wk->zukan.pkm = wk->pkm;
            wk->zukan.nationalDex = nationalDex;
            wk->zukan.boxMessage = wk->boxMessage;
            wk->zukan.boxes = boxes;
            wk->zukan.box = wk->box;
            wk->zukan.gameData = prm->gameData;
            if (firstCatch) {
                wk->zukan.mode = 0;
            } else {
                wk->zukan.mode = 1;
            }
            QueueGameProc(wk->procManager, OVERLAY_ID(297), &data_ov297_021f4e38, &wk->zukan);
            (*state)++;
        } else {
            *state = 4;
        }
        break;
    case 1:
        if (procRunning != TRUE) {
            boxes = GameData_GetBoxSaveAccessor(prm->gameData);
            nickname = FALSE;
            if (wk->zukan.nickname == TRUE) {
                nickname = TRUE;
            }
            if (nickname) {
                wk->nameEntry = pokemonNameEntry(wk->heapId, wk->pkm, 10, NULL, (u32)wk->boxMessage, (u32)boxes, wk->box,
                                                 getTrainerGameInfoAddress(GameData_GetSaveControl(prm->gameData)));
                QueueGameProc(wk->procManager, OVERLAY_ID(280), &NAME_ENTRY_PROC_FUNCTIONS, wk->nameEntry);
                (*state)++;
            } else {
                *state = 3;
            }
        } else {
            return FALSE;
        }
        break;
    case 2:
        if (procRunning != TRUE) {
            if (func_ov012_02165b0c(wk->nameEntry) == FALSE) {
                oldName = GFL_StrBufCreate(32, wk->heapId);
                func_ov012_02165afc(wk->nameEntry, wk->name);
                PokeParty_GetParam(wk->pkm, PKM_PARAM_NICKNAME, oldName);
                PokeParty_SetParam(wk->pkm, PKM_PARAM_NICKNAME, (u32)wk->name);
                if (func_ov012_02165b10(wk->nameEntry, oldName) == FALSE) {
                    RecordAddOne(GameData_GetRecords(prm->gameData), 0x1e);
                }
                GFL_StrBufFree(oldName);
            }
            func_ov012_02165ae8(wk->nameEntry);
            wk->nameEntry = NULL;
            (*state)++;
        } else {
            return FALSE;
        }
        break;
    case 3:
        party = GameData_GetParty(prm->gameData);
        EventBattleReturn_HealBall(wk->pkm);
        if (wk->boxMessage == NULL) {
            PokeParty_AddPkm(party, wk->pkm);
        } else {
            BoxSaveAccessor_InsertPkm(GameData_GetBoxSaveAccessor(prm->gameData), func_0201d620(wk->pkm));
        }
        (*state)++;
        break;
    case 4:
        if (!wk->musicDone) {
            if (prm->setup->unkA8 == BATTLE_RESULT_CAPTURE && wk->evolveMask != 0) {
                if (!wk->musicFading) {
                    GFL_SndBGMFadeOut(30);
                    wk->musicFading = TRUE;
                } else if (!GFL_SndBGMIsFading()) {
                    func_02005d8c();
                    wk->musicDone = TRUE;
                }
            } else {
                wk->musicDone = TRUE;
            }
        } else if (wk->evolveMask != 0) {
            species = 0;
            party = GameData_GetParty(prm->gameData);
            *state = 6;
            while (wk->evolveMask != 0) {
                if (wk->evolveMask & 1) {
                    pkm = PokeParty_GetPkm(party, wk->evolveSlot);
                    species = CheckEvolveSpecies(party, pkm, 0, prm->setup->fieldSituation.env.zoneId,
                                                 GameData_GetSeason(prm->gameData), &method, wk->heapId);
                    evolveSlot = wk->evolveSlot;
                }
                wk->evolveMask = wk->evolveMask >> 1;
                wk->evolveSlot++;
                if (species != 0) {
                    demo = GFL_HeapAllocate(wk->heapId, sizeof(ShinkaDemoParam), FALSE, "event_battle_return.c", 465);
                    demo->gameData = prm->gameData;
                    demo->party = party;
                    demo->species = species;
                    demo->partyIndex = evolveSlot;
                    demo->method = method;
                    demo->unkC = 0;
                    demo->canCancel = TRUE;
                    wk->shinkaDemo = demo;
                    QueueGameProc(wk->procManager, OVERLAY_SHINKA_DEMO, &SHINKA_DEMO_PROC_FUNCTIONS, demo);
                    *state = 5;
                    break;
                }
            }
        } else {
            *state = 6;
        }
        break;
    case 5:
        if (procRunning != TRUE) {
            GFL_HeapFree(wk->shinkaDemo);
            wk->shinkaDemo = NULL;
            if (wk->evolveMask != 0) {
                *state = 4;
            } else {
                (*state)++;
            }
        } else {
            return FALSE;
        }
        break;
    case 6:
    default:
        return TRUE;
    }
    return FALSE;
}

// Marks the party slots whose Pokémon gained a level in the battle, which may evolve
static void EventBattleReturn_CheckLevelUps(EventBattleReturnWork *wk, EventBattleReturnParam *prm) {
    BtlSetup *setup = prm->setup;
    PokeParty *party;
    int count;
    int i;
    u32 levelBefore;

    switch (setup->unkA8) {
    case BATTLE_RESULT_WIN:
    case 3:
    case BATTLE_RESULT_CAPTURE:
        switch (setup->battleType) {
        case BATTLE_TYPE_WILD:
        case BATTLE_TYPE_TRAINER:
            party = GameData_GetParty(prm->gameData);
            count = PokeParty_GetPkmCount(party);
            for (i = 0; i < count; i++) {
                PartyPkm *before = PokeParty_GetPkm(party, i);
                PartyPkm *after = PokeParty_GetPkm(prm->setup->party[0], i);

                levelBefore = PokeParty_GetParam(before, PKM_PARAM_LEVEL, NULL);
                if (levelBefore < PokeParty_GetParam(after, PKM_PARAM_LEVEL, NULL)) {
                    wk->evolveMask |= 1 << i;
                }
            }
            break;
        }
        break;
    }
}

// Pickup and Honey Gather
static void EventBattleReturn_PickupItems(PokeParty *party) {
    int i;
    int j;
    PartyPkm *pkm;
    u16 species;
    u16 item;
    u8 ability;
    int rand;
    u8 levelSlot;
    u8 level;
    int level10;

    for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
        pkm = PokeParty_GetPkm(party, i);
        species = PokeParty_GetParam(pkm, PKM_PARAM_LEGAL_SPECIES, NULL);
        item = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);
        ability = PokeParty_GetParam(pkm, PKM_PARAM_ABILITY, NULL);
        if (ability == ABILITY_PICKUP && species != 0 && species != SPECIES_EGG && item == 0 && GFL_RandomLC(10) == 0) {
            rand = GFL_RandomLC(100);
            levelSlot = (PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL) - 1) / 10;
            if (levelSlot >= 10) {
                levelSlot = 9;
            }
            for (j = 0; j < 9; j++) {
                if (sPickupChances[j] > rand) {
                    PokeParty_SetParam(pkm, PKM_PARAM_ITEM, sPickupItems[levelSlot + j]);
                    break;
                } else if (rand >= 98 && rand <= 99) {
                    PokeParty_SetParam(pkm, PKM_PARAM_ITEM, sPickupRareItems[levelSlot + (99 - rand)]);
                    break;
                }
            }
        }
        if (ability == ABILITY_HONEY_GATHER && species != 0 && species != SPECIES_EGG && item == 0) {
            j = 0;
            level10 = 10;
            level = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);
            while (level10 < level) {
                level10 += 10;
                j++;
            }
            if (GFL_RandomLC(100) < sHoneyGatherChances[j]) {
                PokeParty_SetParam(pkm, PKM_PARAM_ITEM, ITEM_HONEY);
            }
        }
    }
}

// Natural Cure cures the party's status after the battle
static void EventBattleReturn_NaturalCure(PokeParty *party) {
    int i;
    PartyPkm *pkm;
    u16 species;
    u8 ability;

    for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
        pkm = PokeParty_GetPkm(party, i);
        species = PokeParty_GetParam(pkm, PKM_PARAM_LEGAL_SPECIES, NULL);
        ability = PokeParty_GetParam(pkm, PKM_PARAM_ABILITY, NULL);
        if (ability == ABILITY_NATURAL_CURE && species != 0 && species != SPECIES_EGG) {
            PokeParty_SetStatusCond(pkm, 0);
        }
    }
}

// Changes the form of each Burmy that unkE1 marks for the battle's terrain
static void EventBattleReturn_BurmyForms(EventBattleReturnParam *prm, PokeParty *party, u32 terrain) {
    int i;

    if (prm->setup->battleType == 2 || prm->setup->battleType == 3) {
        return;
    }
    for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
        if (prm->setup->unkE1[i]) {
            burmyTransform(prm->gameData, PokeParty_GetPkm(party, i), terrain);
        }
    }
}

// The terrain Burmy changes its form for, from the battle's background
static u8 EventBattleReturn_GetBurmyTerrain(BtlFieldSituation *situation, HeapID heapId) {
    BattleBgData *data = GFL_ArcSysReadHeapNew(ARCID_BATTLE_BG_DATA, 0, heapId);
    u8 terrain = data[situation->env.bgType].burmyTerrain[situation->env.terrain];

    GFL_HeapFree(data);
    return terrain;
}

// Registers each Shaymin that unkE1 marks in the Pokédex again, for its form
static void EventBattleReturn_ShayminForms(EventBattleReturnParam *prm, PokeParty *party) {
    int i;
    PartyPkm *pkm;

    switch (prm->setup->battleType) {
    case BATTLE_TYPE_WILD:
    case BATTLE_TYPE_TRAINER:
        for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
            if (prm->setup->unkE1[i]) {
                pkm = PokeParty_GetPkm(party, i);
                if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL) == SPECIES_SHAYMIN) {
                    PokeDex_RegistPkm(GameData_GetPokedex(prm->gameData), pkm);
                }
            }
        }
        break;
    }
}

// A Pokémon caught in a Heal Ball comes out healed
static void EventBattleReturn_HealBall(PartyPkm *pkm) {
    BOOL wasEncrypted = PokeParty_DecryptPkm(pkm);

    if (PokeParty_GetParam(pkm, PKM_PARAM_POKEBALL, NULL) == ITEM_HEAL_BALL) {
        PokeParty_SetParam(pkm, PKM_PARAM_HP, PokeParty_GetParam(pkm, PKM_PARAM_MAX_HP, NULL));
        PokeParty_SetStatusCond(pkm, 0);
    }
    PokeParty_EncryptPkm(pkm, wasEncrypted);
}
