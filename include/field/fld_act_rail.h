#ifndef POKEBW2_FIELD_FLD_ACT_RAIL_H
#define POKEBW2_FIELD_FLD_ACT_RAIL_H

#include "types.h"
#include "field/field_actor.h"

// Overlay 36's fld_act_rail.c: the functions of the rail actors' action commands, by the command's state, which
// field_actor_movedata.c's ACTOR_ACTION_COMMAND_TABLES_RAIL lists by command. The first five, the turns, are done in
// their first frame and have only a start and an end function
extern const ActorAcmdFunc RAIL_ACMD_FACE_RIGHT_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_FACE_LEFT_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_FACE_UP_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_FACE_DOWN_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_NEXT_STATE_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_STAY_WALK_LEFT_4F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_JUMP_DOWN_2G_16F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_STAY_WALK_UP_32F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_STAY_WALK_DOWN_2F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_STAY_WALK_LEFT_2F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_STAY_WALK_UP_2F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_JUMP_LEFT_1G_8F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_JUMP_RIGHT_2G_16F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_STAY_WALK_DOWN_4F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_JUMP_RIGHT_1G_8F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_STAY_WALK_DOWN_8F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_STAY_WALK_LEFT_8F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_STAY_WALK_RIGHT_2F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_STAY_WALK_UP_8F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_STAY_WALK_RIGHT_16F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_STAY_WALK_LEFT_16F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_STAY_WALK_DOWN_16F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_STAY_WALK_UP_16F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_STAY_WALK_RIGHT_32F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_STAY_WALK_LEFT_32F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_STAY_WALK_RIGHT_4F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_JUMP_DOWN_1G_8F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_DASH_RIGHT_4F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_DASH_LEFT_4F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_DASH_DOWN_4F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_WALK_DOWN_2F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_WALK_LEFT_2F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_STAY_WALK_RIGHT_8F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_WALK_RIGHT_2F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_WALK_UP_2F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_WALK_RIGHT_4F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_WALK_LEFT_4F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_WALK_DOWN_4F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_WALK_UP_4F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_WALK_RIGHT_8F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_WALK_LEFT_8F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_WALK_DOWN_8F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_WALK_UP_8F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_WALK_RIGHT_16F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_WALK_LEFT_16F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_WALK_DOWN_16F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_WALK_UP_16F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_DASH_UP_4F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_STAY_WALK_UP_4F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_JUMP_UP_1G_8F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_JUMP_UP_2G_16F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_JUMP_LEFT_2G_16F_TABLE[];
extern const ActorAcmdFunc RAIL_ACMD_STAY_WALK_DOWN_32F_TABLE[];

#endif // POKEBW2_FIELD_FLD_ACT_RAIL_H
