// Script commands of the field: its subscreen, the ambient sounds, the Pep quiz, a Pokémon shown on the screen, the
// C-Gear warning, the records, the TV and the event camera's shake. The ROM has no name for the file; scrcmd_field.c
// is descriptive. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/pokemon.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_ambience.h"
#include "field/field_camera.h"
#include "field/field_script.h"
#include "field/field_task.h"
#include "field/scrcmd_field.h"
#include "field/sp_poke_event.h"
#include "field/subscreen.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/random.h"
#include "gfl/str.h"
#include "pml/poke_graphic.h"
#include "pml/poke_party.h"
#include "save/adventure.h"
#include "save/hollow_rival_savedata.h"
#include "save/join_avenue.h"
#include "save/player_info.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/trainer_card.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/pms_word.h"
#include "system/printsys.h"
#include "system/vm.h"
#include "system/wordset.h"

typedef struct {
    FieldSubscreen *subscreen;
    u32 mode;
} EventFieldSubscreenChange;

typedef struct {
    u32 se;
    BOOL looped;
} FieldAmbientSound;

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

static const u32 SCRIPTED_SUBSCREEN_LUT[1] = { 7 };

static const FieldAmbientSound FIELD_AMBIENT_SOUNDS[22] = {
    { 0x68a, TRUE },  { 0x690, TRUE },  { 0x691, TRUE },  { 0x692, FALSE }, { 0x693, FALSE }, { 0x6d5, FALSE },
    { 0x6d6, FALSE }, { 0x6d7, FALSE }, { 0x6d8, FALSE }, { 0x6e3, TRUE },  { 0x6e4, TRUE },  { 0x71e, FALSE },
    { 0x71f, FALSE }, { 0x742, FALSE }, { 0x745, TRUE },  { 0x755, TRUE },  { 0x76d, FALSE }, { 0x7ee, TRUE },
    { 0x77d, FALSE }, { 0x77e, FALSE }, { 0x819, TRUE },  { 0x88f, TRUE },
};

static GameEventReturnCode EventFieldSubscreenChange_Callback(GameEvent *event, u32 *state, void *data);
static void EventFieldPokeDisp_InitBmpWin(EventFieldPokeDisp *data);
static void EventFieldPokeDisp_UpdateBmpWin(EventFieldPokeDisp *data);
static void EventFieldPokeDisp_InitClAct(EventFieldPokeDisp *data, u32 species, u32 form, u32 sex, BOOL rare,
                                         u32 personality);
static void EventFieldPokeDisp_UpdateClAct(EventFieldPokeDisp *data);
static GameEventReturnCode EventFieldPokeDisp_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventEvCameraShake_Callback(GameEvent *event, u32 *state, void *data);

static GameEventReturnCode EventFieldSubscreenChange_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldSubscreenChange *change = data;

    switch (*state) {
    case 0:
        if (FieldSubscreen_IsReady(change->subscreen)) {
            FieldSubscreen_ReqChange(change->subscreen, change->mode);
            (*state)++;
        }
        break;
    case 1:
        if (FieldSubscreen_IsReady(change->subscreen)) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

BOOL s0198_FieldSubscreenChange(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work);
    u16 index = ScriptReadAny(vm, env);
    FieldSubscreen *subscreen = Field_GetSubscreen(fieldWork->field);
    GameEvent *event = GameEvent_Create(FieldScriptEnv_GetGameSystem(env), NULL, EventFieldSubscreenChange_Callback,
                                        sizeof(EventFieldSubscreenChange));
    EventFieldSubscreenChange *change = GameEvent_GetData(event);

    change->subscreen = subscreen;
    change->mode = SCRIPTED_SUBSCREEN_LUT[index];
    ScriptWork_CallEvent(work, event);
    FieldScriptSubEvent_Register(8);
    return TRUE;
}

BOOL s0199_FieldSubscreenReturn(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    FieldSubscreen *subscreen = Field_GetSubscreen(((ScriptFieldWork *)ScriptWork_GetFieldWork(work))->field);
    GameEvent *event = GameEvent_Create(FieldScriptEnv_GetGameSystem(env), NULL, EventFieldSubscreenChange_Callback,
                                        sizeof(EventFieldSubscreenChange));
    EventFieldSubscreenChange *change = GameEvent_GetData(event);

    change->subscreen = subscreen;
    change->mode = 0;
    ScriptWork_CallEvent(work, event);
    FieldScriptSubEvent_Unregister(8);
    return TRUE;
}

BOOL FieldScriptSubEventFinish_Subscreen(FinishScriptSubEventsWork *work, u32 *state) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work->scriptWork);
    FieldSubscreen *subscreen = Field_GetSubscreen(fieldWork->field);

    switch (*state) {
    case 0:
        if (FieldSubscreen_IsReady(subscreen)) {
            FieldSubscreen_ReqChange(subscreen, 0);
            FieldScriptSubEvent_Unregister(8);
            (*state)++;
        }
        break;
    case 1:
        if (FieldSubscreen_IsReady(subscreen)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL func_ov036_021c730c(VM *vm, FieldScriptEnv *env) {
    u16 kind = ScriptReadAny(vm, env);
    u16 word1 = ScriptReadAny(vm, env);
    u16 word2 = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    u32 word;

    switch (kind) {
    case 1:
        word = 0x67b;
        break;
    case 2:
        word = 0x6b7;
        break;
    default:
        *result = FALSE;
        return FALSE;
    }
    if (PMSWord_AreEquivalent(word, word1 & 0x7ff) && PMSWord_AreEquivalent(word, word2 & 0x7ff)) {
        *result = TRUE;
    } else {
        *result = FALSE;
    }
    return FALSE;
}

BOOL FieldAmbience_CheckSoundID(u32 se) {
    u32 i;

    for (i = 0; i < 22; i++) {
        if (se == FIELD_AMBIENT_SOUNDS[i].se) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL FieldAmbience_IsLooped(u32 se) {
    u32 i;

    for (i = 0; i < 22; i++) {
        if (se == FIELD_AMBIENT_SOUNDS[i].se && FIELD_AMBIENT_SOUNDS[i].looped == TRUE) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL s01FA_PepQuizGenerate(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u16 hard = ScriptReadAny(vm, env);
    u16 *question = ScriptReadVar(vm, env);
    u16 *answerMsg = ScriptReadVar(vm, env);
    u16 *answer = ScriptReadVar(vm, env);
    u16 answers[31] = {
        0x6a1, 0x5d6, 0x699, 0x5fe, 0x687, 0x5a3, 0x624, 0x6a3, 0x61a, 0x6b5, 0x60b, 0x602, 0x6a0, 0x57d, 0x249, 0x20a,
        0x257, 0x24b, 0x253, 0x208, 0x1ff, 0x213, 0x233, 0x210, 0x22b, 0x252, 0x206, 0x245, 0x26a, 0x2c5, 0x62b,
    };
    AdventureSave *adventure = getSaveAdventureDataBlk(GameData_GetSaveControl(gameData));
    u32 month = GameData_GetMonth(gameData);
    u32 day = GameData_GetDay(gameData);
    u32 birthMonth = getOSBirthdayMonth(adventure);

    if (day == getOSBirthdayDay(adventure) && month == birthMonth) {
        *question = 0x34;
        *answerMsg = 0x53;
        *answer = answers[30];
    } else {
        u32 index = GFL_RandomLC(hard ? 0x1e : 0x16);

        *question = index + 0x16;
        *answerMsg = index + 0x35;
        *answer = answers[index];
    }
    return FALSE;
}

BOOL s0214_CallPokemonPreview(VM *vm, FieldScriptEnv *env) {
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

static void EventFieldPokeDisp_InitClAct(EventFieldPokeDisp *data, u32 species, u32 form, u32 sex, BOOL rare,
                                         u32 personality) {
    ArcTool *arc;

    data->unit = func_0204bf1c(1, 0, 0x8015);
    arc = MakePokeGraArcHandle(0x8015);
    data->chars = PokeGra_LoadClActChars(arc, species, form, sex, rare, 0, 0, personality, 0, 0x8015);
    data->palette = PokeGra_LoadClActPalette(arc, species, form, sex, rare, 0, 0, 0, 0xc0, 0x8015);
    GFL_ArcToolFree(arc);
    data->cellAnims = PokeGra_LoadClActCellAnims(species, form, sex, rare, 0, 0, 1, 0, 0x8015);
    {
        ClActorSetup setup = { 0x80, 0x48, 0, 0, 0 };

        data->actor = func_0204c040(data->unit, data->chars, data->palette, data->cellAnims, &setup, 0, 0x8015);
    }
}

static void EventFieldPokeDisp_UpdateClAct(EventFieldPokeDisp *data) {
    func_0204c108(data->actor);
    func_0204b98c(data->chars);
    func_0204bcd0(data->palette);
    func_0204be64(data->cellAnims);
    func_0204bf98(data->unit);
}

static GameEventReturnCode EventFieldPokeDisp_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldPokeDisp *disp = data;

    switch (*state) {
    case 0:
        disp->frames++;
        if (disp->frames > 15) {
            (*state)++;
        }
        break;
    case 1:
        if (GCTX_HIDGetPressedKeys() & 3) {
            EventFieldPokeDisp_UpdateClAct(disp);
            EventFieldPokeDisp_UpdateBmpWin(disp);
            (*state)++;
        }
        break;
    case 2:
        BmpWin_Free(disp->window);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

BOOL func_ov036_021c7698(VM *vm, FieldScriptEnv *env) {
    u16 flag = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    GameData *gameData = GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env));
    TrainerCardSave *card = getTrainerCardDataBlkAddress(gameData);

    *result = isOneShotDRObtained(card, flag, GetGameDataPlayerInfo(gameData));
    return FALSE;
}

BOOL s019A_CGearControlWarning(VM *vm, FieldScriptEnv *env) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(FieldScriptEnv_GetScriptWork(env));
    HeapID heapId = Field_GetHeapID(fieldWork->field);
    void *msgBGSys = Field_GetMsgBGSys(fieldWork->field);

    if (VM_Read16(vm)) {
        MsgData *msgData = GFL_MsgSysLoadData(FALSE, 2, 0x19, heapId);
        BmpWin *window = BmpWin_CreateStatic(2, 1, 1, 30, 10, 14, 0x40);
        StrBuf *strbuf;

        GFL_BitmapFill(BmpWin_GetBitmap(window), 0xf);
        BmpWin_FlushChar(window);
        BmpWin_FlushMap(window);
        BmpWin_DrawFrame(window, 0, 1, 13);
        strbuf = GFL_StrBufCreate(180, heapId);
        GFL_MsgDataLoadStrbuf(msgData, 3, strbuf);
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 0, 0, strbuf, func_ov036_0218799c(msgBGSys));
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

BOOL s0227_RecordAdd(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 id = ScriptReadAny(vm, env);
    u16 value = ScriptReadAny(vm, env);

    RecordAdd(getTrainerCardInfoBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(gsys))), id, value);
    return FALSE;
}

BOOL s0228_RecordGet(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 id = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);

    *result = RecordGet(getTrainerCardInfoBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(gsys))), id);
    return FALSE;
}

BOOL func_ov036_021c783c(VM *vm, FieldScriptEnv *env) {
    JoinAvenue_RecordActivity(VM_Read16(vm));
    return FALSE;
}

BOOL func_ov036_021c784c(VM *vm, FieldScriptEnv *env) {
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
        WordSet_LoadSpeciesName(wordSet, 3, 0x23d);
    }
    rival = getHollow_RivalBlk(GameData_GetSaveControl(gameData));
    if (func_0200ffa8(rival) == TRUE) {
        WordSet_LoadSpeciesName(wordSet, 1, func_0200ffb4(rival));
    }
    switch (kind) {
    case 0:
        base = 0;
        value = GFL_RandomLC(0x24);
        if (value >= 0x16) {
            value -= 0x16;
            if ((u8)getTrainerGender(playerInfo) == 0) {
                base = 0x24;
            } else {
                base = 0x16;
            }
            if (value >= 10) {
                rival = getHollow_RivalBlk(GameData_GetSaveControl(gameData));
                if (value == 11) {
                    if (func_0200ffcc(rival) == FALSE) {
                        value--;
                    }
                } else if (value == 10) {
                    if (func_0200ffcc(rival) == TRUE) {
                        value++;
                    }
                }
                if (value == 13) {
                    if (func_0200ffa8(rival) == FALSE) {
                        value--;
                    }
                } else if (value == 12) {
                    if (func_0200ffa8(rival) == TRUE) {
                        value++;
                    }
                }
            }
        }
        break;
    case 1:
        base = 0x32;
        value = GFL_RandomLC(0x32);
        break;
    case 2:
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

BOOL s02BC_TVCheckCommercial(VM *vm, FieldScriptEnv *env) {
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

BOOL s02BD_TVGenCommercialMsgID(VM *vm, FieldScriptEnv *env) {
    u16 *result;

    FieldScriptEnv_GetScriptWork(env);
    GameData_GetSaveControl(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env)));
    result = ScriptReadVar(vm, env);
    *result = GFL_RandomLC(8) + 0xb4;
    return FALSE;
}

GameEvent *EventEvCameraShake_Create(GameSystem *gsys, FieldEvCameraShake *shake) {
    // The script's shake is the camera task's setup
    FieldCameraShakeSetup *setup = (FieldCameraShakeSetup *)shake;
    GameEvent *event;
    EventEvCameraShake *data;

    setup->delay = 0;
    setup->mode = 0;
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
    case 0:
        shake->task = FieldCameraShakeTask_Create(field, setup);
        shake->callback = FieldTask_GetCallback(shake->task);
        FieldTaskManager_AddTask(Field_GetTaskManager(field), shake->task, 0);
        (*state)++;
        break;
    case 1:
        if (!FieldTaskManager_CheckCallbackRegistered(Field_GetTaskManager(field), shake->callback)) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

BOOL func_ov036_021c7ad0(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 actorId;
    u16 uid;
    VecFx32 pos;
    GameEvent *event;

    GSYS_GetField(gsys);
    actorId = ScriptReadAny(vm, env);
    uid = ScriptReadAny(vm, env);
    CopyActorWPos(FindFieldActor(GetScrEnvMMdlSys(env), actorId), &pos);
    event = SpPokeEvent_CreateAppear(gsys, &pos, actorId, uid);
    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}
