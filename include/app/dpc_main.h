#ifndef POKEBW2_APP_DPC_MAIN_H
#define POKEBW2_APP_DPC_MAIN_H

#include "types.h"
#include "app/dendou_pc.h"
#include "gfl/bmp.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "nitro/rtc.h"
#include "save/hall_of_fame.h"
#include "struct_decls.h"
#include "system/bmp_oam.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The Hall of Fame PC's work and its set-up, in dpc_main.c (a guessed name). Our names

// The actors: two sets of a team's Pokémon, of which one is shown while the next page's is loaded, and the buttons
enum {
    DPCOBJ_POKE_A,
    DPCOBJ_POKE_B = DPCOBJ_POKE_A + HALL_OF_FAME_TEAM_SIZE,
    DPCOBJ_ARROW_LEFT = DPCOBJ_POKE_B + HALL_OF_FAME_TEAM_SIZE,
    DPCOBJ_ARROW_RIGHT,
    DPCOBJ_CLOSE,
    DPCOBJ_RETURN,
    DPCOBJ_MAX,
};

// The actors' resources: one per Pokémon of both sets, and the buttons'
#define DPCOBJ_RES_BUTTON (HALL_OF_FAME_TEAM_SIZE * 2)
#define DPCOBJ_RES_MAX (DPCOBJ_RES_BUTTON + 1)

// The windows: the page's title on the touch screen and the chosen Pokémon's data on the top screen
enum {
    DPCBMP_WIN_TITLE,
    DPCBMP_WIN_INFO,
    DPCBMP_WIN_MAX,
};

// A team as the screen shows it
typedef struct {
    HallOfFamePokemon pokemon[HALL_OF_FAME_TEAM_SIZE];
    RTCDate date;
    u16 count;
    // Which entry into the Hall of Fame the team was
    u16 number;
} DendouPCTeam;

typedef struct {
    DendouPCParam *param;
    u32 unk4[2];
    DendouPCTeam teams[HALL_OF_FAME_TEAMS];
    // Where each Pokémon is on the ring, in degrees, and where the turn takes it
    s16 angle[HALL_OF_FAME_TEAM_SIZE];
    s16 angleNext[HALL_OF_FAME_TEAM_SIZE];
    TCB *vblankTask;
    u32 unkB2C;
    PaletteFade *palFade;
    // How dark each Pokémon is by its place on the ring, now, and after the turn
    u8 fade[HALL_OF_FAME_TEAM_SIZE];
    u8 fadeNow[HALL_OF_FAME_TEAM_SIZE];
    u8 fadeNext[HALL_OF_FAME_TEAM_SIZE];
    PrintWindow win[DPCBMP_WIN_MAX];
    Font *font;
    Font *smallFont;
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *expandBuf;
    PrintQueue *printQueue;
    ClActUnit *unit;
    ClActor *actors[DPCOBJ_MAX];
    u32 chars[DPCOBJ_RES_MAX];
    u32 palettes[DPCOBJ_RES_MAX];
    u32 cellAnims[DPCOBJ_RES_MAX];
    // The page number, a bitmap shown as OBJs
    BmpOamSys *bmpOam;
    BmpOamActor *pageActor;
    GFLBitmap *pageBitmap;
    // The Pokémon at the front of the ring, and the one chosen to come there
    s8 pokePos;
    s8 pokeNext;
    s8 page;
    u8 pageMax;
    // Which set of actors the next page loads
    u8 objSwap;
    // The turn of the ring: degrees per frame and the frames left
    s8 speed;
    u8 stepCount;
    // The button whose animation DPCSEQ_BUTTON_ANIME waits for, and the sequence after it
    u32 animeObj;
    u32 animeNext;
    u32 seq;
    u32 subSeq;
    // The sequence after the wipe
    u32 wipeNext;
} DendouPCWork;

void DpcMain_InitVBlank(DendouPCWork *wk);
void DpcMain_ExitVBlank(DendouPCWork *wk);
void DpcMain_InitVram(void);
const BGSysVRAMConfig *DpcMain_GetVramBanks(void);
void DpcMain_InitBg(void);
void DpcMain_ExitBg(void);
void DpcMain_LoadBgGraphic(void);
void DpcMain_InitPaletteFade(DendouPCWork *wk);
void DpcMain_ExitPaletteFade(DendouPCWork *wk);
// Darkens each Pokémon's palette by its fadeNow, and whether that is still being sent
void DpcMain_BlendPokePalettes(DendouPCWork *wk);
BOOL DpcMain_IsPaletteFading(DendouPCWork *wk);
void DpcMain_SetBlend(void);
void DpcMain_InitMsg(DendouPCWork *wk);
void DpcMain_ExitMsg(DendouPCWork *wk);
// Loads the teams of the screen's mode from the save
void DpcMain_LoadTeams(DendouPCWork *wk);
void DpcMain_FreeTeams(DendouPCWork *wk);

#endif // POKEBW2_APP_DPC_MAIN_H
