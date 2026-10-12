#ifndef POKEBW2_DEMO_DDEMO_MAIN_H
#define POKEBW2_DEMO_DDEMO_MAIN_H

#include "types.h"
#include "demo/dendou_demo.h"
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

// The Hall of Fame entry demo's work and its set-up, in ddemo_main.c (a guessed name). Our names

// The members of ARCID_DENDOU_DEMO that the demo loads by number
enum {
    NARC_DDEMO_BG_NCLR = 1,
    NARC_DDEMO_OBJ_NCLR = 3,
    NARC_DDEMO_BG_NCGR = 22,
    NARC_DDEMO_OBJ_1ST_NCGR = 24,
    NARC_DDEMO_OBJ_2ND_NCGR = 25,
    NARC_DDEMO_OBJ_1ST_NCER = 26,
    NARC_DDEMO_OBJ_2ND_NCER = 27,
    NARC_DDEMO_BG_NSCR = 28,
    NARC_DDEMO_OBJ_1ST_NANR = 32,
    NARC_DDEMO_OBJ_2ND_NANR = 33,
    NARC_DDEMO_NAME_SPA = 51,
};

// The scene that runs, which chooses what a frame draws
enum {
    DDEMO_SCENE_NONE,
    // The Pokémon one by one, with 3D on both screens for the particles
    DDEMO_SCENE_1ST,
    // The trainer and the team
    DDEMO_SCENE_2ND,
};

// The actors
enum {
    // The first scene's: the Pokémon's front and back, the name plates and the flash
    DDEMOOBJ_POKE_FRONT,
    DDEMOOBJ_POKE_BACK,
    DDEMOOBJ_PLATE_TITLE,
    DDEMOOBJ_PLATE_NAME,
    DDEMOOBJ_FLASH,
    // The second scene's: the trainer on both screens, the plates and the confetti
    DDEMOOBJ_TRAINER_MAIN,
    DDEMOOBJ_TRAINER_SUB,
    DDEMOOBJ_PLATE_UPPER,
    DDEMOOBJ_PLATE_LOWER,
    DDEMOOBJ_CONFETTI,
    DDEMOOBJ_COUNT = DDEMOOBJ_CONFETTI + 24,
};

// The actors' resources
enum {
    DDEMO_CHR_POKE_FRONT,
    DDEMO_CHR_POKE_BACK,
    DDEMO_CHR_OBJ,
    DDEMO_CHR_TRAINER_MAIN,
    DDEMO_CHR_TRAINER_SUB,
    DDEMO_CHR_OBJ_SUB,
    DDEMO_CHR_COUNT,
};

enum {
    DDEMO_PAL_POKE_FRONT,
    DDEMO_PAL_POKE_BACK,
    DDEMO_PAL_OBJ,
    DDEMO_PAL_FONT,
    DDEMO_PAL_TRAINER_MAIN,
    DDEMO_PAL_TRAINER_SUB,
    DDEMO_PAL_OBJ_SUB,
    DDEMO_PAL_COUNT,
};

enum {
    DDEMO_CEL_POKE_FRONT,
    DDEMO_CEL_POKE_BACK,
    DDEMO_CEL_OBJ,
    DDEMO_CEL_TRAINER_MAIN,
    DDEMO_CEL_TRAINER_SUB,
    DDEMO_CEL_OBJ_SUB,
    DDEMO_CEL_COUNT,
};

// The texts drawn on sprites: the first scene's title and Pokémon, the second's title and trainer
enum {
    DDEMO_FOAM_TITLE,
    DDEMO_FOAM_NAME,
    DDEMO_FOAM_TITLE_2ND,
    DDEMO_FOAM_PLAYER,
    DDEMO_FOAM_COUNT,
};

// The particle systems: the Pokémon's type's, and the name plates'
enum {
    DDEMO_PTC_TYPE,
    DDEMO_PTC_NAME,
    DDEMO_PTC_COUNT,
};

#define DDEMO_PTC_WORK_SIZE 0x4800
#define DDEMO_POKE_MAX 6
// The steps of a shake, after which the actor is back in place
#define DDEMO_SHAKE_COUNT 8

typedef struct {
    BmpOamActor *oam;
    GFLBitmap *bmp;
} DDemoFontOam;

typedef struct {
    DendouDemoParam *param;
    TCB *vblankTask;
    // DDEMO_SCENE_*
    u32 scene;
    u32 seHandle;
    G3DManager *g3dMgr;
    u32 unk14;
    MCSSSystem *mcssSys;
    MCSS *mcss;
    G3DCamera *camera;
    // Set once the Pokémon's animation has played to its end
    BOOL mcssAnmEnd;
    ParticleSystem *ptc[DDEMO_PTC_COUNT];
    u8 ptcWork[DDEMO_PTC_COUNT][DDEMO_PTC_WORK_SIZE];
    Font *font;
    Font *fontSmall;
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *strbuf;
    ClActUnit *clUnit;
    ClActor *clact[DDEMOOBJ_COUNT];
    u32 chrRes[DDEMO_CHR_COUNT];
    u32 palRes[DDEMO_PAL_COUNT];
    u32 celRes[DDEMO_CEL_COUNT];
    ClActRenderer *renderer;
    BmpOamSys *bmpOam;
    DDemoFontOam fontOam[DDEMO_FOAM_COUNT];
    // The team's Pokémon that are not Eggs, and the one being shown
    PartyPkm *mons[DDEMO_POKE_MAX];
    u16 species;
    u16 type;
    u8 form;
    u8 monIndex;
    u8 monCount;
    // The shake of an actor around its place
    u8 shakeIndex;
    s16 shakeBaseX;
    s16 shakeBaseY;
    s16 shakeX[DDEMO_SHAKE_COUNT];
    s16 shakeY[DDEMO_SHAKE_COUNT];
    u32 voice;
    // Counts frames, in the steps and in the wait sequence
    int count;
    int mainSeq;
    int subSeq;
    int nextSeq;
    BOOL end;
} DDemoWork;

void DDemoMain_InitVBlank1st(DDemoWork *wk);
void DDemoMain_InitVBlank2nd(DDemoWork *wk);
void DDemoMain_ExitVBlank(DDemoWork *wk);
void DDemoMain_InitVram(u32 scene);
const BGSysVRAMConfig *DDemoMain_GetVramConfig(u32 scene);
void DDemoMain_InitBg(void);
void DDemoMain_ExitBg(void);
void DDemoMain_InitBgMode(void);
void DDemoMain_InitBg2nd(void);
void DDemoMain_ExitBg2nd(void);
void DDemoMain_LoadBg(void);
void DDemoMain_SetBlendAlpha(void);
void DDemoMain_InitMsg(DDemoWork *wk);
void DDemoMain_ExitMsg(DDemoWork *wk);
void DDemoMain_InitSound(DDemoWork *wk);
void DDemoMain_ExitSound(DDemoWork *wk);
void DDemoMain_GetPokeMax(DDemoWork *wk);
void DDemoMain_GetPokeData(DDemoWork *wk);
void DDemoMain_LoadPokeVoice(DDemoWork *wk);
void DDemoMain_Init3D(DDemoWork *wk);
void DDemoMain_Exit3D(DDemoWork *wk);
void DDemoMain_Main3D(DDemoWork *wk);
void DDemoMain_MainDouble3D(DDemoWork *wk);
void DDemoMain_InitDouble3D(void);
void DDemoMain_ExitDouble3D(void);
void DDemoMain_InitParticle(void);
void DDemoMain_ExitParticle(void);
void DDemoMain_CreateTypeParticle(DDemoWork *wk);
void DDemoMain_DeleteTypeParticle(DDemoWork *wk);
void DDemoMain_SetTypeParticle(DDemoWork *wk, u32 id);
void DDemoMain_CreateNameParticle(DDemoWork *wk);
void DDemoMain_DeleteNameParticle(DDemoWork *wk);
void DDemoMain_SetNameParticle(DDemoWork *wk, u32 id);
void DDemoMain_InitMcss(DDemoWork *wk);
void DDemoMain_ExitMcss(DDemoWork *wk);
void DDemoMain_AddMcss(DDemoWork *wk);
void DDemoMain_DelMcss(DDemoWork *wk);
void DDemoMain_MoveMcss(DDemoWork *wk, s32 move);
void DDemoMain_SetMcssCallBack(DDemoWork *wk);

#endif // POKEBW2_DEMO_DDEMO_MAIN_H
