#ifndef POKEBW2_DEMO_HALL_OF_FAME_DEMO_MAIN_H
#define POKEBW2_DEMO_HALL_OF_FAME_DEMO_MAIN_H

#include "types.h"
#include "demo/hall_of_fame_demo.h"
#include "gfl/bmp.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/gx_layers.h"
#include "gfl/msg.h"
#include "gfl/particle.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "struct_decls.h"
#include "system/bmp_oam.h"
#include "system/gf_font.h"
#include "system/mcss.h"
#include "system/wordset.h"

// The Hall of Fame entry demo's work and its set-up, in hall_of_fame_demo_main.c (a guessed name). Our names

// The members of ARCID_HALL_OF_FAME_DEMO that the demo loads by number
enum {
    NARC_HALL_OF_FAME_DEMO_BG_NCLR = 1,
    NARC_HALL_OF_FAME_DEMO_OBJ_NCLR = 3,
    NARC_HALL_OF_FAME_DEMO_BG_NCGR = 22,
    NARC_HALL_OF_FAME_DEMO_OBJ_1ST_NCGR = 24,
    NARC_HALL_OF_FAME_DEMO_OBJ_2ND_NCGR = 25,
    NARC_HALL_OF_FAME_DEMO_OBJ_1ST_NCER = 26,
    NARC_HALL_OF_FAME_DEMO_OBJ_2ND_NCER = 27,
    NARC_HALL_OF_FAME_DEMO_BG_NSCR = 28,
    NARC_HALL_OF_FAME_DEMO_OBJ_1ST_NANR = 32,
    NARC_HALL_OF_FAME_DEMO_OBJ_2ND_NANR = 33,
    NARC_HALL_OF_FAME_DEMO_NAME_SPA = 51,
};

// The scene that runs, which chooses what a frame draws
enum {
    HALL_OF_FAME_DEMO_SCENE_NONE,
    // The Pokémon one by one, with 3D on both screens for the particles
    HALL_OF_FAME_DEMO_SCENE_1ST,
    // The trainer and the team
    HALL_OF_FAME_DEMO_SCENE_2ND,
};

// The actors
enum {
    // The first scene's: the Pokémon's front and back, the name plates and the flash
    HALL_OF_FAME_DEMO_OBJ_POKE_FRONT,
    HALL_OF_FAME_DEMO_OBJ_POKE_BACK,
    HALL_OF_FAME_DEMO_OBJ_PLATE_TITLE,
    HALL_OF_FAME_DEMO_OBJ_PLATE_NAME,
    HALL_OF_FAME_DEMO_OBJ_FLASH,
    // The second scene's: the trainer on both screens, the plates and the confetti
    HALL_OF_FAME_DEMO_OBJ_TRAINER_MAIN,
    HALL_OF_FAME_DEMO_OBJ_TRAINER_SUB,
    HALL_OF_FAME_DEMO_OBJ_PLATE_UPPER,
    HALL_OF_FAME_DEMO_OBJ_PLATE_LOWER,
    HALL_OF_FAME_DEMO_OBJ_CONFETTI,
    HALL_OF_FAME_DEMO_OBJ_COUNT = HALL_OF_FAME_DEMO_OBJ_CONFETTI + 24,
};

// The actors' resources
enum {
    HALL_OF_FAME_DEMO_CHR_POKE_FRONT,
    HALL_OF_FAME_DEMO_CHR_POKE_BACK,
    HALL_OF_FAME_DEMO_CHR_OBJ,
    HALL_OF_FAME_DEMO_CHR_TRAINER_MAIN,
    HALL_OF_FAME_DEMO_CHR_TRAINER_SUB,
    HALL_OF_FAME_DEMO_CHR_OBJ_SUB,
    HALL_OF_FAME_DEMO_CHR_COUNT,
};

enum {
    HALL_OF_FAME_DEMO_PAL_POKE_FRONT,
    HALL_OF_FAME_DEMO_PAL_POKE_BACK,
    HALL_OF_FAME_DEMO_PAL_OBJ,
    HALL_OF_FAME_DEMO_PAL_FONT,
    HALL_OF_FAME_DEMO_PAL_TRAINER_MAIN,
    HALL_OF_FAME_DEMO_PAL_TRAINER_SUB,
    HALL_OF_FAME_DEMO_PAL_OBJ_SUB,
    HALL_OF_FAME_DEMO_PAL_COUNT,
};

enum {
    HALL_OF_FAME_DEMO_CEL_POKE_FRONT,
    HALL_OF_FAME_DEMO_CEL_POKE_BACK,
    HALL_OF_FAME_DEMO_CEL_OBJ,
    HALL_OF_FAME_DEMO_CEL_TRAINER_MAIN,
    HALL_OF_FAME_DEMO_CEL_TRAINER_SUB,
    HALL_OF_FAME_DEMO_CEL_OBJ_SUB,
    HALL_OF_FAME_DEMO_CEL_COUNT,
};

// The texts drawn on sprites: the first scene's title and Pokémon, the second's title and trainer
enum {
    HALL_OF_FAME_DEMO_FOAM_TITLE,
    HALL_OF_FAME_DEMO_FOAM_NAME,
    HALL_OF_FAME_DEMO_FOAM_TITLE_2ND,
    HALL_OF_FAME_DEMO_FOAM_PLAYER,
    HALL_OF_FAME_DEMO_FOAM_COUNT,
};

// The particle systems: the Pokémon's type's, and the name plates'
enum {
    HALL_OF_FAME_DEMO_PTC_TYPE,
    HALL_OF_FAME_DEMO_PTC_NAME,
    HALL_OF_FAME_DEMO_PTC_COUNT,
};

#define HALL_OF_FAME_DEMO_PTC_WORK_SIZE 0x4800
#define HALL_OF_FAME_DEMO_POKE_MAX 6
// The steps of a shake, after which the actor is back in place
#define HALL_OF_FAME_DEMO_SHAKE_COUNT 8

typedef struct {
    BmpOamActor *oam;
    GFLBitmap *bmp;
} HallOfFameDemoFontOam;

typedef struct {
    HallOfFameDemoParam *param;
    TCB *vblankTask;
    // HALL_OF_FAME_DEMO_SCENE_*
    u32 scene;
    u32 seHandle;
    G3DManager *g3dMgr;
    u32 unk14;
    MCSSSystem *mcssSys;
    MCSS *mcss;
    G3DCamera *camera;
    // Set once the Pokémon's animation has played to its end
    BOOL mcssAnmEnd;
    ParticleSystem *ptc[HALL_OF_FAME_DEMO_PTC_COUNT];
    u8 ptcWork[HALL_OF_FAME_DEMO_PTC_COUNT][HALL_OF_FAME_DEMO_PTC_WORK_SIZE];
    Font *font;
    Font *fontSmall;
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *strbuf;
    ClActUnit *clUnit;
    ClActor *clact[HALL_OF_FAME_DEMO_OBJ_COUNT];
    u32 chrRes[HALL_OF_FAME_DEMO_CHR_COUNT];
    u32 palRes[HALL_OF_FAME_DEMO_PAL_COUNT];
    u32 celRes[HALL_OF_FAME_DEMO_CEL_COUNT];
    ClActRenderer *renderer;
    BmpOamSys *bmpOam;
    HallOfFameDemoFontOam fontOam[HALL_OF_FAME_DEMO_FOAM_COUNT];
    // The team's Pokémon that are not Eggs, and the one being shown
    PartyPkm *mons[HALL_OF_FAME_DEMO_POKE_MAX];
    u16 species;
    u16 type;
    u8 form;
    u8 monIndex;
    u8 monCount;
    // The shake of an actor around its place
    u8 shakeIndex;
    s16 shakeBaseX;
    s16 shakeBaseY;
    s16 shakeX[HALL_OF_FAME_DEMO_SHAKE_COUNT];
    s16 shakeY[HALL_OF_FAME_DEMO_SHAKE_COUNT];
    u32 voice;
    // Counts frames, in the steps and in the wait sequence
    int count;
    int mainSeq;
    int subSeq;
    int nextSeq;
    BOOL end;
} HallOfFameDemoWork;

void HallOfFameDemoMain_InitVBlank1st(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_InitVBlank2nd(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_ExitVBlank(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_InitVram(u32 scene);
const BGSysVRAMConfig *HallOfFameDemoMain_GetVramConfig(u32 scene);
void HallOfFameDemoMain_InitBg(void);
void HallOfFameDemoMain_ExitBg(void);
void HallOfFameDemoMain_InitBgMode(void);
void HallOfFameDemoMain_InitBg2nd(void);
void HallOfFameDemoMain_ExitBg2nd(void);
void HallOfFameDemoMain_LoadBg(void);
void HallOfFameDemoMain_SetBlendAlpha(void);
void HallOfFameDemoMain_InitMsg(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_ExitMsg(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_InitSound(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_ExitSound(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_GetPokeMax(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_GetPokeData(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_LoadPokeVoice(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_Init3D(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_Exit3D(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_Main3D(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_MainDouble3D(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_InitDouble3D(void);
void HallOfFameDemoMain_ExitDouble3D(void);
void HallOfFameDemoMain_InitParticle(void);
void HallOfFameDemoMain_ExitParticle(void);
void HallOfFameDemoMain_CreateTypeParticle(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_DeleteTypeParticle(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_SetTypeParticle(HallOfFameDemoWork *wk, u32 id);
void HallOfFameDemoMain_CreateNameParticle(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_DeleteNameParticle(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_SetNameParticle(HallOfFameDemoWork *wk, u32 id);
void HallOfFameDemoMain_InitMcss(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_ExitMcss(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_AddMcss(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_DelMcss(HallOfFameDemoWork *wk);
void HallOfFameDemoMain_MoveMcss(HallOfFameDemoWork *wk, s32 move);
void HallOfFameDemoMain_SetMcssCallBack(HallOfFameDemoWork *wk);

#endif // POKEBW2_DEMO_HALL_OF_FAME_DEMO_MAIN_H
