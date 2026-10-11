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
#include "battle/btl_server.h"
#include "battle/btl_server_cmd.h"
#include "battle/btl_server_flow.h"
#include "battle/btl_setup.h"
#include "battle/btlv.h"
#include "battle/pokewood_cutin.h"
#include "constants/abilities.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "gfl/arc.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "system/comm_player_support.h"
#include "system/printsys.h"
#include "system/time_icon.h"
#include "system/text_speed.h"
#include "system/app_keycursor.h"
#include "system/gf_font.h"
#include "system/ir_check.h"
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
#include "save/pokedex.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/ringtone_sys.h"
#include "text/system/ability_handlers_btl_main.h"

// The first mon ID of each client
static const u8 data_ov167_021d6c24[4] = { 0, 12, 6, 18 };

static const u8 data_ov167_021d6c28[2][3] = {
    { 3, 5, 0 },
    { 2, 4, 0 },
};

static const u8 data_ov167_021d6c2e[2][3] = {
    { 3, 5, 7 },
    { 2, 4, 6 },
};

// The steps of setting up a link battle, after the first, for each battle style
static BOOL (*const data_ov167_021d6c34[7])(BtlMainModule *mainModule, s32 *state) = {
    NULL,
    func_ov167_0219ada0,
    func_ov167_0219af50,
    func_ov167_0219b160,
    func_ov167_0219b3a8,
    func_ov167_0219b610,
    func_ov167_0219bb54,
};

static BOOL (*const data_ov167_021d6c50[7])(BtlMainModule *mainModule, s32 *state) = {
    NULL,
    func_ov167_0219ada0,
    func_ov167_0219af50,
    func_ov167_0219b160,
    func_ov167_0219b3a8,
    func_ov167_0219b4ac,
    func_ov167_0219bb54,
};

static BOOL (*const data_ov167_021d6c6c[7])(BtlMainModule *mainModule, s32 *state) = {
    NULL,
    func_ov167_0219ada0,
    func_ov167_0219af50,
    func_ov167_0219b160,
    func_ov167_0219b3a8,
    func_ov167_0219b9d4,
    func_ov167_0219bb54,
};

static BOOL (*const data_ov167_021d6c88[7])(BtlMainModule *mainModule, s32 *state) = {
    NULL,
    func_ov167_0219ada0,
    func_ov167_0219af50,
    func_ov167_0219b160,
    func_ov167_0219b3a8,
    func_ov167_0219b868,
    func_ov167_0219bb54,
};

static const AdjacentOpponentData data_ov167_021d6ca4[BTL_POS_MAX] = {
    { 2, 2, { 5, 3, 6 }, { 0, 2, 6 } },
    { 2, 2, { 4, 2, 6 }, { 1, 3, 6 } },
    { 3, 3, { 1, 3, 5 }, { 0, 2, 4 } },
    { 3, 3, { 0, 2, 4 }, { 1, 3, 5 } },
    { 2, 2, { 1, 3, 6 }, { 4, 2, 6 } },
    { 2, 2, { 0, 2, 6 }, { 5, 3, 6 } },
};

// The battle system's proc
const GameProcFunctions data_ov167_021d6ce0 = { func_ov167_021998c0, func_ov167_02199c08, func_ov167_02199cd4 };

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
        if (!seq->nextFunc(&seq->state, seq->mainModule)) {
            return FALSE;
        }
        seq->nextFunc = NULL;
    }
    return TRUE;
}

BOOL func_ov167_021998c0(GameProc *proc, u32 *state, void *param, void *work) {
    BtlMainModule *mainModule;
    BtlSetup *setup = param;
    BOOL notInitialized;
    BOOL isSet;

    switch (*state) {
    case 0:
        GFL_HeapCreateChild(HEAPID_USER, HEAPID_BATTLE, 0x20000);
        GFL_HeapCreateChild(HEAPID_USER, 0x14, 0x8000);
        GFL_HeapCreateChild(HEAPID_USER, 0x13, 0xac500);
        mainModule = GFL_ProcInitSubsystem(proc, sizeof(BtlMainModule), HEAPID_BATTLE);
        sys_memset32(0, mainModule, sizeof(BtlMainModule));
        func_ov167_021ce604(0x13);
        func_ov167_021ce10c();
        if (!GFL_SndIsVolumeControlCallbackSet()) {
            GFL_SndSetVolumeControlCallbacks();
            mainModule->unk473_7 = 1;
        }
        mainModule->heapId = HEAPID_BATTLE;
        mainModule->setup = setup;
        setup->unkAC = 6;
        mainModule->unkC0 = mainModule->setup->unk34[0];
        mainModule->unk440 = mainModule->setup->unk8C;
        mainModule->unk442 = mainModule->setup->unk8E;
        mainModule->unk473_3 = 0;
        mainModule->unk473_4 = 0;
        mainModule->unk473_0 = 0;
        mainModule->unk473_1 = 0;
        mainModule->unk473_5 = 0;
        mainModule->unk473_6 = 0;
        mainModule->unk471 = 0;
        mainModule->mainFunc = NULL;
        notInitialized = FALSE;
        mainModule->viewCore = NULL;
        mainModule->result = 7;
        mainModule->unk43C = func_02008a14(mainModule->setup->config);
        if (!func_02008a4c(setup->config)) {
            notInitialized = TRUE;
        }
        mainModule->unk473_2 = (u8)notInitialized;
        mainModule->field = func_ov167_021d5a84(mainModule->heapId);
        func_ov167_0219ccbc(&mainModule->pokeCons[0], mainModule, FALSE);
        func_ov167_0219ccbc(&mainModule->pokeCons[1], mainModule, TRUE);
        func_ov167_0219e164(mainModule->setup);
        mainModule->unk2C0 = NULL;
        mainModule->unk2C4 = NULL;
        if (mainModule->setup->fieldSituation.unk1b == 0) {
            buildSeed(&mainModule->rand);
        } else {
            mainModule->rand = setup->rand;
        }
        func_ov167_021bd054(&mainModule->rand, mainModule->heapId);
        func_ov167_021b1670();
        mainModule->prizeMoney = func_ov167_021bd7b0(setup);
        mainModule->money = 0;
        mainModule->unk438 = 0;
        if (mainModule->setup->fieldSituation.unk1b == 0) {
            u32 i;

            for (i = 0; i < 4; i++) {
                mainModule->setup->unk44[i] = 0;
            }
        }
        func_ov167_021bda58(&mainModule->unk448);
        isSet = FALSE;
        if (func_02008a68(setup->config)) {
            isSet = TRUE;
        }
        mainModule->unk46F = isSet;
        func_ov167_021b9950(setup->fieldSituation.netHandle, setup->unkA0, 0x14);
        func_ov167_021bd874(HEAPID_BATTLE);
        if (setup->unkDD_3) {
            func_ov167_0219e314(mainModule, setup->unkDF);
        }
        (*state)++;
        break;
    case 1:
        mainModule = work;
        if (mainModule->setup->fieldSituation.netHandle != NULL && func_ov167_021b9a70()) {
            mainModule->unk473_0 = 1;
            mainModule->unk473_1 = 1;
            return TRUE;
        }
        if (func_ov167_021b9a30()) {
            func_ov167_021d4a1c(mainModule->setup->fieldSituation.unk18);
            func_ov167_02199ec0(&mainModule->seq, mainModule, mainModule->setup);
            mainModule->unk460 = 0;
            func_ov167_0219d6ac(mainModule);
            func_ov167_0219d9b0(mainModule);
            (*state)++;
        }
        break;
    case 2:
        mainModule = work;
        if (BtlMainSeq_Run(&mainModule->seq)) {
            if (mainModule->unk473_0) {
                mainModule->unk473_1 = 1;
                return TRUE;
            }
            if (mainModule->setup->battleType != 4) {
                func_ov167_0219dad0(mainModule, 4);
            }
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL func_ov167_02199c08(GameProc *proc, u32 *state, void *param, void *work) {
    BtlMainModule *mainModule = work;
    u32 result;

    if (mainModule->mainFunc == NULL) {
        return TRUE;
    }
    if (mainModule->mainFunc(mainModule)) {
        if (!func_ov167_0219c980(mainModule)) {
            result = func_ov167_0219de84(mainModule);
            func_ov167_0219dff8(mainModule);
            func_ov167_0219e1b0(mainModule);
            func_ov167_0219dc10(mainModule);
            switch (result) {
            case 1:
                mainModule->setup->unkA4 = mainModule->prizeMoney + func_ov167_0219caec(mainModule);
                break;
            case 0:
            case 2:
                mainModule->setup->unkA4 = -mainModule->unk438;
                break;
            }
        } else {
            mainModule->setup->unkDD_1 = (u8)func_ov167_02199ca0(mainModule);
        }
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov167_02199ca0(BtlMainModule *mainModule) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (DoesClientExist(mainModule, i) && !func_ov167_021d4880(&mainModule->recReader, i)) {
            return FALSE;
        }
    }
    return TRUE;
}

BOOL func_ov167_02199cd4(GameProc *proc, u32 *state, void *param, void *work) {
    BtlMainModule *mainModule = work;

    switch (*state) {
    case 0:
        if (!mainModule->unk473_5) {
            GFL_SndBGMFadeOut(30);
        }
        mainModule->unk460 = 0;
        if (func_ov167_0219c988(mainModule)) {
            func_ov167_0219e378(mainModule);
        }
        if (gfxRegGetMasterBrightness(REG_MASTER_BRIGHT_ADDR)) {
            mainModule->unk460 = -16;
            GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, -16);
            GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, -16);
            *state += 2;
            break;
        }
        (*state)++;
        break;
    case 1:
        if (mainModule->unk460 > -16) {
            mainModule->unk460--;
            if (gfxRegGetMasterBrightness(REG_MASTER_BRIGHT_ADDR) > mainModule->unk460) {
                GFXRegSetMasterBrightness(REG_MASTER_BRIGHT_ADDR, mainModule->unk460);
            }
            if (gfxRegGetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR) > mainModule->unk460) {
                GFXRegSetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR, mainModule->unk460);
            }
            break;
        }
        (*state)++;
        break;
    case 2:
        if (!mainModule->unk473_5) {
            if (GFL_SndBGMIsFading()) {
                break;
            }
            func_02005d8c();
        }
        (*state)++;
        break;
    case 3:
        if (mainModule->field != NULL) {
            func_ov167_021d5aac(mainModule->field);
            mainModule->field = NULL;
        }
        if (mainModule->unk2C0 != NULL) {
            GFL_HeapFree(mainModule->unk2C0);
            mainModule->unk2C0 = NULL;
        }
        if (mainModule->unk2C4 != NULL) {
            func_ov167_021ba55c(mainModule->unk2C4);
            mainModule->unk2C4 = NULL;
        }
        func_ov167_021bd0a8();
        func_ov167_0219d9e8(mainModule);
        func_ov167_0219d6dc(mainModule);
        func_ov167_02199ff0(&mainModule->seq, mainModule, mainModule->setup);
        (*state)++;
        break;
    case 4:
        if (BtlMainSeq_Run(&mainModule->seq)) {
            func_ov167_021d4a40();
            func_ov167_021b9a54();
            func_ov167_021bd880();
            (*state)++;
        }
        break;
    case 5:
        func_ov167_021ce638();
        GFL_SndPlayerSetMuteStateEx(1, 0x3e);
        PokeVoice_ResetMasterVolume();
        if (mainModule->unk473_7) {
            RingtoneSys_RestoreLidCallbacks();
            mainModule->unk473_7 = 0;
        }
        GFL_ProcReleaseSubsystem(proc);
        GFL_HeapDelete(0x13);
        GFL_HeapDelete(0x14);
        GFL_HeapDelete(HEAPID_BATTLE);
        func_ov167_021ce138();
        return TRUE;
    }
    return FALSE;
}

void func_ov167_02199ec0(BtlMainSeq *seq, BtlMainModule *mainModule, BtlSetup *setup) {
    u32 i;

    for (i = 0; i < BTL_POS_MAX; i++) {
        mainModule->posClientIds[i] = 4;
    }
    if (setup->fieldSituation.unk18 == 0) {
        switch (setup->battleStyle) {
        case 0:
            BtlMainSeq_Set(seq, func_ov167_0219a298, mainModule);
            return;
        case 1:
            if (setup->fieldSituation.unk1a == 0) {
                BtlMainSeq_Set(seq, func_ov167_0219a448, mainModule);
                return;
            }
            BtlMainSeq_Set(seq, func_ov167_0219a5bc, mainModule);
            return;
        case 2:
            BtlMainSeq_Set(seq, func_ov167_0219a848, mainModule);
            return;
        case 3:
            BtlMainSeq_Set(seq, func_ov167_0219a9cc, mainModule);
            return;
        default:
            BtlMainSeq_Set(seq, func_ov167_0219a298, mainModule);
            return;
        }
    }
    switch (setup->battleStyle) {
    case 0:
        BtlMainSeq_Set(seq, func_ov167_0219ab44, mainModule);
        return;
    case 1:
        BtlMainSeq_Set(seq, func_ov167_0219abbc, mainModule);
        return;
    case 2:
        BtlMainSeq_Set(seq, func_ov167_0219ad10, mainModule);
        return;
    case 3:
        BtlMainSeq_Set(seq, func_ov167_0219ac80, mainModule);
        return;
    default:
        BtlMainSeq_Set(seq, func_ov167_0219a298, mainModule);
        return;
    }
}

void func_ov167_02199ff0(BtlMainSeq *seq, BtlMainModule *mainModule, BtlSetup *setup) {
    BtlMainSeq_Set(seq, func_ov167_0219a3f4, mainModule);
}

BOOL func_ov167_0219a004(BtlSetup *setup) {
    switch (setup->battleType) {
    case 0:
        return FALSE;
    case 1:
        return FALSE;
    case 2:
        return FALSE;
    case 3:
        return TRUE;
    default:
        return FALSE;
    }
}

void func_ov167_0219a034(BtlMainModule *mainModule, BtlSetup *setup) {
    u32 i;
    void *chatter;
    u8 chatterFlags[4];

    mainModule->playerClientId = setup->fieldSituation.unk19;
    i = 0;
    mainModule->unk46D = func_ov167_0219c458(mainModule, mainModule->playerClientId, 0);
    func_ov167_0219a0c4(mainModule, setup);
    if (!func_ov167_0219c980(mainModule)) {
        chatter = getChatterBlockAddress(GameData_GetSaveControl(setup->gameData));
        mainModule->unk3E0[0] = allocChatotChatterBlk(mainModule->heapId);
        moveChatter(mainModule->unk3E0[0], chatter);
        mainModule->setup->unk44[0] = func_ov167_0219d258(mainModule, i);
        return;
    }
    for (; i < 4; i++) {
        chatterFlags[func_ov167_0219e048(mainModule->playerClientId, i)] = mainModule->setup->unk44[i];
    }
    for (i = 0; i < 4; i++) {
        mainModule->setup->unk44[i] = chatterFlags[i];
    }
}

void func_ov167_0219a0c4(BtlMainModule *mainModule, BtlSetup *setup) {
    u8 index0;
    u8 index1;
    u8 index2;
    u8 index3;

    index0 = func_ov167_0219e048(mainModule->playerClientId, 0);
    index1 = func_ov167_0219e048(mainModule->playerClientId, 1);
    func_ov167_0219da44(mainModule, 0, setup->party[index0]);
    func_ov167_0219da44(mainModule, 1, setup->party[index1]);
    index2 = func_ov167_0219e048(mainModule->playerClientId, 2);
    index3 = func_ov167_0219e048(mainModule->playerClientId, 3);
    if (setup->party[index2] != NULL) {
        func_ov167_0219da44(mainModule, 2, setup->party[index2]);
    }
    if (setup->party[index3] != NULL) {
        func_ov167_0219da44(mainModule, 3, setup->party[index3]);
    }
}

void func_ov167_0219a140(BtlMainModule *mainModule, u8 clientId) {
    BtlSetup *setup = mainModule->setup;
    u8 index = func_ov167_0219e048(mainModule->playerClientId, clientId);
    BtlTrainerData *trainer = &mainModule->trainers[clientId];

    if (setup->trainers[index]->trainerId != 0) {
        func_ov167_0219d794(trainer, setup->trainers[index]);
    } else {
        func_ov167_0219d72c(trainer, mainModule->heapId, setup->unk34[index]);
    }
}


u8 func_ov167_0219a180(BtlMainModule *mainModule, u8 clientId) {
    BtlSetup *setup = mainModule->setup;

    switch (setup->battleStyle) {
    case 0:
        return 1;
    case 3:
        return 1;
    case 2:
        return 3;
    case 1:
        if (setup->fieldSituation.unk1a == 0) {
            return 2;
        }
        if (setup->battleType == 3) {
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

void func_ov167_0219a1e8(BtlMainModule *mainModule, BtlSetup *setup) {
    if (setup->fieldSituation.unk1b == 0) {
        func_ov167_0219d72c(&mainModule->trainers[0], mainModule->heapId, mainModule->unkC0);
        func_ov167_0219d794(&mainModule->trainers[1], setup->trainers[1]);
    } else {
        func_ov167_0219a140(mainModule, 0);
        func_ov167_0219a140(mainModule, 1);
    }
}

void func_ov167_0219a228(BtlMainModule *mainModule, BtlSetup *setup) {
    if (setup->fieldSituation.unk1b != 0) {
        func_ov167_021d4630(&mainModule->recReader, setup->unkB0, setup->unkB4);
        func_ov167_021b18e8(mainModule->clients[0], &mainModule->recReader);
        func_ov167_021b18e8(mainModule->clients[1], &mainModule->recReader);
    }
}

void func_ov167_0219a25c(BtlMainModule *mainModule, BtlSetup *setup, u32 arg2) {
    BtlClient *client;

    if (setup->fieldSituation.unk1b != 0) {
        client = mainModule->clients[setup->fieldSituation.unk19];
    } else {
        client = mainModule->clients[0];
    }
    mainModule->viewCore = BtlvCore_Create(mainModule, client, &mainModule->pokeCons[0], arg2, 0x13);
    func_ov167_021b190c(client, mainModule->viewCore);
}

BOOL func_ov167_0219a298(u32 *state, BtlMainModule *mainModule) {
    BtlSetup *setup = mainModule->setup;
    u32 unk = func_ov167_0219a004(setup);

    mainModule->clientCount = 2;
    mainModule->posClientIds[0] = 0;
    mainModule->posClientIds[1] = 1;
    func_ov167_0219a034(mainModule, setup);
    func_ov167_0219ccbc(&mainModule->pokeCons[0], mainModule, FALSE);
    func_ov167_0219cd3c(&mainModule->pokeCons[0], mainModule, 0);
    func_ov167_0219cd3c(&mainModule->pokeCons[0], mainModule, 1);
    func_ov167_0219ccbc(&mainModule->pokeCons[1], mainModule, TRUE);
    func_ov167_0219cd3c(&mainModule->pokeCons[1], mainModule, 0);
    func_ov167_0219cd3c(&mainModule->pokeCons[1], mainModule, 1);
    mainModule->server =
        func_ov167_0219e3cc(mainModule, &mainModule->rand, &mainModule->pokeCons[1], unk, mainModule->heapId);
    mainModule->unk46E = 1;
    func_ov167_0219a1e8(mainModule, setup);
    mainModule->clients[0] = BattleClient_Create(mainModule, &mainModule->pokeCons[0], 0, setup->fieldSituation.netHandle, 0, 1,
                                                 0, unk, setup->fieldSituation.unk1b, &mainModule->rand, mainModule->heapId);
    mainModule->clients[1] = BattleClient_Create(mainModule, &mainModule->pokeCons[0], 0, setup->fieldSituation.netHandle, 1, 1,
                                                 1, unk, setup->fieldSituation.unk1b, &mainModule->rand, mainModule->heapId);
    func_ov167_0219a228(mainModule, setup);
    func_ov167_0219a25c(mainModule, setup, unk);
    func_ov167_0219e498(mainModule->server, func_ov167_021b1928(mainModule->clients[0]), 0, 1);
    func_ov167_0219e498(mainModule->server, func_ov167_021b1928(mainModule->clients[1]), 1, 1);
    func_ov167_0219e544(mainModule->server);
    mainModule->mainFunc = func_ov167_0219bbec;
    return TRUE;
}

BOOL func_ov167_0219a3f4(u32 *state, BtlMainModule *mainModule) {
    u32 i;

    if (mainModule->viewCore != NULL) {
        func_ov167_021ce870(mainModule->viewCore);
        mainModule->viewCore = NULL;
    }
    for (i = 0; i < 4; i++) {
        if (mainModule->clients[i] != NULL) {
            BattleClient_Delete(mainModule->clients[i]);
        }
    }
    func_ov167_0219cf50(&mainModule->pokeCons[0]);
    func_ov167_0219cf50(&mainModule->pokeCons[1]);
    if (mainModule->server != NULL) {
        func_ov167_0219e560(mainModule->server);
    }
    if (mainModule->unk0C != NULL) {
        func_ov167_0219e560(mainModule->unk0C);
    }
    return TRUE;
}

BOOL func_ov167_0219a448(u32 *state, BtlMainModule *mainModule) {
    BtlSetup *setup = mainModule->setup;
    u32 unk = func_ov167_0219a004(setup);

    mainModule->playerClientId = 0;
    mainModule->clientCount = 2;
    mainModule->posClientIds[0] = 0;
    mainModule->posClientIds[1] = 1;
    mainModule->posClientIds[2] = 0;
    mainModule->posClientIds[3] = 1;
    func_ov167_0219a034(mainModule, setup);
    func_ov167_0219ccbc(&mainModule->pokeCons[0], mainModule, FALSE);
    func_ov167_0219cd3c(&mainModule->pokeCons[0], mainModule, 0);
    func_ov167_0219cd3c(&mainModule->pokeCons[0], mainModule, 1);
    func_ov167_0219ccbc(&mainModule->pokeCons[1], mainModule, TRUE);
    func_ov167_0219cd3c(&mainModule->pokeCons[1], mainModule, 0);
    func_ov167_0219cd3c(&mainModule->pokeCons[1], mainModule, 1);
    func_ov167_0219a1e8(mainModule, setup);
    mainModule->server =
        func_ov167_0219e3cc(mainModule, &mainModule->rand, &mainModule->pokeCons[1], unk, mainModule->heapId);
    mainModule->unk46E = 1;
    mainModule->clients[0] = BattleClient_Create(mainModule, &mainModule->pokeCons[0], 0, setup->fieldSituation.netHandle, 0, 2,
                                                 0, unk, setup->fieldSituation.unk1b, &mainModule->rand, mainModule->heapId);
    mainModule->clients[1] = BattleClient_Create(mainModule, &mainModule->pokeCons[0], 0, setup->fieldSituation.netHandle, 1, 2,
                                                 1, unk, setup->fieldSituation.unk1b, &mainModule->rand, mainModule->heapId);
    func_ov167_0219a228(mainModule, setup);
    func_ov167_0219a25c(mainModule, setup, unk);
    func_ov167_0219e498(mainModule->server, func_ov167_021b1928(mainModule->clients[0]), 0, 2);
    func_ov167_0219e498(mainModule->server, func_ov167_021b1928(mainModule->clients[1]), 1, 2);
    func_ov167_0219e544(mainModule->server);
    mainModule->mainFunc = func_ov167_0219bbec;
    return TRUE;
}

BOOL func_ov167_0219a5bc(u32 *state, BtlMainModule *mainModule) {
    BtlSetup *setup = mainModule->setup;
    u32 unk = func_ov167_0219a004(setup);
    s32 i;

    mainModule->playerClientId = 0;
    mainModule->clientCount = 2;
    mainModule->posClientIds[0] = 0;
    mainModule->posClientIds[2] = 0;
    mainModule->posClientIds[1] = 1;
    mainModule->posClientIds[3] = 1;
    switch (setup->fieldSituation.unk1a) {
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
    func_ov167_0219a034(mainModule, setup);
    func_ov167_0219ccbc(&mainModule->pokeCons[0], mainModule, FALSE);
    func_ov167_0219ccbc(&mainModule->pokeCons[1], mainModule, TRUE);
    for (i = 0; i < 4; i++) {
        if (DoesClientExist(mainModule, i)) {
            func_ov167_0219cd3c(&mainModule->pokeCons[0], mainModule, i);
            func_ov167_0219cd3c(&mainModule->pokeCons[1], mainModule, i);
        }
    }
    if (setup->fieldSituation.unk1b == 0) {
        func_ov167_0219d72c(&mainModule->trainers[0], mainModule->heapId, mainModule->unkC0);
        func_ov167_0219d794(&mainModule->trainers[1], setup->trainers[1]);
        if (DoesClientExist(mainModule, 2)) {
            func_ov167_0219d794(&mainModule->trainers[2], setup->trainers[2]);
        }
        if (DoesClientExist(mainModule, 3)) {
            func_ov167_0219d794(&mainModule->trainers[3], setup->trainers[3]);
        }
    } else {
        for (i = 0; i < 4; i++) {
            if (DoesClientExist(mainModule, i)) {
                func_ov167_0219a140(mainModule, i);
            }
        }
    }
    mainModule->server =
        func_ov167_0219e3cc(mainModule, &mainModule->rand, &mainModule->pokeCons[1], unk, mainModule->heapId);
    mainModule->unk46E = 1;
    for (i = 0; i < 4; i++) {
        if (DoesClientExist(mainModule, i)) {
            u8 count = func_ov167_0219c3e4(mainModule, i);
            BOOL isAI = FALSE;

            if (i != 0) {
                isAI = TRUE;
            }

            mainModule->clients[i] =
                BattleClient_Create(mainModule, &mainModule->pokeCons[0], 0, setup->fieldSituation.netHandle, i, count,
                                    isAI, unk, setup->fieldSituation.unk1b, &mainModule->rand, mainModule->heapId);
        }
    }
    if (setup->fieldSituation.unk1b != 0) {
        func_ov167_021d4630(&mainModule->recReader, setup->unkB0, setup->unkB4);
        for (i = 0; i < 4; i++) {
            if (DoesClientExist(mainModule, i)) {
                func_ov167_021b18e8(mainModule->clients[i], &mainModule->recReader);
            }
        }
    }
    func_ov167_0219a25c(mainModule, setup, unk);
    for (i = 0; i < 4; i++) {
        if (DoesClientExist(mainModule, i)) {
            u8 count = func_ov167_0219c3e4(mainModule, i);

            func_ov167_0219e498(mainModule->server, func_ov167_021b1928(mainModule->clients[i]), i, count);
        }
    }
    func_ov167_0219e544(mainModule->server);
    mainModule->mainFunc = func_ov167_0219bbec;
    return TRUE;
}

BOOL func_ov167_0219a848(u32 *state, BtlMainModule *mainModule) {
    BtlSetup *setup = mainModule->setup;
    u32 unk = func_ov167_0219a004(setup);

    mainModule->clientCount = 2;
    mainModule->unk46E = 1;
    mainModule->posClientIds[0] = 0;
    mainModule->posClientIds[1] = 1;
    mainModule->posClientIds[2] = 0;
    mainModule->posClientIds[3] = 1;
    mainModule->posClientIds[4] = 0;
    mainModule->posClientIds[5] = 1;
    func_ov167_0219a034(mainModule, setup);
    func_ov167_0219ccbc(&mainModule->pokeCons[0], mainModule, FALSE);
    func_ov167_0219cd3c(&mainModule->pokeCons[0], mainModule, 0);
    func_ov167_0219cd3c(&mainModule->pokeCons[0], mainModule, 1);
    func_ov167_0219ccbc(&mainModule->pokeCons[1], mainModule, TRUE);
    func_ov167_0219cd3c(&mainModule->pokeCons[1], mainModule, 0);
    func_ov167_0219cd3c(&mainModule->pokeCons[1], mainModule, 1);
    func_ov167_0219a1e8(mainModule, setup);
    mainModule->server =
        func_ov167_0219e3cc(mainModule, &mainModule->rand, &mainModule->pokeCons[1], unk, mainModule->heapId);
    mainModule->unk46E = 1;
    mainModule->clients[0] = BattleClient_Create(mainModule, &mainModule->pokeCons[0], 0, setup->fieldSituation.netHandle, 0, 3,
                                                 0, unk, setup->fieldSituation.unk1b, &mainModule->rand, mainModule->heapId);
    mainModule->clients[1] = BattleClient_Create(mainModule, &mainModule->pokeCons[0], 0, setup->fieldSituation.netHandle, 1, 3,
                                                 1, unk, setup->fieldSituation.unk1b, &mainModule->rand, mainModule->heapId);
    func_ov167_0219a228(mainModule, setup);
    func_ov167_0219a25c(mainModule, setup, unk);
    func_ov167_0219e498(mainModule->server, func_ov167_021b1928(mainModule->clients[0]), 0, 3);
    func_ov167_0219e498(mainModule->server, func_ov167_021b1928(mainModule->clients[1]), 1, 3);
    func_ov167_0219e544(mainModule->server);
    mainModule->mainFunc = func_ov167_0219bbec;
    return TRUE;
}

BOOL func_ov167_0219a9cc(u32 *state, BtlMainModule *mainModule) {
    BtlSetup *setup = mainModule->setup;
    u32 unk = func_ov167_0219a004(setup);

    mainModule->clientCount = 2;
    mainModule->posClientIds[0] = 0;
    mainModule->posClientIds[1] = 1;
    mainModule->posClientIds[2] = 0;
    mainModule->posClientIds[3] = 1;
    mainModule->posClientIds[4] = 0;
    mainModule->posClientIds[5] = 1;
    func_ov167_0219a034(mainModule, setup);
    func_ov167_0219ccbc(&mainModule->pokeCons[0], mainModule, FALSE);
    func_ov167_0219cd3c(&mainModule->pokeCons[0], mainModule, 0);
    func_ov167_0219cd3c(&mainModule->pokeCons[0], mainModule, 1);
    func_ov167_0219ccbc(&mainModule->pokeCons[1], mainModule, TRUE);
    func_ov167_0219cd3c(&mainModule->pokeCons[1], mainModule, 0);
    func_ov167_0219cd3c(&mainModule->pokeCons[1], mainModule, 1);
    mainModule->server =
        func_ov167_0219e3cc(mainModule, &mainModule->rand, &mainModule->pokeCons[1], unk, mainModule->heapId);
    mainModule->unk46E = 1;
    func_ov167_0219a1e8(mainModule, setup);
    mainModule->clients[0] = BattleClient_Create(mainModule, &mainModule->pokeCons[0], 0, setup->fieldSituation.netHandle, 0, 1,
                                                 0, unk, setup->fieldSituation.unk1b, &mainModule->rand, mainModule->heapId);
    mainModule->clients[1] = BattleClient_Create(mainModule, &mainModule->pokeCons[0], 0, setup->fieldSituation.netHandle, 1, 1,
                                                 1, unk, setup->fieldSituation.unk1b, &mainModule->rand, mainModule->heapId);
    func_ov167_0219a228(mainModule, setup);
    func_ov167_0219a25c(mainModule, setup, unk);
    func_ov167_0219e498(mainModule->server, func_ov167_021b1928(mainModule->clients[0]), 0, 1);
    func_ov167_0219e498(mainModule->server, func_ov167_021b1928(mainModule->clients[1]), 1, 1);
    func_ov167_0219e544(mainModule->server);
    mainModule->mainFunc = func_ov167_0219bbec;
    return TRUE;
}

BOOL func_ov167_0219ab44(u32 *state, BtlMainModule *mainModule) {
    if (*state == 0) {
        mainModule->clientCount = 2;
        mainModule->posClientIds[0] = 0;
        mainModule->posClientIds[1] = 1;
        mainModule->playerClientId = mainModule->setup->fieldSituation.unk19;
        mainModule->unk46D = func_ov167_0219c458(mainModule, mainModule->playerClientId, 0);
        (*state)++;
        return FALSE;
    }
    if (*state < 7) {
        if (data_ov167_021d6c50[*state](mainModule, &mainModule->unk460)) {
            mainModule->unk460 = 0;
            (*state)++;
        }
        return FALSE;
    }
    return TRUE;
}

BOOL func_ov167_0219abbc(u32 *state, BtlMainModule *mainModule) {
    if (*state == 0) {
        switch (mainModule->setup->fieldSituation.unk1a) {
        case 0:
        default:
            mainModule->clientCount = 2;
            mainModule->posClientIds[0] = 0;
            mainModule->posClientIds[1] = 1;
            mainModule->posClientIds[2] = 0;
            mainModule->posClientIds[3] = 1;
            break;
        case 2:
            mainModule->unk471 = 2;
        case 1:
            mainModule->clientCount = 4;
            mainModule->posClientIds[0] = 0;
            mainModule->posClientIds[1] = 1;
            mainModule->posClientIds[2] = 2;
            mainModule->posClientIds[3] = 3;
            break;
        }
        mainModule->playerClientId = mainModule->setup->fieldSituation.unk19;
        mainModule->unk46D = func_ov167_0219c458(mainModule, mainModule->playerClientId, 0);
        (*state)++;
        return FALSE;
    }
    if (*state < 7) {
        if (data_ov167_021d6c34[*state](mainModule, &mainModule->unk460)) {
            mainModule->unk460 = 0;
            (*state)++;
        }
        return FALSE;
    }
    return TRUE;
}

BOOL func_ov167_0219ac80(u32 *state, BtlMainModule *mainModule) {
    if (*state == 0) {
        mainModule->clientCount = 2;
        mainModule->posClientIds[0] = 0;
        mainModule->posClientIds[1] = 1;
        mainModule->posClientIds[2] = 0;
        mainModule->posClientIds[3] = 1;
        mainModule->posClientIds[4] = 0;
        mainModule->posClientIds[5] = 1;
        mainModule->playerClientId = mainModule->setup->fieldSituation.unk19;
        mainModule->unk46D = func_ov167_0219c458(mainModule, mainModule->playerClientId, 0);
        (*state)++;
        return FALSE;
    }
    if (*state < 7) {
        if (data_ov167_021d6c6c[*state](mainModule, &mainModule->unk460)) {
            mainModule->unk460 = 0;
            (*state)++;
        }
        return FALSE;
    }
    return TRUE;
}

BOOL func_ov167_0219ad10(u32 *state, BtlMainModule *mainModule) {
    if (*state == 0) {
        mainModule->clientCount = 2;
        mainModule->posClientIds[0] = 0;
        mainModule->posClientIds[1] = 1;
        mainModule->posClientIds[2] = 0;
        mainModule->posClientIds[3] = 1;
        mainModule->posClientIds[4] = 0;
        mainModule->posClientIds[5] = 1;
        mainModule->playerClientId = mainModule->setup->fieldSituation.unk19;
        mainModule->unk46D = func_ov167_0219c458(mainModule, mainModule->playerClientId, 0);
        (*state)++;
        return FALSE;
    }
    if (*state < 7) {
        if (data_ov167_021d6c88[*state](mainModule, &mainModule->unk460)) {
            mainModule->unk460 = 0;
            (*state)++;
        }
        return FALSE;
    }
    return TRUE;
}

BOOL func_ov167_0219ada0(BtlMainModule *mainModule, s32 *state) {
    if (func_ov167_021b9a70()) {
        mainModule->unk473_0 = 1;
        return TRUE;
    }
    switch (*state) {
    case 0:
        if (func_ov167_021b9a94(mainModule->playerClientId)) {
            (*state)++;
        }
        break;
    case 1:
        if (func_ov167_021b9b48()) {
            mainModule->unk46A = 0;
            mainModule->unk46E = func_ov167_021b9b60();
            mainModule->setup->unkAD = func_ov167_021b9b78();
            if (func_ov167_021b9b60()) {
                *state = 2;
            } else {
                *state = 3;
            }
        }
        break;
    case 2:
        mainModule->syncData.rand = mainModule->rand;
        mainModule->syncData.unk18 = mainModule->setup->unkA2;
        mainModule->syncData.unk1A = mainModule->setup->unk8C;
        mainModule->syncData.unk1C = mainModule->setup->unk8E;
        if (BtlSetup_IsBattleType(mainModule, 0x1000)) {
            mainModule->syncData.unk1E = mainModule->unk43C;
            mainModule->syncData.unk1F_0 = mainModule->unk473_2;
        } else {
            mainModule->syncData.unk1E = 2;
            mainModule->syncData.unk1F_0 = 1;
        }
        if (func_ov167_021b9bb8(&mainModule->syncData)) {
            (*state)++;
        }
        break;
    case 3:
        if (func_ov167_021b9c0c(&mainModule->syncData)) {
            BtlSetup *setup = mainModule->setup;
            setup->unkA2 = mainModule->syncData.unk18;
            setup->rand = mainModule->syncData.rand;
            mainModule->rand = mainModule->syncData.rand;
            mainModule->unk473_2 = mainModule->syncData.unk1F_0;
            mainModule->unk43C = mainModule->syncData.unk1E;
            mainModule->unk440 = mainModule->syncData.unk1A;
            mainModule->unk442 = mainModule->syncData.unk1C;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL func_ov167_0219af50(BtlMainModule *mainModule, s32 *state) {
    BtlSetup *setup = mainModule->setup;
    u32 i;
    u8 index;

    if (func_ov167_021b9a70()) {
        mainModule->unk473_0 = 1;
        return TRUE;
    }
    switch (*state) {
    case 0:
        func_ov167_021ba2f4(2);
        (*state)++;
        break;
    case 1:
        if (func_ov167_021ba318(2)) {
            (*state)++;
        }
        break;
    case 2:
        if (func_ov167_021b9c38(setup->party[0])) {
            (*state)++;
        }
        break;
    case 3:
        if (func_ov167_021b9ccc()) {
            if (mainModule->unk471 == 0) {
                *state = 10;
                break;
            }
            if (mainModule->unk46E) {
                mainModule->unk2C4 =
                    func_ov167_021ba524(PokeParty_GetSaveDataSize(), HEAPID_TAIL(mainModule->heapId));
            }
            mainModule->unk470 = 0;
            (*state)++;
        }
        break;
    case 4:
        index = 1;
        if (mainModule->unk470 != 0) {
            index = 3;
        }
        mainModule->unk472 = index;
        if (mainModule->unk46E) {
            func_ov167_021ba564(mainModule->unk2C4, setup->party[mainModule->unk472], mainModule->unk472);
        }
        func_ov167_021ba2f4(mainModule->unk470 + 6);
        (*state)++;
        break;
    case 5:
        if (func_ov167_021ba318(mainModule->unk470 + 6)) {
            (*state)++;
        }
        break;
    case 6:
        if (!mainModule->unk46E || func_ov167_021b9e80(mainModule->unk2C4)) {
            (*state)++;
        }
        break;
    case 7:
        if (func_ov167_021b9f84(mainModule->unk472)) {
            mainModule->unk470++;
            if (mainModule->unk470 >= mainModule->unk471) {
                if (mainModule->unk46E) {
                    func_ov167_021ba55c(mainModule->unk2C4);
                    mainModule->unk2C4 = NULL;
                }
                *state = 10;
            } else {
                *state = 4;
            }
        }
        break;
    case 10:
        func_ov167_0219ccbc(&mainModule->pokeCons[0], mainModule, FALSE);
        for (i = 0; i < 4; i++) {
            if (DoesClientExist(mainModule, i)) {
                func_ov167_0219da44(mainModule, i, func_ov167_021b9fa4(i));
                func_ov167_0219df80(mainModule, i, func_ov167_021b9fa4(i));
                func_ov167_0219cd3c(&mainModule->pokeCons[0], mainModule, i);
            }
        }
        (*state)++;
        break;
    case 11:
        func_ov167_0219ccbc(&mainModule->pokeCons[1], mainModule, TRUE);
        for (i = 0; i < 4; i++) {
            if (DoesClientExist(mainModule, i)) {
                func_ov167_0219cd3c(&mainModule->pokeCons[1], mainModule, i);
            }
        }
        (*state)++;
        break;
    case 12:
        func_ov167_021ba000();
        (*state)++;
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov167_0219b160(BtlMainModule *mainModule, s32 *state) {
    BtlSetup *setup = mainModule->setup;
    u32 i;
    PlayerInfo *info;
    MsgData *msgData;
    StrBuf *strBuf;
    const u16 *str;
    u32 j;
    u16 name[16];
    u8 index;

    if (func_ov167_021b9a70()) {
        mainModule->unk473_0 = 1;
        return TRUE;
    }
    switch (*state) {
    case 0:
        func_ov167_021ba2f4(4);
        (*state)++;
        break;
    case 1:
        if (func_ov167_021ba318(4)) {
            (*state)++;
        }
        break;
    case 2:
        if (func_ov167_021ba008(mainModule->unkC0)) {
            (*state)++;
        }
        break;
    case 3:
        if (func_ov167_021ba098()) {
            for (i = 0; i < mainModule->clientCount; i++) {
                info = func_ov167_021ba0cc(i);
                if (info != NULL) {
                    func_ov167_0219d72c(&mainModule->trainers[i], mainModule->heapId, info);
                    if (i != mainModule->playerClientId && setup->unkDD_2) {
                        j = 0;
                        msgData = GFL_MsgSysLoadData(0, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_HANDLERS_BTL_MAIN, mainModule->heapId);
                        strBuf = GFL_MsgDataLoadStrbufNew(msgData, AbilityHandlersBtlMain_Text_Kuro);
                        GFL_MsgDataFree(msgData);
                        str = GFL_StrBufGetStringPtr(strBuf);
                        for (; j < 15 && j < GFL_StrBufGetCharCount(strBuf); j++) {
                            name[j] = str[j];
                        }
                        name[j] = GFL_StrBufGetTerminator();
                        GFL_StrBufFree(strBuf);
                        copyTrainerName(mainModule->trainers[i].playerInfo, name);
                        GFL_StrBufFree(mainModule->trainers[i].name);
                        msgData = GFL_MsgSysLoadData(0, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_HANDLERS_BTL_MAIN, mainModule->heapId);
                        mainModule->trainers[i].name = GFL_MsgDataLoadStrbufNew(msgData, AbilityHandlersBtlMain_Text_Kuro);
                        GFL_MsgDataFree(msgData);
                    }
                    func_ov167_0219dfa8(mainModule, i, mainModule->trainers[i].playerInfo);
                }
            }
            (*state)++;
        }
        break;
    case 4:
        if (mainModule->unk471 == 0) {
            *state = 10;
            break;
        }
        mainModule->unk470 = 0;
        (*state)++;
        break;
    case 5:
        func_ov167_021ba2f4(mainModule->unk470 + 8);
        (*state)++;
        break;
    case 6:
        if (func_ov167_021ba318(mainModule->unk470 + 8)) {
            (*state)++;
        }
        break;
    case 7:
        index = 1;
        if (mainModule->unk470 != 0) {
            index = 3;
        }
        mainModule->unk472 = index;
        if (mainModule->unk46E && !func_ov167_021ba108(setup->trainers[mainModule->unk472])) {
            break;
        }
        (*state)++;
    case 8:
        if (func_ov167_021ba1b0()) {
            func_ov167_0219d808(&mainModule->trainers[mainModule->unk472], func_ov167_021ba1cc());
            func_ov167_021ba1e0();
            mainModule->unk470++;
            if (mainModule->unk470 >= mainModule->unk471) {
                *state = 10;
            } else {
                *state = 5;
            }
        }
        break;
    case 10:
        func_ov167_021ba204();
        (*state)++;
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov167_0219b3a8(BtlMainModule *mainModule, s32 *state) {
    BtlSetup *setup = mainModule->setup;
    u32 i;
    void *chatter;

    if (func_ov167_021b9a70()) {
        mainModule->unk473_0 = 1;
        return TRUE;
    }
    switch (*state) {
    case 0:
        func_ov167_021b9d00();
        func_ov167_021ba2f4(3);
        (*state)++;
        break;
    case 1:
        if (func_ov167_021ba318(3)) {
            (*state)++;
        }
        break;
    case 2:
        if (func_ov167_021b9d0c(getChatterBlockAddress(GameData_GetSaveControl(setup->gameData)))) {
            (*state)++;
        }
        break;
    case 3:
        if (func_ov167_021b9dfc()) {
            for (i = 0; i < 4; i++) {
                if (DoesClientExist(mainModule, i) && !func_ov167_0219d888(mainModule, i)) {
                    chatter = func_ov167_021b9e48(i);
                    if (chatter != NULL) {
                        mainModule->unk3E0[i] = allocChatotChatterBlk(mainModule->heapId);
                        sys_memcpy(chatter, mainModule->unk3E0[i], func_02007e20());
                    }
                }
                func_ov167_0219dfd0(mainModule, i);
            }
            func_ov167_021b9e74();
            (*state)++;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov167_0219b4ac(BtlMainModule *mainModule, s32 *state) {
    BtlSetup *setup = mainModule->setup;
    u8 clientId = mainModule->playerClientId;
    u32 unk = func_ov167_0219a004(setup);
    u32 opponent;
    u32 i;

    if (func_ov167_021b9a70()) {
        mainModule->unk473_0 = 1;
        return TRUE;
    }
    if (mainModule->unk46E) {
        mainModule->server =
            func_ov167_0219e3cc(mainModule, &mainModule->rand, &mainModule->pokeCons[1], unk, mainModule->heapId);
        mainModule->clients[clientId] =
            BattleClient_Create(mainModule, &mainModule->pokeCons[0], setup->fieldSituation.unk18,
                                setup->fieldSituation.netHandle, clientId, 1, 0, unk, 0, &mainModule->rand, mainModule->heapId);
        opponent = 1;
        func_ov167_0219e498(mainModule->server, func_ov167_021b1928(mainModule->clients[clientId]), clientId, 1);
        if (clientId != 0) {
            opponent = 0;
        }
        func_ov167_0219e4d0(mainModule->server, setup->fieldSituation.unk18, setup->fieldSituation.netHandle, opponent, 1);
    } else {
        mainModule->clients[clientId] =
            BattleClient_Create(mainModule, &mainModule->pokeCons[0], setup->fieldSituation.unk18,
                                setup->fieldSituation.netHandle, clientId, 1, 0, unk, 0, &mainModule->rand, mainModule->heapId);
        mainModule->unk0C =
            func_ov167_0219e3cc(mainModule, &mainModule->rand, &mainModule->pokeCons[1], unk, mainModule->heapId);
        for (i = 0; i < 4; i++) {
            if (DoesClientExist(mainModule, i)) {
                func_ov167_0219e514(mainModule->unk0C, i, 1);
            }
        }
        func_ov167_021b1910(mainModule->clients[clientId], mainModule->unk0C);
    }
    return TRUE;
}

BOOL func_ov167_0219b610(BtlMainModule *mainModule, s32 *state) {
    BtlSetup *setup = mainModule->setup;
    u8 clientId = mainModule->playerClientId;
    u32 unk = func_ov167_0219a004(setup);
    u8 posCount = setup->fieldSituation.unk1a == 0 ? 2 : 1;
    u8 ally;
    u32 i;

    if (func_ov167_021b9a70()) {
        mainModule->unk473_0 = 1;
        return TRUE;
    }
    if (mainModule->unk46E) {
        mainModule->server =
            func_ov167_0219e3cc(mainModule, &mainModule->rand, &mainModule->pokeCons[1], unk, mainModule->heapId);
        mainModule->clients[clientId] = BattleClient_Create(mainModule, &mainModule->pokeCons[0],
                                                            setup->fieldSituation.unk18, setup->fieldSituation.netHandle,
                                                            clientId, posCount, 0, unk, 0, &mainModule->rand,
                                                            mainModule->heapId);
        func_ov167_0219e498(mainModule->server, func_ov167_021b1928(mainModule->clients[clientId]), clientId, posCount);
        if (setup->fieldSituation.unk1a == 2) {
            ally = func_ov167_0219c87c(mainModule, clientId);
            mainModule->clients[1] = BattleClient_Create(mainModule, &mainModule->pokeCons[0],
                                                         setup->fieldSituation.unk18, setup->fieldSituation.netHandle, 1,
                                                         posCount, 1, unk, 0, &mainModule->rand, mainModule->heapId);
            func_ov167_0219e498(mainModule->server, func_ov167_021b1928(mainModule->clients[1]), 1, posCount);
            mainModule->clients[3] = BattleClient_Create(mainModule, &mainModule->pokeCons[0],
                                                         setup->fieldSituation.unk18, setup->fieldSituation.netHandle, 3,
                                                         posCount, 1, unk, 0, &mainModule->rand, mainModule->heapId);
            func_ov167_0219e498(mainModule->server, func_ov167_021b1928(mainModule->clients[3]), 3, posCount);
            func_ov167_0219e4d0(mainModule->server, setup->fieldSituation.unk18, setup->fieldSituation.netHandle, ally,
                                posCount);
        } else {
            for (i = 0; i < mainModule->clientCount; i++) {
                if (i != clientId) {
                    func_ov167_0219e4d0(mainModule->server, setup->fieldSituation.unk18, setup->fieldSituation.netHandle,
                                        i, posCount);
                }
            }
        }
    } else {
        mainModule->clients[clientId] = BattleClient_Create(mainModule, &mainModule->pokeCons[0],
                                                            setup->fieldSituation.unk18, setup->fieldSituation.netHandle,
                                                            clientId, posCount, 0, unk, 0, &mainModule->rand,
                                                            mainModule->heapId);
        mainModule->unk0C =
            func_ov167_0219e3cc(mainModule, &mainModule->rand, &mainModule->pokeCons[1], unk, mainModule->heapId);
        for (i = 0; i < 4; i++) {
            if (DoesClientExist(mainModule, i)) {
                func_ov167_0219e514(mainModule->unk0C, i, func_ov167_0219a180(mainModule, i));
            }
        }
        func_ov167_021b1910(mainModule->clients[clientId], mainModule->unk0C);
    }
    return TRUE;
}

BOOL func_ov167_0219b868(BtlMainModule *mainModule, s32 *state) {
    BtlSetup *setup = mainModule->setup;
    u8 clientId = mainModule->playerClientId;
    u32 unk = func_ov167_0219a004(setup);
    u32 opponent;
    u32 i;

    if (func_ov167_021b9a70()) {
        mainModule->unk473_0 = 1;
        return TRUE;
    }
    if (mainModule->unk46E) {
        mainModule->server =
            func_ov167_0219e3cc(mainModule, &mainModule->rand, &mainModule->pokeCons[1], unk, mainModule->heapId);
        mainModule->clients[clientId] =
            BattleClient_Create(mainModule, &mainModule->pokeCons[0], setup->fieldSituation.unk18,
                                setup->fieldSituation.netHandle, clientId, 3, 0, unk, 0, &mainModule->rand, mainModule->heapId);
        opponent = 0;
        func_ov167_0219e498(mainModule->server, func_ov167_021b1928(mainModule->clients[clientId]), clientId, 3);
        if (clientId == 0) {
            opponent = 1;
        }
        func_ov167_0219e4d0(mainModule->server, setup->fieldSituation.unk18, setup->fieldSituation.netHandle, opponent, 3);
    } else {
        mainModule->clients[clientId] =
            BattleClient_Create(mainModule, &mainModule->pokeCons[0], setup->fieldSituation.unk18,
                                setup->fieldSituation.netHandle, clientId, 3, 0, unk, 0, &mainModule->rand, mainModule->heapId);
        mainModule->unk0C =
            func_ov167_0219e3cc(mainModule, &mainModule->rand, &mainModule->pokeCons[1], unk, mainModule->heapId);
        for (i = 0; i < 4; i++) {
            if (DoesClientExist(mainModule, i)) {
                func_ov167_0219e514(mainModule->unk0C, i, func_ov167_0219a180(mainModule, i));
            }
        }
        func_ov167_021b1910(mainModule->clients[clientId], mainModule->unk0C);
    }
    return TRUE;
}

BOOL func_ov167_0219b9d4(BtlMainModule *mainModule, s32 *state) {
    BtlSetup *setup = mainModule->setup;
    u8 clientId = mainModule->playerClientId;
    u32 unk = func_ov167_0219a004(setup);
    u32 i;

    if (func_ov167_021b9a70()) {
        mainModule->unk473_0 = 1;
        return TRUE;
    }
    if (mainModule->unk46E) {
        mainModule->server =
            func_ov167_0219e3cc(mainModule, &mainModule->rand, &mainModule->pokeCons[1], unk, mainModule->heapId);
        i = 0;
        mainModule->clients[clientId] =
            BattleClient_Create(mainModule, &mainModule->pokeCons[0], setup->fieldSituation.unk18,
                                setup->fieldSituation.netHandle, clientId, 1, i, unk, i, &mainModule->rand, mainModule->heapId);
        func_ov167_0219e498(mainModule->server, func_ov167_021b1928(mainModule->clients[clientId]), clientId, 1);
        for (; i < mainModule->clientCount; i++) {
            if (i != clientId) {
                func_ov167_0219e4d0(mainModule->server, setup->fieldSituation.unk18, setup->fieldSituation.netHandle, i,
                                    1);
            }
        }
    } else {
        i = 0;
        mainModule->clients[clientId] =
            BattleClient_Create(mainModule, &mainModule->pokeCons[0], setup->fieldSituation.unk18,
                                setup->fieldSituation.netHandle, clientId, 1, i, unk, i, &mainModule->rand, mainModule->heapId);
        mainModule->unk0C =
            func_ov167_0219e3cc(mainModule, &mainModule->rand, &mainModule->pokeCons[1], unk, mainModule->heapId);
        for (; i < 4; i++) {
            if (DoesClientExist(mainModule, i)) {
                func_ov167_0219e514(mainModule->unk0C, i, func_ov167_0219a180(mainModule, i));
            }
        }
        func_ov167_021b1910(mainModule->clients[clientId], mainModule->unk0C);
    }
    return TRUE;
}

BOOL func_ov167_0219bb54(BtlMainModule *mainModule, s32 *state) {
    if (func_ov167_021b9a70()) {
        mainModule->unk473_0 = 1;
        return TRUE;
    }
    switch (*state) {
    case 0:
        mainModule->viewCore = BtlvCore_Create(mainModule, mainModule->clients[mainModule->playerClientId],
                                               &mainModule->pokeCons[0], func_ov167_0219a004(mainModule->setup), 0x13);
        func_ov167_021b190c(mainModule->clients[mainModule->playerClientId], mainModule->viewCore);
        if (mainModule->unk46E) {
            mainModule->mainFunc = func_ov167_0219bc2c;
            func_ov167_0219e544(mainModule->server);
        } else {
            mainModule->mainFunc = func_ov167_0219bcd0;
        }
        (*state)++;
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov167_0219bbec(BtlMainModule *mainModule) {
    BOOL done = FALSE;
    s32 i;

    func_ov167_0219e5a0(mainModule->server);
    for (i = 0; i < 4; i++) {
        if (mainModule->clients[i] != NULL && func_ov167_021b192c(mainModule->clients[i])) {
            func_ov167_021b1d58(mainModule->clients[i], &mainModule->unk448);
            done = TRUE;
        }
    }
    func_ov167_021ce8c8(mainModule->viewCore);
    return done;
}

BOOL func_ov167_0219bc2c(BtlMainModule *mainModule) {
    BOOL done;
    s32 i;

    if (!mainModule->unk473_0) {
        if (func_ov167_021b9a70()) {
            mainModule->unk473_0 = 1;
            func_ov167_0219bd40(mainModule);
        }
    } else if (mainModule->unk473_1) {
        return TRUE;
    }
    done = func_ov167_0219e5a0(mainModule->server);
    if (done) {
        func_ov167_0219e5e0(mainModule->server, &mainModule->unk448);
    }
    for (i = 0; i < 4; i++) {
        if (mainModule->clients[i] != NULL && mainModule->unkC4[i] == 0) {
            mainModule->unkC4[i] = func_ov167_021b192c(mainModule->clients[i]);
        }
    }
    if (func_ov167_0219bd5c(mainModule)) {
        func_ov167_021b1d58(mainModule->clients[mainModule->playerClientId], &mainModule->unk448);
        done = TRUE;
    }
    func_ov167_021ce8c8(mainModule->viewCore);
    return done;
}

BOOL func_ov167_0219bcd0(BtlMainModule *mainModule) {
    BOOL done;
    s32 i;

    if (!mainModule->unk473_0) {
        if (func_ov167_021b9a70()) {
            mainModule->unk473_0 = 1;
            func_ov167_0219bd40(mainModule);
        }
    } else if (mainModule->unk473_1) {
        return TRUE;
    }
    done = FALSE;
    for (i = 0; i < 4; i++) {
        if (mainModule->clients[i] != NULL && func_ov167_021b192c(mainModule->clients[i])) {
            func_ov167_021b1d58(mainModule->clients[i], &mainModule->unk448);
            done = TRUE;
        }
    }
    func_ov167_021ce8c8(mainModule->viewCore);
    return done;
}

void func_ov167_0219bd40(BtlMainModule *mainModule) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (mainModule->clients[i] != NULL) {
            func_ov167_021b18c4(mainModule->clients[i]);
        }
    }
}

BOOL func_ov167_0219bd5c(BtlMainModule *mainModule) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (mainModule->clients[i] != NULL && mainModule->unkC4[i] == 0) {
            return FALSE;
        }
    }
    return TRUE;
}

// Layout reconstructed from the opponent-position lookup in the game code.
u32 BtlSetup_GetBattleStyle(BtlMainModule *mainModule) {
    return mainModule->setup->battleStyle;
}

u32 func_ov167_0219bd88(BtlMainModule *mainModule) {
    return mainModule->unk473_2;
}

u8 func_ov167_0219bd98(BtlMainModule *mainModule) {
    return mainModule->setup->unk98;
}

BOOL IsSwitchMode(BtlMainModule *mainModule) {
    if (BtlSetup_GetBattleType(mainModule) == 1 && BtlSetup_GetBattleStyle(mainModule) == 0 &&
        func_ov167_0219bee4(mainModule) == 0 && func_ov167_0219c988(mainModule) == 0 &&
        func_02008a68(mainModule->setup->config) == 0) {
        return TRUE;
    }
    return FALSE;
}

s32 func_ov167_0219bde0(BtlMainModule *mainModule) {
    return func_02017c50(mainModule->unk43C);
}

void func_ov167_0219bdf0(BtlMainModule *mainModule) {
    mainModule->unk43C = 3;
}

BOOL func_ov167_0219bdfc(BtlMainModule *mainModule) {
    BOOL result;

    if (mainModule->setup->battleType <= 1) {
        GFL_OvlLoad(OVERLAY_ID(338));
        result = IrCheck_IsGenuineCard();
        GFL_OvlUnload(OVERLAY_ID(338));
        if (!result) {
            return FALSE;
        }
        if (!BtlSetup_IsBattleType(mainModule, 0x200) && !func_ov167_0219c988(mainModule)) {
            return TRUE;
        }
    }
    return FALSE;
}

void *func_ov167_0219be48(BtlMainModule *mainModule) {
    if (mainModule->setup->battleType <= 1) {
        return mainModule->setup->unk88;
    }
    return NULL;
}

u32 GetValidPosMax(BtlMainModule *mainModule) {
    switch (mainModule->setup->battleStyle) {
    case 0:
        return 1;
    case 1:
        return 3;
    case 2:
        return 5;
    case 3:
        return 5;
    default:
        return 5;
    }
}

u32 func_ov167_0219be8c(BtlMainModule *mainModule) {
    switch (mainModule->setup->battleStyle) {
    case 0:
        return 1;
    case 1:
        return 2;
    case 2:
        return 3;
    case 3:
        return 1;
    default:
        return 1;
    }
}

BOOL func_ov167_0219bebc(BtlMainModule *mainModule, u32 pos) {
    return pos < func_ov167_0219be8c(mainModule) * 2;
}

u32 BtlSetup_GetBattleType(BtlMainModule *mainModule) {
    return mainModule->setup->battleType;
}

u8 func_ov167_0219bedc(BtlMainModule *mainModule) {
    return mainModule->setup->fieldSituation.unk1a;
}

u8 func_ov167_0219bee4(BtlMainModule *mainModule) {
    return mainModule->setup->fieldSituation.unk18;
}

BOOL func_ov167_0219beec(BtlMainModule *mainModule) {
    return mainModule->setup->fieldSituation.unk1a != 0;
}

u16 func_ov167_0219bf00(BtlMainModule *mainModule) {
    return mainModule->setup->fieldSituation.unk12;
}

u16 func_ov167_0219bf08(BtlMainModule *mainModule) {
    return mainModule->setup->unk138;
}

u16 func_ov167_0219bf14(BtlMainModule *mainModule) {
    return mainModule->setup->unk13a;
}

u32 GetRunMode(BtlMainModule *mainModule) {
    switch (mainModule->setup->battleType) {
    case 0:
        return 0;
    case 1:
        if (func_ov167_0219c988(mainModule) == 1) {
            return 2;
        }
        return 1;
    case 2:
        return 2;
    case 3:
        return 2;
    default:
        return 1;
    }
}

BtlFieldSituation *GetFieldEffectData(BtlMainModule *mainModule) {
    return &mainModule->setup->fieldSituation;
}

PlayerInfo *func_ov167_0219bf68(BtlMainModule *mainModule) {
    return mainModule->unkC0;
}

BOOL func_ov167_0219bf70(BtlMainModule *mainModule, BattleMon *mon) {
    return PokeDex_IsCaught(mainModule->setup->pokedex, GetBattleMonSpecies(mon));
}

u32 func_ov167_0219bf88(BtlMainModule *mainModule) {
    return PokeDex_GetCaughtNoNational(mainModule->setup->pokedex);
}

GameData *func_ov167_0219bf98(BtlMainModule *mainModule) {
    return mainModule->setup->gameData;
}

void func_ov167_0219bfa0(BtlMainModule *mainModule, u8 clientId, BattleMon *mon) {
    u32 battleType = BtlSetup_GetBattleType(mainModule);
    u32 unk = func_ov167_0219c988(mainModule);

    if (mainModule->setup->unkDE_0 != 1 && battleType <= 1 && unk == 0 && clientId != 0) {
        PokeDex_RegistPkm(mainModule->setup->pokedex, GetSrcData(mon));
    }
}

u8 func_ov167_0219bfe4(BtlMainModule *mainModule, u16 posFlags, u8 *out) {
    u8 type = posFlags >> 8;
    u8 pos = posFlags;

    if (type == 0) {
        *out = pos;
        return *out != 6 ? 1 : 0;
    }
    if (pos != 6) {
        switch (mainModule->setup->battleStyle) {
        case 0:
        default:
            return func_ov167_0219c054(mainModule, type, pos, out);
        case 1:
            return func_ov167_0219c0a0(mainModule, type, pos, out);
        case 2:
            return func_ov167_0219c158(mainModule, type, pos, out);
        case 3:
            return func_ov167_0219c054(mainModule, type, pos, out);
        }
    }
    *out = 6;
    return 0;
}

u8 func_ov167_0219c054(BtlMainModule *mainModule, u8 type, u8 pos, u8 *out) {
    switch (type) {
    default:
        return 0;
    case 1:
    case 2:
    case 6:
        *out = func_ov167_0219c4bc(mainModule, pos, 0);
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

u8 func_ov167_0219c0a0(BtlMainModule *mainModule, u8 type, u8 pos, u8 *out) {
    switch (type) {
    default:
        return 0;
    case 1:
    case 6:
        out[0] = func_ov167_0219c4bc(mainModule, pos, 0);
        out[1] = func_ov167_0219c4bc(mainModule, pos, 1);
        return 2;
    case 2:
        out[0] = func_ov167_0219c4bc(mainModule, pos, 0);
        out[1] = func_ov167_0219c4bc(mainModule, pos, 1);
        out[2] = func_ov167_0219c51c(mainModule, pos);
        return 3;
    case 3:
        out[0] = pos;
        out[1] = func_ov167_0219c51c(mainModule, pos);
        if (out[0] > out[1]) {
            u8 tmp = out[0];

            out[0] = out[1];
            out[1] = tmp;
        }
        return 2;
    case 4:
    case 9:
        out[0] = func_ov167_0219c51c(mainModule, pos);
        return 1;
    case 7:
        out[0] = pos;
        out[1] = func_ov167_0219c51c(mainModule, pos);
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

u8 func_ov167_0219c158(BtlMainModule *mainModule, u8 type, u8 pos, u8 *out) {
    u8 isCenter = func_ov167_0219d2cc(pos);
    u8 index;

    switch (type) {
    default:
    case 0:
    case 1:
        if (isCenter) {
            out[0] = func_ov167_0219c4bc(mainModule, pos, 0);
            out[1] = func_ov167_0219c4bc(mainModule, pos, 1);
            out[2] = func_ov167_0219c4bc(mainModule, pos, 2);
            return 3;
        }
        out[0] = func_ov167_0219c4bc(mainModule, pos, 1);
        index = pos >> 1;
        if (index == 0) {
            index = 2;
        } else if (index == 2) {
            index = 0;
        }
        out[1] = func_ov167_0219c48c(2, pos, index);
        return 2;
    case 6:
        out[0] = func_ov167_0219c4bc(mainModule, pos, 0);
        out[1] = func_ov167_0219c4bc(mainModule, pos, 1);
        out[2] = func_ov167_0219c4bc(mainModule, pos, 2);
        return 3;
    case 7:
        out[0] = GetPosOnSameSide(pos, 0);
        out[1] = GetPosOnSameSide(pos, 1);
        out[2] = GetPosOnSameSide(pos, 2);
        return 3;
    case 8:
        out[0] = func_ov167_0219c4bc(mainModule, pos, 0);
        out[1] = func_ov167_0219c4bc(mainModule, pos, 1);
        out[2] = func_ov167_0219c4bc(mainModule, pos, 2);
        out[3] = GetPosOnSameSide(pos, 0);
        out[4] = GetPosOnSameSide(pos, 1);
        out[5] = GetPosOnSameSide(pos, 2);
        return 6;
    case 2:
        if (isCenter) {
            out[0] = func_ov167_0219c4bc(mainModule, pos, 0);
            out[1] = func_ov167_0219c4bc(mainModule, pos, 1);
            out[2] = func_ov167_0219c4bc(mainModule, pos, 2);
            out[3] = GetPosOnSameSide(pos, 0);
            out[4] = GetPosOnSameSide(pos, 2);
            return 5;
        }
        out[0] = func_ov167_0219c4bc(mainModule, pos, 1);
        index = pos >> 1;
        if (index == 0) {
            index = 2;
        } else if (index == 2) {
            index = 0;
        }
        out[1] = func_ov167_0219c48c(2, pos, index);
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
            out[0] = func_ov167_0219c4bc(mainModule, pos, 0);
            out[1] = func_ov167_0219c4bc(mainModule, pos, 1);
            out[2] = func_ov167_0219c4bc(mainModule, pos, 2);
            out[3] = GetPosOnSameSide(pos, 0);
            out[4] = GetPosOnSameSide(pos, 2);
            out[5] = pos;
            return 6;
        }
        out[0] = func_ov167_0219c4bc(mainModule, pos, 1);
        index = pos >> 1;
        if (index == 0) {
            index = 2;
        } else if (index == 2) {
            index = 0;
        }
        out[1] = func_ov167_0219c48c(2, pos, index);
        out[2] = GetPosOnSameSide(pos, 1);
        out[3] = pos;
        return 4;
    }
}

u32 func_ov167_0219c3e4(BtlMainModule *mainModule, u8 clientId) {
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

BOOL func_ov167_0219c43c(BtlMainModule *mainModule, u8 side) {
    if (side == GetClientSide(mainModule, mainModule->playerClientId)) {
        return TRUE;
    }
    return FALSE;
}

u8 func_ov167_0219c458(BtlMainModule *mainModule, u8 clientId, u8 slot) {
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

u8 func_ov167_0219c48c(u32 battleStyle, u8 pos, u8 index) {
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

u8 func_ov167_0219c4bc(BtlMainModule *mainModule, u8 pos, u8 index) {
    return func_ov167_0219c48c(mainModule->setup->battleStyle, pos, index);
}

u8 func_ov167_0219c4c8(u32 battleStyle, u8 pos) {
    u8 index;

    switch (battleStyle) {
    default:
    case 0:
        return func_ov167_0219c48c(battleStyle, pos, 0);
    case 1:
        return func_ov167_0219c48c(battleStyle, pos, (u8)(pos >> 1) ^ 1);
    case 2:
        index = pos >> 1;
        if (index == 0) {
            index = 2;
        } else if (index == 2) {
            index = 0;
        }
        return func_ov167_0219c48c(battleStyle, pos, index);
    case 3:
        return func_ov167_0219c48c(battleStyle, pos, 0);
    }
}

u8 func_ov167_0219c51c(BtlMainModule *mainModule, u8 pos) {
    switch (mainModule->setup->battleStyle) {
    case 0:
        return pos;
    case 3:
        return pos;
    case 1:
        return (pos + 2) & 3;
    case 2:
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
        first = data_ov167_021d6c24[i];
        last = first + 5;
        if (monId >= first && monId <= last) {
            return i;
        }
    }
    return 0;
}

u8 func_ov167_0219c5a4(BtlMainModule *mainModule, BtlPokeCon *pokeCon, u16 posFlags, u8 *monIds) {
    u8 positions[8];
    u8 count;
    u8 found;
    u8 i;
    BattleMon *mon;

    count = func_ov167_0219bfe4(mainModule, posFlags, positions);
    for (i = 0, found = 0; i < count; i++) {
        mon = func_ov167_0219d180(pokeCon, positions[i]);
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
    slot = func_ov167_0219d140(pokeCon, clientId, monId);
    if (slot >= 0) {
        pos = func_ov167_0219c458(mainModule, clientId, slot);
        if (pos != 6) {
            return pos;
        }
    }
    return 6;
}

u8 func_ov167_0219c62c(BtlMainModule *mainModule, BtlPokeCon *pokeCon, u8 monId) {
    u8 pos = MonIDToBattlePos(mainModule, pokeCon, monId);

    if (pos != BTL_POS_MAX) {
        return func_ov167_0219c6dc(mainModule, pos);
    }
    return 0xff;
}

u8 func_ov167_0219c648(u8 monId) {
    return MonIDToClientID(monId);
}

u8 func_ov167_0219c650(BtlMainModule *mainModule, u8 pos) {
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

u8 func_ov167_0219c658(BtlMainModule *mainModule, u8 pos) {
    return CountClientPosBefore(mainModule, pos);
}

void func_ov167_0219c694(BtlMainModule *mainModule, u8 pos, u8 *clientId, u8 *index) {
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

u8 func_ov167_0219c6dc(BtlMainModule *mainModule, u8 pos) {
    u32 sameSide = TRUE;
    u8 side;
    u32 battleStyle;

    if ((mainModule->unk46D & 1) != (pos & 1)) {
        sameSide = FALSE;
    }
    side = sameSide;
    battleStyle = mainModule->setup->battleStyle;
    if (battleStyle == 0) {
        u32 result = 0;

        if (side == 0) {
            result = 1;
        }
        return result;
    }
    if (battleStyle == 1) {
        return data_ov167_021d6c28[side][(u8)(pos >> 1)];
    }
    return data_ov167_021d6c2e[side][(u8)(pos >> 1)];
}

u8 func_ov167_0219c744(BtlMainModule *mainModule, u8 index) {
    u8 isEnemy = index & 1;
    u8 playerSide;

    if (mainModule->setup->battleStyle == 0) {
        if (isEnemy) {
            return mainModule->unk46D == 0 ? 1 : 0;
        }
        return mainModule->unk46D;
    }
    playerSide = mainModule->unk46D & 1;
    if (isEnemy) {
        return (playerSide ^ 1) + (index - 3);
    }
    return playerSide + (index - 2);
}

BOOL func_ov167_0219c7a4(BtlMainModule *mainModule) {
    if (func_ov167_0219beec(mainModule)) {
        u32 battleType = BtlSetup_GetBattleType(mainModule);

        if (battleType != 1 && battleType != 0) {
            return TRUE;
        }
    }
    return FALSE;
}

PokeParty *func_ov167_0219c7c8(BtlMainModule *mainModule, u8 clientId) {
    func_ov167_0219cf78(&mainModule->pokeCons[0], mainModule, clientId, FALSE);
    return func_ov167_0219d138(&mainModule->pokeCons[0], clientId);
}

PokeParty *func_ov167_0219c7e8(BtlMainModule *mainModule, u8 clientId) {
    if (func_ov167_0219beec(mainModule)) {
        clientId = (clientId + 2) & 3;
        if (DoesClientExist(mainModule, clientId)) {
            func_ov167_0219cf78(&mainModule->pokeCons[0], mainModule, clientId, FALSE);
            return func_ov167_0219d138(&mainModule->pokeCons[0], clientId);
        }
    }
    return NULL;
}

u8 func_ov167_0219c82c(BtlMainModule *mainModule, u8 clientId) {
    if (func_ov167_0219beec(mainModule)) {
        return (clientId & 2) ? 1 : 0;
    }
    return 0;
}

u8 func_ov167_0219c850(BtlMainModule *mainModule) {
    return func_ov167_0219c82c(mainModule, mainModule->playerClientId);
}

u8 GetPlayerClientID(BtlMainModule *mainModule) {
    return mainModule->playerClientId;
}

u8 func_ov167_0219c86c(BtlMainModule *mainModule) {
    return func_ov167_0219c87c(mainModule, mainModule->playerClientId);
}

u8 func_ov167_0219c87c(BtlMainModule *mainModule, u8 clientId) {
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

u8 func_ov167_0219c8b8(BtlMainModule *mainModule, u8 other) {
    return func_ov167_0219c8d0(mainModule, mainModule->playerClientId, other);
}

u32 func_ov167_0219c8d0(BtlMainModule *mainModule, u8 clientId, u8 other) {
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

BagSave *func_ov167_0219c954(BtlMainModule *mainModule) {
    return mainModule->setup->bag;
}

void *func_ov167_0219c95c(BtlMainModule *mainModule) {
    return mainModule->setup->unk7C;
}

BOOL func_ov167_0219c964(BtlMainModule *mainModule) {
    BtlSetup *setup = mainModule->setup;

    if (setup->unkB0 != 0 && setup->fieldSituation.unk1b == 0) {
        return TRUE;
    }
    return FALSE;
}

u8 func_ov167_0219c980(BtlMainModule *mainModule) {
    return mainModule->setup->fieldSituation.unk1b;
}

u32 func_ov167_0219c988(BtlMainModule *mainModule) {
    if (mainModule != NULL) {
        return mainModule->setup->unkDD_3;
    }
    return 0;
}

u32 func_ov167_0219c99c(BtlMainModule *mainModule) {
    if (mainModule != NULL) {
        return mainModule->setup->unkDD_6;
    }
    return 0;
}

u8 func_ov167_0219c9b0(BtlMainModule *mainModule) {
    if (mainModule != NULL) {
        return mainModule->setup->unkDF;
    }
    return 0;
}

u32 func_ov167_0219c9c0(BtlMainModule *mainModule) {
    if (mainModule != NULL) {
        return mainModule->setup->unkDD_7;
    }
    return 0;
}

void func_ov167_0219c9d4(BtlMainModule *mainModule) {
    mainModule->setup->unk134 = 1;
}

u32 func_ov167_0219c9e0(BtlMainModule *mainModule) {
    if (mainModule != NULL) {
        return func_ov167_021ae320(func_ov167_0219f38c(mainModule->server));
    }
    return 0;
}

u32 ReturnZero(BtlMainModule *mainModule, u32 flag) {
    return 0;
}

void func_ov167_0219c9fc(BtlMainModule *mainModule, u8 pos) {
    if (mainModule->setup->unkAC == 6) {
        mainModule->setup->unkAC = CountClientPosBefore(mainModule, pos);
    }
}

void func_ov167_0219ca48(BtlMainModule *mainModule, u32 result) {
    if (mainModule->result == 7) {
        mainModule->result = result;
    }
}

u32 func_ov167_0219ca58(BtlMainModule *mainModule) {
    return func_ov167_0219de84(mainModule);
}

void func_ov167_0219ca60(BtlMainModule *mainModule) {
    mainModule->setup->unkDD_0 = 1;
}

u32 func_ov167_0219ca78(BtlMainModule *mainModule) {
    if (!mainModule->unk473_3) {
        if (mainModule->unk473_6) {
            mainModule->prizeMoney *= 2;
        }
        mainModule->prizeMoney = PassPower_ApplyPrizeMoney(mainModule->prizeMoney);
        mainModule->unk473_3 = 1;
    }
    return mainModule->prizeMoney;
}

void func_ov167_0219cac0(BtlMainModule *mainModule, u32 money) {
    if (!mainModule->unk473_3) {
        mainModule->money += money;
        if (mainModule->money > 99999) {
            mainModule->money = 99999;
        }
    }
}

u32 func_ov167_0219caec(BtlMainModule *mainModule) {
    u32 money = mainModule->money;

    if (mainModule->unk473_6) {
        money *= 2;
        if (money > 99999) {
            money = 99999;
        }
    }
    return money;
}

void func_ov167_0219cb10(BtlMainModule *mainModule) {
    mainModule->unk473_6 = 1;
}

u32 func_ov167_0219cb20(BtlMainModule *mainModule) {
    u32 loss = 0;
    u32 cash;

    if (!mainModule->unk473_4) {
        if (mainModule->setup->battleType <= 1) {
            cash = getCash(getTrainerCardDataBlkAddress(mainModule->setup->gameData));
            loss = func_ov167_021bd820(mainModule->setup->unk98,
                                       GetClientParty(&mainModule->pokeCons[1], mainModule->playerClientId));
            if (loss > cash) {
                loss = cash;
            }
            mainModule->unk438 = loss;
        }
        mainModule->unk473_4 = 1;
    }
    return loss;
}

void func_ov167_0219cb7c(BtlMainModule *mainModule) {
    mainModule->unk473_5 = 1;
}

void ChangeFriendshipWhenFainted(BtlMainModule *mainModule, BattleMon *mon, BOOL reason) {
    if (mainModule->setup->battleType <= 1 && mainModule->setup->fieldSituation.unk1b == 0) {
        ChangeFriendship(mainModule, mon, reason ? 5 : 4);
    }
}

void ChangeFriendship(BtlMainModule *mainModule, BattleMon *mon, u32 reason) {
    u8 monId;
    const BattleMon *param1;
    const BattleMon *param2;
    PartyPkm *src1;
    PartyPkm *src2;
    const BtlFieldSituation *field;

    monId = GetMonID(mon);
    param1 = GetPokeParamConst(&mainModule->pokeCons[1], monId);
    param2 = GetPokeParamConst(&mainModule->pokeCons[0], monId);
    src1 = GetSrcData(param1);
    src2 = GetSrcData(param2);
    field = GetFieldEffectData(mainModule);
    FriendshipManagerCalc(src1, reason, field->env.zoneId, HEAPID_TAIL(mainModule->heapId));
    FriendshipManagerCalc(src2, reason, field->env.zoneId, HEAPID_TAIL(mainModule->heapId));
}

void func_ov167_0219cc34(BtlMainModule *mainModule, u8 monId) {
    BattleMon *mon = GetPokeParam(&mainModule->pokeCons[1], monId);

    func_ov167_021bacf4(GetPokeParam(&mainModule->pokeCons[0], monId), mon);
}

BattleMon *GetIllusionDisguise(BtlMainModule *mainModule, BtlPokeCon *pokeCon, BattleMon *mon) {
    u8 clientId;
    PartyPkm *disguise;
    BattleParty *party;
    u32 count;
    u32 i;
    BattleMon *member;

    if (IsIllusionEnabled(mon)) {
        clientId = func_ov167_0219c648(GetMonID(mon));
        disguise = func_ov167_021bb064(mon);
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

void func_ov167_0219ccbc(BtlPokeCon *pokeCon, BtlMainModule *mainModule, u32 unkE4) {
    u32 i;

    pokeCon->mainModule = mainModule;
    pokeCon->unkE4 = unkE4;
    for (i = 0; i < 4; i++) {
        func_ov167_0219d434(&pokeCon->parties[i]);
    }
    for (i = 0; i < 24; i++) {
        pokeCon->mons[i] = NULL;
    }
    for (i = 0; i < 4; i++) {
        pokeCon->srcParties[i] = NULL;
    }
}

void func_ov167_0219cd00(BtlPokeCon *pokeCon) {
    u32 i;

    for (i = 0; i < 24; i++) {
        if (pokeCon->mons[i] != NULL) {
            func_ov167_021babb8(pokeCon->mons[i]);
            pokeCon->mons[i] = NULL;
        }
    }
    for (i = 0; i < 4; i++) {
        func_ov167_0219d434(&pokeCon->parties[i]);
    }
}

void func_ov167_0219cd3c(BtlPokeCon *pokeCon, BtlMainModule *mainModule, u8 clientId) {
    PokeParty *srcParty;
    BattleParty *battleParty;
    u32 count;
    u8 monId;
    s32 i;
    u8 firstMonId;
    s32 eligible;
    BattleMon *mon;
    PokeParty *party;
    u8 partyCount;
    u8 k;
    u16 species;

    srcParty = func_ov167_0219da94(mainModule, clientId, pokeCon->unkE4);
    battleParty = &pokeCon->parties[clientId];
    count = PokeParty_GetPkmCount(srcParty);
    firstMonId = data_ov167_021d6c24[clientId];
    pokeCon->srcParties[clientId] = srcParty;
    monId = firstMonId;
    for (i = 0; i < count; i++) {
        pokeCon->mons[monId] = BattleMon_Create(PokeParty_GetPkm(srcParty, i), monId, HEAPID_BATTLE);
        AddBattleMonToParty(battleParty, pokeCon->mons[monId]);
        monId++;
    }
    eligible = GetPartyPkmnEligibleForBattle(srcParty);
    if (eligible > 0) {
        monId = firstMonId;
        for (i = 0; i < eligible; i++) {
            if (GetBattleMonStat(pokeCon->mons[monId], 0x11) == ABILITY_ILLUSION) {
                SetIllusionDisguise(pokeCon->mons[monId], PokeParty_GetPkm(srcParty, eligible));
            }
            monId++;
        }
    }
    if (BtlSetup_GetBattleType(mainModule) == 0 && clientId == 1) {
        for (i = 0; i < count; i++) {
            mon = pokeCon->mons[firstMonId];
            if (GetBattleMonSpecies(mon) == SPECIES_ZOROARK && GetBattleMonStat(mon, 0x11) == ABILITY_ILLUSION) {
                species = 0;
                party = func_ov167_0219da94(mainModule, 0, pokeCon->unkE4);
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
                    if (mainModule->unk2C0 == NULL) {
                        mainModule->unk2C0 =
                            PokeParty_NewTempPkm(species, GetBattleMonStat(mon, 0xf), 0, mainModule->heapId);
                    }
                    SetIllusionDisguise(mon, mainModule->unk2C0);
                }
            }
            firstMonId++;
        }
    }
    func_ov167_0219d454(battleParty);
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

void func_ov167_0219cf50(BtlPokeCon *pokeCon) {
    u32 i;

    for (i = 0; i < 24; i++) {
        if (pokeCon->mons[i] != NULL) {
            func_ov167_021babb8(pokeCon->mons[i]);
            pokeCon->mons[i] = NULL;
        }
    }
}

void func_ov167_0219cf78(BtlPokeCon *pokeCon, BtlMainModule *mainModule, u8 clientId, BOOL keepBaseForm) {
    PokeParty *srcParty;
    BattleParty *party;
    u32 count;
    u8 disguiseSlots[6];
    u32 i;
    u32 j;
    BattleMon *mon;
    PartyPkm *disguise;
    PartyPkm *pkm;

    srcParty = func_ov167_0219d138(pokeCon, clientId);
    party = GetPartyData(pokeCon, clientId);
    count = PokeParty_GetPkmCount(srcParty);
    PokeParty_Init(mainModule->unk2B8);
    for (i = 0; i < 6; i++) {
        disguiseSlots[i] = 6;
    }
    for (i = 0; i < count; i++) {
        mon = func_ov167_0219d4e4(party, i);
        func_ov167_021bc384(mon, keepBaseForm);
        PokeParty_AddPkm(mainModule->unk2B8, GetSrcData(mon));
        if (IsIllusionEnabled(mon)) {
            disguise = func_ov167_021bb064(mon);
            for (j = 0; j < count; j++) {
                if (disguise == GetSrcData(func_ov167_0219d4e4(party, j))) {
                    disguiseSlots[i] = j;
                    break;
                }
            }
        }
    }
    PokeParty_Copy(mainModule->unk2B8, srcParty);
    for (i = 0; i < count; i++) {
        pkm = PokeParty_GetPkm(srcParty, i);
        mon = func_ov167_0219d4e4(party, i);
        func_ov167_021bc43c(mon, pkm);
        if (disguiseSlots[i] != 6) {
            SetIllusionDisguise(mon, PokeParty_GetPkm(srcParty, disguiseSlots[i]));
        }
    }
}

void func_ov167_0219d07c(BtlPokeCon *pokeCon, BtlMainModule *mainModule, u8 clientId) {
    PokeParty *srcParty;
    BattleParty *party;
    u32 count;
    u32 firstMonId;
    u32 i;
    u32 j;
    BattleMon *mon;

    srcParty = func_ov167_0219d138(pokeCon, clientId);
    party = GetPartyData(pokeCon, clientId);
    count = PokeParty_GetPkmCount(srcParty);
    PokeParty_Init(mainModule->unk2B8);
    firstMonId = data_ov167_021d6c24[clientId];
    for (i = 0; i < count; i++) {
        for (j = 0; j < count; j++) {
            mon = func_ov167_0219d4e4(party, j);
            if (firstMonId + i == GetMonID(mon)) {
                func_ov167_021bc384(mon, TRUE);
                PokeParty_AddPkm(mainModule->unk2B8, GetSrcData(mon));
                if (func_ov167_021bbfd0(mon)) {
                    mainModule->setup->unkE1[i] = 1;
                }
                break;
            }
        }
    }
    PokeParty_Copy(mainModule->unk2B8, srcParty);
}

PokeParty *func_ov167_0219d138(BtlPokeCon *pokeCon, u8 clientId) {
    return pokeCon->srcParties[clientId];
}

s32 func_ov167_0219d140(BtlPokeCon *pokeCon, u8 clientId, u8 monId) {
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

BattleMon *func_ov167_0219d180(BtlPokeCon *pokeCon, u8 pos) {
    return func_ov167_0219d188(pokeCon, pos);
}

BattleMon *func_ov167_0219d188(BtlPokeCon *pokeCon, u8 pos) {
    u8 i = pos;
    u8 clientId;
    u8 index;
    BattleParty *parties;
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

BattleMon *func_ov167_0219d1e8(BtlPokeCon *pokeCon, u8 clientId, u8 index) {
    return func_ov167_0219d4e4(&pokeCon->parties[clientId], index);
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

BOOL func_ov167_0219d228(BtlMainModule *mainModule, u32 index, void **out) {
    u8 clientId = func_ov167_0219c650(mainModule, func_ov167_0219c744(mainModule, index));

    if (mainModule->unk3E0[clientId] != NULL) {
        *out = mainModule->unk3E0[clientId];
        return TRUE;
    }
    return FALSE;
}

u8 func_ov167_0219d258(BtlMainModule *mainModule, u8 clientId) {
    if (!func_ov167_0219c980(mainModule)) {
        if (mainModule->unk3E0[clientId] != NULL && doesChatotExist(mainModule->unk3E0[clientId])) {
            return func_02007f90(mainModule->unk3E0[clientId]);
        }
        return 0;
    }
    return mainModule->setup->unk44[clientId];
}

u8 GetClientBattlerCount(BtlMainModule *mainModule, u8 clientId) {
    return func_ov167_0219a180(mainModule, clientId);
}

u8 func_ov167_0219d29c(BtlMainModule *mainModule, u8 clientId) {
    u8 count = func_ov167_0219a180(mainModule, clientId);

    if (BtlSetup_GetBattleStyle(mainModule) == 3) {
        count += 2;
    }
    return count;
}

const AdjacentOpponentData *func_ov167_0219d2bc(u8 pos) {
    return &data_ov167_021d6ca4[pos];
}

BOOL func_ov167_0219d2cc(u8 pos) {
    switch (pos) {
    case 2:
    case 3:
        return TRUE;
    default:
        return FALSE;
    }
}

BOOL func_ov167_0219d2dc(u8 pos, u8 *out) {
    if (!func_ov167_0219d2cc(pos)) {
        *out = func_ov167_0219c48c(2, pos, pos >> 1);
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
    return func_ov167_0219d338(GetSideFromMonID(monId));
}

u8 func_ov167_0219d338(u8 side) {
    return side == 0;
}

BOOL IsAdjacentOpponent(u8 pos1, u8 pos2) {
    const AdjacentOpponentData *data;
    u32 i;

    data = func_ov167_0219d2bc(pos1);
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

u8 func_ov167_0219d38c(u32 pos) {
    switch (pos) {
    case 2:
        return 1;
    case 3:
        return 2;
    default:
        return 0;
    }
}

u32 func_ov167_0219d3a4(u32 pos) {
    switch (pos) {
    case 2:
        return 2;
    case 3:
        return 1;
    default:
        return 0;
    }
}

u8 func_ov167_0219d3bc(u8 pos) {
    return pos & 1;
}

BattleParty *GetPartyData(BtlPokeCon *pokeCon, u32 clientId) {
    return &pokeCon->parties[clientId];
}

BattleParty *GetClientParty(BtlPokeCon *pokeCon, u32 clientId) {
    return &pokeCon->parties[clientId];
}

u32 func_ov167_0219d3e0(BtlMainModule *mainModule) {
    if (mainModule->server != NULL) {
        return GetTurnCounter(func_ov167_0219f38c(mainModule->server));
    }
    return 0;
}

u8 func_ov167_0219d3f8(BtlPokeCon *pokeCon, u8 clientId) {
    return GetAlivePartyCount(GetClientParty(pokeCon, clientId));
}

void func_ov167_0219d404(BtlMainModule *mainModule, u8 clientId, u8 value) {
    if (mainModule->unk46E != 0 && DoesClientExist(mainModule, clientId) && mainModule->clients[clientId] != NULL) {
        func_ov167_021b19b0(mainModule->clients[clientId], value);
    }
}

void func_ov167_0219d434(BattleParty *party) {
    s32 i;

    party->count = 0;
    for (i = 0; i < 6; i++) {
        party->mons[i] = NULL;
    }
}

void AddBattleMonToParty(BattleParty *party, BattleMon *mon) {
    party->mons[party->count++] = mon;
}

void func_ov167_0219d454(BattleParty *party) {
    u32 i;
    u32 processed;

    processed = 0;
    i = 0;
    for (; processed < party->count; processed++) {
        if (CanPokemonBattle(party->mons[i])) {
            i++;
        } else {
            func_ov167_0219d518(party, i);
        }
    }
}

u8 GetNumMonsInParty(BattleParty *party) {
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

u8 func_ov167_0219d4b8(BattleParty *party, s32 index) {
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

BattleMon *func_ov167_0219d4e4(BattleParty *party, u8 index) {
    if (index < party->count) {
        return party->mons[index];
    }
    return NULL;
}

BattleMon *GetBattleMonFromParty(BattleParty *party, u8 index) {
    if (index < party->count) {
        return party->mons[index];
    }
    return 0;
}

void func_ov167_0219d504(BattleParty *party, u32 first, u32 second) {
    BattleMon *mon;

    mon = party->mons[first];
    party->mons[first] = party->mons[second];
    party->mons[second] = mon;
}

void func_ov167_0219d518(BattleParty *party, u8 index) {
    BattleMon *mon;

    mon = party->mons[index];
    for (; index < party->count - 1; index++) {
        party->mons[index] = party->mons[index + 1];
    }
    party->mons[index] = mon;
}

void func_ov167_0219d544(BattleParty *party, u32 pos, BattleMon **oldOut, BattleMon **newOut) {
    BattleMon *first;
    BattleMon *next;
    BattleMon *other;
    u32 index1;
    u32 index2;

    if (pos != 0 && pos != 1) {
        first = party->mons[0];
        index1 = func_ov167_0219d38c(pos);
        index2 = func_ov167_0219d3a4(pos);
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

s32 func_ov167_0219d5b0(const BattleParty *party, u32 monId) {
    s32 i;

    for (i = 0; i < party->count; i++) {
        if (GetMonID(party->mons[i]) == monId) {
            return i;
        }
    }
    return -1;
}

BattleMon *func_ov167_0219d5dc(const BattleParty *party) {
    s32 i;

    for (i = 0; i < party->count; i++) {
        if (CanPokemonBattle(party->mons[i])) {
            return party->mons[i];
        }
    }
    return NULL;
}

void func_ov167_0219d604(BtlMainModule *mainModule, BattleParty *party, u8 clientId) {
    s32 last;
    s32 i;
    BattleMon *mon;

    for (last = party->count - 1; last > 0; last--) {
        if (CanPokemonBattle(party->mons[last])) {
            break;
        }
    }
    if (BtlSetup_GetBattleStyle(mainModule) != 3) {
        i = func_ov167_0219c3e4(mainModule, clientId);
    } else {
        i = 3;
    }
    for (; i < party->count; i++) {
        mon = party->mons[i];
        if (GetBattleMonStat(mon, 0x11) == ABILITY_ILLUSION) {
            if (i < last) {
                SetIllusionDisguise(mon, GetSrcData(party->mons[last]));
            } else if (func_ov167_021bb064(mon) != mainModule->unk2C0) {
                func_ov167_021bb054(mon);
            }
        }
    }
    for (i = 0; i < party->count; i++) {
        IsIllusionEnabled(party->mons[i]);
    }
}

void func_ov167_0219d6ac(BtlMainModule *mainModule) {
    u32 i;

    for (i = 0; i < 4; i++) {
        mainModule->trainers[i].playerInfo = NULL;
        mainModule->trainers[i].unk0A = 0;
        mainModule->trainers[i].unk0C = 0;
    }
    for (i = 0; i < 4; i++) {
        mainModule->unk3E0[i] = NULL;
    }
}

void func_ov167_0219d6dc(BtlMainModule *mainModule) {
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
        if (mainModule->unk3E0[i] != NULL) {
            GFL_HeapFree(mainModule->unk3E0[i]);
            mainModule->unk3E0[i] = NULL;
        }
    }
}

void func_ov167_0219d72c(BtlTrainerData *trainer, HeapID heapId, PlayerInfo *src) {
    u16 trainerClass;
    u8 version;

    trainer->playerInfo = func_02008b0c(heapId);
    func_02008b34(src, trainer->playerInfo);
    trainerClass = 0;
    trainer->unk0A = 0;
    version = func_02008bfc(src);
    if (version == VERSION_WHITE2 || version == VERSION_BLACK2) {
        if (getTrainerGender(trainer->playerInfo)) {
            trainerClass = 1;
        }
        trainer->unk08 = trainerClass;
    } else {
        trainer->unk08 = getTrainerGender(trainer->playerInfo) == 0 ? 0xb6 : 0xb7;
    }
    trainer->name = copyTrainerNameToNewStrbuf(trainer->playerInfo->name, HEAPID_BATTLE);
    PMSData_Clear(&trainer->unk18);
    PMSData_Clear(&trainer->unk20);
}

void func_ov167_0219d794(BtlTrainerData *trainer, const BtlSetupTrainer *src) {
    u32 i;

    trainer->playerInfo = NULL;
    if (src != NULL) {
        trainer->unk0A = src->trainerId;
        trainer->unk08 = src->trainerClass;
        trainer->name = GFL_StrBufClone(src->name, HEAPID_BATTLE);
        trainer->unk0C = src->aiFlags;
        sys_memcpy(src->items, trainer->unk10, sizeof(trainer->unk10));
        PMSData_Copy(&trainer->unk18, &src->unk18);
        PMSData_Copy(&trainer->unk20, &src->unk20);
    } else {
        trainer->unk0A = 0;
        trainer->unk08 = 0;
        trainer->name = NULL;
        for (i = 0; i < 4; i++) {
            trainer->unk10[i] = 0;
        }
        PMSData_Clear(&trainer->unk18);
        PMSData_Clear(&trainer->unk20);
    }
}

void func_ov167_0219d808(BtlTrainerData *trainer, const BtlCommTrainerData *src) {
    u32 i;

    trainer->playerInfo = NULL;
    if (src != NULL) {
        trainer->unk0A = src->trainerId;
        trainer->unk08 = src->trainerClass;
        trainer->name = GFL_StrBufCreate(0x20, HEAPID_BATTLE);
        GFL_StrBufCopyString(trainer->name, src->name, src->nameLength + 1);
        trainer->unk0C = src->aiFlags;
        sys_memcpy(src->items, trainer->unk10, sizeof(trainer->unk10));
        PMSData_Copy(&trainer->unk18, &src->unk18);
        PMSData_Copy(&trainer->unk20, &src->unk20);
    } else {
        trainer->unk0A = 0;
        trainer->unk08 = 0;
        trainer->name = NULL;
        for (i = 0; i < 4; i++) {
            trainer->unk10[i] = 0;
        }
        PMSData_Clear(&trainer->unk18);
        PMSData_Clear(&trainer->unk20);
    }
}

BOOL func_ov167_0219d888(BtlMainModule *mainModule, u8 clientId) {
    if (mainModule->trainers[clientId].playerInfo == NULL) {
        return TRUE;
    }
    return FALSE;
}

u16 func_ov167_0219d89c(BtlMainModule *mainModule, u8 clientId, u8 index) {
    if (func_ov167_0219d888(mainModule, clientId)) {
        BtlTrainerData *trainer = &mainModule->trainers[clientId];

        if (index < 4) {
            return trainer->unk10[index];
        }
    }
    return 0;
}

StrBuf *func_ov167_0219d8c4(BtlMainModule *mainModule, u8 clientId, u32 *trainerClass) {
    *trainerClass = mainModule->trainers[clientId].unk08;
    return mainModule->trainers[clientId].name;
}

u32 func_ov167_0219d8d4(BtlMainModule *mainModule, u8 clientId) {
    if (BtlSetup_GetBattleType(mainModule) == 0) {
        if (BtlSetup_IsBattleType(mainModule, 8)) {
            return 0x800;
        }
        if (BtlSetup_GetBattleStyle(mainModule) == 1) {
            return 0x80;
        }
    } else if (func_ov167_0219d888(mainModule, clientId)) {
        return mainModule->trainers[clientId].unk0C;
    }
    return 0;
}

u16 func_ov167_0219d91c(BtlMainModule *mainModule, u8 clientId) {
    if (func_ov167_0219d888(mainModule, clientId)) {
        return mainModule->trainers[clientId].unk0A;
    }
    return 0;
}

u32 func_ov167_0219d938(BtlMainModule *mainModule, u8 clientId) {
    return mainModule->trainers[clientId].unk08;
}

PMSData *func_ov167_0219d944(BtlMainModule *mainModule, u8 clientId, u32 which) {
    if (BtlSetup_GetBattleType(mainModule) == 2 && func_ov167_0219d888(mainModule, clientId)) {
        if (which == 1) {
            return &mainModule->trainers[clientId].unk20;
        }
        return &mainModule->trainers[clientId].unk18;
    }
    return NULL;
}

PlayerInfo *func_ov167_0219d97c(BtlMainModule *mainModule, u8 clientId) {
    if (func_ov167_0219d888(mainModule, clientId)) {
        return NULL;
    }
    return mainModule->trainers[clientId].playerInfo;
}

PlayerInfo *func_ov167_0219d998(BtlMainModule *mainModule) {
    return CommPlayerSupport_GetSupporter(mainModule->setup->unk88);
}

BtlField *func_ov167_0219d9a8(BtlMainModule *mainModule) {
    return mainModule->field;
}

void func_ov167_0219d9b0(BtlMainModule *mainModule) {
    u32 i;

    for (i = 0; i < 4; i++) {
        mainModule->unk298[i] = PokeParty_Create(HEAPID_BATTLE);
        mainModule->unk2A8[i] = PokeParty_Create(HEAPID_BATTLE);
    }
    mainModule->unk2B8 = PokeParty_Create(HEAPID_BATTLE);
}

void func_ov167_0219d9e8(BtlMainModule *mainModule) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (mainModule->unk298[i] != NULL) {
            GFL_HeapFree(mainModule->unk298[i]);
            mainModule->unk298[i] = NULL;
        }
        if (mainModule->unk2A8[i] != NULL) {
            GFL_HeapFree(mainModule->unk2A8[i]);
            mainModule->unk2A8[i] = NULL;
        }
    }
    if (mainModule->unk2B8 != NULL) {
        GFL_HeapFree(mainModule->unk2B8);
        mainModule->unk2B8 = NULL;
    }
}

void func_ov167_0219da44(BtlMainModule *mainModule, u8 clientId, const PokeParty *party) {
    if (mainModule->unk298[clientId] != NULL) {
        PokeParty_Init(mainModule->unk298[clientId]);
        PokeParty_Init(mainModule->unk2A8[clientId]);
        PokeParty_Copy(party, mainModule->unk298[clientId]);
        PokeParty_Copy(party, mainModule->unk2A8[clientId]);
        if (PokeParty_GetPkmCount(mainModule->unk2A8[clientId]) != 0) {
            PokeParty_GetPkm(mainModule->unk2A8[clientId], 0);
        }
    }
}

PokeParty *func_ov167_0219da94(BtlMainModule *mainModule, u8 clientId, u32 useSecond) {
    if (useSecond) {
        if (PokeParty_GetPkmCount(mainModule->unk2A8[clientId]) != 0) {
            PokeParty_GetPkm(mainModule->unk2A8[clientId], 0);
        }
        return mainModule->unk2A8[clientId];
    }
    return mainModule->unk298[clientId];
}

u32 BtlSetup_IsBattleType(BtlMainModule *mainModule, u32 flag) {
    return BtlSetup_CheckFlag(mainModule->setup, flag);
}

void func_ov167_0219dad0(BtlMainModule *mainModule, u32 record) {
    BtlSetup *setup = mainModule->setup;

    if (setup->fieldSituation.unk1b == 0) {
        RecordAddOne(setup->records, record);
    }
}

void func_ov167_0219dae8(BtlMainModule *mainModule, u32 record, u32 value) {
    BtlSetup *setup = mainModule->setup;

    if (setup->fieldSituation.unk1b == 0) {
        RecordAdd(setup->records, record, value);
    }
}

void *func_ov167_0219db00(BtlMainModule *mainModule) {
    return mainModule->setup->unk90;
}

BOOL func_ov167_0219db08(BtlMainModule *mainModule) {
    BOOL result = FALSE;

    switch (func_ov167_0219a004(mainModule->setup)) {
    case 1:
        if (mainModule->setup->unk97 != 0) {
            result = TRUE;
        }
        break;
    }
    return result;
}

BOOL func_ov167_0219db28(BtlMainModule *mainModule) {
    if (!mainModule->setup->unkDD_5) {
        if (func_ov167_0219a004(mainModule->setup) == 1) {
            return func_ov167_0219db08(mainModule);
        }
        switch (BtlSetup_GetBattleType(mainModule)) {
        case 0:
        case 1:
            return TRUE;
        default:
            return FALSE;
        }
    }
    return FALSE;
}

void func_ov167_0219db64(BtlMainModule *mainModule, BattleMon *mon) {
    ChangeFriendship(mainModule, mon, 0);
    func_ov167_0219f2c0(mainModule->server, mon);
}

void func_ov167_0219db7c(BtlMainModule *mainModule, BattleMon *mon, u32 value) {
    u8 monId;
    const BattleMon *param1;
    const BattleMon *param2;
    PartyPkm *src1;
    PartyPkm *src2;
    const BtlFieldSituation *field;

    monId = GetMonID(mon);
    param1 = GetPokeParamConst(&mainModule->pokeCons[1], monId);
    param2 = GetPokeParamConst(&mainModule->pokeCons[0], monId);
    src1 = GetSrcData(param1);
    src2 = GetSrcData(param2);
    field = GetFieldEffectData(mainModule);
    func_02020c8c(src1, value, field->env.zoneId, HEAPID_TAIL(mainModule->heapId));
    func_02020c8c(src2, value, field->env.zoneId, HEAPID_TAIL(mainModule->heapId));
}

void func_ov167_0219dc00(BtlMainModule *mainModule, BattleMon *mon) {
    if (mainModule->server != NULL) {
        func_ov167_0219f2e0(mainModule->server, mon);
    }
}

void func_ov167_0219dc10(BtlMainModule *mainModule) {
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

    if (mainModule->setup->fieldSituation.unk1b != 0 || func_ov167_0219c988(mainModule) != 0) {
        return;
    }
    if (mainModule->setup->battleType <= 1) {
        func_ov167_0219d07c(&mainModule->pokeCons[1], mainModule, mainModule->playerClientId);
        party = func_ov167_0219d138(&mainModule->pokeCons[1], mainModule->playerClientId);
        if (mainModule->setup->battleType == 1 && func_ov167_021bd788(func_ov167_0219d938(mainModule, 1))) {
            func_02020cf0(party, GetFieldEffectData(mainModule)->env.zoneId, HEAPID_TAIL(mainModule->heapId));
        }
        if (mainModule->setup->battleType == 1) {
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
    if (mainModule->setup->battleType == 0) {
        clientId = func_ov167_0219c8d0(mainModule, mainModule->playerClientId, 0);
        func_ov167_0219cf78(&mainModule->pokeCons[1], mainModule, clientId, TRUE);
        PokeParty_Copy(func_ov167_0219d138(&mainModule->pokeCons[1], clientId), mainModule->setup->party[1]);
    }
    if (mainModule->setup->battleType == 4) {
        return;
    }
    for (j = 0; j < 4; j++) {
        index = func_ov167_0219e048(mainModule->playerClientId, j);
        for (k = 0; k < 6; k++) {
            mainModule->setup->unkE7[index][k] = 0;
        }
        if (DoesClientExist(mainModule, index)) {
            battleParty = GetPartyData(&mainModule->pokeCons[0], index);
            monCount = GetNumMonsInParty(battleParty);
            maxHP = 0;
            hp = 0;
            firstMonId = data_ov167_021d6c24[index];
            for (i = 0; i < monCount; i++) {
                mon = func_ov167_0219d4e4(battleParty, i);
                slot = GetMonID(mon) - firstMonId;
                hp += GetBattleMonStat(mon, 13);
                maxHP += GetBattleMonStat(mon, 14);
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

BOOL func_ov167_0219de6c(BtlMainModule *mainModule) {
    switch (BtlSetup_GetBattleType(mainModule)) {
    case 2:
    case 3:
        return TRUE;
    default:
        return FALSE;
    }
}

u32 func_ov167_0219de84(BtlMainModule *mainModule) {
    u32 result;

    if (BtlSetup_GetBattleType(mainModule) == 4) {
        result = 1;
    } else if (mainModule->unk473_0) {
        result = 6;
    } else if (mainModule->setup->unkAC != 6) {
        result = 5;
    } else if (func_ov167_021bda94(&mainModule->unk448)) {
        result = func_ov167_021bda98(&mainModule->unk448, mainModule->playerClientId, BtlSetup_GetBattleType(mainModule));
    } else {
        if (mainModule->result == 7) {
            mainModule->result = 1;
        }
        result = mainModule->result;
    }
    mainModule->setup->unkA8 = result;
    return result;
}

u16 func_ov167_0219defc(BtlMainModule *mainModule) {
    return mainModule->unk442;
}

u16 func_ov167_0219df08(BtlMainModule *mainModule) {
    return mainModule->unk440;
}

BOOL func_ov167_0219df10(BtlMainModule *mainModule) {
    if (mainModule->setup->fieldSituation.unk1b == 0) {
        if (mainModule->unk440 != 0) {
            return func_ov167_021b1d64(mainModule->clients[mainModule->playerClientId]);
        }
        return FALSE;
    }
    return func_ov167_021b1d64(mainModule->clients[mainModule->playerClientId]);
}

BOOL func_ov167_0219df50(BtlMainModule *mainModule) {
    BOOL result;

    if (mainModule->setup->fieldSituation.unk1b != 0) {
        result = func_ov167_021b1d90(mainModule->clients[mainModule->playerClientId]);
        if (result) {
            mainModule->result = 2;
        }
        return result;
    }
    return FALSE;
}

void func_ov167_0219df80(BtlMainModule *mainModule, u8 clientId, const PokeParty *party) {
    u8 index = func_ov167_0219e048(mainModule->playerClientId, clientId);

    if (mainModule->setup->party[index] != NULL) {
        PokeParty_Copy(party, mainModule->setup->party[index]);
    }
}

void func_ov167_0219dfa8(BtlMainModule *mainModule, u8 clientId, const PlayerInfo *src) {
    u8 index = func_ov167_0219e048(mainModule->playerClientId, clientId);

    if (mainModule->setup->unk34[index] != NULL) {
        func_02008b34(src, mainModule->setup->unk34[index]);
    }
}

void func_ov167_0219dfd0(BtlMainModule *mainModule, u8 clientId) {
    u8 index = func_ov167_0219e048(mainModule->playerClientId, clientId);

    mainModule->setup->unk44[index] = func_ov167_0219d258(mainModule, clientId);
}

void func_ov167_0219dff8(BtlMainModule *mainModule) {
    u32 size;
    void *data;

    if (func_ov167_0219c964(mainModule)) {
        data = func_ov167_021b18d4(mainModule->clients[mainModule->playerClientId], &size);
        if (data != NULL) {
            sys_memcpy(data, mainModule->setup->unkB0, size);
            mainModule->setup->unkB4 = size;
            mainModule->setup->rand = mainModule->rand;
        }
    }
}

u8 func_ov167_0219e048(u8 playerClientId, u8 clientId) {
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

void func_ov167_0219e074(BtlMainModule *mainModule, u32 arg1) {
    u32 unk;
    u32 i;

    unk = func_ov167_0219a004(mainModule->setup);
    func_ov167_021bd090(&mainModule->rand);
    func_ov167_0219a0c4(mainModule, mainModule->setup);
    func_ov167_0219cd00(&mainModule->pokeCons[0]);
    func_ov167_0219cd00(&mainModule->pokeCons[1]);
    for (i = 0; i < 4; i++) {
        if (DoesClientExist(mainModule, i)) {
            func_ov167_0219cd3c(&mainModule->pokeCons[1], mainModule, i);
            func_ov167_0219cd3c(&mainModule->pokeCons[0], mainModule, i);
            func_ov167_021b1934(mainModule->clients[i], arg1);
        }
    }
    func_ov167_021ce870(mainModule->viewCore);
    func_ov167_021ce668(0x13);
    mainModule->viewCore = BtlvCore_Create(mainModule, mainModule->clients[mainModule->setup->fieldSituation.unk19],
                                           &mainModule->pokeCons[0], unk, 0x13);
    func_ov167_021b190c(mainModule->clients[mainModule->setup->fieldSituation.unk19], mainModule->viewCore);
    func_ov167_0219e544(mainModule->server);
}

void func_ov167_0219e130(BtlMainModule *mainModule) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (DoesClientExist(mainModule, i)) {
            func_ov167_021b1960(mainModule->clients[i]);
        }
    }
}

BtlServerFlow *func_ov167_0219e158(BtlMainModule *mainModule) {
    return func_ov167_0219f38c(mainModule->server);
}

void func_ov167_0219e164(BtlSetup *setup) {
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

void func_ov167_0219e1b0(BtlMainModule *mainModule) {
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
    flow = func_ov167_0219f38c(mainModule->server);
    setup = mainModule->setup;
    setup->unkD2 = GetTurnCounter(flow);
    setup->unkD3 = func_ov167_021abc80(flow, 0);
    setup->unkD4 = func_ov167_021ab7fc(flow);
    setup->unkD5 = func_ov167_021ab804(flow);
    setup->unkD6 = func_ov167_021ab810(flow);
    setup->unkD7 = func_ov167_021ab81c(flow);
    setup->unkD8 = func_ov167_021ab828(flow);
    if (setup->unkA8 == 1) {
        setup->unkD9 = 1;
    }
    party = GetPartyData(&mainModule->pokeCons[1], 0);
    count = GetNumMonsInParty(party);
    alive = GetAlivePartyCount(party);
    for (i = 0; i < count; i++) {
        mon = func_ov167_0219d4e4(party, i);
        setup->unkDC += func_ov167_021bacb4(mon);
        setup->unkD0 += GetBattleMonStat(mon, 13);
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

u32 func_ov167_0219e300(BtlMainModule *mainModule) {
    return func_ov167_021b19a4(mainModule->clients[0]);
}

BtlSetup *func_ov167_0219e30c(BtlMainModule *mainModule) {
    return mainModule->setup;
}

BtlSetup *func_ov167_0219e310(BtlMainModule *mainModule) {
    return mainModule->setup;
}

void func_ov167_0219e314(BtlMainModule *mainModule, u8 arg1) {
    mainModule->unk474 = GFL_ArcSysReadHeapNew(0x10d, arg1, HEAPID_BATTLE);
    mainModule->unk478 = GFL_HeapAllocate(HEAPID_BATTLE, 0x14, FALSE, "btl_main.c", 6432);
    func_ov167_0219e3c8(mainModule->unk474);
    mainModule->unk478->unk08 = 0xff;
    mainModule->cutin = func_ov167_021d5e1c(HEAPID_BATTLE);
    if (func_ov167_0219c988(mainModule) == 2) {
        func_ov167_0219cb7c(mainModule);
    }
}

void func_ov167_0219e378(BtlMainModule *mainModule) {
    func_ov167_021d5e68(mainModule->cutin);
    GFL_HeapFree(mainModule->unk478);
    GFL_HeapFree(mainModule->unk474);
}

BtlScriptedRules *func_ov167_0219e39c(BtlMainModule *mainModule) {
    if (mainModule->unk474 == NULL) {
        return NULL;
    }
    return mainModule->unk474;
}

void *func_ov167_0219e3ac(BtlMainModule *mainModule) {
    if (mainModule->unk478 == NULL) {
        return NULL;
    }
    return mainModule->unk478;
}

PokewoodCutin *func_ov167_0219e3bc(BtlMainModule *mainModule) {
    return mainModule->cutin;
}

void func_ov167_0219e3c8(void *data) {
}
