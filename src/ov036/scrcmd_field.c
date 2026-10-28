// Script commands of the field: its subscreen, a check of a sentence's two words against two set phrases and the
// ambient sounds. The ROM has no name for the file; scrcmd_field.c is descriptive. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "constants/pms_words.h"
#include "constants/sound.h"
#include "field/field_ambience.h"
#include "field/scrcmd_field.h"
#include "field/subscreen.h"
#include "system/pms_word.h"

typedef struct {
    FieldSubscreen *subscreen;
    u32 mode;
} EventFieldSubscreenChange;

typedef struct {
    u32 se;
    BOOL looped;
} FieldAmbientSound;

static const u32 SCRIPTED_SUBSCREEN_LUT[1] = { FLD_SUBSCREEN_ID_SAVE_REPORT };

static const FieldAmbientSound FIELD_AMBIENT_SOUNDS[] = {
    { SEQ_SE_FLD_25, TRUE },   { SEQ_SE_FLD_31, TRUE },         { SEQ_SE_FLD_32, TRUE },   { SEQ_SE_FLD_33, FALSE },
    { SEQ_SE_FLD_34, FALSE },  { SEQ_SE_FLD_70, FALSE },        { SEQ_SE_FLD_71, FALSE },  { SEQ_SE_FLD_72, FALSE },
    { SEQ_SE_FLD_73, FALSE },  { SEQ_SE_FLD_84, TRUE },         { SEQ_SE_FLD_85, TRUE },   { SEQ_SE_FLD_98, FALSE },
    { SEQ_SE_FLD_99, FALSE },  { SEQ_SE_FLD_108, FALSE },       { SEQ_SE_FLD_111, TRUE },  { SEQ_SE_FLD_116, TRUE },
    { SEQ_SE_FLD_130, FALSE }, { SEQ_SE_FLD_174, TRUE },        { SEQ_SE_FLD_140, FALSE }, { SEQ_SE_FLD_141, FALSE },
    { SEQ_SE_GYM_H02, TRUE },  { SEQ_SE_SW_CATTLEYA_03, TRUE },
};

// The steps of a subscreen change: request it once the subscreen is ready, then wait until it is ready again
enum {
    SUBSCREEN_CHANGE_STATE_REQUEST,
    SUBSCREEN_CHANGE_STATE_WAIT,
};

// The word pairs that Scrcmd_Field_CheckPmsWordPair checks a sentence's two words against, named by their words
enum {
    PMS_WORD_PAIR_EVERYONE_HAPPY = 1,
    PMS_WORD_PAIR_SIMPLE_CONNECTION,
};

static GameEventReturnCode EventFieldSubscreenChange_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldSubscreenChange *change = data;

    switch (*state) {
    case SUBSCREEN_CHANGE_STATE_REQUEST:
        if (FieldSubscreen_IsReady(change->subscreen)) {
            FieldSubscreen_ReqChange(change->subscreen, change->mode);
            (*state)++;
        }
        break;
    case SUBSCREEN_CHANGE_STATE_WAIT:
        if (FieldSubscreen_IsReady(change->subscreen)) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

BOOL Scrcmd_Field_FieldSubscreenChange(VM *vm, FieldScriptEnv *env) {
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
    FieldScriptSubEvent_Register(SCRIPT_SUB_EVENT_SUBSCREEN);
    return TRUE;
}

BOOL Scrcmd_Field_FieldSubscreenReturn(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    FieldSubscreen *subscreen = Field_GetSubscreen(ScriptWork_GetFieldWork(work)->field);
    GameEvent *event = GameEvent_Create(FieldScriptEnv_GetGameSystem(env), NULL, EventFieldSubscreenChange_Callback,
                                        sizeof(EventFieldSubscreenChange));
    EventFieldSubscreenChange *change = GameEvent_GetData(event);

    change->subscreen = subscreen;
    change->mode = FLD_SUBSCREEN_ID_DEFAULT;
    ScriptWork_CallEvent(work, event);
    FieldScriptSubEvent_Unregister(SCRIPT_SUB_EVENT_SUBSCREEN);
    return TRUE;
}

BOOL FieldScriptSubEventFinish_Subscreen(FinishScriptSubEventsWork *work, u32 *state) {
    ScriptFieldWork *fieldWork = ScriptWork_GetFieldWork(work->scriptWork);
    FieldSubscreen *subscreen = Field_GetSubscreen(fieldWork->field);

    switch (*state) {
    case SUBSCREEN_CHANGE_STATE_REQUEST:
        if (FieldSubscreen_IsReady(subscreen)) {
            FieldSubscreen_ReqChange(subscreen, FLD_SUBSCREEN_ID_DEFAULT);
            FieldScriptSubEvent_Unregister(SCRIPT_SUB_EVENT_SUBSCREEN);
            (*state)++;
        }
        break;
    case SUBSCREEN_CHANGE_STATE_WAIT:
        if (FieldSubscreen_IsReady(subscreen)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

BOOL Scrcmd_Field_CheckPmsWordPair(VM *vm, FieldScriptEnv *env) {
    u16 kind = ScriptReadAny(vm, env);
    u16 word1 = ScriptReadAny(vm, env);
    u16 word2 = ScriptReadAny(vm, env);
    u16 *result = ScriptReadVar(vm, env);
    u16 first;
    u16 second;

    switch (kind) {
    case PMS_WORD_PAIR_EVERYONE_HAPPY:
        first = PMS_WORD_PEOPLE_EVERYONE;
        second = PMS_WORD_FEELINGS_HAPPY;
        break;
    case PMS_WORD_PAIR_SIMPLE_CONNECTION:
        first = PMS_WORD_FEELINGS_SIMPLE;
        second = PMS_WORD_CONNECTION_CONNECTION;
        break;
    default:
        *result = FALSE;
        return FALSE;
    }
    if (PMSWord_AreEquivalent(first, word1 & PMS_WORD_INDEX_MASK) &&
        PMSWord_AreEquivalent(second, word2 & PMS_WORD_INDEX_MASK)) {
        *result = TRUE;
    } else {
        *result = FALSE;
    }
    return FALSE;
}

BOOL FieldAmbience_CheckSoundID(u32 se) {
    u32 i;

    for (i = 0; i < NELEMS(FIELD_AMBIENT_SOUNDS); i++) {
        if (se == FIELD_AMBIENT_SOUNDS[i].se) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL FieldAmbience_IsLooped(u32 se) {
    u32 i;

    for (i = 0; i < NELEMS(FIELD_AMBIENT_SOUNDS); i++) {
        if (se == FIELD_AMBIENT_SOUNDS[i].se && FIELD_AMBIENT_SOUNDS[i].looped == TRUE) {
            return TRUE;
        }
    }
    return FALSE;
}
