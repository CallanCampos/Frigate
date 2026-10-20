#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "field/event_mapchange.h"
#include "field/event_sound.h"
#include "field/field.h"
#include "field/field_event.h"
#include "field/field_script.h"
#include "field/field_script_event.h"
#include "field/game_beacon_set.h"
#include "field/musical.h"
#include "field/musical_dressup_sys.h"
#include "field/musical_program.h"
#include "field/musical_stage_sys.h"
#include "field/player_state.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/rtc.h"
#include "pml/poke_party.h"
#include "save/musical_savedata.h"
#include "save/player_info.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/wordset.h"

// The event of a musical show: the dressing room, the stage and the photo, alone with three performers of the
// program or online with up to three other players
struct MusicalEventWork {
    GameSystem *gsys;
    GameData *gameData;
    MusicalCommWork *commWork;
    u32 state;
    u8 subState;
    BOOL online;
    PartyPkm *pkm;
    SaveControl *save;
    GameCommSys *gameComm;
    Ov210Work *ov210;
    // Overlay 211's communication work
    void *comm;
    MusicalDressUpParam *dressUp;
    MusicalStageParam *stage;
    MusicalShotParam *shot;
    MusicalProgram *program;
    MusicalPoke *poke;
    MusicalSave *musicalSave;
    // The stage position of each Pokémon, in entry order
    u8 order[4];
    // The stage positions, from the most points to the fewest
    u8 ranking[4];
    // The stage position of the player's Pokémon
    u8 playerPos;
    BOOL error;
};

static GameEventReturnCode MusicalEvent_Callback(GameEvent *event, u32 *state, void *data);
static void MusicalEvent_InitPoke(MusicalEventWork *work);
static void MusicalEvent_Release(MusicalEventWork *work);
static void MusicalEvent_CreateDressUpParam(MusicalEventWork *work);
static void MusicalEvent_SendPoke(MusicalEventWork *work);
static void MusicalEvent_CreateStageParam(MusicalEventWork *work);
static void MusicalEvent_AddLikedPropPoints(MusicalEventWork *work);
static void MusicalEvent_TakeShot(MusicalEventWork *work);
static void MusicalEvent_RecordShow(MusicalEventWork *work);
static void MusicalEvent_DrawAudienceGifts(MusicalEventWork *work);
static BOOL MusicalEvent_ReturnToField(GameEvent *event, MusicalEventWork *work);
static BOOL MusicalEvent_LeaveField(GameEvent *event, MusicalEventWork *work);
static void MusicalEvent_WarpToDressingRoom(GameEvent *event, MusicalEventWork *work);
static void MusicalEvent_WarpToHall(GameEvent *event, MusicalEventWork *work);
static void MusicalEvent_CallScript(GameEvent *event, MusicalEventWork *work, u16 scriptId);
static u8 MusicalEvent_GetNpcIndex(MusicalEventWork *work, u8 pos);
static u32 MusicalEvent_GetPlayerTrainerClass(PlayerInfo *info);
static void MusicalEvent_SetReceivedProgram(MusicalEventWork *work);

static const VecFx32 data_ov012_0216ade4 = { FX32_CONST(232), 0, FX32_CONST(200) };
// MWCC lays these two out in the reverse of their definitions
static const VecFx32 data_ov012_0216adfc = { FX32_CONST(168), 0, FX32_CONST(200) };
static const VecFx32 data_ov012_0216adf0 = { FX32_CONST(136), 0, FX32_CONST(200) };

// The total points from which a number of the program's prize slots is drawn, from 10 slots down
static const u16 data_ov012_0216ae08[10] = { 12000, 8000, 5500, 3500, 2000, 1000, 500, 250, 100, 0 };

// The props each program can give
// clang-format off
static const u8 data_ov012_0216ae1c[7][40] = {
    {0x19, 0x1a, 0x4b, 0x21, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x30, 0x31, 0x32, 0x3c, 0x40, 0x46, 0x47, 0x48, 0x51,
     0x53, 0x55, 0x5e, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x08, 0x0d, 0x0e, 0x12, 0x24, 0x25, 0x10, 0x11, 0x14, 0x15, 0x20},
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x09, 0x0a, 0x0b, 0x0c, 0x0f, 0x13, 0x16, 0x1b, 0x1c, 0x1f, 0x2f, 0x49,
     0x4a, 0x17, 0x4f, 0x61, 0x19, 0x1a, 0x2d, 0x21, 0x27, 0x28, 0x08, 0x0d, 0x0e, 0x12, 0x24, 0x25, 0x10, 0x11, 0x14, 0x15},
    {0x08, 0x0d, 0x0e, 0x12, 0x24, 0x25, 0x26, 0x38, 0x39, 0x3a, 0x3b, 0x3d, 0x3e, 0x3f, 0x41, 0x42, 0x44, 0x45, 0x4d, 0x4e,
     0x5b, 0x19, 0x1a, 0x2d, 0x21, 0x27, 0x28, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x10, 0x11, 0x14, 0x15, 0x17, 0x1d, 0x1e},
    {0x10, 0x11, 0x14, 0x15, 0x20, 0x1d, 0x1e, 0x22, 0x23, 0x2e, 0x33, 0x34, 0x35, 0x36, 0x4c, 0x50, 0x56, 0x57, 0x58, 0x59,
     0x5a, 0x5c, 0x5d, 0x19, 0x1a, 0x2d, 0x21, 0x27, 0x28, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x08, 0x0d, 0x0e, 0x12, 0x45},
    {0x19, 0x1a, 0x2d, 0x21, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x09, 0x08, 0x0d,
     0x0e, 0x12, 0x24, 0x25, 0x26, 0x38, 0x39, 0x10, 0x11, 0x14, 0x15, 0x20, 0x1d, 0x1e, 0x22, 0x23, 0x4b, 0x2f, 0x4d, 0x58},
    {0x2c, 0x2d, 0x30, 0x31, 0x32, 0x3c, 0x40, 0x46, 0x47, 0x0a, 0x0b, 0x0c, 0x0f, 0x13, 0x16, 0x1b, 0x1c, 0x1f, 0x3a, 0x3b,
     0x3d, 0x3e, 0x3f, 0x41, 0x42, 0x44, 0x45, 0x2e, 0x33, 0x34, 0x35, 0x36, 0x4c, 0x50, 0x56, 0x57, 0x51, 0x49, 0x4e, 0x5a},
    {0x21, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x31, 0x32, 0x3c, 0x47, 0x48, 0x0b, 0x2f, 0x49, 0x4a, 0x1f, 0x4f, 0x61,
     0x25, 0x39, 0x3b, 0x3d, 0x3e, 0x3f, 0x42, 0x45, 0x4d, 0x4e, 0x5b, 0x23, 0x2e, 0x33, 0x34, 0x35, 0x4c, 0x58, 0x59, 0x5a},
};

// clang-format on

GameEvent *MusicalEvent_Create(GameSystem *gsys, GameData *gameData, u8 slot, BOOL online, MusicalCommWork *comm) {
    GameEvent *event;
    MusicalEventWork *work;
    StrBuf *name;
    u16 shots;
    u8 i;
    u8 j;
    u8 other;
    u8 temp;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_TAIL(HEAPID_MUSICAL_EVENT), 0x42000);
    event = GameEvent_Create(gsys, NULL, MusicalEvent_Callback, sizeof(MusicalEventWork));
    work = GameEvent_GetData(event);
    work->gsys = gsys;
    work->gameData = gameData;
    work->online = online;
    work->save = GameData_GetSaveControl(gameData);
    work->gameComm = GSYS_GetGameCommSystem(gsys);
    work->commWork = comm;
    work->commWork->event = work;
    work->comm = comm->comm;
    work->error = FALSE;
    work->program = NULL;
    name = GFL_StrBufCreate(11, HEAPID_GAMEEVENT);
    work->pkm = PokeParty_GetPkm(GameData_GetParty(work->gameData), slot);
    PokeParty_GetParam(work->pkm, PKM_PARAM_NICKNAME, name);
    GameBeacon_BroadcastMusical(name);
    GFL_StrBufFree(name);
    work->dressUp = NULL;
    work->stage = NULL;
    work->shot = NULL;
    work->musicalSave = getMusicalInfoBlkAddress(gameData);
    work->ov210 = MusicalSystem_InitProgramData(HEAPID_GAMEEVENT);
    if (work->online == FALSE || MusComm_IsParent(work->commWork->comm) == TRUE) {
        MusicalSystem_LoadProgramData(work->ov210, work->save, work->gameData,
                                      MusicalSave_GetProgram(work->musicalSave), HEAPID_MUSICAL_EVENT);
    }
    if (work->online == TRUE) {
        work->state = 11;
        work->subState = 0;
        MusComm_SetUp(work->comm, GetGameDataPlayerInfo(work->gameData), func_0201d620(work->pkm), work->gameComm,
                      work->ov210, HEAPID_GAMEEVENT);
    } else {
        shots = MusicalSave_GetTotalPoints(work->musicalSave);
        for (i = 0; i < 4; i++) {
            work->order[i] = i;
        }
        for (j = 0; j < 10; j++) {
            for (i = 0; i < 4; i++) {
                other = GFL_RandomLCAlt(4);
                temp = work->order[i];
                work->order[i] = work->order[other];
                work->order[other] = temp;
            }
        }
        work->playerPos = work->order[0];
        work->program = MusicalProgram_Create(HEAPID_GAMEEVENT, work->ov210, shots);
        work->state = 13;
        work->subState = 0;
    }
    return event;
}

static GameEventReturnCode MusicalEvent_Callback(GameEvent *event, u32 *state, void *data) {
    MusicalEventWork *work = data;

    MusicalEvent_CheckNetError(work);
    switch (work->state) {
    case 11:
        if (MusComm_AreAllInfoReceived(work->commWork->comm) == TRUE) {
            u32 program = 0;
            u32 cpus = 0;
            u8 i;
            RivalDataSave *rivals;

            if (MusComm_IsParent(work->commWork->comm) == TRUE) {
                work->program =
                    MusicalProgram_Create(HEAPID_GAMEEVENT, work->ov210, MusicalSave_GetTotalPoints(work->musicalSave));
                program = MusicalProgram_PackPoints(work->program);
                cpus = MusicalProgram_PackNpcs(work->program);
            }
            MusComm_BeginDataTiming(work->commWork->comm, program, cpus);
            work->state = 12;
            rivals = getHollow_RivalData(work->save);
            for (i = 0; i < 4; i++) {
                if (MusComm_GetSelfPos(work->commWork->comm) != MusComm_GetPlayerPos(work->commWork->comm, i)) {
                    PlayerInfo *info = MusComm_GetPlayerInfo(work->commWork->comm, i);

                    if (info != NULL) {
                        Misc_AddMetTrainer(rivals, getIDAsUInt(info));
                    }
                }
            }
        } else if (work->error == TRUE) {
            work->state = 23;
        }
        break;
    case 12:
        if (MusComm_IsProgramReceived(work->commWork->comm) == TRUE) {
            u8 i;

            work->state = 13;
            for (i = 0; i < 4; i++) {
                work->order[i] = MusComm_GetPlayerPos(work->commWork->comm, i);
            }
            work->playerPos = MusComm_GetSelfPos(work->commWork->comm);
            MusicalEvent_SetReceivedProgram(work);
        } else if (work->error == TRUE) {
            work->state = 23;
        }
        break;
    case 13:
        MusicalEvent_WarpToDressingRoom(event, work);
        work->state = 14;
        break;
    case 14:
        MusicalEvent_InitPoke(work);
        MusicalEvent_CallScript(event, work, 3);
        work->state = 0;
        break;
    case 0:
        if (work->error == TRUE && work->subState == 0) {
            work->state = 23;
        } else if (MusicalEvent_LeaveField(event, work) == TRUE) {
            work->subState = 0;
            work->state = 1;
        }
        break;
    case 1:
        work->state = 3;
        break;
    case 3:
        MusicalEvent_CreateDressUpParam(work);
        GSYS_QueueProc(work->gsys, OVERLAY_NONE, &data_ov012_0216dfc4, work->dressUp);
        work->state = 4;
        break;
    case 4:
        if (GSYS_GetProcMgrState(work->gsys) == FALSE) {
            MusicalEvent_SendPoke(work);
            work->state = 16;
        }
        break;
    case 16:
        if (MusicalEvent_ReturnToField(event, work) == TRUE) {
            work->subState = 0;
            work->state = 17;
            if (work->error == TRUE) {
                work->state = 23;
            }
        }
        break;
    case 17:
        MusicalEvent_CallScript(event, work, 4);
        work->state = 18;
        break;
    case 18:
        if (work->error == TRUE && work->subState == 0) {
            work->state = 23;
        } else if (MusicalEvent_LeaveField(event, work) == TRUE) {
            work->subState = 0;
            work->state = 5;
        }
        break;
    case 5:
        if (work->error == TRUE) {
            work->state = 22;
        } else {
            MusicalEvent_CreateStageParam(work);
            GSYS_QueueProc(work->gsys, OVERLAY_NONE, &data_ov012_0216dfe8, work->stage);
            work->state = 6;
        }
        break;
    case 6:
        if (GSYS_GetProcMgrState(work->gsys) == FALSE) {
            MusicalEvent_AddLikedPropPoints(work);
            work->state = 7;
            if (work->error == TRUE) {
                work->state = 22;
            }
        }
        break;
    case 7:
        MusicalEvent_TakeShot(work);
        GSYS_QueueProc(work->gsys, OVERLAY_ID(209), &MUSICAL_SHOT_PROC_FUNCTIONS, work->shot);
        work->state = 8;
        break;
    case 8:
        if (GSYS_GetProcMgrState(work->gsys) == FALSE) {
            work->state = 19;
            if (work->error == TRUE) {
                work->state = 22;
            }
        }
        break;
    case 19:
        if (MusicalEvent_ReturnToField(event, work) == TRUE) {
            work->subState = 0;
            work->state = 20;
            MusicalEvent_DrawAudienceGifts(work);
            MusicalEvent_RecordShow(work);
        }
        break;
    case 20:
        MusicalEvent_CallScript(event, work, 5);
        work->state = 2;
        break;
    case 2:
        if (work->error == TRUE) {
            work->state = 23;
        } else {
            MusicalEvent_Release(work);
            MusicalEvent_WarpToHall(event, work);
            work->state = 9;
        }
        break;
    case 9:
        MusicalEvent_CallScript(event, work, 10454);
        work->state = 10;
        break;
    case 10:
        GFL_HeapDumpOnFailure(HEAPID_GAMEEVENT);
        GFL_HeapDelete(HEAPID_MUSICAL_EVENT);
        return GAMEEVENT_DONE;
    case 22:
        if (MusicalEvent_ReturnToField(event, work) == TRUE) {
            work->state = 23;
        }
        break;
    case 23:
        GFL_NetErrMarkShown();
        work->state = 24;
        break;
    case 24:
        MusicalEvent_Release(work);
        MusicalEvent_WarpToHall(event, work);
        work->state = 25;
        break;
    case 25:
        work->state = 26;
        break;
    case 26:
        MusicalEvent_CallScript(event, work, 10455);
        work->state = 10;
        break;
    }
    return GAMEEVENT_CONTINUE;
}

static void MusicalEvent_InitPoke(MusicalEventWork *work) {
    work->poke = MusicalSystem_InitPokeFromPkm(work->pkm, HEAPID_GAMEEVENT);
    if (work->online) {
        MusComm_IsParent(work->commWork->comm);
        MusComm_BeginProgramTiming(work->commWork->comm);
    }
}

static void MusicalEvent_Release(MusicalEventWork *work) {
    if (work->dressUp != NULL) {
        MusicalDressUpParam_Free(work->dressUp);
    }
    if (work->stage != NULL) {
        MusicalStageParam_Free(work->stage);
    }
    if (work->shot != NULL) {
        GFL_HeapFree(work->shot->shot);
        GFL_HeapFree(work->shot);
    }
    if (work->ov210 != NULL) {
        MusicalSystem_FreeProgramData(work->ov210);
    }
    if (work->poke != NULL) {
        GFL_HeapFree(work->poke);
    }
    if (work->program != NULL) {
        MusicalProgram_Free(work->program);
    }
    work->commWork->event = NULL;
}

static void MusicalEvent_CreateDressUpParam(MusicalEventWork *work) {
    work->dressUp = MusicalDressUpParam_Create(HEAPID_GAMEEVENT, work->poke, work->save);
    work->dressUp->comm = work->comm;
}

static void MusicalEvent_SendPoke(MusicalEventWork *work) {
    if (work->online == TRUE) {
        MusComm_BeginSendPoke(work->comm, work->poke);
    }
}

static void MusicalEvent_CreateStageParam(MusicalEventWork *work) {
    u8 i;
    u8 cpu;
    MusicalPoke *poke;

    work->stage = MusicalStageParam_Create(HEAPID_GAMEEVENT, work->comm);
    if (work->online == FALSE) {
        for (i = 0; i < 4; i++) {
            if (i == 0) {
                MusicalStageParam_SetPlayerPoke(work->stage, work->order[i], work->poke);
            } else {
                MusicalProgram_PutNpcOnStage(work->program, work->stage, work->order[i], i - 1, HEAPID_GAMEEVENT);
            }
        }
    } else {
        cpu = 0;
        for (i = 0; i < 4; i++) {
            poke = MusComm_GetPokeByPos(work->comm, work->order[i]);
            if (poke != NULL) {
                MusicalStageParam_SetCommPoke(work->stage, work->order[i], poke);
            } else {
                MusicalProgram_PutNpcOnStage(work->program, work->stage, work->order[i], cpu, HEAPID_GAMEEVENT);
                cpu++;
            }
        }
    }
    work->stage->ov210 = work->ov210;
    work->stage->program = work->program;
    MusicalProgram_CalcPoints(HEAPID_GAMEEVENT, work->program, work->stage);
}

// Adds the points of the props the audience liked
static void MusicalEvent_AddLikedPropPoints(MusicalEventWork *work) {
    u8 i;
    u8 j;
    u8 slot;
    u8 kind;
    MusicalPoke *poke;
    void *items;

    if (work->online == TRUE) {
        for (i = 0; i < 4; i++) {
            poke = work->stage->pokes[i];
            for (j = 0; j < 2; j++) {
                slot = MusComm_GetPlayerEquip(work->comm, i, j);
                if (slot != 0xff) {
                    poke->unk54[slot] = TRUE;
                }
            }
        }
    }
    items = MusItemData_Init(HEAPID_GAMEEVENT);
    for (i = 0; i < 4; i++) {
        poke = work->stage->pokes[i];
        for (j = 0; j < 9; j++) {
            if (poke->unk54[j] == TRUE) {
                kind = MusItemData_GetKind(items, poke->equips[j].itemId);
                poke->points += MusicalProgram_GetPropPoints(work->program, kind);
            }
        }
    }
    MusItemData_Free(items);
}

// Takes the photo of the finale
static void MusicalEvent_TakeShot(MusicalEventWork *work) {
    RTCDate date;
    u8 points[4];
    MusicalShot *shot;
    StrBuf *str;
    MsgData *msgData;
    u32 msgId;
    PlayerInfo *info;
    MusicalPoke *poke;
    u8 max = 0;
    u8 value;
    u8 i;
    u8 j;
    u8 pos;
    u8 temp;
    u8 rank;
    u8 slot;

    work->shot = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(MusicalShotParam), FALSE, "musical_event.c", 798);
    work->shot->shot = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(MusicalShot), TRUE, "musical_event.c", 799);
    work->shot->loadComm = FALSE;
    work->shot->loadData = FALSE;
    work->shot->comm = work->comm;
    shot = work->shot->shot;
    RTC_GetCachedDate(&date);
    shot->unk0_0 = func_ov012_02152614(work->program);
    shot->year = date.year;
    shot->month = date.month;
    shot->day = date.day;
    str = MusicalEvent_GetProgramTitle(work, HEAPID_GAMEEVENT);
    GFL_StrBufStoreString(str, shot->title, 0x25);
    GFL_StrBufFree(str);
    shot->unk0_29 = 0;
    shot->unk1AE = 23;
    shot->unk1AF = 1;
    shot->player = work->playerPos;
    for (i = 0; i < 4; i++) {
        value = work->stage->pokes[i]->points;
        if (max < value) {
            max = value;
            shot->tops = 0;
        }
        if (max == value) {
            shot->tops += 1 << i;
        }
    }
    for (pos = 0; pos < 4; pos++) {
        info = NULL;
        poke = work->stage->pokes[pos];
        shot->pokes[pos].species = poke->species;
        shot->pokes[pos].sex = poke->sex;
        shot->pokes[pos].rare = poke->rare;
        shot->pokes[pos].form = poke->form;
        shot->pokes[pos].personality = poke->personality;
        if (pos == work->playerPos) {
            info = GetGameDataPlayerInfo(work->gameData);
        } else if (work->online == TRUE) {
            info = MusComm_GetPlayerInfo(work->comm, MusicalEvent_GetEntryIndex(work, pos));
        }
        if (info != NULL) {
            MI_CpuCopy8(info, shot->pokes[pos].name, sizeof(shot->pokes[pos].name));
        } else {
            msgId = MusicalProgram_GetNpcNameMsg(work->program,
                                                 MusicalEvent_GetNpcIndex(work, MusicalEvent_GetEntryIndex(work, pos)));
            msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_9, HEAPID_GAMEEVENT);
            str = GFL_MsgDataLoadStrbufNew(msgData, msgId);
            GFL_StrBufStoreString(str, shot->pokes[pos].name, 8);
            GFL_StrBufFree(str);
            GFL_MsgDataFree(msgData);
        }
        for (j = 0; j < 8; j++) {
            shot->pokes[pos].equips[j].itemId = 0xff;
            shot->pokes[pos].equips[j].rotation = 0;
            shot->pokes[pos].equips[j].pos = 10;
        }
        for (j = 0; j < 9; j++) {
            if (poke->equips[j].itemId != 0xff) {
                slot = poke->equips[j].slot;
                shot->pokes[pos].equips[slot].itemId = poke->equips[j].itemId;
                shot->pokes[pos].equips[slot].rotation = poke->equips[j].unk2;
                shot->pokes[pos].equips[slot].pos = j;
            }
        }
    }
    work->shot->askSave = TRUE;
    work->shot->save = getAddressOfMusicalDataInfo(GameData_GetSaveControl(GSYS_GetGameData(work->gsys)));
    for (i = 0; i < 4; i++) {
        points[i] = work->stage->pokes[i]->points;
        work->ranking[i] = i;
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4 - (i + 1); j++) {
            if (points[j + 1] > points[j]) {
                temp = points[j + 1];
                rank = work->ranking[j + 1];
                points[j + 1] = points[j];
                work->ranking[j + 1] = work->ranking[j];
                points[j] = temp;
                work->ranking[j] = rank;
            }
        }
    }
}

// Records the show in the save
static void MusicalEvent_RecordShow(MusicalEventWork *work) {
    u8 i;
    u8 points;
    u8 topPoints;
    void *items;
    MusicalPoke *poke;
    GameRecords *records;
    u16 zoneId;

    points = MusicalEvent_GetPoints(work, work->playerPos);
    topPoints = MusicalEvent_GetPoints(work, work->ranking[0]);
    MusicalSave_IncShowCount(work->musicalSave);
    if (points == topPoints && points != 0) {
        MusicalSave_IncWinCount(work->musicalSave);
    }
    MusicalSave_SetLastPoints(work->musicalSave, points);
    MusicalSave_AddTotalPoints(work->musicalSave, points);
    {
        u8 counts[4] = { 0 };

        items = MusItemData_Init(HEAPID_GAMEEVENT);
        poke = work->stage->pokes[work->playerPos];
        for (i = 0; i < 9; i++) {
            if (poke->equips[i].itemId != 0xff) {
                counts[(u8)MusItemData_GetKind(items, poke->equips[i].itemId)]++;
            }
        }
        for (i = 0; i < 4; i++) {
            MusicalSave_SetPropKindCount(work->musicalSave, i, counts[i]);
        }
    }
    MusItemData_Free(items);
    zoneId = GameData_GetPlayerState(work->gameData)->zoneId;
    FriendshipManagerCalc(work->pkm, 6, zoneId, HEAPID_GAMEEVENT);
    records = GameData_GetRecords(work->gameData);
    if (work->online == FALSE) {
        RecordAddOne(records, 0x72);
        if (points == topPoints && points != 0) {
            RecordAddOne(records, 0x73);
        }
    } else {
        RecordAddOne(records, 0x74);
        if (points == topPoints && points != 0) {
            RecordAddOne(records, 0x75);
        }
    }
    RecordAdd(records, 0x76, points);
}

// Draws the props the show gives away to the audience
static void MusicalEvent_DrawAudienceGifts(MusicalEventWork *work) {
    u8 *props;
    u32 total;
    u8 i;
    u8 count;
    u8 numProps;
    u8 program;
    u32 winner;
    u8 prop;
    u8 index;
    u8 temp;
    u8 order[10];
    u8 j;
    u16 *otherTotal;
    u32 chance;
    MusicalAudienceGift *entry;

    props = GFL_HeapAllocate(HEAPID_GAMEEVENT, 40, TRUE, "musical_event.c", 1041);
    total = 0;
    for (i = 0; i < 10; i++) {
        entry = MusicalSave_GetAudienceGift(work->musicalSave, i);
        entry->topKind = 4;
        entry->hasProp = 0;
        entry->prop = 0;
    }
    if (work->online == FALSE) {
        total = MusicalSave_GetTotalPoints(work->musicalSave) + MusicalEvent_GetPoints(work, work->playerPos);
    } else {
        for (i = 0; i < 4; i++) {
            otherTotal = MusComm_GetPlayerMusicalId(work->comm, MusicalEvent_GetEntryIndex(work, i));
            if (MusComm_GetPokeByPos(work->comm, i) != NULL) {
                total += *otherTotal;
                total += MusicalEvent_GetPoints(work, i);
            }
        }
    }
    for (i = 0; i < 10; i++) {
        if (data_ov012_0216ae08[i] <= total) {
            count = 10 - i;
            break;
        }
    }
    program = MusicalProgram_GetPropList(work->program);
    if (program < 7) {
        numProps = 0;
        for (i = 0; i < 40; i++) {
            prop = data_ov012_0216ae1c[program][i];
            if (MusicalSave_HasProp(work->musicalSave, prop) == FALSE) {
                props[numProps] = prop;
                numProps++;
            }
        }
    }
    winner = MusicalProgram_GetTopKind(work->program);
    for (i = 0; i < 10; i++) {
        order[i] = i;
    }
    for (i = 0; i < 10; i++) {
        for (j = 0; j < 10; j++) {
            index = GFL_RandomLCAlt(10);
            temp = order[j];
            order[j] = order[index];
            order[index] = temp;
        }
    }
    for (i = 0; i < count; i++) {
        entry = MusicalSave_GetAudienceGift(work->musicalSave, order[i]);
        chance = MusicalEvent_GetPoints(work, work->playerPos);
        entry->topKind = winner;
        if ((u8)GFL_RandomLCAlt(100) < chance && numProps != 0) {
            index = GFL_RandomLCAlt(numProps);
            entry->hasProp = 1;
            entry->prop = props[index];
            for (; index < numProps - 1; index++) {
                props[index] = props[index + 1];
            }
            numProps--;
        }
    }
    GFL_HeapFree(props);
}

// Goes back to the field
static BOOL MusicalEvent_ReturnToField(GameEvent *event, MusicalEventWork *work) {
    GameData_GetFieldSoundSystem(GSYS_GetGameData(work->gsys));
    switch (work->subState) {
    case 0:
        GameEvent_ChainNext(event, EventBGMPop_CreateEx(work->gsys, 0, 60));
        work->subState++;
        break;
    case 1:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(work->gsys));
        work->subState++;
        break;
    case 2:
        work->subState++;
        break;
    case 3:
        GameEvent_ChainNext(event, EventBGMFadeWait_Create(work->gsys));
        work->subState++;
        break;
    case 4:
        return TRUE;
    }
    return FALSE;
}

// Leaves the field
static BOOL MusicalEvent_LeaveField(GameEvent *event, MusicalEventWork *work) {
    Field *field;

    GameData_GetFieldSoundSystem(GSYS_GetGameData(work->gsys));
    field = GSYS_GetField(work->gsys);
    switch (work->subState) {
    case 0:
        work->subState++;
        break;
    case 1:
        GameEvent_ChainNext(event, EventBGMPushWait_Create(work->gsys, 30));
        work->subState++;
        break;
    case 2:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(work->gsys, field));
        work->subState++;
        break;
    case 3:
        GameEvent_ChainNext(event, EventWaitFieldSound_Create(work->gsys));
        work->subState++;
        break;
    case 4:
        return TRUE;
    }
    return FALSE;
}

// Warps to the dressing room, to the player's place in it
static void MusicalEvent_WarpToDressingRoom(GameEvent *event, MusicalEventWork *work) {
    Field *field = GSYS_GetField(work->gsys);
    VecFx32 pos = data_ov012_0216adf0;

    pos.x += work->playerPos * FX32_CONST(32);
    GameEvent_ChainNext(event, EventMapChange_CreateGridDefault(work->gsys, field, 78, &pos, 0));
}

// Warps back to the musical hall
static void MusicalEvent_WarpToHall(GameEvent *event, MusicalEventWork *work) {
    Field *field = GSYS_GetField(work->gsys);
    VecFx32 pos = data_ov012_0216ade4;
    VecFx32 onlinePos = data_ov012_0216adfc;
    GameEvent *next;

    if (work->online == TRUE) {
        next = EventMapChange_CreateGridDefault(work->gsys, field, 77, &onlinePos, 1);
    } else {
        next = EventMapChange_CreateGridDefault(work->gsys, field, 77, &pos, 1);
    }
    GameEvent_ChainNext(event, next);
}

static void MusicalEvent_CallScript(GameEvent *event, MusicalEventWork *work, u16 scriptId) {
    GameEvent *script = EventScriptCall_Create(work->gsys, scriptId, NULL, HEAPID_FIELDMAP);

    EventScriptCall_GetWork(script);
    GameEvent_ChainNext(event, script);
}

// The number of performers of the program before an entry
static u8 MusicalEvent_GetNpcIndex(MusicalEventWork *work, u8 pos) {
    u8 i;
    u8 count = 0;

    for (i = 0; i < pos; i++) {
        if (work->comm == NULL) {
            if (i != 0) {
                count++;
            }
        } else if (MusComm_GetPlayerInfo(work->comm, i) == NULL) {
            count++;
        }
    }
    return count;
}

static u32 MusicalEvent_GetPlayerTrainerClass(PlayerInfo *info) {
    u8 version = PlayerInfo_GetVersion(info);
    u32 gender = getTrainerGender(info);

    if (version == VERSION_WHITE2 || version == VERSION_BLACK2) {
        if (gender == 0) {
            return 0xe7;
        }
        return 0xf0;
    }
    if (gender == 0) {
        return 1;
    }
    return 4;
}

u8 MusicalEvent_GetPlayerPos(MusicalEventWork *work) {
    return work->playerPos;
}

u8 MusicalEvent_GetEntryIndex(MusicalEventWork *work, u8 pos) {
    u8 i;

    for (i = 0; i < 4; i++) {
        if (pos == work->order[i]) {
            return i;
        }
    }
    return 0;
}

u8 MusicalEvent_GetTopPropKind(MusicalEventWork *work) {
    return MusicalProgram_GetTopKind(work->program);
}

StrBuf *MusicalEvent_GetProgramTitle(MusicalEventWork *work, HeapID heapId) {
    MsgData *msgData = GFL_MsgDataCreateFromHandle(work->ov210->msgArc, heapId);
    StrBuf *str = GFL_MsgDataLoadStrbufNew(msgData, 0);

    GFL_MsgDataFree(msgData);
    return str;
}

u8 MusicalEvent_GetPoints(MusicalEventWork *work, u8 pos) {
    u16 points = work->stage->pokes[pos]->points;

    if (points < 0x100) {
        return points;
    }
    return 0xff;
}

u16 MusicalEvent_GetMaxPoints(MusicalEventWork *work) {
    u16 max = 0;
    u8 i;

    for (i = 0; i < 4; i++) {
        if (max < work->stage->pokes[i]->points) {
            max = work->stage->pokes[i]->points;
        }
    }
    return max;
}

u16 MusicalEvent_GetMinPoints(MusicalEventWork *work) {
    u16 min = 0xff;
    u8 i;

    for (i = 0; i < 4; i++) {
        if (min > work->stage->pokes[i]->points) {
            min = work->stage->pokes[i]->points;
        }
    }
    return min;
}

u8 MusicalEvent_GetPosAtRank(MusicalEventWork *work, u8 rank) {
    return work->ranking[rank];
}

u8 MusicalEvent_GetBestPropKind(MusicalEventWork *work, u8 pos) {
    u8 best;
    u8 i;
    u8 max;
    BOOL tie;
    MusicalPoke *poke;

    max = 0;
    best = 0;
    tie = FALSE;
    i = 0;
    poke = work->stage->pokes[pos];

    while (i < 4) {
        if (max < poke->unk4C[i]) {
            tie = FALSE;
            best = i;
            max = poke->unk4C[i];
        } else if (max == poke->unk4C[i]) {
            tie = TRUE;
        }
        i++;
    }
    if (tie == TRUE) {
        best = 4;
    }
    return best;
}

u8 MusicalEvent_GetOwnerTrainerClass(MusicalEventWork *work, u8 pos) {
    u8 index = MusicalEvent_GetEntryIndex(work, pos);
    PlayerInfo *info;

    if (pos == work->playerPos) {
        return 0;
    }
    if (work->online == TRUE) {
        info = MusComm_GetPlayerInfo(work->comm, index);
        if (info != NULL) {
            return MusicalEvent_GetPlayerTrainerClass(info);
        }
    }
    return MusicalProgram_GetNpcTrainerClass(work->program, MusicalEvent_GetNpcIndex(work, index));
}

void MusicalEvent_WordSetOwnerName(MusicalEventWork *work, u8 pos, WordSet *wordSet, u32 wordIndex) {
    BOOL done = FALSE;
    u8 index = MusicalEvent_GetEntryIndex(work, pos);
    PlayerInfo *info;
    u32 msgId;
    MsgData *msgData;
    StrBuf *name;

    if (pos == work->playerPos) {
        copyVarForText(wordSet, wordIndex, GetGameDataPlayerInfo(work->gameData));
        done = TRUE;
    } else if (work->online == TRUE) {
        info = MusComm_GetPlayerInfo(work->comm, index);
        if (info != NULL) {
            copyVarForText(wordSet, wordIndex, info);
            done = TRUE;
        }
    }
    if (!done) {
        msgId = MusicalProgram_GetNpcNameMsg(work->program, MusicalEvent_GetNpcIndex(work, index));
        msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_9, HEAPID_GAMEEVENT);
        name = GFL_MsgDataLoadStrbufNew(msgData, msgId);
        GFL_WordSetSetStrbuf(wordSet, wordIndex, name, 0, 1, 2);
        GFL_StrBufFree(name);
        GFL_MsgDataFree(msgData);
    }
}

void MusicalEvent_WordSetPokeName(MusicalEventWork *work, u8 pos, WordSet *wordSet, u32 wordIndex) {
    BOOL done = FALSE;
    u8 index = MusicalEvent_GetEntryIndex(work, pos);
    BoxPkm *pkm;
    u32 msgId;
    MsgData *msgData;
    StrBuf *name;

    if (pos == work->playerPos) {
        GetGameDataPlayerInfo(work->gameData);
        loadPokemonNicknameToStrbuf(wordSet, wordIndex, work->pkm);
        done = TRUE;
    } else if (work->online == TRUE) {
        pkm = MusComm_GetPlayerPkm(work->comm, index);
        if (pkm != NULL) {
            loadBoxPokemonNameToStrbuf(wordSet, wordIndex, pkm);
            done = TRUE;
        }
    }
    if (!done) {
        msgId = MusicalProgram_GetNpcNameMsg(work->program, MusicalEvent_GetNpcIndex(work, index));
        msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MUSICAL_EVENT, HEAPID_GAMEEVENT);
        name = GFL_MsgDataLoadStrbufNew(msgData, msgId);
        GFL_WordSetSetStrbuf(wordSet, wordIndex, name, 0, 1, 2);
        GFL_StrBufFree(name);
        GFL_MsgDataFree(msgData);
    }
}

static void MusicalEvent_SetReceivedProgram(MusicalEventWork *work) {
    u32 program = MusComm_GetUnk144(work->comm);
    u32 cpus = MusComm_GetUnk148(work->comm);

    if (work->program == NULL) {
        work->program = MusicalProgram_Create(HEAPID_GAMEEVENT, work->ov210, 0);
    }
    MusicalProgram_UnpackPoints(work->program, program);
    MusicalProgram_UnpackNpcs(work->program, cpus);
}

void MusicalEvent_CheckNetError(MusicalEventWork *work) {
    if (work->online == TRUE && GFL_NetErrCheck() && work->error == FALSE) {
        work->error = TRUE;
    }
}

BOOL MusicalEvent_IsNetError(MusicalEventWork *work) {
    return work->error;
}
