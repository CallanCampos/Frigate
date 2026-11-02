// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "battle/battle_overlay.h"
#include "battle/btl_adapter.h"
#include "battle/btl_calc.h"
#include "battle/btl_client.h"
#include "battle/btl_field.h"
#include "battle/btl_main.h"
#include "battle/btl_net.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_pospoke.h"
#include "battle/btl_result.h"
#include "battle/btl_server.h"
#include "battle/btl_server_cmd.h"
#include "battle/btl_server_flow.h"
#include "battle/btl_setup.h"
#include "battle/btlv.h"
#include "battle/pokewood_cutin.h"
#include "constants/abilities.h"
#include "constants/arc.h"
#include "constants/battle.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "gfl/arc.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/hw.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/chatter.h"
#include "save/config.h"
#include "save/high_link.h"
#include "save/player_info.h"
#include "save/pokedex_savedata.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/app_keycursor.h"
#include "system/comm_player_support.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/ir_check.h"
#include "system/printsys.h"
#include "system/ringtone_sys.h"
#include "system/text_speed.h"
#include "system/time_icon.h"
#include "text/system/ability_handlers_btl_main.h"

// The name shown for a link opponent whose name is hidden: the other version's
#ifdef BLACK2
#define TEXT_HIDDEN_TRAINER_NAME AbilityHandlersBtlMain_Text_Kuro
#else
#define TEXT_HIDDEN_TRAINER_NAME AbilityHandlersBtlMain_Text_Shiro
#endif

// The first mon ID of each client
static const u8 sClientFirstMonIds[4] = { 0, 12, 6, 18 };

// The battle system's proc
const GameProcFunctions BATTLE_PROC_FUNCTIONS = { BtlMain_ProcInit, BtlMain_ProcMain, BtlMain_ProcExit };

// Starts a step of the module
static inline void BtlMainSeq_Set(BtlMainSeq *seq, BtlMainSeqFunc func, BtlMainModule *mainModule) {
    seq->func = func;
    seq->nextFunc = NULL;
    seq->mainModule = mainModule;
    seq->state = 0;
}

// Runs the module's current step, then its next one, until both are done
static inline BOOL BtlMainSeq_Run(BtlMainSeq *seq) {
    if (seq->func != NULL) {
        if (seq->func(&seq->state, seq->mainModule)) {
            seq->func = NULL;
            seq->state = 0;
        }
        return FALSE;
    }
    if (seq->nextFunc != NULL) {
        if (seq->nextFunc(&seq->state, seq->mainModule)) {
            seq->nextFunc = NULL;
        } else {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL BtlMain_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    BtlMainModule *mainModule;
    BtlSetup *setup = param;
    BOOL notInitialized;
    BOOL isSet;
    BOOL flag;

    switch (*state) {
    case 0:
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_BATTLE, 0x20000);
        GFL_HeapCreateChild(HEAPID_USER, 0x14, 0x8000);
        GFL_HeapCreateChild(HEAPID_USER, 0x13, 0xac500);
        mainModule = GFL_ProcInitSubsystem(proc, sizeof(BtlMainModule), HEAPID_BATTLE);
        MIi_CpuClear32(0, mainModule, sizeof(BtlMainModule));
        BtlvCore_InitSystem(0x13);
        BattleOverlay_LoadDev();
        if (!GFL_SndIsVolumeControlCallbackSet()) {
            GFL_SndSetVolumeControlCallbacks();
            mainModule->volumeCallbacksSet = 1;
        }
        mainModule->heapId = HEAPID_BATTLE;
        mainModule->setup = setup;
        setup->capturedSlot = 6;
        mainModule->playerInfo = mainModule->setup->playerInfo[0];
        mainModule->gameTimeLimit = mainModule->setup->unk8C;
        mainModule->commandTimeLimit = mainModule->setup->unk8E;
        mainModule->prizeMoneyFixed = 0;
        mainModule->lossMoneyCalculated = 0;
        mainModule->commError = 0;
        mainModule->setupFailed = 0;
        mainModule->skipExitBgmFadeOut = 0;
        mainModule->moneyDoubled = 0;
        mainModule->npcCount = 0;
        mainModule->mainFunc = NULL;
        notInitialized = FALSE;
        mainModule->viewCore = NULL;
        mainModule->result = BTL_RESULT_NOT_SET;
        mainModule->textSpeed = Config_GetTextSpeed(mainModule->setup->config);
        if (!Config_GetBattleScene(setup->config)) {
            notInitialized = TRUE;
        }
        mainModule->battleSceneOff = (u8)notInitialized;
        mainModule->field = BtlField_Create(mainModule->heapId);
        PokeCon_Init(&mainModule->pokeCons[0], mainModule, FALSE);
        PokeCon_Init(&mainModule->pokeCons[1], mainModule, TRUE);
        BtlSetup_ClearResults(mainModule->setup);
        mainModule->wildIllusionPkm = NULL;
        mainModule->partyPacket = NULL;
        if (mainModule->setup->fieldSituation.unk1b == 0) {
            buildSeed(&mainModule->rand);
        } else {
            mainModule->rand = setup->rand;
        }
        BtlCalc_Init(&mainModule->rand, mainModule->heapId);
        func_ov167_021b1670();
        mainModule->prizeMoney = BtlCalc_GetPrizeMoney(setup);
        mainModule->money = 0;
        mainModule->lossMoney = 0;
        if (mainModule->setup->fieldSituation.unk1b == 0) {
            u32 i;

            for (i = 0; i < 4; i++) {
                mainModule->setup->unk44[i] = 0;
            }
        }
        BtlClientIDList_Init(&mainModule->clientIdList);
        flag = Config_GetBattleStyle(setup->config);
        isSet = FALSE;
        if (flag) {
            isSet = TRUE;
        }
        mainModule->unk46F = isSet;
        BtlNet_Init(setup->fieldSituation.netHandle, setup->unkA0, 0x14);
        BtlCalc_SetHeapID(HEAPID_BATTLE);
        if (setup->studioMode) {
            BtlMain_LoadStudioData(mainModule, setup->unkDF);
        }
        (*state)++;
        break;
    case 1:
        mainModule = work;
        if (mainModule->setup->fieldSituation.netHandle != NULL && BtlNet_CheckError()) {
            mainModule->commError = 1;
            mainModule->setupFailed = 1;
            return TRUE;
        }
        if (BtlNet_IsInitSynced()) {
            BtlAdapter_InitSystem(mainModule->setup->fieldSituation.unk18);
            BtlMain_SetSetupSeq(&mainModule->seq, mainModule, mainModule->setup);
            mainModule->unk460 = 0;
            BtlMain_InitTrainers(mainModule);
            BtlMain_CreateSrcParties(mainModule);
            (*state)++;
        }
        break;
    case 2:
        mainModule = work;
        if (BtlMainSeq_Run(&mainModule->seq)) {
            if (mainModule->commError) {
                mainModule->setupFailed = 1;
                return TRUE;
            }
            if (mainModule->setup->battleType != BTL_TYPE_CAPTURE_DEMO) {
                IncRecord(mainModule, 4);
            }
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL BtlMain_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    BtlMainModule *mainModule = work;
    u32 result;

    if (mainModule->mainFunc == NULL) {
        return TRUE;
    }
    if (mainModule->mainFunc(mainModule)) {
        if (!IsRecPlayMode(mainModule)) {
            result = BtlMain_DetermineResult(mainModule);
            BtlMain_StoreRecData(mainModule);
            BtlMain_StoreResultStats(mainModule);
            BtlMain_StorePartiesToSetup(mainModule);
            switch (result) {
            case BTL_RESULT_VICTORY:
                mainModule->setup->moneyChange = mainModule->prizeMoney + GetBonusMoney(mainModule);
                break;
            case BTL_RESULT_DEFEAT:
            case BTL_RESULT_DRAW:
                mainModule->setup->moneyChange = -mainModule->lossMoney;
                break;
            }
        } else {
            mainModule->setup->recReadComplete = (u8)BtlMain_IsRecReadComplete(mainModule);
        }
        return TRUE;
    }
    return FALSE;
}

BOOL BtlMain_IsRecReadComplete(BtlMainModule *mainModule) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (DoesClientExist(mainModule, i) && !BtlRecReader_IsReadComplete(&mainModule->recReader, i)) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL BtlMain_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    BtlMainModule *mainModule = work;
    s32 brightness;

    switch (*state) {
    case 0:
        if (!mainModule->skipExitBgmFadeOut) {
            GFL_SndBGMFadeOut(30);
        }
        mainModule->unk460 = 0;
        if (GetStudioMode(mainModule)) {
            BtlMain_FreeStudioData(mainModule);
        }
        if (GX_GetMasterBrightness()) {
            mainModule->unk460 = -16;
            GX_SetMasterBrightness(-16);
            GXS_SetMasterBrightness(-16);
            *state += 2;
            break;
        }
        (*state)++;
        break;
    case 1:
        if (mainModule->unk460 > -16) {
            mainModule->unk460--;
            brightness = GX_GetMasterBrightness();
            if (brightness > mainModule->unk460) {
                GX_SetMasterBrightness(mainModule->unk460);
            }
            brightness = GXS_GetMasterBrightness();
            if (brightness > mainModule->unk460) {
                GXS_SetMasterBrightness(mainModule->unk460);
            }
            break;
        }
        (*state)++;
        break;
    case 2:
        if (!mainModule->skipExitBgmFadeOut) {
            if (GFL_SndBGMIsFading()) {
                break;
            }
            GFL_SndBGMStopImmediate();
            (*state)++;
        } else {
            (*state)++;
        }
        break;
    case 3:
        if (mainModule->field != NULL) {
            BtlField_Delete(mainModule->field);
            mainModule->field = NULL;
        }
        if (mainModule->wildIllusionPkm != NULL) {
            GFL_HeapFree(mainModule->wildIllusionPkm);
            mainModule->wildIllusionPkm = NULL;
        }
        if (mainModule->partyPacket != NULL) {
            BtlNet_FreePartyPacket(mainModule->partyPacket);
            mainModule->partyPacket = NULL;
        }
        BtlCalc_Exit();
        BtlMain_FreeSrcParties(mainModule);
        BtlMain_FreeTrainers(mainModule);
        BtlMain_SetCleanupSeq(&mainModule->seq, mainModule, mainModule->setup);
        (*state)++;
        break;
    case 4:
        if (BtlMainSeq_Run(&mainModule->seq)) {
            BtlAdapter_QuitSystem();
            BtlNet_Exit();
            func_ov167_021bd880();
            (*state)++;
        }
        break;
    case 5:
        BtlvCore_ExitSystem();
        GFL_SndPlayerSetMuteStateEx(1, 0x3e);
        PokeVoice_ResetMasterVolume();
        if (mainModule->volumeCallbacksSet) {
            RingtoneSys_RestoreLidCallbacks();
            mainModule->volumeCallbacksSet = 0;
        }
        GFL_ProcReleaseSubsystem(proc);
        GFL_HeapDelete(0x13);
        GFL_HeapDelete(0x14);
        GFL_HeapDelete(HEAPID_BATTLE);
        BattleOverlay_UnloadDev();
        return TRUE;
    }
    return FALSE;
}

void BtlMain_SetSetupSeq(BtlMainSeq *seq, BtlMainModule *mainModule, BtlSetup *setup) {
    u32 i;

    for (i = 0; i < BTL_POS_MAX; i++) {
        mainModule->posClientIds[i] = 4;
    }
    if (setup->fieldSituation.unk18 == 0) {
        switch (setup->battleStyle) {
        case BTL_STYLE_SINGLE:
            BtlMainSeq_Set(seq, BtlMain_SetupAloneSingle, mainModule);
            return;
        case BTL_STYLE_DOUBLE:
            if (setup->fieldSituation.multiMode == 0) {
                BtlMainSeq_Set(seq, BtlMain_SetupAloneDouble, mainModule);
                return;
            }
            BtlMainSeq_Set(seq, BtlMain_SetupAloneMulti, mainModule);
            return;
        case BTL_STYLE_TRIPLE:
            BtlMainSeq_Set(seq, BtlMain_SetupAloneTriple, mainModule);
            return;
        case BTL_STYLE_ROTATION:
            BtlMainSeq_Set(seq, BtlMain_SetupAloneRotation, mainModule);
            return;
        default:
            BtlMainSeq_Set(seq, BtlMain_SetupAloneSingle, mainModule);
            return;
        }
    }
    switch (setup->battleStyle) {
    case BTL_STYLE_SINGLE:
        BtlMainSeq_Set(seq, BtlMain_SetupCommSingle, mainModule);
        return;
    case BTL_STYLE_DOUBLE:
        BtlMainSeq_Set(seq, BtlMain_SetupCommDouble, mainModule);
        return;
    case BTL_STYLE_TRIPLE:
        BtlMainSeq_Set(seq, BtlMain_SetupCommTriple, mainModule);
        return;
    case BTL_STYLE_ROTATION:
        BtlMainSeq_Set(seq, BtlMain_SetupCommRotation, mainModule);
        return;
    default:
        BtlMainSeq_Set(seq, BtlMain_SetupAloneSingle, mainModule);
        return;
    }
}

void BtlMain_SetCleanupSeq(BtlMainSeq *seq, BtlMainModule *mainModule, BtlSetup *setup) {
    BtlMainSeq_Set(seq, BtlMain_Cleanup, mainModule);
}

u8 BtlMain_IsCommCompetitor(BtlSetup *setup) {
    switch (setup->battleType) {
    case BTL_TYPE_WILD:
        return FALSE;
    case BTL_TYPE_TRAINER:
        return FALSE;
    case BTL_TYPE_FACILITY:
        return FALSE;
    case BTL_TYPE_COMM:
        return TRUE;
    default:
        return FALSE;
    }
}

void BtlMain_SetupPlayerAndParties(BtlMainModule *mainModule, BtlSetup *setup) {
    u32 i;
    PerapVoice *chatter;
    u8 chatterFlags[4];

    mainModule->playerClientId = setup->fieldSituation.unk19;
    i = 0;
    mainModule->playerPos = GetClientBattlePos(mainModule, mainModule->playerClientId, 0);
    BtlMain_LoadSetupParties(mainModule, setup);
    if (!IsRecPlayMode(mainModule)) {
        chatter = getChatterBlockAddress(GameData_GetSaveControl(setup->gameData));
        mainModule->chatters[0] = allocChatotChatterBlk(mainModule->heapId);
        moveChatter(mainModule->chatters[0], chatter);
        mainModule->setup->unk44[0] = GetClientChatterEffect(mainModule, i);
        return;
    }
    for (; i < 4; i++) {
        chatterFlags[ClientIDToSetupIndex(mainModule->playerClientId, i)] = mainModule->setup->unk44[i];
    }
    for (i = 0; i < 4; i++) {
        mainModule->setup->unk44[i] = chatterFlags[i];
    }
}

void BtlMain_LoadSetupParties(BtlMainModule *mainModule, BtlSetup *setup) {
    u8 index0;
    u8 index1;
    u8 index2;
    u8 index3;

    index0 = ClientIDToSetupIndex(mainModule->playerClientId, 0);
    index1 = ClientIDToSetupIndex(mainModule->playerClientId, 1);
    SetClientSrcParty(mainModule, 0, setup->party[index0]);
    SetClientSrcParty(mainModule, 1, setup->party[index1]);
    index2 = ClientIDToSetupIndex(mainModule->playerClientId, 2);
    index3 = ClientIDToSetupIndex(mainModule->playerClientId, 3);
    if (setup->party[index2] != NULL) {
        SetClientSrcParty(mainModule, 2, setup->party[index2]);
    }
    if (setup->party[index3] != NULL) {
        SetClientSrcParty(mainModule, 3, setup->party[index3]);
    }
}

void BtlMain_SetupClientTrainer(BtlMainModule *mainModule, u8 clientId) {
    BtlSetup *setup = mainModule->setup;
    u8 index = ClientIDToSetupIndex(mainModule->playerClientId, clientId);
    BtlTrainerData *trainer = &mainModule->trainers[clientId];

    if (setup->trainers[index]->trainerId != 0) {
        TrainerData_SetFromSetup(trainer, setup->trainers[index]);
    } else {
        TrainerData_SetFromPlayer(trainer, mainModule->heapId, setup->playerInfo[index]);
    }
}

u8 CalcClientBattlerCount(BtlMainModule *mainModule, u8 clientId) {
    BtlSetup *setup = mainModule->setup;

    switch (setup->battleStyle) {
    case BTL_STYLE_SINGLE:
        return 1;
    case BTL_STYLE_ROTATION:
        return 1;
    case BTL_STYLE_TRIPLE:
        return 3;
    case BTL_STYLE_DOUBLE:
        if (setup->fieldSituation.multiMode == 0) {
            return 2;
        }
        if (setup->battleType == BTL_TYPE_COMM) {
            return 1;
        }
        if (setup->party[(u8)((clientId + 2) & 3)] != NULL &&
            PokeParty_GetPkmCount(setup->party[(u8)((clientId + 2) & 3)]) > 0) {
            return 1;
        }
        return 2;
    default:
        return 1;
    }
}

void BtlMain_SetupTrainers(BtlMainModule *mainModule, BtlSetup *setup) {
    if (setup->fieldSituation.unk1b == 0) {
        TrainerData_SetFromPlayer(&mainModule->trainers[0], mainModule->heapId, mainModule->playerInfo);
        TrainerData_SetFromSetup(&mainModule->trainers[1], setup->trainers[1]);
    } else {
        BtlMain_SetupClientTrainer(mainModule, 0);
        BtlMain_SetupClientTrainer(mainModule, 1);
    }
}

void BtlMain_SetupRecReader(BtlMainModule *mainModule, BtlSetup *setup) {
    if (setup->fieldSituation.unk1b != 0) {
        BtlRecReader_Init(&mainModule->recReader, setup->unkB0, setup->unkB4);
        BattleClient_SetRecPlayData(mainModule->clients[0], &mainModule->recReader);
        BattleClient_SetRecPlayData(mainModule->clients[1], &mainModule->recReader);
    }
}

void BtlMain_CreateViewCore(BtlMainModule *mainModule, BtlSetup *setup, u32 commCompetitor) {
    BtlClient *client;

    if (setup->fieldSituation.unk1b != 0) {
        client = mainModule->clients[setup->fieldSituation.unk19];
    } else {
        client = mainModule->clients[0];
    }
    mainModule->viewCore = BtlvCore_Create(mainModule, client, &mainModule->pokeCons[0], commCompetitor, 0x13);
    BattleClient_SetViewCore(client, mainModule->viewCore);
}

BOOL BtlMain_SetupAloneSingle(u32 *state, BtlMainModule *mainModule) {
    BtlSetup *setup = mainModule->setup;
    u8 commCompetitor = BtlMain_IsCommCompetitor(setup);

    mainModule->clientCount = 2;
    mainModule->posClientIds[0] = 0;
    mainModule->posClientIds[1] = 1;
    BtlMain_SetupPlayerAndParties(mainModule, setup);
    PokeCon_Init(&mainModule->pokeCons[0], mainModule, FALSE);
    PokeCon_CreateClientParty(&mainModule->pokeCons[0], mainModule, 0);
    PokeCon_CreateClientParty(&mainModule->pokeCons[0], mainModule, 1);
    PokeCon_Init(&mainModule->pokeCons[1], mainModule, TRUE);
    PokeCon_CreateClientParty(&mainModule->pokeCons[1], mainModule, 0);
    PokeCon_CreateClientParty(&mainModule->pokeCons[1], mainModule, 1);
    mainModule->server = BattleServer_Create(mainModule, &mainModule->rand, &mainModule->pokeCons[1], commCompetitor,
                                             mainModule->heapId);
    mainModule->isServer = 1;
    BtlMain_SetupTrainers(mainModule, setup);
    mainModule->clients[0] =
        BattleClient_Create(mainModule, &mainModule->pokeCons[0], 0, setup->fieldSituation.netHandle, 0, 1, 0,
                            commCompetitor, setup->fieldSituation.unk1b, &mainModule->rand, mainModule->heapId);
    mainModule->clients[1] =
        BattleClient_Create(mainModule, &mainModule->pokeCons[0], 0, setup->fieldSituation.netHandle, 1, 1, 1,
                            commCompetitor, setup->fieldSituation.unk1b, &mainModule->rand, mainModule->heapId);
    BtlMain_SetupRecReader(mainModule, setup);
    BtlMain_CreateViewCore(mainModule, setup, commCompetitor);
    BattleServer_AttachClient(mainModule->server, BattleClient_GetAdapter(mainModule->clients[0]), 0, 1);
    BattleServer_AttachClient(mainModule->server, BattleClient_GetAdapter(mainModule->clients[1]), 1, 1);
    BattleServer_Start(mainModule->server);
    mainModule->mainFunc = BtlMain_MainLoopAlone;
    return TRUE;
}

BOOL BtlMain_Cleanup(u32 *state, BtlMainModule *mainModule) {
    u32 i;

    if (mainModule->viewCore != NULL) {
        BtlvCore_Delete(mainModule->viewCore);
        mainModule->viewCore = NULL;
    }
    for (i = 0; i < 4; i++) {
        if (mainModule->clients[i] != NULL) {
            BattleClient_Delete(mainModule->clients[i]);
        }
    }
    PokeCon_FreeMons(&mainModule->pokeCons[0]);
    PokeCon_FreeMons(&mainModule->pokeCons[1]);
    if (mainModule->server != NULL) {
        BattleServer_Delete(mainModule->server);
    }
    if (mainModule->cmdCheckServer != NULL) {
        BattleServer_Delete(mainModule->cmdCheckServer);
    }
    return TRUE;
}

BOOL BtlMain_SetupAloneDouble(u32 *state, BtlMainModule *mainModule) {
    BtlSetup *setup = mainModule->setup;
    u8 commCompetitor = BtlMain_IsCommCompetitor(setup);

    mainModule->playerClientId = 0;
    mainModule->clientCount = 2;
    mainModule->posClientIds[0] = 0;
    mainModule->posClientIds[1] = 1;
    mainModule->posClientIds[2] = 0;
    mainModule->posClientIds[3] = 1;
    BtlMain_SetupPlayerAndParties(mainModule, setup);
    PokeCon_Init(&mainModule->pokeCons[0], mainModule, FALSE);
    PokeCon_CreateClientParty(&mainModule->pokeCons[0], mainModule, 0);
    PokeCon_CreateClientParty(&mainModule->pokeCons[0], mainModule, 1);
    PokeCon_Init(&mainModule->pokeCons[1], mainModule, TRUE);
    PokeCon_CreateClientParty(&mainModule->pokeCons[1], mainModule, 0);
    PokeCon_CreateClientParty(&mainModule->pokeCons[1], mainModule, 1);
    BtlMain_SetupTrainers(mainModule, setup);
    mainModule->server = BattleServer_Create(mainModule, &mainModule->rand, &mainModule->pokeCons[1], commCompetitor,
                                             mainModule->heapId);
    mainModule->isServer = 1;
    mainModule->clients[0] =
        BattleClient_Create(mainModule, &mainModule->pokeCons[0], 0, setup->fieldSituation.netHandle, 0, 2, 0,
                            commCompetitor, setup->fieldSituation.unk1b, &mainModule->rand, mainModule->heapId);
    mainModule->clients[1] =
        BattleClient_Create(mainModule, &mainModule->pokeCons[0], 0, setup->fieldSituation.netHandle, 1, 2, 1,
                            commCompetitor, setup->fieldSituation.unk1b, &mainModule->rand, mainModule->heapId);
    BtlMain_SetupRecReader(mainModule, setup);
    BtlMain_CreateViewCore(mainModule, setup, commCompetitor);
    BattleServer_AttachClient(mainModule->server, BattleClient_GetAdapter(mainModule->clients[0]), 0, 2);
    BattleServer_AttachClient(mainModule->server, BattleClient_GetAdapter(mainModule->clients[1]), 1, 2);
    BattleServer_Start(mainModule->server);
    mainModule->mainFunc = BtlMain_MainLoopAlone;
    return TRUE;
}

BOOL BtlMain_SetupAloneMulti(u32 *state, BtlMainModule *mainModule) {
    BtlSetup *setup = mainModule->setup;
    u8 commCompetitor = BtlMain_IsCommCompetitor(setup);
    s32 i;

    mainModule->playerClientId = 0;
    mainModule->clientCount = 2;
    mainModule->posClientIds[0] = 0;
    mainModule->posClientIds[2] = 0;
    mainModule->posClientIds[1] = 1;
    mainModule->posClientIds[3] = 1;
    switch (setup->fieldSituation.multiMode) {
    case 1:
    case 2:
    case 3:
        mainModule->posClientIds[2] = 2;
        mainModule->posClientIds[3] = 3;
        mainModule->clientCount = 4;
        break;
    case 4:
        mainModule->posClientIds[3] = 3;
        mainModule->clientCount = 3;
        break;
    case 5:
        mainModule->posClientIds[2] = 2;
        mainModule->clientCount = 3;
        break;
    }
    BtlMain_SetupPlayerAndParties(mainModule, setup);
    PokeCon_Init(&mainModule->pokeCons[0], mainModule, FALSE);
    PokeCon_Init(&mainModule->pokeCons[1], mainModule, TRUE);
    for (i = 0; i < 4; i++) {
        if (DoesClientExist(mainModule, i)) {
            PokeCon_CreateClientParty(&mainModule->pokeCons[0], mainModule, i);
            PokeCon_CreateClientParty(&mainModule->pokeCons[1], mainModule, i);
        }
    }
    if (setup->fieldSituation.unk1b == 0) {
        TrainerData_SetFromPlayer(&mainModule->trainers[0], mainModule->heapId, mainModule->playerInfo);
        TrainerData_SetFromSetup(&mainModule->trainers[1], setup->trainers[1]);
        if (DoesClientExist(mainModule, 2)) {
            TrainerData_SetFromSetup(&mainModule->trainers[2], setup->trainers[2]);
        }
        if (DoesClientExist(mainModule, 3)) {
            TrainerData_SetFromSetup(&mainModule->trainers[3], setup->trainers[3]);
        }
    } else {
        for (i = 0; i < 4; i++) {
            if (DoesClientExist(mainModule, i)) {
                BtlMain_SetupClientTrainer(mainModule, i);
            }
        }
    }
    mainModule->server = BattleServer_Create(mainModule, &mainModule->rand, &mainModule->pokeCons[1], commCompetitor,
                                             mainModule->heapId);
    mainModule->isServer = 1;
    for (i = 0; i < 4; i++) {
        if (DoesClientExist(mainModule, i)) {
            u8 count = GetClientPosCount(mainModule, i);
            BOOL isAI = FALSE;

            if (i != 0) {
                isAI = TRUE;
            }

            mainModule->clients[i] = BattleClient_Create(
                mainModule, &mainModule->pokeCons[0], 0, setup->fieldSituation.netHandle, i, count, isAI,
                commCompetitor, setup->fieldSituation.unk1b, &mainModule->rand, mainModule->heapId);
        }
    }
    if (setup->fieldSituation.unk1b != 0) {
        BtlRecReader_Init(&mainModule->recReader, setup->unkB0, setup->unkB4);
        for (i = 0; i < 4; i++) {
            if (DoesClientExist(mainModule, i)) {
                BattleClient_SetRecPlayData(mainModule->clients[i], &mainModule->recReader);
            }
        }
    }
    BtlMain_CreateViewCore(mainModule, setup, commCompetitor);
    for (i = 0; i < 4; i++) {
        if (DoesClientExist(mainModule, i)) {
            u8 count = GetClientPosCount(mainModule, i);

            BattleServer_AttachClient(mainModule->server, BattleClient_GetAdapter(mainModule->clients[i]), i, count);
        }
    }
    BattleServer_Start(mainModule->server);
    mainModule->mainFunc = BtlMain_MainLoopAlone;
    return TRUE;
}

BOOL BtlMain_SetupAloneTriple(u32 *state, BtlMainModule *mainModule) {
    BtlSetup *setup = mainModule->setup;
    u8 commCompetitor = BtlMain_IsCommCompetitor(setup);

    mainModule->clientCount = 2;
    mainModule->isServer = 1;
    mainModule->posClientIds[0] = 0;
    mainModule->posClientIds[1] = 1;
    mainModule->posClientIds[2] = 0;
    mainModule->posClientIds[3] = 1;
    mainModule->posClientIds[4] = 0;
    mainModule->posClientIds[5] = 1;
    BtlMain_SetupPlayerAndParties(mainModule, setup);
    PokeCon_Init(&mainModule->pokeCons[0], mainModule, FALSE);
    PokeCon_CreateClientParty(&mainModule->pokeCons[0], mainModule, 0);
    PokeCon_CreateClientParty(&mainModule->pokeCons[0], mainModule, 1);
    PokeCon_Init(&mainModule->pokeCons[1], mainModule, TRUE);
    PokeCon_CreateClientParty(&mainModule->pokeCons[1], mainModule, 0);
    PokeCon_CreateClientParty(&mainModule->pokeCons[1], mainModule, 1);
    BtlMain_SetupTrainers(mainModule, setup);
    mainModule->server = BattleServer_Create(mainModule, &mainModule->rand, &mainModule->pokeCons[1], commCompetitor,
                                             mainModule->heapId);
    mainModule->isServer = 1;
    mainModule->clients[0] =
        BattleClient_Create(mainModule, &mainModule->pokeCons[0], 0, setup->fieldSituation.netHandle, 0, 3, 0,
                            commCompetitor, setup->fieldSituation.unk1b, &mainModule->rand, mainModule->heapId);
    mainModule->clients[1] =
        BattleClient_Create(mainModule, &mainModule->pokeCons[0], 0, setup->fieldSituation.netHandle, 1, 3, 1,
                            commCompetitor, setup->fieldSituation.unk1b, &mainModule->rand, mainModule->heapId);
    BtlMain_SetupRecReader(mainModule, setup);
    BtlMain_CreateViewCore(mainModule, setup, commCompetitor);
    BattleServer_AttachClient(mainModule->server, BattleClient_GetAdapter(mainModule->clients[0]), 0, 3);
    BattleServer_AttachClient(mainModule->server, BattleClient_GetAdapter(mainModule->clients[1]), 1, 3);
    BattleServer_Start(mainModule->server);
    mainModule->mainFunc = BtlMain_MainLoopAlone;
    return TRUE;
}

BOOL BtlMain_SetupAloneRotation(u32 *state, BtlMainModule *mainModule) {
    BtlSetup *setup = mainModule->setup;
    u8 commCompetitor = BtlMain_IsCommCompetitor(setup);

    mainModule->clientCount = 2;
    mainModule->posClientIds[0] = 0;
    mainModule->posClientIds[1] = 1;
    mainModule->posClientIds[2] = 0;
    mainModule->posClientIds[3] = 1;
    mainModule->posClientIds[4] = 0;
    mainModule->posClientIds[5] = 1;
    BtlMain_SetupPlayerAndParties(mainModule, setup);
    PokeCon_Init(&mainModule->pokeCons[0], mainModule, FALSE);
    PokeCon_CreateClientParty(&mainModule->pokeCons[0], mainModule, 0);
    PokeCon_CreateClientParty(&mainModule->pokeCons[0], mainModule, 1);
    PokeCon_Init(&mainModule->pokeCons[1], mainModule, TRUE);
    PokeCon_CreateClientParty(&mainModule->pokeCons[1], mainModule, 0);
    PokeCon_CreateClientParty(&mainModule->pokeCons[1], mainModule, 1);
    mainModule->server = BattleServer_Create(mainModule, &mainModule->rand, &mainModule->pokeCons[1], commCompetitor,
                                             mainModule->heapId);
    mainModule->isServer = 1;
    BtlMain_SetupTrainers(mainModule, setup);
    mainModule->clients[0] =
        BattleClient_Create(mainModule, &mainModule->pokeCons[0], 0, setup->fieldSituation.netHandle, 0, 1, 0,
                            commCompetitor, setup->fieldSituation.unk1b, &mainModule->rand, mainModule->heapId);
    mainModule->clients[1] =
        BattleClient_Create(mainModule, &mainModule->pokeCons[0], 0, setup->fieldSituation.netHandle, 1, 1, 1,
                            commCompetitor, setup->fieldSituation.unk1b, &mainModule->rand, mainModule->heapId);
    BtlMain_SetupRecReader(mainModule, setup);
    BtlMain_CreateViewCore(mainModule, setup, commCompetitor);
    BattleServer_AttachClient(mainModule->server, BattleClient_GetAdapter(mainModule->clients[0]), 0, 1);
    BattleServer_AttachClient(mainModule->server, BattleClient_GetAdapter(mainModule->clients[1]), 1, 1);
    BattleServer_Start(mainModule->server);
    mainModule->mainFunc = BtlMain_MainLoopAlone;
    return TRUE;
}

BOOL BtlMain_SetupCommSingle(u32 *state, BtlMainModule *mainModule) {
    // The steps of setting up the link battle, after the first
    static BOOL (*const sCommSingleSetupSteps[7])(BtlMainModule *mainModule, s32 *state) = {
        NULL,
        BtlMain_CommExchangeSyncData,
        BtlMain_CommExchangeParties,
        BtlMain_CommExchangeTrainers,
        BtlMain_CommExchangeChatter,
        BtlMain_CommCreateSingle,
        BtlMain_CommCreateViewCore,
    };

    if (*state == 0) {
        mainModule->clientCount = 2;
        mainModule->posClientIds[0] = 0;
        mainModule->posClientIds[1] = 1;
        mainModule->playerClientId = mainModule->setup->fieldSituation.unk19;
        mainModule->playerPos = GetClientBattlePos(mainModule, mainModule->playerClientId, 0);
        (*state)++;
        return FALSE;
    }
    if (*state < 7) {
        if (sCommSingleSetupSteps[*state](mainModule, &mainModule->unk460)) {
            mainModule->unk460 = 0;
            (*state)++;
        }
        return FALSE;
    }
    return TRUE;
}

BOOL BtlMain_SetupCommDouble(u32 *state, BtlMainModule *mainModule) {
    // The steps of setting up the link battle, after the first
    static BOOL (*const sCommDoubleSetupSteps[7])(BtlMainModule *mainModule, s32 *state) = {
        NULL,
        BtlMain_CommExchangeSyncData,
        BtlMain_CommExchangeParties,
        BtlMain_CommExchangeTrainers,
        BtlMain_CommExchangeChatter,
        BtlMain_CommCreateDouble,
        BtlMain_CommCreateViewCore,
    };

    if (*state == 0) {
        switch (mainModule->setup->fieldSituation.multiMode) {
        case 0:
        default:
            mainModule->clientCount = 2;
            mainModule->posClientIds[0] = 0;
            mainModule->posClientIds[1] = 1;
            mainModule->posClientIds[2] = 0;
            mainModule->posClientIds[3] = 1;
            break;
        case 2:
            mainModule->npcCount = 2;
        case 1:
            mainModule->clientCount = 4;
            mainModule->posClientIds[0] = 0;
            mainModule->posClientIds[1] = 1;
            mainModule->posClientIds[2] = 2;
            mainModule->posClientIds[3] = 3;
            break;
        }
        mainModule->playerClientId = mainModule->setup->fieldSituation.unk19;
        mainModule->playerPos = GetClientBattlePos(mainModule, mainModule->playerClientId, 0);
        (*state)++;
        return FALSE;
    }
    if (*state < 7) {
        if (sCommDoubleSetupSteps[*state](mainModule, &mainModule->unk460)) {
            mainModule->unk460 = 0;
            (*state)++;
        }
        return FALSE;
    }
    return TRUE;
}

BOOL BtlMain_SetupCommRotation(u32 *state, BtlMainModule *mainModule) {
    // The steps of setting up the link battle, after the first
    static BOOL (*const sCommRotationSetupSteps[7])(BtlMainModule *mainModule, s32 *state) = {
        NULL,
        BtlMain_CommExchangeSyncData,
        BtlMain_CommExchangeParties,
        BtlMain_CommExchangeTrainers,
        BtlMain_CommExchangeChatter,
        BtlMain_CommCreateRotation,
        BtlMain_CommCreateViewCore,
    };

    if (*state == 0) {
        mainModule->clientCount = 2;
        mainModule->posClientIds[0] = 0;
        mainModule->posClientIds[1] = 1;
        mainModule->posClientIds[2] = 0;
        mainModule->posClientIds[3] = 1;
        mainModule->posClientIds[4] = 0;
        mainModule->posClientIds[5] = 1;
        mainModule->playerClientId = mainModule->setup->fieldSituation.unk19;
        mainModule->playerPos = GetClientBattlePos(mainModule, mainModule->playerClientId, 0);
        (*state)++;
        return FALSE;
    }
    if (*state < 7) {
        if (sCommRotationSetupSteps[*state](mainModule, &mainModule->unk460)) {
            mainModule->unk460 = 0;
            (*state)++;
        }
        return FALSE;
    }
    return TRUE;
}

BOOL BtlMain_SetupCommTriple(u32 *state, BtlMainModule *mainModule) {
    // The steps of setting up the link battle, after the first
    static BOOL (*const sCommTripleSetupSteps[7])(BtlMainModule *mainModule, s32 *state) = {
        NULL,
        BtlMain_CommExchangeSyncData,
        BtlMain_CommExchangeParties,
        BtlMain_CommExchangeTrainers,
        BtlMain_CommExchangeChatter,
        BtlMain_CommCreateTriple,
        BtlMain_CommCreateViewCore,
    };

    if (*state == 0) {
        mainModule->clientCount = 2;
        mainModule->posClientIds[0] = 0;
        mainModule->posClientIds[1] = 1;
        mainModule->posClientIds[2] = 0;
        mainModule->posClientIds[3] = 1;
        mainModule->posClientIds[4] = 0;
        mainModule->posClientIds[5] = 1;
        mainModule->playerClientId = mainModule->setup->fieldSituation.unk19;
        mainModule->playerPos = GetClientBattlePos(mainModule, mainModule->playerClientId, 0);
        (*state)++;
        return FALSE;
    }
    if (*state < 7) {
        if (sCommTripleSetupSteps[*state](mainModule, &mainModule->unk460)) {
            mainModule->unk460 = 0;
            (*state)++;
        }
        return FALSE;
    }
    return TRUE;
}

BOOL BtlMain_CommExchangeSyncData(BtlMainModule *mainModule, s32 *state) {
    if (BtlNet_CheckError()) {
        mainModule->commError = 1;
        return TRUE;
    }
    switch (*state) {
    case 0:
        if (BtlNet_SendVersion(mainModule->playerClientId)) {
            (*state)++;
        }
        break;
    case 1:
        if (BtlNet_IsServerDecided()) {
            mainModule->unk46A = 0;
            mainModule->isServer = BtlNet_IsServer();
            mainModule->setup->unkAD = BtlNet_GetCommonVersion();
            if (BtlNet_IsServer()) {
                *state = 2;
            } else {
                *state = 3;
            }
        }
        break;
    case 2:
        mainModule->syncData.rand = mainModule->rand;
        mainModule->syncData.unk18 = mainModule->setup->unkA2;
        mainModule->syncData.gameTimeLimit = mainModule->setup->unk8C;
        mainModule->syncData.commandTimeLimit = mainModule->setup->unk8E;
        if (BtlSetup_IsBattleType(mainModule, 0x1000)) {
            mainModule->syncData.textSpeed = mainModule->textSpeed;
            mainModule->syncData.battleSceneOff = mainModule->battleSceneOff;
        } else {
            mainModule->syncData.textSpeed = 2;
            mainModule->syncData.battleSceneOff = 1;
        }
        if (BtlNet_SendSyncData(&mainModule->syncData)) {
            (*state)++;
        }
        break;
    case 3:
        if (BtlNet_GetSyncData(&mainModule->syncData)) {
            BtlSetup *setup = mainModule->setup;
            setup->unkA2 = mainModule->syncData.unk18;
            setup->rand = mainModule->syncData.rand;
            mainModule->rand = mainModule->syncData.rand;
            mainModule->battleSceneOff = mainModule->syncData.battleSceneOff;
            mainModule->textSpeed = mainModule->syncData.textSpeed;
            mainModule->gameTimeLimit = mainModule->syncData.gameTimeLimit;
            mainModule->commandTimeLimit = mainModule->syncData.commandTimeLimit;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL BtlMain_CommExchangeParties(BtlMainModule *mainModule, s32 *state) {
    BtlSetup *setup = mainModule->setup;
    u32 i;
    u8 index;

    if (BtlNet_CheckError()) {
        mainModule->commError = 1;
        return TRUE;
    }
    switch (*state) {
    case 0:
        BtlNet_StartTiming(2);
        (*state)++;
        break;
    case 1:
        if (BtlNet_CheckTiming(2)) {
            (*state)++;
        }
        break;
    case 2:
        if (BtlNet_SendParty(setup->party[0])) {
            (*state)++;
        }
        break;
    case 3:
        if (BtlNet_ArePartiesReceived()) {
            if (mainModule->npcCount == 0) {
                *state = 10;
                break;
            }
            if (mainModule->isServer) {
                mainModule->partyPacket =
                    BtlNet_CreatePartyPacket(PokeParty_GetSaveDataSize(), HEAPID_TAIL(mainModule->heapId));
            }
            mainModule->npcIndex = 0;
            (*state)++;
        }
        break;
    case 4:
        index = 1;
        if (mainModule->npcIndex != 0) {
            index = 3;
        }
        mainModule->npcClientId = index;
        if (mainModule->isServer) {
            BtlNet_SetPartyPacket(mainModule->partyPacket, setup->party[mainModule->npcClientId],
                                  mainModule->npcClientId);
        }
        BtlNet_StartTiming(mainModule->npcIndex + 6);
        (*state)++;
        break;
    case 5:
        if (BtlNet_CheckTiming(mainModule->npcIndex + 6)) {
            (*state)++;
        }
        break;
    case 6:
        if (!mainModule->isServer || BtlNet_SendPartyPacket(mainModule->partyPacket)) {
            (*state)++;
        }
        break;
    case 7:
        if (BtlNet_IsClientPartyReceived(mainModule->npcClientId)) {
            mainModule->npcIndex++;
            if (mainModule->npcIndex >= mainModule->npcCount) {
                if (mainModule->isServer) {
                    BtlNet_FreePartyPacket(mainModule->partyPacket);
                    mainModule->partyPacket = NULL;
                }
                *state = 10;
            } else {
                *state = 4;
            }
        }
        break;
    case 10:
        PokeCon_Init(&mainModule->pokeCons[0], mainModule, FALSE);
        for (i = 0; i < 4; i++) {
            if (DoesClientExist(mainModule, i)) {
                SetClientSrcParty(mainModule, i, BtlNet_GetParty(i));
                SetSetupParty(mainModule, i, BtlNet_GetParty(i));
                PokeCon_CreateClientParty(&mainModule->pokeCons[0], mainModule, i);
            }
        }
        (*state)++;
        break;
    case 11:
        PokeCon_Init(&mainModule->pokeCons[1], mainModule, TRUE);
        for (i = 0; i < 4; i++) {
            if (DoesClientExist(mainModule, i)) {
                PokeCon_CreateClientParty(&mainModule->pokeCons[1], mainModule, i);
            }
        }
        (*state)++;
        break;
    case 12:
        BtlNet_EndPartyExchange();
        (*state)++;
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

BOOL BtlMain_CommExchangeTrainers(BtlMainModule *mainModule, s32 *state) {
    BtlSetup *setup = mainModule->setup;
    u32 i;
    PlayerInfo *info;
    MsgData *msgData;
    StrBuf *strBuf;
    const u16 *str;
    s32 j;
    u16 name[16];
    u8 index;

    if (BtlNet_CheckError()) {
        mainModule->commError = 1;
        return TRUE;
    }
    switch (*state) {
    case 0:
        BtlNet_StartTiming(4);
        (*state)++;
        break;
    case 1:
        if (BtlNet_CheckTiming(4)) {
            (*state)++;
        }
        break;
    case 2:
        if (BtlNet_SendPlayerInfo(mainModule->playerInfo)) {
            (*state)++;
        }
        break;
    case 3:
        if (BtlNet_ArePlayerInfosReceived()) {
            for (i = 0; i < mainModule->clientCount; i++) {
                info = BtlNet_GetPlayerInfo(i);
                if (info != NULL) {
                    TrainerData_SetFromPlayer(&mainModule->trainers[i], mainModule->heapId, info);
                    if (i != mainModule->playerClientId && setup->unkDD_2) {
                        j = 0;
                        msgData = GFL_MsgSysLoadData(0, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_HANDLERS_BTL_MAIN,
                                                     mainModule->heapId);
                        strBuf = GFL_MsgDataLoadStrbufNew(msgData, TEXT_HIDDEN_TRAINER_NAME);
                        GFL_MsgDataFree(msgData);
                        str = GFL_StrBufGetStringPtr(strBuf);
                        for (; j < 15 && j < GFL_StrBufGetCharCount(strBuf); j++) {
                            name[j] = str[j];
                        }
                        name[j] = GFL_StrBufGetTerminator();
                        GFL_StrBufFree(strBuf);
                        copyTrainerName(mainModule->trainers[i].playerInfo, name);
                        GFL_StrBufFree(mainModule->trainers[i].name);
                        msgData = GFL_MsgSysLoadData(0, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_HANDLERS_BTL_MAIN,
                                                     mainModule->heapId);
                        strBuf = GFL_MsgDataLoadStrbufNew(msgData, TEXT_HIDDEN_TRAINER_NAME);
                        GFL_MsgDataFree(msgData);
                        mainModule->trainers[i].name = strBuf;
                    }
                    SetSetupPlayerInfo(mainModule, i, mainModule->trainers[i].playerInfo);
                }
            }
            (*state)++;
        }
        break;
    case 4:
        if (mainModule->npcCount == 0) {
            *state = 10;
            break;
        }
        mainModule->npcIndex = 0;
        (*state)++;
        break;
    case 5:
        BtlNet_StartTiming(mainModule->npcIndex + 8);
        (*state)++;
        break;
    case 6:
        if (BtlNet_CheckTiming(mainModule->npcIndex + 8)) {
            (*state)++;
        }
        break;
    case 7:
        index = 1;
        if (mainModule->npcIndex != 0) {
            index = 3;
        }
        mainModule->npcClientId = index;
        if (mainModule->isServer && !BtlNet_SendTrainer(setup->trainers[mainModule->npcClientId])) {
            break;
        }
        (*state)++;
    case 8:
        if (BtlNet_IsTrainerReceived()) {
            TrainerData_SetFromComm(&mainModule->trainers[mainModule->npcClientId], BtlNet_GetTrainer());
            BtlNet_FreeTrainer();
            mainModule->npcIndex++;
            if (mainModule->npcIndex >= mainModule->npcCount) {
                *state = 10;
            } else {
                *state = 5;
            }
        }
        break;
    case 10:
        BtlNet_EndTrainerExchange();
        (*state)++;
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

BOOL BtlMain_CommExchangeChatter(BtlMainModule *mainModule, s32 *state) {
    BtlSetup *setup = mainModule->setup;
    u32 i;
    PerapVoice *chatter;

    if (BtlNet_CheckError()) {
        mainModule->commError = 1;
        return TRUE;
    }
    switch (*state) {
    case 0:
        BtlNet_BeginChatterExchange();
        BtlNet_StartTiming(3);
        (*state)++;
        break;
    case 1:
        if (BtlNet_CheckTiming(3)) {
            (*state)++;
        }
        break;
    case 2:
        if (BtlNet_SendChatter(getChatterBlockAddress(GameData_GetSaveControl(setup->gameData)))) {
            (*state)++;
        }
        break;
    case 3:
        if (BtlNet_AreChattersReceived()) {
            for (i = 0; i < 4; i++) {
                if (DoesClientExist(mainModule, i) && !IsClientNPCTrainer(mainModule, i)) {
                    chatter = BtlNet_GetChatter(i);
                    if (chatter != NULL) {
                        mainModule->chatters[i] = allocChatotChatterBlk(mainModule->heapId);
                        MI_CpuCopy8(chatter, mainModule->chatters[i], PerapVoice_GetSize());
                    }
                }
                SetSetupChatterEffect(mainModule, i);
            }
            BtlNet_EndChatterExchange();
            (*state)++;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

BOOL BtlMain_CommCreateSingle(BtlMainModule *mainModule, s32 *state) {
    BtlSetup *setup = mainModule->setup;
    u8 clientId = mainModule->playerClientId;
    u8 commCompetitor = BtlMain_IsCommCompetitor(setup);
    u32 opponent;
    u32 i;

    if (BtlNet_CheckError()) {
        mainModule->commError = 1;
        return TRUE;
    }
    if (mainModule->isServer) {
        mainModule->server = BattleServer_Create(mainModule, &mainModule->rand, &mainModule->pokeCons[1],
                                                 commCompetitor, mainModule->heapId);
        mainModule->clients[clientId] = BattleClient_Create(
            mainModule, &mainModule->pokeCons[0], setup->fieldSituation.unk18, setup->fieldSituation.netHandle,
            clientId, 1, 0, commCompetitor, 0, &mainModule->rand, mainModule->heapId);
        opponent = 1;
        BattleServer_AttachClient(mainModule->server, BattleClient_GetAdapter(mainModule->clients[clientId]), clientId,
                                  1);
        if (clientId != 0) {
            opponent = 0;
        }
        BattleServer_AttachNetClient(mainModule->server, setup->fieldSituation.unk18, setup->fieldSituation.netHandle,
                                     opponent, 1);
    } else {
        mainModule->clients[clientId] = BattleClient_Create(
            mainModule, &mainModule->pokeCons[0], setup->fieldSituation.unk18, setup->fieldSituation.netHandle,
            clientId, 1, 0, commCompetitor, 0, &mainModule->rand, mainModule->heapId);
        mainModule->cmdCheckServer = BattleServer_Create(mainModule, &mainModule->rand, &mainModule->pokeCons[1],
                                                         commCompetitor, mainModule->heapId);
        for (i = 0; i < 4; i++) {
            if (DoesClientExist(mainModule, i)) {
                BattleServer_AttachCmdCheckClient(mainModule->cmdCheckServer, i, 1);
            }
        }
        BattleClient_SetCmdCheckServer(mainModule->clients[clientId], mainModule->cmdCheckServer);
    }
    return TRUE;
}

BOOL BtlMain_CommCreateDouble(BtlMainModule *mainModule, s32 *state) {
    BtlSetup *setup = mainModule->setup;
    u8 clientId = mainModule->playerClientId;
    u8 commCompetitor = BtlMain_IsCommCompetitor(setup);
    u8 posCount = setup->fieldSituation.multiMode == 0 ? 2 : 1;
    u8 ally;
    u32 i;

    if (BtlNet_CheckError()) {
        mainModule->commError = 1;
        return TRUE;
    }
    if (mainModule->isServer) {
        mainModule->server = BattleServer_Create(mainModule, &mainModule->rand, &mainModule->pokeCons[1],
                                                 commCompetitor, mainModule->heapId);
        mainModule->clients[clientId] = BattleClient_Create(
            mainModule, &mainModule->pokeCons[0], setup->fieldSituation.unk18, setup->fieldSituation.netHandle,
            clientId, posCount, 0, commCompetitor, 0, &mainModule->rand, mainModule->heapId);
        BattleServer_AttachClient(mainModule->server, BattleClient_GetAdapter(mainModule->clients[clientId]), clientId,
                                  posCount);
        if (setup->fieldSituation.multiMode == 2) {
            ally = GetAllyClientID(mainModule, clientId);
            mainModule->clients[1] = BattleClient_Create(
                mainModule, &mainModule->pokeCons[0], setup->fieldSituation.unk18, setup->fieldSituation.netHandle, 1,
                posCount, 1, commCompetitor, 0, &mainModule->rand, mainModule->heapId);
            BattleServer_AttachClient(mainModule->server, BattleClient_GetAdapter(mainModule->clients[1]), 1, posCount);
            mainModule->clients[3] = BattleClient_Create(
                mainModule, &mainModule->pokeCons[0], setup->fieldSituation.unk18, setup->fieldSituation.netHandle, 3,
                posCount, 1, commCompetitor, 0, &mainModule->rand, mainModule->heapId);
            BattleServer_AttachClient(mainModule->server, BattleClient_GetAdapter(mainModule->clients[3]), 3, posCount);
            BattleServer_AttachNetClient(mainModule->server, setup->fieldSituation.unk18,
                                         setup->fieldSituation.netHandle, ally, posCount);
        } else {
            for (i = 0; i < mainModule->clientCount; i++) {
                if (i != clientId) {
                    BattleServer_AttachNetClient(mainModule->server, setup->fieldSituation.unk18,
                                                 setup->fieldSituation.netHandle, i, posCount);
                }
            }
        }
    } else {
        mainModule->clients[clientId] = BattleClient_Create(
            mainModule, &mainModule->pokeCons[0], setup->fieldSituation.unk18, setup->fieldSituation.netHandle,
            clientId, posCount, 0, commCompetitor, 0, &mainModule->rand, mainModule->heapId);
        mainModule->cmdCheckServer = BattleServer_Create(mainModule, &mainModule->rand, &mainModule->pokeCons[1],
                                                         commCompetitor, mainModule->heapId);
        for (i = 0; i < 4; i++) {
            if (DoesClientExist(mainModule, i)) {
                BattleServer_AttachCmdCheckClient(mainModule->cmdCheckServer, i, CalcClientBattlerCount(mainModule, i));
            }
        }
        BattleClient_SetCmdCheckServer(mainModule->clients[clientId], mainModule->cmdCheckServer);
    }
    return TRUE;
}

BOOL BtlMain_CommCreateTriple(BtlMainModule *mainModule, s32 *state) {
    BtlSetup *setup = mainModule->setup;
    u8 clientId = mainModule->playerClientId;
    u8 commCompetitor = BtlMain_IsCommCompetitor(setup);
    u32 opponent;
    u32 i;

    if (BtlNet_CheckError()) {
        mainModule->commError = 1;
        return TRUE;
    }
    if (mainModule->isServer) {
        mainModule->server = BattleServer_Create(mainModule, &mainModule->rand, &mainModule->pokeCons[1],
                                                 commCompetitor, mainModule->heapId);
        mainModule->clients[clientId] = BattleClient_Create(
            mainModule, &mainModule->pokeCons[0], setup->fieldSituation.unk18, setup->fieldSituation.netHandle,
            clientId, 3, 0, commCompetitor, 0, &mainModule->rand, mainModule->heapId);
        opponent = 0;
        BattleServer_AttachClient(mainModule->server, BattleClient_GetAdapter(mainModule->clients[clientId]), clientId,
                                  3);
        if (clientId == 0) {
            opponent = 1;
        }
        BattleServer_AttachNetClient(mainModule->server, setup->fieldSituation.unk18, setup->fieldSituation.netHandle,
                                     opponent, 3);
    } else {
        mainModule->clients[clientId] = BattleClient_Create(
            mainModule, &mainModule->pokeCons[0], setup->fieldSituation.unk18, setup->fieldSituation.netHandle,
            clientId, 3, 0, commCompetitor, 0, &mainModule->rand, mainModule->heapId);
        mainModule->cmdCheckServer = BattleServer_Create(mainModule, &mainModule->rand, &mainModule->pokeCons[1],
                                                         commCompetitor, mainModule->heapId);
        for (i = 0; i < 4; i++) {
            if (DoesClientExist(mainModule, i)) {
                BattleServer_AttachCmdCheckClient(mainModule->cmdCheckServer, i, CalcClientBattlerCount(mainModule, i));
            }
        }
        BattleClient_SetCmdCheckServer(mainModule->clients[clientId], mainModule->cmdCheckServer);
    }
    return TRUE;
}

BOOL BtlMain_CommCreateRotation(BtlMainModule *mainModule, s32 *state) {
    BtlSetup *setup = mainModule->setup;
    u8 clientId = mainModule->playerClientId;
    u8 commCompetitor = BtlMain_IsCommCompetitor(setup);
    u32 i;

    if (BtlNet_CheckError()) {
        mainModule->commError = 1;
        return TRUE;
    }
    if (mainModule->isServer) {
        mainModule->server = BattleServer_Create(mainModule, &mainModule->rand, &mainModule->pokeCons[1],
                                                 commCompetitor, mainModule->heapId);
        i = 0;
        mainModule->clients[clientId] = BattleClient_Create(
            mainModule, &mainModule->pokeCons[0], setup->fieldSituation.unk18, setup->fieldSituation.netHandle,
            clientId, 1, i, commCompetitor, i, &mainModule->rand, mainModule->heapId);
        BattleServer_AttachClient(mainModule->server, BattleClient_GetAdapter(mainModule->clients[clientId]), clientId,
                                  1);
        for (; i < mainModule->clientCount; i++) {
            if (i != clientId) {
                BattleServer_AttachNetClient(mainModule->server, setup->fieldSituation.unk18,
                                             setup->fieldSituation.netHandle, i, 1);
            }
        }
    } else {
        i = 0;
        mainModule->clients[clientId] = BattleClient_Create(
            mainModule, &mainModule->pokeCons[0], setup->fieldSituation.unk18, setup->fieldSituation.netHandle,
            clientId, 1, i, commCompetitor, i, &mainModule->rand, mainModule->heapId);
        mainModule->cmdCheckServer = BattleServer_Create(mainModule, &mainModule->rand, &mainModule->pokeCons[1],
                                                         commCompetitor, mainModule->heapId);
        for (; i < 4; i++) {
            if (DoesClientExist(mainModule, i)) {
                BattleServer_AttachCmdCheckClient(mainModule->cmdCheckServer, i, CalcClientBattlerCount(mainModule, i));
            }
        }
        BattleClient_SetCmdCheckServer(mainModule->clients[clientId], mainModule->cmdCheckServer);
    }
    return TRUE;
}

BOOL BtlMain_CommCreateViewCore(BtlMainModule *mainModule, s32 *state) {
    if (BtlNet_CheckError()) {
        mainModule->commError = 1;
        return TRUE;
    }
    switch (*state) {
    case 0:
        mainModule->viewCore =
            BtlvCore_Create(mainModule, mainModule->clients[mainModule->playerClientId], &mainModule->pokeCons[0],
                            BtlMain_IsCommCompetitor(mainModule->setup), 0x13);
        BattleClient_SetViewCore(mainModule->clients[mainModule->playerClientId], mainModule->viewCore);
        if (mainModule->isServer) {
            mainModule->mainFunc = BtlMain_MainLoopCommServer;
            BattleServer_Start(mainModule->server);
        } else {
            mainModule->mainFunc = BtlMain_MainLoopCommClient;
        }
        (*state)++;
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

BOOL BtlMain_MainLoopAlone(BtlMainModule *mainModule) {
    BOOL done = FALSE;
    s32 i;

    BattleServer_Main(mainModule->server);
    for (i = 0; i < 4; i++) {
        if (mainModule->clients[i] != NULL && BattleClient_Main(mainModule->clients[i])) {
            BattleClient_GetClientIDList(mainModule->clients[i], &mainModule->clientIdList);
            done = TRUE;
        }
    }
    BtlvCore_Update(mainModule->viewCore);
    return done;
}

BOOL BtlMain_MainLoopCommServer(BtlMainModule *mainModule) {
    BOOL done;
    s32 i;

    if (!mainModule->commError) {
        if (BtlNet_CheckError()) {
            mainModule->commError = 1;
            BtlMain_ForceQuitClients(mainModule);
        }
    } else if (mainModule->setupFailed) {
        return TRUE;
    }
    done = BattleServer_Main(mainModule->server);
    if (done) {
        BattleServer_GetClientIDList(mainModule->server, &mainModule->clientIdList);
    }
    for (i = 0; i < 4; i++) {
        if (mainModule->clients[i] != NULL && mainModule->clientsDone[i] == 0) {
            mainModule->clientsDone[i] = BattleClient_Main(mainModule->clients[i]);
        }
    }
    if (BtlMain_AreAllClientsDone(mainModule)) {
        BattleClient_GetClientIDList(mainModule->clients[mainModule->playerClientId], &mainModule->clientIdList);
        done = TRUE;
    }
    BtlvCore_Update(mainModule->viewCore);
    return done;
}

BOOL BtlMain_MainLoopCommClient(BtlMainModule *mainModule) {
    BOOL done;
    s32 i;

    if (!mainModule->commError) {
        if (BtlNet_CheckError()) {
            mainModule->commError = 1;
            BtlMain_ForceQuitClients(mainModule);
        }
    } else if (mainModule->setupFailed) {
        return TRUE;
    }
    done = FALSE;
    for (i = 0; i < 4; i++) {
        if (mainModule->clients[i] != NULL && BattleClient_Main(mainModule->clients[i])) {
            BattleClient_GetClientIDList(mainModule->clients[i], &mainModule->clientIdList);
            done = TRUE;
        }
    }
    BtlvCore_Update(mainModule->viewCore);
    return done;
}

void BtlMain_ForceQuitClients(BtlMainModule *mainModule) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (mainModule->clients[i] != NULL) {
            BattleClient_SetForceQuit(mainModule->clients[i]);
        }
    }
}

BOOL BtlMain_AreAllClientsDone(BtlMainModule *mainModule) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (mainModule->clients[i] != NULL && mainModule->clientsDone[i] == 0) {
            return FALSE;
        }
    }
    return TRUE;
}

// Layout reconstructed from the opponent-position lookup in the game code.
u32 BtlSetup_GetBattleStyle(BtlMainModule *mainModule) {
    return mainModule->setup->battleStyle;
}

u32 IsBattleSceneOff(BtlMainModule *mainModule) {
    return mainModule->battleSceneOff;
}

u8 GetBadgeCount(BtlMainModule *mainModule) {
    return mainModule->setup->unk98;
}

BOOL IsSwitchMode(BtlMainModule *mainModule) {
    if (BtlSetup_GetBattleType(mainModule) == BTL_TYPE_TRAINER &&
        BtlSetup_GetBattleStyle(mainModule) == BTL_STYLE_SINGLE && GetCommMode(mainModule) == 0 &&
        GetStudioMode(mainModule) == BTL_STUDIO_MODE_NONE && Config_GetBattleStyle(mainModule->setup->config) == 0) {
        return TRUE;
    }
    return FALSE;
}

s32 GetTextWait(BtlMainModule *mainModule) {
    return TextSpeed_GetWait(mainModule->textSpeed);
}

void SetFastestTextSpeed(BtlMainModule *mainModule) {
    mainModule->textSpeed = 3;
}

BOOL IsExpGainEnabled(BtlMainModule *mainModule) {
    BOOL result;

    if (mainModule->setup->battleType <= BTL_TYPE_TRAINER) {
        GFL_OvlLoad(OVERLAY_ID(338));
        result = IrCheck_IsGenuineCard();
        GFL_OvlUnload(OVERLAY_ID(338));
        if (!result) {
            return FALSE;
        }
        if (!BtlSetup_IsBattleType(mainModule, 0x200) && !GetStudioMode(mainModule)) {
            return TRUE;
        }
    }
    return FALSE;
}

CommPlayerSupport *GetCommPlayerSupport(BtlMainModule *mainModule) {
    if (mainModule->setup->battleType <= BTL_TYPE_TRAINER) {
        return mainModule->setup->unk88;
    }
    return NULL;
}

u32 GetValidPosMax(BtlMainModule *mainModule) {
    switch (mainModule->setup->battleStyle) {
    case BTL_STYLE_SINGLE:
        return 1;
    case BTL_STYLE_DOUBLE:
        return 3;
    case BTL_STYLE_TRIPLE:
        return 5;
    case BTL_STYLE_ROTATION:
        return 5;
    default:
        return 5;
    }
}

u32 GetActivePosCount(BtlMainModule *mainModule) {
    switch (mainModule->setup->battleStyle) {
    case BTL_STYLE_SINGLE:
        return 1;
    case BTL_STYLE_DOUBLE:
        return 2;
    case BTL_STYLE_TRIPLE:
        return 3;
    case BTL_STYLE_ROTATION:
        return 1;
    default:
        return 1;
    }
}

BOOL IsActivePos(BtlMainModule *mainModule, u32 pos) {
    return pos < GetActivePosCount(mainModule) * 2;
}

u32 BtlSetup_GetBattleType(BtlMainModule *mainModule) {
    return mainModule->setup->battleType;
}

u8 GetMultiMode(BtlMainModule *mainModule) {
    return mainModule->setup->fieldSituation.multiMode;
}

u8 GetCommMode(BtlMainModule *mainModule) {
    return mainModule->setup->fieldSituation.unk18;
}

BOOL IsMultiMode(BtlMainModule *mainModule) {
    return mainModule->setup->fieldSituation.multiMode != 0;
}

u16 GetWinBGM(BtlMainModule *mainModule) {
    return mainModule->setup->fieldSituation.winBgm;
}

u16 GetStudioEndingBGM0(BtlMainModule *mainModule) {
    return mainModule->setup->unk138;
}

u16 GetStudioEndingBGM2(BtlMainModule *mainModule) {
    return mainModule->setup->unk13a;
}

u32 GetRunMode(BtlMainModule *mainModule) {
    switch (mainModule->setup->battleType) {
    case BTL_TYPE_WILD:
        return 0;
    case BTL_TYPE_TRAINER:
        if (GetStudioMode(mainModule) == BTL_STUDIO_MODE_FILM) {
            return 2;
        }
        return 1;
    case BTL_TYPE_FACILITY:
        return 2;
    case BTL_TYPE_COMM:
        return 2;
    default:
        return 1;
    }
}

BtlFieldSituation *GetFieldEffectData(BtlMainModule *mainModule) {
    return &mainModule->setup->fieldSituation;
}

PlayerInfo *GetPlayerInfo(BtlMainModule *mainModule) {
    return mainModule->playerInfo;
}

BOOL IsMonCaught(BtlMainModule *mainModule, BattleMon *mon) {
    return PokeDex_IsCaught(mainModule->setup->pokedex, GetBattleMonSpecies(mon));
}

u32 GetDexCaughtCount(BtlMainModule *mainModule) {
    return PokeDex_GetCaughtNoNational(mainModule->setup->pokedex);
}

GameData *GetGameData(BtlMainModule *mainModule) {
    return mainModule->setup->gameData;
}

void RegisterDexMon(BtlMainModule *mainModule, u8 clientId, BattleMon *mon) {
    u32 battleType = BtlSetup_GetBattleType(mainModule);
    u32 studioMode = GetStudioMode(mainModule);

    if (mainModule->setup->noPokedexRegistration != 1 && battleType <= BTL_TYPE_TRAINER &&
        studioMode == BTL_STUDIO_MODE_NONE && clientId != 0) {
        PokeDex_RegistPkm(mainModule->setup->pokedex, GetSrcData(mon));
    }
}

u8 ExpandPosFlags(BtlMainModule *mainModule, u16 posFlags, u8 *out) {
    u8 type = posFlags >> 8;
    u8 pos = posFlags;

    if (type == 0) {
        *out = pos;
        return *out != BTL_POS_NONE ? 1 : 0;
    }
    if (pos != BTL_POS_NONE) {
        switch (mainModule->setup->battleStyle) {
        case BTL_STYLE_SINGLE:
        default:
            return ExpandPosFlagsSingle(mainModule, type, pos, out);
        case BTL_STYLE_DOUBLE:
            return ExpandPosFlagsDouble(mainModule, type, pos, out);
        case BTL_STYLE_TRIPLE:
            return ExpandPosFlagsTriple(mainModule, type, pos, out);
        case BTL_STYLE_ROTATION:
            return ExpandPosFlagsSingle(mainModule, type, pos, out);
        }
    }
    *out = BTL_POS_NONE;
    return 0;
}

u8 ExpandPosFlagsSingle(BtlMainModule *mainModule, u8 type, u8 pos, u8 *out) {
    switch (type) {
    default:
        return 0;
    case 1:
    case 2:
    case 6:
        *out = GetOpposingPos(mainModule, pos, 0);
        break;
    case 3:
        *out = pos;
        break;
    case 5:
    case 8:
        out[0] = 0;
        out[1] = 1;
        return 2;
    }
    return 1;
}

u8 ExpandPosFlagsDouble(BtlMainModule *mainModule, u8 type, u8 pos, u8 *out) {
    switch (type) {
    default:
        return 0;
    case 1:
    case 6:
        out[0] = GetOpposingPos(mainModule, pos, 0);
        out[1] = GetOpposingPos(mainModule, pos, 1);
        return 2;
    case 2:
        out[0] = GetOpposingPos(mainModule, pos, 0);
        out[1] = GetOpposingPos(mainModule, pos, 1);
        out[2] = GetPartnerPos(mainModule, pos);
        return 3;
    case 3:
        out[0] = pos;
        out[1] = GetPartnerPos(mainModule, pos);
        if (out[0] > out[1]) {
            u8 tmp = out[0];

            out[0] = out[1];
            out[1] = tmp;
        }
        return 2;
    case 4:
    case 9:
        out[0] = GetPartnerPos(mainModule, pos);
        return 1;
    case 7:
        out[0] = pos;
        out[1] = GetPartnerPos(mainModule, pos);
        return 2;
    case 5:
    case 8:
        out[0] = 0;
        out[1] = 1;
        out[2] = 2;
        out[3] = 3;
        return 4;
    }
}

u8 ExpandPosFlagsTriple(BtlMainModule *mainModule, u8 type, u8 pos, u8 *out) {
    u8 isCenter = IsTripleCenterPos(pos);
    u8 index;

    switch (type) {
    default:
    case 0:
    case 1:
        if (isCenter) {
            out[0] = GetOpposingPos(mainModule, pos, 0);
            out[1] = GetOpposingPos(mainModule, pos, 1);
            out[2] = GetOpposingPos(mainModule, pos, 2);
            return 3;
        }
        out[0] = GetOpposingPos(mainModule, pos, 1);
        index = pos >> 1;
        if (index == 0) {
            index = 2;
        } else if (index == 2) {
            index = 0;
        }
        out[1] = GetOpposingPosForStyle(2, pos, index);
        return 2;
    case 6:
        out[0] = GetOpposingPos(mainModule, pos, 0);
        out[1] = GetOpposingPos(mainModule, pos, 1);
        out[2] = GetOpposingPos(mainModule, pos, 2);
        return 3;
    case 7:
        out[0] = GetPosOnSameSide(pos, 0);
        out[1] = GetPosOnSameSide(pos, 1);
        out[2] = GetPosOnSameSide(pos, 2);
        return 3;
    case 8:
        out[0] = GetOpposingPos(mainModule, pos, 0);
        out[1] = GetOpposingPos(mainModule, pos, 1);
        out[2] = GetOpposingPos(mainModule, pos, 2);
        out[3] = GetPosOnSameSide(pos, 0);
        out[4] = GetPosOnSameSide(pos, 1);
        out[5] = GetPosOnSameSide(pos, 2);
        return 6;
    case 2:
        if (isCenter) {
            out[0] = GetOpposingPos(mainModule, pos, 0);
            out[1] = GetOpposingPos(mainModule, pos, 1);
            out[2] = GetOpposingPos(mainModule, pos, 2);
            out[3] = GetPosOnSameSide(pos, 0);
            out[4] = GetPosOnSameSide(pos, 2);
            return 5;
        }
        out[0] = GetOpposingPos(mainModule, pos, 1);
        index = pos >> 1;
        if (index == 0) {
            index = 2;
        } else if (index == 2) {
            index = 0;
        }
        out[1] = GetOpposingPosForStyle(2, pos, index);
        out[2] = GetPosOnSameSide(pos, 1);
        return 3;
    case 3:
        if (isCenter) {
            out[0] = pos;
            out[1] = GetPosOnSameSide(pos, 0);
            out[2] = GetPosOnSameSide(pos, 2);
            return 3;
        }
        out[0] = pos;
        out[1] = GetPosOnSameSide(pos, 1);
        return 2;
    case 9:
        if (isCenter) {
            out[0] = GetPosOnSameSide(pos, 0);
            out[1] = GetPosOnSameSide(pos, 2);
            return 2;
        }
        out[0] = GetPosOnSameSide(pos, 1);
        return 1;
    case 4:
        if (isCenter) {
            out[0] = GetPosOnSameSide(pos, 0);
            out[1] = GetPosOnSameSide(pos, 2);
            return 2;
        }
        out[0] = GetPosOnSameSide(pos, 1);
        return 1;
    case 5:
        if (isCenter) {
            out[0] = GetOpposingPos(mainModule, pos, 0);
            out[1] = GetOpposingPos(mainModule, pos, 1);
            out[2] = GetOpposingPos(mainModule, pos, 2);
            out[3] = GetPosOnSameSide(pos, 0);
            out[4] = GetPosOnSameSide(pos, 2);
            out[5] = pos;
            return 6;
        }
        out[0] = GetOpposingPos(mainModule, pos, 1);
        index = pos >> 1;
        if (index == 0) {
            index = 2;
        } else if (index == 2) {
            index = 0;
        }
        out[1] = GetOpposingPosForStyle(2, pos, index);
        out[2] = GetPosOnSameSide(pos, 1);
        out[3] = pos;
        return 4;
    }
}

u32 GetClientPosCount(BtlMainModule *mainModule, u8 clientId) {
    u32 count;
    u32 i;

    for (i = 0, count = 0; i < BTL_POS_MAX; i++) {
        if (clientId == mainModule->posClientIds[i]) {
            count++;
        }
    }
    return count;
}

BOOL DoesClientExist(BtlMainModule *mainModule, u8 clientId) {
    u32 i;

    if (clientId < 4) {
        for (i = 0; i < 6; i++) {
            if (mainModule->posClientIds[i] == clientId) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

u8 GetClientSide(BtlMainModule *mainModule, u8 clientId) {
    return clientId & 1;
}

BOOL IsPlayerSide(BtlMainModule *mainModule, u8 side) {
    if (side == GetClientSide(mainModule, mainModule->playerClientId)) {
        return TRUE;
    }
    return FALSE;
}

u8 GetClientBattlePos(BtlMainModule *mainModule, u8 clientId, u8 slot) {
    u8 pos;

    for (pos = 0; pos < BTL_POS_MAX; pos++) {
        if (clientId == mainModule->posClientIds[pos]) {
            if (slot == 0) {
                return pos;
            }
            slot--;
        }
    }
    return BTL_POS_MAX;
}

u8 GetOpposingPosForStyle(u32 battleStyle, u8 pos, u8 index) {
    if ((pos & 1) == 0) {
        return index * 2 + 1;
    }
    return index * 2;
}

u8 GetPosOnSameSide(u8 pos, u8 index) {
    if ((pos & 1) == 0) {
        return index * 2;
    }
    return index * 2 + 1;
}

u8 GetOpposingPos(BtlMainModule *mainModule, u8 pos, u8 index) {
    return GetOpposingPosForStyle(mainModule->setup->battleStyle, pos, index);
}

u8 GetFacingPos(u32 battleStyle, u8 pos) {
    u8 index;

    switch (battleStyle) {
    default:
    case BTL_STYLE_SINGLE:
        return GetOpposingPosForStyle(battleStyle, pos, 0);
    case BTL_STYLE_DOUBLE:
        return GetOpposingPosForStyle(battleStyle, pos, (u8)(pos >> 1) ^ 1);
    case BTL_STYLE_TRIPLE:
        index = pos >> 1;
        if (index == 0) {
            index = 2;
        } else if (index == 2) {
            index = 0;
        }
        return GetOpposingPosForStyle(battleStyle, pos, index);
    case BTL_STYLE_ROTATION:
        return GetOpposingPosForStyle(battleStyle, pos, 0);
    }
}

u8 GetPartnerPos(BtlMainModule *mainModule, u8 pos) {
    switch (mainModule->setup->battleStyle) {
    case BTL_STYLE_SINGLE:
        return pos;
    case BTL_STYLE_ROTATION:
        return pos;
    case BTL_STYLE_DOUBLE:
        return (pos + 2) & 3;
    case BTL_STYLE_TRIPLE:
        return pos;
    default:
        return pos;
    }
}

BOOL AreClientsOnOppositeSides(BtlMainModule *mainModule, u8 clientId1, u8 clientId2) {
    return (clientId1 & 1) != (clientId2 & 1);
}

u8 BattlePosToClientID(BtlMainModule *mainModule, u8 pos) {
    return mainModule->posClientIds[pos];
}

u8 MonIDToClientID(u8 monId) {
    u8 i;
    u8 first;
    u8 last;

    for (i = 0; i < 4; i++) {
        first = sClientFirstMonIds[i];
        last = first + 5;
        if (monId >= first && monId <= last) {
            return i;
        }
    }
    return 0;
}

u8 ExpandPosFlagsToMonIDs(BtlMainModule *mainModule, BtlPokeCon *pokeCon, u16 posFlags, u8 *monIds) {
    u8 positions[8];
    u8 count;
    u8 found;
    u8 i;
    BattleMon *mon;

    count = ExpandPosFlags(mainModule, posFlags, positions);
    for (i = 0, found = 0; i < count; i++) {
        mon = PokeCon_GetPosMon(pokeCon, positions[i]);
        if (mon != NULL && !IsFainted(mon)) {
            monIds[found++] = GetMonID(mon);
        }
    }
    return found;
}

u8 MonIDToBattlePos(BtlMainModule *mainModule, BtlPokeCon *pokeCon, u8 monId) {
    u8 clientId;
    s32 slot;
    u8 pos;

    clientId = MonIDToClientID(monId);
    slot = PokeCon_FindClientMonIndex(pokeCon, clientId, monId);
    if (slot >= 0) {
        pos = GetClientBattlePos(mainModule, clientId, slot);
        if (pos != BTL_POS_NONE) {
            return pos;
        }
    }
    return BTL_POS_NONE;
}

u8 MonIDToViewPos(BtlMainModule *mainModule, BtlPokeCon *pokeCon, u8 monId) {
    u8 pos = MonIDToBattlePos(mainModule, pokeCon, monId);

    if (pos != BTL_POS_MAX) {
        return BattlePosToViewPos(mainModule, pos);
    }
    return 0xff;
}

u8 GetClientIDFromMonID(u8 monId) {
    return MonIDToClientID(monId);
}

u8 GetClientIDFromPos(BtlMainModule *mainModule, u8 pos) {
    return BattlePosToClientID(mainModule, pos);
}

// How many positions before pos belong to the same client
static inline u8 CountClientPosBefore(BtlMainModule *mainModule, u8 pos) {
    u8 clientId = BattlePosToClientID(mainModule, pos);
    u8 count = 0;

    while (pos-- != 0) {
        if (clientId == mainModule->posClientIds[pos]) {
            count++;
        }
    }
    return count;
}

u8 GetPosIndexInClient(BtlMainModule *mainModule, u8 pos) {
    return CountClientPosBefore(mainModule, pos);
}

void GetPosClientAndIndex(BtlMainModule *mainModule, u8 pos, u8 *clientId, u8 *index) {
    u8 i = pos;
    u8 count;

    *clientId = BattlePosToClientID(mainModule, pos);
    count = 0;
    while (i-- != 0) {
        if (*clientId == mainModule->posClientIds[i]) {
            count++;
        }
    }
    *index = count;
}

u8 BattlePosToViewPos(BtlMainModule *mainModule, u8 pos) {
    static const u8 sDoubleViewPositions[2][3] = {
        { 3, 5, 0 },
        { 2, 4, 0 },
    };

    static const u8 sTripleViewPositions[2][3] = {
        { 3, 5, 7 },
        { 2, 4, 6 },
    };

    u32 sameSide = TRUE;
    u8 side;
    u32 battleStyle;

    if ((mainModule->playerPos & 1) != (pos & 1)) {
        sameSide = FALSE;
    }
    side = sameSide;
    battleStyle = mainModule->setup->battleStyle;
    if (battleStyle == BTL_STYLE_SINGLE) {
        u32 result = 0;

        if (side == 0) {
            result = 1;
        }
        return result;
    }
    if (battleStyle == BTL_STYLE_DOUBLE) {
        return sDoubleViewPositions[side][(u8)(pos >> 1)];
    }
    return sTripleViewPositions[side][(u8)(pos >> 1)];
}

u8 ViewPosToBattlePos(BtlMainModule *mainModule, u8 index) {
    u8 isEnemy = index & 1;
    u8 playerSide;

    if (mainModule->setup->battleStyle == BTL_STYLE_SINGLE) {
        if (isEnemy) {
            return mainModule->playerPos == 0 ? 1 : 0;
        }
        return mainModule->playerPos;
    }
    playerSide = mainModule->playerPos & 1;
    if (isEnemy) {
        return (playerSide ^ 1) + (index - 3);
    }
    return playerSide + (index - 2);
}

BOOL BtlMain_IsCommMulti(BtlMainModule *mainModule) {
    if (IsMultiMode(mainModule)) {
        u32 battleType = BtlSetup_GetBattleType(mainModule);

        if (battleType != BTL_TYPE_TRAINER && battleType != BTL_TYPE_WILD) {
            return TRUE;
        }
    }
    return FALSE;
}

PokeParty *GetClientPokeParty(BtlMainModule *mainModule, u8 clientId) {
    PokeCon_StoreClientSrcParty(&mainModule->pokeCons[0], mainModule, clientId, FALSE);
    return PokeCon_GetSrcParty(&mainModule->pokeCons[0], clientId);
}

PokeParty *GetAllyPokeParty(BtlMainModule *mainModule, u8 clientId) {
    if (IsMultiMode(mainModule)) {
        clientId = (clientId + 2) & 3;
        if (DoesClientExist(mainModule, clientId)) {
            PokeCon_StoreClientSrcParty(&mainModule->pokeCons[0], mainModule, clientId, FALSE);
            return PokeCon_GetSrcParty(&mainModule->pokeCons[0], clientId);
        }
    }
    return NULL;
}

u8 GetMultiTrainerIndex(BtlMainModule *mainModule, u8 clientId) {
    if (IsMultiMode(mainModule)) {
        return (clientId & 2) ? 1 : 0;
    }
    return 0;
}

u8 GetPlayerMultiTrainerIndex(BtlMainModule *mainModule) {
    return GetMultiTrainerIndex(mainModule, mainModule->playerClientId);
}

u8 GetPlayerClientID(BtlMainModule *mainModule) {
    return mainModule->playerClientId;
}

u8 GetPlayerAllyClientID(BtlMainModule *mainModule) {
    return GetAllyClientID(mainModule, mainModule->playerClientId);
}

u8 GetAllyClientID(BtlMainModule *mainModule, u8 clientId) {
    u8 allyId = (clientId + 2) & 3;

    if (!DoesClientExist(mainModule, allyId)) {
        allyId = 4;
    }
    return allyId;
}

BOOL IsAllyClientID(u8 clientId1, u8 clientId2) {
    if (clientId1 == clientId2) {
        return TRUE;
    }
    if (clientId1 == (u8)((clientId2 + 2) & 3)) {
        return TRUE;
    }
    return FALSE;
}

u8 GetPlayerOpponentClientID(BtlMainModule *mainModule, u8 other) {
    return GetOpponentClientID(mainModule, mainModule->playerClientId, other);
}

u32 GetOpponentClientID(BtlMainModule *mainModule, u8 clientId, u8 other) {
    u8 oppositeSide = clientId;
    oppositeSide &= 1;
    oppositeSide ^= 1;

    if (other != 0) {
        u8 candidate = oppositeSide + ((other & 1) << 1);
        if (DoesClientExist(mainModule, candidate)) {
            oppositeSide = candidate;
        }
    }
    return oppositeSide;
}

void BattleClient_SubItem(BtlMainModule *mainModule, u8 clientId, u16 item) {
    BtlSetup *setup;

    setup = mainModule->setup;
    if (setup->fieldSituation.unk1b == 0 && clientId == mainModule->playerClientId) {
        BagSave_SubItem(setup->bag, item, 1, mainModule->heapId);
    }
}

void BattleClient_AddItem(BtlMainModule *mainModule, u8 clientId, u16 item) {
    BtlSetup *setup;

    setup = mainModule->setup;
    if (setup->fieldSituation.unk1b == 0 && clientId == mainModule->playerClientId) {
        BagSave_AddItem(setup->bag, item, 1, mainModule->heapId);
    }
}

BagSave *GetBagSave(BtlMainModule *mainModule) {
    return mainModule->setup->bag;
}

BagCursor *GetBagCursor(BtlMainModule *mainModule) {
    return mainModule->setup->unk7C;
}

BOOL IsRecordingEnabled(BtlMainModule *mainModule) {
    BtlSetup *setup = mainModule->setup;

    if (setup->unkB0 != 0 && setup->fieldSituation.unk1b == 0) {
        return TRUE;
    }
    return FALSE;
}

u8 IsRecPlayMode(BtlMainModule *mainModule) {
    return mainModule->setup->fieldSituation.unk1b;
}

u32 GetStudioMode(BtlMainModule *mainModule) {
    if (mainModule != NULL) {
        return mainModule->setup->studioMode;
    }
    return 0;
}

u32 func_ov167_0219c99c(BtlMainModule *mainModule) {
    if (mainModule != NULL) {
        return mainModule->setup->unkDD_6;
    }
    return 0;
}

u8 GetStudioMovieId(BtlMainModule *mainModule) {
    if (mainModule != NULL) {
        return mainModule->setup->unkDF;
    }
    return 0;
}

u32 IsStudioSkipDisabled(BtlMainModule *mainModule) {
    if (mainModule != NULL) {
        return mainModule->setup->unkDD_7;
    }
    return 0;
}

void SetRecPlayQuit(BtlMainModule *mainModule) {
    mainModule->setup->unk134 = 1;
}

u32 GetScriptedRulesOutcome(BtlMainModule *mainModule) {
    if (mainModule != NULL) {
        return BtlServerFlow_GetScriptedRulesOutcome(BattleServer_GetFlow(mainModule->server));
    }
    return 0;
}

u32 ReturnZero(BtlMainModule *mainModule, u32 flag) {
    return 0;
}

void BtlMain_StoreSlotNthResult(BtlMainModule *mainModule, u8 pos) {
    if (mainModule->setup->capturedSlot == 6) {
        mainModule->setup->capturedSlot = CountClientPosBefore(mainModule, pos);
    }
}

void SetBattleResult(BtlMainModule *mainModule, u32 result) {
    if (mainModule->result == BTL_RESULT_NOT_SET) {
        mainModule->result = result;
    }
}

u32 GetBattleResult(BtlMainModule *mainModule) {
    return BtlMain_DetermineResult(mainModule);
}

void SetCmdCheckError(BtlMainModule *mainModule) {
    mainModule->setup->unkDD_0 = 1;
}

u32 FixPrizeMoney(BtlMainModule *mainModule) {
    if (!mainModule->prizeMoneyFixed) {
        if (mainModule->moneyDoubled) {
            mainModule->prizeMoney *= 2;
        }
        mainModule->prizeMoney = PassPower_ApplyPrizeMoney(mainModule->prizeMoney);
        mainModule->prizeMoneyFixed = 1;
    }
    return mainModule->prizeMoney;
}

void AddBonusMoney(BtlMainModule *mainModule, u32 money) {
    if (!mainModule->prizeMoneyFixed) {
        mainModule->money += money;
        if (mainModule->money > 99999) {
            mainModule->money = 99999;
        }
    }
}

u32 GetBonusMoney(BtlMainModule *mainModule) {
    u32 money = mainModule->money;

    if (mainModule->moneyDoubled) {
        money *= 2;
        if (money > 99999) {
            money = 99999;
        }
    }
    return money;
}

void SetMoneyDoubled(BtlMainModule *mainModule) {
    mainModule->moneyDoubled = 1;
}

u32 CalcLossMoney(BtlMainModule *mainModule) {
    u32 loss = 0;
    u32 cash;

    if (!mainModule->lossMoneyCalculated) {
        if (mainModule->setup->battleType <= BTL_TYPE_TRAINER) {
            cash = getCash(getTrainerCardDataBlkAddress(mainModule->setup->gameData));
            loss = BtlCalc_GetLostMoney(mainModule->setup->unk98,
                                        GetClientParty(&mainModule->pokeCons[1], mainModule->playerClientId));
            if (loss > cash) {
                loss = cash;
            }
            mainModule->lossMoney = loss;
        }
        mainModule->lossMoneyCalculated = 1;
    }
    return loss;
}

void SkipExitBGMFadeOut(BtlMainModule *mainModule) {
    mainModule->skipExitBgmFadeOut = 1;
}

void ChangeFriendshipWhenFainted(BtlMainModule *mainModule, BattleMon *mon, BOOL reason) {
    if (mainModule->setup->battleType <= BTL_TYPE_TRAINER && mainModule->setup->fieldSituation.unk1b == 0) {
        ChangeFriendship(mainModule, mon, reason ? 5 : 4);
    }
}

void ChangeFriendship(BtlMainModule *mainModule, BattleMon *mon, u32 reason) {
    u8 monId;
    const BattleMon *serverMon;
    const BattleMon *clientMon;
    PartyPkm *src1;
    PartyPkm *src2;
    const BtlFieldSituation *field;

    monId = GetMonID(mon);
    serverMon = GetPokeParamConst(&mainModule->pokeCons[1], monId);
    clientMon = GetPokeParamConst(&mainModule->pokeCons[0], monId);
    src1 = GetSrcData(serverMon);
    src2 = GetSrcData(clientMon);
    field = GetFieldEffectData(mainModule);
    FriendshipManagerCalc(src1, reason, field->env.zoneId, HEAPID_TAIL(mainModule->heapId));
    FriendshipManagerCalc(src2, reason, field->env.zoneId, HEAPID_TAIL(mainModule->heapId));
}

void SyncServerMonMoves(BtlMainModule *mainModule, u8 monId) {
    BattleMon *mon = GetPokeParam(&mainModule->pokeCons[1], monId);

    BattleMon_CopyMoves(GetPokeParam(&mainModule->pokeCons[0], monId), mon);
}

BattleMon *GetIllusionDisguise(BtlMainModule *mainModule, BtlPokeCon *pokeCon, BattleMon *mon) {
    u8 clientId;
    PartyPkm *disguise;
    BattleParty *party;
    u32 count;
    u32 i;
    BattleMon *member;

    if (IsIllusionEnabled(mon)) {
        clientId = GetClientIDFromMonID(GetMonID(mon));
        disguise = BattleMon_GetViewSrcData(mon);
        party = GetClientParty(pokeCon, clientId);
        count = GetNumMonsInParty(party);
        for (i = 0; i < count; i++) {
            member = GetBattleMonFromParty(party, i);
            if (GetSrcData(member) == disguise) {
                return member;
            }
        }
    }
    return mon;
}

void PokeCon_Init(BtlPokeCon *pokeCon, BtlMainModule *mainModule, u32 forServer) {
    u32 i;

    pokeCon->mainModule = mainModule;
    pokeCon->forServer = forServer;
    for (i = 0; i < 4; i++) {
        BattleParty_Clear(&pokeCon->parties[i]);
    }
    for (i = 0; i < 24; i++) {
        pokeCon->mons[i] = NULL;
    }
    for (i = 0; i < 4; i++) {
        pokeCon->srcParties[i] = NULL;
    }
}

void PokeCon_Clear(BtlPokeCon *pokeCon) {
    u32 i;

    for (i = 0; i < 24; i++) {
        if (pokeCon->mons[i] != NULL) {
            BattleMon_Delete(pokeCon->mons[i]);
            pokeCon->mons[i] = NULL;
        }
    }
    for (i = 0; i < 4; i++) {
        BattleParty_Clear(&pokeCon->parties[i]);
    }
}

void PokeCon_CreateClientParty(BtlPokeCon *pokeCon, BtlMainModule *mainModule, u8 clientId) {
    PokeParty *srcParty;
    BattleParty *battleParty;
    u32 count;
    u8 monId;
    s32 i;
    s32 eligible;
    BattleMon *mon;
    PokeParty *party;
    u8 partyCount;
    u8 k;
    u16 species;
    PartyPkm *pkm;
    u8 firstMonId;

    srcParty = GetClientSrcParty(mainModule, clientId, pokeCon->forServer);
    battleParty = &pokeCon->parties[clientId];
    count = PokeParty_GetPkmCount(srcParty);
    firstMonId = sClientFirstMonIds[clientId];
    pokeCon->srcParties[clientId] = srcParty;
    for (monId = firstMonId, i = 0; i < count; i++) {
        pkm = PokeParty_GetPkm(srcParty, i);
        pokeCon->mons[monId] = BattleMon_Create(pkm, monId, HEAPID_BATTLE);
        AddBattleMonToParty(battleParty, pokeCon->mons[monId]);
        monId++;
    }
    eligible = GetPartyPkmnEligibleForBattle(srcParty);
    if (eligible > 0) {
        for (monId = firstMonId, i = 0; i < eligible; monId++) {
            if (GetBattleMonStat(pokeCon->mons[monId], BATTLEMON_EFFECTIVE_ABILITY) == ABILITY_ILLUSION) {
                SetIllusionDisguise(pokeCon->mons[monId], PokeParty_GetPkm(srcParty, eligible));
            }
            i++;
        }
    }
    if (BtlSetup_GetBattleType(mainModule) == BTL_TYPE_WILD && clientId == 1) {
        for (i = 0; i < count; firstMonId++) {
            mon = pokeCon->mons[firstMonId];
            if (GetBattleMonSpecies(mon) == SPECIES_ZOROARK &&
                GetBattleMonStat(mon, BATTLEMON_EFFECTIVE_ABILITY) == ABILITY_ILLUSION) {
                party = GetClientSrcParty(mainModule, 0, pokeCon->forServer);
                species = 0;
                partyCount = PokeParty_GetPkmCount(party);
                for (k = 0; k < partyCount; k++) {
                    u16 partySpecies = PokeParty_GetParam(PokeParty_GetPkm(party, k), PKM_PARAM_SPECIES, NULL);

                    if (partySpecies == SPECIES_RAIKOU) {
                        species = SPECIES_ENTEI;
                        break;
                    }
                    if (partySpecies == SPECIES_ENTEI) {
                        species = SPECIES_SUICUNE;
                        break;
                    }
                    if (partySpecies == SPECIES_SUICUNE) {
                        species = SPECIES_RAIKOU;
                        break;
                    }
                }
                if (species != 0) {
                    if (mainModule->wildIllusionPkm == NULL) {
                        mainModule->wildIllusionPkm = PokeParty_NewTempPkm(
                            species, GetBattleMonStat(mon, BATTLEMON_LEVEL), 0, mainModule->heapId);
                    }
                    SetIllusionDisguise(mon, mainModule->wildIllusionPkm);
                }
            }
            i++;
        }
    }
    BattleParty_MoveFaintedToEnd(battleParty);
}

s32 GetPartyPkmnEligibleForBattle(PokeParty *party) {
    s32 index;
    PartyPkm *pkm;

    for (index = PokeParty_GetPkmCount(party) - 1; index >= 0; index--) {
        pkm = PokeParty_GetPkm(party, index);
        if (PokeParty_GetParam(pkm, (PkmField)PKM_PARAM_IS_EGG, NULL) == 0 &&
            PokeParty_GetParam(pkm, (PkmField)PKM_PARAM_HP, NULL) != 0) {
            return index;
        }
    }
    return -1;
}

void PokeCon_FreeMons(BtlPokeCon *pokeCon) {
    u32 i;

    for (i = 0; i < 24; i++) {
        if (pokeCon->mons[i] != NULL) {
            BattleMon_Delete(pokeCon->mons[i]);
            pokeCon->mons[i] = NULL;
        }
    }
}

void PokeCon_StoreClientSrcParty(BtlPokeCon *pokeCon, BtlMainModule *mainModule, u8 clientId, BOOL keepBaseForm) {
    PokeParty *srcParty;
    BattleParty *party;
    u32 count;
    u8 disguiseSlots[6];
    u32 i;
    u32 j;
    BattleMon *mon;
    PartyPkm *disguise;
    PartyPkm *pkm;

    srcParty = PokeCon_GetSrcParty(pokeCon, clientId);
    party = GetPartyData(pokeCon, clientId);
    count = PokeParty_GetPkmCount(srcParty);
    PokeParty_Init(mainModule->tempParty);
    for (i = 0; i < 6; i++) {
        disguiseSlots[i] = 6;
    }
    for (i = 0; i < count; i++) {
        mon = BattleParty_GetMon(party, i);
        BattleMon_WriteToSrc(mon, keepBaseForm);
        PokeParty_AddPkm(mainModule->tempParty, GetSrcData(mon));
        if (IsIllusionEnabled(mon)) {
            disguise = BattleMon_GetViewSrcData(mon);
            for (j = 0; j < count; j++) {
                if (disguise == GetSrcData(BattleParty_GetMon(party, j))) {
                    disguiseSlots[i] = j;
                    break;
                }
            }
        }
    }
    PokeParty_Copy(mainModule->tempParty, srcParty);
    for (i = 0; i < count; i++) {
        pkm = PokeParty_GetPkm(srcParty, i);
        mon = BattleParty_GetMon(party, i);
        BattleMon_SetSrcData(mon, pkm);
        if (disguiseSlots[i] != 6) {
            SetIllusionDisguise(mon, PokeParty_GetPkm(srcParty, disguiseSlots[i]));
        }
    }
}

void PokeCon_StoreClientSrcPartyInOrder(BtlPokeCon *pokeCon, BtlMainModule *mainModule, u8 clientId) {
    PokeParty *srcParty;
    BattleParty *party;
    u32 count;
    u32 firstMonId;
    u32 i;
    u32 j;
    BattleMon *mon;

    srcParty = PokeCon_GetSrcParty(pokeCon, clientId);
    party = GetPartyData(pokeCon, clientId);
    count = PokeParty_GetPkmCount(srcParty);
    PokeParty_Init(mainModule->tempParty);
    firstMonId = sClientFirstMonIds[clientId];
    for (i = 0; i < count; i++) {
        for (j = 0; j < count; j++) {
            mon = BattleParty_GetMon(party, j);
            if (firstMonId + i == GetMonID(mon)) {
                BattleMon_WriteToSrc(mon, TRUE);
                PokeParty_AddPkm(mainModule->tempParty, GetSrcData(mon));
                if (BattleMon_HasAppeared(mon)) {
                    mainModule->setup->appeared[i] = 1;
                }
                break;
            }
        }
    }
    PokeParty_Copy(mainModule->tempParty, srcParty);
}

PokeParty *PokeCon_GetSrcParty(BtlPokeCon *pokeCon, u8 clientId) {
    return pokeCon->srcParties[clientId];
}

s32 PokeCon_FindClientMonIndex(BtlPokeCon *pokeCon, u8 clientId, u8 monId) {
    BattleParty *parties = pokeCon->parties;
    u32 count = GetNumMonsInParty(&parties[clientId]);
    u32 i;

    for (i = 0; i < count; i++) {
        if (monId == GetMonID(GetBattleMonFromParty(&parties[clientId], i))) {
            return i;
        }
    }
    return -1;
}

BattleMon *PokeCon_GetPosMon(BtlPokeCon *pokeCon, u8 pos) {
    return PokeCon_GetPosMonInner(pokeCon, pos);
}

BattleMon *PokeCon_GetPosMonInner(const BtlPokeCon *pokeCon, u8 pos) {
    u8 i = pos;
    u8 clientId;
    u8 index;
    const BattleParty *parties;
    BtlMainModule *mainModule = pokeCon->mainModule;

    clientId = BattlePosToClientID(mainModule, pos);
    index = 0;
    while (i-- != 0) {
        if (clientId == mainModule->posClientIds[i]) {
            index++;
        }
    }
    parties = pokeCon->parties;
    if (index < GetNumMonsInParty(&parties[clientId])) {
        return GetBattleMonFromParty(&parties[clientId], index);
    }
    return NULL;
}

BattleMon *PokeCon_GetClientPartyMon(BtlPokeCon *pokeCon, u8 clientId, u8 index) {
    return BattleParty_GetMon(&pokeCon->parties[clientId], index);
}

BattleMon *GetClientMonData(BtlPokeCon *pokeCon, u8 clientId, u8 monId) {
    return GetBattleMonFromParty(&pokeCon->parties[clientId], monId);
}

BattleMon *GetPokeParam(BtlPokeCon *pokeCon, u8 monId) {
    return pokeCon->mons[monId];
}

const BattleMon *GetPokeParamConst(const BtlPokeCon *pokeCon, u8 monId) {
    return pokeCon->mons[monId];
}

BOOL GetViewPosChatter(BtlMainModule *mainModule, u32 index, PerapVoice **out) {
    u8 clientId = GetClientIDFromPos(mainModule, ViewPosToBattlePos(mainModule, index));

    if (mainModule->chatters[clientId] != NULL) {
        *out = mainModule->chatters[clientId];
        return TRUE;
    }
    return FALSE;
}

u8 GetClientChatterEffect(BtlMainModule *mainModule, u8 clientId) {
    if (!IsRecPlayMode(mainModule)) {
        if (mainModule->chatters[clientId] != NULL && doesChatotExist(mainModule->chatters[clientId])) {
            return PerapVoice_GetChatterEffect(mainModule->chatters[clientId]);
        }
        return 0;
    }
    return mainModule->setup->unk44[clientId];
}

u8 GetClientBattlerCount(BtlMainModule *mainModule, u8 clientId) {
    return CalcClientBattlerCount(mainModule, clientId);
}

u8 GetClientBenchStart(BtlMainModule *mainModule, u8 clientId) {
    u8 count = CalcClientBattlerCount(mainModule, clientId);

    if (BtlSetup_GetBattleStyle(mainModule) == BTL_STYLE_ROTATION) {
        count += 2;
    }
    return count;
}

const AdjacentOpponentData *GetAdjacentOpponentData(u8 pos) {
    static const AdjacentOpponentData sAdjacentOpponentData[BTL_POS_MAX] = {
        { 2, 2, { 5, 3, 6 }, { 0, 2, 6 } }, { 2, 2, { 4, 2, 6 }, { 1, 3, 6 } }, { 3, 3, { 1, 3, 5 }, { 0, 2, 4 } },
        { 3, 3, { 0, 2, 4 }, { 1, 3, 5 } }, { 2, 2, { 1, 3, 6 }, { 4, 2, 6 } }, { 2, 2, { 0, 2, 6 }, { 5, 3, 6 } },
    };

    return &sAdjacentOpponentData[pos];
}

BOOL IsTripleCenterPos(u8 pos) {
    switch (pos) {
    case 2:
    case 3:
        return TRUE;
    default:
        return FALSE;
    }
}

BOOL GetNonAdjacentOpponentPos(u8 pos, u8 *out) {
    if (!IsTripleCenterPos(pos)) {
        *out = GetOpposingPosForStyle(BTL_STYLE_TRIPLE, pos, pos >> 1);
        return TRUE;
    }
    return FALSE;
}

BOOL IsAllyMonID(u8 monId1, u8 monId2) {
    return GetSideFromMonID(monId1) == GetSideFromMonID(monId2);
}

u8 GetSideFromMonID(u8 monId) {
    BtlSide side = monId < 12 ? BTL_SIDE_1ST : BTL_SIDE_2ND;
    return side;
}

// Public function name from swan.
u8 GetSideFromOpposingMonID(u8 monId) {
    return GetOpposingSide(GetSideFromMonID(monId));
}

u8 GetOpposingSide(u8 side) {
    return side == 0;
}

BOOL IsAdjacentOpponent(u8 pos1, u8 pos2) {
    const AdjacentOpponentData *data;
    u32 i;

    data = GetAdjacentOpponentData(pos1);
    for (i = 0; i < data->count1; i++) {
        if (pos2 == data->list1[i]) {
            return TRUE;
        }
    }
    for (i = 0; i < data->count2; i++) {
        if (pos2 == data->list2[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

u8 GetRotationInSlot(u32 pos) {
    switch (pos) {
    case 2:
        return 1;
    case 3:
        return 2;
    default:
        return 0;
    }
}

u32 GetRotationOutSlot(u32 pos) {
    switch (pos) {
    case 2:
        return 2;
    case 3:
        return 1;
    default:
        return 0;
    }
}

u8 GetPosSide(u8 pos) {
    return pos & 1;
}

BattleParty *GetPartyData(BtlPokeCon *pokeCon, u32 clientId) {
    return &pokeCon->parties[clientId];
}

BattleParty *GetClientParty(BtlPokeCon *pokeCon, u32 clientId) {
    return &pokeCon->parties[clientId];
}

u32 GetServerTurnCount(BtlMainModule *mainModule) {
    if (mainModule->server != NULL) {
        return GetTurnCounter(BattleServer_GetFlow(mainModule->server));
    }
    return 0;
}

u8 PokeCon_GetClientAliveCount(BtlPokeCon *pokeCon, u8 clientId) {
    return GetAlivePartyCount(GetClientParty(pokeCon, clientId));
}

void AddClientShooterEnergy(BtlMainModule *mainModule, u8 clientId, u8 value) {
    if (mainModule->isServer != 0 && DoesClientExist(mainModule, clientId) && mainModule->clients[clientId] != NULL) {
        BattleClient_AddShooterEnergy(mainModule->clients[clientId], value);
    }
}

void BattleParty_Clear(BattleParty *party) {
    s32 i;

    party->count = 0;
    for (i = 0; i < 6; i++) {
        party->mons[i] = NULL;
    }
}

void AddBattleMonToParty(BattleParty *party, BattleMon *mon) {
    party->mons[party->count++] = mon;
}

void BattleParty_MoveFaintedToEnd(BattleParty *party) {
    u32 i;
    u32 processed;

    processed = 0;
    i = 0;
    for (; processed < party->count; processed++) {
        if (CanPokemonBattle(party->mons[i])) {
            i++;
        } else {
            BattleParty_MoveToEnd(party, i);
        }
    }
}

u8 GetNumMonsInParty(const BattleParty *party) {
    return party->count;
}

u8 GetAlivePartyCount(BattleParty *party) {
    s32 i;
    s32 count;

    i = 0;
    count = 0;
    for (; i < party->count; i++) {
        if (CanPokemonBattle(party->mons[i])) {
            count++;
        }
    }
    return count;
}

u8 BattleParty_CountAliveFrom(BattleParty *party, s32 index) {
    s32 i;
    u32 count;

    count = 0;
    for (i = index; i < party->count; i++) {
        if (CanPokemonBattle(party->mons[i])) {
            count++;
        }
    }
    return count;
}

BattleMon *BattleParty_GetMon(BattleParty *party, u8 index) {
    if (index < party->count) {
        return party->mons[index];
    }
    return NULL;
}

BattleMon *GetBattleMonFromParty(const BattleParty *party, u8 index) {
    if (index < party->count) {
        return party->mons[index];
    }
    return 0;
}

void BattleParty_Swap(BattleParty *party, u32 first, u32 second) {
    BattleMon *mon;

    mon = party->mons[first];
    party->mons[first] = party->mons[second];
    party->mons[second] = mon;
}

void BattleParty_MoveToEnd(BattleParty *party, u8 index) {
    BattleMon *mon;

    mon = party->mons[index];
    for (; index < party->count - 1; index++) {
        party->mons[index] = party->mons[index + 1];
    }
    party->mons[index] = mon;
}

void BattleParty_Rotate(BattleParty *party, u32 pos, BattleMon **oldOut, BattleMon **newOut) {
    BattleMon *first;
    BattleMon *next;
    BattleMon *other;
    u32 index1;
    u32 index2;

    if (pos != 0 && pos != 1) {
        first = party->mons[0];
        index1 = GetRotationInSlot(pos);
        index2 = GetRotationOutSlot(pos);
        next = party->mons[index1];
        party->mons[0] = next;
        other = party->mons[index2];
        party->mons[index1] = other;
        party->mons[index2] = first;
        if (oldOut != NULL) {
            *oldOut = first;
        }
        if (newOut != NULL) {
            *newOut = party->mons[0];
        }
    }
}

s32 FindPartyMon(const BattleParty *party, BattleMon *mon) {
    s32 i;

    for (i = 0; i < party->count; i++) {
        if (party->mons[i] == mon) {
            return i;
        }
    }
    return -1;
}

s32 BattleParty_FindMonID(const BattleParty *party, u32 monId) {
    s32 i;

    for (i = 0; i < party->count; i++) {
        if (GetMonID(party->mons[i]) == monId) {
            return i;
        }
    }
    return -1;
}

BattleMon *BattleParty_GetFirstAlive(const BattleParty *party) {
    s32 i;

    for (i = 0; i < party->count; i++) {
        if (CanPokemonBattle(party->mons[i])) {
            return party->mons[i];
        }
    }
    return NULL;
}

void UpdatePartyIllusionDisguises(BtlMainModule *mainModule, BattleParty *party, u8 clientId) {
    s32 last;
    s32 i;
    BattleMon *mon;

    for (last = party->count - 1; last > 0; last--) {
        if (CanPokemonBattle(party->mons[last])) {
            break;
        }
    }
    if (BtlSetup_GetBattleStyle(mainModule) != BTL_STYLE_ROTATION) {
        i = GetClientPosCount(mainModule, clientId);
    } else {
        i = 3;
    }
    for (; i < party->count; i++) {
        mon = party->mons[i];
        if (GetBattleMonStat(mon, BATTLEMON_EFFECTIVE_ABILITY) == ABILITY_ILLUSION) {
            if (i < last) {
                SetIllusionDisguise(mon, GetSrcData(party->mons[last]));
            } else if (BattleMon_GetViewSrcData(mon) != mainModule->wildIllusionPkm) {
                BattleMon_ResetIllusion(mon);
            }
        }
    }
    for (i = 0; i < party->count; i++) {
        IsIllusionEnabled(party->mons[i]);
    }
}

void BtlMain_InitTrainers(BtlMainModule *mainModule) {
    u32 i;

    for (i = 0; i < 4; i++) {
        mainModule->trainers[i].playerInfo = NULL;
        mainModule->trainers[i].trainerId = 0;
        mainModule->trainers[i].aiFlags = 0;
    }
    for (i = 0; i < 4; i++) {
        mainModule->chatters[i] = NULL;
    }
}

void BtlMain_FreeTrainers(BtlMainModule *mainModule) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (mainModule->trainers[i].playerInfo != NULL) {
            GFL_HeapFree(mainModule->trainers[i].playerInfo);
            mainModule->trainers[i].playerInfo = NULL;
        }
        if (mainModule->trainers[i].name != NULL) {
            GFL_HeapFree(mainModule->trainers[i].name);
            mainModule->trainers[i].name = NULL;
        }
    }
    for (i = 0; i < 4; i++) {
        if (mainModule->chatters[i] != NULL) {
            GFL_HeapFree(mainModule->chatters[i]);
            mainModule->chatters[i] = NULL;
        }
    }
}

void TrainerData_SetFromPlayer(BtlTrainerData *trainer, HeapID heapId, PlayerInfo *src) {
    u16 trainerClass;
    u8 version;

    trainer->playerInfo = PlayerInfo_Create(heapId);
    PlayerInfo_Copy(src, trainer->playerInfo);
    trainerClass = 0;
    trainer->trainerId = 0;
    version = PlayerInfo_GetVersion(src);
    if (version == VERSION_WHITE2 || version == VERSION_BLACK2) {
        if (getTrainerGender(trainer->playerInfo)) {
            trainerClass = 1;
        }
        trainer->trainerClass = trainerClass;
    } else {
        trainer->trainerClass = getTrainerGender(trainer->playerInfo) == 0 ? 0xb6 : 0xb7;
    }
    trainer->name = copyTrainerNameToNewStrbuf(trainer->playerInfo->name, HEAPID_BATTLE);
    PMSData_Clear(&trainer->winWords);
    PMSData_Clear(&trainer->loseWords);
}

void TrainerData_SetFromSetup(BtlTrainerData *trainer, const BtlSetupTrainer *src) {
    u32 i;

    trainer->playerInfo = NULL;
    if (src != NULL) {
        trainer->trainerId = src->trainerId;
        trainer->trainerClass = src->trainerClass;
        trainer->name = GFL_StrBufClone(src->name, HEAPID_BATTLE);
        trainer->aiFlags = src->aiFlags;
        MI_CpuCopy8(src->items, trainer->items, sizeof(trainer->items));
        PMSData_Copy(&trainer->winWords, &src->winWords);
        PMSData_Copy(&trainer->loseWords, &src->loseWords);
    } else {
        trainer->trainerId = 0;
        trainer->trainerClass = 0;
        trainer->name = NULL;
        for (i = 0; i < 4; i++) {
            trainer->items[i] = 0;
        }
        PMSData_Clear(&trainer->winWords);
        PMSData_Clear(&trainer->loseWords);
    }
}

void TrainerData_SetFromComm(BtlTrainerData *trainer, const BtlCommTrainerData *src) {
    u32 i;

    trainer->playerInfo = NULL;
    if (src != NULL) {
        trainer->trainerId = src->trainerId;
        trainer->trainerClass = src->trainerClass;
        trainer->name = GFL_StrBufCreate(0x20, HEAPID_BATTLE);
        GFL_StrBufCopyString(trainer->name, src->name, src->nameLength + 1);
        trainer->aiFlags = src->aiFlags;
        MI_CpuCopy8(src->items, trainer->items, sizeof(trainer->items));
        PMSData_Copy(&trainer->winWords, &src->winWords);
        PMSData_Copy(&trainer->loseWords, &src->loseWords);
    } else {
        trainer->trainerId = 0;
        trainer->trainerClass = 0;
        trainer->name = NULL;
        for (i = 0; i < 4; i++) {
            trainer->items[i] = 0;
        }
        PMSData_Clear(&trainer->winWords);
        PMSData_Clear(&trainer->loseWords);
    }
}

BOOL IsClientNPCTrainer(BtlMainModule *mainModule, u8 clientId) {
    if (mainModule->trainers[clientId].playerInfo == NULL) {
        return TRUE;
    }
    return FALSE;
}

u16 GetClientTrainerItem(BtlMainModule *mainModule, u8 clientId, u8 index) {
    if (IsClientNPCTrainer(mainModule, clientId)) {
        BtlTrainerData *trainer = &mainModule->trainers[clientId];

        if (index < 4) {
            return trainer->items[index];
        }
    }
    return 0;
}

StrBuf *GetClientTrainerName(BtlMainModule *mainModule, u8 clientId, u32 *trainerClass) {
    *trainerClass = mainModule->trainers[clientId].trainerClass;
    return mainModule->trainers[clientId].name;
}

u32 GetClientAIFlags(BtlMainModule *mainModule, u8 clientId) {
    if (BtlSetup_GetBattleType(mainModule) == BTL_TYPE_WILD) {
        if (BtlSetup_IsBattleType(mainModule, 8)) {
            return 0x800;
        }
        if (BtlSetup_GetBattleStyle(mainModule) == BTL_STYLE_DOUBLE) {
            return 0x80;
        }
    } else if (IsClientNPCTrainer(mainModule, clientId)) {
        return mainModule->trainers[clientId].aiFlags;
    }
    return 0;
}

u16 GetClientTrainerID(BtlMainModule *mainModule, u8 clientId) {
    if (IsClientNPCTrainer(mainModule, clientId)) {
        return mainModule->trainers[clientId].trainerId;
    }
    return 0;
}

u32 GetClientTrainerClass(BtlMainModule *mainModule, u8 clientId) {
    return mainModule->trainers[clientId].trainerClass;
}

PMSData *GetClientTrainerMsg(BtlMainModule *mainModule, u8 clientId, u32 which) {
    if (BtlSetup_GetBattleType(mainModule) == BTL_TYPE_FACILITY && IsClientNPCTrainer(mainModule, clientId)) {
        if (which == 1) {
            return &mainModule->trainers[clientId].loseWords;
        }
        return &mainModule->trainers[clientId].winWords;
    }
    return NULL;
}

PlayerInfo *GetClientPlayerInfo(BtlMainModule *mainModule, u8 clientId) {
    if (IsClientNPCTrainer(mainModule, clientId)) {
        return NULL;
    }
    return mainModule->trainers[clientId].playerInfo;
}

PlayerInfo *GetCommSupporterInfo(BtlMainModule *mainModule) {
    return CommPlayerSupport_GetSupporter(mainModule->setup->unk88);
}

BtlField *GetBtlField(BtlMainModule *mainModule) {
    return mainModule->field;
}

void BtlMain_CreateSrcParties(BtlMainModule *mainModule) {
    u32 i;

    for (i = 0; i < 4; i++) {
        mainModule->clientSrcParties[i] = PokeParty_Create(HEAPID_BATTLE);
        mainModule->serverSrcParties[i] = PokeParty_Create(HEAPID_BATTLE);
    }
    mainModule->tempParty = PokeParty_Create(HEAPID_BATTLE);
}

void BtlMain_FreeSrcParties(BtlMainModule *mainModule) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (mainModule->clientSrcParties[i] != NULL) {
            GFL_HeapFree(mainModule->clientSrcParties[i]);
            mainModule->clientSrcParties[i] = NULL;
        }
        if (mainModule->serverSrcParties[i] != NULL) {
            GFL_HeapFree(mainModule->serverSrcParties[i]);
            mainModule->serverSrcParties[i] = NULL;
        }
    }
    if (mainModule->tempParty != NULL) {
        GFL_HeapFree(mainModule->tempParty);
        mainModule->tempParty = NULL;
    }
}

void SetClientSrcParty(BtlMainModule *mainModule, u8 clientId, const PokeParty *party) {
    if (mainModule->clientSrcParties[clientId] != NULL) {
        PokeParty_Init(mainModule->clientSrcParties[clientId]);
        PokeParty_Init(mainModule->serverSrcParties[clientId]);
        PokeParty_Copy(party, mainModule->clientSrcParties[clientId]);
        PokeParty_Copy(party, mainModule->serverSrcParties[clientId]);
        if (PokeParty_GetPkmCount(mainModule->serverSrcParties[clientId]) != 0) {
            PokeParty_GetPkm(mainModule->serverSrcParties[clientId], 0);
        }
    }
}

PokeParty *GetClientSrcParty(BtlMainModule *mainModule, u8 clientId, u32 forServer) {
    if (forServer) {
        if (PokeParty_GetPkmCount(mainModule->serverSrcParties[clientId]) != 0) {
            PokeParty_GetPkm(mainModule->serverSrcParties[clientId], 0);
        }
        return mainModule->serverSrcParties[clientId];
    }
    return mainModule->clientSrcParties[clientId];
}

u32 BtlSetup_IsBattleType(BtlMainModule *mainModule, u32 flag) {
    return BtlSetup_CheckFlag(mainModule->setup, flag);
}

void IncRecord(BtlMainModule *mainModule, u32 record) {
    BtlSetup *setup = mainModule->setup;

    if (setup->fieldSituation.unk1b == 0) {
        RecordAddOne(setup->records, record);
    }
}

void AddRecord(BtlMainModule *mainModule, u32 record, u32 value) {
    BtlSetup *setup = mainModule->setup;

    if (setup->fieldSituation.unk1b == 0) {
        RecordAdd(setup->records, record, value);
    }
}

u8 *GetShooterSettings(BtlMainModule *mainModule) {
    return mainModule->setup->unk90;
}

BOOL IsShooterEnabled(BtlMainModule *mainModule) {
    BOOL result = FALSE;

    switch (BtlMain_IsCommCompetitor(mainModule->setup)) {
    case 1:
        if (mainModule->setup->unk97 != 0) {
            result = TRUE;
        }
        break;
    }
    return result;
}

BOOL IsBagEnabled(BtlMainModule *mainModule) {
    if (!mainModule->setup->unkDD_5) {
        if (BtlMain_IsCommCompetitor(mainModule->setup) == 1) {
            return IsShooterEnabled(mainModule);
        }
        switch (BtlSetup_GetBattleType(mainModule)) {
        case BTL_TYPE_WILD:
        case BTL_TYPE_TRAINER:
            return TRUE;
        default:
            return FALSE;
        }
    }
    return FALSE;
}

void NotifyMonLevelUp(BtlMainModule *mainModule, BattleMon *mon) {
    ChangeFriendship(mainModule, mon, 0);
    BattleServer_SendNicknameBeacon(mainModule->server, mon);
}

void ChangeFriendshipByItem(BtlMainModule *mainModule, BattleMon *mon, u16 item) {
    u8 monId;
    const BattleMon *serverMon;
    const BattleMon *clientMon;
    PartyPkm *src1;
    PartyPkm *src2;
    const BtlFieldSituation *field;

    monId = GetMonID(mon);
    serverMon = GetPokeParamConst(&mainModule->pokeCons[1], monId);
    clientMon = GetPokeParamConst(&mainModule->pokeCons[0], monId);
    src1 = GetSrcData(serverMon);
    src2 = GetSrcData(clientMon);
    field = GetFieldEffectData(mainModule);
    FriendshipManagerCalcItem(src1, item, field->env.zoneId, HEAPID_TAIL(mainModule->heapId));
    FriendshipManagerCalcItem(src2, item, field->env.zoneId, HEAPID_TAIL(mainModule->heapId));
}

void NotifyMonCaught(BtlMainModule *mainModule, BattleMon *mon) {
    if (mainModule->server != NULL) {
        BattleServer_NotifyCapture(mainModule->server, mon);
    }
}

void BtlMain_StorePartiesToSetup(BtlMainModule *mainModule) {
    PokeParty *party;
    u32 count;
    u32 i;
    PartyPkm *original;
    PartyPkm *pkm;
    u16 item;
    u8 clientId;
    u32 j;
    u32 index;
    u32 k;
    BattleParty *battleParty;
    u32 monCount;
    u32 hp;
    u32 maxHP;
    u8 firstMonId;
    u8 slot;
    BattleMon *mon;

    if (mainModule->setup->fieldSituation.unk1b != 0 || GetStudioMode(mainModule) != BTL_STUDIO_MODE_NONE) {
        return;
    }
    if (mainModule->setup->battleType <= BTL_TYPE_TRAINER) {
        PokeCon_StoreClientSrcPartyInOrder(&mainModule->pokeCons[1], mainModule, mainModule->playerClientId);
        party = PokeCon_GetSrcParty(&mainModule->pokeCons[1], mainModule->playerClientId);
        if (mainModule->setup->battleType == BTL_TYPE_TRAINER &&
            BtlCalc_IsBossTrainerClass(GetClientTrainerClass(mainModule, 1))) {
            FriendshipManagerCalcParty(party, GetFieldEffectData(mainModule)->env.zoneId,
                                       HEAPID_TAIL(mainModule->heapId));
        }
        if (mainModule->setup->battleType == BTL_TYPE_TRAINER) {
            count = PokeParty_GetPkmCount(party);
            for (i = 0; i < count; i++) {
                original = PokeParty_GetPkm(mainModule->setup->party[0], i);
                pkm = PokeParty_GetPkm(party, i);
                item = PokeParty_GetParam(original, PKM_PARAM_ITEM, NULL);
                if (ItemGetParam(item, 0x10) == 0) {
                    PokeParty_SetParam(pkm, PKM_PARAM_ITEM, item);
                } else if (item != PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL)) {
                    PokeParty_SetParam(pkm, PKM_PARAM_ITEM, 0);
                }
            }
        }
        PokeParty_Copy(party, mainModule->setup->party[0]);
        for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
            PokeParty_GetPkm(party, i);
        }
    }
    if (mainModule->setup->battleType == BTL_TYPE_WILD) {
        clientId = GetOpponentClientID(mainModule, mainModule->playerClientId, 0);
        PokeCon_StoreClientSrcParty(&mainModule->pokeCons[1], mainModule, clientId, TRUE);
        PokeParty_Copy(PokeCon_GetSrcParty(&mainModule->pokeCons[1], clientId), mainModule->setup->party[1]);
    }
    if (mainModule->setup->battleType == BTL_TYPE_CAPTURE_DEMO) {
        return;
    }
    for (j = 0; j < 4; j++) {
        index = ClientIDToSetupIndex(mainModule->playerClientId, j);
        for (k = 0; k < 6; k++) {
            mainModule->setup->unkE7[index][k] = 0;
        }
        if (DoesClientExist(mainModule, index)) {
            battleParty = GetPartyData(&mainModule->pokeCons[0], index);
            monCount = GetNumMonsInParty(battleParty);
            maxHP = 0;
            hp = 0;
            firstMonId = sClientFirstMonIds[index];
            for (i = 0; i < monCount; i++) {
                mon = BattleParty_GetMon(battleParty, i);
                slot = GetMonID(mon) - firstMonId;
                hp += GetBattleMonStat(mon, BATTLEMON_HP);
                maxHP += GetBattleMonStat(mon, BATTLEMON_MAX_HP);
                if (IsFainted(mon)) {
                    mainModule->setup->unkE7[index][slot] = 2;
                } else if (GetBattleMonStatus(mon) != 0) {
                    mainModule->setup->unkE7[index][slot] = 1;
                }
            }
            mainModule->setup->unk100[index] = hp * 100 / maxHP;
        }
    }
}

BOOL BtlMain_IsCommTrainerBattle(BtlMainModule *mainModule) {
    switch (BtlSetup_GetBattleType(mainModule)) {
    case BTL_TYPE_FACILITY:
    case BTL_TYPE_COMM:
        return TRUE;
    default:
        return FALSE;
    }
}

u32 BtlMain_DetermineResult(BtlMainModule *mainModule) {
    u32 result;

    if (BtlSetup_GetBattleType(mainModule) == BTL_TYPE_CAPTURE_DEMO) {
        result = BTL_RESULT_VICTORY;
    } else if (mainModule->commError) {
        result = BTL_RESULT_6;
    } else if (mainModule->setup->capturedSlot != 6) {
        result = BTL_RESULT_CAPTURE;
    } else if (BtlClientIDList_GetCount(&mainModule->clientIdList)) {
        result = BtlClientIDList_GetEscapeResult(&mainModule->clientIdList, mainModule->playerClientId,
                                                 BtlSetup_GetBattleType(mainModule));
    } else {
        if (mainModule->result == BTL_RESULT_NOT_SET) {
            mainModule->result = BTL_RESULT_VICTORY;
        }
        result = mainModule->result;
    }
    mainModule->setup->result = result;
    return result;
}

u16 GetCommandTimeLimit(BtlMainModule *mainModule) {
    return mainModule->commandTimeLimit;
}

u16 GetGameTimeLimit(BtlMainModule *mainModule) {
    return mainModule->gameTimeLimit;
}

BOOL IsGameTimeUp(BtlMainModule *mainModule) {
    if (mainModule->setup->fieldSituation.unk1b == 0) {
        if (mainModule->gameTimeLimit != 0) {
            return BattleClient_IsGameTimeUp(mainModule->clients[mainModule->playerClientId]);
        }
        return FALSE;
    }
    return BattleClient_IsGameTimeUp(mainModule->clients[mainModule->playerClientId]);
}

BOOL BtlMain_CheckOwnWindowDone(BtlMainModule *mainModule) {
    BOOL result;

    if (mainModule->setup->fieldSituation.unk1b != 0) {
        result = BattleClient_IsRecPlayCanceled(mainModule->clients[mainModule->playerClientId]);
        if (result) {
            mainModule->result = BTL_RESULT_DRAW;
        }
        return result;
    }
    return FALSE;
}

void SetSetupParty(BtlMainModule *mainModule, u8 clientId, const PokeParty *party) {
    u8 index = ClientIDToSetupIndex(mainModule->playerClientId, clientId);

    if (mainModule->setup->party[index] != NULL) {
        PokeParty_Copy(party, mainModule->setup->party[index]);
    }
}

void SetSetupPlayerInfo(BtlMainModule *mainModule, u8 clientId, const PlayerInfo *src) {
    u8 index = ClientIDToSetupIndex(mainModule->playerClientId, clientId);

    if (mainModule->setup->playerInfo[index] != NULL) {
        PlayerInfo_Copy(src, mainModule->setup->playerInfo[index]);
    }
}

void SetSetupChatterEffect(BtlMainModule *mainModule, u8 clientId) {
    u8 index = ClientIDToSetupIndex(mainModule->playerClientId, clientId);

    mainModule->setup->unk44[index] = GetClientChatterEffect(mainModule, clientId);
}

void BtlMain_StoreRecData(BtlMainModule *mainModule) {
    u32 size;
    void *data;

    if (IsRecordingEnabled(mainModule)) {
        data = BattleClient_GetRecordData(mainModule->clients[mainModule->playerClientId], &size);
        if (data != NULL) {
            MI_CpuCopy8(data, mainModule->setup->unkB0, size);
            mainModule->setup->unkB4 = size;
            mainModule->setup->rand = mainModule->rand;
        }
    }
}

u8 ClientIDToSetupIndex(u8 playerClientId, u8 clientId) {
    u32 result;

    if (playerClientId == clientId) {
        return 0;
    }
    result = 1;
    if ((u8)(playerClientId & result) == (u8)(clientId & result)) {
        return 2;
    }
    if (clientId > 1) {
        result = 3;
    }
    return result;
}

void BtlMain_StartRecSkip(BtlMainModule *mainModule, u32 chapter) {
    u8 commCompetitor;
    u32 i;

    commCompetitor = BtlMain_IsCommCompetitor(mainModule->setup);
    BtlCalc_ResetRand(&mainModule->rand);
    BtlMain_LoadSetupParties(mainModule, mainModule->setup);
    PokeCon_Clear(&mainModule->pokeCons[0]);
    PokeCon_Clear(&mainModule->pokeCons[1]);
    for (i = 0; i < 4; i++) {
        if (DoesClientExist(mainModule, i)) {
            PokeCon_CreateClientParty(&mainModule->pokeCons[1], mainModule, i);
            PokeCon_CreateClientParty(&mainModule->pokeCons[0], mainModule, i);
            BattleClient_StartChapterSkip(mainModule->clients[i], chapter);
        }
    }
    BtlvCore_Delete(mainModule->viewCore);
    BtlvCore_ResetGraphics(0x13);
    mainModule->viewCore = BtlvCore_Create(mainModule, mainModule->clients[mainModule->setup->fieldSituation.unk19],
                                           &mainModule->pokeCons[0], commCompetitor, 0x13);
    BattleClient_SetViewCore(mainModule->clients[mainModule->setup->fieldSituation.unk19], mainModule->viewCore);
    BattleServer_Start(mainModule->server);
}

void BtlMain_EndRecSkip(BtlMainModule *mainModule) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (DoesClientExist(mainModule, i)) {
            BattleClient_EndChapterSkip(mainModule->clients[i]);
        }
    }
}

BtlServerFlow *GetServerFlow(BtlMainModule *mainModule) {
    return BattleServer_GetFlow(mainModule->server);
}

void BtlSetup_ClearResults(BtlSetup *setup) {
    setup->unkD2 = 0;
    setup->unkD3 = 0;
    setup->unkD4 = 0;
    setup->unkD5 = 0;
    setup->unkD6 = 0;
    setup->unkD7 = 0;
    setup->unkD8 = 0;
    setup->unkD9 = 0;
    setup->unkDA = 0;
    setup->unkDB = 0;
    setup->unkDC = 0;
    setup->unkD0 = 0;
}

void BtlMain_StoreResultStats(BtlMainModule *mainModule) {
    BtlServerFlow *flow;
    BtlSetup *setup;
    BattleParty *party;
    u32 count;
    u32 alive;
    u32 i;
    BattleMon *mon;

    if (mainModule->server == NULL) {
        return;
    }
    flow = BattleServer_GetFlow(mainModule->server);
    setup = mainModule->setup;
    setup->unkD2 = GetTurnCounter(flow);
    setup->unkD3 = BtlServerFlow_GetSwitchCount(flow, 0);
    setup->unkD4 = BtlServerFlow_GetPlayerImmuneHitCount(flow);
    setup->unkD5 = BtlServerFlow_GetPlayerSuperEffectiveHitCount(flow);
    setup->unkD6 = BtlServerFlow_GetPlayerNotVeryEffectiveHitCount(flow);
    setup->unkD7 = BtlServerFlow_GetFoeImmuneHitCount(flow);
    setup->unkD8 = BtlServerFlow_GetFoeNotVeryEffectiveHitCount(flow);
    if (setup->result == BTL_RESULT_VICTORY) {
        setup->unkD9 = 1;
    }
    party = GetPartyData(&mainModule->pokeCons[1], 0);
    count = GetNumMonsInParty(party);
    alive = GetAlivePartyCount(party);
    for (i = 0; i < count; i++) {
        mon = BattleParty_GetMon(party, i);
        setup->unkDC += BattleMon_GetUsedMoveKindCount(mon);
        setup->unkD0 += GetBattleMonStat(mon, BATTLEMON_HP);
    }
    setup->unkDB = count - alive;
    party = GetPartyData(&mainModule->pokeCons[1], 1);
    count = GetNumMonsInParty(party);
    setup->unkDA = count - GetAlivePartyCount(party);
    if (DoesClientExist(mainModule, 3)) {
        party = GetPartyData(&mainModule->pokeCons[1], 3);
        if (party != NULL) {
            count = GetNumMonsInParty(party);
            setup->unkDA += (u8)(count - GetAlivePartyCount(party));
        }
    }
}

u32 GetPlayerFirstActionMove(BtlMainModule *mainModule) {
    return BattleClient_GetFirstActionMove(mainModule->clients[0]);
}

BtlSetup *GetBattleSetup(BtlMainModule *mainModule) {
    return mainModule->setup;
}

BtlSetup *GetBattleSetupParam(BtlMainModule *mainModule) {
    return mainModule->setup;
}

void BtlMain_LoadStudioData(BtlMainModule *mainModule, u8 movie) {
    mainModule->studioRules = GFL_ArcSysReadHeapNew(ARCID_POKEWOOD_MOVIE, movie, HEAPID_BATTLE);
    mainModule->studioWork = GFL_HeapAllocate(HEAPID_BATTLE, 0x14, FALSE, "btl_main.c", 6432);
    func_ov167_0219e3c8(mainModule->studioRules);
    mainModule->studioWork->aiMoveIndex = 0xff;
    mainModule->cutin = PokewoodCutin_Create(HEAPID_BATTLE);
    if (GetStudioMode(mainModule) == BTL_STUDIO_MODE_PLAYBACK) {
        SkipExitBGMFadeOut(mainModule);
    }
}

void BtlMain_FreeStudioData(BtlMainModule *mainModule) {
    PokewoodCutin_Delete(mainModule->cutin);
    GFL_HeapFree(mainModule->studioWork);
    GFL_HeapFree(mainModule->studioRules);
}

BtlScriptedRules *GetStudioRules(BtlMainModule *mainModule) {
    if (mainModule->studioRules == NULL) {
        return NULL;
    }
    return mainModule->studioRules;
}

BtlStudioWork *GetStudioWork(BtlMainModule *mainModule) {
    if (mainModule->studioWork == NULL) {
        return NULL;
    }
    return mainModule->studioWork;
}

PokewoodCutin *GetPokewoodCutin(BtlMainModule *mainModule) {
    return mainModule->cutin;
}

void func_ov167_0219e3c8(BtlScriptedRules *data) {
}
