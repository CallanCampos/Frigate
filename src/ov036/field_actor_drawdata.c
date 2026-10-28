// The billboard actors' animations, by animation set (FIELD_BILLBOARD_ANIM_SETS, which field_actor_billboard.c reads),
// each set a table of animations by animation number, defined after the animations it adds, set by set; the functions
// of each kind of scene node (FIELD_ACTOR_MAIN_VTABLES); and the scale and face size of each billboard size.
// FIELD_ACTOR_MAIN_VTABLES, BILLBOARD_SCALES and ACTOR_BILLBOARD_SIZES_INT are swan's names
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0); the animation-set table's name and type are ours.
// The ROM gives this file no name, so
// field_actor_drawdata.c, after field_actor_movedata.c, is a guess. It holds only data, whose .rodata
// (0x021cd0f0-0x021cf2d8 in Black 2) is one run sorted by size after field_actor_movedata.c's. Nothing in the file
// refers to BILLBOARD_SCALES and ACTOR_BILLBOARD_SIZES_INT, so they get sections of their own, after the shared one

// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "field/field_actor.h"
#include "field/field_actor_drawdata.h"
#include "field/field_actor_internal.h"
#include "gfl/billboard_actor.h"

const FieldActorSceneNodeVTable *const FIELD_ACTOR_MAIN_VTABLES[] = {
    &FIELD_ACTOR_VTABLE_VANISH,          &FIELD_ACTOR_VTABLE_DIR_ANIME_1,      &FIELD_ACTOR_VTABLE_DIR_ANIME_B,
    &FIELD_ACTOR_VTABLE_DIR_ANIME_3,     &FIELD_ACTOR_VTABLE_DIR_ANIME_MOVING, &FIELD_ACTOR_VTABLE_DIR_ANIME_SIZED,
    &FIELD_ACTOR_VTABLE_TWO_FRAME,       &FIELD_ACTOR_VTABLE_THREE_FRAME,      &FIELD_ACTOR_VTABLE_POSITION_ONLY,
    &FIELD_ACTOR_VTABLE_WALK_ANIME,      &FIELD_ACTOR_VTABLE_WALKING,          &FIELD_ACTOR_VTABLE_WALKING,
    &FIELD_ACTOR_VTABLE_G3D_OBJ,         &FIELD_ACTOR_VTABLE_BILLBOARD_OFFSET, &FIELD_ACTOR_VTABLE_STATIC,
    &FIELD_ACTOR_VTABLE_DIR_ANIME_SIZED, &FIELD_ACTOR_VTABLE_MULTI_PATTERN,    &FIELD_ACTOR_VTABLE_TWO_PATTERN,
    &FIELD_ACTOR_VTABLE_THREE_PATTERN,   &FIELD_ACTOR_VTABLE_WALK_ANIME_B,     &FIELD_ACTOR_VTABLE_DIR_ANIME_SIZED,
};

// The size of a billboard actor's faces in pixels, and its scale, by its billboard size
const BillboardSize ACTOR_BILLBOARD_SIZES_INT[3] = { { 32, 32 }, { 16, 16 }, { 64, 64 } };

const BillboardScale BILLBOARD_SCALES[3] = { { 0x2000, 0x2000 }, { 0x1000, 0x1000 }, { 0x4000, 0x4000 } };

// Animation set 1
static const BillboardActorAnimStep sBillboardSet1Anim0[] = { { 0, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet1Anim1[] = { { 3, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet1Anim2[] = { { 6, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet1Anim3[] = { { 9, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet1Anim4[] = {
    { 1, 0, 0, 16 }, { 0, 0, 0, 16 }, { 2, 0, 0, 16 }, { 0, 0, 0, 16 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim5[] = {
    { 4, 0, 0, 16 }, { 3, 0, 0, 16 }, { 5, 0, 0, 16 }, { 3, 0, 0, 16 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim6[] = {
    { 7, 0, 0, 16 }, { 6, 0, 0, 16 }, { 8, 0, 0, 16 }, { 6, 0, 0, 16 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim7[] = {
    { 10, 0, 0, 16 }, { 9, 0, 0, 16 }, { 11, 0, 0, 16 }, { 9, 0, 0, 16 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim8[] = {
    { 1, 0, 0, 8 }, { 0, 0, 0, 8 }, { 2, 0, 0, 8 }, { 0, 0, 0, 8 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim9[] = {
    { 4, 0, 0, 8 }, { 3, 0, 0, 8 }, { 5, 0, 0, 8 }, { 3, 0, 0, 8 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim10[] = {
    { 7, 0, 0, 8 }, { 6, 0, 0, 8 }, { 8, 0, 0, 8 }, { 6, 0, 0, 8 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim11[] = {
    { 10, 0, 0, 8 }, { 9, 0, 0, 8 }, { 11, 0, 0, 8 }, { 9, 0, 0, 8 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim12[] = {
    { 1, 0, 0, 4 }, { 0, 0, 0, 4 }, { 2, 0, 0, 4 }, { 0, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim13[] = {
    { 4, 0, 0, 4 }, { 3, 0, 0, 4 }, { 5, 0, 0, 4 }, { 3, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim14[] = {
    { 7, 0, 0, 4 }, { 6, 0, 0, 4 }, { 8, 0, 0, 4 }, { 6, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim15[] = {
    { 10, 0, 0, 4 }, { 9, 0, 0, 4 }, { 11, 0, 0, 4 }, { 9, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim16[] = {
    { 1, 0, 0, 2 }, { 0, 0, 0, 2 }, { 2, 0, 0, 2 }, { 0, 0, 0, 2 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim17[] = {
    { 4, 0, 0, 2 }, { 3, 0, 0, 2 }, { 5, 0, 0, 2 }, { 3, 0, 0, 2 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim18[] = {
    { 7, 0, 0, 2 }, { 6, 0, 0, 2 }, { 8, 0, 0, 2 }, { 6, 0, 0, 2 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim19[] = {
    { 10, 0, 0, 2 }, { 9, 0, 0, 2 }, { 11, 0, 0, 2 }, { 9, 0, 0, 2 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim20[] = {
    { 1, 0, 0, 1 }, { 0, 0, 0, 1 }, { 2, 0, 0, 1 }, { 0, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim21[] = {
    { 4, 0, 0, 1 }, { 3, 0, 0, 1 }, { 5, 0, 0, 1 }, { 3, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim22[] = {
    { 7, 0, 0, 1 }, { 6, 0, 0, 1 }, { 8, 0, 0, 1 }, { 6, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim23[] = {
    { 10, 0, 0, 1 }, { 9, 0, 0, 1 }, { 11, 0, 0, 1 }, { 9, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim24[] = {
    { 1, 0, 0, 3 }, { 0, 0, 0, 3 }, { 2, 0, 0, 3 }, { 0, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim25[] = {
    { 4, 0, 0, 3 }, { 3, 0, 0, 3 }, { 5, 0, 0, 3 }, { 3, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim26[] = {
    { 7, 0, 0, 3 }, { 6, 0, 0, 3 }, { 8, 0, 0, 3 }, { 6, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim27[] = {
    { 10, 0, 0, 3 }, { 9, 0, 0, 3 }, { 11, 0, 0, 3 }, { 9, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim28[] = {
    { 1, 0, 0, 2 }, { 0, 0, 0, 1 }, { 2, 0, 0, 2 }, { 0, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim29[] = {
    { 4, 0, 0, 2 }, { 3, 0, 0, 1 }, { 5, 0, 0, 2 }, { 3, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim30[] = {
    { 7, 0, 0, 2 }, { 6, 0, 0, 1 }, { 8, 0, 0, 2 }, { 6, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim31[] = {
    { 10, 0, 0, 2 }, { 9, 0, 0, 1 }, { 11, 0, 0, 2 }, { 9, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim32[] = {
    { 1, 0, 0, 4 }, { 0, 0, 0, 3 }, { 2, 0, 0, 4 }, { 0, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim33[] = {
    { 4, 0, 0, 4 }, { 3, 0, 0, 3 }, { 5, 0, 0, 4 }, { 3, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim34[] = {
    { 7, 0, 0, 4 }, { 6, 0, 0, 3 }, { 8, 0, 0, 4 }, { 6, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim35[] = {
    { 10, 0, 0, 4 }, { 9, 0, 0, 3 }, { 11, 0, 0, 4 }, { 9, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim36[] = {
    { 1, 0, 0, 6 }, { 0, 0, 0, 6 }, { 2, 0, 0, 6 }, { 0, 0, 0, 6 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim37[] = {
    { 4, 0, 0, 6 }, { 3, 0, 0, 6 }, { 5, 0, 0, 6 }, { 3, 0, 0, 6 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim38[] = {
    { 7, 0, 0, 6 }, { 6, 0, 0, 6 }, { 8, 0, 0, 6 }, { 6, 0, 0, 6 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim39[] = {
    { 10, 0, 0, 6 }, { 9, 0, 0, 6 }, { 11, 0, 0, 6 }, { 9, 0, 0, 6 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim40[] = {
    { 13, 0, 0, 4 }, { 12, 0, 0, 4 }, { 14, 0, 0, 4 }, { 12, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim41[] = {
    { 16, 0, 0, 4 }, { 15, 0, 0, 4 }, { 17, 0, 0, 4 }, { 15, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim42[] = {
    { 19, 0, 0, 4 }, { 18, 0, 0, 4 }, { 20, 0, 0, 4 }, { 18, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim43[] = {
    { 22, 0, 0, 4 }, { 21, 0, 0, 4 }, { 23, 0, 0, 4 }, { 21, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim44[] = {
    { 13, 0, 0, 4 }, { 12, 0, 0, 4 }, { 14, 0, 0, 4 }, { 12, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim45[] = {
    { 16, 0, 0, 4 }, { 15, 0, 0, 4 }, { 17, 0, 0, 4 }, { 15, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim46[] = {
    { 19, 0, 0, 4 }, { 18, 0, 0, 4 }, { 20, 0, 0, 4 }, { 18, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet1Anim47[] = {
    { 22, 0, 0, 4 }, { 21, 0, 0, 4 }, { 23, 0, 0, 4 }, { 21, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet1Animations[] = {
    sBillboardSet1Anim0,  sBillboardSet1Anim1,  sBillboardSet1Anim2,  sBillboardSet1Anim3,  sBillboardSet1Anim4,
    sBillboardSet1Anim5,  sBillboardSet1Anim6,  sBillboardSet1Anim7,  sBillboardSet1Anim8,  sBillboardSet1Anim9,
    sBillboardSet1Anim10, sBillboardSet1Anim11, sBillboardSet1Anim12, sBillboardSet1Anim13, sBillboardSet1Anim14,
    sBillboardSet1Anim15, sBillboardSet1Anim16, sBillboardSet1Anim17, sBillboardSet1Anim18, sBillboardSet1Anim19,
    sBillboardSet1Anim20, sBillboardSet1Anim21, sBillboardSet1Anim22, sBillboardSet1Anim23, sBillboardSet1Anim24,
    sBillboardSet1Anim25, sBillboardSet1Anim26, sBillboardSet1Anim27, sBillboardSet1Anim28, sBillboardSet1Anim29,
    sBillboardSet1Anim30, sBillboardSet1Anim31, sBillboardSet1Anim32, sBillboardSet1Anim33, sBillboardSet1Anim34,
    sBillboardSet1Anim35, sBillboardSet1Anim36, sBillboardSet1Anim37, sBillboardSet1Anim38, sBillboardSet1Anim39,
    sBillboardSet1Anim40, sBillboardSet1Anim41, sBillboardSet1Anim42, sBillboardSet1Anim43, sBillboardSet1Anim44,
    sBillboardSet1Anim45, sBillboardSet1Anim46, sBillboardSet1Anim47,
};

// Animation set 2
static const BillboardActorAnimStep sBillboardSet2Anim0[] = { { 0, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet2Anim1[] = { { 2, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet2Anim2[] = { { 4, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet2Anim3[] = { { 4, 1, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet2Anim4[] = {
    { 1, 0, 0, 16 }, { 0, 0, 0, 16 }, { 1, 1, 0, 16 }, { 0, 0, 0, 16 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim5[] = {
    { 3, 0, 0, 16 }, { 2, 0, 0, 16 }, { 3, 1, 0, 16 }, { 2, 0, 0, 16 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim6[] = {
    { 5, 0, 0, 16 }, { 4, 0, 0, 16 }, { 6, 0, 0, 16 }, { 4, 0, 0, 16 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim7[] = {
    { 5, 1, 0, 16 }, { 4, 1, 0, 16 }, { 6, 1, 0, 16 }, { 4, 1, 0, 16 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim8[] = {
    { 1, 0, 0, 8 }, { 0, 0, 0, 8 }, { 1, 1, 0, 8 }, { 0, 0, 0, 8 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim9[] = {
    { 3, 0, 0, 8 }, { 2, 0, 0, 8 }, { 3, 1, 0, 8 }, { 2, 0, 0, 8 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim10[] = {
    { 5, 0, 0, 8 }, { 4, 0, 0, 8 }, { 6, 0, 0, 8 }, { 4, 0, 0, 8 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim11[] = {
    { 5, 1, 0, 8 }, { 4, 1, 0, 8 }, { 6, 1, 0, 8 }, { 4, 1, 0, 8 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim12[] = {
    { 1, 0, 0, 4 }, { 0, 0, 0, 4 }, { 1, 1, 0, 4 }, { 0, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim13[] = {
    { 3, 0, 0, 4 }, { 2, 0, 0, 4 }, { 3, 1, 0, 4 }, { 2, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim14[] = {
    { 5, 0, 0, 4 }, { 4, 0, 0, 4 }, { 6, 0, 0, 4 }, { 4, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim15[] = {
    { 5, 1, 0, 4 }, { 4, 1, 0, 4 }, { 6, 1, 0, 4 }, { 4, 1, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim16[] = {
    { 1, 0, 0, 2 }, { 0, 0, 0, 2 }, { 1, 1, 0, 2 }, { 0, 0, 0, 2 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim17[] = {
    { 3, 0, 0, 2 }, { 2, 0, 0, 2 }, { 3, 1, 0, 2 }, { 2, 0, 0, 2 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim18[] = {
    { 5, 0, 0, 2 }, { 4, 0, 0, 2 }, { 6, 0, 0, 2 }, { 4, 0, 0, 2 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim19[] = {
    { 5, 1, 0, 2 }, { 4, 1, 0, 2 }, { 6, 1, 0, 2 }, { 4, 1, 0, 2 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim20[] = {
    { 1, 0, 0, 1 }, { 0, 0, 0, 1 }, { 1, 1, 0, 1 }, { 0, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim21[] = {
    { 3, 0, 0, 1 }, { 2, 0, 0, 1 }, { 3, 1, 0, 1 }, { 2, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim22[] = {
    { 5, 0, 0, 1 }, { 4, 0, 0, 1 }, { 6, 0, 0, 1 }, { 4, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim23[] = {
    { 5, 1, 0, 1 }, { 4, 1, 0, 1 }, { 6, 1, 0, 1 }, { 4, 1, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim24[] = {
    { 1, 0, 0, 3 }, { 0, 0, 0, 3 }, { 1, 1, 0, 3 }, { 0, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim25[] = {
    { 3, 0, 0, 3 }, { 2, 0, 0, 3 }, { 3, 1, 0, 3 }, { 2, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim26[] = {
    { 5, 0, 0, 3 }, { 4, 0, 0, 3 }, { 6, 0, 0, 3 }, { 4, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim27[] = {
    { 5, 1, 0, 3 }, { 4, 1, 0, 3 }, { 6, 1, 0, 3 }, { 4, 1, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim28[] = {
    { 1, 0, 0, 2 }, { 0, 0, 0, 1 }, { 1, 1, 0, 2 }, { 0, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim29[] = {
    { 3, 0, 0, 2 }, { 2, 0, 0, 1 }, { 3, 1, 0, 2 }, { 2, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim30[] = {
    { 5, 0, 0, 2 }, { 4, 0, 0, 1 }, { 6, 0, 0, 2 }, { 4, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim31[] = {
    { 5, 1, 0, 2 }, { 4, 1, 0, 1 }, { 6, 1, 0, 2 }, { 4, 1, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim32[] = {
    { 1, 0, 0, 4 }, { 0, 0, 0, 3 }, { 1, 1, 0, 4 }, { 0, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim33[] = {
    { 3, 0, 0, 4 }, { 2, 0, 0, 3 }, { 3, 1, 0, 4 }, { 2, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim34[] = {
    { 5, 0, 0, 4 }, { 4, 0, 0, 3 }, { 6, 0, 0, 4 }, { 4, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim35[] = {
    { 5, 1, 0, 4 }, { 4, 1, 0, 3 }, { 6, 1, 0, 4 }, { 4, 1, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim36[] = {
    { 1, 0, 0, 6 }, { 0, 0, 0, 6 }, { 1, 1, 0, 6 }, { 0, 0, 0, 6 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim37[] = {
    { 3, 0, 0, 6 }, { 2, 0, 0, 6 }, { 3, 1, 0, 6 }, { 2, 0, 0, 6 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim38[] = {
    { 5, 0, 0, 6 }, { 4, 0, 0, 6 }, { 6, 0, 0, 6 }, { 4, 0, 0, 6 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet2Anim39[] = {
    { 5, 1, 0, 6 }, { 4, 1, 0, 6 }, { 6, 1, 0, 6 }, { 4, 1, 0, 6 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet2Animations[] = {
    sBillboardSet2Anim0,  sBillboardSet2Anim1,  sBillboardSet2Anim2,  sBillboardSet2Anim3,  sBillboardSet2Anim4,
    sBillboardSet2Anim5,  sBillboardSet2Anim6,  sBillboardSet2Anim7,  sBillboardSet2Anim8,  sBillboardSet2Anim9,
    sBillboardSet2Anim10, sBillboardSet2Anim11, sBillboardSet2Anim12, sBillboardSet2Anim13, sBillboardSet2Anim14,
    sBillboardSet2Anim15, sBillboardSet2Anim16, sBillboardSet2Anim17, sBillboardSet2Anim18, sBillboardSet2Anim19,
    sBillboardSet2Anim20, sBillboardSet2Anim21, sBillboardSet2Anim22, sBillboardSet2Anim23, sBillboardSet2Anim24,
    sBillboardSet2Anim25, sBillboardSet2Anim26, sBillboardSet2Anim27, sBillboardSet2Anim28, sBillboardSet2Anim29,
    sBillboardSet2Anim30, sBillboardSet2Anim31, sBillboardSet2Anim32, sBillboardSet2Anim33, sBillboardSet2Anim34,
    sBillboardSet2Anim35, sBillboardSet2Anim36, sBillboardSet2Anim37, sBillboardSet2Anim38, sBillboardSet2Anim39,
};

// Animation set 3
static const BillboardActorAnimStep sBillboardSet3Anim0[] = { { 0, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet3Anim1[] = { { 3, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet3Anim2[] = { { 6, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet3Anim3[] = { { 9, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet3Anim4[] = {
    { 1, 0, 0, 16 }, { 0, 0, 0, 16 }, { 2, 0, 0, 16 }, { 0, 0, 0, 16 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim5[] = {
    { 4, 0, 0, 16 }, { 3, 0, 0, 16 }, { 5, 0, 0, 16 }, { 3, 0, 0, 16 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim6[] = {
    { 7, 0, 0, 16 }, { 6, 0, 0, 16 }, { 8, 0, 0, 16 }, { 6, 0, 0, 16 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim7[] = {
    { 10, 0, 0, 16 }, { 9, 0, 0, 16 }, { 11, 0, 0, 16 }, { 9, 0, 0, 16 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim8[] = {
    { 1, 0, 0, 8 }, { 0, 0, 0, 8 }, { 2, 0, 0, 8 }, { 0, 0, 0, 8 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim9[] = {
    { 4, 0, 0, 8 }, { 3, 0, 0, 8 }, { 5, 0, 0, 8 }, { 3, 0, 0, 8 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim10[] = {
    { 7, 0, 0, 8 }, { 6, 0, 0, 8 }, { 8, 0, 0, 8 }, { 6, 0, 0, 8 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim11[] = {
    { 10, 0, 0, 8 }, { 9, 0, 0, 8 }, { 11, 0, 0, 8 }, { 9, 0, 0, 8 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim12[] = {
    { 1, 0, 0, 4 }, { 0, 0, 0, 4 }, { 2, 0, 0, 4 }, { 0, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim13[] = {
    { 4, 0, 0, 4 }, { 3, 0, 0, 4 }, { 5, 0, 0, 4 }, { 3, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim14[] = {
    { 7, 0, 0, 4 }, { 6, 0, 0, 4 }, { 8, 0, 0, 4 }, { 6, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim15[] = {
    { 10, 0, 0, 4 }, { 9, 0, 0, 4 }, { 11, 0, 0, 4 }, { 9, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim16[] = {
    { 1, 0, 0, 2 }, { 0, 0, 0, 2 }, { 2, 0, 0, 2 }, { 0, 0, 0, 2 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim17[] = {
    { 4, 0, 0, 2 }, { 3, 0, 0, 2 }, { 5, 0, 0, 2 }, { 3, 0, 0, 2 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim18[] = {
    { 7, 0, 0, 2 }, { 6, 0, 0, 2 }, { 8, 0, 0, 2 }, { 6, 0, 0, 2 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim19[] = {
    { 10, 0, 0, 2 }, { 9, 0, 0, 2 }, { 11, 0, 0, 2 }, { 9, 0, 0, 2 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim20[] = {
    { 1, 0, 0, 1 }, { 0, 0, 0, 1 }, { 2, 0, 0, 1 }, { 0, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim21[] = {
    { 4, 0, 0, 1 }, { 3, 0, 0, 1 }, { 5, 0, 0, 1 }, { 3, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim22[] = {
    { 7, 0, 0, 1 }, { 6, 0, 0, 1 }, { 8, 0, 0, 1 }, { 6, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim23[] = {
    { 10, 0, 0, 1 }, { 9, 0, 0, 1 }, { 11, 0, 0, 1 }, { 9, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim24[] = {
    { 1, 0, 0, 3 }, { 0, 0, 0, 3 }, { 2, 0, 0, 3 }, { 0, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim25[] = {
    { 4, 0, 0, 3 }, { 3, 0, 0, 3 }, { 5, 0, 0, 3 }, { 3, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim26[] = {
    { 7, 0, 0, 3 }, { 6, 0, 0, 3 }, { 8, 0, 0, 3 }, { 6, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim27[] = {
    { 10, 0, 0, 3 }, { 9, 0, 0, 3 }, { 11, 0, 0, 3 }, { 9, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim28[] = {
    { 1, 0, 0, 2 }, { 0, 0, 0, 1 }, { 2, 0, 0, 2 }, { 0, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim29[] = {
    { 4, 0, 0, 2 }, { 3, 0, 0, 1 }, { 5, 0, 0, 2 }, { 3, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim30[] = {
    { 7, 0, 0, 2 }, { 6, 0, 0, 1 }, { 8, 0, 0, 2 }, { 6, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim31[] = {
    { 10, 0, 0, 2 }, { 9, 0, 0, 1 }, { 11, 0, 0, 2 }, { 9, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim32[] = {
    { 1, 0, 0, 4 }, { 0, 0, 0, 3 }, { 2, 0, 0, 4 }, { 0, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim33[] = {
    { 4, 0, 0, 4 }, { 3, 0, 0, 3 }, { 5, 0, 0, 4 }, { 3, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim34[] = {
    { 7, 0, 0, 4 }, { 6, 0, 0, 3 }, { 8, 0, 0, 4 }, { 6, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim35[] = {
    { 10, 0, 0, 4 }, { 9, 0, 0, 3 }, { 11, 0, 0, 4 }, { 9, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim36[] = {
    { 1, 0, 0, 6 }, { 0, 0, 0, 6 }, { 2, 0, 0, 6 }, { 0, 0, 0, 6 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim37[] = {
    { 4, 0, 0, 6 }, { 3, 0, 0, 6 }, { 5, 0, 0, 6 }, { 3, 0, 0, 6 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim38[] = {
    { 7, 0, 0, 6 }, { 6, 0, 0, 6 }, { 8, 0, 0, 6 }, { 6, 0, 0, 6 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet3Anim39[] = {
    { 10, 0, 0, 6 }, { 9, 0, 0, 6 }, { 11, 0, 0, 6 }, { 9, 0, 0, 6 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet3Animations[] = {
    sBillboardSet3Anim0,  sBillboardSet3Anim1,  sBillboardSet3Anim2,  sBillboardSet3Anim3,  sBillboardSet3Anim4,
    sBillboardSet3Anim5,  sBillboardSet3Anim6,  sBillboardSet3Anim7,  sBillboardSet3Anim8,  sBillboardSet3Anim9,
    sBillboardSet3Anim10, sBillboardSet3Anim11, sBillboardSet3Anim12, sBillboardSet3Anim13, sBillboardSet3Anim14,
    sBillboardSet3Anim15, sBillboardSet3Anim16, sBillboardSet3Anim17, sBillboardSet3Anim18, sBillboardSet3Anim19,
    sBillboardSet3Anim20, sBillboardSet3Anim21, sBillboardSet3Anim22, sBillboardSet3Anim23, sBillboardSet3Anim24,
    sBillboardSet3Anim25, sBillboardSet3Anim26, sBillboardSet3Anim27, sBillboardSet3Anim28, sBillboardSet3Anim29,
    sBillboardSet3Anim30, sBillboardSet3Anim31, sBillboardSet3Anim32, sBillboardSet3Anim33, sBillboardSet3Anim34,
    sBillboardSet3Anim35, sBillboardSet3Anim36, sBillboardSet3Anim37, sBillboardSet3Anim38, sBillboardSet3Anim39,
};

// Animation set 4
static const BillboardActorAnimStep sBillboardSet4Anim0[] = { { 0, 0, 0, 4 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 } };

static const BillboardActorAnimStep *const sBillboardSet4Animations[] = {
    sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0,
    sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0,
    sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0,
    sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0,
    sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0,
    sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0,
    sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0,
    sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0, sBillboardSet4Anim0,
};

// Animation set 5
static const BillboardActorAnimStep sBillboardSet5Anim0[] = { { 0, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet5Anim1[] = { { 1, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet5Anim2[] = { { 2, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet5Anim3[] = { { 3, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep *const sBillboardSet5Animations[] = {
    sBillboardSet5Anim0,
    sBillboardSet5Anim1,
    sBillboardSet5Anim2,
    sBillboardSet5Anim3,
};

// Animation set 6
static const BillboardActorAnimStep sBillboardSet6Anim0[] = { { 12, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet6Anim1[] = { { 13, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet6Anim2[] = { { 14, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet6Anim3[] = { { 15, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet6Anim4[] = {
    { 1, 0, 0, 16 }, { 0, 0, 0, 16 }, { 2, 0, 0, 16 }, { 0, 0, 0, 16 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim5[] = {
    { 4, 0, 0, 16 }, { 3, 0, 0, 16 }, { 5, 0, 0, 16 }, { 3, 0, 0, 16 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim6[] = {
    { 7, 0, 0, 16 }, { 6, 0, 0, 16 }, { 8, 0, 0, 16 }, { 6, 0, 0, 16 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim7[] = {
    { 10, 0, 0, 16 }, { 9, 0, 0, 16 }, { 11, 0, 0, 16 }, { 9, 0, 0, 16 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim8[] = {
    { 1, 0, 0, 8 }, { 0, 0, 0, 8 }, { 2, 0, 0, 8 }, { 0, 0, 0, 8 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim9[] = {
    { 4, 0, 0, 8 }, { 3, 0, 0, 8 }, { 5, 0, 0, 8 }, { 3, 0, 0, 8 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim10[] = {
    { 7, 0, 0, 8 }, { 6, 0, 0, 8 }, { 8, 0, 0, 8 }, { 6, 0, 0, 8 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim11[] = {
    { 10, 0, 0, 8 }, { 9, 0, 0, 8 }, { 11, 0, 0, 8 }, { 9, 0, 0, 8 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim12[] = {
    { 1, 0, 0, 4 }, { 0, 0, 0, 4 }, { 2, 0, 0, 4 }, { 0, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim13[] = {
    { 4, 0, 0, 4 }, { 3, 0, 0, 4 }, { 5, 0, 0, 4 }, { 3, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim14[] = {
    { 7, 0, 0, 4 }, { 6, 0, 0, 4 }, { 8, 0, 0, 4 }, { 6, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim15[] = {
    { 10, 0, 0, 4 }, { 9, 0, 0, 4 }, { 11, 0, 0, 4 }, { 9, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim16[] = {
    { 1, 0, 0, 2 }, { 0, 0, 0, 2 }, { 2, 0, 0, 2 }, { 0, 0, 0, 2 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim17[] = {
    { 4, 0, 0, 2 }, { 3, 0, 0, 2 }, { 5, 0, 0, 2 }, { 3, 0, 0, 2 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim18[] = {
    { 7, 0, 0, 2 }, { 6, 0, 0, 2 }, { 8, 0, 0, 2 }, { 6, 0, 0, 2 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim19[] = {
    { 10, 0, 0, 2 }, { 9, 0, 0, 2 }, { 11, 0, 0, 2 }, { 9, 0, 0, 2 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim20[] = {
    { 1, 0, 0, 4 }, { 0, 0, 0, 4 }, { 2, 0, 0, 4 }, { 0, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim21[] = {
    { 4, 0, 0, 4 }, { 3, 0, 0, 4 }, { 5, 0, 0, 4 }, { 3, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim22[] = {
    { 7, 0, 0, 4 }, { 6, 0, 0, 4 }, { 8, 0, 0, 4 }, { 6, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim23[] = {
    { 10, 0, 0, 4 }, { 9, 0, 0, 4 }, { 11, 0, 0, 4 }, { 9, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim24[] = {
    { 1, 0, 0, 3 }, { 0, 0, 0, 3 }, { 2, 0, 0, 3 }, { 0, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim25[] = {
    { 4, 0, 0, 3 }, { 3, 0, 0, 3 }, { 5, 0, 0, 3 }, { 3, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim26[] = {
    { 7, 0, 0, 3 }, { 6, 0, 0, 3 }, { 8, 0, 0, 3 }, { 6, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim27[] = {
    { 10, 0, 0, 3 }, { 9, 0, 0, 3 }, { 11, 0, 0, 3 }, { 9, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim28[] = {
    { 1, 0, 0, 2 }, { 0, 0, 0, 1 }, { 2, 0, 0, 2 }, { 0, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim29[] = {
    { 4, 0, 0, 2 }, { 3, 0, 0, 1 }, { 5, 0, 0, 2 }, { 3, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim30[] = {
    { 7, 0, 0, 2 }, { 6, 0, 0, 1 }, { 8, 0, 0, 2 }, { 6, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim31[] = {
    { 10, 0, 0, 2 }, { 9, 0, 0, 1 }, { 11, 0, 0, 2 }, { 9, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim32[] = {
    { 1, 0, 0, 4 }, { 0, 0, 0, 3 }, { 2, 0, 0, 4 }, { 0, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim33[] = {
    { 4, 0, 0, 4 }, { 3, 0, 0, 3 }, { 5, 0, 0, 4 }, { 3, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim34[] = {
    { 7, 0, 0, 4 }, { 6, 0, 0, 3 }, { 8, 0, 0, 4 }, { 6, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim35[] = {
    { 10, 0, 0, 4 }, { 9, 0, 0, 3 }, { 11, 0, 0, 4 }, { 9, 0, 0, 3 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim36[] = {
    { 1, 0, 0, 6 }, { 0, 0, 0, 6 }, { 2, 0, 0, 6 }, { 0, 0, 0, 6 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim37[] = {
    { 4, 0, 0, 6 }, { 3, 0, 0, 6 }, { 5, 0, 0, 6 }, { 3, 0, 0, 6 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim38[] = {
    { 7, 0, 0, 6 }, { 6, 0, 0, 6 }, { 8, 0, 0, 6 }, { 6, 0, 0, 6 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet6Anim39[] = {
    { 10, 0, 0, 6 }, { 9, 0, 0, 6 }, { 11, 0, 0, 6 }, { 9, 0, 0, 6 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet6Animations[] = {
    sBillboardSet6Anim0,  sBillboardSet6Anim1,  sBillboardSet6Anim2,  sBillboardSet6Anim3,  sBillboardSet6Anim4,
    sBillboardSet6Anim5,  sBillboardSet6Anim6,  sBillboardSet6Anim7,  sBillboardSet6Anim8,  sBillboardSet6Anim9,
    sBillboardSet6Anim10, sBillboardSet6Anim11, sBillboardSet6Anim12, sBillboardSet6Anim13, sBillboardSet6Anim14,
    sBillboardSet6Anim15, sBillboardSet6Anim16, sBillboardSet6Anim17, sBillboardSet6Anim18, sBillboardSet6Anim19,
    sBillboardSet6Anim20, sBillboardSet6Anim21, sBillboardSet6Anim22, sBillboardSet6Anim23, sBillboardSet6Anim24,
    sBillboardSet6Anim25, sBillboardSet6Anim26, sBillboardSet6Anim27, sBillboardSet6Anim28, sBillboardSet6Anim29,
    sBillboardSet6Anim30, sBillboardSet6Anim31, sBillboardSet6Anim32, sBillboardSet6Anim33, sBillboardSet6Anim34,
    sBillboardSet6Anim35, sBillboardSet6Anim36, sBillboardSet6Anim37, sBillboardSet6Anim38, sBillboardSet6Anim39,
};

// Animation set 7
static const BillboardActorAnimStep sBillboardSet7Anim0[] = { { 0, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet7Anim1[] = { { 0, 0, 0, 5 },
                                                              { 1, 0, 0, 4 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep *const sBillboardSet7Animations[] = { sBillboardSet7Anim0, sBillboardSet7Anim1 };

// Animation set 8
static const BillboardActorAnimStep sBillboardSet8Anim0[] = { { 4, 0, 0, 1 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet8Anim1[] = {
    { 0, 0, 0, 5 },  { 1, 0, 0, 5 }, { 0, 0, 0, 5 }, { 1, 0, 0, 5 },
    { 2, 0, 0, 10 }, { 3, 0, 0, 5 }, { 4, 0, 0, 5 }, { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet8Anim2[] = {
    { 4, 0, 0, 5 }, { 3, 0, 0, 5 }, { 2, 0, 0, 10 },
    { 1, 0, 0, 5 }, { 0, 0, 0, 5 }, { 1, 0, 0, 5 },
    { 0, 0, 0, 5 }, { 4, 0, 0, 1 }, { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet8Animations[] = {
    sBillboardSet8Anim0,
    sBillboardSet8Anim1,
    sBillboardSet8Anim2,
};

// Animation set 9
static const BillboardActorAnimStep sBillboardSet9Anim0[] = { { 0, 0, 0, 4 },
                                                              { 1, 0, 0, 4 },
                                                              { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep *const sBillboardSet9Animations[] = { sBillboardSet9Anim0 };

// Animation set 10
static const BillboardActorAnimStep sBillboardSet10Anim0[] = {
    { 0, 0, 0, 4 }, { 1, 0, 0, 4 }, { 2, 0, 0, 4 }, { 3, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet10Anim1[] = {
    { 4, 0, 0, 4 }, { 5, 0, 0, 4 }, { 6, 0, 0, 4 }, { 7, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet10Anim2[] = {
    { 8, 0, 0, 4 }, { 9, 0, 0, 4 }, { 10, 0, 0, 4 }, { 11, 0, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet10Anim3[] = {
    { 8, 1, 0, 4 }, { 9, 1, 0, 4 }, { 10, 1, 0, 4 }, { 11, 1, 0, 4 }, { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet10Anim4[] = {
    { 2, 0, 0, 4 },
    { 1, 0, 0, 4 },
    { 0, 0, 0, 4 },
    { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet10Anim5[] = {
    { 6, 0, 0, 4 },
    { 5, 0, 0, 4 },
    { 4, 0, 0, 4 },
    { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet10Anim6[] = {
    { 10, 0, 0, 4 },
    { 9, 0, 0, 4 },
    { 8, 0, 0, 4 },
    { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet10Anim7[] = {
    { 10, 1, 0, 4 },
    { 9, 1, 0, 4 },
    { 8, 1, 0, 4 },
    { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet10Anim8[] = { { 1, 0, 0, 4 },
                                                               { 2, 0, 0, 4 },
                                                               { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet10Anim9[] = { { 5, 0, 0, 4 },
                                                               { 6, 0, 0, 4 },
                                                               { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet10Anim10[] = {
    { 9, 0, 0, 4 },
    { 10, 0, 0, 4 },
    { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet10Anim11[] = {
    { 9, 1, 0, 4 },
    { 10, 1, 0, 4 },
    { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet10Anim12[] = {
    { 2, 0, 0, 4 },
    { 1, 0, 0, 4 },
    { 0, 0, 0, 4 },
    { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet10Anim13[] = {
    { 6, 0, 0, 4 },
    { 5, 0, 0, 4 },
    { 4, 0, 0, 4 },
    { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet10Anim14[] = {
    { 10, 0, 0, 4 },
    { 9, 0, 0, 4 },
    { 8, 0, 0, 4 },
    { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet10Anim15[] = {
    { 10, 1, 0, 4 },
    { 9, 1, 0, 4 },
    { 8, 1, 0, 4 },
    { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet10Animations[] = {
    sBillboardSet10Anim0,  sBillboardSet10Anim1,  sBillboardSet10Anim2,  sBillboardSet10Anim3,
    sBillboardSet10Anim4,  sBillboardSet10Anim5,  sBillboardSet10Anim6,  sBillboardSet10Anim7,
    sBillboardSet10Anim8,  sBillboardSet10Anim9,  sBillboardSet10Anim10, sBillboardSet10Anim11,
    sBillboardSet10Anim12, sBillboardSet10Anim13, sBillboardSet10Anim14, sBillboardSet10Anim15,
};

// Animation set 11
static const BillboardActorAnimStep sBillboardSet11Anim0[] = {
    { 0, 0, 0, 2 }, { 1, 0, 0, 2 }, { 2, 0, 0, 2 }, { 3, 0, 0, 2 }, { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet11Animations[] = { sBillboardSet11Anim0 };

// Animation set 12
static const BillboardActorAnimStep sBillboardSet12Anim0[] = {
    { 0, 0, 0, 4 },
    { 1, 0, 0, 4 },
    { 0, 0, 0, 4 },
    { 2, 0, 0, 4 },
    { 3, 0, 0, 4 },
    { 2, 0, 0, 4 },
    { 0, 0, 0, 3 },
    { 1, 0, 0, 3 },
    { 0, 0, 0, 3 },
    { 2, 0, 0, 3 },
    { 3, 0, 0, 3 },
    { 2, 0, 0, 3 },
    { 0, 0, 0, 2 },
    { 1, 0, 0, 2 },
    { 0, 0, 0, 2 },
    { 2, 0, 0, 2 },
    { 3, 0, 0, 2 },
    { 2, 0, 0, 2 },
    { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 12 },
};

static const BillboardActorAnimStep sBillboardSet12Anim1[] = {
    { 4, 0, 0, 4 },
    { 5, 0, 0, 4 },
    { 4, 0, 0, 4 },
    { 6, 0, 0, 4 },
    { 7, 0, 0, 4 },
    { 6, 0, 0, 4 },
    { 4, 0, 0, 3 },
    { 5, 0, 0, 3 },
    { 4, 0, 0, 3 },
    { 6, 0, 0, 3 },
    { 7, 0, 0, 3 },
    { 6, 0, 0, 3 },
    { 4, 0, 0, 2 },
    { 5, 0, 0, 2 },
    { 4, 0, 0, 2 },
    { 6, 0, 0, 2 },
    { 7, 0, 0, 2 },
    { 6, 0, 0, 2 },
    { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 12 },
};

static const BillboardActorAnimStep sBillboardSet12Anim2[] = {
    { 8, 0, 0, 4 },
    { 9, 0, 0, 4 },
    { 8, 0, 0, 4 },
    { 10, 0, 0, 4 },
    { 11, 0, 0, 4 },
    { 10, 0, 0, 4 },
    { 8, 0, 0, 3 },
    { 9, 0, 0, 3 },
    { 8, 0, 0, 3 },
    { 10, 0, 0, 3 },
    { 11, 0, 0, 3 },
    { 10, 0, 0, 3 },
    { 8, 0, 0, 2 },
    { 9, 0, 0, 2 },
    { 8, 0, 0, 2 },
    { 10, 0, 0, 2 },
    { 11, 0, 0, 2 },
    { 10, 0, 0, 2 },
    { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 12 },
};

static const BillboardActorAnimStep sBillboardSet12Anim3[] = {
    { 12, 0, 0, 4 },
    { 13, 0, 0, 4 },
    { 12, 0, 0, 4 },
    { 14, 0, 0, 4 },
    { 15, 0, 0, 4 },
    { 14, 0, 0, 4 },
    { 12, 0, 0, 3 },
    { 13, 0, 0, 3 },
    { 12, 0, 0, 3 },
    { 14, 0, 0, 3 },
    { 15, 0, 0, 3 },
    { 14, 0, 0, 3 },
    { 12, 0, 0, 2 },
    { 13, 0, 0, 2 },
    { 12, 0, 0, 2 },
    { 14, 0, 0, 2 },
    { 15, 0, 0, 2 },
    { 14, 0, 0, 2 },
    { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 12 },
};

static const BillboardActorAnimStep *const sBillboardSet12Animations[] = {
    sBillboardSet12Anim0,
    sBillboardSet12Anim1,
    sBillboardSet12Anim2,
    sBillboardSet12Anim3,
};

// Animation set 13
static const BillboardActorAnimStep sBillboardSet13Anim0[] = {
    { 0, 0, 0, 10 },
    { 1, 0, 0, 10 },
    { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet13Anim1[] = {
    { 2, 0, 0, 5 },
    { 3, 0, 0, 10 },
    { 2, 0, 0, 5 },
    { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet13Anim2[] = {
    { 4, 0, 0, 10 },
    { 5, 0, 0, 10 },
    { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet13Anim3[] = {
    { 4, 1, 0, 10 },
    { 5, 1, 0, 10 },
    { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet13Animations[] = {
    sBillboardSet13Anim0,
    sBillboardSet13Anim1,
    sBillboardSet13Anim2,
    sBillboardSet13Anim3,
};

// Animation set 14
static const BillboardActorAnimStep sBillboardSet14Anim0[] = {
    { 0, 0, 0, 10 },
    { 1, 0, 0, 10 },
    { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet14Anim1[] = {
    { 2, 0, 0, 5 },
    { 3, 0, 0, 10 },
    { 2, 0, 0, 5 },
    { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet14Anim2[] = {
    { 4, 0, 0, 10 },
    { 5, 0, 0, 10 },
    { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet14Anim3[] = {
    { 6, 0, 0, 10 },
    { 7, 0, 0, 10 },
    { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet14Animations[] = {
    sBillboardSet14Anim0,
    sBillboardSet14Anim1,
    sBillboardSet14Anim2,
    sBillboardSet14Anim3,
};

// Animation set 15
static const BillboardActorAnimStep sBillboardSet15Anim0[] = {
    { 2, 0, 0, 2 }, { 7, 0, 0, 4 }, { 8, 0, 0, 6 },
    { 7, 0, 0, 4 }, { 2, 0, 0, 2 }, { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet15Animations[] = {
    sBillboardSet2Anim0,  sBillboardSet2Anim1,  sBillboardSet2Anim2,  sBillboardSet2Anim3,  sBillboardSet2Anim4,
    sBillboardSet2Anim5,  sBillboardSet2Anim6,  sBillboardSet2Anim7,  sBillboardSet2Anim8,  sBillboardSet2Anim9,
    sBillboardSet2Anim10, sBillboardSet2Anim11, sBillboardSet2Anim12, sBillboardSet2Anim13, sBillboardSet2Anim14,
    sBillboardSet2Anim15, sBillboardSet2Anim16, sBillboardSet2Anim17, sBillboardSet2Anim18, sBillboardSet2Anim19,
    sBillboardSet2Anim20, sBillboardSet2Anim21, sBillboardSet2Anim22, sBillboardSet2Anim23, sBillboardSet2Anim24,
    sBillboardSet2Anim25, sBillboardSet2Anim26, sBillboardSet2Anim27, sBillboardSet2Anim28, sBillboardSet2Anim29,
    sBillboardSet2Anim30, sBillboardSet2Anim31, sBillboardSet2Anim32, sBillboardSet2Anim33, sBillboardSet2Anim34,
    sBillboardSet2Anim35, sBillboardSet2Anim36, sBillboardSet2Anim37, sBillboardSet2Anim38, sBillboardSet2Anim39,
    sBillboardSet15Anim0,
};

// Animation set 16
static const BillboardActorAnimStep sBillboardSet16Anim0[] = {
    { 12, 0, 0, 16 },
    { 13, 0, 0, 8 },
    { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet16Animations[] = {
    sBillboardSet3Anim0,  sBillboardSet3Anim1,  sBillboardSet3Anim2,  sBillboardSet3Anim3,  sBillboardSet3Anim4,
    sBillboardSet3Anim5,  sBillboardSet3Anim6,  sBillboardSet3Anim7,  sBillboardSet3Anim8,  sBillboardSet3Anim9,
    sBillboardSet3Anim10, sBillboardSet3Anim11, sBillboardSet3Anim12, sBillboardSet3Anim13, sBillboardSet3Anim14,
    sBillboardSet3Anim15, sBillboardSet3Anim16, sBillboardSet3Anim17, sBillboardSet3Anim18, sBillboardSet3Anim19,
    sBillboardSet3Anim20, sBillboardSet3Anim21, sBillboardSet3Anim22, sBillboardSet3Anim23, sBillboardSet3Anim24,
    sBillboardSet3Anim25, sBillboardSet3Anim26, sBillboardSet3Anim27, sBillboardSet3Anim28, sBillboardSet3Anim29,
    sBillboardSet3Anim30, sBillboardSet3Anim31, sBillboardSet3Anim32, sBillboardSet3Anim33, sBillboardSet3Anim34,
    sBillboardSet3Anim35, sBillboardSet3Anim36, sBillboardSet3Anim37, sBillboardSet3Anim38, sBillboardSet3Anim39,
    sBillboardSet16Anim0,
};

// Animation set 17
static const BillboardActorAnimStep sBillboardSet17Anim0[] = {
    { 3, 0, 0, 10 }, { 12, 0, 0, 8 }, { 13, 0, 0, 25 },
    { 14, 0, 0, 3 }, { 3, 0, 0, 10 }, { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet17Animations[] = {
    sBillboardSet3Anim0,  sBillboardSet3Anim1,  sBillboardSet3Anim2,  sBillboardSet3Anim3,  sBillboardSet3Anim4,
    sBillboardSet3Anim5,  sBillboardSet3Anim6,  sBillboardSet3Anim7,  sBillboardSet3Anim8,  sBillboardSet3Anim9,
    sBillboardSet3Anim10, sBillboardSet3Anim11, sBillboardSet3Anim12, sBillboardSet3Anim13, sBillboardSet3Anim14,
    sBillboardSet3Anim15, sBillboardSet3Anim16, sBillboardSet3Anim17, sBillboardSet3Anim18, sBillboardSet3Anim19,
    sBillboardSet3Anim20, sBillboardSet3Anim21, sBillboardSet3Anim22, sBillboardSet3Anim23, sBillboardSet3Anim24,
    sBillboardSet3Anim25, sBillboardSet3Anim26, sBillboardSet3Anim27, sBillboardSet3Anim28, sBillboardSet3Anim29,
    sBillboardSet3Anim30, sBillboardSet3Anim31, sBillboardSet3Anim32, sBillboardSet3Anim33, sBillboardSet3Anim34,
    sBillboardSet3Anim35, sBillboardSet3Anim36, sBillboardSet3Anim37, sBillboardSet3Anim38, sBillboardSet3Anim39,
    sBillboardSet17Anim0,
};

// Animation set 18
static const BillboardActorAnimStep sBillboardSet18Anim0[] = {
    { 3, 0, 0, 10 },  { 12, 0, 0, 5 }, { 13, 0, 0, 2 }, { 15, 0, 0, 10 },
    { 14, 0, 0, 10 }, { 13, 0, 0, 1 }, { 3, 0, 0, 10 }, { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet18Anim1[] = {
    { 3, 0, 0, 4 },  { 16, 0, 0, 4 }, { 17, 0, 0, 4 }, { 16, 0, 0, 4 }, { 18, 0, 0, 4 },
    { 16, 0, 0, 4 }, { 17, 0, 0, 4 }, { 16, 0, 0, 4 }, { 3, 0, 0, 4 },  { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet18Anim2[] = {
    { 3, 0, 0, 10 }, { 21, 0, 0, 5 }, { 19, 0, 0, 2 }, { 20, 0, 0, 10 }, { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet18Animations[] = {
    sBillboardSet3Anim0,  sBillboardSet3Anim1,  sBillboardSet3Anim2,  sBillboardSet3Anim3,  sBillboardSet3Anim4,
    sBillboardSet3Anim5,  sBillboardSet3Anim6,  sBillboardSet3Anim7,  sBillboardSet3Anim8,  sBillboardSet3Anim9,
    sBillboardSet3Anim10, sBillboardSet3Anim11, sBillboardSet3Anim12, sBillboardSet3Anim13, sBillboardSet3Anim14,
    sBillboardSet3Anim15, sBillboardSet3Anim16, sBillboardSet3Anim17, sBillboardSet3Anim18, sBillboardSet3Anim19,
    sBillboardSet3Anim20, sBillboardSet3Anim21, sBillboardSet3Anim22, sBillboardSet3Anim23, sBillboardSet3Anim24,
    sBillboardSet3Anim25, sBillboardSet3Anim26, sBillboardSet3Anim27, sBillboardSet3Anim28, sBillboardSet3Anim29,
    sBillboardSet3Anim30, sBillboardSet3Anim31, sBillboardSet3Anim32, sBillboardSet3Anim33, sBillboardSet3Anim34,
    sBillboardSet3Anim35, sBillboardSet3Anim36, sBillboardSet3Anim37, sBillboardSet3Anim38, sBillboardSet3Anim39,
    sBillboardSet18Anim0, sBillboardSet18Anim1, sBillboardSet18Anim2,
};

// Animation set 19
static const BillboardActorAnimStep sBillboardSet19Anim0[] = {
    { 3, 0, 0, 10 }, { 12, 0, 0, 5 }, { 13, 0, 0, 2 }, { 15, 0, 0, 10 },
    { 14, 0, 0, 5 }, { 13, 0, 0, 2 }, { 3, 0, 0, 10 }, { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet19Animations[] = {
    sBillboardSet3Anim0,  sBillboardSet3Anim1,  sBillboardSet3Anim2,  sBillboardSet3Anim3,  sBillboardSet3Anim4,
    sBillboardSet3Anim5,  sBillboardSet3Anim6,  sBillboardSet3Anim7,  sBillboardSet3Anim8,  sBillboardSet3Anim9,
    sBillboardSet3Anim10, sBillboardSet3Anim11, sBillboardSet3Anim12, sBillboardSet3Anim13, sBillboardSet3Anim14,
    sBillboardSet3Anim15, sBillboardSet3Anim16, sBillboardSet3Anim17, sBillboardSet3Anim18, sBillboardSet3Anim19,
    sBillboardSet3Anim20, sBillboardSet3Anim21, sBillboardSet3Anim22, sBillboardSet3Anim23, sBillboardSet3Anim24,
    sBillboardSet3Anim25, sBillboardSet3Anim26, sBillboardSet3Anim27, sBillboardSet3Anim28, sBillboardSet3Anim29,
    sBillboardSet3Anim30, sBillboardSet3Anim31, sBillboardSet3Anim32, sBillboardSet3Anim33, sBillboardSet3Anim34,
    sBillboardSet3Anim35, sBillboardSet3Anim36, sBillboardSet3Anim37, sBillboardSet3Anim38, sBillboardSet3Anim39,
    sBillboardSet19Anim0,
};

// Animation set 20
static const BillboardActorAnimStep sBillboardSet20Anim0[] = {
    { 14, 0, 0, 2 },
    { 13, 0, 0, 2 },
    { 12, 0, 0, 2 },
    { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet20Anim1[] = {
    { 12, 0, 0, 2 },
    { 13, 0, 0, 2 },
    { 14, 0, 0, 2 },
    { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet20Animations[] = {
    sBillboardSet3Anim0,  sBillboardSet3Anim1,  sBillboardSet3Anim2,  sBillboardSet3Anim3,  sBillboardSet3Anim8,
    sBillboardSet3Anim9,  sBillboardSet3Anim10, sBillboardSet3Anim11, sBillboardSet3Anim12, sBillboardSet3Anim13,
    sBillboardSet3Anim14, sBillboardSet3Anim15, sBillboardSet3Anim16, sBillboardSet3Anim17, sBillboardSet3Anim18,
    sBillboardSet3Anim19, sBillboardSet3Anim20, sBillboardSet3Anim21, sBillboardSet3Anim22, sBillboardSet3Anim23,
    sBillboardSet3Anim20, sBillboardSet3Anim21, sBillboardSet3Anim22, sBillboardSet3Anim23, sBillboardSet3Anim24,
    sBillboardSet3Anim25, sBillboardSet3Anim26, sBillboardSet3Anim27, sBillboardSet3Anim28, sBillboardSet3Anim29,
    sBillboardSet3Anim30, sBillboardSet3Anim31, sBillboardSet3Anim32, sBillboardSet3Anim33, sBillboardSet3Anim34,
    sBillboardSet3Anim35, sBillboardSet3Anim36, sBillboardSet3Anim37, sBillboardSet3Anim38, sBillboardSet3Anim39,
    sBillboardSet20Anim0, sBillboardSet20Anim1,
};

// Animation set 21
static const BillboardActorAnimStep sBillboardSet21Anim0[] = {
    { 3, 0, 0, 2 }, { 12, 0, 0, 4 }, { 13, 0, 0, 4 }, { 12, 0, 0, 2 },
    { 3, 0, 0, 2 }, { 13, 0, 0, 4 }, { 3, 0, 0, 2 },  { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet21Animations[] = {
    sBillboardSet3Anim0,  sBillboardSet3Anim1,  sBillboardSet3Anim2,  sBillboardSet3Anim3,  sBillboardSet3Anim4,
    sBillboardSet3Anim5,  sBillboardSet3Anim6,  sBillboardSet3Anim7,  sBillboardSet3Anim8,  sBillboardSet3Anim9,
    sBillboardSet3Anim10, sBillboardSet3Anim11, sBillboardSet3Anim12, sBillboardSet3Anim13, sBillboardSet3Anim14,
    sBillboardSet3Anim15, sBillboardSet3Anim16, sBillboardSet3Anim17, sBillboardSet3Anim18, sBillboardSet3Anim19,
    sBillboardSet3Anim20, sBillboardSet3Anim21, sBillboardSet3Anim22, sBillboardSet3Anim23, sBillboardSet3Anim24,
    sBillboardSet3Anim25, sBillboardSet3Anim26, sBillboardSet3Anim27, sBillboardSet3Anim28, sBillboardSet3Anim29,
    sBillboardSet3Anim30, sBillboardSet3Anim31, sBillboardSet3Anim32, sBillboardSet3Anim33, sBillboardSet3Anim34,
    sBillboardSet3Anim35, sBillboardSet3Anim36, sBillboardSet3Anim37, sBillboardSet3Anim38, sBillboardSet3Anim39,
    sBillboardSet21Anim0,
};

// Animation set 22
static const BillboardActorAnimStep sBillboardSet22Anim0[] = { { 0, 0, 0, 2 },
                                                               { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet22Anim1[] = {
    { 0, 0, 0, 4 }, { 1, 0, 0, 6 }, { 2, 0, 0, 6 }, { 3, 0, 0, 6 }, { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet22Animations[] = { sBillboardSet22Anim0, sBillboardSet22Anim1 };

// Animation set 23
static const BillboardActorAnimStep sBillboardSet23Anim0[] = { { 0, 0, 0, 1 },
                                                               { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep sBillboardSet23Anim1[] = {
    { 0, 0, 0, 5 }, { 1, 0, 0, 5 }, { 2, 0, 0, 5 },
    { 3, 0, 0, 5 }, { 4, 0, 0, 5 }, { 5, 0, 0, 5 },
    { 6, 0, 0, 5 }, { 7, 0, 0, 5 }, { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet23Anim2[] = {
    { 8, 0, 0, 5 },  { 9, 0, 0, 5 },  { 10, 0, 0, 5 },
    { 11, 0, 0, 5 }, { 12, 0, 0, 5 }, { 13, 0, 0, 5 },
    { 14, 0, 0, 5 }, { 15, 0, 0, 5 }, { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet23Anim3[] = {
    { 16, 0, 0, 8 }, { 17, 0, 0, 8 }, { 18, 0, 0, 8 }, { 19, 0, 0, 8 }, { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet23Anim4[] = {
    { 0, 0, 0, 4 },
    { 1, 0, 0, 4 },
    { 2, 0, 0, 4 },
    { 3, 0, 0, 4 },
    { 4, 0, 0, 4 },
    { 5, 0, 0, 4 },
    { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet23Animations[] = {
    sBillboardSet23Anim0, sBillboardSet23Anim1, sBillboardSet23Anim2, sBillboardSet23Anim3, sBillboardSet23Anim4,
};

// Animation set 24
static const BillboardActorAnimStep sBillboardSet24Anim0[] = {
    { 0, 0, 0, 7 }, { 1, 0, 0, 7 }, { 2, 0, 0, 7 },
    { 3, 0, 0, 7 }, { 1, 0, 0, 7 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet24Anim1[] = {
    { 4, 0, 0, 10 },
    { 5, 0, 0, 10 },
    { 6, 0, 0, 10 },
    { 14, 0, 0, 10 },
    { 6, 0, 0, 10 },
    { 5, 0, 0, 10 },
    { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet24Anim2[] = {
    { 0, 0, 0, 7 },
    { 1, 0, 0, 7 },
    { 2, 0, 0, 7 },
    { 3, 0, 0, 7 },
    { 1, 0, 0, 7 },
    { 0, 0, 0, 5 },
    { 13, 0, 0, 5 },
    { 8, 0, 0, 5 },
    { 9, 0, 0, 5 },
    { 10, 0, 0, 5 },
    { 11, 0, 0, 5 },
    { 7, 0, 0, 5 },
    { 8, 0, 0, 5 },
    { 9, 0, 0, 5 },
    { 10, 0, 0, 5 },
    { 11, 0, 0, 5 },
    { 7, 0, 0, 5 },
    { 8, 0, 0, 5 },
    { 9, 0, 0, 5 },
    { 10, 0, 0, 5 },
    { 11, 0, 0, 5 },
    { 7, 0, 0, 5 },
    { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 17 },
};

static const BillboardActorAnimStep sBillboardSet24Anim3[] = { { 12, 0, 0, 1 },
                                                               { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 } };

static const BillboardActorAnimStep *const sBillboardSet24Animations[] = {
    sBillboardSet24Anim0,
    sBillboardSet24Anim1,
    sBillboardSet24Anim2,
    sBillboardSet24Anim3,
};

// Animation set 25
static const BillboardActorAnimStep sBillboardSet25Anim0[] = {
    { 4, 0, 0, 7 }, { 5, 0, 0, 7 }, { 6, 0, 0, 7 },
    { 5, 0, 0, 7 }, { 7, 0, 0, 7 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet25Anim1[] = {
    { 0, 0, 0, 7 }, { 1, 0, 0, 7 }, { 2, 0, 0, 7 },
    { 3, 0, 0, 7 }, { 1, 0, 0, 7 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet25Anim2[] = {
    { 8, 0, 0, 2 },
    { 9, 0, 0, 2 },
    { 13, 0, 0, 2 },
    { 14, 0, 0, 2 },
    { 13, 0, 0, 2 },
    { 14, 0, 0, 2 },
    { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 4 },
};

static const BillboardActorAnimStep sBillboardSet25Anim3[] = {
    { 4, 0, 0, 7 },  { 10, 0, 0, 6 }, { 11, 0, 0, 13 },
    { 12, 0, 0, 6 }, { 0, 0, 0, 4 },  { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet25Animations[] = {
    sBillboardSet25Anim0,
    sBillboardSet25Anim1,
    sBillboardSet25Anim2,
    sBillboardSet25Anim3,
};

// Animation set 26
static const BillboardActorAnimStep sBillboardSet26Anim0[] = {
    { 8, 0, 0, 5 },  { 9, 0, 0, 5 }, { 10, 0, 0, 5 },
    { 11, 0, 0, 5 }, { 7, 0, 0, 5 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet26Animations[] = { sBillboardSet26Anim0 };

// Animation set 27
static const BillboardActorAnimStep sBillboardSet27Anim0[] = {
    { 3, 0, 0, 10 },  { 12, 0, 0, 2 }, { 13, 0, 0, 10 }, { 14, 0, 0, 2 },
    { 16, 0, 0, 20 }, { 15, 0, 0, 2 }, { 16, 0, 0, 2 },  { 15, 0, 0, 2 },
    { 16, 0, 0, 2 },  { 15, 0, 0, 2 }, { 16, 0, 0, 2 },  { 15, 0, 0, 2 },
    { 16, 0, 0, 30 }, { 17, 0, 0, 3 }, { 3, 0, 0, 10 },  { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet27Animations[] = {
    sBillboardSet3Anim0,  sBillboardSet3Anim1,  sBillboardSet3Anim2,  sBillboardSet3Anim3,  sBillboardSet3Anim4,
    sBillboardSet3Anim5,  sBillboardSet3Anim6,  sBillboardSet3Anim7,  sBillboardSet3Anim8,  sBillboardSet3Anim9,
    sBillboardSet3Anim10, sBillboardSet3Anim11, sBillboardSet3Anim12, sBillboardSet3Anim13, sBillboardSet3Anim14,
    sBillboardSet3Anim15, sBillboardSet3Anim16, sBillboardSet3Anim17, sBillboardSet3Anim18, sBillboardSet3Anim19,
    sBillboardSet3Anim20, sBillboardSet3Anim21, sBillboardSet3Anim22, sBillboardSet3Anim23, sBillboardSet3Anim24,
    sBillboardSet3Anim25, sBillboardSet3Anim26, sBillboardSet3Anim27, sBillboardSet3Anim28, sBillboardSet3Anim29,
    sBillboardSet3Anim30, sBillboardSet3Anim31, sBillboardSet3Anim32, sBillboardSet3Anim33, sBillboardSet3Anim34,
    sBillboardSet3Anim35, sBillboardSet3Anim36, sBillboardSet3Anim37, sBillboardSet3Anim38, sBillboardSet3Anim39,
    sBillboardSet27Anim0,
};

// Animation set 28
static const BillboardActorAnimStep sBillboardSet28Anim0[] = {
    { 12, 0, 0, 3 },
    { 13, 0, 0, 3 },
    { 3, 0, 0, 3 },
    { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet28Animations[] = {
    sBillboardSet3Anim0,  sBillboardSet3Anim1,  sBillboardSet3Anim2,  sBillboardSet3Anim3,  sBillboardSet3Anim4,
    sBillboardSet3Anim5,  sBillboardSet3Anim6,  sBillboardSet3Anim7,  sBillboardSet3Anim8,  sBillboardSet3Anim9,
    sBillboardSet3Anim10, sBillboardSet3Anim11, sBillboardSet3Anim12, sBillboardSet3Anim13, sBillboardSet3Anim14,
    sBillboardSet3Anim15, sBillboardSet3Anim16, sBillboardSet3Anim17, sBillboardSet3Anim18, sBillboardSet3Anim19,
    sBillboardSet3Anim20, sBillboardSet3Anim21, sBillboardSet3Anim22, sBillboardSet3Anim23, sBillboardSet3Anim24,
    sBillboardSet3Anim25, sBillboardSet3Anim26, sBillboardSet3Anim27, sBillboardSet3Anim28, sBillboardSet3Anim29,
    sBillboardSet3Anim30, sBillboardSet3Anim31, sBillboardSet3Anim32, sBillboardSet3Anim33, sBillboardSet3Anim34,
    sBillboardSet3Anim35, sBillboardSet3Anim36, sBillboardSet3Anim37, sBillboardSet3Anim38, sBillboardSet3Anim39,
    sBillboardSet28Anim0,
};

// Animation set 29
static const BillboardActorAnimStep sBillboardSet29Anim0[] = {
    { 12, 0, 0, 3 },
    { 13, 0, 0, 10 },
    { 14, 0, 0, 10 },
    { 13, 0, 0, 10 },
    { 14, 0, 0, 10 },
    { 13, 0, 0, 3 },
    { 15, 0, 0, 2 },
    { 16, 0, 0, 3 },
    { 17, 0, 0, 30 },
    { 18, 0, 0, 1 },
    { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet29Anim1[] = {
    { 13, 0, 0, 10 }, { 14, 0, 0, 10 }, { 13, 0, 0, 10 },
    { 14, 0, 0, 10 }, { 13, 0, 0, 3 },  { 15, 0, 0, 2 },
    { 16, 0, 0, 3 },  { 17, 0, 0, 30 }, { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet29Animations[] = {
    sBillboardSet3Anim0,  sBillboardSet3Anim1,  sBillboardSet3Anim2,  sBillboardSet3Anim3,  sBillboardSet3Anim4,
    sBillboardSet3Anim5,  sBillboardSet3Anim6,  sBillboardSet3Anim7,  sBillboardSet3Anim8,  sBillboardSet3Anim9,
    sBillboardSet3Anim10, sBillboardSet3Anim11, sBillboardSet3Anim12, sBillboardSet3Anim13, sBillboardSet3Anim14,
    sBillboardSet3Anim15, sBillboardSet3Anim16, sBillboardSet3Anim17, sBillboardSet3Anim18, sBillboardSet3Anim19,
    sBillboardSet3Anim20, sBillboardSet3Anim21, sBillboardSet3Anim22, sBillboardSet3Anim23, sBillboardSet3Anim24,
    sBillboardSet3Anim25, sBillboardSet3Anim26, sBillboardSet3Anim27, sBillboardSet3Anim28, sBillboardSet3Anim29,
    sBillboardSet3Anim30, sBillboardSet3Anim31, sBillboardSet3Anim32, sBillboardSet3Anim33, sBillboardSet3Anim34,
    sBillboardSet3Anim35, sBillboardSet3Anim36, sBillboardSet3Anim37, sBillboardSet3Anim38, sBillboardSet3Anim39,
    sBillboardSet29Anim0, sBillboardSet29Anim1,
};

// Animation set 30
static const BillboardActorAnimStep sBillboardSet30Anim0[] = {
    { 12, 0, 0, 3 }, { 13, 0, 0, 5 }, { 14, 0, 0, 3 },
    { 15, 0, 0, 8 }, { 16, 0, 0, 2 }, { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet30Animations[] = {
    sBillboardSet3Anim0,  sBillboardSet3Anim1,  sBillboardSet3Anim2,  sBillboardSet3Anim3,  sBillboardSet3Anim4,
    sBillboardSet3Anim5,  sBillboardSet3Anim6,  sBillboardSet3Anim7,  sBillboardSet3Anim8,  sBillboardSet3Anim9,
    sBillboardSet3Anim10, sBillboardSet3Anim11, sBillboardSet3Anim12, sBillboardSet3Anim13, sBillboardSet3Anim14,
    sBillboardSet3Anim15, sBillboardSet3Anim16, sBillboardSet3Anim17, sBillboardSet3Anim18, sBillboardSet3Anim19,
    sBillboardSet3Anim20, sBillboardSet3Anim21, sBillboardSet3Anim22, sBillboardSet3Anim23, sBillboardSet3Anim24,
    sBillboardSet3Anim25, sBillboardSet3Anim26, sBillboardSet3Anim27, sBillboardSet3Anim28, sBillboardSet3Anim29,
    sBillboardSet3Anim30, sBillboardSet3Anim31, sBillboardSet3Anim32, sBillboardSet3Anim33, sBillboardSet3Anim34,
    sBillboardSet3Anim35, sBillboardSet3Anim36, sBillboardSet3Anim37, sBillboardSet3Anim38, sBillboardSet3Anim39,
    sBillboardSet30Anim0,
};

// Animation set 31
static const BillboardActorAnimStep sBillboardSet31Anim0[] = {
    { 3, 0, 0, 5 },  { 12, 0, 0, 4 }, { 13, 0, 0, 11 }, { 14, 0, 0, 5 }, { 13, 0, 0, 8 },
    { 14, 0, 0, 5 }, { 13, 0, 0, 8 }, { 12, 0, 0, 4 },  { 3, 0, 0, 5 },  { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet31Anim1[] = {
    { 3, 0, 0, 2 },  { 12, 0, 0, 6 }, { 15, 0, 0, 3 },
    { 16, 0, 0, 4 }, { 17, 0, 0, 8 }, { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet31Animations[] = {
    sBillboardSet3Anim0,  sBillboardSet3Anim1,  sBillboardSet3Anim2,  sBillboardSet3Anim3,  sBillboardSet3Anim4,
    sBillboardSet3Anim5,  sBillboardSet3Anim6,  sBillboardSet3Anim7,  sBillboardSet3Anim8,  sBillboardSet3Anim9,
    sBillboardSet3Anim10, sBillboardSet3Anim11, sBillboardSet3Anim12, sBillboardSet3Anim13, sBillboardSet3Anim14,
    sBillboardSet3Anim15, sBillboardSet3Anim16, sBillboardSet3Anim17, sBillboardSet3Anim18, sBillboardSet3Anim19,
    sBillboardSet3Anim20, sBillboardSet3Anim21, sBillboardSet3Anim22, sBillboardSet3Anim23, sBillboardSet3Anim24,
    sBillboardSet3Anim25, sBillboardSet3Anim26, sBillboardSet3Anim27, sBillboardSet3Anim28, sBillboardSet3Anim29,
    sBillboardSet3Anim30, sBillboardSet3Anim31, sBillboardSet3Anim32, sBillboardSet3Anim33, sBillboardSet3Anim34,
    sBillboardSet3Anim35, sBillboardSet3Anim36, sBillboardSet3Anim37, sBillboardSet3Anim38, sBillboardSet3Anim39,
    sBillboardSet31Anim0, sBillboardSet31Anim1,
};

// Animation set 32
static const BillboardActorAnimStep sBillboardSet32Anim0[] = {
    { 9, 0, 0, 9 },   { 10, 0, 0, 33 }, { 9, 0, 0, 3 }, { 11, 0, 0, 44 }, { 4, 1, 0, 4 },
    { 12, 0, 0, 40 }, { 4, 1, 0, 20 },  { 3, 0, 0, 3 }, { 2, 0, 0, 2 },   { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet32Anim1[] = {
    { 2, 0, 0, 2 },
    { 7, 0, 0, 3 },
    { 8, 0, 0, 2 },
    { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 2 },
};

static const BillboardActorAnimStep sBillboardSet32Anim2[] = {
    { 8, 0, 0, 2 },
    { 7, 0, 0, 3 },
    { 2, 0, 0, 2 },
    { BILLBOARD_ACTOR_ANIM_CMD_END, 0, 0, 0 },
};

static const BillboardActorAnimStep *const sBillboardSet32Animations[] = {
    sBillboardSet2Anim0,  sBillboardSet2Anim1,  sBillboardSet2Anim2,  sBillboardSet2Anim3,  sBillboardSet2Anim4,
    sBillboardSet2Anim5,  sBillboardSet2Anim6,  sBillboardSet2Anim7,  sBillboardSet2Anim8,  sBillboardSet2Anim9,
    sBillboardSet2Anim10, sBillboardSet2Anim11, sBillboardSet2Anim12, sBillboardSet2Anim13, sBillboardSet2Anim14,
    sBillboardSet2Anim15, sBillboardSet2Anim16, sBillboardSet2Anim17, sBillboardSet2Anim18, sBillboardSet2Anim19,
    sBillboardSet2Anim20, sBillboardSet2Anim21, sBillboardSet2Anim22, sBillboardSet2Anim23, sBillboardSet2Anim24,
    sBillboardSet2Anim25, sBillboardSet2Anim26, sBillboardSet2Anim27, sBillboardSet2Anim28, sBillboardSet2Anim29,
    sBillboardSet2Anim30, sBillboardSet2Anim31, sBillboardSet2Anim32, sBillboardSet2Anim33, sBillboardSet2Anim34,
    sBillboardSet2Anim35, sBillboardSet2Anim36, sBillboardSet2Anim37, sBillboardSet2Anim38, sBillboardSet2Anim39,
    sBillboardSet32Anim0, sBillboardSet32Anim1, sBillboardSet32Anim2,
};

// Animation set 33
static const BillboardActorAnimStep sBillboardSet33Anim0[] = {
    { 0, 0, 0, 7 }, { 1, 0, 0, 7 }, { 2, 0, 0, 7 },
    { 3, 0, 0, 7 }, { 1, 0, 0, 7 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet33Anim1[] = {
    { 4, 0, 0, 7 }, { 5, 0, 0, 7 }, { 6, 0, 0, 7 },
    { 7, 0, 0, 7 }, { 5, 0, 0, 7 }, { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 0 },
};

static const BillboardActorAnimStep sBillboardSet33Anim2[] = {
    { 8, 0, 0, 2 },
    { 9, 0, 0, 2 },
    { 10, 0, 0, 2 },
    { 11, 0, 0, 2 },
    { 10, 0, 0, 2 },
    { 11, 0, 0, 2 },
    { BILLBOARD_ACTOR_ANIM_CMD_GOTO, 0, 0, 4 },
};

static const BillboardActorAnimStep *const sBillboardSet33Animations[] = {
    sBillboardSet33Anim0,
    sBillboardSet33Anim1,
    sBillboardSet33Anim2,
};

// The animation sets, by the set number of an object code's record
const FieldBillboardAnimSet FIELD_BILLBOARD_ANIM_SETS[] = {
    { NULL, 0 },
    { sBillboardSet1Animations, NELEMS(sBillboardSet1Animations) },
    { sBillboardSet2Animations, NELEMS(sBillboardSet2Animations) },
    { sBillboardSet3Animations, NELEMS(sBillboardSet3Animations) },
    { sBillboardSet4Animations, NELEMS(sBillboardSet4Animations) },
    { sBillboardSet5Animations, NELEMS(sBillboardSet5Animations) },
    { sBillboardSet6Animations, NELEMS(sBillboardSet6Animations) },
    { sBillboardSet7Animations, NELEMS(sBillboardSet7Animations) },
    { sBillboardSet8Animations, NELEMS(sBillboardSet8Animations) },
    { sBillboardSet9Animations, NELEMS(sBillboardSet9Animations) },
    { sBillboardSet10Animations, NELEMS(sBillboardSet10Animations) },
    { sBillboardSet11Animations, NELEMS(sBillboardSet11Animations) },
    { sBillboardSet12Animations, NELEMS(sBillboardSet12Animations) },
    { sBillboardSet13Animations, NELEMS(sBillboardSet13Animations) },
    { sBillboardSet14Animations, NELEMS(sBillboardSet14Animations) },
    { sBillboardSet15Animations, NELEMS(sBillboardSet15Animations) },
    { sBillboardSet16Animations, NELEMS(sBillboardSet16Animations) },
    { sBillboardSet17Animations, NELEMS(sBillboardSet17Animations) },
    { sBillboardSet18Animations, NELEMS(sBillboardSet18Animations) },
    { sBillboardSet19Animations, NELEMS(sBillboardSet19Animations) },
    { sBillboardSet20Animations, NELEMS(sBillboardSet20Animations) },
    { sBillboardSet21Animations, NELEMS(sBillboardSet21Animations) },
    { sBillboardSet22Animations, NELEMS(sBillboardSet22Animations) },
    { sBillboardSet23Animations, NELEMS(sBillboardSet23Animations) },
    { sBillboardSet24Animations, NELEMS(sBillboardSet24Animations) },
    { sBillboardSet25Animations, NELEMS(sBillboardSet25Animations) },
    { sBillboardSet26Animations, NELEMS(sBillboardSet26Animations) },
    { sBillboardSet27Animations, NELEMS(sBillboardSet27Animations) },
    { sBillboardSet28Animations, NELEMS(sBillboardSet28Animations) },
    { sBillboardSet29Animations, NELEMS(sBillboardSet29Animations) },
    { sBillboardSet30Animations, NELEMS(sBillboardSet30Animations) },
    { sBillboardSet31Animations, NELEMS(sBillboardSet31Animations) },
    { sBillboardSet32Animations, NELEMS(sBillboardSet32Animations) },
    { sBillboardSet33Animations, NELEMS(sBillboardSet33Animations) },
};
