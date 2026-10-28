// Script commands of the field: a Pokémon shown on the screen, the C-Gear warning, the records, the TV and the event
// camera's shake. The ROM has no name for the file; scrcmd_poke_disp.c is descriptive, after its first command. It
// is a file of its own because overlay 36's .rodata has its 8-byte actor setup after the 62-byte answer list of
// scrcmd_pep_quiz.c, which one file's size sort can't give. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except Scrcmd_PokeDisp_IsOneShotDRObtained,
// Scrcmd_PokeDisp_TVGenProgramMsgID, Scrcmd_PokeDisp_StartSwapNpcEvent and Scrcmd_PokeDisp_JoinAvenueRecordActivity,
// which are ours
#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "field/field_actor.h"
#include "field/field_camera.h"
#include "field/scrcmd_poke_disp.h"
#include "field/sp_poke_event.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/random.h"
#include "pml/poke_graphic.h"
#include "pml/poke_party.h"
#include "save/hollow_rival_savedata.h"
#include "save/join_avenue.h"
#include "save/records.h"
#include "save/trainer_card.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/pms_word.h"
#include "system/vm.h"
#include "text/system/s019a_cgear_control_warning.h"

typedef struct {
    BmpWin *window;
    ClActUnit *unit;
    ClActor *actor;
    u32 chars;
    u32 palette;
    u32 cellAnims;
    u32 frames;
} EventFieldPokeDisp;

typedef struct {
    FieldTask *task;
    FieldTaskCallback callback;
    FieldCameraShakeSetup setup;
    u32 unk28;
} EventEvCameraShake;

// The steps of the Pokémon's display: show it for 16 frames, wait for A or B, then free its window
enum {
    POKE_DISP_STATE_DELAY,
    POKE_DISP_STATE_WAIT_INPUT,
    POKE_DISP_STATE_END,
};

// The TV programs whose messages Scrcmd_PokeDisp_TVGenProgramMsgID picks: Interview Unova, or Pokémon Idol Chat for the
// rest of its rolls; Moves for Living; and the Personality Assessment and Horoscope
enum {
    TV_PROGRAM_INTERVIEW,
    TV_PROGRAM_MOVES_FOR_LIVING,
    TV_PROGRAM_HOROSCOPE,
};

// The steps of the camera shake event: start the shake's task, then wait until it ends
enum {
    EV_CAMERA_SHAKE_STATE_START,
    EV_CAMERA_SHAKE_STATE_WAIT,
};

static void EventFieldPokeDisp_InitBmpWin(EventFieldPokeDisp *data);
static void EventFieldPokeDisp_UpdateBmpWin(EventFieldPokeDisp *data);
static void EventFieldPokeDisp_InitClAct(EventFieldPokeDisp *data, u32 species, u32 form, u32 sex, u32 rare,
                                         u32 personality);
static void EventFieldPokeDisp_UpdateClAct(EventFieldPokeDisp *data);
static GameEventReturnCode EventFieldPokeDisp_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventEvCameraShake_Callback(GameEvent *event, u32 *state, void *data);

BOOL Scrcmd_PokeDisp_CallPokemonPreview(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 species;
    u16 form;
    u16 sex;
    u16 rare;
    ScriptWork *work;
    GameEvent *event;
    EventFieldPokeDisp *data;

    GSYS_GetField(gsys);
    species = ScriptReadAny(vm, env);
    form = ScriptReadAny(vm, env);
    sex = ScriptReadAny(vm, env);
    rare = ScriptReadAny(vm, env);
    work = FieldScriptEnv_GetScriptWork(env);
    event = GameEvent_Create(gsys, NULL, EventFieldPokeDisp_Callback, sizeof(EventFieldPokeDisp));
    data = GameEvent_GetData(event);
    data->frames = 0;
    EventFieldPokeDisp_InitBmpWin(data);
    EventFieldPokeDisp_InitClAct(data, species, form, sex, rare, 0);
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static void EventFieldPokeDisp_InitBmpWin(EventFieldPokeDisp *data) {
    data->window = BmpWin_CreateDynamic(1, 10, 3, 12, 12, 10, 1);
    GFL_BitmapFill(BmpWin_GetBitmap(data->window), 0x11);
    BmpWin_FlushChar(data->window);
    BmpWin_FlushMap(data->window);
    BmpWin_DrawFrame(data->window, 1, 1, 10);
}

static void EventFieldPokeDisp_UpdateBmpWin(EventFieldPokeDisp *data) {
    BmpWin_ClearFrame(data->window, 1);
}

static void EventFieldPokeDisp_InitClAct(EventFieldPokeDisp *data, u32 species, u32 form, u32 sex, u32 rare,
                                         u32 personality) {
    ArcTool *arc;

    data->unit = ClActUnit_Create(1, 0, HEAPID_TAIL(HEAPID_FIELDMAP));
    arc = MakePokeGraArcHandle(HEAPID_TAIL(HEAPID_FIELDMAP));
    data->chars =
        PokeGra_LoadClActChars(arc, species, form, sex, rare, 0, 0, personality, 0, HEAPID_TAIL(HEAPID_FIELDMAP));
    data->palette =
        PokeGra_LoadClActPalette(arc, species, form, sex, rare, 0, 0, 0, 0xc0, HEAPID_TAIL(HEAPID_FIELDMAP));
    GFL_ArcToolFree(arc);
    data->cellAnims = PokeGra_LoadClActCellAnims(species, form, sex, rare, 0, 0, 1, 0, HEAPID_TAIL(HEAPID_FIELDMAP));
    {
        ClActorSetup setup = { 0x80, 0x48, 0, 0, 0 };

        data->actor = ClActor_Create(data->unit, data->chars, data->palette, data->cellAnims, &setup, 0,
                                     HEAPID_TAIL(HEAPID_FIELDMAP));
    }
}

static void EventFieldPokeDisp_UpdateClAct(EventFieldPokeDisp *data) {
    ClActor_Delete(data->actor);
    ClActSys_FreeChars(data->chars);
    ClActSys_FreePltt(data->palette);
    ClActSys_FreeCellAnims(data->cellAnims);
    ClActUnit_Delete(data->unit);
}

static GameEventReturnCode EventFieldPokeDisp_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldPokeDisp *disp = data;

    switch (*state) {
    case POKE_DISP_STATE_DELAY:
        disp->frames++;
        if (disp->frames > 15) {
            (*state)++;
        }
        break;
    case POKE_DISP_STATE_WAIT_INPUT:
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            EventFieldPokeDisp_UpdateClAct(disp);
            EventFieldPokeDisp_UpdateBmpWin(disp);
            (*state)++;
        }
        break;
    case POKE_DISP_STATE_END:
        BmpWin_Free(disp->window);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

BOOL Scrcmd_PokeDisp_IsOneShotDRObtained(VM *vm, FieldScriptEnv *env) {
    u16 flag = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    GameData *gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    TrainerCardSave *card = getTrainerCardDataBlkAddress(gameData);

    *result = isOneShotDRObtained(card, flag, GetGameDataPlayerInfo(gameData));
    return FALSE;
}

BOOL Scrcmd_PokeDisp_CGearControlWarning(VM *vm, FieldScriptEnv *env) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    HeapID heapId = Field_GetHeapID(fieldWork->field);
    FieldMsgBG *msgBGSys = Field_GetMsgBGSys(fieldWork->field);

    if (VM_Read16(vm)) {
        MsgData *msgData =
            GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_S019A_CGEAR_CONTROL_WARNING, heapId);
        BmpWin *window = BmpWin_CreateStatic(2, 1, 1, 30, 10, 14, 0x40);
        StrBuf *strbuf;

        GFL_BitmapFill(BmpWin_GetBitmap(window), 0xf);
        BmpWin_FlushChar(window);
        BmpWin_FlushMap(window);
        BmpWin_DrawFrame(window, 0, 1, 13);
        strbuf = GFL_StrBufCreate(180, heapId);
        GFL_MsgDataLoadStrbuf(msgData, S019aCgearControlWarning_Text_WarningPleaseTurnC, strbuf);
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 0, 0, strbuf, MsgBGPrinter_GetFont(msgBGSys));
        BmpWin_FlushChar(window);
        GFL_BGSysQueueScrLoad(2);
        GFL_StrBufFree(strbuf);
        GFL_MsgDataFree(msgData);
        BmpWin_Free(window);
    } else {
        GFL_BGSysFillScrArea(2, 0, 0, 0, 32, 12, 0x11);
        GFL_BGSysQueueScrLoad(2);
    }
    return FALSE;
}

BOOL Scrcmd_PokeDisp_RecordAdd(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 id = ScriptReadAny(vm, env);
    u16 value = ScriptReadAny(vm, env);

    RecordAdd(getTrainerCardInfoBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(gsys))), id, value);
    return FALSE;
}

BOOL Scrcmd_PokeDisp_RecordGet(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 id = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = RecordGet(getTrainerCardInfoBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(gsys))), id);
    return FALSE;
}

BOOL Scrcmd_PokeDisp_JoinAvenueRecordActivity(VM *vm, FieldScriptEnv *env) {
    JoinAvenue_RecordActivity(VM_Read16(vm));
    return FALSE;
}

BOOL Scrcmd_PokeDisp_TVGenProgramMsgID(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameData *gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
    WordSet *wordSet = ScriptWork_GetWordSet(work);
    u16 kind = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    PokeParty *party;
    HollowRivalSave *rival;
    int i;
    int count;
    u32 base;
    u32 value;

    copyVarForText(wordSet, 0, playerInfo);
    i = 0;
    party = GameData_GetParty(gameData);
    count = PokeParty_GetPkmCount(party);
    for (; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == 0) {
            WordSet_LoadSpeciesName(wordSet, 3, (u16)PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL));
            break;
        }
    }
    if (i == count) {
        WordSet_LoadSpeciesName(wordSet, 3, SPECIES_CINCCINO);
    }
    rival = getHollow_RivalBlk(GameData_GetSaveControl(gameData));
    if (HollowRivalSave_HasSpecies(rival) == TRUE) {
        WordSet_LoadSpeciesName(wordSet, 1, HollowRivalSave_GetSpecies(rival));
    }
    switch (kind) {
    case TV_PROGRAM_INTERVIEW:
        base = 0;
        value = GFL_RandomLC(0x24);
        if (value >= 0x16) {
            value -= 0x16;
            if ((u8)getTrainerGender(playerInfo) == GENDER_MALE) {
                base = 0x24;
            } else {
                base = 0x16;
            }
            if (value >= 10) {
                rival = getHollow_RivalBlk(GameData_GetSaveControl(gameData));
                if (value == 11) {
                    if (HollowRivalSave_IsMet(rival) == FALSE) {
                        value--;
                    }
                } else if (value == 10) {
                    if (HollowRivalSave_IsMet(rival) == TRUE) {
                        value++;
                    }
                }
                if (value == 13) {
                    if (HollowRivalSave_HasSpecies(rival) == FALSE) {
                        value--;
                    }
                } else if (value == 12) {
                    if (HollowRivalSave_HasSpecies(rival) == TRUE) {
                        value++;
                    }
                }
            }
        }
        break;
    case TV_PROGRAM_MOVES_FOR_LIVING:
        base = 0x32;
        value = GFL_RandomLC(0x32);
        break;
    case TV_PROGRAM_HOROSCOPE:
        base = 0x64;
        value = GFL_RandomLC(0x50);
        break;
    default:
        *result = 0;
        return FALSE;
    }
    *result = base + value;
    return FALSE;
}

BOOL Scrcmd_PokeDisp_TVCheckCommercial(VM *vm, FieldScriptEnv *env) {
    BOOL show = FALSE;
    u16 rare = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    u32 roll = GFL_RandomLC(100);

    if (rare) {
        if (roll < 10) {
            show = TRUE;
        }
    } else if (roll < 20) {
        show = TRUE;
    }
    if (show == TRUE) {
        *result = TRUE;
    } else {
        *result = FALSE;
    }
    return FALSE;
}

BOOL Scrcmd_PokeDisp_TVGenCommercialMsgID(VM *vm, FieldScriptEnv *env) {
    u16 *result;

    FieldScriptEnv_GetScriptWork(env);
    GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    result = ScriptReadVar(vm, env);
    *result = GFL_RandomLC(8) + 0xb4;
    return FALSE;
}

GameEvent *EventEvCameraShake_Create(GameSystem *gsys, FieldCameraShakeSetup *setup) {
    GameEvent *event;
    EventEvCameraShake *data;

    setup->delay = 0;
    setup->mode = CAMERA_SHAKE_BOTH;
    event = GameEvent_Create(gsys, NULL, EventEvCameraShake_Callback, sizeof(EventEvCameraShake));
    data = GameEvent_GetData(event);
    data->setup = *setup;
    return event;
}

static GameEventReturnCode EventEvCameraShake_Callback(GameEvent *event, u32 *state, void *data) {
    Field *field = GSYS_GetField(GameEvent_GetGameSystem(event));
    EventEvCameraShake *shake = GameEvent_GetData(event);
    FieldCameraShakeSetup *setup = &shake->setup;

    switch (*state) {
    case EV_CAMERA_SHAKE_STATE_START:
        shake->task = FieldCameraShakeTask_Create(field, setup);
        shake->callback = FieldTask_GetCallback(shake->task);
        FieldTaskManager_AddTask(Field_GetTaskManager(field), shake->task, 0);
        (*state)++;
        break;
    case EV_CAMERA_SHAKE_STATE_WAIT:
        if (!FieldTaskManager_CheckCallbackRegistered(Field_GetTaskManager(field), shake->callback)) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

BOOL Scrcmd_PokeDisp_StartSwapNpcEvent(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 actorId;
    u16 uid;
    VecFx32 pos;
    GameEvent *event;

    GSYS_GetField(gsys);
    actorId = ScriptReadAny(vm, env);
    uid = ScriptReadAny(vm, env);
    CopyActorWPos(FindFieldActor(GetScrEnvActorSys(env), actorId), &pos);
    event = SpPokeEvent_CreateAppear(gsys, &pos, actorId, uid);
    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}
