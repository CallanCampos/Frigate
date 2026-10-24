#include "types.h"
#include "app/pokemon_trade/pokemontrade_2d.h"
#include "app/pokemon_trade/pokemontrade_3d.h"
#include "app/pokemon_trade/pokemontrade_message.h"
#include "app/pokemon_trade/pokemontrade_proc.h"
#include "app/pokemon_trade/pokemontrade_save.h"
#include "app/pokemon_trade/pokemontrade_touchbar.h"
#include "app/pokemon_trade_local.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "constants/version.h"
#include "field/unity_tower.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "pml/evolution.h"
#include "pml/item.h"
#include "pml/mail.h"
#include "pml/met_data.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/chatter.h"
#include "save/medal_box.h"
#include "save/player_info.h"
#include "save/pokedex.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/wifi_list.h"
#include "system/bmp_winframe.h"
#include "system/country_region.h"
#include "system/game_data.h"
#include "system/net_save.h"
#include "system/wipe.h"
#include "system/wordset.h"

// Writing the trade to the save: the copies of what it changes, kept to restore if the save fails, the trade itself,
// the evolution it may start and the saving between the two machines

static void PoketradeSave_SetWaitCounter(PokemonTradeWork *wk, int timer);
static void PoketradeSave_PopBgmFadeIn60(PokemonTradeWork *wk);
static void PoketradeSave_PopBgm(PokemonTradeWork *wk);
static void PoketradeSave_PopBgmFadeIn24(PokemonTradeWork *wk);
static void PoketradeSave_StepEventMessage1(PokemonTradeWork *wk);
static void PoketradeSave_StepEventMessage2(PokemonTradeWork *wk);
static void PoketradeSave_StepWaitFade1(PokemonTradeWork *wk);
static void PoketradeSave_StepWaitFade2(PokemonTradeWork *wk);
static void PoketradeSave_StepWaitSyncRange(PokemonTradeWork *wk);
static void PoketradeSave_SendTradeSummary(PokemonTradeWork *wk);
static void *func_ov194_021bef14(GameData *gameData, HeapID heapId);
static void PoketradeSave_RestoreSaveControl(GameData *gameData, void *copy);
static void *func_ov194_021bef7c(GameData *gameData, HeapID heapId);
static void PoketradeSave_RestoreZukan(GameData *gameData, void *copy);
static void *func_ov194_021befdc(GameData *gameData, HeapID heapId);
static void PoketradeSave_RestoreBlock1F(GameData *gameData, void *copy);
static void PoketradeSave_FreeBackups(void *work);
static void PoketradeSave_RestoreBackups(PokemonTradeWork *wk);
static BOOL PoketradeSave_ConsumePerapVoice(PokemonTradeWork *wk, PokeParty *party, int slot);
static void PoketradeSave_ApplyTradeResult(PokemonTradeWork *wk);
static void PoketradeSave_StepAfterTrade(PokemonTradeWork *wk);
static void PoketradeSave_StepWaitEvolutionMessage(PokemonTradeWork *wk);
static void PoketradeSave_StepRegisterReceived(PokemonTradeWork *wk);
static void PoketradeSave_StepCheckEvolution(PokemonTradeWork *wk);
static void PoketradeSave_StepBeginSyncRange24(PokemonTradeWork *wk);
static void PoketradeSave_StepWaitSyncRange24(PokemonTradeWork *wk);
static void PoketradeSave_StepStartSave(PokemonTradeWork *wk);
static void PoketradeSave_StepWaitSaveDone(PokemonTradeWork *wk);
static void PoketradeSave_StepBeginWipeOut(PokemonTradeWork *wk);
static void PoketradeSave_StepRebuildSceneAfterWipe(PokemonTradeWork *wk);

static void PoketradeSave_SetWaitCounter(PokemonTradeWork *wk, int timer) {
    wk->timer = timer;
}

static void PoketradeSave_PopBgmFadeIn60(PokemonTradeWork *wk) {
    if (wk->bgmCount != 0) {
        GFL_SndBGMPop();
        wk->bgmCount--;
        GFL_SndBGMSetPaused(FALSE);
        GFL_SndBGMFadeIn(60);
    }
}

static void PoketradeSave_PopBgm(PokemonTradeWork *wk) {
    GFL_SndBGMPop();
    wk->bgmCount--;
}

static void PoketradeSave_PopBgmFadeIn24(PokemonTradeWork *wk) {
    GFL_SndBGMPop();
    wk->bgmCount--;
    GFL_SndBGMSetPaused(FALSE);
    GFL_SndBGMFadeIn(24);
}

// After the trade's animation
void PoketradeSave_ResetClactAndBeginEvent(PokemonTradeWork *wk) {
    ClActUnit_Delete(wk->clactUnit);
    ClActSys_Delete();
    Poketrade2D_InitClactSystemSub(wk);
    wk->clactUnit = ClActUnit_Create(340, 0, wk->heapId);
    if (wk->type == 5) {
        PoketradeSave_PopBgmFadeIn60(wk);
        wk->param->next = 3;
        PokemonTrade_SetState(wk, PokemonTrade_FadeOutToEnd);
        return;
    }
    if (wk->type == 6) {
        PoketradeSave_PopBgmFadeIn60(wk);
        wk->param->next = 3;
        PokemonTrade_SetState(wk, NULL);
        return;
    }
    PoketradeSave_CreateMessageResources(wk);
    GFL_SndBGMSetPaused(TRUE);
    GFL_SndBGMPush();
    wk->bgmCount++;
    GFL_SndBGMFadeOut(6);
    wk->timer = 0;
    PokemonTrade_SetState(wk, PoketradeSave_StepEventMessage1);
}

static void PoketradeSave_StepEventMessage1(PokemonTradeWork *wk) {
    PartyPkm *pkm;
    if (wk->timer >= 6) {
        GFL_SndBGMPlay(SEQ_ME_POKEGET, 0xffff);
        GFL_SndBGMFadeIn(6);
        if (wk->type == 7) {
            GFL_MsgDataLoadStrbuf(wk->msgData, 154, wk->strbufTemplate);
        } else if (wk->type == 8) {
            GFL_MsgDataLoadStrbuf(wk->msgData, 157, wk->strbufTemplate);
        } else if (wk->unk11EE != 0) {
            GFL_MsgDataLoadStrbuf(wk->msgData, 132, wk->strbufTemplate);
        } else {
            GFL_MsgDataLoadStrbuf(wk->msgData, 48, wk->strbufTemplate);
        }
        if (PokemonTrade_IsNegoType(wk)) {
            pkm = PokemonTrade_GetPkm(wk, 0);
        } else {
            pkm = PokemonTrade_GetPkm(wk, 1);
        }
        loadPokemonNicknameToStrbuf(wk->wordSet, 1, pkm);
        copyVarForText(wk->wordSet, 0, wk->partnerInfo);
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, wk->strbufTemplate);
        wk->cursorImage = LoadCursorImageEndOfHeap(6, 15, 0, wk->heapId);
        PoketradeSave_OpenMessageWindowAtPos(wk, TRUE, 0);
        PoketradeSave_SetWaitCounter(wk, 0);
        GFL_BGSysSetBGEnabled(6, TRUE);
        wk->timer = 0;
        PokemonTrade_SetState(wk, PoketradeSave_StepEventMessage2);
    }
}

static void PoketradeSave_StepEventMessage2(PokemonTradeWork *wk) {
    PartyPkm *pkm;
    if (wk->timer >= 200) {
        if (PokemonTrade_IsNegoType(wk)) {
            pkm = PokemonTrade_GetPkm(wk, 0);
        } else {
            pkm = PokemonTrade_GetPkm(wk, 1);
        }
        loadPokemonNicknameToStrbuf(wk->wordSet, 1, pkm);
        copyVarForText(wk->wordSet, 0, wk->partnerInfo);
        if (wk->type != 7 && wk->type != 8) {
            if (wk->unk11EE != 0) {
                GFL_MsgDataLoadStrbuf(wk->msgData, 133, wk->strbufTemplate);
                GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, wk->strbufTemplate);
                PoketradeSave_OpenMessageWindowAtPos(wk, TRUE, 0);
            } else {
                GFL_MsgDataLoadStrbuf(wk->msgData, 49, wk->strbufTemplate);
                GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, wk->strbufTemplate);
                PoketradeSave_OpenMessageWindowAtPos(wk, TRUE, 0);
            }
        }
        PokemonTrade_SetState(wk, PoketradeSave_StepWaitFade1);
    }
}

static void PoketradeSave_StepWaitFade1(PokemonTradeWork *wk) {
    if (wk->timer >= 270) {
        PoketradeSave_PopBgmFadeIn24(wk);
        PokemonTrade_SetState(wk, PoketradeSave_StepWaitFade2);
    }
}

static void PoketradeSave_StepWaitFade2(PokemonTradeWork *wk) {
    if (wk->timer >= 420) {
        PoketradeSave_SetWaitCounter(wk, 0);
        PokemonTrade_SetState(wk, PoketradeSave_StepAfterTrade);
    }
}

static void PoketradeSave_StepWaitSyncRange(PokemonTradeWork *wk) {
    if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x17, 8)) {
        PokemonTrade_SetState(wk, PoketradeSave_StepRegisterReceived);
    }
}

// Sends this player's profile, with the Pokémon traded, for the other machine's records
static void PoketradeSave_SendTradeSummary(PokemonTradeWork *wk) {
    UnityTowerVisitor profile;
    UnityTowerSurveySave *survey = getUnityTower_SurveySaveBlkAddrress(GameData_GetSaveControl(wk->gameData));
    PartyPkm *sent;
    PartyPkm *received;
    if (PokemonTrade_IsNegoType(wk)) {
        sent = PokemonTrade_GetPkm(wk, 0);
        received = PokemonTrade_GetPkm(wk, 1);
    } else {
        sent = PokemonTrade_GetPkm(wk, 1);
        received = PokemonTrade_GetPkm(wk, 0);
    }
    PlayerInfo_Copy(GetGameDataPlayerInfo(wk->gameData), &profile.info);
    if (PokeParty_GetParam(sent, PKM_PARAM_IS_EGG, NULL)) {
        profile.sentSpecies = SPECIES_EGG;
    } else {
        profile.sentSpecies = PokeParty_GetParam(sent, PKM_PARAM_SPECIES, NULL);
    }
    if (PokeParty_GetParam(received, PKM_PARAM_IS_EGG, NULL)) {
        profile.receivedSpecies = SPECIES_EGG;
    } else {
        profile.receivedSpecies = PokeParty_GetParam(received, PKM_PARAM_SPECIES, NULL);
    }
    profile.hobby = getPlayerSurveys(survey);
    profile.countryCount = UnityTower_GetCountryCount(survey);
    profile.unk26_0 = UnityTower_GetRatingLevel(survey);
    if (PokemonTrade_IsNetwork(wk)) {
        if (GFL_NetSendCommand(NetHandle_GetOwn(), TRADE_NET_CMD_UNK12, sizeof(profile), &profile)) {
            NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x17, 8);
            PokemonTrade_SetState(wk, PoketradeSave_StepWaitSyncRange);
        }
    } else {
        PokemonTrade_SetState(wk, PoketradeSave_StepBeginWipeOut);
    }
}

// GFLib's GFL_STD_MemCopy, an inline over the copy: the box's copy reads its destination before the call that gives
// the source
static inline void GFL_STD_MemCopy(const void *src, void *dest, u32 size) {
    MI_CpuCopy8(src, dest, size);
}

// The copies of the mail and of the records are inlined, and not `static`: each has a file name string of its own
// in `.data`, ahead of the one the file's other allocations share, which MWCC gives only to an external inline
inline void *BackupMail(GameData *gameData, HeapID heapId) {
    void *copy = GFL_HeapAllocate(heapId, MailBox_GetSize(), FALSE, "pokemontrade_save.c", 370);
    u32 size = MailBox_GetSize();
    GFL_STD_MemCopy(MailBox_GetFromGameData(gameData), copy, size);
    return copy;
}

inline void *BackupRecords(GameData *gameData, HeapID heapId) {
    void *copy = GFL_HeapAllocate(heapId, GameRecords_GetSize(), FALSE, "pokemontrade_save.c", 401);
    u32 size = GameRecords_GetSize();
    GFL_STD_MemCopy(GameData_GetRecords(gameData), copy, size);
    return copy;
}

static void *func_ov194_021bef14(GameData *gameData, HeapID heapId) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    void *copy = GFL_HeapAllocate(heapId, UnityTower_GetSurveySaveSize(), FALSE, "pokemontrade_save.c", 431);
    u32 size = UnityTower_GetSurveySaveSize();
    MI_CpuCopy8(getUnityTower_SurveySaveBlkAddrress(save), copy, size);
    return copy;
}

static void PoketradeSave_RestoreSaveControl(GameData *gameData, void *copy) {
    MI_CpuCopy8(copy, getUnityTower_SurveySaveBlkAddrress(GameData_GetSaveControl(gameData)),
                UnityTower_GetSurveySaveSize());
    GFL_HeapFree(copy);
}

static void *func_ov194_021bef7c(GameData *gameData, HeapID heapId) {
    void *copy = GFL_HeapAllocate(heapId, getSizeofPokedexData(), FALSE, "pokemontrade_save.c", 460);
    u32 size = getSizeofPokedexData();
    MI_CpuCopy8(GameData_GetPokedex(gameData), copy, size);
    return copy;
}

static void PoketradeSave_RestoreZukan(GameData *gameData, void *copy) {
    MI_CpuCopy8(copy, GameData_GetPokedex(gameData), getSizeofPokedexData());
    GFL_HeapFree(copy);
}

static void *func_ov194_021befdc(GameData *gameData, HeapID heapId) {
    void *copy = GFL_HeapAllocate(heapId, PalPadFriendList_GetSize(), FALSE, "pokemontrade_save.c", 489);
    u32 size = PalPadFriendList_GetSize();
    MI_CpuCopy8(GameData_GetPalPadFriendList(gameData), copy, size);
    return copy;
}

static void PoketradeSave_RestoreBlock1F(GameData *gameData, void *copy) {
    MI_CpuCopy8(copy, GameData_GetPalPadFriendList(gameData), PalPadFriendList_GetSize());
    GFL_HeapFree(copy);
}

// Frees the copies once the save has gone through
static void PoketradeSave_FreeBackups(void *work) {
    PokemonTradeWork *wk = work;
    if (wk->backup.mail != NULL) {
        GFL_HeapFree(wk->backup.mail);
    }
    if (wk->backup.records != NULL) {
        GFL_HeapFree(wk->backup.records);
    }
    if (wk->backup.survey != NULL) {
        GFL_HeapFree(wk->backup.survey);
    }
    if (wk->backup.pokedex != NULL) {
        GFL_HeapFree(wk->backup.pokedex);
    }
    if (wk->backup.playersMet != NULL) {
        GFL_HeapFree(wk->backup.playersMet);
    }
    if (wk->backup.party != NULL) {
        GFL_HeapFree(wk->backup.party);
    }
    if (wk->backup.box != NULL) {
        GFL_HeapFree(wk->backup.box);
    }
    MI_CpuFill8(&wk->backup, 0, sizeof(wk->backup));
    wk->unk11F7 = 0;
}

// Restores what the trade changed from the copies
static void PoketradeSave_RestoreBackups(PokemonTradeWork *wk) {
    void *copy;
    if ((copy = wk->backup.mail) != NULL) {
        GameData *gameData = wk->gameData;
        MailData *block;
        // The save is fetched and left unused, as if the block once came from it
        GameData_GetSaveControl(gameData);
        block = MailBox_GetFromGameData(gameData);
        MI_CpuCopy8(copy, block, MailBox_GetSize());
        GFL_HeapFree(copy);
    }
    if ((copy = wk->backup.records) != NULL) {
        GameRecords *records = GameData_GetRecords(wk->gameData);
        MI_CpuCopy8(copy, records, GameRecords_GetSize());
        GFL_HeapFree(copy);
    }
    if (wk->backup.survey != NULL) {
        PoketradeSave_RestoreSaveControl(wk->gameData, wk->backup.survey);
    }
    if (wk->backup.pokedex != NULL) {
        PoketradeSave_RestoreZukan(wk->gameData, wk->backup.pokedex);
    }
    if (wk->backup.playersMet != NULL) {
        PoketradeSave_RestoreBlock1F(wk->gameData, wk->backup.playersMet);
    }
    if (wk->backup.party != NULL) {
        PokeParty_Copy(wk->backup.party, wk->party);
        PlayTime_SetLastSaved(GameData_GetPlayTime(wk->gameData), &wk->backup.unk1C);
        GFL_HeapFree(wk->backup.party);
        wk->backup.party = NULL;
    }
    if (wk->backup.box != NULL) {
        u32 size = getSizeofBox();
        MI_CpuCopy8(wk->backup.box, BoxSaveAccessor_GetBox(wk->boxes, wk->param->box), size);
        GFL_HeapFree(wk->backup.box);
    }
    if (wk->backup.chatot) {
        setChatotPresent(getChatterDataAddress(wk->gameData));
    }
    MI_CpuFill8(&wk->backup, 0, sizeof(wk->backup));
}

// Forgets the Chatot's cry if the only Chatot of the party is traded away
static BOOL PoketradeSave_ConsumePerapVoice(PokemonTradeWork *wk, PokeParty *party, int slot) {
    int chatots = 0;
    int count, i;
    PerapVoice *chatter = getChatterDataAddress(wk->gameData);
    if (!doesChatotExist(chatter)) {
        return FALSE;
    }
    count = PokeParty_GetPkmCount(party);
    for (i = 0; i < count; i++) {
        if (PokeParty_GetParam(PokeParty_GetPkm(party, i), PKM_PARAM_SPECIES, NULL) == SPECIES_CHATOT) {
            chatots++;
        }
    }
    if (chatots != 1) {
        return FALSE;
    }
    if (PokeParty_GetParam(PokeParty_GetPkm(party, slot), PKM_PARAM_SPECIES, NULL) == SPECIES_CHATOT) {
        setChatotNotPresent(chatter);
        return TRUE;
    }
    return FALSE;
}

// Trades the Pokémon in the save, keeping copies of what changes
static void PoketradeSave_ApplyTradeResult(PokemonTradeWork *wk) {
    PartyPkm *pkm;
    GameRecords *records;
    if (!PokemonTrade_IsNegoType(wk)) {
        wk->backup.mail = BackupMail(wk->gameData, wk->heapId);
        wk->backup.records = BackupRecords(wk->gameData, wk->heapId);
        wk->backup.survey = func_ov194_021bef14(wk->gameData, wk->heapId);
        wk->backup.pokedex = func_ov194_021bef7c(wk->gameData, wk->heapId);
        wk->backup.playersMet = func_ov194_021befdc(wk->gameData, wk->heapId);
        PlayTime_GetLastSaved(GameData_GetPlayTime(wk->gameData), &wk->backup.unk1C);
        if (wk->param->box == BoxSaveAccessor_GetAvailableBoxCount(wk->boxes)) {
            wk->backup.party = PokeParty_Create(wk->heapId);
            PokeParty_Copy(wk->party, wk->backup.party);
            wk->backup.chatot = PoketradeSave_ConsumePerapVoice(wk, wk->party, wk->param->slot);
        } else {
            u32 size;
            wk->backup.box = GFL_HeapAllocate(wk->heapId, getSizeofPokeBox(), FALSE, "pokemontrade_save.c", 640);
            size = getSizeofPokeBox();
            GFL_STD_MemCopy(BoxSaveAccessor_GetBox(wk->boxes, wk->param->box), wk->backup.box, size);
        }
    } else {
        PokemonTradeParam *param = wk->param;
        if (param->box == BoxSaveAccessor_GetAvailableBoxCount(wk->boxes)) {
            PoketradeSave_ConsumePerapVoice(wk, wk->party, param->slot);
        }
        wk->unk11F7 = 1;
    }
    {
        PokemonTradeParam *param = wk->param;
        if (param->box != BoxSaveAccessor_GetAvailableBoxCount(wk->boxes)) {
            pkm = PokeParty_GetPkm(param->party, 0);
            if (PML_ItemIsMail(PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL))) {
                MailData *block = MailBox_GetFromGameData(wk->gameData);
                int mailSlot = MailBox_GetFreeSlot(block, 0);
                if (mailSlot != -1) {
                    MailData *mail = CreateMailData(wk->heapId);
                    PokeParty_GetParam(pkm, PKM_PARAM_MAIL, mail);
                    MailBox_SetMail(block, 0, mailSlot, mail);
                    ResetMailData(mail);
                    PokeParty_SetParam(pkm, PKM_PARAM_MAIL, (uptr)mail);
                    PokeParty_SetParam(pkm, PKM_PARAM_ITEM, 0);
                    GFL_HeapFree(mail);
                }
            }
        }
    }
    {
        u8 country = UnityTowerVisitor_GetCountry(wk->partnerInfo);
        u8 province = UnityTowerVisitor_GetProvince(wk->partnerInfo);
        u32 validCountry = Country_GetValidCountry(country, province, TrainerInfo_GetRegion(wk->partnerInfo));
        u32 validProvince = Country_GetValidRegion(country, province, TrainerInfo_GetRegion(wk->partnerInfo));
        if (country != validCountry || province != validProvince) {
            PlayerInfo_SetCountryAndProvince(wk->partnerInfo, 0, 0);
        }
    }
    PalPadFriendList_AddPlayer(GameData_GetPalPadFriendList(wk->gameData), wk->partnerInfo);
    {
        // A trade with a game of the other colour
        BOOL medal = FALSE;
        u8 version = PlayerInfo_GetVersion(wk->partnerInfo);
#ifdef BLACK2
        if (version == VERSION_WHITE || version == VERSION_WHITE2) {
            medal = TRUE;
        }
#else
        if (version == VERSION_BLACK || version == VERSION_BLACK2) {
            medal = TRUE;
        }
#endif
        if (medal) {
            MedalBox_GiveMedal(SaveControl_GetMedalBox(GameData_GetSaveControl(wk->gameData)), 165);
        }
    }
    {
        int i;
        UnityTowerSurveySave *survey = getUnityTower_SurveySaveBlkAddrress(GameData_GetSaveControl(wk->gameData));
        PlayerInfo *infos[2];
        infos[0] = wk->myInfo;
        infos[1] = wk->partnerInfo;
        for (i = 0; i < 2; i++) {
            PlayerInfo *info = infos[i];
            if (info != NULL) {
                u8 country = UnityTowerVisitor_GetCountry(info);
                u8 province = UnityTowerVisitor_GetProvince(info);
                // The region is read and left unused
                TrainerInfo_GetRegion(info);
                if (!UnityTower_GetRegionState(survey, country, province)) {
                    UnityTower_SetRegionState(survey, country, province, TRUE);
                }
            }
        }
    }
    pkm = PokeParty_GetPkm(wk->param->party, 0);
    records = GameData_GetRecords(wk->gameData);
    PokeParty_Recover(pkm);
    if (!PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
        PokeParty_SetParam(pkm, PKM_PARAM_HAPPINESS, 70);
    }
    if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL)) {
        PokeParty_SetupMetData(pkm, 6, wk->myInfo, 30003, wk->heapId);
    }
    switch (wk->type) {
    case 2:
    case 3:
        RecordAddOne(records, 16);
        break;
    case 1:
        RecordAddOne(records, 12);
        break;
    case 0:
        RecordAddOne(records, 36);
        break;
    }
    addPkmToDex(GameData_GetPokedex(wk->gameData), pkm);
    if (wk->type == 3) {
        PalPadFriendList_IncrementTradeCount(GameData_GetPalPadFriendList(wk->gameData));
    }
    if (PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL) == SPECIES_SHAYMIN &&
        PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL) == 1) {
        PokeParty_ChangeForme(pkm, 0);
        addPkmToDex(GameData_GetPokedex(wk->gameData), pkm);
    }
    if (wk->param->box == BoxSaveAccessor_GetAvailableBoxCount(wk->boxes)) {
        copyPkmIntoPartyBlk(wk->party, wk->param->slot, pkm);
    } else {
        BoxSaveAccessor_SetPkm(wk->boxes, wk->param->box, wk->param->slot, PokeParty_ToBoxPkm(pkm));
    }
    switch (wk->type) {
    case 2:
        WifiList_AddFriendRecord(GameData_GetWifiList(wk->gameData), wk->param->friendIndex - 1, 0, 0, 1);
        break;
    case 0:
    case 1: {
        u32 index;
        if (WifiList_FindFriendByPlayerInfo(GameData_GetWifiList(wk->gameData), wk->partnerInfo, &index)) {
            WifiList_AddFriendRecord(GameData_GetWifiList(wk->gameData), index, 0, 0, 1);
        }
        break;
    }
    }
}

static void PoketradeSave_StepAfterTrade(PokemonTradeWork *wk) {
    Poketrade3D_DeleteMcssSlot(wk, 0);
    Poketrade3D_DeleteMcssSlot(wk, 1);
    Poketrade3D_DeleteMcssSlot(wk, 2);
    Poketrade3D_DeleteMcssSlot(wk, 3);
    if (wk->type == 4 || wk->type == 7 || wk->type == 8) {
        wk->param->next = 3;
        PoketradeSave_PopBgmFadeIn60(wk);
        PokemonTrade_SetState(wk, PokemonTrade_FadeOutToEnd);
    } else if (wk->type == 1 || wk->type == 2 || wk->type == 3) {
        PokemonTrade_SetState(wk, PoketradeSave_SendTradeSummary);
    } else {
        PokemonTrade_SetState(wk, PoketradeSave_StepRegisterReceived);
    }
}

static void PoketradeSave_StepWaitEvolutionMessage(PokemonTradeWork *wk) {
    if (PoketradeSave_IsMessagePrintDone(wk) && wk->timer > 180) {
        PokemonTrade_SetState(wk, PoketradeSave_StepCheckEvolution);
    }
}

// Puts the received Pokémon in the parameter's party, for the event
static void PoketradeSave_StepRegisterReceived(PokemonTradeWork *wk) {
    PartyPkm *pkm;
    int slot;
    if (PokemonTrade_IsNegoType(wk)) {
        int index = wk->unkFA0 % 3;
        pkm = PokemonTrade_GetPkm(wk, 0);
        wk->param->box = wk->negoBox[0][index];
        slot = wk->negoSlot[0][index];
    } else {
        pkm = PokemonTrade_GetPkm(wk, 1);
        wk->param->box = wk->selectBox;
        slot = wk->selectSlot;
    }
    wk->param->slot = slot;
    PokeParty_InitCore(wk->param->party, 6);
    PokeParty_AddPkm(wk->param->party, pkm);
    addPkmToDex(GameData_GetPokedex(wk->gameData), pkm);
    if (wk->param->box != BoxSaveAccessor_GetAvailableBoxCount(wk->boxes) &&
        PML_ItemIsMail(PokeParty_GetParam(PokeParty_GetPkm(wk->param->party, 0), PKM_PARAM_ITEM, NULL))) {
        GFL_MsgDataLoadStrbuf(wk->msgData, 107, wk->strbuf);
        PoketradeSave_OpenMessageWindowTop(wk);
        PokemonTrade_SetState(wk, PoketradeSave_StepWaitEvolutionMessage);
        wk->timer = 0;
        return;
    }
    PokemonTrade_SetState(wk, PoketradeSave_StepCheckEvolution);
}

// Starts the received Pokémon's evolution if it evolves by trade
static void PoketradeSave_StepCheckEvolution(PokemonTradeWork *wk) {
    if (wk->gameData == NULL) {
        PokemonTrade_SetState(wk, PoketradeSave_StepBeginWipeOut);
    } else {
        if (wk->param != NULL) {
            PartyPkm *pkm;
            PartyPkm *other;
            u32 species;
            u32 method;
            if (PokemonTrade_IsNegoType(wk)) {
                pkm = PokemonTrade_GetPkm(wk, 0);
                other = PokemonTrade_GetPkm(wk, 1);
            } else {
                pkm = PokemonTrade_GetPkm(wk, 1);
                other = PokemonTrade_GetPkm(wk, 0);
            }
            species =
                CheckEvolveSpecies(NULL, pkm, 1, (uptr)other, GameData_GetSeason(wk->gameData), &method, wk->heapId);
            if (species != 0) {
                wk->param->next = TRADE_NEXT_EVOLVE;
                wk->param->evolveSpecies = species;
                wk->param->evolveMethod = method;
                PokemonTrade_SetState(wk, PokemonTrade_FadeOutToEnd);
                PoketradeSave_PopBgm(wk);
                return;
            }
        }
        GFL_MsgDataLoadStrbuf(wk->msgData, 50, wk->strbuf);
        PoketradeSave_OpenMessageWindowTop(wk);
        PoketradeSave_ShowTimeIcon(wk);
        PokemonTrade_SetState(wk, PoketradeSave_StepBeginSyncRange24);
    }
    PoketradeSave_PopBgmFadeIn60(wk);
}

void PoketradeSave_StepSaveCancelNotice(PokemonTradeWork *wk) {
    PoketradeSave_CreateMessageResources(wk);
    GFL_MsgDataLoadStrbuf(wk->msgData, 50, wk->strbuf);
    wk->cursorImage = LoadCursorImageEndOfHeap(6, 15, 0, wk->heapId);
    PoketradeSave_OpenMessageWindowTop(wk);
    PoketradeSave_ShowTimeIcon(wk);
    PoketradeSave_SetWaitCounter(wk, 0);
    GFL_BGSysSetBGEnabled(6, TRUE);
    GFL_NetSetIconScreen(TRUE, wk->heapId);
    GFL_FadeSet(3, 16, 0, 2);
    PokemonTrade_SetState(wk, PoketradeSave_StepBeginSyncRange24);
}

static void PoketradeSave_StepBeginSyncRange24(PokemonTradeWork *wk) {
    NetHandle_SendTimingPair(NetHandle_GetOwn(), 0x18, 8);
    PokemonTrade_SetState(wk, PoketradeSave_StepWaitSyncRange24);
}

static void PoketradeSave_StepWaitSyncRange24(PokemonTradeWork *wk) {
    if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 0x18, 8)) {
        PokemonTrade_SetState(wk, PoketradeSave_StepStartSave);
    }
}

static void PoketradeSave_StepStartSave(PokemonTradeWork *wk) {
    if (PoketradeSave_IsMessagePrintDone(wk)) {
        if (PokemonTrade_IsNetwork(wk)) {
            PoketradeSave_ApplyTradeResult(wk);
            wk->netSave = NetSave_CreateWithCallback(wk->heapId, wk->gameData, PoketradeSave_FreeBackups, wk);
            PokemonTrade_SetState(wk, PoketradeSave_StepWaitSaveDone);
        } else {
            PokemonTrade_SetState(wk, PoketradeSave_StepBeginWipeOut);
        }
    }
}

static void PoketradeSave_StepWaitSaveDone(PokemonTradeWork *wk) {
    if (NetSave_Update(wk->netSave)) {
        PoketradeSave_RestoreBackups(wk);
        NetSave_Free(wk->netSave);
        wk->netSave = NULL;
        PokemonTrade_SetState(wk, PoketradeSave_StepBeginWipeOut);
    }
}

static void PoketradeSave_StepBeginWipeOut(PokemonTradeWork *wk) {
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0, 6, 1, wk->heapId);
    wk->timer = 0;
    wk->unk11F7 = 0;
    PokemonTrade_SetState(wk, PoketradeSave_StepRebuildSceneAfterWipe);
}

static void PoketradeSave_StepRebuildSceneAfterWipe(PokemonTradeWork *wk) {
    if (wk->timer >= 6 && GFL_WipeIsFinished()) {
        if (wk->cursorImage != 0) {
            // The library takes the position and size as u16s, which the prototype doesn't say
            GFL_BGSysFreeCharMemory(6, (u16)wk->cursorImage, (u16)(wk->cursorImage >> 16));
        }
        wk->cursorImage = 0;
        GFL_BGSysSetEnabledBGsA(0);
        GFL_BGSysSetEnabledBGsB(0);
        ClActUnit_Delete(wk->clactUnit);
        wk->clactUnit = ClActUnit_Create(340, 0, wk->heapId);
        PoketradeSave_CloseMessageWindow(wk);
        Poketrade2D_ExitSubBgs(wk);
        Poketrade2D_ExitSummaryBgs(wk);
        Poketrade2D_SetDisplayConfig(0);
        Poketrade2D_SetupSubBgs(wk);
        Poketrade2D_RegisterIconSpriteResources(wk);
        Poketrade2D_CreateBoxIconActors(wk);
        Poketrade2D_CreateBoxWindows(wk);
        Poketrade2D_LoadBackgroundGfx(wk);
        Poketrade2D_CreateFontResource(wk);
        Poketrade2D_LoadMainGfxAndMessageWindow(wk);
        Poketrade3D_AddUnitAndInit(wk, 0);
        GFL_BGSysSetEnabledBGsA(0x1f);
        GFL_BGSysSetEnabledBGsB(0x1e);
        PokemonTrade_CreateTouchBar(wk);
        GFL_NetSetIconScreen(TRUE, wk->heapId);
        wk->checkCount = 0;
        PokemonTrade_SetState(wk, PokemonTrade_StepResetForNewRound);
    }
}
