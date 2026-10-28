#ifndef POKEBW2_FIELD_FIELD_ACMD_H
#define POKEBW2_FIELD_FIELD_ACMD_H

#include "types.h"
#include "field/field_actor.h"
#include "struct_decls.h"

// An action command: the command in the low 16 bits and how many times to run it in the high 16. A list of them ends
// with ACMD_END
#define ACMD(code, count) (((count) << 16) | (code))
#define ACMD_END 0xfe
// A list that ends with ACMD_END_KEEP ends the same way, but keeps its queue
#define ACMD_END_KEEP 0xfd
// The command of an actor that has none
#define ACMD_NONE 0xff

// The action commands, by their index in ACTOR_ACTION_COMMAND_TABLES. Most come in four, one for each direction, from
// the one for DIR_UP named here (GetAcmdForDir adds the direction). The walks take 32 to 2 frames a tile and the walks
// on the spot as long (swan's ACMD_WALK*_TABLE and ACMD_TURN*_TABLE); the jumps go 1 and 2 tiles. Our names
enum {
    ACMD_FACE = 0x00,
    ACMD_WALK_32F = 0x04,
    ACMD_WALK_16F = 0x08,
    ACMD_WALK_8F = 0x0c,
    ACMD_WALK_4F = 0x10,
    ACMD_WALK_2F = 0x14,
    ACMD_STAY_WALK_32F = 0x18,
    ACMD_STAY_WALK_16F = 0x1c,
    ACMD_STAY_WALK_8F = 0x20,
    ACMD_STAY_WALK_4F = 0x24,
    ACMD_STAY_WALK_2F = 0x28,
    ACMD_JUMP_1 = 0x34,
    ACMD_JUMP_2 = 0x38,
    // The "!" bubble over the actor (fldeff_emotion_bubble.c's kind 0) with its sound; one command, for no direction
    ACMD_EMOTION_BUBBLE = 0x4b,
    // The walks that take 6 and 12 frames a tile, uphill from ACMD_WALK_4F and ACMD_WALK_8F
    ACMD_WALK_UPHILL_4F = 0x4c,
    ACMD_WALK_3F = 0x50,
    ACMD_DASH = 0x58,
    ACMD_WALK_7F = 0x60,
    ACMD_BIKE_DEEP_SAND = 0x9b,
    ACMD_DASH_UPHILL = 0xa3,
    ACMD_WALK_UPHILL_8F = 0xa7,
};

FieldAcmdTCB *FieldAcmdTCB_Create(FieldActor *actor, const u32 *action);
FieldAcmdTCB *FieldAcmdTCB_CreateWalkOneTile(FieldActor *actor, u32 direction);
BOOL FieldAcmdTCB_CheckEnded(FieldAcmdTCB *task);
void FieldAcmdTCB_Remove(FieldAcmdTCB *task);

extern const u32 ACMD_QUEUE_WALK_N_8F[2];
extern const u32 ACMD_QUEUE_WALK_S_8F[2];
extern const u32 ACMD_QUEUE_WALK_W_8F[2];
extern const u32 ACMD_QUEUE_WALK_E_8F[2];

// The functions of each action command, by the command's state, which field_actor_movedata.c's
// ACTOR_ACTION_COMMAND_TABLES lists by command
extern const ActorAcmdFunc ACMD_SET_FACE_DIR_LOCK_FLAG_TABLE[];
extern const ActorAcmdFunc data_ov036_021cacb4[];
extern const ActorAcmdFunc ACMD_SET_ANIME10_TABLE[];
extern const ActorAcmdFunc data_ov036_021cacc4[];
extern const ActorAcmdFunc ACMD_FACE_RIGHT_TABLE[];
extern const ActorAcmdFunc ACMD_SET_ANIME_PAUSE_FLAG_TABLE[];
extern const ActorAcmdFunc ACMD_CLEAR_FACE_DIR_LOCK_FLAG_TABLE[];
extern const ActorAcmdFunc ACMD_RESET_DRAW_OFFSET_TABLE[];
extern const ActorAcmdFunc ACMD_CLEAR_HIDDEN_FLAG_TABLE[];
extern const ActorAcmdFunc ACMD_DIR_TABLE[];
extern const ActorAcmdFunc ACMD_CLEAR_ANIME_PAUSE_FLAG_TABLE[];
extern const ActorAcmdFunc ACMD_SET_HIDDEN_FLAG_TABLE[];
extern const ActorAcmdFunc ACMD_FACE_DOWN_TABLE[];
extern const ActorAcmdFunc ACMD_FACE_LEFT_TABLE[];
extern const ActorAcmdFunc ACMD_HOP_DOWN_FACE_UP8_F_TABLE[];
extern const ActorAcmdFunc ACMD_MARK_WAIT_EXCL_NO_SE_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_WALK_BOB_INIT_TABLE[];
extern const ActorAcmdFunc ACMD_DRAW_READY_WAIT_INIT10_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_LEFT16_F_FAST_ANIME_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_DOWN16_F_FAST_ANIME_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_UP16_F_FAST_ANIME_TABLE[];
extern const ActorAcmdFunc ACMD_JUMP_RIGHT3_G24_F_NO_SE_TABLE[];
extern const ActorAcmdFunc data_ov036_021cada0[];
extern const ActorAcmdFunc ACMD_HOP_RIGHT_FACE_LEFT8_F_TABLE[];
extern const ActorAcmdFunc ACMD_MARK_WAIT_EXCL_TABLE[];
extern const ActorAcmdFunc ACMD_STEP_RIGHT_FACE_LEFT4_F_TABLE[];
extern const ActorAcmdFunc ACMD_HOP_UP_FACE_DOWN8_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_RIGHT16_F_FAST_ANIME_TABLE[];
extern const ActorAcmdFunc ACMD_DRAW_READY_WAIT_INIT11_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_WALK22_F_INIT_TABLE[];
extern const ActorAcmdFunc ACMD_DASH_STEP_RIGHT_FACE_LEFT4_F_TABLE[];
extern const ActorAcmdFunc ACMD_DRAW_READY_WAIT_INIT12_TABLE[];
extern const ActorAcmdFunc ACMD_HOP_UP_FACE_LEFT8_F_TABLE[];
extern const ActorAcmdFunc ACMD_HOP_DOWN_FACE_RIGHT8_F_TABLE[];
extern const ActorAcmdFunc ACMD_ANIME_WAIT_END_INIT11_SE_TABLE[];
extern const ActorAcmdFunc ACMD_DASH_STEP_LEFT_FACE_RIGHT4_F_TABLE[];
extern const ActorAcmdFunc ACMD_DASH_STEP_DOWN_FACE_UP4_F_TABLE[];
extern const ActorAcmdFunc ACMD_DASH_STEP_UP_FACE_DOWN4_F_TABLE[];
extern const ActorAcmdFunc data_ov036_021cae60[];
extern const ActorAcmdFunc ACMD_DASH_STEP_LEFT_FACE_UP4_F_TABLE[];
extern const ActorAcmdFunc ACMD_DASH_STEP_DOWN_FACE_LEFT4_F_TABLE[];
extern const ActorAcmdFunc ACMD_DASH_STEP_UP_FACE_RIGHT4_F_TABLE[];
extern const ActorAcmdFunc ACMD_DASH_STEP_RIGHT_FACE_UP4_F_TABLE[];
extern const ActorAcmdFunc ACMD_DASH_STEP_LEFT_FACE_DOWN4_F_TABLE[];
extern const ActorAcmdFunc ACMD_DASH_STEP_DOWN_FACE_RIGHT4_F_TABLE[];
extern const ActorAcmdFunc ACMD_DASH_STEP_UP_FACE_LEFT4_F_TABLE[];
extern const ActorAcmdFunc ACMD_DASH_RIGHT6_F_TABLE[];
extern const ActorAcmdFunc ACMD_JUMP_LEFT3_G24_F_NO_SE_TABLE[];
extern const ActorAcmdFunc ACMD_JUMP_DOWN3_G24_F_NO_SE_TABLE[];
extern const ActorAcmdFunc ACMD_JUMP_UP3_G24_F_NO_SE_TABLE[];
extern const ActorAcmdFunc ACMD_STEP_RIGHT_FACE_LEFT2_F_TABLE[];
extern const ActorAcmdFunc ACMD_STEP_LEFT_FACE_RIGHT2_F_TABLE[];
extern const ActorAcmdFunc ACMD_HOP_LEFT_FACE_RIGHT8_F_TABLE[];
extern const ActorAcmdFunc ACMD_FLY_READY_INIT_TABLE[];
extern const ActorAcmdFunc ACMD_DASH_LEFT2_F_TABLE[];
extern const ActorAcmdFunc ACMD_STEP_LEFT_FACE_RIGHT4_F_TABLE[];
extern const ActorAcmdFunc ACMD_STEP_DOWN_FACE_UP4_F_TABLE[];
extern const ActorAcmdFunc ACMD_STEP_UP_FACE_DOWN4_F_TABLE[];
extern const ActorAcmdFunc ACMD_HOP_LEFT_FACE_DOWN8_F_TABLE[];
extern const ActorAcmdFunc ACMD_HOP_RIGHT_FACE_UP8_F_TABLE[];
extern const ActorAcmdFunc ACMD_FLY_UP_INIT_TABLE[];
extern const ActorAcmdFunc ACMD_DRAW_READY_WAIT_INIT13_TABLE[];
extern const ActorAcmdFunc ACMD_STEP_UP_FACE_RIGHT8_F_TABLE[];
extern const ActorAcmdFunc ACMD_STEP_DOWN_FACE_LEFT8_F_TABLE[];
extern const ActorAcmdFunc ACMD_HOP_UP_FACE_RIGHT8_F_TABLE[];
extern const ActorAcmdFunc ACMD_STEP_LEFT_FACE_UP8_F_TABLE[];
extern const ActorAcmdFunc ACMD_STEP_RIGHT_FACE_UP8_F_TABLE[];
extern const ActorAcmdFunc ACMD_STEP_LEFT_FACE_DOWN8_F_TABLE[];
extern const ActorAcmdFunc ACMD_STEP_DOWN_FACE_RIGHT8_F_TABLE[];
extern const ActorAcmdFunc ACMD_STEP_UP_FACE_LEFT8_F_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_IDLE8_F_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_WALK16_F41_F_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_WALK32_F40_F_TABLE[];
extern const ActorAcmdFunc ACMD_HIDE_OFF_INIT_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_WALK_DOWN4_F_TABLE[];
extern const ActorAcmdFunc ACMD_ANIME_WAIT_END_INIT10_SE_TABLE[];
extern const ActorAcmdFunc ACMD_ANIME_WAIT_END_INIT12_ANIME_WAIT_END_TABLE[];
extern const ActorAcmdFunc ACMD_ANIME_WAIT_END_INIT12_ANIME_WAIT_END_STOP_TABLE[];
extern const ActorAcmdFunc ACMD_ANIME_WAIT_END_INIT11_ANIME_WAIT_END_TABLE[];
extern const ActorAcmdFunc ACMD_ANIME_WAIT_END_INIT11_ANIME_WAIT_END_STOP_TABLE[];
extern const ActorAcmdFunc ACMD_ANIME_WAIT_END_INIT10_ANIME_WAIT_END_TABLE[];
extern const ActorAcmdFunc ACMD_ANIME_WAIT_END_INIT10_ANIME_WAIT_END_STOP_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_RIGHT12_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_LEFT12_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_DOWN12_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_UP12_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_RIGHT7_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_LEFT7_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_DOWN7_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_UP7_F_TABLE[];
extern const ActorAcmdFunc ACMD_DASH_RIGHT4_F_TABLE[];
extern const ActorAcmdFunc ACMD_DASH_LEFT4_F_TABLE[];
extern const ActorAcmdFunc ACMD_DASH_DOWN4_F_TABLE[];
extern const ActorAcmdFunc ACMD_RUN4F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_RIGHT3_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_LEFT3_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_DOWN3_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_UP3_F_TABLE[];
extern const ActorAcmdFunc ACMD_DASH_LEFT6_F_TABLE[];
extern const ActorAcmdFunc ACMD_DASH_DOWN6_F_TABLE[];
extern const ActorAcmdFunc ACMD_DASH_UP6_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_RIGHT6_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_LEFT6_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_DOWN6_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_UP6_F_TABLE[];
extern const ActorAcmdFunc ACMD_MARK_WAIT_ELLIPSIS_TABLE[];
extern const ActorAcmdFunc ACMD_MARK_WAIT_NOTE_TABLE[];
extern const ActorAcmdFunc ACMD_MARK_WAIT_QUESTION_TABLE[];
extern const ActorAcmdFunc data_ov036_021cb1b4[];
extern const ActorAcmdFunc ACMD_MARK_WAIT_NOTE_NO_SE_TABLE[];
extern const ActorAcmdFunc ACMD_STEP_DOWN_FACE_UP2_F_TABLE[];
extern const ActorAcmdFunc ACMD_STEP_UP_FACE_DOWN2_F_TABLE[];
extern const ActorAcmdFunc ACMD_JUMP_RIGHT1_G16_F_TABLE[];
extern const ActorAcmdFunc data_ov036_021cb1f0[];
extern const ActorAcmdFunc ACMD_HOP_DOWN_FACE_LEFT8_F_TABLE[];
extern const ActorAcmdFunc ACMD_STEP_UP_FACE_DOWN8_F_TABLE[];
extern const ActorAcmdFunc ACMD_STEP_DOWN_FACE_UP8_F_TABLE[];
extern const ActorAcmdFunc ACMD_HOP_LEFT_FACE_UP8_F_TABLE[];
extern const ActorAcmdFunc ACMD_FALL_INIT_TABLE[];
extern const ActorAcmdFunc ACMD_RISE_INIT_TABLE[];
extern const ActorAcmdFunc ACMD_WAIT32_F_TABLE[];
extern const ActorAcmdFunc ACMD_WAIT4_F_TABLE[];
extern const ActorAcmdFunc ACMD_WAIT8_F_TABLE[];
extern const ActorAcmdFunc ACMD_STEP_LEFT_FACE_RIGHT8_F_TABLE[];
extern const ActorAcmdFunc ACMD_WAIT15_F_TABLE[];
extern const ActorAcmdFunc ACMD_WAIT2_F_TABLE[];
extern const ActorAcmdFunc ACMD_WAIT1_F_TABLE[];
extern const ActorAcmdFunc ACMD_JUMP_RIGHT3_G12_F_TABLE[];
extern const ActorAcmdFunc ACMD_JUMP_LEFT3_G12_F_TABLE[];
extern const ActorAcmdFunc ACMD_WAIT16_F_TABLE[];
extern const ActorAcmdFunc ACMD_JUMP3G16F_TABLE[];
extern const ActorAcmdFunc ACMD_JUMP_RIGHT2_G16_F_TABLE[];
extern const ActorAcmdFunc ACMD_JUMP_LEFT2_G16_F_TABLE[];
extern const ActorAcmdFunc ACMD_JUMP_DOWN2_G16_F_TABLE[];
extern const ActorAcmdFunc ACMD_JUMP_UP2_G16_F_TABLE[];
extern const ActorAcmdFunc ACMD_JUMP_RIGHT1_G8_F_TABLE[];
extern const ActorAcmdFunc ACMD_JUMP_LEFT1_G8_F_TABLE[];
extern const ActorAcmdFunc ACMD_JUMP_DOWN1_G8_F_TABLE[];
extern const ActorAcmdFunc ACMD_JUMP_UP1_G8_F_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_JUMP_RIGHT8_F_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_JUMP_LEFT8_F_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_JUMP_DOWN8_F_TABLE[];
extern const ActorAcmdFunc ACMD_JUMP8F[];
extern const ActorAcmdFunc ACMD_STAY_JUMP_RIGHT16_F_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_JUMP_LEFT16_F_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_JUMP_DOWN16_F_TABLE[];
extern const ActorAcmdFunc ACMD_JUMP16F[];
extern const ActorAcmdFunc ACMD_STAY_WALK_RIGHT2_F_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_WALK_LEFT2_F_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_WALK_DOWN2_F_TABLE[];
extern const ActorAcmdFunc ACMD_TURN2_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_WALK_RIGHT4_F_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_WALK_LEFT4_F_TABLE[];
extern const ActorAcmdFunc ACMD_STEP_RIGHT_FACE_LEFT8_F_TABLE[];
extern const ActorAcmdFunc ACMD_TURN4_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_WALK_RIGHT8_F_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_WALK_LEFT8_F_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_WALK_DOWN8_F_TABLE[];
extern const ActorAcmdFunc ACMD_TURN8_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_WALK_RIGHT16_F_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_WALK_LEFT16_F_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_WALK_DOWN16_F_TABLE[];
extern const ActorAcmdFunc ACMD_TURN16_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_WALK_RIGHT32_F_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_WALK_LEFT32_F_TABLE[];
extern const ActorAcmdFunc ACMD_STAY_WALK_DOWN32_F_TABLE[];
extern const ActorAcmdFunc ACMD_TURN32_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_RIGHT1_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_LEFT1_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_DOWN1_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_UP1_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_RIGHT2_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_LEFT2_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_DOWN2_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK2_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_RIGHT4_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_LEFT4_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_DOWN4_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK4_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_RIGHT8_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_LEFT8_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_DOWN8_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK8_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_RIGHT16_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_LEFT16_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_DOWN16_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK16_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_RIGHT32_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_LEFT32_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK_DOWN32_F_TABLE[];
extern const ActorAcmdFunc ACMD_WALK32_TABLE[];
extern const ActorAcmdFunc data_ov036_021cb598[];
extern const ActorAcmdFunc ACMD_ANIME_WAIT_END_FACE_DOWN_INIT_TABLE[];

#endif // POKEBW2_FIELD_FIELD_ACMD_H
