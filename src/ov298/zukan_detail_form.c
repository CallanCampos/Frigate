#include "types.h"
#include "app/zukan_detail.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nnsys/g2d.h"
#include "pml/personal.h"
#include "pml/poke_graphic.h"
#include "pml/poke_party.h"
#include "pml/species_names.h"
#include "save/pokedex.h"
#include "system/game_data.h"
#include "system/mcss.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/btl_server_flow_title.h"

// The detail screen's forms page, zukan_detail_form.c by the ROM's own string: the Pokémon's sprite on the top screen
// in each sex, color and form that the Pokédex has seen, one at a time or two side by side to compare them, with its
// icon, the names of the forms shown and a slider to pick the second one on the touch screen. The one in front is the
// one that the Pokédex shows of the species

// The Pokédex's shortcut for the Y button, which the touch bar's check box registers
#define SHORTCUT_POKEDEX_FORM 23

#define ARCID_POKEICON 7
// For each species and form, the message of its next form's name in system message file 450
#define ARCID_FORM_NAME_TABLE 97

enum {
    FORM_SEQ_INIT,
    FORM_SEQ_FADE_IN,
    FORM_SEQ_WAIT_BLEND,
    FORM_SEQ_START_PALFADE,
    FORM_SEQ_WAIT_FADE_IN,
    FORM_SEQ_MAIN,
    FORM_SEQ_FADE_OUT_POKEMON,
    FORM_SEQ_WAIT_FADE_OUT,
    FORM_SEQ_FADE_OUT,
    FORM_SEQ_EXIT,
};

// How the page ends
enum {
    FORM_EXIT_NONE,
    FORM_EXIT_PAGE,
    FORM_EXIT_SCREEN,
};

// The page shows one entry, or the one in front and a second one side by side. The arrows slide to the next or
// previous entry, and A or a touch of the top screen opens the comparison, where the form button brings the second
// one to the front
enum {
    FORM_MODE_SINGLE,
    FORM_MODE_SLIDE,
    FORM_MODE_OPEN,
    FORM_MODE_OPENED,
    FORM_MODE_COMPARE,
    FORM_MODE_SELECT,
    FORM_MODE_CLOSE,
    FORM_MODE_CLOSED,
};

// The sprites: the front and back of the entry in front, and of the second one
enum {
    SPRITE_CUR_FRONT,
    SPRITE_CUR_BACK,
    SPRITE_NEXT_FRONT,
    SPRITE_NEXT_BACK,
    SPRITE_COUNT,
    SPRITE_NONE = SPRITE_COUNT,
};

// Where a sprite stands: alone in the middle, off the screen to the left or right while it slides in, or on the left
// or right side of the comparison
enum {
    POS_CENTER,
    POS_LEFT_OUT,
    POS_RIGHT_OUT,
    POS_LEFT,
    POS_RIGHT,
    POS_COUNT,
};

// The places of ZukanDetailForm_GetPlacePos
enum {
    PLACE_SINGLE,
    PLACE_LEFT,
    PLACE_RIGHT,
};

// The sprite's animation plays twice when it is started
#define ANIM_LOOPS 2

// The name of an entry: the species', the sex's or the form's
enum {
    ENTRY_NAME_SPECIES,
    ENTRY_NAME_SEX,
    ENTRY_NAME_FORM,
};

enum {
    ENTRY_COLOR_NONE,
    ENTRY_COLOR_NORMAL,
    ENTRY_COLOR_RARE,
};

#define ENTRY_MAX 64

// The text: the species' name, the counts of forms and of shiny colors, their labels, and the names of the two
// entries shown
enum {
    WINDOW_NAME,
    WINDOW_COUNTS,
    WINDOW_LABELS,
    WINDOW_CUR,
    WINDOW_NEXT,
    WINDOW_COUNT,
};

enum {
    STRBUF_FORM_COUNT,
    STRBUF_RARE_COUNT,
    STRBUF_SPECIES,
    STRBUF_MALE,
    STRBUF_FEMALE,
    STRBUF_RARE,
    STRBUF_COUNT,
};

// The slider's bar and knob, and the color marks of the two entries shown
enum {
    ACTOR_SLIDER_BAR,
    ACTOR_SLIDER_KNOB,
    ACTOR_COLOR_CUR,
    ACTOR_COLOR_NEXT,
    ACTOR_COUNT,
};

// The buttons on the touch screen: turn the Pokémon around, play its animation, and the arrows to the previous and
// next entry. ZukanDetailForm_GetButtonInput also returns BUTTON_NONE, or BUTTON_BUSY while one plays its animation
typedef enum {
    BUTTON_TURN,
    BUTTON_PLAY,
    BUTTON_ARROW_L,
    BUTTON_ARROW_R,
    BUTTON_COUNT,
    BUTTON_NONE = 5,
    BUTTON_BUSY,
} FormButton;

// Whether a color mark is shown, a type of its own, so that MWCC keeps the first one's constant in a register
typedef enum {
    MARK_HIDDEN,
    MARK_SHOWN,
} MarkState;

enum {
    BUTTON_STATE_NONE,
    BUTTON_STATE_PUSHED,
    BUTTON_STATE_ANIMATING,
    BUTTON_STATE_DONE,
};

// The slider spans 65 pixels from x 156, with its knob at y 48
#define SLIDER_X 156
#define SLIDER_WIDTH 65
#define SLIDER_KNOB_Y 48

// The time that the form button's move takes, half a turn of the sine
#define SELECT_TIME 0x8000
#define SELECT_STEP 0x400

typedef struct ZukanDetailFormWork ZukanDetailFormWork;

typedef struct {
    u32 species;
    u32 form;
    // 3 for any sex
    u32 sex;
    u32 rare;
    u32 a6;
    BOOL back;
    u32 personality;
    ZukanDetailFormPos pos[POS_COUNT];
} SpritePositions;

// The data of a sprite's animation callback, which stops it after ANIM_LOOPS loops
typedef struct {
    int sprite;
    ZukanDetailFormWork *wk;
    u8 loops;
    BOOL stop;
} SpriteAnim;

typedef struct {
    MCSS *mcss;
    SpriteAnim *anim;
    ZukanDetailFormPos pos[POS_COUNT];
} Sprite;

typedef struct {
    // ENTRY_NAME_*
    int name;
    // ENTRY_COLOR_*
    int color;
    // The sex's message, or the form
    u16 value;
    u8 sex;
    u8 rare;
    u16 form;
} FormEntry;

typedef struct {
    s16 x;
    s16 y;
    // ZUKAN_DETAIL_FORM_RES_*
    u32 chars;
    u32 palette;
    u32 cellAnims;
    // The area that is touched
    u8 rectX;
    u8 rectY;
    u8 rectWidth;
    u8 rectHeight;
    u8 activeAnim;
    u8 pushedAnim;
    u32 key;
    u32 se;
    int state;
    ClActor *actor;
} Button;

struct ZukanDetailFormWork {
    ClActUnit *unit;
    Font *font;
    MCSSSystem *mcssSys;
    Sprite sprites[SPRITE_COUNT];
    void *tcbBuffer;
    TCBManager *tcbMgr;
    // Whether the front sprites are shown, or the back ones
    BOOL front;
    // The icon of the entry in front, two to switch between
    ArcTool *iconArc;
    u32 iconChars[2];
    u32 iconPalette;
    u32 iconCellAnims;
    ClActor *iconActors[2];
    int icon;
    // Every sex, color and form of the species that the Pokédex has seen, the count of their names and of shiny
    // ones, the entry in front and the second one
    FormEntry entries[ENTRY_MAX];
    u16 count;
    u16 nameCount;
    u16 rareCount;
    u16 cur;
    u16 next;
    BmpWin *windows[WINDOW_COUNT];
    BOOL flushPending[WINDOW_COUNT];
    PrintQueue *printQueues[WINDOW_COUNT];
    MsgData *msgData;
    u16 *formNameTable;
    MsgData *formMsgData;
    StrBuf *strbufs[STRBUF_COUNT];
    u32 bgChars;
    ZukanDetailBackground *backgroundMain;
    ZukanDetailBackground *backgroundSub;
    u32 resources[ZUKAN_DETAIL_FORM_RES_COUNT];
    ClActor *actors[ACTOR_COUNT];
    Button buttons[BUTTON_COUNT];
    // The BUTTON_* playing its animation, or BUTTON_NONE
    int pushed;
    // Whether the slider's knob jumps to the second entry's place, and whether it is held
    BOOL sliderSnap;
    BOOL dragging;
    TCB *vblankTcb;
    ZukanDetailBlend *blendMain;
    ZukanDetailBlend *blendSub;
    ZukanDetailPalFade *palFade;
    int exit;
    BOOL inputEnabled;
    int mode;
    // The form button's move of the second entry to the front
    BOOL selectMoving;
    int selectTime;
    // The slide to the next entry: 1 while it slides, 2 once it is in place
    int slide;
    BOOL slideFromRight;
    // Whether the entry in front and the second one have reached their places as the comparison opens or closes
    BOOL curInPlace;
    BOOL nextInPlace;
    // Whether the slider is checked after the second entry has changed, outside the comparison's own input
    BOOL sliderActive;
    // The touch bar is unlocked once both the form button's animation and the move have ended
    BOOL selectPending;
    BOOL selectMoved;
    BOOL selectButtonDone;
};

static BOOL ZukanDetailForm_Init(ZukanDetailProcSys *sys, int *seq, void *param_, void *work,
                                 ZukanDetailCommon *common);
static BOOL ZukanDetailForm_Exit(ZukanDetailProcSys *sys, int *seq, void *param, void *work, ZukanDetailCommon *common);
static BOOL ZukanDetailForm_Main(ZukanDetailProcSys *sys, int *seq, void *param, void *work, ZukanDetailCommon *common);
static void ZukanDetailForm_Command(ZukanDetailProcSys *sys, int *seq, void *param, void *work,
                                    ZukanDetailCommon *common, int command);
static void ZukanDetailForm_Draw(ZukanDetailProcSys *sys, int *seq, void *param, void *work, ZukanDetailCommon *common);
static void ZukanDetailForm_VBlank(TCB *tcb, void *data);
static void ZukanDetailForm_GetEntryText(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                         ZukanDetailCommon *common, StrBuf **name, StrBuf **color, BOOL *freeName,
                                         BOOL *freeColor, u16 index);
static void ZukanDetailForm_CreateText(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common);
static void ZukanDetailForm_FreeText(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common);
static void ZukanDetailForm_CreatePrintQueues(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                              ZukanDetailCommon *common);
static void ZukanDetailForm_FreePrintQueues(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                            ZukanDetailCommon *common);
static void ZukanDetailForm_UpdateText(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common);
static void ZukanDetailForm_FlushWindow(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common,
                                        u8 window);
static void ZukanDetailForm_PrintName(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common);
static void ZukanDetailForm_PrintEntry(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common,
                                       u8 window, PrintQueue *printQueue, BmpWin *bmpWin, BOOL *flushPending,
                                       u16 index);
static void ZukanDetailForm_PrintCur(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common);
static void ZukanDetailForm_PrintNext(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common);
static void ZukanDetailForm_ScrollNextBG(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                         ZukanDetailCommon *common);
static void ZukanDetailForm_CreateMCSS(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common);
static void ZukanDetailForm_FreeMCSS(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common);
static MCSS *ZukanDetailForm_AddMCSS(MCSSSystem *system, u32 species, u32 form, u32 sex, u32 rare, u32 a6, u32 back,
                                     u32 personality);
static void ZukanDetailForm_RemoveMCSS(MCSSSystem *system, MCSS *mcss);
static void ZukanDetailForm_SetShadow(MCSS *mcss);
static void ZukanDetailForm_GetBesidePos(MCSS *mcss, VecFx32 *pos);
static void ZukanDetailForm_CreateSprite(Sprite *sprite, HeapID heapId, MCSSSystem *system, u32 species, u32 form,
                                         u32 sex, u32 rare, u32 a6, u32 back, u32 personality);
static void ZukanDetailForm_FreeSprite(Sprite *sprite, MCSSSystem *system);
static void ZukanDetailForm_InitSpriteAnim(SpriteAnim *anim, int sprite, ZukanDetailFormWork *wk);
static void ZukanDetailForm_PlaySpriteAnim(Sprite *sprite);
static void ZukanDetailForm_StopSpriteAnim(Sprite *sprite);
static void ZukanDetailForm_UpdateSprite(Sprite *sprite);
static void ZukanDetailForm_SpriteAnimEnd(u32 param, fx32 frame);
static void ZukanDetailForm_Turn(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common);
static void ZukanDetailForm_LoadIconResources(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                              ZukanDetailCommon *common);
static void ZukanDetailForm_FreeIconResources(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                              ZukanDetailCommon *common);
static ClActor *ZukanDetailForm_CreateIcon(ArcTool *arc, u32 *chars, u32 palette, u32 cellAnims, ClActUnit *unit,
                                           HeapID heapId, u32 species, u32 form, u32 sex, BOOL egg);
static void ZukanDetailForm_FreeIcon(u32 chars, ClActor *actor);
static void ZukanDetailForm_GatherEntries(ZukanDetailFormParam *param, ZukanDetailCommon *common, FormEntry *entries,
                                          u16 *count, u16 *nameCount, u16 *rareCount, u16 *cur, u16 *next);
static void ZukanDetailForm_LoadEntries(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                        ZukanDetailCommon *common);
static void ZukanDetailForm_CreateSprites(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                          ZukanDetailCommon *common, int frontSprite, int backSprite, int place,
                                          u16 index);
static void ZukanDetailForm_CreateCurIcon(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                          ZukanDetailCommon *common, u16 index);
static void ZukanDetailForm_ChangePokemon(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                          ZukanDetailCommon *common);
static void ZukanDetailForm_LoadTurnedSprites(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                              ZukanDetailCommon *common);
static void ZukanDetailForm_LoadAllSprites(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                           ZukanDetailCommon *common);
static void ZukanDetailForm_ChangeNext(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common);
static void ZukanDetailForm_ChangeIcon(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common);
static void ZukanDetailForm_CheckInput(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common);
static void ZukanDetailForm_SetMode(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common,
                                    int mode);
static void ZukanDetailForm_CreateActors(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                         ZukanDetailCommon *common);
static void ZukanDetailForm_FreeActors(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common);
static FormButton ZukanDetailForm_GetButtonInput(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                                 ZukanDetailCommon *common);
static void ZukanDetailForm_UpdateArrows(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                         ZukanDetailCommon *common);
static void ZukanDetailForm_UpdateButtons(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                          ZukanDetailCommon *common);
static void ZukanDetailForm_UpdateColorMarks(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                             ZukanDetailCommon *common);
static void ZukanDetailForm_UpdateSlider(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                         ZukanDetailCommon *common);
static void ZukanDetailForm_CheckSlider(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                        ZukanDetailCommon *common);
static BOOL ZukanDetailForm_DragSlider(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common);
static BOOL ZukanDetailForm_SnapSlider(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common);
static void ZukanDetailForm_GetSliderSpan(u16 count, u16 index, u8 *start, u8 *center, u8 *end);
static u16 ZukanDetailForm_GetSliderIndex(u16 count, u8 x);
static u16 ZukanDetailForm_GetSliderPos(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common,
                                        u16 cur, u16 next);
static u16 ZukanDetailForm_GetSliderEntry(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                          ZukanDetailCommon *common, u16 cur, u16 pos);
static void ZukanDetailForm_UpdateSelect(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                         ZukanDetailCommon *common);
static void ZukanDetailForm_StepNext(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common);
static void ZukanDetailForm_PlaceNext(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common);
static void ZukanDetailForm_UpdateSlide(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                        ZukanDetailCommon *common);
static void ZukanDetailForm_UpdateOpenClose(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                            ZukanDetailCommon *common);
static void ZukanDetailForm_LoadNext(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common);
static void ZukanDetailForm_SwapCurNext(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                        ZukanDetailCommon *common);
static void ZukanDetailForm_SetActorsOpaque(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                            ZukanDetailCommon *common);
static void ZukanDetailForm_SetActorsBlended(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                             ZukanDetailCommon *common);
static void ZukanDetailForm_InitSpritePositions(ZukanDetailFormPos *pos, u32 species, u32 form, u32 sex, u32 rare,
                                                u32 a6, u32 back, u32 personality);
static void ZukanDetailForm_GetSpritePos(Sprite *sprite, int pos, VecFx32 *out);
static void ZukanDetailForm_GetPlacePos(Sprite *sprite, int place, VecFx32 *out);
static void ZukanDetailForm_GetSpritePosF32(Sprite *sprite, int pos, ZukanDetailFormPos *out);

// Sprite positions, of which the code only uses the last: the offset from a sprite of the one beside it in the
// comparison
static VecFx32 sZukanDetailFormOffsets[3] = {
    { 0, FX32_CONST(-13.9), 0 },
    { FX32_CONST(-16), FX32_CONST(-13.9), 0 },
    { FX32_CONST(32), 0, 0 },
};

// The sprites whose drawing is off center
static const SpritePositions sZukanDetailFormSpritePositions[] = {
    { SPECIES_UNOWN,
      17,
      3,
      0,
      0,
      TRUE,
      0,
      { { -0.2f, -13.9f, 0.0f },
        { -64.2f, -13.9f, 0.0f },
        { 63.8f, -13.9f, 0.0f },
        { -16.2f, -13.9f, 0.0f },
        { 15.8f, -13.9f, 0.0f } } },
    { SPECIES_MILOTIC,
      0,
      3,
      0,
      0,
      TRUE,
      0,
      { { -0.02f, -13.9f, 0.0f },
        { -64.02f, -13.9f, 0.0f },
        { 63.98f, -13.9f, 0.0f },
        { -16.11f, -13.9f, 0.0f },
        { 15.89f, -13.9f, 0.0f } } },
    { SPECIES_KARRABLAST,
      0,
      3,
      0,
      0,
      TRUE,
      0,
      { { 0.0f, -13.7f, 0.0f },
        { -64.0f, -13.7f, 0.0f },
        { 64.0f, -13.7f, 0.0f },
        { -16.4f, -13.7f, 0.0f },
        { 15.9f, -13.7f, 0.0f } } },
    { SPECIES_THROH,
      0,
      3,
      0,
      0,
      FALSE,
      0,
      { { 0.1f, -13.9f, 0.0f },
        { -63.9f, -13.9f, 0.0f },
        { 64.1f, -13.9f, 0.0f },
        { -16.0f, -13.9f, 0.0f },
        { 16.0f, -13.9f, 0.0f } } },
    { SPECIES_OSHAWOTT,
      0,
      3,
      0,
      0,
      TRUE,
      0,
      { { -0.3f, -13.9f, 0.0f },
        { -64.3f, -13.9f, 0.0f },
        { 63.7f, -13.9f, 0.0f },
        { -16.3f, -13.9f, 0.0f },
        { 15.7f, -13.9f, 0.0f } } },
    { SPECIES_AXEW,
      0,
      3,
      0,
      0,
      FALSE,
      0,
      { { -0.3f, -13.9f, 0.0f },
        { -64.3f, -13.9f, 0.0f },
        { 63.7f, -13.9f, 0.0f },
        { -15.7f, -13.9f, 0.0f },
        { 16.0f, -13.9f, 0.0f } } },
    { SPECIES_AXEW,
      0,
      3,
      0,
      0,
      TRUE,
      0,
      { { 0.0f, -13.8f, 0.0f },
        { -64.0f, -13.8f, 0.0f },
        { 64.0f, -13.8f, 0.0f },
        { -16.0f, -13.8f, 0.0f },
        { 16.0f, -13.8f, 0.0f } } },
    { SPECIES_HEATMOR,
      0,
      3,
      0,
      0,
      TRUE,
      0,
      { { 0.0f, -13.8f, 0.0f },
        { -64.0f, -13.8f, 0.0f },
        { 64.0f, -13.8f, 0.0f },
        { -16.0f, -13.8f, 0.0f },
        { 16.0f, -13.8f, 0.0f } } },
    { SPECIES_SEEL,
      0,
      3,
      0,
      0,
      TRUE,
      0,
      { { -0.2f, -13.9f, 0.0f },
        { -64.2f, -13.9f, 0.0f },
        { 63.8f, -13.9f, 0.0f },
        { -16.4f, -13.9f, 0.0f },
        { 15.9f, -13.9f, 0.0f } } },
    { SPECIES_BALTOY,
      0,
      3,
      0,
      0,
      FALSE,
      0,
      { { 0.0f, -13.8f, 0.0f },
        { -64.0f, -13.8f, 0.0f },
        { 64.0f, -13.8f, 0.0f },
        { -16.0f, -13.8f, 0.0f },
        { 16.0f, -13.8f, 0.0f } } },
    { SPECIES_MAWILE,
      0,
      3,
      0,
      0,
      TRUE,
      0,
      { { 0.0f, -13.8f, 0.0f },
        { -64.0f, -13.8f, 0.0f },
        { 64.0f, -13.8f, 0.0f },
        { -16.0f, -13.8f, 0.0f },
        { 16.0f, -13.8f, 0.0f } } },
    { SPECIES_PONYTA,
      0,
      3,
      0,
      0,
      FALSE,
      0,
      { { 0.13f, -13.9f, 0.0f },
        { -63.87f, -13.9f, 0.0f },
        { 64.13f, -13.9f, 0.0f },
        { -16.0f, -13.9f, 0.0f },
        { 16.01f, -13.9f, 0.0f } } },
    { SPECIES_HORSEA,
      0,
      3,
      0,
      0,
      FALSE,
      0,
      { { 0.0f, -13.9f, 0.0f },
        { -64.0f, -13.9f, 0.0f },
        { 64.0f, -13.9f, 0.0f },
        { -16.0f, -13.9f, 0.0f },
        { 16.02f, -13.9f, 0.0f } } },
    { SPECIES_MAGBY,
      0,
      3,
      0,
      0,
      FALSE,
      0,
      { { 0.07f, -13.9f, 0.0f },
        { -63.3f, -13.9f, 0.0f },
        { 64.7f, -13.9f, 0.0f },
        { -16.07f, -13.9f, 0.0f },
        { 16.22f, -13.9f, 0.0f } } },
    { SPECIES_RIOLU,
      0,
      3,
      0,
      0,
      FALSE,
      0,
      { { 0.0f, -13.92f, 0.0f },
        { -64.0f, -13.92f, 0.0f },
        { 64.0f, -13.92f, 0.0f },
        { -16.0f, -13.92f, 0.0f },
        { 16.0f, -13.92f, 0.0f } } },
};

const ZukanDetailProcFuncs ZUKAN_DETAIL_FORM_PROC_FUNCS = {
    ZukanDetailForm_Init, ZukanDetailForm_Main, ZukanDetailForm_Exit, ZukanDetailForm_Command, ZukanDetailForm_Draw,
};

void ZukanDetailForm_InitParam(ZukanDetailFormParam *param, HeapID heapId) {
    param->heapId = heapId;
}

static BOOL ZukanDetailForm_Init(ZukanDetailProcSys *sys, int *seq, void *param_, void *work,
                                 ZukanDetailCommon *common) {
    ZukanDetailFormParam *param = param_;
    ZukanDetailFormWork *wk = ZukanDetailProcSys_AllocWork(sys, sizeof(ZukanDetailFormWork), param->heapId);
    u8 i;

    sys_memset(wk, 0, sizeof(ZukanDetailFormWork));
    wk->unit = ZukanDetailGraphic_GetClActUnit(ZukanDetailCommon_GetGraphic(common));
    wk->font = ZukanDetailCommon_GetFont(common);
    for (i = 0; i < SPRITE_COUNT; i++) {
        wk->sprites[i].mcss = NULL;
        wk->sprites[i].anim = NULL;
    }
    for (i = 0; i < 2; i++) {
        wk->iconActors[i] = NULL;
    }
    wk->icon = 0;
    wk->front = TRUE;
    wk->vblankTcb = GFL_VBlankTCBAdd(ZukanDetailForm_VBlank, wk, 1);
    ZukanDetailForm_CreatePrintQueues(param, wk, common);
    wk->blendMain = ZukanDetailBlend_Create(param->heapId);
    wk->blendSub = ZukanDetailBlend_Create(param->heapId);
    ZukanDetailBlend_InitPlanes(wk->blendMain);
    ZukanDetailBlend_InitPlanes(wk->blendSub);
    ZukanDetailBlend_SetOut(0, wk->blendMain);
    ZukanDetailBlend_SetOut(1, wk->blendSub);
    wk->palFade = ZukanDetailPalFade_Create(param->heapId);
    wk->exit = FORM_EXIT_NONE;
    wk->inputEnabled = TRUE;
    wk->mode = FORM_MODE_SINGLE;
    wk->sliderActive = FALSE;
    wk->selectPending = FALSE;
    wk->selectMoved = TRUE;
    wk->selectButtonDone = TRUE;
    return TRUE;
}

static BOOL ZukanDetailForm_Exit(ZukanDetailProcSys *sys, int *seq, void *param_, void *work,
                                 ZukanDetailCommon *common) {
    ZukanDetailFormParam *param = param_;
    ZukanDetailFormWork *wk = work;
    ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);
    u8 i;

    ZukanDetailBackground_Free(wk->backgroundSub);
    ZukanDetailBackground_Free(wk->backgroundMain);
    ZukanDetail_FreeBG(7, wk->bgChars);
    ZukanDetailForm_FreeActors(param, wk, common);
    for (i = 0; i < 2; i++) {
        if (wk->iconActors[i] != NULL) {
            ZukanDetailForm_FreeIcon(wk->iconChars[i], wk->iconActors[i]);
        }
        wk->iconActors[i] = NULL;
    }
    ZukanDetailForm_FreeIconResources(param, wk, common);
    ZukanDetailForm_FreeText(param, wk, common);
    ZukanDetailTouchbar_SetBGPriority(touchbar, 0);
    ZukanDetailPalFade_Free(wk->palFade);
    ZukanDetailBlend_Free(wk->blendSub);
    ZukanDetailBlend_Free(wk->blendMain);
    ZukanDetailForm_FreePrintQueues(param, wk, common);
    GFL_TCBRemove(wk->vblankTcb);
    ZukanDetailProcSys_FreeWork(sys);
    return TRUE;
}

static BOOL ZukanDetailForm_Main(ZukanDetailProcSys *sys, int *seq, void *param_, void *work,
                                 ZukanDetailCommon *common) {
    ZukanDetailFormParam *param = param_;
    ZukanDetailFormWork *wk = work;
    ZukanDetailGraphic *graphic = ZukanDetailCommon_GetGraphic(common);
    ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);
    ZukanDetailHeadbar *headbar = ZukanDetailCommon_GetHeadbar(common);

    switch (*seq) {
    case FORM_SEQ_INIT: {
        u8 bg;

        *seq = FORM_SEQ_FADE_IN;
        for (bg = 0; bg <= 7; bg++) {
            if (bg != 1 && bg != 5) {
                GFL_BGSysMoveBG(bg, BG_MOVE_SET_X, 0);
                GFL_BGSysMoveBG(bg, BG_MOVE_SET_Y, 0);
                GFL_BGSysClearBG(bg);
            }
        }
        GFL_BGSysSetBGPriority(0, 0);
        GFL_BGSysSetBGPriority(2, 2);
        GFL_BGSysSetBGPriority(3, 3);
        GFL_BGSysSetBGPriority(6, 1);
        GFL_BGSysSetBGPriority(4, 3);
        GFL_BGSysSetBGPriority(7, 2);
        ZukanDetailTouchbar_SetBGPriority(touchbar, 2);
        gfxSetEngineModeA(GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BG0_AS_3D);
        ZukanDetailGraphic_Create3D(graphic, param->heapId);
        ZukanDetailForm_CreateMCSS(param, wk, common);
        ZukanDetailForm_CreateText(param, wk, common);
        ZukanDetailForm_CreateActors(param, wk, common);
        ZukanDetailForm_LoadEntries(param, wk, common);
        ZukanDetailForm_LoadIconResources(param, wk, common);
        ZukanDetailForm_ChangePokemon(param, wk, common);
        func_0204c318(wk->iconActors[wk->icon], 1);
        MCSS_Hide(wk->sprites[SPRITE_CUR_FRONT].mcss);
        func_0201ae2c(wk->sprites[SPRITE_CUR_FRONT].mcss, 16, 16, 0, 0);
        if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
            MCSS_Hide(wk->sprites[SPRITE_NEXT_FRONT].mcss);
            func_0201ae2c(wk->sprites[SPRITE_NEXT_FRONT].mcss, 16, 16, 0, 0);
        }
        wk->bgChars = ZukanDetail_LoadBG(FALSE, param->heapId, 7, 1, 1, 0, ARCID_ZUKAN_GRA, 2, 12, 36, 0);
        wk->backgroundMain = ZukanDetailBackground_Create(param->heapId, 3, 3, 1, 2);
        wk->backgroundSub = ZukanDetailBackground_Create(param->heapId, 3, 4, 2, 3);
        ZukanDetailPalFade_ReadPalettes(wk->palFade);
        ZukanDetailPalFade_SetHidden(wk->palFade);
        break;
    }
    case FORM_SEQ_FADE_IN:
        *seq = FORM_SEQ_WAIT_BLEND;
        ZukanDetailBlend_SetIn(0, wk->blendMain);
        ZukanDetailBlend_SetIn(1, wk->blendSub);
        ZukanDetailForm_SetActorsOpaque(param, wk, common);
        break;
    case FORM_SEQ_WAIT_BLEND:
        *seq = FORM_SEQ_START_PALFADE;
        break;
    case FORM_SEQ_START_PALFADE: {
        BOOL ready = TRUE;

        if (func_0201aee8(wk->sprites[SPRITE_CUR_FRONT].mcss)) {
            ready = FALSE;
        }
        if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL && func_0201aee8(wk->sprites[SPRITE_NEXT_FRONT].mcss)) {
            ready = FALSE;
        }
        if (ready) {
            *seq = FORM_SEQ_WAIT_FADE_IN;
            ZukanDetailPalFade_StartIn(wk->palFade);
            if (ZukanDetailTouchbar_GetState(touchbar) != ZUKAN_DETAIL_TOUCHBAR_SHOWN) {
                ZukanDetailTouchbar_SetType(touchbar, ZUKAN_DETAIL_TOUCHBAR_GENERAL, ZUKAN_DETAIL_PAGE_FORM - 1,
                                            ZukanDetailCommon_GetCount(common) > 1 ? TRUE : FALSE);
                ZukanDetailTouchbar_Appear(touchbar, 0);
            } else {
                ZukanDetailTouchbar_SetPage(touchbar, ZUKAN_DETAIL_PAGE_FORM - 1);
            }
            ZukanDetailTouchbar_SetActive(touchbar, FALSE);
            ZukanDetailTouchbar_SetCheck(
                touchbar, GameData_IsShortcutRegistered(ZukanDetailCommon_GetGameData(common), SHORTCUT_POKEDEX_FORM));
            if (ZukanDetailHeadbar_GetState(headbar) != ZUKAN_DETAIL_TOUCHBAR_SHOWN) {
                ZukanDetailHeadbar_SetTitle(headbar, 3);
                ZukanDetailHeadbar_Appear(headbar);
            }
            MCSS_Show(wk->sprites[SPRITE_CUR_FRONT].mcss);
            func_0201ae2c(wk->sprites[SPRITE_CUR_FRONT].mcss, 16, 0, -2, 0);
            if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
                MCSS_Show(wk->sprites[SPRITE_NEXT_FRONT].mcss);
                func_0201ae2c(wk->sprites[SPRITE_NEXT_FRONT].mcss, 16, 0, -2, 0);
            }
        }
        break;
    }
    case FORM_SEQ_WAIT_FADE_IN: {
        BOOL ready = TRUE;

        if (func_0201aee8(wk->sprites[SPRITE_CUR_FRONT].mcss)) {
            ready = FALSE;
        }
        if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL && func_0201aee8(wk->sprites[SPRITE_NEXT_FRONT].mcss)) {
            ready = FALSE;
        }
        if (!ZukanDetailPalFade_IsFading(wk->palFade) &&
            ZukanDetailTouchbar_GetState(touchbar) == ZUKAN_DETAIL_TOUCHBAR_SHOWN &&
            ZukanDetailHeadbar_GetState(headbar) == ZUKAN_DETAIL_TOUCHBAR_SHOWN && ready) {
            ZukanDetailTouchbar_Unlock(touchbar);
            ZukanDetailTouchbar_SetActive(touchbar, TRUE);
            *seq = FORM_SEQ_MAIN;
        }
        break;
    }
    case FORM_SEQ_MAIN:
        if (wk->exit != FORM_EXIT_NONE) {
            u8 i;

            *seq = FORM_SEQ_FADE_OUT_POKEMON;
            for (i = 0; i < SPRITE_COUNT; i++) {
                if (wk->sprites[i].mcss != NULL) {
                    func_0201ae2c(wk->sprites[i].mcss, 0, 0, 0, 0);
                }
            }
        } else {
            ZukanDetailForm_CheckInput(param, wk, common);
        }
        break;
    case FORM_SEQ_FADE_OUT_POKEMON: {
        BOOL ready = TRUE;
        u8 i;

        for (i = 0; i < SPRITE_COUNT; i++) {
            if (wk->sprites[i].mcss != NULL && func_0201aee8(wk->sprites[i].mcss)) {
                ready = FALSE;
                break;
            }
        }
        if (ready) {
            *seq = FORM_SEQ_WAIT_FADE_OUT;
            ZukanDetailPalFade_StartOut(wk->palFade);
            ZukanDetailHeadbar_Disappear(headbar);
            if (wk->exit == FORM_EXIT_SCREEN) {
                ZukanDetailTouchbar_Disappear(touchbar, 0);
            }
            for (i = 0; i < SPRITE_COUNT; i++) {
                if (wk->sprites[i].mcss != NULL) {
                    func_0201ae2c(wk->sprites[i].mcss, 0, 16, -2, 0);
                }
            }
        }
        break;
    }
    case FORM_SEQ_WAIT_FADE_OUT: {
        BOOL done = FALSE;
        BOOL ready = TRUE;
        u8 i;

        for (i = 0; i < SPRITE_COUNT; i++) {
            if (wk->sprites[i].mcss != NULL && func_0201aee8(wk->sprites[i].mcss)) {
                ready = FALSE;
                break;
            }
        }
        if (!ZukanDetailPalFade_IsFading(wk->palFade) &&
            ZukanDetailHeadbar_GetState(headbar) == ZUKAN_DETAIL_TOUCHBAR_HIDDEN && ready) {
            if (wk->exit == FORM_EXIT_SCREEN) {
                if (ZukanDetailTouchbar_GetState(touchbar) == ZUKAN_DETAIL_TOUCHBAR_HIDDEN) {
                    done = TRUE;
                }
            } else {
                done = TRUE;
            }
        }
        if (done) {
            *seq = FORM_SEQ_FADE_OUT;
        }
        break;
    }
    case FORM_SEQ_FADE_OUT:
        *seq = FORM_SEQ_EXIT;
        ZukanDetailForm_SetActorsBlended(param, wk, common);
        ZukanDetailBlend_SetOut(0, wk->blendMain);
        ZukanDetailBlend_SetOut(1, wk->blendSub);
        break;
    case FORM_SEQ_EXIT: {
        u8 i;

        for (i = 0; i < SPRITE_COUNT; i++) {
            ZukanDetailForm_FreeSprite(&wk->sprites[i], wk->mcssSys);
        }
        ZukanDetailForm_FreeMCSS(param, wk, common);
        ZukanDetailGraphic_Free3D(graphic);
        gfxSetEngineModeA(GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BG0_AS_2D);
        return TRUE;
    }
    }

    if (wk->mcssSys != NULL) {
        u8 i;

        GFL_TCBMgrUpdate(wk->tcbMgr);
        MCSSSys_Update(wk->mcssSys);
        for (i = 0; i < SPRITE_COUNT; i++) {
            ZukanDetailForm_UpdateSprite(&wk->sprites[i]);
        }
    }
    if (*seq >= FORM_SEQ_START_PALFADE) {
        ZukanDetailForm_UpdateButtons(param, wk, common);
        ZukanDetailForm_UpdateSelect(param, wk, common);
        ZukanDetailForm_CheckSlider(param, wk, common);
        ZukanDetailForm_UpdateSlide(param, wk, common);
        ZukanDetailForm_UpdateOpenClose(param, wk, common);
        ZukanDetailBackground_Update(wk->backgroundMain);
        ZukanDetailBackground_Update(wk->backgroundSub);
    }
    ZukanDetailBlend_Update(wk->blendMain, wk->blendSub);
    ZukanDetailPalFade_Update(wk->palFade);
    ZukanDetailForm_UpdateText(param, wk, common);
    return FALSE;
}

// Moves an index around the entries
static inline void ZukanDetailForm_AddIndex(u16 *index, u16 add, u16 count) {
    *index += add;
    *index %= count;
}

static void ZukanDetailForm_Command(ZukanDetailProcSys *sys, int *seq, void *param, void *work,
                                    ZukanDetailCommon *common, int command) {
    ZukanDetailFormWork *wk = work;

    if (wk != NULL) {
        ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);
        BOOL unlock = FALSE;

        wk->sliderSnap = FALSE;
        wk->sliderActive = FALSE;

        // Input waits while the bar's icons play their animations
        switch (command) {
        case ZUKAN_DETAIL_CMD_CLOSE_TOUCH:
        case ZUKAN_DETAIL_CMD_RETURN_TOUCH:
        case ZUKAN_DETAIL_CMD_CUR_D_TOUCH:
        case ZUKAN_DETAIL_CMD_CUR_U_TOUCH:
        case ZUKAN_DETAIL_CMD_CHECK_TOUCH:
        case ZUKAN_DETAIL_CMD_INFO_TOUCH:
        case ZUKAN_DETAIL_CMD_MAP_TOUCH:
        case ZUKAN_DETAIL_CMD_VOICE_TOUCH:
        case ZUKAN_DETAIL_CMD_FORM_CUR_R_TOUCH:
        case ZUKAN_DETAIL_CMD_FORM_CUR_L_TOUCH:
        case ZUKAN_DETAIL_CMD_FORM_CUR_D_TOUCH:
        case ZUKAN_DETAIL_CMD_FORM_CUR_U_TOUCH:
        case ZUKAN_DETAIL_CMD_FORM_BUTTON_TOUCH:
            wk->inputEnabled = FALSE;
            break;
        }
        switch (command) {
        case ZUKAN_DETAIL_CMD_CLOSE:
        case ZUKAN_DETAIL_CMD_RETURN:
        case ZUKAN_DETAIL_CMD_CUR_D:
        case ZUKAN_DETAIL_CMD_CUR_U:
        case ZUKAN_DETAIL_CMD_CHECK:
        case ZUKAN_DETAIL_CMD_INFO:
        case ZUKAN_DETAIL_CMD_MAP:
        case ZUKAN_DETAIL_CMD_VOICE:
        case ZUKAN_DETAIL_CMD_FORM_CUR_R:
        case ZUKAN_DETAIL_CMD_FORM_CUR_L:
        case ZUKAN_DETAIL_CMD_FORM_CUR_D:
        case ZUKAN_DETAIL_CMD_FORM_CUR_U:
        case ZUKAN_DETAIL_CMD_FORM_BUTTON:
            unlock = TRUE;
            wk->inputEnabled = TRUE;
            break;
        }

        switch (command) {
        case ZUKAN_DETAIL_CMD_CUR_D_TOUCH:
        case ZUKAN_DETAIL_CMD_FORM_CUR_D_TOUCH:
            if (wk->mode == FORM_MODE_SINGLE) {
                u16 species = ZukanDetailCommon_GetSpecies(common);

                ZukanDetailCommon_GoNext(common);
                if (species != ZukanDetailCommon_GetSpecies(common)) {
                    ZukanDetailForm_ChangePokemon(param, wk, common);
                }
            } else if (wk->mode == FORM_MODE_COMPARE) {
                // Only Pokémon with two entries or more to compare
                FormEntry entries[ENTRY_MAX];
                u16 count;
                u16 nameCount;
                u16 rareCount;
                u16 cur;
                u16 next;
                u16 species = ZukanDetailCommon_GetSpecies(common);
                u16 newSpecies;

                ZukanDetailCommon_GoNext(common);
                newSpecies = ZukanDetailCommon_GetSpecies(common);
                while (species != newSpecies) {
                    ZukanDetailForm_GatherEntries(param, common, entries, &count, &nameCount, &rareCount, &cur, &next);
                    if (count >= 2) {
                        break;
                    }
                    ZukanDetailCommon_GoNext(common);
                    newSpecies = ZukanDetailCommon_GetSpecies(common);
                }
                if (species != newSpecies) {
                    ZukanDetailForm_ChangePokemon(param, wk, common);
                    if (count >= 3) {
                        ZukanDetailTouchbar_SetFormArrowsVisible(touchbar, TRUE);
                    } else {
                        ZukanDetailTouchbar_SetFormArrowsVisible(touchbar, FALSE);
                    }
                }
            }
            break;
        case ZUKAN_DETAIL_CMD_CUR_U_TOUCH:
        case ZUKAN_DETAIL_CMD_FORM_CUR_U_TOUCH:
            if (wk->mode == FORM_MODE_SINGLE) {
                u16 species = ZukanDetailCommon_GetSpecies(common);

                ZukanDetailCommon_GoPrev(common);
                if (species != ZukanDetailCommon_GetSpecies(common)) {
                    ZukanDetailForm_ChangePokemon(param, wk, common);
                }
            } else if (wk->mode == FORM_MODE_COMPARE) {
                FormEntry entries[ENTRY_MAX];
                u16 count;
                u16 nameCount;
                u16 rareCount;
                u16 cur;
                u16 next;
                u16 species = ZukanDetailCommon_GetSpecies(common);
                u16 newSpecies;

                ZukanDetailCommon_GoPrev(common);
                newSpecies = ZukanDetailCommon_GetSpecies(common);
                while (species != newSpecies) {
                    ZukanDetailForm_GatherEntries(param, common, entries, &count, &nameCount, &rareCount, &cur, &next);
                    if (count >= 2) {
                        break;
                    }
                    ZukanDetailCommon_GoPrev(common);
                    newSpecies = ZukanDetailCommon_GetSpecies(common);
                }
                if (species != newSpecies) {
                    ZukanDetailForm_ChangePokemon(param, wk, common);
                    if (count >= 3) {
                        ZukanDetailTouchbar_SetFormArrowsVisible(touchbar, TRUE);
                    } else {
                        ZukanDetailTouchbar_SetFormArrowsVisible(touchbar, FALSE);
                    }
                }
            }
            break;
        case ZUKAN_DETAIL_CMD_FORM_CUR_R_TOUCH:
            if (wk->count >= 3) {
                ZukanDetailForm_AddIndex(&wk->next, 1, wk->count);
                if (wk->cur == wk->next) {
                    ZukanDetailForm_AddIndex(&wk->next, 1, wk->count);
                }
                ZukanDetailForm_ChangeNext(param, wk, common);
                wk->sliderSnap = TRUE;
                wk->sliderActive = TRUE;
            }
            break;
        case ZUKAN_DETAIL_CMD_FORM_CUR_L_TOUCH:
            if (wk->count >= 3) {
                ZukanDetailForm_AddIndex(&wk->next, wk->count - 1, wk->count);
                if (wk->cur == wk->next) {
                    ZukanDetailForm_AddIndex(&wk->next, wk->count - 1, wk->count);
                }
                ZukanDetailForm_ChangeNext(param, wk, common);
                wk->sliderSnap = TRUE;
                wk->sliderActive = TRUE;
            }
            break;
        case ZUKAN_DETAIL_CMD_FORM_BUTTON_TOUCH:
            if (wk->count >= 2) {
                ZukanDetailForm_SetMode(param, wk, common, FORM_MODE_SELECT);
                wk->selectPending = TRUE;
                wk->selectMoved = FALSE;
                wk->selectButtonDone = FALSE;
            } else {
                wk->selectPending = TRUE;
                wk->selectMoved = TRUE;
                wk->selectButtonDone = FALSE;
            }
            break;
        }

        switch (command) {
        case ZUKAN_DETAIL_CMD_NONE:
            break;
        case ZUKAN_DETAIL_CMD_CLOSE:
        case ZUKAN_DETAIL_CMD_RETURN:
            wk->exit = FORM_EXIT_SCREEN;
            break;
        case ZUKAN_DETAIL_CMD_INFO:
        case ZUKAN_DETAIL_CMD_MAP:
        case ZUKAN_DETAIL_CMD_VOICE:
            wk->exit = FORM_EXIT_PAGE;
            break;
        case ZUKAN_DETAIL_CMD_CUR_D:
        case ZUKAN_DETAIL_CMD_FORM_CUR_D:
            ZukanDetailTouchbar_Unlock(touchbar);
            break;
        case ZUKAN_DETAIL_CMD_CUR_U:
        case ZUKAN_DETAIL_CMD_FORM_CUR_U:
            ZukanDetailTouchbar_Unlock(touchbar);
            break;
        case ZUKAN_DETAIL_CMD_CHECK:
            GameData_SetKeyItemRegistration(ZukanDetailCommon_GetGameData(common), SHORTCUT_POKEDEX_FORM,
                                            ZukanDetailTouchbar_GetCheck(touchbar));
            ZukanDetailTouchbar_Unlock(touchbar);
            break;
        case ZUKAN_DETAIL_CMD_FORM_CUR_R:
            ZukanDetailTouchbar_Unlock(touchbar);
            break;
        case ZUKAN_DETAIL_CMD_FORM_CUR_L:
            ZukanDetailTouchbar_Unlock(touchbar);
            break;
        case ZUKAN_DETAIL_CMD_FORM_RETURN:
            if (wk->mode == FORM_MODE_COMPARE) {
                ZukanDetailForm_SetMode(param, wk, common, FORM_MODE_CLOSE);
            }
            break;
        case ZUKAN_DETAIL_CMD_FORM_BUTTON:
            if (wk->selectPending) {
                wk->selectButtonDone = TRUE;
                if (wk->selectMoved) {
                    wk->selectPending = FALSE;
                    ZukanDetailTouchbar_Unlock(touchbar);
                }
            }
            break;
        default:
            if (unlock) {
                ZukanDetailTouchbar_Unlock(touchbar);
            }
            break;
        }
    }
}

static void ZukanDetailForm_Draw(ZukanDetailProcSys *sys, int *seq, void *param, void *work,
                                 ZukanDetailCommon *common) {
    ZukanDetailFormWork *wk = work;

    if (wk != NULL && wk->mcssSys != NULL) {
        MCSSSys_Draw(wk->mcssSys);
    }
}

static void ZukanDetailForm_VBlank(TCB *tcb, void *data) {
    ZukanDetailFormWork *wk = data;

    ZukanDetailPalFade_VBlank(wk->palFade);
}

static void ZukanDetailForm_GetEntryText(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                         ZukanDetailCommon *common, StrBuf **name, StrBuf **color, BOOL *freeName,
                                         BOOL *freeColor, u16 index) {
    StrBuf *nameStr = NULL;
    StrBuf *colorStr = NULL;
    BOOL nameNew = FALSE;
    BOOL colorNew = FALSE;
    u16 species = ZukanDetailCommon_GetSpecies(common);

    switch (wk->entries[index].name) {
    case ENTRY_NAME_SPECIES: {
        WordSet *wordSet = GFL_WordSetSystemCreateDefault(param->heapId);
        StrBuf *template = wk->strbufs[STRBUF_SPECIES];

        nameStr = GFL_StrBufCreate(64, param->heapId);
        WordSet_LoadSpeciesName(wordSet, 0, species);
        GFL_WordSetFormatStrbuf(wordSet, nameStr, template);
        GFL_WordSetSystemFree(wordSet);
        nameNew = TRUE;
        break;
    }
    case ENTRY_NAME_SEX:
        switch (wk->entries[index].value) {
        case 0:
            nameStr = wk->strbufs[STRBUF_MALE];
            break;
        case 1:
            nameStr = wk->strbufs[STRBUF_FEMALE];
            break;
        }
        break;
    case ENTRY_NAME_FORM: {
        u16 form = wk->entries[index].value;
        u16 i;

        for (i = 0; i != form; i++) {
            species = wk->formNameTable[species];
            if (species == 0) {
                break;
            }
        }
        nameStr = GFL_MsgDataLoadStrbufNew(wk->formMsgData, species);
        nameNew = TRUE;
        break;
    }
    }

    switch (wk->entries[index].color) {
    case ENTRY_COLOR_NONE:
    case ENTRY_COLOR_NORMAL:
        break;
    case ENTRY_COLOR_RARE:
        colorStr = wk->strbufs[STRBUF_RARE];
        colorNew = FALSE;
        break;
    }

    *name = nameStr;
    *color = colorStr;
    *freeName = nameNew;
    *freeColor = colorNew;
}

static void ZukanDetailForm_CreateText(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                       ZukanDetailCommon *common) {
    ZukanDetailWindowData windows[WINDOW_COUNT] = {
        { 6, 19, 16, 8, 2, 0, 1 }, { 6, 27, 19, 2, 4, 0, 1 }, { 6, 3, 19, 23, 4, 0, 1 },
        { 2, 0, 1, 16, 4, 0, 1 },  { 2, 16, 1, 16, 4, 0, 1 },
    };
    u32 size;
    StrBuf *strbuf2;
    StrBuf *strbuf1;
    GFLBitmap *bitmap;
    u8 i;

    GFL_G2DIOLoadNCLR(ARCID_FONT, 5, PALTYPE_SUB_BG, 0, 0, 0x20, param->heapId);
    GFL_G2DIOLoadNCLR(ARCID_FONT, 5, PALTYPE_MAIN_BG, 0, 0, 0x20, param->heapId);
    for (i = 0; i < WINDOW_COUNT; i++) {
        wk->windows[i] = BmpWin_CreateDynamic(windows[i].bg, windows[i].x, windows[i].y, windows[i].width,
                                              windows[i].height, windows[i].palette, windows[i].fromEnd);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[i]), 0);
        BmpWin_FlushChar(wk->windows[i]);
    }
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_SERVER_FLOW_TITLE, param->heapId);
    wk->formMsgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0450, param->heapId);
    wk->formNameTable = GFL_ArcSysReadHeapNewLZGetLen(ARCID_FORM_NAME_TABLE, 0, FALSE, param->heapId, &size);

    bitmap = BmpWin_GetBitmap(wk->windows[WINDOW_LABELS]);
    strbuf1 = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlServerFlowTitle_Text_FormsSeen);
    func_02021c7c(wk->printQueues[WINDOW_LABELS], bitmap, 0, 1, strbuf1, wk->font, PRINT_COLOR(15, 2, 0));
    GFL_StrBufFree(strbuf1);
    strbuf2 = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlServerFlowTitle_Text_ShinySeen);
    func_02021c7c(wk->printQueues[WINDOW_LABELS], bitmap, 0, 17, strbuf2, wk->font, PRINT_COLOR(15, 2, 0));
    GFL_StrBufFree(strbuf2);
    wk->flushPending[WINDOW_LABELS] = TRUE;
    ZukanDetailForm_FlushWindow(param, wk, common, WINDOW_LABELS);

    for (i = 0; i < STRBUF_COUNT; i++) {
        wk->strbufs[i] = GFL_MsgDataLoadStrbufNew(wk->msgData, ZUKAN_DETAIL_FORM_STRBUF_MESSAGES[i]);
    }
}

static void ZukanDetailForm_FreeText(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        wk->flushPending[i] = FALSE;
        func_02021c44(wk->printQueues[i]);
        BmpWin_Free(wk->windows[i]);
    }
    for (i = 0; i < STRBUF_COUNT; i++) {
        GFL_StrBufFree(wk->strbufs[i]);
    }
    GFL_MsgDataFree(wk->msgData);
    GFL_MsgDataFree(wk->formMsgData);
    GFL_HeapFree(wk->formNameTable);
}

static void ZukanDetailForm_CreatePrintQueues(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                              ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        wk->printQueues[i] = func_02021998(param->heapId);
        wk->flushPending[i] = FALSE;
    }
}

static void ZukanDetailForm_FreePrintQueues(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                            ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        wk->flushPending[i] = FALSE;
        func_02021c44(wk->printQueues[i]);
        func_02021a18(wk->printQueues[i]);
    }
}

static void ZukanDetailForm_UpdateText(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                       ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        func_02021a3c(wk->printQueues[i]);
        ZukanDetailForm_FlushWindow(param, wk, common, i);
    }
}

static void ZukanDetailForm_FlushWindow(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common,
                                        u8 window) {
    if (wk->flushPending[window] && !func_02021c1c(wk->printQueues[window], BmpWin_GetBitmap(wk->windows[window]))) {
        BmpWin_Transfer(wk->windows[window]);
        wk->flushPending[window] = FALSE;
    }
}

static void ZukanDetailForm_PrintName(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common) {
    u8 i;

    for (i = WINDOW_NAME; i <= WINDOW_COUNTS; i++) {
        wk->flushPending[i] = FALSE;
        func_02021c44(wk->printQueues[i]);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[i]), 0);
    }

    {
        u16 species = ZukanDetailCommon_GetSpecies(common);
        GFLBitmap *bitmap = BmpWin_GetBitmap(wk->windows[WINDOW_NAME]);
        StrBuf *name = GFL_MsgDataLoadStrbufNew(g_PMLSpeciesNamesResident, species);
        u16 width = GFL_FontGetBlockWidth(name, wk->font, 0);
        int windowWidth = GFL_BitmapGetWidth(bitmap);
        u16 x = (windowWidth - width) / 2;

        func_02021c7c(wk->printQueues[WINDOW_NAME], bitmap, x, 1, name, wk->font, PRINT_COLOR(15, 2, 0));
        wk->flushPending[WINDOW_NAME] = TRUE;
        GFL_StrBufFree(name);
    }

    {
        GFLBitmap *bitmap = BmpWin_GetBitmap(wk->windows[WINDOW_COUNTS]);

        {
            WordSet *wordSet = GFL_WordSetSystemCreateDefault(param->heapId);
            StrBuf *template = wk->strbufs[STRBUF_FORM_COUNT];
            StrBuf *strbuf = GFL_StrBufCreate(8, param->heapId);

            WordSetNumber(wordSet, 0, wk->nameCount, 2, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, strbuf, template);
            func_02021c7c(wk->printQueues[WINDOW_COUNTS], bitmap, 0, 1, strbuf, wk->font, PRINT_COLOR(15, 2, 0));
            GFL_StrBufFree(strbuf);
            GFL_WordSetSystemFree(wordSet);
        }
        {
            WordSet *wordSet = GFL_WordSetSystemCreateDefault(param->heapId);
            StrBuf *template = wk->strbufs[STRBUF_RARE_COUNT];
            StrBuf *strbuf = GFL_StrBufCreate(8, param->heapId);

            WordSetNumber(wordSet, 0, wk->rareCount, 2, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, strbuf, template);
            func_02021c7c(wk->printQueues[WINDOW_COUNTS], bitmap, 0, 17, strbuf, wk->font, PRINT_COLOR(15, 2, 0));
            GFL_StrBufFree(strbuf);
            GFL_WordSetSystemFree(wordSet);
        }
        wk->flushPending[WINDOW_COUNTS] = TRUE;
    }

    for (i = WINDOW_NAME; i <= WINDOW_COUNTS; i++) {
        ZukanDetailForm_FlushWindow(param, wk, common, i);
    }
}

static void ZukanDetailForm_PrintEntry(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common,
                                       u8 window, PrintQueue *printQueue, BmpWin *bmpWin, BOOL *flushPending,
                                       u16 index) {
    GFLBitmap *bitmap = BmpWin_GetBitmap(bmpWin);
    int windowWidth = GFL_BitmapGetWidth(bitmap);
    StrBuf *name;
    StrBuf *color;
    BOOL freeName;
    BOOL freeColor;
    int y;

    ZukanDetailForm_GetEntryText(param, wk, common, &name, &color, &freeName, &freeColor, index);
    // The name alone is printed in the middle of the window
    y = 1;
    if (color == NULL) {
        y = 9;
    }
    {
        u16 width = GFL_FontGetBlockWidth(name, wk->font, 0);
        u16 x = (windowWidth - width) / 2;

        func_02021c7c(printQueue, bitmap, x, y, name, wk->font, PRINT_COLOR(15, 2, 0));
    }
    if (color != NULL) {
        u16 width = GFL_FontGetBlockWidth(color, wk->font, 0);
        u16 x = (windowWidth - width) / 2;

        y += 16;
        func_02021c7c(printQueue, bitmap, x, y, color, wk->font, PRINT_COLOR(15, 2, 0));
    }
    if (name != NULL && freeName) {
        GFL_StrBufFree(name);
    }
    if (color != NULL && freeColor) {
        GFL_StrBufFree(color);
    }
    *flushPending = TRUE;
    ZukanDetailForm_FlushWindow(param, wk, common, window);
}

static void ZukanDetailForm_PrintCur(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common) {
    wk->flushPending[WINDOW_CUR] = FALSE;
    func_02021c44(wk->printQueues[WINDOW_CUR]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_CUR]), 0);
    ZukanDetailForm_PrintEntry(param, wk, common, WINDOW_CUR, wk->printQueues[WINDOW_CUR], wk->windows[WINDOW_CUR],
                               &wk->flushPending[WINDOW_CUR], wk->cur);
}

static void ZukanDetailForm_PrintNext(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common) {
    wk->flushPending[WINDOW_NEXT] = FALSE;
    func_02021c44(wk->printQueues[WINDOW_NEXT]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_NEXT]), 0);
    switch (wk->mode) {
    case FORM_MODE_SINGLE:
        BmpWin_FlushChar(wk->windows[WINDOW_NEXT]);
        break;
    case FORM_MODE_COMPARE:
        if (wk->count >= 2) {
            ZukanDetailForm_PrintEntry(param, wk, common, WINDOW_NEXT, wk->printQueues[WINDOW_NEXT],
                                       wk->windows[WINDOW_NEXT], &wk->flushPending[WINDOW_NEXT], wk->next);
        }
        break;
    }
}

// The second entry's name shows on the BG beside the first one's in the comparison
static void ZukanDetailForm_ScrollNextBG(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                         ZukanDetailCommon *common) {
    switch (wk->mode) {
    case FORM_MODE_SINGLE:
        GFL_BGSysMoveBGReq(2, BG_MOVE_SET_X, -64);
        break;
    case FORM_MODE_COMPARE:
        if (wk->count >= 2) {
            GFL_BGSysMoveBGReq(2, BG_MOVE_SET_X, 0);
        } else {
            GFL_BGSysMoveBGReq(2, BG_MOVE_SET_X, -64);
        }
        break;
    }
}

static void ZukanDetailForm_CreateMCSS(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                       ZukanDetailCommon *common) {
    wk->mcssSys = MCSSSys_Create(SPRITE_COUNT, param->heapId);
    func_0201aefc(wk->mcssSys, 0);
    func_0201aacc(wk->mcssSys);
    func_02019b98(wk->mcssSys, GetPokemonGraphicsARCID());
    wk->tcbBuffer =
        GFL_HeapAllocate(param->heapId, GFL_TCBMgrCalcAllocSize(SPRITE_COUNT), FALSE, "zukan_detail_form.c", 2643);
    wk->tcbMgr = GFL_TCBMgrCreate(SPRITE_COUNT, wk->tcbBuffer);
    func_02019bcc(wk->mcssSys, wk->tcbMgr);
}

static void ZukanDetailForm_FreeMCSS(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common) {
    func_02019bb4(wk->mcssSys);
    MCSSSys_Free(wk->mcssSys);
    func_0203a610(wk->tcbMgr);
    GFL_HeapFree(wk->tcbBuffer);
}

static MCSS *ZukanDetailForm_AddMCSS(MCSSSystem *system, u32 species, u32 form, u32 sex, u32 rare, u32 a6, u32 back,
                                     u32 personality) {
    VecFx32 scale = { FX32_ONE * 16, FX32_ONE * 16, FX32_ONE };
    MCSSLoadInfo info;
    MCSS *mcss;

    if (species == SPECIES_SPINDA) {
        func_0201c188(system, personality);
    }
    SetupPokemonLoaderFSTool(species, form, sex, rare, a6, &info, back);
    mcss = MCSSSys_Add(system, 0, 0, 0, &info);
    func_0201aecc(mcss, 1);
    MCSS_PauseAnimation(mcss);
    MCSS_SetScale(mcss, &scale);
    ZukanDetailForm_SetShadow(mcss);
    return mcss;
}

static void ZukanDetailForm_RemoveMCSS(MCSSSystem *system, MCSS *mcss) {
    MCSS_Hide(mcss);
    MCSSSys_Remove(system, mcss);
}

static void ZukanDetailForm_SetShadow(MCSS *mcss) {
    f32 x = -(f32)func_0201adf0(mcss) * 0.25f;
    VecFx32 offset;

    offset.x = FX32_CONST(x);
    offset.y = 0;
    offset.z = 0;
    func_0201ab54(mcss, &offset);
}

static void ZukanDetailForm_GetBesidePos(MCSS *mcss, VecFx32 *pos) {
    VecFx32 cur;

    MCSS_GetPosition(mcss, &cur);
    pos->x = cur.x + sZukanDetailFormOffsets[2].x;
    pos->y = cur.y + sZukanDetailFormOffsets[2].y;
    pos->z = cur.z + sZukanDetailFormOffsets[2].z;
}

static void ZukanDetailForm_CreateSprite(Sprite *sprite, HeapID heapId, MCSSSystem *system, u32 species, u32 form,
                                         u32 sex, u32 rare, u32 a6, u32 back, u32 personality) {
    sprite->mcss = ZukanDetailForm_AddMCSS(system, species, form, sex, rare, a6, back, personality);
    sprite->anim = GFL_HeapAllocate(heapId, sizeof(SpriteAnim), TRUE, "zukan_detail_form.c", 2737);
    NNS_G2dSetAnimCtrlCallBackFunctor(&func_0201adc4(sprite->mcss)->animCtrl, NNS_G2D_ANMCALLBACKTYPE_LAST_FRM,
                                      (u32)sprite->anim, ZukanDetailForm_SpriteAnimEnd);
    ZukanDetailForm_InitSpritePositions(sprite->pos, species, form, sex, rare, a6, back, personality);
}

static void ZukanDetailForm_FreeSprite(Sprite *sprite, MCSSSystem *system) {
    if (sprite->mcss != NULL) {
        ZukanDetailForm_RemoveMCSS(system, sprite->mcss);
    }
    sprite->mcss = NULL;
    if (sprite->anim != NULL) {
        GFL_HeapFree(sprite->anim);
    }
    sprite->anim = NULL;
}

static void ZukanDetailForm_InitSpriteAnim(SpriteAnim *anim, int sprite, ZukanDetailFormWork *wk) {
    anim->sprite = sprite;
    anim->wk = wk;
    anim->loops = 0;
    anim->stop = FALSE;
}

static void ZukanDetailForm_PlaySpriteAnim(Sprite *sprite) {
    if (sprite->mcss != NULL && sprite->anim != NULL) {
        NNSG2dMultiCellAnimation *mcAnim = func_0201adc4(sprite->mcss);

        sprite->anim->loops = 0;
        sprite->anim->stop = FALSE;
        NNS_G2dRestartMCAnimation(mcAnim);
        MCSS_ResumeAnimation(sprite->mcss);
    }
}

static void ZukanDetailForm_StopSpriteAnim(Sprite *sprite) {
    if (sprite->mcss != NULL && sprite->anim != NULL) {
        sprite->anim->stop = TRUE;
    }
}

static void ZukanDetailForm_UpdateSprite(Sprite *sprite) {
    if (sprite->mcss != NULL && sprite->anim != NULL) {
        NNSG2dMultiCellAnimation *mcAnim = func_0201adc4(sprite->mcss);

        if (sprite->anim->stop) {
            MCSS_PauseAnimation(sprite->mcss);
            NNS_G2dRestartMCAnimation(mcAnim);
            sprite->anim->stop = FALSE;
        }
    }
}

static void ZukanDetailForm_SpriteAnimEnd(u32 param, fx32 frame) {
    SpriteAnim *anim = (SpriteAnim *)param;
    ZukanDetailFormWork *wk = anim->wk;

    anim->loops++;
    if (anim->loops >= ANIM_LOOPS) {
        anim->stop = TRUE;
    }
    func_0201b25c(wk->sprites[anim->sprite].mcss);
}

static void ZukanDetailForm_Turn(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common) {
    if (wk->front) {
        MCSS_Hide(wk->sprites[SPRITE_CUR_FRONT].mcss);
        if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
            MCSS_Hide(wk->sprites[SPRITE_NEXT_FRONT].mcss);
        }
        wk->front = FALSE;
    } else {
        MCSS_Hide(wk->sprites[SPRITE_CUR_BACK].mcss);
        if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
            MCSS_Hide(wk->sprites[SPRITE_NEXT_BACK].mcss);
        }
        wk->front = TRUE;
    }
    ZukanDetailForm_LoadTurnedSprites(param, wk, common);
    if (wk->front) {
        MCSS_Show(wk->sprites[SPRITE_CUR_FRONT].mcss);
        if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
            MCSS_Show(wk->sprites[SPRITE_NEXT_FRONT].mcss);
        }
    } else {
        MCSS_Show(wk->sprites[SPRITE_CUR_BACK].mcss);
        if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
            MCSS_Show(wk->sprites[SPRITE_NEXT_BACK].mcss);
        }
    }
}

static void ZukanDetailForm_LoadIconResources(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                              ZukanDetailCommon *common) {
    wk->iconArc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, param->heapId);
    wk->iconPalette = func_0204bc48(wk->iconArc, func_02021114(), CLACT_VRAM_SUB, 0, param->heapId);
    wk->iconCellAnims = func_0204bde0(wk->iconArc, func_02021154(), getOBJTileMapping_SubEng(), param->heapId);
}

static void ZukanDetailForm_FreeIconResources(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                              ZukanDetailCommon *common) {
    func_0204be64(wk->iconCellAnims);
    func_0204bcd0(wk->iconPalette);
    GFL_ArcToolFree(wk->iconArc);
}

static ClActor *ZukanDetailForm_CreateIcon(ArcTool *arc, u32 *chars, u32 palette, u32 cellAnims, ClActUnit *unit,
                                           HeapID heapId, u32 species, u32 form, u32 sex, BOOL egg) {
    ClActorSetup setup;
    ClActor *actor;

    setup.x = 128;
    setup.y = 128;
    setup.sequence = 1;
    setup.priority = 1;
    setup.bgPriority = 0;
    *chars = func_0204b81c(arc, PokeParty_GetIconIndex(species, form, sex, egg), FALSE, CLACT_VRAM_SUB, heapId);
    actor = func_0204c040(unit, *chars, palette, cellAnims, &setup, CLACT_SURFACE_SUB, heapId);
    func_0204c520(actor, FALSE);
    func_0204c378(actor, func_02021034(species, form, sex, egg), 0);
    func_0204c318(actor, 0);
    return actor;
}

static void ZukanDetailForm_FreeIcon(u32 chars, ClActor *actor) {
    func_0204c108(actor);
    func_0204b98c(chars);
}

// Lists every sex, color and form of the species that the Pokédex has seen. The one that it shows comes first, with
// the next one as the second entry
static void ZukanDetailForm_GatherEntries(ZukanDetailFormParam *param, ZukanDetailCommon *common, FormEntry *entries,
                                          u16 *count, u16 *nameCount, u16 *rareCount, u16 *cur, u16 *next) {
    u32 sexes[3] = { 0, 1, 2 };
    BOOL rares[2] = { FALSE, TRUE };
    u32 shownSex;
    u32 shownRare;
    u32 shownForm;
    u16 species = ZukanDetailCommon_GetSpecies(common);
    PokeDexSave *pokedex = GameData_GetPokedex(ZukanDetailCommon_GetGameData(common));
    u32 sexRatio;
    u16 shownEntry;
    u8 i;
    u8 j;
    BOOL found;
    u16 prevValue;
    u32 form;
    u16 n = 0;
    void *personal = PML_PersonalLoad(species, 0, param->heapId);
    // The personal data's count of forms is read, but the Pokédex's own count is used
    u32 formCount = PML_PersonalGetParam(personal, PERSONAL_FORM_COUNT);

    sexRatio = PML_PersonalGetParam(personal, PERSONAL_SEX_RATIO);
    PML_PersonalFree(personal);
    formCount = getNumberOfForms(species);
    func_0200d3c8(pokedex, species, &shownSex, &shownRare, &shownForm, param->heapId);
    shownEntry = 0;
    {
        BOOL hasSex[3] = { FALSE, FALSE, FALSE };

        switch (sexRatio) {
        case 0:
            hasSex[0] = TRUE;
            break;
        case 254:
            hasSex[1] = TRUE;
            break;
        case 255:
            hasSex[2] = TRUE;
            break;
        default:
            hasSex[0] = TRUE;
            hasSex[1] = TRUE;
            break;
        }

        for (form = 0; form < formCount; form++) {
            BOOL seenRare[2] = { FALSE, FALSE };

            for (i = 0; i < 3; i++) {
                if (hasSex[i]) {
                    u32 sex = sexes[i];

                    for (j = 0; j < 2; j++) {
                        BOOL rare;

                        found = FALSE;
                        rare = rares[j];
                        if (shownForm == form && shownRare == rare && shownSex == sex) {
                            shownEntry = n;
                            found = TRUE;
                        } else if (formCount >= 2 && shownForm == form && shownRare == rare) {
                            // Every form is listed once in each color, as the one shown
                        } else if (func_0200d8d4(pokedex, species, sex, rare, form)) {
                            found = TRUE;
                        }
                        if (found && ((formCount >= 2 && !seenRare[j]) || formCount < 2)) {
                            seenRare[j] = TRUE;
                            if (formCount >= 2) {
                                entries[n].name = ENTRY_NAME_FORM;
                                entries[n].color = rare ? ENTRY_COLOR_RARE : ENTRY_COLOR_NORMAL;
                                entries[n].value = form;
                            } else {
                                entries[n].name = sexRatio == 255 ? ENTRY_NAME_SPECIES : ENTRY_NAME_SEX;
                                entries[n].color = rare ? ENTRY_COLOR_RARE : ENTRY_COLOR_NORMAL;
                                entries[n].value = sex == 1 ? 1 : 0;
                            }
                            entries[n].sex = sex;
                            entries[n].rare = rare;
                            entries[n].form = form;
                            n++;
                        }
                    }
                }
            }
        }
    }

    *count = n;
    *cur = shownEntry;
    *next = (shownEntry + 1) % n;
    *nameCount = 0;
    *rareCount = 0;
    for (i = 0; i < *count; i++) {
        if (entries[i].color == ENTRY_COLOR_RARE) {
            (*rareCount)++;
        }
    }
    for (i = 0; i < *count; i++) {
        if (entries[i].name == ENTRY_NAME_SPECIES) {
            *nameCount = 1;
        } else if (i == 0 || prevValue != entries[i].value) {
            prevValue = entries[i].value;
            (*nameCount)++;
        }
    }
}

static void ZukanDetailForm_LoadEntries(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                        ZukanDetailCommon *common) {
    ZukanDetailForm_GatherEntries(param, common, wk->entries, &wk->count, &wk->nameCount, &wk->rareCount, &wk->cur,
                                  &wk->next);
}

// Adds the entry's front and back sprites, unless SPRITE_NONE
static void ZukanDetailForm_CreateSprites(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                          ZukanDetailCommon *common, int frontSprite, int backSprite, int place,
                                          u16 index) {
    u32 personality = 0;
    u16 species = ZukanDetailCommon_GetSpecies(common);
    u32 form = wk->entries[index].form;
    u32 sex = wk->entries[index].sex;
    u32 rare = wk->entries[index].rare;
    VecFx32 pos;

    if (species == SPECIES_SPINDA) {
        personality = func_0200da18(GameData_GetPokedex(ZukanDetailCommon_GetGameData(common)), 0);
    }
    if (frontSprite != SPRITE_NONE) {
        ZukanDetailForm_CreateSprite(&wk->sprites[frontSprite], param->heapId, wk->mcssSys, species, form, sex, rare, 0,
                                     0, personality);
        ZukanDetailForm_GetPlacePos(&wk->sprites[frontSprite], place, &pos);
        MCSS_SetPosition(wk->sprites[frontSprite].mcss, &pos);
        if (!wk->front) {
            MCSS_Hide(wk->sprites[frontSprite].mcss);
        }
    }
    if (backSprite != SPRITE_NONE) {
        ZukanDetailForm_CreateSprite(&wk->sprites[backSprite], param->heapId, wk->mcssSys, species, form, sex, rare, 0,
                                     1, personality);
        ZukanDetailForm_GetPlacePos(&wk->sprites[backSprite], place, &pos);
        MCSS_SetPosition(wk->sprites[backSprite].mcss, &pos);
        if (wk->front) {
            MCSS_Hide(wk->sprites[backSprite].mcss);
        }
    }
}

static void ZukanDetailForm_CreateCurIcon(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                          ZukanDetailCommon *common, u16 index) {
    u16 species = ZukanDetailCommon_GetSpecies(common);

    wk->iconActors[wk->icon] =
        ZukanDetailForm_CreateIcon(wk->iconArc, &wk->iconChars[wk->icon], wk->iconPalette, wk->iconCellAnims, wk->unit,
                                   param->heapId, species, wk->entries[index].form, wk->entries[index].sex, FALSE);
}

// Whether the comparison shows the second entry
static inline BOOL ZukanDetailForm_IsComparing(ZukanDetailFormWork *wk) {
    BOOL comparing = FALSE;

    if (wk->mode == FORM_MODE_COMPARE && wk->count >= 2) {
        comparing = TRUE;
    }
    return comparing;
}

static void ZukanDetailForm_ChangePokemon(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                          ZukanDetailCommon *common) {
    u8 i;
    int frontSprite;
    int backSprite;
    int place;

    for (i = 0; i < SPRITE_COUNT; i++) {
        ZukanDetailForm_FreeSprite(&wk->sprites[i], wk->mcssSys);
    }
    ZukanDetailForm_LoadEntries(param, wk, common);
    place = PLACE_SINGLE;
    if (ZukanDetailForm_IsComparing(wk)) {
        place = PLACE_LEFT;
    }
    frontSprite = wk->front ? SPRITE_CUR_FRONT : SPRITE_NONE;
    backSprite = wk->front ? SPRITE_NONE : SPRITE_CUR_BACK;
    ZukanDetailForm_CreateSprites(param, wk, common, frontSprite, backSprite, place, wk->cur);
    if (frontSprite != SPRITE_NONE) {
        ZukanDetailForm_InitSpriteAnim(wk->sprites[SPRITE_CUR_FRONT].anim, SPRITE_CUR_FRONT, wk);
    }
    if (backSprite != SPRITE_NONE) {
        ZukanDetailForm_InitSpriteAnim(wk->sprites[SPRITE_CUR_BACK].anim, SPRITE_CUR_BACK, wk);
    }
    if (ZukanDetailForm_IsComparing(wk)) {
        frontSprite = wk->front ? SPRITE_NEXT_FRONT : SPRITE_NONE;
        backSprite = wk->front ? SPRITE_NONE : SPRITE_NEXT_BACK;
        ZukanDetailForm_CreateSprites(param, wk, common, frontSprite, backSprite, PLACE_RIGHT, wk->next);
        if (frontSprite != SPRITE_NONE) {
            ZukanDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_FRONT].anim, SPRITE_NEXT_FRONT, wk);
        }
        if (backSprite != SPRITE_NONE) {
            ZukanDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_BACK].anim, SPRITE_NEXT_BACK, wk);
        }
    }
    ZukanDetailForm_ChangeIcon(param, wk, common);
    ZukanDetailForm_PrintName(param, wk, common);
    ZukanDetailForm_ScrollNextBG(param, wk, common);
    ZukanDetailForm_PrintCur(param, wk, common);
    ZukanDetailForm_PrintNext(param, wk, common);
    ZukanDetailForm_UpdateColorMarks(param, wk, common);
    ZukanDetailForm_UpdateSlider(param, wk, common);
    ZukanDetailForm_UpdateArrows(param, wk, common);
}

// Adds the sprites of the side now shown that have not been loaded yet, and places the others
static void ZukanDetailForm_LoadTurnedSprites(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                              ZukanDetailCommon *common) {
    int frontSprite;
    int backSprite;
    int place;
    VecFx32 pos;

    func_02019bcc(wk->mcssSys, NULL);
    frontSprite = SPRITE_CUR_FRONT;
    if (wk->sprites[SPRITE_CUR_FRONT].mcss != NULL) {
        frontSprite = SPRITE_NONE;
    }
    backSprite = SPRITE_CUR_BACK;
    if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
        backSprite = SPRITE_NONE;
    }
    if (wk->front) {
        backSprite = SPRITE_NONE;
    } else {
        frontSprite = SPRITE_NONE;
    }
    place = PLACE_SINGLE;
    if (ZukanDetailForm_IsComparing(wk)) {
        place = PLACE_LEFT;
    }
    ZukanDetailForm_CreateSprites(param, wk, common, frontSprite, backSprite, place, wk->cur);
    if (frontSprite != SPRITE_NONE) {
        ZukanDetailForm_InitSpriteAnim(wk->sprites[SPRITE_CUR_FRONT].anim, SPRITE_CUR_FRONT, wk);
    } else if (wk->sprites[SPRITE_CUR_FRONT].mcss != NULL) {
        ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_FRONT], place, &pos);
        MCSS_SetPosition(wk->sprites[SPRITE_CUR_FRONT].mcss, &pos);
    }
    if (backSprite != SPRITE_NONE) {
        ZukanDetailForm_InitSpriteAnim(wk->sprites[SPRITE_CUR_BACK].anim, SPRITE_CUR_BACK, wk);
    } else if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
        ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_BACK], place, &pos);
        MCSS_SetPosition(wk->sprites[SPRITE_CUR_BACK].mcss, &pos);
    }
    if (ZukanDetailForm_IsComparing(wk)) {
        frontSprite = SPRITE_NEXT_FRONT;
        if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
            frontSprite = SPRITE_NONE;
        }
        backSprite = SPRITE_NEXT_BACK;
        if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
            backSprite = SPRITE_NONE;
        }
        ZukanDetailForm_CreateSprites(param, wk, common, frontSprite, backSprite, PLACE_RIGHT, wk->next);
        if (frontSprite != SPRITE_NONE) {
            ZukanDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_FRONT].anim, SPRITE_NEXT_FRONT, wk);
        } else if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
            ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_NEXT_FRONT], PLACE_RIGHT, &pos);
            MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &pos);
        }
        if (backSprite != SPRITE_NONE) {
            ZukanDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_BACK].anim, SPRITE_NEXT_BACK, wk);
        } else if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
            ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_NEXT_BACK], PLACE_RIGHT, &pos);
            MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &pos);
        }
    }
    func_02019bcc(wk->mcssSys, wk->tcbMgr);
    if (wk->mode == FORM_MODE_SINGLE) {
        wk->slideFromRight = TRUE;
        ZukanDetailForm_PlaceNext(param, wk, common);
    }
}

// Adds every sprite not loaded yet, to play their animations
static void ZukanDetailForm_LoadAllSprites(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                           ZukanDetailCommon *common) {
    int frontSprite = SPRITE_CUR_FRONT;
    int backSprite;
    int place;
    VecFx32 pos;

    func_02019bcc(wk->mcssSys, NULL);
    if (wk->sprites[SPRITE_CUR_FRONT].mcss != NULL) {
        frontSprite = SPRITE_NONE;
    }
    backSprite = SPRITE_CUR_BACK;
    if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
        backSprite = SPRITE_NONE;
    }
    place = PLACE_SINGLE;
    if (ZukanDetailForm_IsComparing(wk)) {
        place = PLACE_LEFT;
    }
    ZukanDetailForm_CreateSprites(param, wk, common, frontSprite, backSprite, place, wk->cur);
    if (frontSprite != SPRITE_NONE) {
        ZukanDetailForm_InitSpriteAnim(wk->sprites[SPRITE_CUR_FRONT].anim, SPRITE_CUR_FRONT, wk);
    } else {
        ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_FRONT], place, &pos);
        MCSS_SetPosition(wk->sprites[SPRITE_CUR_FRONT].mcss, &pos);
    }
    if (backSprite != SPRITE_NONE) {
        ZukanDetailForm_InitSpriteAnim(wk->sprites[SPRITE_CUR_BACK].anim, SPRITE_CUR_BACK, wk);
    } else {
        ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_BACK], place, &pos);
        MCSS_SetPosition(wk->sprites[SPRITE_CUR_BACK].mcss, &pos);
    }
    frontSprite = SPRITE_NEXT_FRONT;
    if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
        frontSprite = SPRITE_NONE;
    }
    backSprite = SPRITE_NEXT_BACK;
    if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
        backSprite = SPRITE_NONE;
    }
    ZukanDetailForm_CreateSprites(param, wk, common, frontSprite, backSprite, PLACE_RIGHT, wk->next);
    if (frontSprite != SPRITE_NONE) {
        ZukanDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_FRONT].anim, SPRITE_NEXT_FRONT, wk);
    } else {
        ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_NEXT_FRONT], PLACE_RIGHT, &pos);
        MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &pos);
    }
    if (backSprite != SPRITE_NONE) {
        ZukanDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_BACK].anim, SPRITE_NEXT_BACK, wk);
    } else {
        ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_NEXT_BACK], PLACE_RIGHT, &pos);
        MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &pos);
    }
    func_02019bcc(wk->mcssSys, wk->tcbMgr);
    if (wk->mode == FORM_MODE_SINGLE) {
        wk->slideFromRight = TRUE;
        ZukanDetailForm_PlaceNext(param, wk, common);
    }
}

static void ZukanDetailForm_ChangeNext(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                       ZukanDetailCommon *common) {
    int frontSprite;
    int backSprite;

    ZukanDetailForm_FreeSprite(&wk->sprites[SPRITE_NEXT_FRONT], wk->mcssSys);
    ZukanDetailForm_FreeSprite(&wk->sprites[SPRITE_NEXT_BACK], wk->mcssSys);
    frontSprite = wk->front ? SPRITE_NEXT_FRONT : SPRITE_NONE;
    backSprite = wk->front ? SPRITE_NONE : SPRITE_NEXT_BACK;
    ZukanDetailForm_CreateSprites(param, wk, common, frontSprite, backSprite, PLACE_RIGHT, wk->next);
    if (frontSprite != SPRITE_NONE) {
        ZukanDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_FRONT].anim, SPRITE_NEXT_FRONT, wk);
    }
    if (backSprite != SPRITE_NONE) {
        ZukanDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_BACK].anim, SPRITE_NEXT_BACK, wk);
    }
    ZukanDetailForm_PrintNext(param, wk, common);
    ZukanDetailForm_UpdateColorMarks(param, wk, common);
}

static void ZukanDetailForm_ChangeIcon(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                       ZukanDetailCommon *common) {
    if (wk->iconActors[wk->icon] != NULL) {
        func_0204c124(wk->iconActors[wk->icon], FALSE);
    }
    wk->icon = (wk->icon + 1) % 2;
    // Frees the other icon as the screen's exit does, testing it again
    if (wk->iconActors[wk->icon] != NULL) {
        if (wk->iconActors[wk->icon] != NULL) {
            ZukanDetailForm_FreeIcon(wk->iconChars[wk->icon], wk->iconActors[wk->icon]);
        }
        wk->iconActors[wk->icon] = NULL;
    }
    ZukanDetailForm_CreateCurIcon(param, wk, common, wk->cur);
}

static void ZukanDetailForm_CheckInput(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                       ZukanDetailCommon *common) {
    ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);

    switch (wk->mode) {
    case FORM_MODE_SINGLE:
        if (wk->inputEnabled) {
            FormButton input = ZukanDetailForm_GetButtonInput(param, wk, common);

            if (input != BUTTON_NONE) {
                switch (input) {
                case BUTTON_TURN:
                    ZukanDetailForm_Turn(param, wk, common);
                    break;
                case BUTTON_PLAY: {
                    u8 i;

                    ZukanDetailForm_LoadAllSprites(param, wk, common);
                    for (i = 0; i < SPRITE_COUNT; i++) {
                        ZukanDetailForm_PlaySpriteAnim(&wk->sprites[i]);
                    }
                    break;
                }
                case BUTTON_ARROW_L:
                    wk->slideFromRight = TRUE;
                    ZukanDetailForm_StepNext(param, wk, common);
                    ZukanDetailForm_SetMode(param, wk, common, FORM_MODE_SLIDE);
                    break;
                case BUTTON_ARROW_R:
                    wk->slideFromRight = FALSE;
                    ZukanDetailForm_StepNext(param, wk, common);
                    ZukanDetailForm_SetMode(param, wk, common, FORM_MODE_SLIDE);
                    break;
                }
            } else {
                BOOL open = FALSE;
                BOOL touch;

                if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
                    open = TRUE;
                    touch = FALSE;
                } else {
                    u32 x;
                    u32 y;

                    if (func_0203dac8(&x, &y) && y < 168) {
                        open = TRUE;
                        touch = TRUE;
                    }
                }
                if (open && wk->count >= 2) {
                    if ((wk->front && wk->sprites[SPRITE_NEXT_FRONT].mcss == NULL) ||
                        (!wk->front && wk->sprites[SPRITE_NEXT_BACK].mcss == NULL)) {
                        ZukanDetailForm_LoadNext(param, wk, common);
                        wk->slideFromRight = TRUE;
                        ZukanDetailForm_PlaceNext(param, wk, common);
                    }
                    ZukanDetailForm_SetMode(param, wk, common, FORM_MODE_OPEN);
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                    func_0203d564(touch);
                    ZukanDetailTouchbar_SetActive(touchbar, FALSE);
                }
            }
        }
        break;
    case FORM_MODE_COMPARE:
        if (!ZukanDetailTouchbar_IsFormButtonTriggered(touchbar) && wk->inputEnabled && !wk->dragging) {
            FormButton input = ZukanDetailForm_GetButtonInput(param, wk, common);

            if (input != BUTTON_NONE) {
                switch (input) {
                case BUTTON_TURN:
                    ZukanDetailForm_Turn(param, wk, common);
                    break;
                case BUTTON_PLAY: {
                    u8 i;

                    ZukanDetailForm_LoadAllSprites(param, wk, common);
                    for (i = 0; i < SPRITE_COUNT; i++) {
                        ZukanDetailForm_PlaySpriteAnim(&wk->sprites[i]);
                    }
                    break;
                }
                }
            }
        }
        break;
    }
}

static void ZukanDetailForm_SetMode(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common,
                                    int mode) {
    ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);
    ZukanDetailHeadbar *headbar = ZukanDetailCommon_GetHeadbar(common);
    BOOL changed = FALSE;

    switch (wk->mode) {
    case FORM_MODE_SINGLE:
        switch (mode) {
        case FORM_MODE_SLIDE:
            if (wk->slide == 0) {
                ZukanDetailForm_StopSpriteAnim(&wk->sprites[SPRITE_NEXT_FRONT]);
                ZukanDetailForm_StopSpriteAnim(&wk->sprites[SPRITE_NEXT_BACK]);
                wk->slide = 1;
            }
            break;
        case FORM_MODE_OPEN:
            wk->curInPlace = FALSE;
            wk->nextInPlace = FALSE;
            if (wk->count >= 2) {
                wk->slideFromRight = TRUE;
                ZukanDetailForm_PlaceNext(param, wk, common);
            }
            break;
        }
        break;
    case FORM_MODE_SLIDE:
        if (mode == FORM_MODE_SINGLE && wk->pushed == BUTTON_NONE) {
            ZukanDetailTouchbar_SetActive(touchbar, TRUE);
        }
        break;
    case FORM_MODE_OPEN:
        if (mode == FORM_MODE_OPENED) {
            ZukanDetailTouchbar_SetVisibleAll(touchbar, FALSE);
        }
        break;
    case FORM_MODE_OPENED:
        if (mode == FORM_MODE_COMPARE) {
            VecFx32 pos;

            changed = TRUE;
            if (wk->count >= 2) {
                if (wk->sprites[SPRITE_CUR_FRONT].mcss != NULL) {
                    ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_FRONT], PLACE_LEFT, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_CUR_FRONT].mcss, &pos);
                }
                if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
                    ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_BACK], PLACE_LEFT, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_CUR_BACK].mcss, &pos);
                }
                if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
                    ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_NEXT_FRONT], PLACE_RIGHT, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &pos);
                }
                if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
                    ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_NEXT_BACK], PLACE_RIGHT, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &pos);
                }
            }
            ZukanDetailTouchbar_SetType(touchbar, ZUKAN_DETAIL_TOUCHBAR_FORM, ZUKAN_DETAIL_PAGE_FORM - 1,
                                        ZukanDetailCommon_GetCount(common) > 1 ? TRUE : FALSE);
            if (wk->count >= 3) {
                ZukanDetailTouchbar_SetFormArrowsVisible(touchbar, TRUE);
            } else {
                ZukanDetailTouchbar_SetFormArrowsVisible(touchbar, FALSE);
            }
            ZukanDetailHeadbar_SetTitle(headbar, 4);
        }
        break;
    case FORM_MODE_COMPARE:
        switch (mode) {
        case FORM_MODE_SELECT:
            if (!wk->selectMoving) {
                wk->selectMoving = TRUE;
                wk->selectTime = 0;
            }
            break;
        case FORM_MODE_CLOSE:
            wk->curInPlace = FALSE;
            wk->nextInPlace = FALSE;
            break;
        }
        break;
    case FORM_MODE_SELECT:
        if (mode == FORM_MODE_COMPARE && wk->selectPending) {
            wk->selectMoved = TRUE;
            if (wk->selectButtonDone) {
                wk->selectPending = FALSE;
                ZukanDetailTouchbar_Unlock(touchbar);
            }
        }
        break;
    case FORM_MODE_CLOSE:
        if (mode == FORM_MODE_CLOSED) {
            ZukanDetailTouchbar_SetVisibleAll(touchbar, FALSE);
        }
        break;
    case FORM_MODE_CLOSED:
        if (mode == FORM_MODE_SINGLE) {
            VecFx32 pos;

            changed = TRUE;
            if (wk->count >= 2) {
                if (wk->sprites[SPRITE_CUR_FRONT].mcss != NULL) {
                    ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_FRONT], PLACE_SINGLE, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_CUR_FRONT].mcss, &pos);
                }
                if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
                    ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_BACK], PLACE_SINGLE, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_CUR_BACK].mcss, &pos);
                }
                ZukanDetailForm_GetBesidePos(wk->sprites[wk->front ? SPRITE_CUR_FRONT : SPRITE_CUR_BACK].mcss, &pos);
                if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
                    ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_NEXT_FRONT], PLACE_RIGHT, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &pos);
                }
                if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
                    ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_NEXT_BACK], PLACE_RIGHT, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &pos);
                }
            }
            ZukanDetailTouchbar_SetType(touchbar, ZUKAN_DETAIL_TOUCHBAR_GENERAL, ZUKAN_DETAIL_PAGE_FORM - 1,
                                        ZukanDetailCommon_GetCount(common) > 1 ? TRUE : FALSE);
            ZukanDetailTouchbar_SetCheck(
                touchbar, GameData_IsShortcutRegistered(ZukanDetailCommon_GetGameData(common), SHORTCUT_POKEDEX_FORM));
            ZukanDetailHeadbar_SetTitle(headbar, 3);
            if (wk->count >= 2) {
                wk->slideFromRight = TRUE;
                ZukanDetailForm_PlaceNext(param, wk, common);
            }
        }
        break;
    }

    wk->mode = mode;
    if (changed) {
        ZukanDetailForm_ScrollNextBG(param, wk, common);
        ZukanDetailForm_PrintNext(param, wk, common);
        ZukanDetailForm_UpdateColorMarks(param, wk, common);
        ZukanDetailForm_UpdateSlider(param, wk, common);
        ZukanDetailForm_UpdateArrows(param, wk, common);
        wk->pushed = BUTTON_NONE;
        ZukanDetailTouchbar_SetActive(touchbar, TRUE);
    }
}

static void ZukanDetailForm_CreateActors(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                         ZukanDetailCommon *common) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, param->heapId);
    u8 i;

    wk->resources[ZUKAN_DETAIL_FORM_RES_MAIN_PALETTE] = func_0204bbb8(arc, 3, CLACT_VRAM_MAIN, 0, 0, 3, param->heapId);
    wk->resources[ZUKAN_DETAIL_FORM_RES_MAIN_CHARS] = func_0204b81c(arc, 13, FALSE, CLACT_VRAM_MAIN, param->heapId);
    wk->resources[ZUKAN_DETAIL_FORM_RES_MAIN_CELL_ANIMS] = func_0204bde0(arc, 28, 45, param->heapId);
    wk->resources[ZUKAN_DETAIL_FORM_RES_COLOR_PALETTE] =
        func_0204bbb8(arc, 0, CLACT_VRAM_MAIN, 0x60, 0, 1, param->heapId);
    wk->resources[ZUKAN_DETAIL_FORM_RES_COLOR_CHARS] = func_0204b81c(arc, 10, FALSE, CLACT_VRAM_MAIN, param->heapId);
    wk->resources[ZUKAN_DETAIL_FORM_RES_COLOR_CELL_ANIMS] = func_0204bde0(arc, 27, 44, param->heapId);
    wk->resources[ZUKAN_DETAIL_FORM_RES_ARROW_PALETTE] =
        func_0204bbb8(arc, 6, CLACT_VRAM_MAIN, 0x80, 0, 2, param->heapId);
    wk->resources[ZUKAN_DETAIL_FORM_RES_ARROW_CHARS] = func_0204b81c(arc, 16, FALSE, CLACT_VRAM_MAIN, param->heapId);
    wk->resources[ZUKAN_DETAIL_FORM_RES_ARROW_CELL_ANIMS] = func_0204bde0(arc, 30, 47, param->heapId);
    GFL_ArcToolFree(arc);

    for (i = 0; i < ACTOR_COUNT; i++) {
        ClActorSetup setup;

        sys_memset(&setup, 0, sizeof(ClActorSetup));
        setup.x = ZUKAN_DETAIL_FORM_ACTORS[i].x;
        setup.y = ZUKAN_DETAIL_FORM_ACTORS[i].y;
        setup.sequence = ZUKAN_DETAIL_FORM_ACTORS[i].sequence;
        setup.priority = ZUKAN_DETAIL_FORM_ACTORS[i].priority;
        setup.bgPriority = ZUKAN_DETAIL_FORM_ACTORS[i].bgPriority;
        wk->actors[i] = func_0204c040(wk->unit, wk->resources[ZUKAN_DETAIL_FORM_ACTORS[i].chars],
                                      wk->resources[ZUKAN_DETAIL_FORM_ACTORS[i].palette],
                                      wk->resources[ZUKAN_DETAIL_FORM_ACTORS[i].cellAnims], &setup, CLACT_SURFACE_MAIN,
                                      param->heapId);
        func_0204c520(wk->actors[i], TRUE);
        func_0204c124(wk->actors[i], FALSE);
        func_0204c318(wk->actors[i], 1);
    }

    wk->buttons[BUTTON_TURN].x = 64;
    wk->buttons[BUTTON_TURN].y = 152;
    wk->buttons[BUTTON_TURN].chars = ZUKAN_DETAIL_FORM_RES_MAIN_CHARS;
    wk->buttons[BUTTON_TURN].palette = ZUKAN_DETAIL_FORM_RES_MAIN_PALETTE;
    wk->buttons[BUTTON_TURN].cellAnims = ZUKAN_DETAIL_FORM_RES_MAIN_CELL_ANIMS;
    wk->buttons[BUTTON_TURN].rectX = 64;
    wk->buttons[BUTTON_TURN].rectY = 152;
    wk->buttons[BUTTON_TURN].rectWidth = 64;
    wk->buttons[BUTTON_TURN].rectHeight = 16;
    wk->buttons[BUTTON_TURN].activeAnim = 24;
    wk->buttons[BUTTON_TURN].pushedAnim = 26;
    wk->buttons[BUTTON_TURN].key = PAD_BUTTON_SELECT;
    wk->buttons[BUTTON_TURN].se = SEQ_SE_DECIDE1;
    wk->buttons[BUTTON_TURN].state = BUTTON_STATE_NONE;
    wk->buttons[BUTTON_TURN].actor = NULL;

    wk->buttons[BUTTON_PLAY].x = 128;
    wk->buttons[BUTTON_PLAY].y = 152;
    wk->buttons[BUTTON_PLAY].chars = ZUKAN_DETAIL_FORM_RES_MAIN_CHARS;
    wk->buttons[BUTTON_PLAY].palette = ZUKAN_DETAIL_FORM_RES_MAIN_PALETTE;
    wk->buttons[BUTTON_PLAY].cellAnims = ZUKAN_DETAIL_FORM_RES_MAIN_CELL_ANIMS;
    wk->buttons[BUTTON_PLAY].rectX = 128;
    wk->buttons[BUTTON_PLAY].rectY = 152;
    wk->buttons[BUTTON_PLAY].rectWidth = 64;
    wk->buttons[BUTTON_PLAY].rectHeight = 16;
    wk->buttons[BUTTON_PLAY].activeAnim = 23;
    wk->buttons[BUTTON_PLAY].pushedAnim = 25;
    wk->buttons[BUTTON_PLAY].key = PAD_BUTTON_START;
    wk->buttons[BUTTON_PLAY].se = SEQ_SE_DECIDE1;
    wk->buttons[BUTTON_PLAY].state = BUTTON_STATE_NONE;
    wk->buttons[BUTTON_PLAY].actor = NULL;

    wk->buttons[BUTTON_ARROW_L].x = 128;
    wk->buttons[BUTTON_ARROW_L].y = 96;
    wk->buttons[BUTTON_ARROW_L].chars = ZUKAN_DETAIL_FORM_RES_ARROW_CHARS;
    wk->buttons[BUTTON_ARROW_L].palette = ZUKAN_DETAIL_FORM_RES_ARROW_PALETTE;
    wk->buttons[BUTTON_ARROW_L].cellAnims = ZUKAN_DETAIL_FORM_RES_ARROW_CELL_ANIMS;
    wk->buttons[BUTTON_ARROW_L].rectX = 0;
    wk->buttons[BUTTON_ARROW_L].rectY = 0;
    wk->buttons[BUTTON_ARROW_L].rectWidth = 24;
    wk->buttons[BUTTON_ARROW_L].rectHeight = 16;
    wk->buttons[BUTTON_ARROW_L].activeAnim = 4;
    wk->buttons[BUTTON_ARROW_L].pushedAnim = 5;
    wk->buttons[BUTTON_ARROW_L].key = PAD_BUTTON_L;
    wk->buttons[BUTTON_ARROW_L].se = SEQ_SE_SELECT3;
    wk->buttons[BUTTON_ARROW_L].state = BUTTON_STATE_NONE;
    wk->buttons[BUTTON_ARROW_L].actor = NULL;

    wk->buttons[BUTTON_ARROW_R].x = 228;
    wk->buttons[BUTTON_ARROW_R].y = 96;
    wk->buttons[BUTTON_ARROW_R].chars = ZUKAN_DETAIL_FORM_RES_ARROW_CHARS;
    wk->buttons[BUTTON_ARROW_R].palette = ZUKAN_DETAIL_FORM_RES_ARROW_PALETTE;
    wk->buttons[BUTTON_ARROW_R].cellAnims = ZUKAN_DETAIL_FORM_RES_ARROW_CELL_ANIMS;
    wk->buttons[BUTTON_ARROW_R].rectX = 232;
    wk->buttons[BUTTON_ARROW_R].rectY = 0;
    wk->buttons[BUTTON_ARROW_R].rectWidth = 24;
    wk->buttons[BUTTON_ARROW_R].rectHeight = 16;
    wk->buttons[BUTTON_ARROW_R].activeAnim = 2;
    wk->buttons[BUTTON_ARROW_R].pushedAnim = 3;
    wk->buttons[BUTTON_ARROW_R].key = PAD_BUTTON_R;
    wk->buttons[BUTTON_ARROW_R].se = SEQ_SE_SELECT3;
    wk->buttons[BUTTON_ARROW_R].state = BUTTON_STATE_NONE;
    wk->buttons[BUTTON_ARROW_R].actor = NULL;

    for (i = 0; i < BUTTON_COUNT; i++) {
        ClActorSetup setup;

        sys_memset(&setup, 0, sizeof(ClActorSetup));
        setup.x = wk->buttons[i].x;
        setup.y = wk->buttons[i].y;
        setup.sequence = wk->buttons[i].activeAnim;
        setup.priority = 0;
        setup.bgPriority = 3;
        wk->buttons[i].actor =
            func_0204c040(wk->unit, wk->resources[wk->buttons[i].chars], wk->resources[wk->buttons[i].palette],
                          wk->resources[wk->buttons[i].cellAnims], &setup, CLACT_SURFACE_MAIN, param->heapId);
        func_0204c520(wk->buttons[i].actor, TRUE);
        func_0204c318(wk->buttons[i].actor, 1);
    }
    wk->pushed = BUTTON_NONE;
}

static void ZukanDetailForm_FreeActors(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                       ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < BUTTON_COUNT; i++) {
        func_0204c108(wk->buttons[i].actor);
    }
    for (i = 0; i < ACTOR_COUNT; i++) {
        func_0204c108(wk->actors[i]);
    }
    func_0204bcd0(wk->resources[ZUKAN_DETAIL_FORM_RES_MAIN_PALETTE]);
    func_0204b98c(wk->resources[ZUKAN_DETAIL_FORM_RES_MAIN_CHARS]);
    func_0204be64(wk->resources[ZUKAN_DETAIL_FORM_RES_MAIN_CELL_ANIMS]);
    func_0204bcd0(wk->resources[ZUKAN_DETAIL_FORM_RES_COLOR_PALETTE]);
    func_0204b98c(wk->resources[ZUKAN_DETAIL_FORM_RES_COLOR_CHARS]);
    func_0204be64(wk->resources[ZUKAN_DETAIL_FORM_RES_COLOR_CELL_ANIMS]);
    func_0204bcd0(wk->resources[ZUKAN_DETAIL_FORM_RES_ARROW_PALETTE]);
    func_0204b98c(wk->resources[ZUKAN_DETAIL_FORM_RES_ARROW_CHARS]);
    func_0204be64(wk->resources[ZUKAN_DETAIL_FORM_RES_ARROW_CELL_ANIMS]);
}

// The button pushed with its key or touched, which starts its animation
static FormButton ZukanDetailForm_GetButtonInput(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                                 ZukanDetailCommon *common) {
    ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);
    FormButton button = BUTTON_NONE;
    BOOL touch;
    u8 i;

    if (button == BUTTON_NONE) {
        for (i = 0; i < BUTTON_COUNT; i++) {
            if (func_0204c138(wk->buttons[i].actor)) {
                if (GCTX_HIDGetPressedKeys() & wk->buttons[i].key) {
                    touch = FALSE;
                    button = i;
                    break;
                }
                // The arrows repeat while their keys are held
                if ((i == BUTTON_ARROW_L || i == BUTTON_ARROW_R) && (GCTX_HIDGetTypedKeys() & wk->buttons[i].key)) {
                    touch = FALSE;
                    button = i;
                    break;
                }
            }
        }
    }
    if (button == BUTTON_NONE) {
        u32 x;
        u32 y;

        if (func_0203dac8(&x, &y)) {
            for (i = 0; i < BUTTON_COUNT; i++) {
                if (func_0204c138(wk->buttons[i].actor) && wk->buttons[i].rectX <= x &&
                    x < wk->buttons[i].rectX + wk->buttons[i].rectWidth && wk->buttons[i].rectY <= y &&
                    y < wk->buttons[i].rectY + wk->buttons[i].rectHeight) {
                    touch = TRUE;
                    button = i;
                    break;
                }
            }
        }
    }
    if (button != BUTTON_NONE) {
        if (wk->pushed != BUTTON_NONE) {
            button = BUTTON_BUSY;
        } else {
            int other;

            func_0203d564(touch);
            ZukanDetailTouchbar_SetActive(touchbar, FALSE);
            wk->buttons[button].state = BUTTON_STATE_PUSHED;
            func_0204c488(wk->buttons[button].actor, wk->buttons[button].pushedAnim);
            GFL_SndSEPlay(wk->buttons[button].se);
            // Pushing one arrow greys out the other
            other = BUTTON_NONE;
            if (button == BUTTON_ARROW_L) {
                other = BUTTON_ARROW_R;
            } else if (button == BUTTON_ARROW_R) {
                other = BUTTON_ARROW_L;
            }
            if (other != BUTTON_NONE) {
                func_0204c520(wk->buttons[other].actor, FALSE);
                func_0204c4d4(wk->buttons[other].actor, 0);
            }
            wk->pushed = button;
        }
    }
    return button;
}

static void ZukanDetailForm_UpdateArrows(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                         ZukanDetailCommon *common) {
    BOOL visible = FALSE;

    switch (wk->mode) {
    case FORM_MODE_SINGLE:
        if (wk->count >= 2) {
            visible = TRUE;
        }
        break;
    case FORM_MODE_COMPARE:
        break;
    }
    if (visible) {
        func_0204c124(wk->buttons[BUTTON_ARROW_L].actor, TRUE);
        func_0204c124(wk->buttons[BUTTON_ARROW_R].actor, TRUE);
    } else {
        func_0204c124(wk->buttons[BUTTON_ARROW_L].actor, FALSE);
        func_0204c124(wk->buttons[BUTTON_ARROW_R].actor, FALSE);
    }
}

static void ZukanDetailForm_UpdateButtons(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                          ZukanDetailCommon *common) {
    ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);

    if (wk->pushed != BUTTON_NONE) {
        switch (wk->buttons[wk->pushed].state) {
        case BUTTON_STATE_NONE:
            break;
        case BUTTON_STATE_PUSHED:
            wk->buttons[wk->pushed].state = BUTTON_STATE_ANIMATING;
            break;
        case BUTTON_STATE_ANIMATING:
            if (!func_0204c560(wk->buttons[wk->pushed].actor)) {
                wk->buttons[wk->pushed].state = BUTTON_STATE_DONE;
            }
            break;
        case BUTTON_STATE_DONE: {
            int other;

            func_0204c488(wk->buttons[wk->pushed].actor, wk->buttons[wk->pushed].activeAnim);
            wk->buttons[wk->pushed].state = BUTTON_STATE_NONE;
            other = BUTTON_NONE;
            if (wk->pushed == BUTTON_ARROW_L) {
                other = BUTTON_ARROW_R;
            } else if (wk->pushed == BUTTON_ARROW_R) {
                other = BUTTON_ARROW_L;
            }
            if (other != BUTTON_NONE && wk->buttons[other].state == BUTTON_STATE_NONE) {
                func_0204c520(wk->buttons[other].actor, TRUE);
                func_0204c488(wk->buttons[other].actor, wk->buttons[other].activeAnim);
            }
            wk->pushed = BUTTON_NONE;
            if (wk->mode == FORM_MODE_SINGLE || wk->mode == FORM_MODE_COMPARE) {
                ZukanDetailTouchbar_SetActive(touchbar, TRUE);
            }
            break;
        }
        }
    }
}

// Shows whether the entries shown are in their shiny colors
static void ZukanDetailForm_UpdateColorMarks(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                             ZukanDetailCommon *common) {
    MarkState showCur = MARK_SHOWN;
    MarkState showNext = MARK_HIDDEN;
    ClActorPos pos;

    switch (wk->mode) {
    case FORM_MODE_SINGLE:
        break;
    case FORM_MODE_COMPARE:
        if (wk->count >= 2) {
            showNext = MARK_SHOWN;
        }
        break;
    }
    if (showCur && showNext) {
        pos.x = 0;
        pos.y = 0;
        func_0204c140(wk->actors[ACTOR_COLOR_CUR], &pos, CLACT_SURFACE_MAIN);
        pos.x = 128;
        pos.y = 0;
        func_0204c140(wk->actors[ACTOR_COLOR_NEXT], &pos, CLACT_SURFACE_MAIN);
    } else if (showCur) {
        pos.x = 64;
        pos.y = 0;
        func_0204c140(wk->actors[ACTOR_COLOR_CUR], &pos, CLACT_SURFACE_MAIN);
    }
    {
        u8 anim = wk->entries[wk->cur].color == ENTRY_COLOR_RARE ? 1 : 0;

        func_0204c124(wk->actors[ACTOR_COLOR_CUR], TRUE);
        func_0204c488(wk->actors[ACTOR_COLOR_CUR], anim);
    }
    if (showNext) {
        u8 anim = wk->entries[wk->next].color == ENTRY_COLOR_RARE ? 1 : 0;

        func_0204c124(wk->actors[ACTOR_COLOR_NEXT], TRUE);
        func_0204c488(wk->actors[ACTOR_COLOR_NEXT], anim);
    } else {
        func_0204c124(wk->actors[ACTOR_COLOR_NEXT], FALSE);
    }
}

static void ZukanDetailForm_UpdateSlider(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                         ZukanDetailCommon *common) {
    BOOL visible = FALSE;

    switch (wk->mode) {
    case FORM_MODE_SINGLE:
        break;
    case FORM_MODE_COMPARE:
        if (wk->count >= 3) {
            visible = TRUE;
        }
        break;
    }
    if (visible) {
        u8 x;
        ClActorPos pos;

        ZukanDetailForm_GetSliderSpan(wk->count - 1, ZukanDetailForm_GetSliderPos(param, wk, common, wk->cur, wk->next),
                                      NULL, &x, NULL);
        pos.x = x;
        pos.y = SLIDER_KNOB_Y;
        func_0204c140(wk->actors[ACTOR_SLIDER_KNOB], &pos, CLACT_SURFACE_MAIN);
        func_0204c124(wk->actors[ACTOR_SLIDER_BAR], TRUE);
        func_0204c124(wk->actors[ACTOR_SLIDER_KNOB], TRUE);
        wk->sliderSnap = FALSE;
        wk->sliderActive = FALSE;
        wk->dragging = FALSE;
    } else {
        func_0204c124(wk->actors[ACTOR_SLIDER_BAR], FALSE);
        func_0204c124(wk->actors[ACTOR_SLIDER_KNOB], FALSE);
    }
}

static void ZukanDetailForm_CheckSlider(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                        ZukanDetailCommon *common) {
    if (!ZukanDetailTouchbar_IsFormButtonTriggered(ZukanDetailCommon_GetTouchbar(common)) &&
        ((wk->mode == FORM_MODE_COMPARE && wk->inputEnabled && wk->pushed == BUTTON_NONE) || wk->sliderActive)) {
        if (!ZukanDetailForm_SnapSlider(param, wk, common)) {
            ZukanDetailForm_DragSlider(param, wk, common);
        }
    }
}

// Picks the second entry with the slider's knob
static BOOL ZukanDetailForm_DragSlider(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                       ZukanDetailCommon *common) {
    ZukanDetailTouchbar *touchbar = ZukanDetailCommon_GetTouchbar(common);
    BOOL moved = FALSE;
    u32 x;
    u32 y;

    if (wk->dragging) {
        if (!func_0203da84(&x, &y)) {
            wk->dragging = FALSE;
            ZukanDetailTouchbar_SetActive(touchbar, TRUE);
        }
    } else if (wk->count >= 3) {
        if (func_0203dac8(&x, &y) && x >= 144 && x <= 231 && y >= 40 && y <= 63) {
            wk->dragging = TRUE;
            ZukanDetailTouchbar_SetActive(touchbar, FALSE);
            moved = TRUE;
        }
    }
    if (wk->dragging) {
        u16 next = wk->next;
        ClActorPos pos;

        x = MATH_CLAMP(x, SLIDER_X, SLIDER_X + SLIDER_WIDTH - 1);
        pos.x = x;
        pos.y = SLIDER_KNOB_Y;
        func_0204c140(wk->actors[ACTOR_SLIDER_KNOB], &pos, CLACT_SURFACE_MAIN);
        wk->next = ZukanDetailForm_GetSliderEntry(param, wk, common, wk->cur,
                                                  ZukanDetailForm_GetSliderIndex(wk->count - 1, x));
        if (next != wk->next) {
            ZukanDetailForm_ChangeNext(param, wk, common);
            moved = TRUE;
        }
    }
    if (moved) {
        GFL_SndSEPlay(SEQ_SE_SYS_06);
    }
    if (wk->dragging) {
        func_0203d564(TRUE);
    }
    return wk->dragging;
}

static BOOL ZukanDetailForm_SnapSlider(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                       ZukanDetailCommon *common) {
    BOOL snapped = FALSE;

    if (wk->sliderSnap) {
        u8 x;
        ClActorPos pos;

        ZukanDetailForm_GetSliderSpan(wk->count - 1, ZukanDetailForm_GetSliderPos(param, wk, common, wk->cur, wk->next),
                                      NULL, &x, NULL);
        pos.x = x;
        pos.y = SLIDER_KNOB_Y;
        func_0204c140(wk->actors[ACTOR_SLIDER_KNOB], &pos, CLACT_SURFACE_MAIN);
        snapped = TRUE;
    }
    return snapped;
}

// The span of the slider for one of count entries, and the knob's place in it
static void ZukanDetailForm_GetSliderSpan(u16 count, u16 index, u8 *start, u8 *center, u8 *end) {
    u8 s = index * SLIDER_WIDTH / count + SLIDER_X;
    u8 e = (index + 1) * SLIDER_WIDTH / count + SLIDER_X;
    u8 c;

    if (index == 0) {
        c = s;
    } else if (index == count - 1) {
        c = e - 1;
    } else if (s >= e) {
        c = s;
    } else {
        c = (s + e - 1) / 2;
    }
    if (start != NULL) {
        *start = s;
    }
    if (end != NULL) {
        *end = e;
    }
    if (center != NULL) {
        *center = c;
    }
}

static u16 ZukanDetailForm_GetSliderIndex(u16 count, u8 x) {
    u16 i;
    u8 start;
    u8 center;
    u8 end;

    for (i = 0; i < count; i++) {
        ZukanDetailForm_GetSliderSpan(count, i, &start, &center, &end);
        if (i == 0) {
            if (x < end) {
                break;
            }
        } else if (i == count - 1) {
            break;
        } else if (start <= x && x < end) {
            break;
        }
    }
    return i;
}

// The second entry's place on the slider, which skips the entry in front
static u16 ZukanDetailForm_GetSliderPos(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common,
                                        u16 cur, u16 next) {
    u16 pos = 0;
    u16 i;

    for (i = 0; i < wk->count; i++) {
        if (i == cur) {
            continue;
        }
        if (i == next) {
            break;
        }
        pos++;
    }
    return pos;
}

static u16 ZukanDetailForm_GetSliderEntry(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                          ZukanDetailCommon *common, u16 cur, u16 pos) {
    u16 entry = 0;
    u16 n = 0;
    u16 i;

    for (i = 0; i < wk->count; i++) {
        if (i != cur) {
            if (n == pos) {
                entry = i;
                break;
            }
            n++;
        }
    }
    return entry;
}

// The form button's move: the two entries swap places along half a turn, up and down, and the second one becomes the
// one that the Pokédex shows
static void ZukanDetailForm_UpdateSelect(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                         ZukanDetailCommon *common) {
    if (wk->selectMoving == TRUE) {
        wk->selectTime += SELECT_STEP;
        if (wk->selectTime >= SELECT_TIME) {
            wk->selectTime = SELECT_TIME;
            wk->selectMoving = FALSE;
            ZukanDetailForm_SetMode(param, wk, common, FORM_MODE_COMPARE);
            if (wk->count >= 2) {
                VecFx32 pos;

                if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
                    ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_NEXT_FRONT], PLACE_LEFT, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &pos);
                }
                if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
                    ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_NEXT_BACK], PLACE_LEFT, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &pos);
                }
                ZukanDetailForm_GetBesidePos(wk->sprites[wk->front ? SPRITE_NEXT_FRONT : SPRITE_NEXT_BACK].mcss, &pos);
                if (wk->sprites[SPRITE_CUR_FRONT].mcss != NULL) {
                    ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_FRONT], PLACE_RIGHT, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_CUR_FRONT].mcss, &pos);
                }
                if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
                    ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_BACK], PLACE_RIGHT, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_CUR_BACK].mcss, &pos);
                }
                ZukanDetailForm_SwapCurNext(param, wk, common);
                ZukanDetailForm_ChangeIcon(param, wk, common);
                ZukanDetailForm_PrintCur(param, wk, common);
                ZukanDetailForm_PrintNext(param, wk, common);
                ZukanDetailForm_UpdateColorMarks(param, wk, common);
                {
                    u16 species = ZukanDetailCommon_GetSpecies(common);
                    PokeDexSave *pokedex = GameData_GetPokedex(ZukanDetailCommon_GetGameData(common));
                    u8 sex = wk->entries[wk->cur].sex;
                    BOOL rare = wk->entries[wk->cur].rare ? TRUE : FALSE;

                    addToDex(pokedex, species, sex, rare, wk->entries[wk->cur].form);
                }
                if (wk->count >= 3) {
                    wk->sliderSnap = TRUE;
                }
            }
        } else {
            f32 sin = (f32)FX_SinIdx(wk->selectTime) / FX32_ONE;
            ZukanDetailFormPos from;
            ZukanDetailFormPos to;
            VecFx32 pos;

            if (wk->sprites[SPRITE_CUR_FRONT].mcss != NULL) {
                f32 x;
                f32 y;

                ZukanDetailForm_GetSpritePosF32(&wk->sprites[SPRITE_CUR_FRONT], POS_LEFT, &from);
                ZukanDetailForm_GetSpritePosF32(&wk->sprites[SPRITE_CUR_FRONT], POS_RIGHT, &to);
                x = from.x + (f32)wk->selectTime * (to.x - from.x) / SELECT_TIME;
                y = from.y + 16.0f * sin;
                pos.x = FX32_CONST(x);
                pos.y = FX32_CONST(y);
                pos.z = FX32_CONST(from.z);
                MCSS_SetPosition(wk->sprites[SPRITE_CUR_FRONT].mcss, &pos);
            }
            if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
                f32 x;
                f32 y;

                ZukanDetailForm_GetSpritePosF32(&wk->sprites[SPRITE_CUR_BACK], POS_LEFT, &from);
                ZukanDetailForm_GetSpritePosF32(&wk->sprites[SPRITE_CUR_BACK], POS_RIGHT, &to);
                x = from.x + (f32)wk->selectTime * (to.x - from.x) / SELECT_TIME;
                y = from.y + 16.0f * sin;
                pos.x = FX32_CONST(x);
                pos.y = FX32_CONST(y);
                pos.z = FX32_CONST(from.z);
                MCSS_SetPosition(wk->sprites[SPRITE_CUR_BACK].mcss, &pos);
            }
            {
                ZukanDetailFormPos nextTo;
                ZukanDetailFormPos nextFrom;
                VecFx32 nextPos;

                if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
                    f32 x;
                    f32 y;

                    ZukanDetailForm_GetSpritePosF32(&wk->sprites[SPRITE_NEXT_FRONT], POS_LEFT, &nextTo);
                    ZukanDetailForm_GetSpritePosF32(&wk->sprites[SPRITE_NEXT_FRONT], POS_RIGHT, &nextFrom);
                    x = nextFrom.x + (f32)wk->selectTime * (nextTo.x - nextFrom.x) / SELECT_TIME;
                    y = nextFrom.y + -16.0f * sin;
                    nextPos.x = FX32_CONST(x);
                    nextPos.y = FX32_CONST(y);
                    nextPos.z = FX32_CONST(nextFrom.z);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &nextPos);
                }
                if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
                    f32 x;
                    f32 y;

                    ZukanDetailForm_GetSpritePosF32(&wk->sprites[SPRITE_NEXT_BACK], POS_LEFT, &nextTo);
                    ZukanDetailForm_GetSpritePosF32(&wk->sprites[SPRITE_NEXT_BACK], POS_RIGHT, &nextFrom);
                    x = nextFrom.x + (f32)wk->selectTime * (nextTo.x - nextFrom.x) / SELECT_TIME;
                    y = nextFrom.y + -16.0f * sin;
                    nextPos.x = FX32_CONST(x);
                    nextPos.y = FX32_CONST(y);
                    nextPos.z = FX32_CONST(nextFrom.z);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &nextPos);
                }
            }
        }
    }
}

// The second entry becomes the next or previous one
static void ZukanDetailForm_StepNext(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common) {
    u16 next = wk->next;

    if (!wk->slideFromRight) {
        if (wk->count >= 3) {
            wk->next = wk->cur + wk->count - 1;
            wk->next %= wk->count;
        }
    } else if (wk->count >= 3) {
        wk->next = wk->cur + 1;
        wk->next %= wk->count;
    }
    if ((wk->front && wk->sprites[SPRITE_NEXT_FRONT].mcss == NULL) ||
        (!wk->front && wk->sprites[SPRITE_NEXT_BACK].mcss == NULL) || wk->next != next) {
        int frontSprite;
        int backSprite;

        ZukanDetailForm_FreeSprite(&wk->sprites[SPRITE_NEXT_FRONT], wk->mcssSys);
        ZukanDetailForm_FreeSprite(&wk->sprites[SPRITE_NEXT_BACK], wk->mcssSys);
        frontSprite = wk->front ? SPRITE_NEXT_FRONT : SPRITE_NONE;
        backSprite = wk->front ? SPRITE_NONE : SPRITE_NEXT_BACK;
        ZukanDetailForm_CreateSprites(param, wk, common, frontSprite, backSprite, PLACE_RIGHT, wk->next);
        if (frontSprite != SPRITE_NONE) {
            ZukanDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_FRONT].anim, SPRITE_NEXT_FRONT, wk);
        }
        if (backSprite != SPRITE_NONE) {
            ZukanDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_BACK].anim, SPRITE_NEXT_BACK, wk);
        }
    }
    ZukanDetailForm_PlaceNext(param, wk, common);
}

// Places the second entry off the screen, where it slides in from
static void ZukanDetailForm_PlaceNext(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common) {
    VecFx32 pos;

    if (!wk->slideFromRight) {
        pos.x = FX32_CONST(-64);
        pos.y = FX32_CONST(-13.9);
        pos.z = 0;
        if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
            ZukanDetailForm_GetSpritePos(&wk->sprites[SPRITE_NEXT_FRONT], POS_LEFT_OUT, &pos);
            MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &pos);
        }
        if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
            ZukanDetailForm_GetSpritePos(&wk->sprites[SPRITE_NEXT_BACK], POS_LEFT_OUT, &pos);
            MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &pos);
        }
    } else {
        pos.x = FX32_CONST(64);
        pos.y = FX32_CONST(-13.9);
        pos.z = 0;
        if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
            ZukanDetailForm_GetSpritePos(&wk->sprites[SPRITE_NEXT_FRONT], POS_RIGHT_OUT, &pos);
            MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &pos);
        }
        if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
            ZukanDetailForm_GetSpritePos(&wk->sprites[SPRITE_NEXT_BACK], POS_RIGHT_OUT, &pos);
            MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &pos);
        }
    }
}

// The arrows' slide: both entries move 2 a frame until the second one is in the middle
static void ZukanDetailForm_UpdateSlide(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                        ZukanDetailCommon *common) {
    if (wk->slide == 1) {
        VecFx32 pos;
        ZukanDetailFormPos target;
        BOOL done = FALSE;
        int sprite = wk->front ? SPRITE_NEXT_FRONT : SPRITE_NEXT_BACK;
        f32 targetX;
        f32 x;

        ZukanDetailForm_GetSpritePosF32(&wk->sprites[sprite], POS_CENTER, &target);
        targetX = target.x;
        MCSS_GetPosition(wk->sprites[sprite].mcss, &pos);
        x = (f32)pos.x / FX32_ONE;
        if (!wk->slideFromRight) {
            x += 2.0f;
            if (x >= targetX) {
                done = TRUE;
            }
        } else {
            x -= 2.0f;
            if (x <= targetX) {
                done = TRUE;
            }
        }
        if (done) {
            x = targetX;
            wk->slide = 2;
        }
        pos.x = FX32_CONST(x);
        if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
            MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &pos);
        }
        if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
            MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &pos);
        }

        MCSS_GetPosition(wk->sprites[wk->front ? SPRITE_CUR_FRONT : SPRITE_CUR_BACK].mcss, &pos);
        x = (f32)pos.x / FX32_ONE;
        if (!wk->slideFromRight) {
            x += 2.0f;
        } else {
            x -= 2.0f;
        }
        pos.x = FX32_CONST(x);
        if (wk->sprites[SPRITE_CUR_FRONT].mcss != NULL) {
            MCSS_SetPosition(wk->sprites[SPRITE_CUR_FRONT].mcss, &pos);
        }
        if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
            MCSS_SetPosition(wk->sprites[SPRITE_CUR_BACK].mcss, &pos);
        }
    } else if (wk->slide == 2) {
        BOOL rare = FALSE;

        ZukanDetailForm_SetMode(param, wk, common, FORM_MODE_SINGLE);
        ZukanDetailForm_SwapCurNext(param, wk, common);
        ZukanDetailForm_StopSpriteAnim(&wk->sprites[SPRITE_NEXT_FRONT]);
        ZukanDetailForm_StopSpriteAnim(&wk->sprites[SPRITE_NEXT_BACK]);
        ZukanDetailForm_ChangeIcon(param, wk, common);
        ZukanDetailForm_PrintCur(param, wk, common);
        ZukanDetailForm_PrintNext(param, wk, common);
        ZukanDetailForm_UpdateColorMarks(param, wk, common);
        {
            u16 species = ZukanDetailCommon_GetSpecies(common);
            PokeDexSave *pokedex = GameData_GetPokedex(ZukanDetailCommon_GetGameData(common));
            u8 sex = wk->entries[wk->cur].sex;

            if (wk->entries[wk->cur].rare) {
                rare = TRUE;
            }
            addToDex(pokedex, species, sex, rare, wk->entries[wk->cur].form);
        }
        if (wk->sprites[SPRITE_CUR_FRONT].mcss != NULL) {
            VecFx32 pos;

            ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_FRONT], PLACE_SINGLE, &pos);
            MCSS_SetPosition(wk->sprites[SPRITE_CUR_FRONT].mcss, &pos);
        }
        if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
            VecFx32 pos;

            ZukanDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_BACK], PLACE_SINGLE, &pos);
            MCSS_SetPosition(wk->sprites[SPRITE_CUR_BACK].mcss, &pos);
        }
        wk->slideFromRight = TRUE;
        ZukanDetailForm_StepNext(param, wk, common);
        wk->slide = 0;
    }
}

// The comparison opens with the entry in front moving left and the second one moving in from the right, and closes
// the other way
static void ZukanDetailForm_UpdateOpenClose(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                            ZukanDetailCommon *common) {
    if (wk->count < 2) {
        if (wk->mode == FORM_MODE_OPEN) {
            ZukanDetailForm_SetMode(param, wk, common, FORM_MODE_OPENED);
        } else if (wk->mode == FORM_MODE_CLOSE) {
            ZukanDetailForm_SetMode(param, wk, common, FORM_MODE_CLOSED);
        }
    } else if (wk->mode == FORM_MODE_OPEN) {
        VecFx32 pos;

        if (!wk->curInPlace) {
            int sprite = wk->front ? SPRITE_CUR_FRONT : SPRITE_CUR_BACK;
            ZukanDetailFormPos target;
            f32 targetX;
            f32 x;

            ZukanDetailForm_GetSpritePosF32(&wk->sprites[sprite], POS_LEFT, &target);
            targetX = target.x;
            MCSS_GetPosition(wk->sprites[sprite].mcss, &pos);
            x = (f32)pos.x / FX32_ONE - 0.5f;
            if (x <= targetX) {
                x = targetX;
                wk->curInPlace = TRUE;
            }
            pos.x = FX32_CONST(x);
            if (wk->sprites[SPRITE_CUR_FRONT].mcss != NULL) {
                MCSS_SetPosition(wk->sprites[SPRITE_CUR_FRONT].mcss, &pos);
            }
            if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
                MCSS_SetPosition(wk->sprites[SPRITE_CUR_BACK].mcss, &pos);
            }
            if (wk->curInPlace) {
                VecFx32 place;

                if (wk->sprites[SPRITE_CUR_FRONT].mcss != NULL) {
                    ZukanDetailForm_GetSpritePos(&wk->sprites[SPRITE_CUR_FRONT], POS_LEFT, &place);
                    MCSS_SetPosition(wk->sprites[SPRITE_CUR_FRONT].mcss, &place);
                }
                if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
                    ZukanDetailForm_GetSpritePos(&wk->sprites[SPRITE_CUR_BACK], POS_LEFT, &place);
                    MCSS_SetPosition(wk->sprites[SPRITE_CUR_BACK].mcss, &place);
                }
            }
        }
        if (!wk->nextInPlace) {
            int sprite = wk->front ? SPRITE_NEXT_FRONT : SPRITE_NEXT_BACK;
            ZukanDetailFormPos target;
            f32 targetX;
            f32 x;

            ZukanDetailForm_GetSpritePosF32(&wk->sprites[sprite], POS_RIGHT, &target);
            targetX = target.x;
            MCSS_GetPosition(wk->sprites[sprite].mcss, &pos);
            x = (f32)pos.x / FX32_ONE - 1.5f;
            if (x <= targetX) {
                x = targetX;
                wk->nextInPlace = TRUE;
            }
            pos.x = FX32_CONST(x);
            if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
                MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &pos);
            }
            if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
                MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &pos);
            }
            if (wk->nextInPlace) {
                VecFx32 place;

                if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
                    ZukanDetailForm_GetSpritePos(&wk->sprites[SPRITE_NEXT_FRONT], POS_RIGHT, &place);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &place);
                }
                if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
                    ZukanDetailForm_GetSpritePos(&wk->sprites[SPRITE_NEXT_BACK], POS_RIGHT, &place);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &place);
                }
            }
        }
        if (wk->curInPlace && wk->nextInPlace) {
            ZukanDetailForm_SetMode(param, wk, common, FORM_MODE_OPENED);
        }
    } else if (wk->mode == FORM_MODE_OPENED) {
        ZukanDetailForm_SetMode(param, wk, common, FORM_MODE_COMPARE);
    } else if (wk->mode == FORM_MODE_CLOSE) {
        VecFx32 pos;

        if (!wk->curInPlace) {
            int sprite = wk->front ? SPRITE_CUR_FRONT : SPRITE_CUR_BACK;
            ZukanDetailFormPos target;
            f32 targetX;
            f32 x;

            ZukanDetailForm_GetSpritePosF32(&wk->sprites[sprite], POS_CENTER, &target);
            targetX = target.x;
            MCSS_GetPosition(wk->sprites[sprite].mcss, &pos);
            x = (f32)pos.x / FX32_ONE;
            x += 0.5f;
            if (x >= targetX) {
                x = targetX;
                wk->curInPlace = TRUE;
            }
            pos.x = FX32_CONST(x);
            if (wk->sprites[SPRITE_CUR_FRONT].mcss != NULL) {
                MCSS_SetPosition(wk->sprites[SPRITE_CUR_FRONT].mcss, &pos);
            }
            if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
                MCSS_SetPosition(wk->sprites[SPRITE_CUR_BACK].mcss, &pos);
            }
            if (wk->curInPlace) {
                VecFx32 place;

                if (wk->sprites[SPRITE_CUR_FRONT].mcss != NULL) {
                    ZukanDetailForm_GetSpritePos(&wk->sprites[SPRITE_CUR_FRONT], POS_CENTER, &place);
                    MCSS_SetPosition(wk->sprites[SPRITE_CUR_FRONT].mcss, &place);
                }
                if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
                    ZukanDetailForm_GetSpritePos(&wk->sprites[SPRITE_CUR_BACK], POS_CENTER, &place);
                    MCSS_SetPosition(wk->sprites[SPRITE_CUR_BACK].mcss, &place);
                }
            }
        }
        if (!wk->nextInPlace) {
            int sprite = wk->front ? SPRITE_NEXT_FRONT : SPRITE_NEXT_BACK;
            ZukanDetailFormPos target;
            f32 targetX;
            f32 x;

            ZukanDetailForm_GetSpritePosF32(&wk->sprites[sprite], POS_RIGHT_OUT, &target);
            targetX = target.x;
            MCSS_GetPosition(wk->sprites[sprite].mcss, &pos);
            x = (f32)pos.x / FX32_ONE;
            x += 1.5f;
            if (x >= targetX) {
                x = targetX;
                wk->nextInPlace = TRUE;
            }
            pos.x = FX32_CONST(x);
            if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
                MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &pos);
            }
            if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
                MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &pos);
            }
            if (wk->nextInPlace) {
                VecFx32 place;

                if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
                    ZukanDetailForm_GetSpritePos(&wk->sprites[SPRITE_NEXT_FRONT], POS_RIGHT_OUT, &place);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &place);
                }
                if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
                    ZukanDetailForm_GetSpritePos(&wk->sprites[SPRITE_NEXT_BACK], POS_RIGHT_OUT, &place);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &place);
                }
            }
        }
        if (wk->curInPlace && wk->nextInPlace) {
            ZukanDetailForm_SetMode(param, wk, common, FORM_MODE_CLOSED);
        }
    } else if (wk->mode == FORM_MODE_CLOSED) {
        ZukanDetailForm_SetMode(param, wk, common, FORM_MODE_SINGLE);
    }
}

// Loads the second entry's sprite for the comparison
static void ZukanDetailForm_LoadNext(ZukanDetailFormParam *param, ZukanDetailFormWork *wk, ZukanDetailCommon *common) {
    if (wk->mode == FORM_MODE_SINGLE) {
        int frontSprite;
        int backSprite;

        ZukanDetailForm_FreeSprite(&wk->sprites[SPRITE_NEXT_FRONT], wk->mcssSys);
        ZukanDetailForm_FreeSprite(&wk->sprites[SPRITE_NEXT_BACK], wk->mcssSys);
        func_02019bcc(wk->mcssSys, NULL);
        frontSprite = wk->front ? SPRITE_NEXT_FRONT : SPRITE_NONE;
        backSprite = wk->front ? SPRITE_NONE : SPRITE_NEXT_BACK;
        ZukanDetailForm_CreateSprites(param, wk, common, frontSprite, backSprite, PLACE_RIGHT, wk->next);
        if (frontSprite != SPRITE_NONE) {
            ZukanDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_FRONT].anim, SPRITE_NEXT_FRONT, wk);
        }
        if (backSprite != SPRITE_NONE) {
            ZukanDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_BACK].anim, SPRITE_NEXT_BACK, wk);
        }
        func_02019bcc(wk->mcssSys, wk->tcbMgr);
    }
}

static void ZukanDetailForm_SwapCurNext(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                        ZukanDetailCommon *common) {
    u16 index = wk->cur;
    u8 i;

    wk->cur = wk->next;
    wk->next = index;
    {
        MCSS *mcss = wk->sprites[SPRITE_CUR_FRONT].mcss;
        SpriteAnim *anim = wk->sprites[SPRITE_CUR_FRONT].anim;

        wk->sprites[SPRITE_CUR_FRONT].mcss = wk->sprites[SPRITE_NEXT_FRONT].mcss;
        wk->sprites[SPRITE_NEXT_FRONT].mcss = mcss;
        wk->sprites[SPRITE_CUR_FRONT].anim = wk->sprites[SPRITE_NEXT_FRONT].anim;
        wk->sprites[SPRITE_NEXT_FRONT].anim = anim;
        if (wk->sprites[SPRITE_CUR_FRONT].anim != NULL) {
            wk->sprites[SPRITE_CUR_FRONT].anim->sprite = SPRITE_CUR_FRONT;
        }
        if (wk->sprites[SPRITE_NEXT_FRONT].anim != NULL) {
            wk->sprites[SPRITE_NEXT_FRONT].anim->sprite = SPRITE_NEXT_FRONT;
        }
    }
    {
        MCSS *mcss = wk->sprites[SPRITE_CUR_BACK].mcss;
        SpriteAnim *anim = wk->sprites[SPRITE_CUR_BACK].anim;

        wk->sprites[SPRITE_CUR_BACK].mcss = wk->sprites[SPRITE_NEXT_BACK].mcss;
        wk->sprites[SPRITE_NEXT_BACK].mcss = mcss;
        wk->sprites[SPRITE_CUR_BACK].anim = wk->sprites[SPRITE_NEXT_BACK].anim;
        wk->sprites[SPRITE_NEXT_BACK].anim = anim;
        if (wk->sprites[SPRITE_CUR_BACK].anim != NULL) {
            wk->sprites[SPRITE_CUR_BACK].anim->sprite = SPRITE_CUR_BACK;
        }
        if (wk->sprites[SPRITE_NEXT_BACK].anim != NULL) {
            wk->sprites[SPRITE_NEXT_BACK].anim->sprite = SPRITE_NEXT_BACK;
        }
    }
    for (i = 0; i < POS_COUNT; i++) {
        ZukanDetailFormPos front = wk->sprites[SPRITE_CUR_FRONT].pos[i];
        ZukanDetailFormPos back;

        wk->sprites[SPRITE_CUR_FRONT].pos[i] = wk->sprites[SPRITE_NEXT_FRONT].pos[i];
        wk->sprites[SPRITE_NEXT_FRONT].pos[i] = front;
        back = wk->sprites[SPRITE_CUR_BACK].pos[i];
        wk->sprites[SPRITE_CUR_BACK].pos[i] = wk->sprites[SPRITE_NEXT_BACK].pos[i];
        wk->sprites[SPRITE_NEXT_BACK].pos[i] = back;
    }
}

static void ZukanDetailForm_SetActorsOpaque(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                            ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < 2; i++) {
        if (wk->iconActors[i] != NULL) {
            func_0204c318(wk->iconActors[i], 0);
        }
    }
    func_0204c318(wk->actors[ACTOR_SLIDER_BAR], 0);
    func_0204c318(wk->actors[ACTOR_SLIDER_KNOB], 0);
    for (i = 0; i < BUTTON_COUNT; i++) {
        func_0204c318(wk->buttons[i].actor, 0);
    }
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 0,
                        GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 |
                            GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ | GX_BLEND_PLANEMASK_BD,
                        12, 4);
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG3,
                        GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 |
                            GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ | GX_BLEND_PLANEMASK_BD,
                        12, 4);
}

static void ZukanDetailForm_SetActorsBlended(ZukanDetailFormParam *param, ZukanDetailFormWork *wk,
                                             ZukanDetailCommon *common) {
    u8 i;

    for (i = 0; i < 2; i++) {
        if (wk->iconActors[i] != NULL) {
            func_0204c318(wk->iconActors[i], 1);
        }
    }
    func_0204c318(wk->actors[ACTOR_SLIDER_BAR], 1);
    func_0204c318(wk->actors[ACTOR_SLIDER_KNOB], 1);
    for (i = 0; i < BUTTON_COUNT; i++) {
        func_0204c318(wk->buttons[i].actor, 1);
    }
    ZukanDetailBlend_InitPlanes(wk->blendMain);
    ZukanDetailBlend_InitPlanes(wk->blendSub);
}

static void ZukanDetailForm_InitSpritePositions(ZukanDetailFormPos *pos, u32 species, u32 form, u32 sex, u32 rare,
                                                u32 a6, u32 back, u32 personality) {
    u8 i;
    u16 j;

    for (i = 0; i < POS_COUNT; i++) {
        pos[i] = ZUKAN_DETAIL_FORM_DEFAULT_POSITIONS[i];
    }
    for (j = 0; j < NELEMS(sZukanDetailFormSpritePositions); j++) {
        const SpritePositions *positions = &sZukanDetailFormSpritePositions[j];

        if (species == positions->species && form == positions->form && back == positions->back &&
            (positions->sex == 3 || sex == positions->sex)) {
            for (i = 0; i < POS_COUNT; i++) {
                pos[i] = positions->pos[i];
            }
            return;
        }
    }
}

static void ZukanDetailForm_GetSpritePos(Sprite *sprite, int pos, VecFx32 *out) {
    out->x = FX32_CONST(sprite->pos[pos].x);
    out->y = FX32_CONST(sprite->pos[pos].y);
    out->z = FX32_CONST(sprite->pos[pos].z);
}

static void ZukanDetailForm_GetPlacePos(Sprite *sprite, int place, VecFx32 *out) {
    int pos = POS_CENTER;

    switch (place) {
    case PLACE_SINGLE:
        break;
    case PLACE_LEFT:
        pos = POS_LEFT;
        break;
    case PLACE_RIGHT:
        pos = POS_RIGHT;
        break;
    }
    ZukanDetailForm_GetSpritePos(sprite, pos, out);
}

static void ZukanDetailForm_GetSpritePosF32(Sprite *sprite, int pos, ZukanDetailFormPos *out) {
    *out = sprite->pos[pos];
}
