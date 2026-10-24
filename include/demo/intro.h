#ifndef POKEBW2_DEMO_INTRO_H
#define POKEBW2_DEMO_INTRO_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/str.h"
#include "save/config.h"
#include "save/save_control_intr.h"
#include "struct_decls.h"
#include "system/mcss.h"

// The intro of a new game, overlay 294, which ov162 runs before and after the name entry
#define OVERLAY_INTRO OVERLAY_ID(294)

// Which part of the intro runs, as the number of the script it starts from: ov162 runs the intro, then the player's
// name entry, the intro again, the rival's name entry, and the intro once more
enum {
    INTRO_MODE_START = 1,
    INTRO_MODE_PLAYER_NAMED = 7,
    INTRO_MODE_RIVAL_NAMED = 10,
};

// What ov162 does once the intro ends: runs the name entry that the intro asked for, the rival's name entry, or starts
// the game
enum {
    INTRO_RESULT_DONE,
    INTRO_RESULT_ENTER_NAME,
    INTRO_RESULT_ENTER_RIVAL_NAME,
};

typedef struct {
    PlayerInfo *playerInfo;
    Config *config;
    // INTRO_MODE_*
    u32 mode;
    // ov162's creation of the save data, which runs while the intro does
    SaveControlIntr *saveTask;
    // The handle of the Pokémon's cry
    u32 pokeVoice;
    const u16 *rivalName;
    // INTRO_RESULT_*
    u32 result;
} IntroParam;

typedef struct IntroGraphic IntroGraphic;
typedef struct IntroCmd IntroCmd;
typedef struct IntroMcss IntroMcss;
typedef struct IntroG3d IntroG3d;
typedef struct IntroParticle IntroParticle;
typedef struct IntroMsg IntroMsg;

extern const GameProcFunctions INTRO_PROC_FUNCTIONS;
// The sound sequences that the intro plays, which ov162 loads before it
extern const u32 INTRO_SOUND_COUNT;
extern const u32 INTRO_SOUNDS[];

// intro_graphic.c
IntroGraphic *IntroGraphic_Create(u32 layout, u32 mode, HeapID heapId);
void IntroGraphic_Free(IntroGraphic *graphic);
void IntroGraphic_Update(IntroGraphic *graphic);
void IntroGraphic_Begin3D(IntroGraphic *graphic);
void IntroGraphic_End3D(IntroGraphic *graphic);
ClActUnit *IntroGraphic_GetClActUnit(IntroGraphic *graphic);

// intro_cmd.c
IntroCmd *IntroCmd_Create(IntroG3d *g3d, IntroParticle *particle, IntroMcss *mcss, IntroParam *param,
                          IntroGraphic *graphic, HeapID heapId);
void IntroCmd_Free(IntroCmd *cmd);
// Runs the script, and returns FALSE once it has ended
BOOL IntroCmd_Update(IntroCmd *cmd);

// intro_msg.c
typedef struct {
    u32 message;
    s32 value;
} IntroMenuItem;

// IntroMsg_GetMenuResult's results
enum {
    INTRO_MENU_NONE,
    INTRO_MENU_CHOSEN,
    INTRO_MENU_CANCELLED,
};

IntroMsg *IntroMsg_Create(HeapID heapId);
void IntroMsg_Free(IntroMsg *msg);
void IntroMsg_LoadMessages(IntroMsg *msg, BOOL preload, u16 fileId);
void IntroMsg_Update(IntroMsg *msg);
void IntroMsg_Print(IntroMsg *msg, u32 messageId, BOOL frame);
void IntroMsg_Clear(IntroMsg *msg);
u32 IntroMsg_GetPrintState(IntroMsg *msg);
BOOL IntroMsg_UpdatePrint(IntroMsg *msg);
void IntroMsg_OpenMenu(IntroMsg *msg, const IntroMenuItem *items, u32 count, BOOL a3);
void IntroMsg_CloseMenu(IntroMsg *msg);
void IntroMsg_UpdateMenu(IntroMsg *msg);
u32 IntroMsg_GetMenuResult(IntroMsg *msg, s32 *value);
WordSet *IntroMsg_GetWordSet(IntroMsg *msg);
void IntroMsg_ShowWaitIcon(IntroMsg *msg);
void IntroMsg_HideWaitIcon(IntroMsg *msg);

// intro_mcss.c
IntroMcss *IntroMcss_Create(HeapID heapId, u32 mode);
void IntroMcss_Free(IntroMcss *mcss);
void IntroMcss_Draw(IntroMcss *mcss);
void IntroMcss_Add(IntroMcss *mcss, fx32 x, fx32 y, fx32 z, const MCSSLoadInfo *info, u8 index);
void IntroMcss_AddPokemon(IntroMcss *mcss, fx32 x, fx32 y, fx32 z, u32 species, u8 index);
void IntroMcss_SetVisible(IntroMcss *mcss, BOOL visible, u8 index);
void IntroMcss_SetAnimation(IntroMcss *mcss, u8 index, u32 animation, BOOL restartAtEnd);
BOOL IntroMcss_IsAnimationEnded(IntroMcss *mcss, u8 index);
void IntroMcss_ClearAnimationEnded(IntroMcss *mcss, u8 index);
void IntroMcss_SetAlpha(IntroMcss *mcss, u8 index, u32 alpha);
void IntroMcss_SetFlipV(IntroMcss *mcss, u8 index, BOOL a2);
BOOL IntroMcss_MoveX(IntroMcss *mcss, u8 index, fx32 step, fx32 target);
BOOL IntroMcss_DecreaseY(IntroMcss *mcss, fx32 step, fx32 min);
void IntroMcss_Update(IntroMcss *mcss);

// intro_g3d.c
IntroG3d *IntroG3d_Create(IntroGraphic *graphic, u32 mode, HeapID heapId);
void IntroG3d_Free(IntroG3d *g3d);
void IntroG3d_Draw(IntroG3d *g3d);
BOOL IntroG3d_Open(IntroG3d *g3d);
void IntroG3d_SetVisible(IntroG3d *g3d, BOOL visible);
void IntroG3d_SetMode(IntroG3d *g3d, u32 mode);
BOOL IntroG3d_Animate(IntroG3d *g3d);
BOOL IntroG3d_AnimateBack(IntroG3d *g3d);
void IntroG3d_SetFrame(IntroG3d *g3d, u32 frame);

// intro_particle.c
IntroParticle *IntroParticle_Create(IntroGraphic *graphic, HeapID heapId);
void IntroParticle_Free(IntroParticle *particle);
void IntroParticle_Update(IntroParticle *particle);
void IntroParticle_SetPos(IntroParticle *particle, fx32 x, fx32 y, fx32 z);

#endif // POKEBW2_DEMO_INTRO_H
