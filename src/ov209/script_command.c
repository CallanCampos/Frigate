#include "types.h"
#include "app/musical/sta_act_effect.h"
#include "app/musical/sta_act_light.h"
#include "app/musical/sta_act_obj.h"
#include "app/musical/sta_act_poke.h"
#include "app/musical/sta_act_script.h"
#include "app/musical/sta_acting.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/math.h"
#include "nnsys/snd.h"
#include "system/gf_font.h"
#include "system/vm.h"

// Overlay 209's script_command.c: the commands of the stage's scripts. Each reads its arguments from the script as
// 32-bit words. A command that moves something over time starts a task, which frees its work when it is done, and
// most of them can make the script wait for it

// The target of StaScript_GetPokeMask that means the script's own Pokémon
#define STA_SCRIPT_OWN_POKE ((u32) - 1)

// Frames of each hop of the Pokémon as they line up at the end
#define STA_SCRIPT_LINE_UP_JUMP_PERIOD 15

// The file name of the allocations, which the original has in .data ahead of the command table rather than as
// string literals after it
static char sFile[] = "script_command.c";

// A value that goes from start to end over frames
typedef struct {
    StaActing *stage;
    int start;
    int end;
    u16 frames;
    u16 count;
} StaScriptMoveValue;

// A position that goes from start to end over frames, by step each frame
typedef struct {
    StaActing *stage;
    VecFx32 start;
    VecFx32 end;
    VecFx32 step;
    u16 frames;
    u16 count;
} StaScriptMoveVec;

// A position that follows a Pokémon at an offset for frames
typedef struct {
    StaActing *stage;
    StaActPoke *poke;
    VecFx32 offset;
    u16 frames;
    u16 count;
} StaScriptFollow;

// A period that repeats a number of times
typedef struct {
    StaActing *stage;
    u32 count;
    u32 period;
    u32 repeat;
} StaScriptRepeat;

// The work of the curtain and scroll tasks
typedef struct {
    StaScriptMoveValue move;
    StaActScriptSys *sys;
    StaActScriptTask *task;
} StaScriptValueWork;

// The work of the tasks that move a Pokémon, an object or a light
typedef struct {
    void *target;
    StaScriptMoveVec move;
    StaActScriptSys *sys;
    StaActScriptTask *task;
} StaScriptMoveWork;

typedef struct {
    StaScriptRepeat repeat;
    StaActPoke *poke;
    fx32 height;
    StaActScriptSys *sys;
    StaActScriptTask *task;
} StaScriptJumpWork;

typedef struct {
    StaScriptMoveValue move;
    u32 unused[2];
    StaActPoke *poke;
    StaActScriptSys *sys;
    StaActScriptTask *task;
} StaScriptRotateWork;

// Creates an emitter at a random position in a box every period
typedef struct {
    StaScriptRepeat repeat;
    StaActEffect *effect;
    VecFx32 min;
    VecFx32 range;
    u16 emitterNo;
    StaActScriptSys *sys;
    StaActScriptTask *task;
} StaScriptEmitterWork;

typedef struct {
    u8 pokePos;
    StaActLight *light;
    StaScriptFollow follow;
    u32 unused[2];
    StaActScriptSys *sys;
    StaActScriptTask *task;
} StaScriptLightFollowWork;

static BOOL StaScript_UpdateMoveValue(StaScriptMoveValue *move, int *value) {
    int diff;

    move->count++;
    if (move->count >= move->frames) {
        *value = move->end;
        return TRUE;
    }
    diff = move->end - move->start;
    *value = move->start + diff * move->count / move->frames;
    return FALSE;
}

static BOOL StaScript_UpdateMoveVec(StaScriptMoveVec *move, VecFx32 *pos) {
    move->count++;
    if (move->count >= move->frames) {
        pos->x = move->end.x;
        pos->y = move->end.y;
        pos->z = move->end.z;
        return TRUE;
    }
    VEC_MultAdd(FX32_CONST(move->count), &move->step, &move->start, pos);
    return FALSE;
}

static BOOL StaScript_UpdateFollow(StaScriptFollow *follow, VecFx32 *pos) {
    VecFx32 pokePos;

    StaActPoke_GetPosition(StaActing_GetPokeSys(follow->stage), follow->poke, &pokePos);
    VEC_Add(&pokePos, &follow->offset, pos);
    follow->count++;
    if (follow->count >= follow->frames) {
        return TRUE;
    }
    return FALSE;
}

// 1 when a period ends, 2 when the last one does, else 0
static u32 StaScript_UpdateRepeat(StaScriptRepeat *repeat) {
    repeat->count++;
    if (repeat->count >= repeat->period) {
        repeat->repeat--;
        repeat->count = 0;
        if (repeat->repeat == 0) {
            return 2;
        }
        return 1;
    }
    return 0;
}

static BOOL StaScriptCmd_End(VM *vm, void *work) {
    StaActScript *script = work;

    script->flags |= STA_SCRIPT_FLAG_END;
    return TRUE;
}

static BOOL StaScriptCmd_Nop(VM *vm, void *work) {
    StaActScript *script = work;

    VM_Read32(script->vm);
    return FALSE;
}

static BOOL StaScriptCmd_Wait(VM *vm, void *work) {
    StaActScript *script = work;

    script->wait = VM_Read32(script->vm);
    return TRUE;
}

static BOOL StaScriptCmd_WaitFrame(VM *vm, void *work) {
    StaActScript *script = work;
    u32 frame = VM_Read32(script->vm);

    if (script->frame >= frame) {
        return FALSE;
    }
    script->wait = frame - script->frame;
    return TRUE;
}

static BOOL StaScriptCmd_SyncWait(VM *vm, void *work) {
    StaActScript *script = work;

    VM_Read32(script->vm);
    if (script->flags & STA_SCRIPT_FLAG_SYNC) {
        script->flags |= STA_SCRIPT_FLAG_WAIT_SYNC;
    }
    return TRUE;
}

static void StaScript_CurtainTask(TCB *tcb, void *data);
static void StaScript_ScrollTask(TCB *tcb, void *data);
static void StaScript_PokeMoveTask(TCB *tcb, void *data);
static void StaScript_PokeJumpTask(TCB *tcb, void *data);
static void StaScript_PokeRotateTask(TCB *tcb, void *data);
static void StaScript_ObjMoveTask(TCB *tcb, void *data);
static void StaScript_EmitterTask(TCB *tcb, void *data);
static void StaScript_LightMoveTask(TCB *tcb, void *data);
static void StaScript_LightFollowTask(TCB *tcb, void *data);

static BOOL StaScriptCmd_CurtainOpen(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    BOOL wait = VM_Read32(script->vm);
    StaScriptValueWork *curtain = GFL_HeapAllocate(sys->heapId, sizeof(StaScriptValueWork), FALSE, sFile, 424);

    curtain->sys = sys;
    curtain->move.stage = sys->stage;
    curtain->move.count = 0;
    curtain->move.start = StaActing_GetCurtainOffset(sys->stage);
    curtain->move.end = 0xe0;
    curtain->move.frames = MATH_ABS(curtain->move.end - curtain->move.start) / 2;
    curtain->task = StaActScript_AddTask(sys, StaScript_CurtainTask, curtain, 10);
    if (wait == TRUE) {
        script->wait = curtain->move.frames;
        return TRUE;
    }
    return FALSE;
}

static BOOL StaScriptCmd_CurtainClose(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    BOOL wait = VM_Read32(script->vm);
    StaScriptValueWork *curtain = GFL_HeapAllocate(sys->heapId, sizeof(StaScriptValueWork), FALSE, sFile, 453);

    curtain->sys = sys;
    curtain->move.stage = sys->stage;
    curtain->move.count = 0;
    curtain->move.start = StaActing_GetCurtainOffset(sys->stage);
    curtain->move.end = 0;
    curtain->move.frames = MATH_ABS(curtain->move.end - curtain->move.start) / 2;
    curtain->task = StaActScript_AddTask(sys, StaScript_CurtainTask, curtain, 10);
    StaActing_PlayApplause(sys->stage);
    if (wait == TRUE) {
        script->wait = curtain->move.frames;
        return TRUE;
    }
    return FALSE;
}

static BOOL StaScriptCmd_CurtainMove(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 frames = VM_Read32(script->vm);
    u32 offset = VM_Read32(script->vm);
    BOOL wait = VM_Read32(script->vm);

    if (frames == 0) {
        StaActing_SetCurtainOffset(sys->stage, offset);
    } else {
        StaScriptValueWork *curtain = GFL_HeapAllocate(sys->heapId, sizeof(StaScriptValueWork), FALSE, sFile, 493);

        curtain->sys = sys;
        curtain->move.stage = sys->stage;
        curtain->move.count = 0;
        curtain->move.start = StaActing_GetCurtainOffset(sys->stage);
        curtain->move.end = offset;
        curtain->move.frames = frames;
        curtain->task = StaActScript_AddTask(sys, StaScript_CurtainTask, curtain, 10);
        if (wait == TRUE) {
            script->wait = curtain->move.frames;
            return TRUE;
        }
    }
    return FALSE;
}

static void StaScript_CurtainTask(TCB *tcb, void *data) {
    StaScriptValueWork *curtain = data;
    int offset;
    BOOL end = StaScript_UpdateMoveValue(&curtain->move, &offset);

    StaActing_SetCurtainOffset(curtain->move.stage, offset);
    if (end == TRUE) {
        StaActScript_DelTask(curtain->sys, curtain->task);
    }
}

static BOOL StaScriptCmd_Scroll(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 frames = VM_Read32(script->vm);
    u32 offset = VM_Read32(script->vm);
    BOOL wait = VM_Read32(script->vm);

    if (frames == 0) {
        StaActing_SetScrollTarget(sys->stage, offset);
    } else {
        StaScriptValueWork *scroll = GFL_HeapAllocate(sys->heapId, sizeof(StaScriptValueWork), FALSE, sFile, 553);

        scroll->sys = sys;
        scroll->move.stage = sys->stage;
        scroll->move.count = 0;
        scroll->move.start = StaActing_GetScrollOffset(sys->stage);
        scroll->move.end = offset;
        scroll->move.frames = frames;
        scroll->task = StaActScript_AddTask(sys, StaScript_ScrollTask, scroll, 10);
        if (wait == TRUE) {
            script->wait = scroll->move.frames;
            return TRUE;
        }
    }
    return FALSE;
}

static void StaScript_ScrollTask(TCB *tcb, void *data) {
    StaScriptValueWork *scroll = data;
    int offset;
    BOOL end = StaScript_UpdateMoveValue(&scroll->move, &offset);

    StaActing_SetScrollTarget(scroll->move.stage, offset);
    if (end == TRUE) {
        StaActing_SetScriptScroll(scroll->move.stage, FALSE);
        StaActScript_DelTask(scroll->sys, scroll->task);
    }
}

static BOOL StaScriptCmd_Nop2(VM *vm, void *work) {
    StaActScript *script = work;

    VM_Read32(script->vm);
    return FALSE;
}

static BOOL StaScriptCmd_ButtonEnable(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;

    StaActing_EnableButtons(sys->stage);
    StaActing_SetNpcItemTiming(sys->stage, 0, 0);
    return FALSE;
}

static BOOL StaScriptCmd_ButtonDisable(VM *vm, void *work) {
    StaActScript *script = work;

    StaActing_DisableButtons(script->sys->stage);
    return FALSE;
}

// The Pokémon a command's target means, as a mask of positions
static u32 StaScript_GetPokeMask(StaActScript *script, u32 target) {
    if (target == STA_SCRIPT_OWN_POKE) {
        return script->pokeNo;
    }
    return 1 << target;
}

static BOOL StaScriptCmd_PokeShow(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 mask;
    StaActPokeSys *pokeSys;
    BOOL show;
    u8 i;

    mask = StaScript_GetPokeMask(script, VM_Read32(script->vm));
    show = VM_Read32(script->vm);
    pokeSys = StaActing_GetPokeSys(sys->stage);

    for (i = 0; i < 4; i++) {
        if ((1 << i) & mask) {
            StaActPoke_SetShowFlg(pokeSys, StaActing_GetPoke(sys->stage, i), show);
        }
    }
    return FALSE;
}

static BOOL StaScriptCmd_PokeFlip(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 mask = StaScript_GetPokeMask(script, VM_Read32(script->vm));
    u32 flip = VM_Read32(script->vm);
    StaActPokeSys *pokeSys = StaActing_GetPokeSys(sys->stage);
    u8 i;

    for (i = 0; i < 4; i++) {
        if ((1 << i) & mask) {
            StaActPoke *poke = StaActing_GetPoke(sys->stage, i);
            BOOL isFlip = FALSE;

            if (flip) {
                isFlip = TRUE;
            }
            StaActPoke_SetFlip(pokeSys, poke, isFlip);
        }
    }
    return FALSE;
}

static BOOL StaScriptCmd_PokeMove(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 mask;
    int frames;
    BOOL wait;
    StaActPokeSys *pokeSys;
    fx32 x;
    fx32 y;
    fx32 z;
    u8 i;

    mask = StaScript_GetPokeMask(script, VM_Read32(script->vm));
    frames = VM_Read32(script->vm);
    x = VM_Read32(script->vm);
    y = VM_Read32(script->vm);
    z = VM_Read32(script->vm);
    wait = VM_Read32(script->vm);
    pokeSys = StaActing_GetPokeSys(sys->stage);

    for (i = 0; i < 4; i++) {
        if ((1 << i) & mask) {
            StaActPoke *poke = StaActing_GetPoke(sys->stage, i);

            if (frames == 0) {
                VecFx32 pos;

                pos.x = x;
                pos.y = y;
                pos.z = z;
                StaActPoke_SetPosition(pokeSys, poke, &pos);
            } else {
                VecFx32 diff;
                StaScriptMoveWork *move = GFL_HeapAllocate(sys->heapId, sizeof(StaScriptMoveWork), FALSE, sFile, 734);

                move->sys = sys;
                move->target = poke;
                move->move.stage = sys->stage;
                move->move.count = 0;
                StaActPoke_GetPosition(pokeSys, poke, &move->move.start);
                move->move.end.x = x;
                move->move.end.y = y;
                move->move.end.z = z;
                move->move.frames = frames;
                VEC_Subtract(&move->move.end, &move->move.start, &diff);
                move->move.step.x = diff.x / frames;
                move->move.step.y = diff.y / frames;
                move->move.step.z = diff.z / frames;
                move->task = StaActScript_AddTask(sys, StaScript_PokeMoveTask, move, 10);
            }
        }
    }
    if (wait == TRUE) {
        script->wait = frames;
        return TRUE;
    }
    return FALSE;
}

static BOOL StaScriptCmd_PokeMoveBy(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 mask = StaScript_GetPokeMask(script, VM_Read32(script->vm));
    int frames = VM_Read32(script->vm);
    fx32 x = VM_Read32(script->vm);
    fx32 y = VM_Read32(script->vm);
    fx32 z = VM_Read32(script->vm);
    BOOL wait = VM_Read32(script->vm);
    u8 i;
    StaActPokeSys *pokeSys = StaActing_GetPokeSys(sys->stage);

    for (i = 0; i < 4; i++) {
        if ((1 << i) & mask) {
            StaActPoke *poke = StaActing_GetPoke(sys->stage, i);

            if (frames == 0) {
                VecFx32 pos;

                StaActPoke_GetPosition(pokeSys, poke, &pos);
                pos.x += x;
                pos.y += y;
                pos.z += z;
                StaActPoke_SetPosition(pokeSys, poke, &pos);
            } else {
                StaScriptMoveWork *move = GFL_HeapAllocate(sys->heapId, sizeof(StaScriptMoveWork), FALSE, sFile, 799);

                move->sys = sys;
                move->target = poke;
                move->move.stage = sys->stage;
                move->move.count = 0;
                StaActPoke_GetPosition(pokeSys, poke, &move->move.start);
                VEC_Set(&move->move.end, move->move.start.x + x, move->move.start.y + y, move->move.start.z + z);
                move->move.frames = frames;
                move->move.step.x = x / frames;
                move->move.step.y = y / frames;
                move->move.step.z = z / frames;
                move->task = StaActScript_AddTask(sys, StaScript_PokeMoveTask, move, 10);
            }
        }
    }
    if (wait == TRUE) {
        script->wait = frames;
        return TRUE;
    }
    return FALSE;
}

static void StaScript_PokeMoveTask(TCB *tcb, void *data) {
    StaScriptMoveWork *move = data;
    StaActPokeSys *pokeSys = StaActing_GetPokeSys(move->move.stage);
    VecFx32 pos;
    BOOL end = StaScript_UpdateMoveVec(&move->move, &pos);

    StaActPoke_SetPosition(pokeSys, move->target, &pos);
    if (end == TRUE) {
        StaActScript_DelTask(move->sys, move->task);
    }
}

static BOOL StaScriptCmd_PokeStopAnime(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 mask = StaScript_GetPokeMask(script, VM_Read32(script->vm));
    StaActPokeSys *pokeSys = StaActing_GetPokeSys(sys->stage);
    u8 i;

    for (i = 0; i < 4; i++) {
        if ((1 << i) & mask) {
            StaActPoke_StopAnime(pokeSys, StaActing_GetPoke(sys->stage, i));
        }
    }
    return FALSE;
}

static BOOL StaScriptCmd_PokeStartAnime(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 mask = StaScript_GetPokeMask(script, VM_Read32(script->vm));
    StaActPokeSys *pokeSys = StaActing_GetPokeSys(sys->stage);
    u8 i;

    for (i = 0; i < 4; i++) {
        if ((1 << i) & mask) {
            StaActPoke_StartAnime(pokeSys, StaActing_GetPoke(sys->stage, i));
        }
    }
    return FALSE;
}

static BOOL StaScriptCmd_PokeChangeAnime(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 mask = StaScript_GetPokeMask(script, VM_Read32(script->vm));
    u32 anime = VM_Read32(script->vm);
    StaActPokeSys *pokeSys = StaActing_GetPokeSys(sys->stage);
    u8 i;

    for (i = 0; i < 4; i++) {
        if ((1 << i) & mask) {
            StaActPoke_ChangeAnime(pokeSys, StaActing_GetPoke(sys->stage, i), anime);
        }
    }
    return FALSE;
}

static BOOL StaScriptCmd_PokeFront(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 mask = StaScript_GetPokeMask(script, VM_Read32(script->vm));
    u32 front = VM_Read32(script->vm);
    StaActPokeSys *pokeSys = StaActing_GetPokeSys(sys->stage);
    u8 i;

    for (i = 0; i < 4; i++) {
        if ((1 << i) & mask) {
            StaActPoke_SetFront(pokeSys, StaActing_GetPoke(sys->stage, i), front != 0);
        }
    }
    return FALSE;
}

static BOOL StaScriptCmd_PokeShowItem(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 mask = StaScript_GetPokeMask(script, VM_Read32(script->vm));
    u32 show = VM_Read32(script->vm);
    StaActPokeSys *pokeSys = StaActing_GetPokeSys(sys->stage);
    u8 i;

    for (i = 0; i < 4; i++) {
        if ((1 << i) & mask) {
            StaActPoke_SetShowItem(pokeSys, StaActing_GetPoke(sys->stage, i), show != 0);
        }
    }
    return FALSE;
}

static BOOL StaScriptCmd_PokeAppeal(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 mask = StaScript_GetPokeMask(script, VM_Read32(script->vm));
    StaActPokeSys *pokeSys = StaActing_GetPokeSys(sys->stage);
    u8 i;

    for (i = 0; i < 4; i++) {
        if ((1 << i) & mask) {
            StaActPoke *poke = StaActing_GetPoke(sys->stage, i);

            StaActing_StartAppealEffect(sys->stage, i);
        }
    }
    return FALSE;
}

static BOOL StaScriptCmd_AudienceFocus(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 mask = StaScript_GetPokeMask(script, VM_Read32(script->vm));
    u8 i;

    for (i = 0; i < 4; i++) {
        if ((1 << i) & mask) {
            StaActing_SetAudienceFocus(sys->stage, i, TRUE);
        }
    }
    return FALSE;
}

static BOOL StaScriptCmd_AudienceUnfocus(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;

    StaActing_SetAudienceFocus(sys->stage, 0, FALSE);
    StaActing_SetAudienceFocus(sys->stage, 1, FALSE);
    StaActing_SetAudienceFocus(sys->stage, 2, FALSE);
    StaActing_SetAudienceFocus(sys->stage, 3, FALSE);
    return FALSE;
}

static BOOL StaScriptCmd_AppealScript(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 pos = VM_Read32(script->vm);
    u32 file = VM_Read32(script->vm);

    StaActing_StartAppealScript(sys->stage, file, pos);
    StaActing_SetNpcItemTiming(sys->stage, 1, pos);
    return FALSE;
}

static BOOL StaScriptCmd_PokeScript(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 mask = StaScript_GetPokeMask(script, VM_Read32(script->vm));
    u32 file = VM_Read32(script->vm);

    StaActing_StartPokeScript(sys->stage, file, mask);
    return FALSE;
}

static BOOL StaScriptCmd_PokeJump(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 mask;
    BOOL wait;
    fx32 height;
    u32 period;
    u32 count;
    StaActPokeSys *pokeSys;
    u8 i;

    mask = StaScript_GetPokeMask(script, VM_Read32(script->vm));
    period = VM_Read32(script->vm);
    count = VM_Read32(script->vm);
    height = VM_Read32(script->vm);
    wait = VM_Read32(script->vm);
    pokeSys = StaActing_GetPokeSys(sys->stage);

    for (i = 0; i < 4; i++) {
        if ((1 << i) & mask) {
            StaScriptJumpWork *jump = GFL_HeapAllocate(sys->heapId, sizeof(StaScriptJumpWork), FALSE, sFile, 1095);

            jump->sys = sys;
            jump->poke = StaActing_GetPoke(sys->stage, i);
            jump->height = height;
            jump->repeat.stage = sys->stage;
            jump->repeat.count = 0;
            jump->repeat.period = period;
            jump->repeat.repeat = count;
            jump->task = StaActScript_AddTask(sys, StaScript_PokeJumpTask, jump, 10);
        }
    }
    if (wait == TRUE) {
        script->wait = count * period;
        return TRUE;
    }
    return FALSE;
}

static void StaScript_PokeJumpTask(TCB *tcb, void *data) {
    StaScriptJumpWork *jump = data;
    StaActPokeSys *pokeSys = StaActing_GetPokeSys(jump->repeat.stage);

    if (StaScript_UpdateRepeat(&jump->repeat) == 2) {
        VecFx32 offset = { 0, 0, 0 };

        StaActPoke_SetPositionOffset(pokeSys, jump->poke, &offset);
        StaActScript_DelTask(jump->sys, jump->task);
    } else {
        VecFx32 offset;

        offset.x = 0;
        offset.y = -FX_Mul(FX_SinIdx((jump->repeat.count << 15) / jump->repeat.period), jump->height);
        offset.z = 0;
        StaActPoke_SetPositionOffset(pokeSys, jump->poke, &offset);
    }
}

static BOOL StaScriptCmd_PokeRotate(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 mask = StaScript_GetPokeMask(script, VM_Read32(script->vm));
    u32 frames = VM_Read32(script->vm);
    int start = VM_Read32(script->vm);
    int angle = VM_Read32(script->vm);
    BOOL wait = VM_Read32(script->vm);
    StaActPokeSys *pokeSys = StaActing_GetPokeSys(sys->stage);
    u8 i;

    for (i = 0; i < 4; i++) {
        if ((1 << i) & mask) {
            StaScriptRotateWork *rotate =
                GFL_HeapAllocate(sys->heapId, sizeof(StaScriptRotateWork), FALSE, sFile, 1175);

            rotate->sys = sys;
            rotate->poke = StaActing_GetPoke(sys->stage, i);
            rotate->move.stage = sys->stage;
            rotate->move.start = start;
            rotate->move.end = start + angle;
            rotate->move.frames = frames;
            rotate->move.count = 0;
            rotate->task = StaActScript_AddTask(sys, StaScript_PokeRotateTask, rotate, 10);
        }
    }
    if (wait == TRUE) {
        script->wait = frames;
        return TRUE;
    }
    return FALSE;
}

static void StaScript_PokeRotateTask(TCB *tcb, void *data) {
    StaScriptRotateWork *rotate = data;
    StaActPokeSys *pokeSys = StaActing_GetPokeSys(rotate->move.stage);
    int angle;

    if (StaScript_UpdateMoveValue(&rotate->move, &angle) == TRUE) {
        StaActPoke_SetRotate(pokeSys, rotate->poke, DEG_TO_IDX(rotate->move.end));
        StaActScript_DelTask(rotate->sys, rotate->task);
    } else {
        StaActPoke_SetRotate(pokeSys, rotate->poke, DEG_TO_IDX(angle));
    }
}

// Lines the Pokémon up around the one with the most points, the others ahead of it in order of where they stand,
// hopping as they go, and scrolls to them
static BOOL StaScriptCmd_LineUp(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    int frames = VM_Read32(script->vm);
    BOOL wait = VM_Read32(script->vm);
    StaActPokeSys *pokeSys = StaActing_GetPokeSys(sys->stage);
    u8 maxRank = 0;
    s8 ranks[4] = { 0, 0, 0, 0 };
    u16 maxPoints = 0;
    u8 top;
    fx32 totalX;
    fx32 topX;
    fx32 x[4];
    VecFx32 pos;
    u8 i;
    u8 j;

    for (i = 0; i < 4; i++) {
        u16 points = StaActing_GetBonusPoints(sys->stage, i) + StaActing_GetPoints(sys->stage, i);

        if (maxPoints < points) {
            maxPoints = points;
        }
    }
    for (i = 0; i < 4; i++) {
        u16 points = StaActing_GetBonusPoints(sys->stage, i) + StaActing_GetPoints(sys->stage, i);

        if (maxPoints == points) {
            top = i;
        }
    }
    for (i = 0; i < 4; i++) {
        StaActPoke_GetPosition(pokeSys, StaActing_GetPoke(sys->stage, i), &pos);
        x[i] = pos.x;
        if (i == top) {
            topX = pos.x;
        }
    }
    for (i = 0; i < 4; i++) {
        x[i] -= topX;
    }
    for (i = 0; i < 4; i++) {
        if (i != top) {
            for (j = 0; j < 4; j++) {
                if (i != j) {
                    if (x[i] > 0 && x[j] >= 0 && x[i] > x[j]) {
                        ranks[i]++;
                    } else if (x[i] < 0 && x[j] <= 0 && x[i] < x[j]) {
                        ranks[i]--;
                    }
                }
            }
            if (maxRank < MATH_ABS(ranks[i])) {
                maxRank = MATH_ABS(ranks[i]);
            }
        }
    }

    totalX = 0;
    for (i = 0; i < 4; i++) {
        StaScriptMoveWork *move = GFL_HeapAllocate(sys->heapId, sizeof(StaScriptMoveWork), FALSE, sFile, 1332);

        move->sys = sys;
        move->target = StaActing_GetPoke(sys->stage, i);
        move->move.stage = sys->stage;
        move->move.count = 0;
        StaActPoke_GetPosition(pokeSys, move->target, &move->move.start);
        move->move.end.x = topX + ranks[i] * FX32_CONST(64);
        move->move.end.y = move->move.start.y;
        move->move.end.z = move->move.start.z;
        move->move.frames = MATH_ABS(ranks[i]) * frames / maxRank;
        move->move.step.x = (move->move.end.x - move->move.start.x) / move->move.frames;
        move->move.step.y = (move->move.end.y - move->move.start.y) / move->move.frames;
        move->move.step.z = (move->move.end.z - move->move.start.z) / move->move.frames;
        move->task = StaActScript_AddTask(sys, StaScript_PokeMoveTask, move, 10);
        if (move->move.frames != 0) {
            StaScriptJumpWork *jump = GFL_HeapAllocate(sys->heapId, sizeof(StaScriptJumpWork), FALSE, sFile, 1359);
            u16 jumpFrames = move->move.frames;
            u8 rest = jumpFrames % STA_SCRIPT_LINE_UP_JUMP_PERIOD;
            int extra = rest != 0 ? 1 : 0;

            jump->sys = sys;
            jump->poke = StaActing_GetPoke(sys->stage, i);
            jump->height = FX32_CONST(8);
            jump->repeat.stage = sys->stage;
            jump->repeat.count = 0;
            jump->repeat.period = STA_SCRIPT_LINE_UP_JUMP_PERIOD;
            jump->repeat.repeat = (u8)(jumpFrames / STA_SCRIPT_LINE_UP_JUMP_PERIOD + extra);
            jump->task = StaActScript_AddTask(sys, StaScript_PokeJumpTask, jump, 10);
        }
        totalX += move->move.end.x;
    }

    {
        StaScriptValueWork *scroll = GFL_HeapAllocate(sys->heapId, sizeof(StaScriptValueWork), FALSE, sFile, 1383);

        scroll->sys = sys;
        scroll->move.stage = sys->stage;
        scroll->move.count = 0;
        scroll->move.start = StaActing_GetScrollOffset(sys->stage);
        scroll->move.end = FX_FX32_TO_F32(totalX / 4) - 128.0f;
        scroll->move.frames = frames;
        scroll->task = StaActScript_AddTask(sys, StaScript_ScrollTask, scroll, 10);
    }
    StaActing_SetScriptScroll(sys->stage, TRUE);
    StaActing_SetScrollFixed(sys->stage, TRUE);
    if (wait == TRUE) {
        script->wait = frames;
        return TRUE;
    }
    return FALSE;
}

static BOOL StaScriptCmd_SetPokeMask(VM *vm, void *work) {
    StaActScript *script = work;
    u32 poke0 = VM_Read32(script->vm);
    u32 poke1 = VM_Read32(script->vm);
    u32 poke2 = VM_Read32(script->vm);
    u32 poke3 = VM_Read32(script->vm);

    script->pokeNo = 0;
    if (poke0 == TRUE) {
        script->pokeNo += 1;
    }
    if (poke1 == TRUE) {
        script->pokeNo += 2;
    }
    if (poke2 == TRUE) {
        script->pokeNo += 4;
    }
    if (poke3 == TRUE) {
        script->pokeNo += 8;
    }
    return FALSE;
}

static BOOL StaScriptCmd_ObjAdd(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 index = VM_Read32(script->vm);
    u32 objId = VM_Read32(script->vm);

    StaActing_SetObj(sys->stage, StaActObj_AddObj(StaActing_GetObjSys(sys->stage), objId), index);
    return FALSE;
}

static BOOL StaScriptCmd_ObjDel(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 index = VM_Read32(script->vm);
    StaActObjSys *objSys = StaActing_GetObjSys(sys->stage);

    StaActObj_DelObj(objSys, StaActing_GetObj(sys->stage, index));
    StaActing_SetObj(sys->stage, NULL, index);
    return FALSE;
}

static BOOL StaScriptCmd_ObjShow(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 index = VM_Read32(script->vm);
    StaActObjSys *objSys = StaActing_GetObjSys(sys->stage);

    StaActObj_SetShowFlg(objSys, StaActing_GetObj(sys->stage, index), TRUE);
    return FALSE;
}

static BOOL StaScriptCmd_ObjHide(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 index = VM_Read32(script->vm);
    StaActObjSys *objSys = StaActing_GetObjSys(sys->stage);

    StaActObj_SetShowFlg(objSys, StaActing_GetObj(sys->stage, index), FALSE);
    return FALSE;
}

static BOOL StaScriptCmd_ObjMove(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 index = VM_Read32(script->vm);
    int frames = VM_Read32(script->vm);
    fx32 x = VM_Read32(script->vm);
    fx32 y = VM_Read32(script->vm);
    fx32 z = VM_Read32(script->vm);
    StaActObjSys *objSys = StaActing_GetObjSys(sys->stage);
    StaActObj *obj = StaActing_GetObj(sys->stage, index);

    if (frames == 0) {
        VecFx32 pos;

        pos.x = x;
        pos.y = y;
        pos.z = z;
        StaActObj_SetPosition(objSys, obj, &pos);
    } else {
        VecFx32 diff;
        StaScriptMoveWork *move = GFL_HeapAllocate(sys->heapId, sizeof(StaScriptMoveWork), FALSE, sFile, 1541);

        move->sys = sys;
        move->target = obj;
        move->move.stage = sys->stage;
        move->move.count = 0;
        StaActObj_GetPosition(objSys, obj, &move->move.start);
        move->move.end.x = x;
        move->move.end.y = y;
        move->move.end.z = z;
        move->move.frames = frames;
        VEC_Subtract(&move->move.end, &move->move.start, &diff);
        move->move.step.x = diff.x / frames;
        move->move.step.y = diff.y / frames;
        move->move.step.z = diff.z / frames;
        move->task = StaActScript_AddTask(sys, StaScript_ObjMoveTask, move, 10);
    }
    return FALSE;
}

static void StaScript_ObjMoveTask(TCB *tcb, void *data) {
    StaScriptMoveWork *move = data;
    StaActObjSys *objSys = StaActing_GetObjSys(move->move.stage);
    VecFx32 pos;
    BOOL end = StaScript_UpdateMoveVec(&move->move, &pos);

    StaActObj_SetPosition(objSys, move->target, &pos);
    if (end == TRUE) {
        StaActScript_DelTask(move->sys, move->task);
    }
}

static BOOL StaScriptCmd_EffectAdd(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 index = VM_Read32(script->vm);
    u32 effectNo = VM_Read32(script->vm);

    StaActing_SetEffect(sys->stage, StaActEffect_AddEffect(StaActing_GetEffectSys(sys->stage), effectNo + 51), index);
    return FALSE;
}

static BOOL StaScriptCmd_EffectDel(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 index = VM_Read32(script->vm);
    StaActEffectSys *effectSys = StaActing_GetEffectSys(sys->stage);

    StaActEffect_DelEffect(effectSys, StaActing_GetEffect(sys->stage, index));
    StaActing_SetEffect(sys->stage, NULL, index);
    return FALSE;
}

// Stage positions are in fixed point pixels from the top left, the particles' from the bottom left in 16ths
static BOOL StaScriptCmd_EmitterCreate(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 index = VM_Read32(script->vm);
    u32 emitterNo = VM_Read32(script->vm);
    fx32 x = VM_Read32(script->vm);
    fx32 y = VM_Read32(script->vm);
    fx32 z = VM_Read32(script->vm);
    StaActEffect *effect = StaActing_GetEffect(sys->stage, index);
    VecFx32 pos;

    pos.x = x / 16;
    pos.y = (FX32_CONST(192) - y) / 16;
    pos.z = z;
    StaActEffect_CreateEmitter(effect, emitterNo, &pos);
    return FALSE;
}

static BOOL StaScriptCmd_EmitterDelete(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 index = VM_Read32(script->vm);
    u32 emitterNo = VM_Read32(script->vm);

    StaActEffect_DeleteEmitter(StaActing_GetEffect(sys->stage, index), emitterNo);
    return FALSE;
}

static BOOL StaScriptCmd_EmitterRandom(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 index = VM_Read32(script->vm);
    u32 emitterNo = VM_Read32(script->vm);
    u32 period = VM_Read32(script->vm);
    u32 count = VM_Read32(script->vm);
    fx32 minX = VM_Read32(script->vm);
    fx32 minY = VM_Read32(script->vm);
    fx32 minZ = VM_Read32(script->vm);
    fx32 maxX = VM_Read32(script->vm);
    fx32 maxY = VM_Read32(script->vm);
    fx32 maxZ = VM_Read32(script->vm);
    StaActEffect *effect = StaActing_GetEffect(sys->stage, index);
    StaScriptEmitterWork *emitter = GFL_HeapAllocate(sys->heapId, sizeof(StaScriptEmitterWork), FALSE, sFile, 1685);

    emitter->sys = sys;
    emitter->effect = effect;
    emitter->emitterNo = emitterNo;
    emitter->min.x = minX;
    emitter->min.y = minY;
    emitter->min.z = minZ;
    emitter->range.x = maxX - minX;
    emitter->range.y = maxY - minY;
    emitter->range.z = maxZ - minZ;
    emitter->repeat.stage = sys->stage;
    emitter->repeat.count = 0;
    emitter->repeat.period = period;
    emitter->repeat.repeat = count;
    emitter->task = StaActScript_AddTask(sys, StaScript_EmitterTask, emitter, 10);
    return FALSE;
}

static void StaScript_EmitterTask(TCB *tcb, void *data) {
    StaScriptEmitterWork *emitter = data;
    u32 ret = StaScript_UpdateRepeat(&emitter->repeat);

    if (ret == 2) {
        StaActScript_DelTask(emitter->sys, emitter->task);
    } else if (ret == 1) {
        VecFx32 pos;

        pos.x = (emitter->min.x + GFL_RandomLCAlt(emitter->range.x)) / 16;
        pos.y = (FX32_CONST(192) - (emitter->min.y + GFL_RandomLCAlt(emitter->range.y))) / 16;
        pos.z = emitter->min.z + GFL_RandomLCAlt(emitter->range.z);
        StaActEffect_CreateEmitter(emitter->effect, emitter->emitterNo, &pos);
    }
}

static BOOL StaScriptCmd_LightAdd(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 index = VM_Read32(script->vm);
    u32 unk = VM_Read32(script->vm);
    StaActLightSys *lightSys = StaActing_GetLightSys(sys->stage);
    StaActLight *light = StaActLight_AddLight(lightSys, 1);

    StaActLight_SetColor(lightSys, light, GX_RGB(31, 31, 0), 16);
    func_ov209_021bd770(lightSys, light, unk, 0);
    StaActing_SetLight(sys->stage, light, index);
    StaActing_RefreshAudience(sys->stage);
    return FALSE;
}

static BOOL StaScriptCmd_LightDel(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 index = VM_Read32(script->vm);
    StaActLightSys *lightSys = StaActing_GetLightSys(sys->stage);

    StaActLight_DelLight(lightSys, StaActing_GetLight(sys->stage, index));
    StaActing_SetLight(sys->stage, NULL, index);
    StaActing_RefreshAudience(sys->stage);
    return FALSE;
}

static BOOL StaScriptCmd_LightMove(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 index = VM_Read32(script->vm);
    int frames = VM_Read32(script->vm);
    fx32 x = VM_Read32(script->vm);
    fx32 y = VM_Read32(script->vm);
    fx32 z = VM_Read32(script->vm);
    StaActLightSys *lightSys = StaActing_GetLightSys(sys->stage);
    StaActLight *light = StaActing_GetLight(sys->stage, index);

    if (frames == 0) {
        VecFx32 pos;

        pos.x = x;
        pos.y = y;
        pos.z = z;
        StaActLight_SetPosition(lightSys, light, &pos);
    } else {
        VecFx32 diff;
        StaScriptMoveWork *move = GFL_HeapAllocate(sys->heapId, sizeof(StaScriptMoveWork), FALSE, sFile, 1800);

        move->sys = sys;
        move->target = light;
        move->move.stage = sys->stage;
        move->move.count = 0;
        StaActLight_GetPosition(lightSys, light, &move->move.start);
        move->move.end.x = x;
        move->move.end.y = y;
        move->move.end.z = z;
        move->move.frames = frames;
        VEC_Subtract(&move->move.end, &move->move.start, &diff);
        move->move.step.x = diff.x / frames;
        move->move.step.y = diff.y / frames;
        move->move.step.z = diff.z / frames;
        move->task = StaActScript_AddTask(sys, StaScript_LightMoveTask, move, 10);
    }
    return FALSE;
}

static void StaScript_LightMoveTask(TCB *tcb, void *data) {
    StaScriptMoveWork *move = data;
    StaActLightSys *lightSys = StaActing_GetLightSys(move->move.stage);
    VecFx32 pos;
    BOOL end = StaScript_UpdateMoveVec(&move->move, &pos);

    StaActLight_SetPosition(lightSys, move->target, &pos);
    if (end == TRUE) {
        StaActScript_DelTask(move->sys, move->task);
    }
}

static BOOL StaScriptCmd_LightFollow(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 index;
    u32 mask;
    u32 frames;
    StaActLightSys *lightSys;
    StaActLight *light;
    StaActPokeSys *pokeSys;
    fx32 x;
    fx32 y;
    fx32 z;
    u8 i;

    index = VM_Read32(script->vm);
    mask = StaScript_GetPokeMask(script, VM_Read32(script->vm));
    frames = VM_Read32(script->vm);
    x = VM_Read32(script->vm);
    y = VM_Read32(script->vm);
    z = VM_Read32(script->vm);
    lightSys = StaActing_GetLightSys(sys->stage);
    light = StaActing_GetLight(sys->stage, index);
    pokeSys = StaActing_GetPokeSys(sys->stage);

    for (i = 0; i < 4; i++) {
        StaActPoke *poke = StaActing_GetPoke(sys->stage, i);

        if ((1 << i) & mask) {
            if (frames == 0) {
                VecFx32 pos;
                VecFx32 offset;

                StaActPoke_GetPosition(pokeSys, poke, &pos);
                offset.x = x;
                offset.y = y;
                offset.z = z;
                VEC_Add(&pos, &offset, &pos);
                StaActLight_SetPosition(lightSys, light, &pos);
            } else {
                StaActAudience *audience = StaActing_GetAudience(sys->stage);
                StaScriptLightFollowWork *follow =
                    GFL_HeapAllocate(sys->heapId, sizeof(StaScriptLightFollowWork), FALSE, sFile, 1873);

                follow->sys = sys;
                follow->pokePos = i;
                follow->light = light;
                follow->follow.stage = sys->stage;
                follow->follow.poke = poke;
                follow->follow.count = 0;
                follow->follow.offset.x = x;
                follow->follow.offset.y = y;
                follow->follow.offset.z = z;
                follow->follow.frames = frames;
                follow->task = StaActScript_AddTask(sys, StaScript_LightFollowTask, follow, 7);
            }
        }
    }
    return FALSE;
}

static void StaScript_LightFollowTask(TCB *tcb, void *data) {
    StaScriptLightFollowWork *follow = data;
    StaActLightSys *lightSys = StaActing_GetLightSys(follow->follow.stage);
    StaActAudience *audience = StaActing_GetAudience(follow->follow.stage);
    VecFx32 pos;
    BOOL end = StaScript_UpdateFollow(&follow->follow, &pos);

    StaActLight_SetPosition(lightSys, follow->light, &pos);
    if (end == TRUE) {
        StaActScript_DelTask(follow->sys, follow->task);
    }
}

static BOOL StaScriptCmd_LightColor(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 index = VM_Read32(script->vm);
    u32 r = VM_Read32(script->vm);
    u32 g = VM_Read32(script->vm);
    u32 b = VM_Read32(script->vm);
    u32 alpha = VM_Read32(script->vm);
    StaActLightSys *lightSys = StaActing_GetLightSys(sys->stage);

    StaActLight_SetColor(lightSys, StaActing_GetLight(sys->stage, index), GX_RGB((u8)r, (u8)g, (u8)b), alpha);
    return FALSE;
}

static BOOL StaScriptCmd_PrintMessage(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 msgId = VM_Read32(script->vm);
    u32 wait = VM_Read32(script->vm);

    StaActing_PrintMessage(sys->stage, msgId, wait);
    return FALSE;
}

static BOOL StaScriptCmd_ClearMessage(VM *vm, void *work) {
    StaActScript *script = work;

    StaActing_ClearMessage(script->sys->stage);
    return FALSE;
}

static BOOL StaScriptCmd_MessageColor(VM *vm, void *work) {
    StaActScript *script = work;
    u32 letter = VM_Read32(script->vm);
    u32 shadow = VM_Read32(script->vm);
    u32 background = VM_Read32(script->vm);

    GFL_TextRndUpdateColorIndexLUT(letter, shadow, background);
    return FALSE;
}

static u32 StaScript_ConvertSeq(u32 seq);
static u16 StaScript_GetWaveArc(u32 seq);

static BOOL StaScriptCmd_PlayStrm(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;

    StaActing_PlayStrm(sys->stage, StaScript_ConvertSeq(VM_Read32(script->vm)));
    return FALSE;
}

static BOOL StaScriptCmd_PlaySeq(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;

    StaActing_PlaySeq(sys->stage, StaScript_ConvertSeq(VM_Read32(script->vm)));
    return FALSE;
}

static BOOL StaScriptCmd_StopSeq(VM *vm, void *work) {
    StaActScript *script = work;

    StaActing_StopSeq(script->sys->stage);
    return FALSE;
}

static BOOL StaScriptCmd_PlayWave(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;
    u32 seq = VM_Read32(script->vm);
    u32 a1 = VM_Read32(script->vm);
    u32 a2 = VM_Read32(script->vm);

    StaActing_PlayWave(sys->stage, StaScript_GetWaveArc(StaScript_ConvertSeq(seq)), a1, a2);
    return FALSE;
}

// The first wave archive of a sequence's bank
static u16 StaScript_GetWaveArc(u32 seq) {
    const NNSSndArcBankInfo *bank = NNS_SndArcGetBankInfo(NNS_SndArcGetSeqInfo(seq)->param.bankNo);
    int i;

    for (i = 0; i < NNS_SND_ARC_BANK_TO_WAVEARC_NUM; i++) {
        if (bank->waveArcNo[i] != NNS_SND_ARC_INVALID_WAVEARC_NO) {
            return bank->waveArcNo[i];
        }
    }
    return NNS_SND_ARC_INVALID_WAVEARC_NO;
}

// The sequence a program plays in place of another, the same in both versions
static u32 StaScript_ConvertSeq(u32 seq) {
    u32 seqs[9][2] = {
        { 1109, 1109 }, { 1110, 1110 }, { 1111, 1111 }, { 1112, 1112 }, { 1113, 1113 },
        { 1174, 1172 }, { 1175, 1173 }, { 1176, 1174 }, { 1177, 1175 },
    };
    u32 i;

    for (i = 0; i < 9; i++) {
        if (seq == seqs[i][0]) {
            return seqs[i][1];
        }
    }
    return seq;
}

static BOOL StaScriptCmd_PlayBgm(VM *vm, void *work) {
    StaActScript *script = work;
    StaActScriptSys *sys = script->sys;

    StaActing_PlayBgm(sys->stage, VM_Read32(script->vm));
    return FALSE;
}

static BOOL StaScriptCmd_StopBgm(VM *vm, void *work) {
    StaActScript *script = work;

    StaActing_StopBgm(script->sys->stage);
    return FALSE;
}

VMCommand STA_SCRIPT_COMMANDS[] = {
    StaScriptCmd_End,          StaScriptCmd_Wait,          StaScriptCmd_WaitFrame,      StaScriptCmd_SyncWait,
    StaScriptCmd_CurtainOpen,  StaScriptCmd_CurtainClose,  StaScriptCmd_CurtainMove,    StaScriptCmd_Scroll,
    StaScriptCmd_Nop2,         StaScriptCmd_PokeShow,      StaScriptCmd_PokeFlip,       StaScriptCmd_PokeMove,
    StaScriptCmd_PokeMoveBy,   StaScriptCmd_PokeStopAnime, StaScriptCmd_PokeStartAnime, StaScriptCmd_PokeChangeAnime,
    StaScriptCmd_PokeJump,     StaScriptCmd_SetPokeMask,   StaScriptCmd_ObjAdd,         StaScriptCmd_ObjDel,
    StaScriptCmd_ObjShow,      StaScriptCmd_ObjHide,       StaScriptCmd_ObjMove,        StaScriptCmd_EffectAdd,
    StaScriptCmd_EffectDel,    StaScriptCmd_EmitterCreate, StaScriptCmd_EmitterDelete,  StaScriptCmd_EmitterRandom,
    StaScriptCmd_LightAdd,     StaScriptCmd_LightDel,      StaScriptCmd_LightMove,      StaScriptCmd_LightFollow,
    StaScriptCmd_LightColor,   StaScriptCmd_PrintMessage,  StaScriptCmd_ClearMessage,   StaScriptCmd_MessageColor,
    StaScriptCmd_PlaySeq,      StaScriptCmd_StopSeq,       StaScriptCmd_PokeRotate,     StaScriptCmd_PokeFront,
    StaScriptCmd_PokeShowItem, StaScriptCmd_PokeAppeal,    StaScriptCmd_AudienceFocus,  StaScriptCmd_AudienceUnfocus,
    StaScriptCmd_ButtonEnable, StaScriptCmd_ButtonDisable, StaScriptCmd_LineUp,         StaScriptCmd_Nop,
    StaScriptCmd_PlayBgm,      StaScriptCmd_StopBgm,       StaScriptCmd_AppealScript,   StaScriptCmd_PokeScript,
    StaScriptCmd_PlayWave,     StaScriptCmd_PlayStrm,
};
