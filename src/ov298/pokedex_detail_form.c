#include "types.h"
#include "app/pokedex_detail.h"
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
#include "save/pokedex_savedata.h"
#include "system/game_data.h"
#include "system/mcss.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/btl_server_flow_title.h"

// The detail screen's forms page, pokedex_detail_form.c, zukan_detail_form.c by the ROM's own string: the Pokémon's
// sprite on the top screen in each sex, color and form that the Pokédex has seen, one at a time or two side by side to
// compare them, with its icon, the names of the forms shown and a slider to pick the second one on the touch screen.
// The one in front is the one that the Pokédex shows of the species

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

// The places of PokedexDetailForm_GetPlacePos
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

// The lines of an entry's text: its name alone in the middle of the window, or its name above its color. A type of
// its own, so that MWCC keeps the color's line as a variable and narrows it at the call
typedef enum {
    ENTRY_LINE_TOP = 1,
    ENTRY_LINE_MIDDLE = 9,
    ENTRY_LINE_BOTTOM = 17,
} EntryLine;

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
// next entry. PokedexDetailForm_GetButtonInput also returns BUTTON_NONE, or BUTTON_BUSY while one plays its animation
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

typedef struct PokedexDetailFormWork PokedexDetailFormWork;

typedef struct {
    u32 species;
    u32 form;
    // 3 for any sex
    u32 sex;
    u32 rare;
    u32 a6;
    BOOL back;
    u32 personality;
    PokedexDetailFormPos pos[POS_COUNT];
} SpritePositions;

// The data of a sprite's animation callback, which stops it after ANIM_LOOPS loops
typedef struct {
    int sprite;
    PokedexDetailFormWork *wk;
    u8 loops;
    BOOL stop;
} SpriteAnim;

typedef struct {
    MCSS *mcss;
    SpriteAnim *anim;
    PokedexDetailFormPos pos[POS_COUNT];
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
    // POKEDEX_DETAIL_FORM_RES_*
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

struct PokedexDetailFormWork {
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
    PokedexDetailBackground *backgroundMain;
    PokedexDetailBackground *backgroundSub;
    u32 resources[POKEDEX_DETAIL_FORM_RES_COUNT];
    ClActor *actors[ACTOR_COUNT];
    Button buttons[BUTTON_COUNT];
    // The BUTTON_* playing its animation, or BUTTON_NONE
    int pushed;
    // Whether the slider's knob jumps to the second entry's place, and whether it is held
    BOOL sliderSnap;
    BOOL dragging;
    TCB *vblankTcb;
    PokedexDetailBlend *blendMain;
    PokedexDetailBlend *blendSub;
    PokedexDetailPalFade *palFade;
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

static BOOL PokedexDetailForm_Init(PokedexDetailProcSys *sys, int *seq, void *param_, void *work,
                                   PokedexDetailCommon *common);
static BOOL PokedexDetailForm_Exit(PokedexDetailProcSys *sys, int *seq, void *param, void *work,
                                   PokedexDetailCommon *common);
static BOOL PokedexDetailForm_Main(PokedexDetailProcSys *sys, int *seq, void *param, void *work,
                                   PokedexDetailCommon *common);
static void PokedexDetailForm_Command(PokedexDetailProcSys *sys, int *seq, void *param, void *work,
                                      PokedexDetailCommon *common, int command);
static void PokedexDetailForm_Draw(PokedexDetailProcSys *sys, int *seq, void *param, void *work,
                                   PokedexDetailCommon *common);
static void PokedexDetailForm_VBlank(TCB *tcb, void *data);
static void PokedexDetailForm_GetEntryText(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                           PokedexDetailCommon *common, StrBuf **name, StrBuf **color, BOOL *freeName,
                                           BOOL *freeColor, u16 index);
static void PokedexDetailForm_CreateText(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailForm_FreeText(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                       PokedexDetailCommon *common);
static void PokedexDetailForm_CreatePrintQueues(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                                PokedexDetailCommon *common);
static void PokedexDetailForm_FreePrintQueues(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                              PokedexDetailCommon *common);
static void PokedexDetailForm_UpdateText(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailForm_FlushWindow(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                          PokedexDetailCommon *common, u8 window);
static void PokedexDetailForm_PrintName(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                        PokedexDetailCommon *common);
static void PokedexDetailForm_PrintEntry(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                         PokedexDetailCommon *common, u8 window, PrintQueue *printQueue, BmpWin *bmpWin,
                                         BOOL *flushPending, u16 index);
static void PokedexDetailForm_PrintCur(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                       PokedexDetailCommon *common);
static void PokedexDetailForm_PrintNext(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                        PokedexDetailCommon *common);
static void PokedexDetailForm_ScrollNextBG(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                           PokedexDetailCommon *common);
static void PokedexDetailForm_CreateMCSS(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailForm_FreeMCSS(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                       PokedexDetailCommon *common);
static MCSS *PokedexDetailForm_AddMCSS(MCSSSystem *system, u32 species, u32 form, u32 sex, u32 rare, u32 a6, u32 back,
                                       u32 personality);
static void PokedexDetailForm_RemoveMCSS(MCSSSystem *system, MCSS *mcss);
static void PokedexDetailForm_SetShadow(MCSS *mcss);
static void PokedexDetailForm_GetBesidePos(MCSS *mcss, VecFx32 *pos);
static void PokedexDetailForm_CreateSprite(Sprite *sprite, HeapID heapId, MCSSSystem *system, u32 species, u32 form,
                                           u32 sex, u32 rare, u32 a6, u32 back, u32 personality);
static void PokedexDetailForm_FreeSprite(Sprite *sprite, MCSSSystem *system);
static void PokedexDetailForm_InitSpriteAnim(SpriteAnim *anim, int sprite, PokedexDetailFormWork *wk);
static void PokedexDetailForm_PlaySpriteAnim(Sprite *sprite);
static void PokedexDetailForm_StopSpriteAnim(Sprite *sprite);
static void PokedexDetailForm_UpdateSprite(Sprite *sprite);
static void PokedexDetailForm_SpriteAnimEnd(u32 param, fx32 frame);
static void PokedexDetailForm_Turn(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                   PokedexDetailCommon *common);
static void PokedexDetailForm_LoadIconResources(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                                PokedexDetailCommon *common);
static void PokedexDetailForm_FreeIconResources(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                                PokedexDetailCommon *common);
static ClActor *PokedexDetailForm_CreateIcon(ArcTool *arc, u32 *chars, u32 palette, u32 cellAnims, ClActUnit *unit,
                                             HeapID heapId, u32 species, u32 form, u32 sex, BOOL egg);
static void PokedexDetailForm_FreeIcon(u32 chars, ClActor *actor);
static void PokedexDetailForm_GatherEntries(PokedexDetailFormParam *param, PokedexDetailCommon *common,
                                            FormEntry *entries, u16 *count, u16 *nameCount, u16 *rareCount, u16 *cur,
                                            u16 *next);
static void PokedexDetailForm_LoadEntries(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                          PokedexDetailCommon *common);
static void PokedexDetailForm_CreateSprites(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                            PokedexDetailCommon *common, int frontSprite, int backSprite, int place,
                                            u16 index);
static void PokedexDetailForm_CreateCurIcon(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                            PokedexDetailCommon *common, u16 index);
static void PokedexDetailForm_ChangePokemon(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                            PokedexDetailCommon *common);
static void PokedexDetailForm_LoadTurnedSprites(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                                PokedexDetailCommon *common);
static void PokedexDetailForm_LoadAllSprites(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                             PokedexDetailCommon *common);
static void PokedexDetailForm_ChangeNext(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailForm_ChangeIcon(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailForm_CheckInput(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailForm_SetMode(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                      PokedexDetailCommon *common, int mode);
static void PokedexDetailForm_CreateActors(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                           PokedexDetailCommon *common);
static void PokedexDetailForm_FreeActors(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                         PokedexDetailCommon *common);
static FormButton PokedexDetailForm_GetButtonInput(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                                   PokedexDetailCommon *common);
static void PokedexDetailForm_UpdateArrows(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                           PokedexDetailCommon *common);
static void PokedexDetailForm_UpdateButtons(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                            PokedexDetailCommon *common);
static void PokedexDetailForm_UpdateColorMarks(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                               PokedexDetailCommon *common);
static void PokedexDetailForm_UpdateSlider(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                           PokedexDetailCommon *common);
static void PokedexDetailForm_CheckSlider(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                          PokedexDetailCommon *common);
static BOOL PokedexDetailForm_DragSlider(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                         PokedexDetailCommon *common);
static BOOL PokedexDetailForm_SnapSlider(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                         PokedexDetailCommon *common);
static void PokedexDetailForm_GetSliderSpan(u16 count, u16 index, u8 *start, u8 *center, u8 *end);
static u16 PokedexDetailForm_GetSliderIndex(u16 count, u8 x);
static u16 PokedexDetailForm_GetSliderPos(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                          PokedexDetailCommon *common, u16 cur, u16 next);
static u16 PokedexDetailForm_GetSliderEntry(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                            PokedexDetailCommon *common, u16 cur, u16 pos);
static void PokedexDetailForm_UpdateSelect(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                           PokedexDetailCommon *common);
static void PokedexDetailForm_StepNext(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                       PokedexDetailCommon *common);
static void PokedexDetailForm_PlaceNext(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                        PokedexDetailCommon *common);
static void PokedexDetailForm_UpdateSlide(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                          PokedexDetailCommon *common);
static void PokedexDetailForm_UpdateOpenClose(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                              PokedexDetailCommon *common);
static void PokedexDetailForm_LoadNext(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                       PokedexDetailCommon *common);
static void PokedexDetailForm_SwapCurNext(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                          PokedexDetailCommon *common);
static void PokedexDetailForm_SetActorsOpaque(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                              PokedexDetailCommon *common);
static void PokedexDetailForm_SetActorsBlended(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                               PokedexDetailCommon *common);
static void PokedexDetailForm_InitSpritePositions(PokedexDetailFormPos *pos, u32 species, u32 form, u32 sex, u32 rare,
                                                  u32 a6, u32 back, u32 personality);
static void PokedexDetailForm_GetSpritePos(Sprite *sprite, int pos, VecFx32 *out);
static void PokedexDetailForm_GetPlacePos(Sprite *sprite, int place, VecFx32 *out);
static void PokedexDetailForm_GetSpritePosF32(Sprite *sprite, int pos, PokedexDetailFormPos *out);

// Sprite positions, of which the code only uses the last: the offset from a sprite of the one beside it in the
// comparison
static VecFx32 sPokedexDetailFormOffsets[3] = {
    { 0, FX32_CONST(-13.9), 0 },
    { FX32_CONST(-16), FX32_CONST(-13.9), 0 },
    { FX32_CONST(32), 0, 0 },
};

// The sprites whose drawing is off center
static const SpritePositions sPokedexDetailFormSpritePositions[] = {
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

const PokedexDetailProcFuncs POKEDEX_DETAIL_FORM_PROC_FUNCS = {
    PokedexDetailForm_Init,    PokedexDetailForm_Main, PokedexDetailForm_Exit,
    PokedexDetailForm_Command, PokedexDetailForm_Draw,
};

void PokedexDetailForm_InitParam(PokedexDetailFormParam *param, HeapID heapId) {
    param->heapId = heapId;
}

static BOOL PokedexDetailForm_Init(PokedexDetailProcSys *sys, int *seq, void *param_, void *work,
                                   PokedexDetailCommon *common) {
    PokedexDetailFormParam *param = param_;
    PokedexDetailFormWork *wk = PokedexDetailProcSys_AllocWork(sys, sizeof(PokedexDetailFormWork), param->heapId);
    u8 i;

    MI_CpuFill8(wk, 0, sizeof(PokedexDetailFormWork));
    wk->unit = PokedexDetailGraphic_GetClActUnit(PokedexDetailCommon_GetGraphic(common));
    wk->font = PokedexDetailCommon_GetFont(common);
    for (i = 0; i < SPRITE_COUNT; i++) {
        wk->sprites[i].mcss = NULL;
        wk->sprites[i].anim = NULL;
    }
    for (i = 0; i < 2; i++) {
        wk->iconActors[i] = NULL;
    }
    wk->icon = 0;
    wk->front = TRUE;
    wk->vblankTcb = GFL_VBlankTCBAdd(PokedexDetailForm_VBlank, wk, 1);
    PokedexDetailForm_CreatePrintQueues(param, wk, common);
    wk->blendMain = PokedexDetailBlend_Create(param->heapId);
    wk->blendSub = PokedexDetailBlend_Create(param->heapId);
    PokedexDetailBlend_InitPlanes(wk->blendMain);
    PokedexDetailBlend_InitPlanes(wk->blendSub);
    PokedexDetailBlend_SetOut(0, wk->blendMain);
    PokedexDetailBlend_SetOut(1, wk->blendSub);
    wk->palFade = PokedexDetailPalFade_Create(param->heapId);
    wk->exit = FORM_EXIT_NONE;
    wk->inputEnabled = TRUE;
    wk->mode = FORM_MODE_SINGLE;
    wk->sliderActive = FALSE;
    wk->selectPending = FALSE;
    wk->selectMoved = TRUE;
    wk->selectButtonDone = TRUE;
    return TRUE;
}

static BOOL PokedexDetailForm_Exit(PokedexDetailProcSys *sys, int *seq, void *param_, void *work,
                                   PokedexDetailCommon *common) {
    PokedexDetailFormParam *param = param_;
    PokedexDetailFormWork *wk = work;
    PokedexDetailTouchbar *touchbar = PokedexDetailCommon_GetTouchbar(common);
    u8 i;

    PokedexDetailBackground_Free(wk->backgroundSub);
    PokedexDetailBackground_Free(wk->backgroundMain);
    PokedexDetail_FreeBG(7, wk->bgChars);
    PokedexDetailForm_FreeActors(param, wk, common);
    for (i = 0; i < 2; i++) {
        if (wk->iconActors[i] != NULL) {
            PokedexDetailForm_FreeIcon(wk->iconChars[i], wk->iconActors[i]);
        }
        wk->iconActors[i] = NULL;
    }
    PokedexDetailForm_FreeIconResources(param, wk, common);
    PokedexDetailForm_FreeText(param, wk, common);
    PokedexDetailTouchbar_SetBGPriority(touchbar, 0);
    PokedexDetailPalFade_Free(wk->palFade);
    PokedexDetailBlend_Free(wk->blendSub);
    PokedexDetailBlend_Free(wk->blendMain);
    PokedexDetailForm_FreePrintQueues(param, wk, common);
    GFL_TCBRemove(wk->vblankTcb);
    PokedexDetailProcSys_FreeWork(sys);
    return TRUE;
}

static BOOL PokedexDetailForm_Main(PokedexDetailProcSys *sys, int *seq, void *param_, void *work,
                                   PokedexDetailCommon *common) {
    PokedexDetailFormParam *param = param_;
    PokedexDetailFormWork *wk = work;
    PokedexDetailGraphic *graphic = PokedexDetailCommon_GetGraphic(common);
    PokedexDetailTouchbar *touchbar = PokedexDetailCommon_GetTouchbar(common);
    PokedexDetailHeadbar *headbar = PokedexDetailCommon_GetHeadbar(common);

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
        PokedexDetailTouchbar_SetBGPriority(touchbar, 2);
        GX_SetGraphicsMode(GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BG0_AS_3D);
        PokedexDetailGraphic_Create3D(graphic, param->heapId);
        PokedexDetailForm_CreateMCSS(param, wk, common);
        PokedexDetailForm_CreateText(param, wk, common);
        PokedexDetailForm_CreateActors(param, wk, common);
        PokedexDetailForm_LoadEntries(param, wk, common);
        PokedexDetailForm_LoadIconResources(param, wk, common);
        PokedexDetailForm_ChangePokemon(param, wk, common);
        ClActor_SetObjMode(wk->iconActors[wk->icon], 1);
        MCSS_Hide(wk->sprites[SPRITE_CUR_FRONT].mcss);
        MCSS_StartFade(wk->sprites[SPRITE_CUR_FRONT].mcss, 16, 16, 0, 0);
        if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
            MCSS_Hide(wk->sprites[SPRITE_NEXT_FRONT].mcss);
            MCSS_StartFade(wk->sprites[SPRITE_NEXT_FRONT].mcss, 16, 16, 0, 0);
        }
        wk->bgChars = PokedexDetail_LoadBG(FALSE, param->heapId, 7, 1, 1, 0, ARCID_ZUKAN_GRA, 2, 12, 36, 0);
        wk->backgroundMain = PokedexDetailBackground_Create(param->heapId, 3, 3, 1, 2);
        wk->backgroundSub = PokedexDetailBackground_Create(param->heapId, 3, 4, 2, 3);
        PokedexDetailPalFade_ReadPalettes(wk->palFade);
        PokedexDetailPalFade_SetHidden(wk->palFade);
        break;
    }
    case FORM_SEQ_FADE_IN:
        *seq = FORM_SEQ_WAIT_BLEND;
        PokedexDetailBlend_SetIn(0, wk->blendMain);
        PokedexDetailBlend_SetIn(1, wk->blendSub);
        PokedexDetailForm_SetActorsOpaque(param, wk, common);
        break;
    case FORM_SEQ_WAIT_BLEND:
        *seq = FORM_SEQ_START_PALFADE;
        break;
    case FORM_SEQ_START_PALFADE: {
        BOOL ready = TRUE;

        if (MCSS_IsFading(wk->sprites[SPRITE_CUR_FRONT].mcss)) {
            ready = FALSE;
        }
        if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL && MCSS_IsFading(wk->sprites[SPRITE_NEXT_FRONT].mcss)) {
            ready = FALSE;
        }
        if (ready) {
            *seq = FORM_SEQ_WAIT_FADE_IN;
            PokedexDetailPalFade_StartIn(wk->palFade);
            if (PokedexDetailTouchbar_GetState(touchbar) != POKEDEX_DETAIL_TOUCHBAR_SHOWN) {
                PokedexDetailTouchbar_SetType(touchbar, POKEDEX_DETAIL_TOUCHBAR_GENERAL, POKEDEX_DETAIL_PAGE_FORM - 1,
                                              PokedexDetailCommon_GetCount(common) > 1 ? TRUE : FALSE);
                PokedexDetailTouchbar_Appear(touchbar, 0);
            } else {
                PokedexDetailTouchbar_SetPage(touchbar, POKEDEX_DETAIL_PAGE_FORM - 1);
            }
            PokedexDetailTouchbar_SetActive(touchbar, FALSE);
            PokedexDetailTouchbar_SetCheck(
                touchbar,
                GameData_IsShortcutRegistered(PokedexDetailCommon_GetGameData(common), SHORTCUT_POKEDEX_FORM));
            if (PokedexDetailHeadbar_GetState(headbar) != POKEDEX_DETAIL_TOUCHBAR_SHOWN) {
                PokedexDetailHeadbar_SetTitle(headbar, 3);
                PokedexDetailHeadbar_Appear(headbar);
            }
            MCSS_Show(wk->sprites[SPRITE_CUR_FRONT].mcss);
            MCSS_StartFade(wk->sprites[SPRITE_CUR_FRONT].mcss, 16, 0, -2, 0);
            if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
                MCSS_Show(wk->sprites[SPRITE_NEXT_FRONT].mcss);
                MCSS_StartFade(wk->sprites[SPRITE_NEXT_FRONT].mcss, 16, 0, -2, 0);
            }
        }
        break;
    }
    case FORM_SEQ_WAIT_FADE_IN: {
        BOOL ready = TRUE;

        if (MCSS_IsFading(wk->sprites[SPRITE_CUR_FRONT].mcss)) {
            ready = FALSE;
        }
        if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL && MCSS_IsFading(wk->sprites[SPRITE_NEXT_FRONT].mcss)) {
            ready = FALSE;
        }
        if (!PokedexDetailPalFade_IsFading(wk->palFade) &&
            PokedexDetailTouchbar_GetState(touchbar) == POKEDEX_DETAIL_TOUCHBAR_SHOWN &&
            PokedexDetailHeadbar_GetState(headbar) == POKEDEX_DETAIL_TOUCHBAR_SHOWN && ready) {
            PokedexDetailTouchbar_Unlock(touchbar);
            PokedexDetailTouchbar_SetActive(touchbar, TRUE);
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
                    MCSS_StartFade(wk->sprites[i].mcss, 0, 0, 0, 0);
                }
            }
        } else {
            PokedexDetailForm_CheckInput(param, wk, common);
        }
        break;
    case FORM_SEQ_FADE_OUT_POKEMON: {
        BOOL ready = TRUE;
        u8 i;

        for (i = 0; i < SPRITE_COUNT; i++) {
            if (wk->sprites[i].mcss != NULL && MCSS_IsFading(wk->sprites[i].mcss)) {
                ready = FALSE;
                break;
            }
        }
        if (ready) {
            *seq = FORM_SEQ_WAIT_FADE_OUT;
            PokedexDetailPalFade_StartOut(wk->palFade);
            PokedexDetailHeadbar_Disappear(headbar);
            if (wk->exit == FORM_EXIT_SCREEN) {
                PokedexDetailTouchbar_Disappear(touchbar, 0);
            }
            for (i = 0; i < SPRITE_COUNT; i++) {
                if (wk->sprites[i].mcss != NULL) {
                    MCSS_StartFade(wk->sprites[i].mcss, 0, 16, -2, 0);
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
            if (wk->sprites[i].mcss != NULL && MCSS_IsFading(wk->sprites[i].mcss)) {
                ready = FALSE;
                break;
            }
        }
        if (!PokedexDetailPalFade_IsFading(wk->palFade) &&
            PokedexDetailHeadbar_GetState(headbar) == POKEDEX_DETAIL_TOUCHBAR_HIDDEN && ready) {
            if (wk->exit == FORM_EXIT_SCREEN) {
                if (PokedexDetailTouchbar_GetState(touchbar) == POKEDEX_DETAIL_TOUCHBAR_HIDDEN) {
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
        PokedexDetailForm_SetActorsBlended(param, wk, common);
        PokedexDetailBlend_SetOut(0, wk->blendMain);
        PokedexDetailBlend_SetOut(1, wk->blendSub);
        break;
    case FORM_SEQ_EXIT: {
        u8 i;

        for (i = 0; i < SPRITE_COUNT; i++) {
            PokedexDetailForm_FreeSprite(&wk->sprites[i], wk->mcssSys);
        }
        PokedexDetailForm_FreeMCSS(param, wk, common);
        PokedexDetailGraphic_Free3D(graphic);
        GX_SetGraphicsMode(GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BG0_AS_2D);
        return TRUE;
    }
    }

    if (wk->mcssSys != NULL) {
        u8 i;

        GFL_TCBMgrUpdate(wk->tcbMgr);
        MCSSSys_Update(wk->mcssSys);
        for (i = 0; i < SPRITE_COUNT; i++) {
            PokedexDetailForm_UpdateSprite(&wk->sprites[i]);
        }
    }
    if (*seq >= FORM_SEQ_START_PALFADE) {
        PokedexDetailForm_UpdateButtons(param, wk, common);
        PokedexDetailForm_UpdateSelect(param, wk, common);
        PokedexDetailForm_CheckSlider(param, wk, common);
        PokedexDetailForm_UpdateSlide(param, wk, common);
        PokedexDetailForm_UpdateOpenClose(param, wk, common);
        PokedexDetailBackground_Update(wk->backgroundMain);
        PokedexDetailBackground_Update(wk->backgroundSub);
    }
    PokedexDetailBlend_Update(wk->blendMain, wk->blendSub);
    PokedexDetailPalFade_Update(wk->palFade);
    PokedexDetailForm_UpdateText(param, wk, common);
    return FALSE;
}

// Moves the next entry's index around the entries
static inline void PokedexDetailForm_AddNext(PokedexDetailFormWork *wk, u16 add) {
    wk->next += add;
    wk->next %= wk->count;
}

static void PokedexDetailForm_Command(PokedexDetailProcSys *sys, int *seq, void *param, void *work,
                                      PokedexDetailCommon *common, int command) {
    PokedexDetailFormWork *wk = work;

    if (wk != NULL) {
        PokedexDetailTouchbar *touchbar = PokedexDetailCommon_GetTouchbar(common);
        BOOL unlock = FALSE;

        wk->sliderSnap = FALSE;
        wk->sliderActive = FALSE;

        // Input waits while the bar's icons play their animations
        switch (command) {
        case POKEDEX_DETAIL_CMD_CLOSE_TOUCH:
        case POKEDEX_DETAIL_CMD_RETURN_TOUCH:
        case POKEDEX_DETAIL_CMD_CUR_D_TOUCH:
        case POKEDEX_DETAIL_CMD_CUR_U_TOUCH:
        case POKEDEX_DETAIL_CMD_CHECK_TOUCH:
        case POKEDEX_DETAIL_CMD_INFO_TOUCH:
        case POKEDEX_DETAIL_CMD_MAP_TOUCH:
        case POKEDEX_DETAIL_CMD_VOICE_TOUCH:
        case POKEDEX_DETAIL_CMD_FORM_CUR_R_TOUCH:
        case POKEDEX_DETAIL_CMD_FORM_CUR_L_TOUCH:
        case POKEDEX_DETAIL_CMD_FORM_CUR_D_TOUCH:
        case POKEDEX_DETAIL_CMD_FORM_CUR_U_TOUCH:
        case POKEDEX_DETAIL_CMD_FORM_BUTTON_TOUCH:
            wk->inputEnabled = FALSE;
            break;
        }
        switch (command) {
        case POKEDEX_DETAIL_CMD_CLOSE:
        case POKEDEX_DETAIL_CMD_RETURN:
        case POKEDEX_DETAIL_CMD_CUR_D:
        case POKEDEX_DETAIL_CMD_CUR_U:
        case POKEDEX_DETAIL_CMD_CHECK:
        case POKEDEX_DETAIL_CMD_INFO:
        case POKEDEX_DETAIL_CMD_MAP:
        case POKEDEX_DETAIL_CMD_VOICE:
        case POKEDEX_DETAIL_CMD_FORM_CUR_R:
        case POKEDEX_DETAIL_CMD_FORM_CUR_L:
        case POKEDEX_DETAIL_CMD_FORM_CUR_D:
        case POKEDEX_DETAIL_CMD_FORM_CUR_U:
        case POKEDEX_DETAIL_CMD_FORM_BUTTON:
            unlock = TRUE;
            wk->inputEnabled = TRUE;
            break;
        }

        switch (command) {
        case POKEDEX_DETAIL_CMD_CUR_D_TOUCH:
        case POKEDEX_DETAIL_CMD_FORM_CUR_D_TOUCH:
            if (wk->mode == FORM_MODE_SINGLE) {
                u16 species = PokedexDetailCommon_GetSpecies(common);

                PokedexDetailCommon_GoNext(common);
                if (species != PokedexDetailCommon_GetSpecies(common)) {
                    PokedexDetailForm_ChangePokemon(param, wk, common);
                }
            } else if (wk->mode == FORM_MODE_COMPARE) {
                // Only Pokémon with two entries or more to compare
                FormEntry entries[ENTRY_MAX];
                u16 count;
                u16 nameCount;
                u16 rareCount;
                u16 cur;
                u16 next;
                u16 species = PokedexDetailCommon_GetSpecies(common);
                u16 newSpecies;

                PokedexDetailCommon_GoNext(common);
                newSpecies = PokedexDetailCommon_GetSpecies(common);
                while (species != newSpecies) {
                    PokedexDetailForm_GatherEntries(param, common, entries, &count, &nameCount, &rareCount, &cur,
                                                    &next);
                    if (count >= 2) {
                        break;
                    }
                    PokedexDetailCommon_GoNext(common);
                    newSpecies = PokedexDetailCommon_GetSpecies(common);
                }
                if (species != newSpecies) {
                    PokedexDetailForm_ChangePokemon(param, wk, common);
                    if (count >= 3) {
                        PokedexDetailTouchbar_SetFormArrowsVisible(touchbar, TRUE);
                    } else {
                        PokedexDetailTouchbar_SetFormArrowsVisible(touchbar, FALSE);
                    }
                }
            }
            break;
        case POKEDEX_DETAIL_CMD_CUR_U_TOUCH:
        case POKEDEX_DETAIL_CMD_FORM_CUR_U_TOUCH:
            if (wk->mode == FORM_MODE_SINGLE) {
                u16 species = PokedexDetailCommon_GetSpecies(common);

                PokedexDetailCommon_GoPrev(common);
                if (species != PokedexDetailCommon_GetSpecies(common)) {
                    PokedexDetailForm_ChangePokemon(param, wk, common);
                }
            } else if (wk->mode == FORM_MODE_COMPARE) {
                FormEntry entries[ENTRY_MAX];
                u16 count;
                u16 nameCount;
                u16 rareCount;
                u16 cur;
                u16 next;
                u16 species = PokedexDetailCommon_GetSpecies(common);
                u16 newSpecies;

                PokedexDetailCommon_GoPrev(common);
                newSpecies = PokedexDetailCommon_GetSpecies(common);
                while (species != newSpecies) {
                    PokedexDetailForm_GatherEntries(param, common, entries, &count, &nameCount, &rareCount, &cur,
                                                    &next);
                    if (count >= 2) {
                        break;
                    }
                    PokedexDetailCommon_GoPrev(common);
                    newSpecies = PokedexDetailCommon_GetSpecies(common);
                }
                if (species != newSpecies) {
                    PokedexDetailForm_ChangePokemon(param, wk, common);
                    if (count >= 3) {
                        PokedexDetailTouchbar_SetFormArrowsVisible(touchbar, TRUE);
                    } else {
                        PokedexDetailTouchbar_SetFormArrowsVisible(touchbar, FALSE);
                    }
                }
            }
            break;
        case POKEDEX_DETAIL_CMD_FORM_CUR_R_TOUCH:
            if (wk->count >= 3) {
                PokedexDetailForm_AddNext(wk, 1);
                if (wk->cur == wk->next) {
                    PokedexDetailForm_AddNext(wk, 1);
                }
                PokedexDetailForm_ChangeNext(param, wk, common);
                wk->sliderSnap = TRUE;
                wk->sliderActive = TRUE;
            }
            break;
        case POKEDEX_DETAIL_CMD_FORM_CUR_L_TOUCH:
            if (wk->count >= 3) {
                PokedexDetailForm_AddNext(wk, wk->count - 1);
                if (wk->cur == wk->next) {
                    PokedexDetailForm_AddNext(wk, wk->count - 1);
                }
                PokedexDetailForm_ChangeNext(param, wk, common);
                wk->sliderSnap = TRUE;
                wk->sliderActive = TRUE;
            }
            break;
        case POKEDEX_DETAIL_CMD_FORM_BUTTON_TOUCH:
            if (wk->count >= 2) {
                PokedexDetailForm_SetMode(param, wk, common, FORM_MODE_SELECT);
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
        case POKEDEX_DETAIL_CMD_NONE:
            break;
        case POKEDEX_DETAIL_CMD_CLOSE:
        case POKEDEX_DETAIL_CMD_RETURN:
            wk->exit = FORM_EXIT_SCREEN;
            break;
        case POKEDEX_DETAIL_CMD_INFO:
        case POKEDEX_DETAIL_CMD_MAP:
        case POKEDEX_DETAIL_CMD_VOICE:
            wk->exit = FORM_EXIT_PAGE;
            break;
        case POKEDEX_DETAIL_CMD_CUR_D:
        case POKEDEX_DETAIL_CMD_FORM_CUR_D:
            PokedexDetailTouchbar_Unlock(touchbar);
            break;
        case POKEDEX_DETAIL_CMD_CUR_U:
        case POKEDEX_DETAIL_CMD_FORM_CUR_U:
            PokedexDetailTouchbar_Unlock(touchbar);
            break;
        case POKEDEX_DETAIL_CMD_CHECK:
            GameData_SetKeyItemRegistration(PokedexDetailCommon_GetGameData(common), SHORTCUT_POKEDEX_FORM,
                                            PokedexDetailTouchbar_GetCheck(touchbar));
            PokedexDetailTouchbar_Unlock(touchbar);
            break;
        case POKEDEX_DETAIL_CMD_FORM_CUR_R:
            PokedexDetailTouchbar_Unlock(touchbar);
            break;
        case POKEDEX_DETAIL_CMD_FORM_CUR_L:
            PokedexDetailTouchbar_Unlock(touchbar);
            break;
        case POKEDEX_DETAIL_CMD_FORM_RETURN:
            if (wk->mode == FORM_MODE_COMPARE) {
                PokedexDetailForm_SetMode(param, wk, common, FORM_MODE_CLOSE);
            }
            break;
        case POKEDEX_DETAIL_CMD_FORM_BUTTON:
            if (wk->selectPending) {
                wk->selectButtonDone = TRUE;
                if (wk->selectMoved) {
                    wk->selectPending = FALSE;
                    PokedexDetailTouchbar_Unlock(touchbar);
                }
            }
            break;
        default:
            if (unlock) {
                PokedexDetailTouchbar_Unlock(touchbar);
            }
            break;
        }
    }
}

static void PokedexDetailForm_Draw(PokedexDetailProcSys *sys, int *seq, void *param, void *work,
                                   PokedexDetailCommon *common) {
    PokedexDetailFormWork *wk = work;

    if (wk != NULL && wk->mcssSys != NULL) {
        MCSSSys_Draw(wk->mcssSys);
    }
}

static void PokedexDetailForm_VBlank(TCB *tcb, void *data) {
    PokedexDetailFormWork *wk = data;

    PokedexDetailPalFade_VBlank(wk->palFade);
}

static void PokedexDetailForm_GetEntryText(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                           PokedexDetailCommon *common, StrBuf **name, StrBuf **color, BOOL *freeName,
                                           BOOL *freeColor, u16 index) {
    StrBuf *nameStr = NULL;
    StrBuf *colorStr = NULL;
    BOOL nameNew = FALSE;
    BOOL colorNew = FALSE;
    u16 species = PokedexDetailCommon_GetSpecies(common);

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

static void PokedexDetailForm_CreateText(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                         PokedexDetailCommon *common) {
    PokedexDetailWindowData windows[WINDOW_COUNT] = {
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
    wk->formMsgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ZUKAN_DETAIL_FORM, param->heapId);
    wk->formNameTable = GFL_ArcSysReadHeapNewLZGetLen(ARCID_FORM_NAME_TABLE, 0, FALSE, param->heapId, &size);

    bitmap = BmpWin_GetBitmap(wk->windows[WINDOW_LABELS]);
    strbuf1 = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlServerFlowTitle_Text_FormsSeen);
    PrintQueue_PrintColor(wk->printQueues[WINDOW_LABELS], bitmap, 0, 1, strbuf1, wk->font, PRINT_COLOR(15, 2, 0));
    GFL_StrBufFree(strbuf1);
    strbuf2 = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlServerFlowTitle_Text_ShinySeen);
    PrintQueue_PrintColor(wk->printQueues[WINDOW_LABELS], bitmap, 0, 17, strbuf2, wk->font, PRINT_COLOR(15, 2, 0));
    GFL_StrBufFree(strbuf2);
    wk->flushPending[WINDOW_LABELS] = TRUE;
    PokedexDetailForm_FlushWindow(param, wk, common, WINDOW_LABELS);

    for (i = 0; i < STRBUF_COUNT; i++) {
        wk->strbufs[i] = GFL_MsgDataLoadStrbufNew(wk->msgData, POKEDEX_DETAIL_FORM_STRBUF_MESSAGES[i]);
    }
}

static void PokedexDetailForm_FreeText(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                       PokedexDetailCommon *common) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        wk->flushPending[i] = FALSE;
        PrintQueue_Clear(wk->printQueues[i]);
        BmpWin_Free(wk->windows[i]);
    }
    for (i = 0; i < STRBUF_COUNT; i++) {
        GFL_StrBufFree(wk->strbufs[i]);
    }
    GFL_MsgDataFree(wk->msgData);
    GFL_MsgDataFree(wk->formMsgData);
    GFL_HeapFree(wk->formNameTable);
}

static void PokedexDetailForm_CreatePrintQueues(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                                PokedexDetailCommon *common) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        wk->printQueues[i] = PrintQueue_CreateDefault(param->heapId);
        wk->flushPending[i] = FALSE;
    }
}

static void PokedexDetailForm_FreePrintQueues(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                              PokedexDetailCommon *common) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        wk->flushPending[i] = FALSE;
        PrintQueue_Clear(wk->printQueues[i]);
        PrintQueue_Free(wk->printQueues[i]);
    }
}

static void PokedexDetailForm_UpdateText(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                         PokedexDetailCommon *common) {
    u8 i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        PrintQueue_Main(wk->printQueues[i]);
        PokedexDetailForm_FlushWindow(param, wk, common, i);
    }
}

static void PokedexDetailForm_FlushWindow(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                          PokedexDetailCommon *common, u8 window) {
    if (wk->flushPending[window] &&
        !PrintQueue_IsBitmapPending(wk->printQueues[window], BmpWin_GetBitmap(wk->windows[window]))) {
        BmpWin_Transfer(wk->windows[window]);
        wk->flushPending[window] = FALSE;
    }
}

static void PokedexDetailForm_PrintName(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                        PokedexDetailCommon *common) {
    u8 i;

    for (i = WINDOW_NAME; i <= WINDOW_COUNTS; i++) {
        wk->flushPending[i] = FALSE;
        PrintQueue_Clear(wk->printQueues[i]);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[i]), 0);
    }

    {
        u16 species = PokedexDetailCommon_GetSpecies(common);
        GFLBitmap *bitmap = BmpWin_GetBitmap(wk->windows[WINDOW_NAME]);
        StrBuf *name = GFL_MsgDataLoadStrbufNew(g_PMLSpeciesNamesResident, species);
        u16 width = GFL_FontGetBlockWidth(name, wk->font, 0);
        int windowWidth = GFL_BitmapGetWidth(bitmap);
        u16 x = (windowWidth - width) / 2;

        PrintQueue_PrintColor(wk->printQueues[WINDOW_NAME], bitmap, x, 1, name, wk->font, PRINT_COLOR(15, 2, 0));
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
            PrintQueue_PrintColor(wk->printQueues[WINDOW_COUNTS], bitmap, 0, 1, strbuf, wk->font,
                                  PRINT_COLOR(15, 2, 0));
            GFL_StrBufFree(strbuf);
            GFL_WordSetSystemFree(wordSet);
        }
        {
            WordSet *wordSet = GFL_WordSetSystemCreateDefault(param->heapId);
            StrBuf *template = wk->strbufs[STRBUF_RARE_COUNT];
            StrBuf *strbuf = GFL_StrBufCreate(8, param->heapId);

            WordSetNumber(wordSet, 0, wk->rareCount, 2, 1, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, strbuf, template);
            PrintQueue_PrintColor(wk->printQueues[WINDOW_COUNTS], bitmap, 0, 17, strbuf, wk->font,
                                  PRINT_COLOR(15, 2, 0));
            GFL_StrBufFree(strbuf);
            GFL_WordSetSystemFree(wordSet);
        }
        wk->flushPending[WINDOW_COUNTS] = TRUE;
    }

    for (i = WINDOW_NAME; i <= WINDOW_COUNTS; i++) {
        PokedexDetailForm_FlushWindow(param, wk, common, i);
    }
}

static void PokedexDetailForm_PrintEntry(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                         PokedexDetailCommon *common, u8 window, PrintQueue *printQueue, BmpWin *bmpWin,
                                         BOOL *flushPending, u16 index) {
    GFLBitmap *bitmap = BmpWin_GetBitmap(bmpWin);
    int windowWidth = GFL_BitmapGetWidth(bitmap);
    StrBuf *name;
    StrBuf *color;
    BOOL freeName;
    BOOL freeColor;
    EntryLine y;
    EntryLine colorY = ENTRY_LINE_BOTTOM;

    PokedexDetailForm_GetEntryText(param, wk, common, &name, &color, &freeName, &freeColor, index);
    // The name alone is printed in the middle of the window
    y = ENTRY_LINE_TOP;
    if (color == NULL) {
        y = ENTRY_LINE_MIDDLE;
    }
    {
        u16 width = GFL_FontGetBlockWidth(name, wk->font, 0);
        u16 x = (windowWidth - width) / 2;

        PrintQueue_PrintColor(printQueue, bitmap, x, y, name, wk->font, PRINT_COLOR(15, 2, 0));
    }
    if (color != NULL) {
        u16 width = GFL_FontGetBlockWidth(color, wk->font, 0);
        u16 x = (windowWidth - width) / 2;

        PrintQueue_PrintColor(printQueue, bitmap, x, colorY, color, wk->font, PRINT_COLOR(15, 2, 0));
    }
    if (name != NULL && freeName) {
        GFL_StrBufFree(name);
    }
    if (color != NULL && freeColor) {
        GFL_StrBufFree(color);
    }
    *flushPending = TRUE;
    PokedexDetailForm_FlushWindow(param, wk, common, window);
}

static void PokedexDetailForm_PrintCur(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                       PokedexDetailCommon *common) {
    wk->flushPending[WINDOW_CUR] = FALSE;
    PrintQueue_Clear(wk->printQueues[WINDOW_CUR]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_CUR]), 0);
    PokedexDetailForm_PrintEntry(param, wk, common, WINDOW_CUR, wk->printQueues[WINDOW_CUR], wk->windows[WINDOW_CUR],
                                 &wk->flushPending[WINDOW_CUR], wk->cur);
}

static void PokedexDetailForm_PrintNext(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                        PokedexDetailCommon *common) {
    wk->flushPending[WINDOW_NEXT] = FALSE;
    PrintQueue_Clear(wk->printQueues[WINDOW_NEXT]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[WINDOW_NEXT]), 0);
    switch (wk->mode) {
    case FORM_MODE_SINGLE:
        BmpWin_FlushChar(wk->windows[WINDOW_NEXT]);
        break;
    case FORM_MODE_COMPARE:
        if (wk->count >= 2) {
            PokedexDetailForm_PrintEntry(param, wk, common, WINDOW_NEXT, wk->printQueues[WINDOW_NEXT],
                                         wk->windows[WINDOW_NEXT], &wk->flushPending[WINDOW_NEXT], wk->next);
        }
        break;
    }
}

// The second entry's name shows on the BG beside the first one's in the comparison
static void PokedexDetailForm_ScrollNextBG(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                           PokedexDetailCommon *common) {
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

static void PokedexDetailForm_CreateMCSS(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                         PokedexDetailCommon *common) {
    wk->mcssSys = MCSSSys_Create(SPRITE_COUNT, param->heapId);
    MCSSSys_SetTexBase(wk->mcssSys, 0);
    MCSSSys_EnableOrtho(wk->mcssSys);
    MCSSSys_OpenArc(wk->mcssSys, GetPokemonGraphicsARCID());
    wk->tcbBuffer =
        GFL_HeapAllocate(param->heapId, GFL_TCBMgrCalcAllocSize(SPRITE_COUNT), FALSE, "zukan_detail_form.c", 2643);
    wk->tcbMgr = GFL_TCBMgrCreate(SPRITE_COUNT, wk->tcbBuffer);
    MCSSSys_SetTCBManager(wk->mcssSys, wk->tcbMgr);
}

static void PokedexDetailForm_FreeMCSS(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                       PokedexDetailCommon *common) {
    MCSSSys_CloseArc(wk->mcssSys);
    MCSSSys_Free(wk->mcssSys);
    GFL_TCBMgrDelete(wk->tcbMgr);
    GFL_HeapFree(wk->tcbBuffer);
}

static MCSS *PokedexDetailForm_AddMCSS(MCSSSystem *system, u32 species, u32 form, u32 sex, u32 rare, u32 a6, u32 back,
                                       u32 personality) {
    VecFx32 scale = { FX32_ONE * 16, FX32_ONE * 16, FX32_ONE };
    MCSSLoadInfo info;
    MCSS *mcss;

    if (species == SPECIES_SPINDA) {
        MCSSSys_SetSpindaLoadCallback(system, personality);
    }
    SetupPokemonLoaderFSTool(species, form, sex, rare, a6, &info, back);
    mcss = MCSSSys_Add(system, 0, 0, 0, &info);
    MCSS_SetNoShadow(mcss, 1);
    MCSS_PauseAnimation(mcss);
    MCSS_SetScale(mcss, &scale);
    PokedexDetailForm_SetShadow(mcss);
    return mcss;
}

static void PokedexDetailForm_RemoveMCSS(MCSSSystem *system, MCSS *mcss) {
    MCSS_Hide(mcss);
    MCSSSys_Remove(system, mcss);
}

static void PokedexDetailForm_SetShadow(MCSS *mcss) {
    f32 x = -(f32)MCSS_GetOffsetX(mcss) * 0.25f;
    VecFx32 offset;

    offset.x = FX32_CONST(x);
    offset.y = 0;
    offset.z = 0;
    MCSS_SetPositionOffset(mcss, &offset);
}

static void PokedexDetailForm_GetBesidePos(MCSS *mcss, VecFx32 *pos) {
    VecFx32 cur;

    MCSS_GetPosition(mcss, &cur);
    pos->x = cur.x + sPokedexDetailFormOffsets[2].x;
    pos->y = cur.y + sPokedexDetailFormOffsets[2].y;
    pos->z = cur.z + sPokedexDetailFormOffsets[2].z;
}

static void PokedexDetailForm_CreateSprite(Sprite *sprite, HeapID heapId, MCSSSystem *system, u32 species, u32 form,
                                           u32 sex, u32 rare, u32 a6, u32 back, u32 personality) {
    sprite->mcss = PokedexDetailForm_AddMCSS(system, species, form, sex, rare, a6, back, personality);
    sprite->anim = GFL_HeapAllocate(heapId, sizeof(SpriteAnim), TRUE, "zukan_detail_form.c", 2737);
    NNS_G2dSetAnimCtrlCallBackFunctor(&MCSS_GetMultiCellAnimation(sprite->mcss)->animCtrl,
                                      NNS_G2D_ANMCALLBACKTYPE_LAST_FRM, (u32)sprite->anim,
                                      PokedexDetailForm_SpriteAnimEnd);
    PokedexDetailForm_InitSpritePositions(sprite->pos, species, form, sex, rare, a6, back, personality);
}

static void PokedexDetailForm_FreeSprite(Sprite *sprite, MCSSSystem *system) {
    if (sprite->mcss != NULL) {
        PokedexDetailForm_RemoveMCSS(system, sprite->mcss);
    }
    sprite->mcss = NULL;
    if (sprite->anim != NULL) {
        GFL_HeapFree(sprite->anim);
    }
    sprite->anim = NULL;
}

static void PokedexDetailForm_InitSpriteAnim(SpriteAnim *anim, int sprite, PokedexDetailFormWork *wk) {
    anim->sprite = sprite;
    anim->wk = wk;
    anim->loops = 0;
    anim->stop = FALSE;
}

static void PokedexDetailForm_PlaySpriteAnim(Sprite *sprite) {
    if (sprite->mcss != NULL && sprite->anim != NULL) {
        NNSG2dMultiCellAnimation *mcAnim = MCSS_GetMultiCellAnimation(sprite->mcss);

        sprite->anim->loops = 0;
        sprite->anim->stop = FALSE;
        NNS_G2dRestartMCAnimation(mcAnim);
        MCSS_ResumeAnimation(sprite->mcss);
    }
}

static void PokedexDetailForm_StopSpriteAnim(Sprite *sprite) {
    if (sprite->mcss != NULL && sprite->anim != NULL) {
        sprite->anim->stop = TRUE;
    }
}

static void PokedexDetailForm_UpdateSprite(Sprite *sprite) {
    if (sprite->mcss != NULL && sprite->anim != NULL) {
        NNSG2dMultiCellAnimation *mcAnim = MCSS_GetMultiCellAnimation(sprite->mcss);

        if (sprite->anim->stop) {
            MCSS_PauseAnimation(sprite->mcss);
            NNS_G2dRestartMCAnimation(mcAnim);
            sprite->anim->stop = FALSE;
        }
    }
}

static void PokedexDetailForm_SpriteAnimEnd(u32 param, fx32 frame) {
    SpriteAnim *anim = (SpriteAnim *)param;
    PokedexDetailFormWork *wk = anim->wk;

    anim->loops++;
    if (anim->loops >= ANIM_LOOPS) {
        anim->stop = TRUE;
    }
    MCSS_RequestRestart(wk->sprites[anim->sprite].mcss);
}

static void PokedexDetailForm_Turn(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                   PokedexDetailCommon *common) {
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
    PokedexDetailForm_LoadTurnedSprites(param, wk, common);
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

static void PokedexDetailForm_LoadIconResources(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                                PokedexDetailCommon *common) {
    wk->iconArc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, param->heapId);
    wk->iconPalette = ClActSys_LoadPlttEx(wk->iconArc, PokeIcon_GetPaletteFile(), CLACT_VRAM_SUB, 0, param->heapId);
    wk->iconCellAnims =
        ClActSys_LoadCellAnims(wk->iconArc, PokeIcon_GetCellFile_Sub(), getOBJTileMapping_SubEng(), param->heapId);
}

static void PokedexDetailForm_FreeIconResources(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                                PokedexDetailCommon *common) {
    ClActSys_FreeCellAnims(wk->iconCellAnims);
    ClActSys_FreePltt(wk->iconPalette);
    GFL_ArcToolFree(wk->iconArc);
}

static ClActor *PokedexDetailForm_CreateIcon(ArcTool *arc, u32 *chars, u32 palette, u32 cellAnims, ClActUnit *unit,
                                             HeapID heapId, u32 species, u32 form, u32 sex, BOOL egg) {
    ClActorSetup setup;
    ClActor *actor;

    setup.x = 128;
    setup.y = 128;
    setup.sequence = 1;
    setup.priority = 1;
    setup.bgPriority = 0;
    *chars = ClActSys_LoadChars(arc, PokeParty_GetIconIndex(species, form, sex, egg), FALSE, CLACT_VRAM_SUB, heapId);
    actor = ClActor_Create(unit, *chars, palette, cellAnims, &setup, CLACT_SURFACE_SUB, heapId);
    ClActor_SetAutoAnim(actor, FALSE);
    ClActor_SetPalette(actor, PokeIcon_GetPalette(species, form, sex, egg), 0);
    ClActor_SetObjMode(actor, 0);
    return actor;
}

static void PokedexDetailForm_FreeIcon(u32 chars, ClActor *actor) {
    ClActor_Delete(actor);
    ClActSys_FreeChars(chars);
}

// Lists every sex, color and form of the species that the Pokédex has seen. The one that it shows comes first, with
// the next one as the second entry
static void PokedexDetailForm_GatherEntries(PokedexDetailFormParam *param, PokedexDetailCommon *common,
                                            FormEntry *entries, u16 *count, u16 *nameCount, u16 *rareCount, u16 *cur,
                                            u16 *next) {
    u32 sexes[3] = { 0, 1, 2 };
    BOOL rares[2] = { FALSE, TRUE };
    u32 shownSex;
    u32 shownRare;
    u32 shownForm;
    u16 species = PokedexDetailCommon_GetSpecies(common);
    PokeDexSave *pokedex = GameData_GetPokedex(PokedexDetailCommon_GetGameData(common));
    u32 sexRatio;
    u16 shownEntry;
    u8 i;
    u8 j;
    BOOL found;
    u16 prevValue;
    u32 form;
    u16 n = 0;
    PersonalData *personal = PML_PersonalLoad(species, 0, param->heapId);
    // The personal data's count of forms is read, but the Pokédex's own count is used
    u32 formCount = PML_PersonalGetParam(personal, PERSONAL_FORM_COUNT);

    sexRatio = PML_PersonalGetParam(personal, PERSONAL_SEX_RATIO);
    PML_PersonalFree(personal);
    formCount = getNumberOfForms(species);
    PokeDex_GetShownSexRareForm(pokedex, species, &shownSex, &shownRare, &shownForm, param->heapId);
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
                        } else if (PokeDex_HasSeenVariant(pokedex, species, sex, rare, form)) {
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

static void PokedexDetailForm_LoadEntries(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                          PokedexDetailCommon *common) {
    PokedexDetailForm_GatherEntries(param, common, wk->entries, &wk->count, &wk->nameCount, &wk->rareCount, &wk->cur,
                                    &wk->next);
}

// Adds the entry's front and back sprites, unless SPRITE_NONE
static void PokedexDetailForm_CreateSprites(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                            PokedexDetailCommon *common, int frontSprite, int backSprite, int place,
                                            u16 index) {
    u32 personality = 0;
    u16 species = PokedexDetailCommon_GetSpecies(common);
    u32 form = wk->entries[index].form;
    u32 sex = wk->entries[index].sex;
    u32 rare = wk->entries[index].rare;
    VecFx32 pos;

    if (species == SPECIES_SPINDA) {
        personality = PokeDex_GetSpindaPersonality(GameData_GetPokedex(PokedexDetailCommon_GetGameData(common)), 0);
    }
    if (frontSprite != SPRITE_NONE) {
        PokedexDetailForm_CreateSprite(&wk->sprites[frontSprite], param->heapId, wk->mcssSys, species, form, sex, rare,
                                       0, 0, personality);
        PokedexDetailForm_GetPlacePos(&wk->sprites[frontSprite], place, &pos);
        MCSS_SetPosition(wk->sprites[frontSprite].mcss, &pos);
        if (!wk->front) {
            MCSS_Hide(wk->sprites[frontSprite].mcss);
        }
    }
    if (backSprite != SPRITE_NONE) {
        PokedexDetailForm_CreateSprite(&wk->sprites[backSprite], param->heapId, wk->mcssSys, species, form, sex, rare,
                                       0, 1, personality);
        PokedexDetailForm_GetPlacePos(&wk->sprites[backSprite], place, &pos);
        MCSS_SetPosition(wk->sprites[backSprite].mcss, &pos);
        if (wk->front) {
            MCSS_Hide(wk->sprites[backSprite].mcss);
        }
    }
}

static void PokedexDetailForm_CreateCurIcon(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                            PokedexDetailCommon *common, u16 index) {
    u16 species = PokedexDetailCommon_GetSpecies(common);

    wk->iconActors[wk->icon] = PokedexDetailForm_CreateIcon(wk->iconArc, &wk->iconChars[wk->icon], wk->iconPalette,
                                                            wk->iconCellAnims, wk->unit, param->heapId, species,
                                                            wk->entries[index].form, wk->entries[index].sex, FALSE);
}

// Whether the comparison shows the second entry
static inline BOOL PokedexDetailForm_IsComparing(PokedexDetailFormWork *wk) {
    BOOL comparing = FALSE;

    if (wk->mode == FORM_MODE_COMPARE && wk->count >= 2) {
        comparing = TRUE;
    }
    return comparing;
}

static void PokedexDetailForm_ChangePokemon(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                            PokedexDetailCommon *common) {
    u8 i;
    int frontSprite;
    int backSprite;
    int place;

    for (i = 0; i < SPRITE_COUNT; i++) {
        PokedexDetailForm_FreeSprite(&wk->sprites[i], wk->mcssSys);
    }
    PokedexDetailForm_LoadEntries(param, wk, common);
    place = PLACE_SINGLE;
    if (PokedexDetailForm_IsComparing(wk)) {
        place = PLACE_LEFT;
    }
    frontSprite = wk->front ? SPRITE_CUR_FRONT : SPRITE_NONE;
    backSprite = wk->front ? SPRITE_NONE : SPRITE_CUR_BACK;
    PokedexDetailForm_CreateSprites(param, wk, common, frontSprite, backSprite, place, wk->cur);
    if (frontSprite != SPRITE_NONE) {
        PokedexDetailForm_InitSpriteAnim(wk->sprites[SPRITE_CUR_FRONT].anim, SPRITE_CUR_FRONT, wk);
    }
    if (backSprite != SPRITE_NONE) {
        PokedexDetailForm_InitSpriteAnim(wk->sprites[SPRITE_CUR_BACK].anim, SPRITE_CUR_BACK, wk);
    }
    if (PokedexDetailForm_IsComparing(wk)) {
        frontSprite = wk->front ? SPRITE_NEXT_FRONT : SPRITE_NONE;
        backSprite = wk->front ? SPRITE_NONE : SPRITE_NEXT_BACK;
        PokedexDetailForm_CreateSprites(param, wk, common, frontSprite, backSprite, PLACE_RIGHT, wk->next);
        if (frontSprite != SPRITE_NONE) {
            PokedexDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_FRONT].anim, SPRITE_NEXT_FRONT, wk);
        }
        if (backSprite != SPRITE_NONE) {
            PokedexDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_BACK].anim, SPRITE_NEXT_BACK, wk);
        }
    }
    PokedexDetailForm_ChangeIcon(param, wk, common);
    PokedexDetailForm_PrintName(param, wk, common);
    PokedexDetailForm_ScrollNextBG(param, wk, common);
    PokedexDetailForm_PrintCur(param, wk, common);
    PokedexDetailForm_PrintNext(param, wk, common);
    PokedexDetailForm_UpdateColorMarks(param, wk, common);
    PokedexDetailForm_UpdateSlider(param, wk, common);
    PokedexDetailForm_UpdateArrows(param, wk, common);
}

// Adds the sprites of the side now shown that have not been loaded yet, and places the others
static void PokedexDetailForm_LoadTurnedSprites(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                                PokedexDetailCommon *common) {
    int frontSprite;
    int backSprite;
    int place;
    VecFx32 pos;

    MCSSSys_SetTCBManager(wk->mcssSys, NULL);
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
    if (PokedexDetailForm_IsComparing(wk)) {
        place = PLACE_LEFT;
    }
    PokedexDetailForm_CreateSprites(param, wk, common, frontSprite, backSprite, place, wk->cur);
    if (frontSprite != SPRITE_NONE) {
        PokedexDetailForm_InitSpriteAnim(wk->sprites[SPRITE_CUR_FRONT].anim, SPRITE_CUR_FRONT, wk);
    } else if (wk->sprites[SPRITE_CUR_FRONT].mcss != NULL) {
        PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_FRONT], place, &pos);
        MCSS_SetPosition(wk->sprites[SPRITE_CUR_FRONT].mcss, &pos);
    }
    if (backSprite != SPRITE_NONE) {
        PokedexDetailForm_InitSpriteAnim(wk->sprites[SPRITE_CUR_BACK].anim, SPRITE_CUR_BACK, wk);
    } else if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
        PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_BACK], place, &pos);
        MCSS_SetPosition(wk->sprites[SPRITE_CUR_BACK].mcss, &pos);
    }
    if (PokedexDetailForm_IsComparing(wk)) {
        frontSprite = SPRITE_NEXT_FRONT;
        if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
            frontSprite = SPRITE_NONE;
        }
        backSprite = SPRITE_NEXT_BACK;
        if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
            backSprite = SPRITE_NONE;
        }
        PokedexDetailForm_CreateSprites(param, wk, common, frontSprite, backSprite, PLACE_RIGHT, wk->next);
        if (frontSprite != SPRITE_NONE) {
            PokedexDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_FRONT].anim, SPRITE_NEXT_FRONT, wk);
        } else if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
            PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_NEXT_FRONT], PLACE_RIGHT, &pos);
            MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &pos);
        }
        if (backSprite != SPRITE_NONE) {
            PokedexDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_BACK].anim, SPRITE_NEXT_BACK, wk);
        } else if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
            PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_NEXT_BACK], PLACE_RIGHT, &pos);
            MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &pos);
        }
    }
    MCSSSys_SetTCBManager(wk->mcssSys, wk->tcbMgr);
    if (wk->mode == FORM_MODE_SINGLE) {
        wk->slideFromRight = TRUE;
        PokedexDetailForm_PlaceNext(param, wk, common);
    }
}

// Adds every sprite not loaded yet, to play their animations
static void PokedexDetailForm_LoadAllSprites(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                             PokedexDetailCommon *common) {
    int frontSprite = SPRITE_CUR_FRONT;
    int backSprite;
    int place;
    VecFx32 pos;

    MCSSSys_SetTCBManager(wk->mcssSys, NULL);
    if (wk->sprites[SPRITE_CUR_FRONT].mcss != NULL) {
        frontSprite = SPRITE_NONE;
    }
    backSprite = SPRITE_CUR_BACK;
    if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
        backSprite = SPRITE_NONE;
    }
    place = PLACE_SINGLE;
    if (PokedexDetailForm_IsComparing(wk)) {
        place = PLACE_LEFT;
    }
    PokedexDetailForm_CreateSprites(param, wk, common, frontSprite, backSprite, place, wk->cur);
    if (frontSprite != SPRITE_NONE) {
        PokedexDetailForm_InitSpriteAnim(wk->sprites[SPRITE_CUR_FRONT].anim, SPRITE_CUR_FRONT, wk);
    } else {
        PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_FRONT], place, &pos);
        MCSS_SetPosition(wk->sprites[SPRITE_CUR_FRONT].mcss, &pos);
    }
    if (backSprite != SPRITE_NONE) {
        PokedexDetailForm_InitSpriteAnim(wk->sprites[SPRITE_CUR_BACK].anim, SPRITE_CUR_BACK, wk);
    } else {
        PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_BACK], place, &pos);
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
    PokedexDetailForm_CreateSprites(param, wk, common, frontSprite, backSprite, PLACE_RIGHT, wk->next);
    if (frontSprite != SPRITE_NONE) {
        PokedexDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_FRONT].anim, SPRITE_NEXT_FRONT, wk);
    } else {
        PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_NEXT_FRONT], PLACE_RIGHT, &pos);
        MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &pos);
    }
    if (backSprite != SPRITE_NONE) {
        PokedexDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_BACK].anim, SPRITE_NEXT_BACK, wk);
    } else {
        PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_NEXT_BACK], PLACE_RIGHT, &pos);
        MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &pos);
    }
    MCSSSys_SetTCBManager(wk->mcssSys, wk->tcbMgr);
    if (wk->mode == FORM_MODE_SINGLE) {
        wk->slideFromRight = TRUE;
        PokedexDetailForm_PlaceNext(param, wk, common);
    }
}

static void PokedexDetailForm_ChangeNext(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                         PokedexDetailCommon *common) {
    int frontSprite;
    int backSprite;

    PokedexDetailForm_FreeSprite(&wk->sprites[SPRITE_NEXT_FRONT], wk->mcssSys);
    PokedexDetailForm_FreeSprite(&wk->sprites[SPRITE_NEXT_BACK], wk->mcssSys);
    frontSprite = wk->front ? SPRITE_NEXT_FRONT : SPRITE_NONE;
    backSprite = wk->front ? SPRITE_NONE : SPRITE_NEXT_BACK;
    PokedexDetailForm_CreateSprites(param, wk, common, frontSprite, backSprite, PLACE_RIGHT, wk->next);
    if (frontSprite != SPRITE_NONE) {
        PokedexDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_FRONT].anim, SPRITE_NEXT_FRONT, wk);
    }
    if (backSprite != SPRITE_NONE) {
        PokedexDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_BACK].anim, SPRITE_NEXT_BACK, wk);
    }
    PokedexDetailForm_PrintNext(param, wk, common);
    PokedexDetailForm_UpdateColorMarks(param, wk, common);
}

static void PokedexDetailForm_ChangeIcon(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                         PokedexDetailCommon *common) {
    if (wk->iconActors[wk->icon] != NULL) {
        ClActor_SetVisible(wk->iconActors[wk->icon], FALSE);
    }
    wk->icon = (wk->icon + 1) % 2;
    // Frees the other icon as the screen's exit does, testing it again
    if (wk->iconActors[wk->icon] != NULL) {
        if (wk->iconActors[wk->icon] != NULL) {
            PokedexDetailForm_FreeIcon(wk->iconChars[wk->icon], wk->iconActors[wk->icon]);
        }
        wk->iconActors[wk->icon] = NULL;
    }
    PokedexDetailForm_CreateCurIcon(param, wk, common, wk->cur);
}

static void PokedexDetailForm_CheckInput(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                         PokedexDetailCommon *common) {
    PokedexDetailTouchbar *touchbar = PokedexDetailCommon_GetTouchbar(common);

    switch (wk->mode) {
    case FORM_MODE_SINGLE:
        if (wk->inputEnabled) {
            FormButton input = PokedexDetailForm_GetButtonInput(param, wk, common);

            if (input != BUTTON_NONE) {
                switch (input) {
                case BUTTON_TURN:
                    PokedexDetailForm_Turn(param, wk, common);
                    break;
                case BUTTON_PLAY: {
                    u8 i;

                    PokedexDetailForm_LoadAllSprites(param, wk, common);
                    for (i = 0; i < SPRITE_COUNT; i++) {
                        PokedexDetailForm_PlaySpriteAnim(&wk->sprites[i]);
                    }
                    break;
                }
                case BUTTON_ARROW_L:
                    wk->slideFromRight = TRUE;
                    PokedexDetailForm_StepNext(param, wk, common);
                    PokedexDetailForm_SetMode(param, wk, common, FORM_MODE_SLIDE);
                    break;
                case BUTTON_ARROW_R:
                    wk->slideFromRight = FALSE;
                    PokedexDetailForm_StepNext(param, wk, common);
                    PokedexDetailForm_SetMode(param, wk, common, FORM_MODE_SLIDE);
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

                    if (GFL_HIDGetTouchPressedPoint(&x, &y) && y < 168) {
                        open = TRUE;
                        touch = TRUE;
                    }
                }
                if (open && wk->count >= 2) {
                    if ((wk->front && wk->sprites[SPRITE_NEXT_FRONT].mcss == NULL) ||
                        (!wk->front && wk->sprites[SPRITE_NEXT_BACK].mcss == NULL)) {
                        PokedexDetailForm_LoadNext(param, wk, common);
                        wk->slideFromRight = TRUE;
                        PokedexDetailForm_PlaceNext(param, wk, common);
                    }
                    PokedexDetailForm_SetMode(param, wk, common, FORM_MODE_OPEN);
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                    GCTX_HIDSetUsingTouch(touch);
                    PokedexDetailTouchbar_SetActive(touchbar, FALSE);
                }
            }
        }
        break;
    case FORM_MODE_COMPARE:
        if (!PokedexDetailTouchbar_IsFormButtonTriggered(touchbar) && wk->inputEnabled && !wk->dragging) {
            FormButton input = PokedexDetailForm_GetButtonInput(param, wk, common);

            if (input != BUTTON_NONE) {
                switch (input) {
                case BUTTON_TURN:
                    PokedexDetailForm_Turn(param, wk, common);
                    break;
                case BUTTON_PLAY: {
                    u8 i;

                    PokedexDetailForm_LoadAllSprites(param, wk, common);
                    for (i = 0; i < SPRITE_COUNT; i++) {
                        PokedexDetailForm_PlaySpriteAnim(&wk->sprites[i]);
                    }
                    break;
                }
                }
            }
        }
        break;
    }
}

static void PokedexDetailForm_SetMode(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                      PokedexDetailCommon *common, int mode) {
    PokedexDetailTouchbar *touchbar = PokedexDetailCommon_GetTouchbar(common);
    PokedexDetailHeadbar *headbar = PokedexDetailCommon_GetHeadbar(common);
    BOOL changed = FALSE;

    switch (wk->mode) {
    case FORM_MODE_SINGLE:
        switch (mode) {
        case FORM_MODE_SLIDE:
            if (wk->slide == 0) {
                PokedexDetailForm_StopSpriteAnim(&wk->sprites[SPRITE_NEXT_FRONT]);
                PokedexDetailForm_StopSpriteAnim(&wk->sprites[SPRITE_NEXT_BACK]);
                wk->slide = 1;
            }
            break;
        case FORM_MODE_OPEN:
            wk->curInPlace = FALSE;
            wk->nextInPlace = FALSE;
            if (wk->count >= 2) {
                wk->slideFromRight = TRUE;
                PokedexDetailForm_PlaceNext(param, wk, common);
            }
            break;
        }
        break;
    case FORM_MODE_SLIDE:
        if (mode == FORM_MODE_SINGLE && wk->pushed == BUTTON_NONE) {
            PokedexDetailTouchbar_SetActive(touchbar, TRUE);
        }
        break;
    case FORM_MODE_OPEN:
        if (mode == FORM_MODE_OPENED) {
            PokedexDetailTouchbar_SetVisibleAll(touchbar, FALSE);
        }
        break;
    case FORM_MODE_OPENED:
        if (mode == FORM_MODE_COMPARE) {
            VecFx32 pos;

            changed = TRUE;
            if (wk->count >= 2) {
                if (wk->sprites[SPRITE_CUR_FRONT].mcss != NULL) {
                    PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_FRONT], PLACE_LEFT, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_CUR_FRONT].mcss, &pos);
                }
                if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
                    PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_BACK], PLACE_LEFT, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_CUR_BACK].mcss, &pos);
                }
                if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
                    PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_NEXT_FRONT], PLACE_RIGHT, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &pos);
                }
                if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
                    PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_NEXT_BACK], PLACE_RIGHT, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &pos);
                }
            }
            PokedexDetailTouchbar_SetType(touchbar, POKEDEX_DETAIL_TOUCHBAR_FORM, POKEDEX_DETAIL_PAGE_FORM - 1,
                                          PokedexDetailCommon_GetCount(common) > 1 ? TRUE : FALSE);
            if (wk->count >= 3) {
                PokedexDetailTouchbar_SetFormArrowsVisible(touchbar, TRUE);
            } else {
                PokedexDetailTouchbar_SetFormArrowsVisible(touchbar, FALSE);
            }
            PokedexDetailHeadbar_SetTitle(headbar, 4);
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
                PokedexDetailTouchbar_Unlock(touchbar);
            }
        }
        break;
    case FORM_MODE_CLOSE:
        if (mode == FORM_MODE_CLOSED) {
            PokedexDetailTouchbar_SetVisibleAll(touchbar, FALSE);
        }
        break;
    case FORM_MODE_CLOSED:
        if (mode == FORM_MODE_SINGLE) {
            VecFx32 pos;

            changed = TRUE;
            if (wk->count >= 2) {
                if (wk->sprites[SPRITE_CUR_FRONT].mcss != NULL) {
                    PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_FRONT], PLACE_SINGLE, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_CUR_FRONT].mcss, &pos);
                }
                if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
                    PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_BACK], PLACE_SINGLE, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_CUR_BACK].mcss, &pos);
                }
                PokedexDetailForm_GetBesidePos(wk->sprites[wk->front ? SPRITE_CUR_FRONT : SPRITE_CUR_BACK].mcss, &pos);
                if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
                    PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_NEXT_FRONT], PLACE_RIGHT, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &pos);
                }
                if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
                    PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_NEXT_BACK], PLACE_RIGHT, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &pos);
                }
            }
            PokedexDetailTouchbar_SetType(touchbar, POKEDEX_DETAIL_TOUCHBAR_GENERAL, POKEDEX_DETAIL_PAGE_FORM - 1,
                                          PokedexDetailCommon_GetCount(common) > 1 ? TRUE : FALSE);
            PokedexDetailTouchbar_SetCheck(
                touchbar,
                GameData_IsShortcutRegistered(PokedexDetailCommon_GetGameData(common), SHORTCUT_POKEDEX_FORM));
            PokedexDetailHeadbar_SetTitle(headbar, 3);
            if (wk->count >= 2) {
                wk->slideFromRight = TRUE;
                PokedexDetailForm_PlaceNext(param, wk, common);
            }
        }
        break;
    }

    wk->mode = mode;
    if (changed) {
        PokedexDetailForm_ScrollNextBG(param, wk, common);
        PokedexDetailForm_PrintNext(param, wk, common);
        PokedexDetailForm_UpdateColorMarks(param, wk, common);
        PokedexDetailForm_UpdateSlider(param, wk, common);
        PokedexDetailForm_UpdateArrows(param, wk, common);
        wk->pushed = BUTTON_NONE;
        PokedexDetailTouchbar_SetActive(touchbar, TRUE);
    }
}

static void PokedexDetailForm_CreateActors(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                           PokedexDetailCommon *common) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, param->heapId);
    u8 i;

    wk->resources[POKEDEX_DETAIL_FORM_RES_MAIN_PALETTE] =
        ClActSys_LoadPlttRange(arc, 3, CLACT_VRAM_MAIN, 0, 0, 3, param->heapId);
    wk->resources[POKEDEX_DETAIL_FORM_RES_MAIN_CHARS] =
        ClActSys_LoadChars(arc, 13, FALSE, CLACT_VRAM_MAIN, param->heapId);
    wk->resources[POKEDEX_DETAIL_FORM_RES_MAIN_CELL_ANIMS] = ClActSys_LoadCellAnims(arc, 28, 45, param->heapId);
    wk->resources[POKEDEX_DETAIL_FORM_RES_COLOR_PALETTE] =
        ClActSys_LoadPlttRange(arc, 0, CLACT_VRAM_MAIN, 0x60, 0, 1, param->heapId);
    wk->resources[POKEDEX_DETAIL_FORM_RES_COLOR_CHARS] =
        ClActSys_LoadChars(arc, 10, FALSE, CLACT_VRAM_MAIN, param->heapId);
    wk->resources[POKEDEX_DETAIL_FORM_RES_COLOR_CELL_ANIMS] = ClActSys_LoadCellAnims(arc, 27, 44, param->heapId);
    wk->resources[POKEDEX_DETAIL_FORM_RES_ARROW_PALETTE] =
        ClActSys_LoadPlttRange(arc, 6, CLACT_VRAM_MAIN, 0x80, 0, 2, param->heapId);
    wk->resources[POKEDEX_DETAIL_FORM_RES_ARROW_CHARS] =
        ClActSys_LoadChars(arc, 16, FALSE, CLACT_VRAM_MAIN, param->heapId);
    wk->resources[POKEDEX_DETAIL_FORM_RES_ARROW_CELL_ANIMS] = ClActSys_LoadCellAnims(arc, 30, 47, param->heapId);
    GFL_ArcToolFree(arc);

    for (i = 0; i < ACTOR_COUNT; i++) {
        ClActorSetup setup;

        MI_CpuFill8(&setup, 0, sizeof(ClActorSetup));
        setup.x = POKEDEX_DETAIL_FORM_ACTORS[i].x;
        setup.y = POKEDEX_DETAIL_FORM_ACTORS[i].y;
        setup.sequence = POKEDEX_DETAIL_FORM_ACTORS[i].sequence;
        setup.priority = POKEDEX_DETAIL_FORM_ACTORS[i].priority;
        setup.bgPriority = POKEDEX_DETAIL_FORM_ACTORS[i].bgPriority;
        wk->actors[i] = ClActor_Create(wk->unit, wk->resources[POKEDEX_DETAIL_FORM_ACTORS[i].chars],
                                       wk->resources[POKEDEX_DETAIL_FORM_ACTORS[i].palette],
                                       wk->resources[POKEDEX_DETAIL_FORM_ACTORS[i].cellAnims], &setup,
                                       CLACT_SURFACE_MAIN, param->heapId);
        ClActor_SetAutoAnim(wk->actors[i], TRUE);
        ClActor_SetVisible(wk->actors[i], FALSE);
        ClActor_SetObjMode(wk->actors[i], 1);
    }

    wk->buttons[BUTTON_TURN].x = 64;
    wk->buttons[BUTTON_TURN].y = 152;
    wk->buttons[BUTTON_TURN].chars = POKEDEX_DETAIL_FORM_RES_MAIN_CHARS;
    wk->buttons[BUTTON_TURN].palette = POKEDEX_DETAIL_FORM_RES_MAIN_PALETTE;
    wk->buttons[BUTTON_TURN].cellAnims = POKEDEX_DETAIL_FORM_RES_MAIN_CELL_ANIMS;
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
    wk->buttons[BUTTON_PLAY].chars = POKEDEX_DETAIL_FORM_RES_MAIN_CHARS;
    wk->buttons[BUTTON_PLAY].palette = POKEDEX_DETAIL_FORM_RES_MAIN_PALETTE;
    wk->buttons[BUTTON_PLAY].cellAnims = POKEDEX_DETAIL_FORM_RES_MAIN_CELL_ANIMS;
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
    wk->buttons[BUTTON_ARROW_L].chars = POKEDEX_DETAIL_FORM_RES_ARROW_CHARS;
    wk->buttons[BUTTON_ARROW_L].palette = POKEDEX_DETAIL_FORM_RES_ARROW_PALETTE;
    wk->buttons[BUTTON_ARROW_L].cellAnims = POKEDEX_DETAIL_FORM_RES_ARROW_CELL_ANIMS;
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
    wk->buttons[BUTTON_ARROW_R].chars = POKEDEX_DETAIL_FORM_RES_ARROW_CHARS;
    wk->buttons[BUTTON_ARROW_R].palette = POKEDEX_DETAIL_FORM_RES_ARROW_PALETTE;
    wk->buttons[BUTTON_ARROW_R].cellAnims = POKEDEX_DETAIL_FORM_RES_ARROW_CELL_ANIMS;
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

        MI_CpuFill8(&setup, 0, sizeof(ClActorSetup));
        setup.x = wk->buttons[i].x;
        setup.y = wk->buttons[i].y;
        setup.sequence = wk->buttons[i].activeAnim;
        setup.priority = 0;
        setup.bgPriority = 3;
        wk->buttons[i].actor =
            ClActor_Create(wk->unit, wk->resources[wk->buttons[i].chars], wk->resources[wk->buttons[i].palette],
                           wk->resources[wk->buttons[i].cellAnims], &setup, CLACT_SURFACE_MAIN, param->heapId);
        ClActor_SetAutoAnim(wk->buttons[i].actor, TRUE);
        ClActor_SetObjMode(wk->buttons[i].actor, 1);
    }
    wk->pushed = BUTTON_NONE;
}

static void PokedexDetailForm_FreeActors(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                         PokedexDetailCommon *common) {
    u8 i;

    for (i = 0; i < BUTTON_COUNT; i++) {
        ClActor_Delete(wk->buttons[i].actor);
    }
    for (i = 0; i < ACTOR_COUNT; i++) {
        ClActor_Delete(wk->actors[i]);
    }
    ClActSys_FreePltt(wk->resources[POKEDEX_DETAIL_FORM_RES_MAIN_PALETTE]);
    ClActSys_FreeChars(wk->resources[POKEDEX_DETAIL_FORM_RES_MAIN_CHARS]);
    ClActSys_FreeCellAnims(wk->resources[POKEDEX_DETAIL_FORM_RES_MAIN_CELL_ANIMS]);
    ClActSys_FreePltt(wk->resources[POKEDEX_DETAIL_FORM_RES_COLOR_PALETTE]);
    ClActSys_FreeChars(wk->resources[POKEDEX_DETAIL_FORM_RES_COLOR_CHARS]);
    ClActSys_FreeCellAnims(wk->resources[POKEDEX_DETAIL_FORM_RES_COLOR_CELL_ANIMS]);
    ClActSys_FreePltt(wk->resources[POKEDEX_DETAIL_FORM_RES_ARROW_PALETTE]);
    ClActSys_FreeChars(wk->resources[POKEDEX_DETAIL_FORM_RES_ARROW_CHARS]);
    ClActSys_FreeCellAnims(wk->resources[POKEDEX_DETAIL_FORM_RES_ARROW_CELL_ANIMS]);
}

// The button pushed with its key or touched, which starts its animation
static FormButton PokedexDetailForm_GetButtonInput(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                                   PokedexDetailCommon *common) {
    PokedexDetailTouchbar *touchbar = PokedexDetailCommon_GetTouchbar(common);
    FormButton button = BUTTON_NONE;
    BOOL touch;
    u8 i;

    if (button == BUTTON_NONE) {
        for (i = 0; i < BUTTON_COUNT; i++) {
            if (ClActor_IsVisible(wk->buttons[i].actor)) {
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

        if (GFL_HIDGetTouchPressedPoint(&x, &y)) {
            for (i = 0; i < BUTTON_COUNT; i++) {
                if (ClActor_IsVisible(wk->buttons[i].actor) && wk->buttons[i].rectX <= x &&
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

            GCTX_HIDSetUsingTouch(touch);
            PokedexDetailTouchbar_SetActive(touchbar, FALSE);
            wk->buttons[button].state = BUTTON_STATE_PUSHED;
            ClActor_SetSequence(wk->buttons[button].actor, wk->buttons[button].pushedAnim);
            GFL_SndSEPlay(wk->buttons[button].se);
            // Pushing one arrow greys out the other
            other = BUTTON_NONE;
            if (button == BUTTON_ARROW_L) {
                other = BUTTON_ARROW_R;
            } else if (button == BUTTON_ARROW_R) {
                other = BUTTON_ARROW_L;
            }
            if (other != BUTTON_NONE) {
                ClActor_SetAutoAnim(wk->buttons[other].actor, FALSE);
                ClActor_SetAnimTime(wk->buttons[other].actor, 0);
            }
            wk->pushed = button;
        }
    }
    return button;
}

static void PokedexDetailForm_UpdateArrows(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                           PokedexDetailCommon *common) {
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
        ClActor_SetVisible(wk->buttons[BUTTON_ARROW_L].actor, TRUE);
        ClActor_SetVisible(wk->buttons[BUTTON_ARROW_R].actor, TRUE);
    } else {
        ClActor_SetVisible(wk->buttons[BUTTON_ARROW_L].actor, FALSE);
        ClActor_SetVisible(wk->buttons[BUTTON_ARROW_R].actor, FALSE);
    }
}

static void PokedexDetailForm_UpdateButtons(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                            PokedexDetailCommon *common) {
    PokedexDetailTouchbar *touchbar = PokedexDetailCommon_GetTouchbar(common);

    if (wk->pushed != BUTTON_NONE) {
        switch (wk->buttons[wk->pushed].state) {
        case BUTTON_STATE_NONE:
            break;
        case BUTTON_STATE_PUSHED:
            wk->buttons[wk->pushed].state = BUTTON_STATE_ANIMATING;
            break;
        case BUTTON_STATE_ANIMATING:
            if (!ClActor_IsAnimActive(wk->buttons[wk->pushed].actor)) {
                wk->buttons[wk->pushed].state = BUTTON_STATE_DONE;
            }
            break;
        case BUTTON_STATE_DONE: {
            int other;

            ClActor_SetSequence(wk->buttons[wk->pushed].actor, wk->buttons[wk->pushed].activeAnim);
            wk->buttons[wk->pushed].state = BUTTON_STATE_NONE;
            other = BUTTON_NONE;
            if (wk->pushed == BUTTON_ARROW_L) {
                other = BUTTON_ARROW_R;
            } else if (wk->pushed == BUTTON_ARROW_R) {
                other = BUTTON_ARROW_L;
            }
            if (other != BUTTON_NONE && wk->buttons[other].state == BUTTON_STATE_NONE) {
                ClActor_SetAutoAnim(wk->buttons[other].actor, TRUE);
                ClActor_SetSequence(wk->buttons[other].actor, wk->buttons[other].activeAnim);
            }
            wk->pushed = BUTTON_NONE;
            if (wk->mode == FORM_MODE_SINGLE || wk->mode == FORM_MODE_COMPARE) {
                PokedexDetailTouchbar_SetActive(touchbar, TRUE);
            }
            break;
        }
        }
    }
}

// Shows whether the entries shown are in their shiny colors
static void PokedexDetailForm_UpdateColorMarks(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                               PokedexDetailCommon *common) {
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
        ClActor_SetSurfacePos(wk->actors[ACTOR_COLOR_CUR], &pos, CLACT_SURFACE_MAIN);
        pos.x = 128;
        pos.y = 0;
        ClActor_SetSurfacePos(wk->actors[ACTOR_COLOR_NEXT], &pos, CLACT_SURFACE_MAIN);
    } else if (showCur) {
        pos.x = 64;
        pos.y = 0;
        ClActor_SetSurfacePos(wk->actors[ACTOR_COLOR_CUR], &pos, CLACT_SURFACE_MAIN);
    }
    {
        u8 anim = wk->entries[wk->cur].color == ENTRY_COLOR_RARE ? 1 : 0;

        ClActor_SetVisible(wk->actors[ACTOR_COLOR_CUR], TRUE);
        ClActor_SetSequence(wk->actors[ACTOR_COLOR_CUR], anim);
    }
    if (showNext) {
        u8 anim = wk->entries[wk->next].color == ENTRY_COLOR_RARE ? 1 : 0;

        ClActor_SetVisible(wk->actors[ACTOR_COLOR_NEXT], TRUE);
        ClActor_SetSequence(wk->actors[ACTOR_COLOR_NEXT], anim);
    } else {
        ClActor_SetVisible(wk->actors[ACTOR_COLOR_NEXT], FALSE);
    }
}

static void PokedexDetailForm_UpdateSlider(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                           PokedexDetailCommon *common) {
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

        PokedexDetailForm_GetSliderSpan(
            wk->count - 1, PokedexDetailForm_GetSliderPos(param, wk, common, wk->cur, wk->next), NULL, &x, NULL);
        pos.x = x;
        pos.y = SLIDER_KNOB_Y;
        ClActor_SetSurfacePos(wk->actors[ACTOR_SLIDER_KNOB], &pos, CLACT_SURFACE_MAIN);
        ClActor_SetVisible(wk->actors[ACTOR_SLIDER_BAR], TRUE);
        ClActor_SetVisible(wk->actors[ACTOR_SLIDER_KNOB], TRUE);
        wk->sliderSnap = FALSE;
        wk->sliderActive = FALSE;
        wk->dragging = FALSE;
    } else {
        ClActor_SetVisible(wk->actors[ACTOR_SLIDER_BAR], FALSE);
        ClActor_SetVisible(wk->actors[ACTOR_SLIDER_KNOB], FALSE);
    }
}

static void PokedexDetailForm_CheckSlider(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                          PokedexDetailCommon *common) {
    if (!PokedexDetailTouchbar_IsFormButtonTriggered(PokedexDetailCommon_GetTouchbar(common)) &&
        ((wk->mode == FORM_MODE_COMPARE && wk->inputEnabled && wk->pushed == BUTTON_NONE) || wk->sliderActive)) {
        if (!PokedexDetailForm_SnapSlider(param, wk, common)) {
            PokedexDetailForm_DragSlider(param, wk, common);
        }
    }
}

// Picks the second entry with the slider's knob
static BOOL PokedexDetailForm_DragSlider(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                         PokedexDetailCommon *common) {
    PokedexDetailTouchbar *touchbar = PokedexDetailCommon_GetTouchbar(common);
    BOOL moved = FALSE;
    u32 x;
    u32 y;

    if (wk->dragging) {
        if (!GFL_HIDGetTouchHeldPoint(&x, &y)) {
            wk->dragging = FALSE;
            PokedexDetailTouchbar_SetActive(touchbar, TRUE);
        }
    } else if (wk->count >= 3) {
        if (GFL_HIDGetTouchPressedPoint(&x, &y) && x >= 144 && x <= 231 && y >= 40 && y <= 63) {
            wk->dragging = TRUE;
            PokedexDetailTouchbar_SetActive(touchbar, FALSE);
            moved = TRUE;
        }
    }
    if (wk->dragging) {
        u16 next = wk->next;
        ClActorPos pos;

        x = MATH_CLAMP(x, SLIDER_X, SLIDER_X + SLIDER_WIDTH - 1);
        pos.x = x;
        pos.y = SLIDER_KNOB_Y;
        ClActor_SetSurfacePos(wk->actors[ACTOR_SLIDER_KNOB], &pos, CLACT_SURFACE_MAIN);
        wk->next = PokedexDetailForm_GetSliderEntry(param, wk, common, wk->cur,
                                                    PokedexDetailForm_GetSliderIndex(wk->count - 1, x));
        if (next != wk->next) {
            PokedexDetailForm_ChangeNext(param, wk, common);
            moved = TRUE;
        }
    }
    if (moved) {
        GFL_SndSEPlay(SEQ_SE_SYS_06);
    }
    if (wk->dragging) {
        GCTX_HIDSetUsingTouch(TRUE);
    }
    return wk->dragging;
}

static BOOL PokedexDetailForm_SnapSlider(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                         PokedexDetailCommon *common) {
    BOOL snapped = FALSE;

    if (wk->sliderSnap) {
        u8 x;
        ClActorPos pos;

        PokedexDetailForm_GetSliderSpan(
            wk->count - 1, PokedexDetailForm_GetSliderPos(param, wk, common, wk->cur, wk->next), NULL, &x, NULL);
        pos.x = x;
        pos.y = SLIDER_KNOB_Y;
        ClActor_SetSurfacePos(wk->actors[ACTOR_SLIDER_KNOB], &pos, CLACT_SURFACE_MAIN);
        snapped = TRUE;
    }
    return snapped;
}

// The span of the slider for one of count entries, and the knob's place in it
static void PokedexDetailForm_GetSliderSpan(u16 count, u16 index, u8 *start, u8 *center, u8 *end) {
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

static u16 PokedexDetailForm_GetSliderIndex(u16 count, u8 x) {
    u16 i;
    u8 start;
    u8 center;
    u8 end;

    for (i = 0; i < count; i++) {
        PokedexDetailForm_GetSliderSpan(count, i, &start, &center, &end);
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
static u16 PokedexDetailForm_GetSliderPos(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                          PokedexDetailCommon *common, u16 cur, u16 next) {
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

static u16 PokedexDetailForm_GetSliderEntry(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                            PokedexDetailCommon *common, u16 cur, u16 pos) {
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
static void PokedexDetailForm_UpdateSelect(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                           PokedexDetailCommon *common) {
    if (wk->selectMoving == TRUE) {
        wk->selectTime += SELECT_STEP;
        if (wk->selectTime >= SELECT_TIME) {
            wk->selectTime = SELECT_TIME;
            wk->selectMoving = FALSE;
            PokedexDetailForm_SetMode(param, wk, common, FORM_MODE_COMPARE);
            if (wk->count >= 2) {
                VecFx32 pos;

                if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
                    PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_NEXT_FRONT], PLACE_LEFT, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &pos);
                }
                if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
                    PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_NEXT_BACK], PLACE_LEFT, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &pos);
                }
                PokedexDetailForm_GetBesidePos(wk->sprites[wk->front ? SPRITE_NEXT_FRONT : SPRITE_NEXT_BACK].mcss,
                                               &pos);
                if (wk->sprites[SPRITE_CUR_FRONT].mcss != NULL) {
                    PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_FRONT], PLACE_RIGHT, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_CUR_FRONT].mcss, &pos);
                }
                if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
                    PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_BACK], PLACE_RIGHT, &pos);
                    MCSS_SetPosition(wk->sprites[SPRITE_CUR_BACK].mcss, &pos);
                }
                PokedexDetailForm_SwapCurNext(param, wk, common);
                PokedexDetailForm_ChangeIcon(param, wk, common);
                PokedexDetailForm_PrintCur(param, wk, common);
                PokedexDetailForm_PrintNext(param, wk, common);
                PokedexDetailForm_UpdateColorMarks(param, wk, common);
                {
                    u16 species = PokedexDetailCommon_GetSpecies(common);
                    PokeDexSave *pokedex = GameData_GetPokedex(PokedexDetailCommon_GetGameData(common));
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
            PokedexDetailFormPos from;
            PokedexDetailFormPos to;
            VecFx32 pos;

            if (wk->sprites[SPRITE_CUR_FRONT].mcss != NULL) {
                f32 x;
                f32 y;

                PokedexDetailForm_GetSpritePosF32(&wk->sprites[SPRITE_CUR_FRONT], POS_LEFT, &from);
                PokedexDetailForm_GetSpritePosF32(&wk->sprites[SPRITE_CUR_FRONT], POS_RIGHT, &to);
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

                PokedexDetailForm_GetSpritePosF32(&wk->sprites[SPRITE_CUR_BACK], POS_LEFT, &from);
                PokedexDetailForm_GetSpritePosF32(&wk->sprites[SPRITE_CUR_BACK], POS_RIGHT, &to);
                x = from.x + (f32)wk->selectTime * (to.x - from.x) / SELECT_TIME;
                y = from.y + 16.0f * sin;
                pos.x = FX32_CONST(x);
                pos.y = FX32_CONST(y);
                pos.z = FX32_CONST(from.z);
                MCSS_SetPosition(wk->sprites[SPRITE_CUR_BACK].mcss, &pos);
            }
            {
                PokedexDetailFormPos nextTo;
                PokedexDetailFormPos nextFrom;
                VecFx32 nextPos;

                if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
                    f32 x;
                    f32 y;

                    PokedexDetailForm_GetSpritePosF32(&wk->sprites[SPRITE_NEXT_FRONT], POS_LEFT, &nextTo);
                    PokedexDetailForm_GetSpritePosF32(&wk->sprites[SPRITE_NEXT_FRONT], POS_RIGHT, &nextFrom);
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

                    PokedexDetailForm_GetSpritePosF32(&wk->sprites[SPRITE_NEXT_BACK], POS_LEFT, &nextTo);
                    PokedexDetailForm_GetSpritePosF32(&wk->sprites[SPRITE_NEXT_BACK], POS_RIGHT, &nextFrom);
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
static void PokedexDetailForm_StepNext(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                       PokedexDetailCommon *common) {
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

        PokedexDetailForm_FreeSprite(&wk->sprites[SPRITE_NEXT_FRONT], wk->mcssSys);
        PokedexDetailForm_FreeSprite(&wk->sprites[SPRITE_NEXT_BACK], wk->mcssSys);
        frontSprite = wk->front ? SPRITE_NEXT_FRONT : SPRITE_NONE;
        backSprite = wk->front ? SPRITE_NONE : SPRITE_NEXT_BACK;
        PokedexDetailForm_CreateSprites(param, wk, common, frontSprite, backSprite, PLACE_RIGHT, wk->next);
        if (frontSprite != SPRITE_NONE) {
            PokedexDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_FRONT].anim, SPRITE_NEXT_FRONT, wk);
        }
        if (backSprite != SPRITE_NONE) {
            PokedexDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_BACK].anim, SPRITE_NEXT_BACK, wk);
        }
    }
    PokedexDetailForm_PlaceNext(param, wk, common);
}

// Places the second entry off the screen, where it slides in from
static void PokedexDetailForm_PlaceNext(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                        PokedexDetailCommon *common) {
    VecFx32 pos;

    if (!wk->slideFromRight) {
        pos.x = FX32_CONST(-64);
        pos.y = FX32_CONST(-13.9);
        pos.z = 0;
        if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
            PokedexDetailForm_GetSpritePos(&wk->sprites[SPRITE_NEXT_FRONT], POS_LEFT_OUT, &pos);
            MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &pos);
        }
        if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
            PokedexDetailForm_GetSpritePos(&wk->sprites[SPRITE_NEXT_BACK], POS_LEFT_OUT, &pos);
            MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &pos);
        }
    } else {
        pos.x = FX32_CONST(64);
        pos.y = FX32_CONST(-13.9);
        pos.z = 0;
        if (wk->sprites[SPRITE_NEXT_FRONT].mcss != NULL) {
            PokedexDetailForm_GetSpritePos(&wk->sprites[SPRITE_NEXT_FRONT], POS_RIGHT_OUT, &pos);
            MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &pos);
        }
        if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
            PokedexDetailForm_GetSpritePos(&wk->sprites[SPRITE_NEXT_BACK], POS_RIGHT_OUT, &pos);
            MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &pos);
        }
    }
}

// The arrows' slide: both entries move 2 a frame until the second one is in the middle
static void PokedexDetailForm_UpdateSlide(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                          PokedexDetailCommon *common) {
    if (wk->slide == 1) {
        VecFx32 pos;
        PokedexDetailFormPos target;
        BOOL done = FALSE;
        int sprite = wk->front ? SPRITE_NEXT_FRONT : SPRITE_NEXT_BACK;
        f32 targetX;
        f32 x;

        PokedexDetailForm_GetSpritePosF32(&wk->sprites[sprite], POS_CENTER, &target);
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

        PokedexDetailForm_SetMode(param, wk, common, FORM_MODE_SINGLE);
        PokedexDetailForm_SwapCurNext(param, wk, common);
        PokedexDetailForm_StopSpriteAnim(&wk->sprites[SPRITE_NEXT_FRONT]);
        PokedexDetailForm_StopSpriteAnim(&wk->sprites[SPRITE_NEXT_BACK]);
        PokedexDetailForm_ChangeIcon(param, wk, common);
        PokedexDetailForm_PrintCur(param, wk, common);
        PokedexDetailForm_PrintNext(param, wk, common);
        PokedexDetailForm_UpdateColorMarks(param, wk, common);
        {
            u16 species = PokedexDetailCommon_GetSpecies(common);
            PokeDexSave *pokedex = GameData_GetPokedex(PokedexDetailCommon_GetGameData(common));
            u8 sex = wk->entries[wk->cur].sex;

            if (wk->entries[wk->cur].rare) {
                rare = TRUE;
            }
            addToDex(pokedex, species, sex, rare, wk->entries[wk->cur].form);
        }
        if (wk->sprites[SPRITE_CUR_FRONT].mcss != NULL) {
            VecFx32 pos;

            PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_FRONT], PLACE_SINGLE, &pos);
            MCSS_SetPosition(wk->sprites[SPRITE_CUR_FRONT].mcss, &pos);
        }
        if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
            VecFx32 pos;

            PokedexDetailForm_GetPlacePos(&wk->sprites[SPRITE_CUR_BACK], PLACE_SINGLE, &pos);
            MCSS_SetPosition(wk->sprites[SPRITE_CUR_BACK].mcss, &pos);
        }
        wk->slideFromRight = TRUE;
        PokedexDetailForm_StepNext(param, wk, common);
        wk->slide = 0;
    }
}

// The comparison opens with the entry in front moving left and the second one moving in from the right, and closes
// the other way
static void PokedexDetailForm_UpdateOpenClose(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                              PokedexDetailCommon *common) {
    if (wk->count < 2) {
        if (wk->mode == FORM_MODE_OPEN) {
            PokedexDetailForm_SetMode(param, wk, common, FORM_MODE_OPENED);
        } else if (wk->mode == FORM_MODE_CLOSE) {
            PokedexDetailForm_SetMode(param, wk, common, FORM_MODE_CLOSED);
        }
    } else if (wk->mode == FORM_MODE_OPEN) {
        VecFx32 pos;

        if (!wk->curInPlace) {
            int sprite = wk->front ? SPRITE_CUR_FRONT : SPRITE_CUR_BACK;
            PokedexDetailFormPos target;
            f32 targetX;
            f32 x;

            PokedexDetailForm_GetSpritePosF32(&wk->sprites[sprite], POS_LEFT, &target);
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
                    PokedexDetailForm_GetSpritePos(&wk->sprites[SPRITE_CUR_FRONT], POS_LEFT, &place);
                    MCSS_SetPosition(wk->sprites[SPRITE_CUR_FRONT].mcss, &place);
                }
                if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
                    PokedexDetailForm_GetSpritePos(&wk->sprites[SPRITE_CUR_BACK], POS_LEFT, &place);
                    MCSS_SetPosition(wk->sprites[SPRITE_CUR_BACK].mcss, &place);
                }
            }
        }
        if (!wk->nextInPlace) {
            int sprite = wk->front ? SPRITE_NEXT_FRONT : SPRITE_NEXT_BACK;
            PokedexDetailFormPos target;
            f32 targetX;
            f32 x;

            PokedexDetailForm_GetSpritePosF32(&wk->sprites[sprite], POS_RIGHT, &target);
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
                    PokedexDetailForm_GetSpritePos(&wk->sprites[SPRITE_NEXT_FRONT], POS_RIGHT, &place);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &place);
                }
                if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
                    PokedexDetailForm_GetSpritePos(&wk->sprites[SPRITE_NEXT_BACK], POS_RIGHT, &place);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &place);
                }
            }
        }
        if (wk->curInPlace && wk->nextInPlace) {
            PokedexDetailForm_SetMode(param, wk, common, FORM_MODE_OPENED);
        }
    } else if (wk->mode == FORM_MODE_OPENED) {
        PokedexDetailForm_SetMode(param, wk, common, FORM_MODE_COMPARE);
    } else if (wk->mode == FORM_MODE_CLOSE) {
        VecFx32 pos;

        if (!wk->curInPlace) {
            int sprite = wk->front ? SPRITE_CUR_FRONT : SPRITE_CUR_BACK;
            PokedexDetailFormPos target;
            f32 targetX;
            f32 x;

            PokedexDetailForm_GetSpritePosF32(&wk->sprites[sprite], POS_CENTER, &target);
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
                    PokedexDetailForm_GetSpritePos(&wk->sprites[SPRITE_CUR_FRONT], POS_CENTER, &place);
                    MCSS_SetPosition(wk->sprites[SPRITE_CUR_FRONT].mcss, &place);
                }
                if (wk->sprites[SPRITE_CUR_BACK].mcss != NULL) {
                    PokedexDetailForm_GetSpritePos(&wk->sprites[SPRITE_CUR_BACK], POS_CENTER, &place);
                    MCSS_SetPosition(wk->sprites[SPRITE_CUR_BACK].mcss, &place);
                }
            }
        }
        if (!wk->nextInPlace) {
            int sprite = wk->front ? SPRITE_NEXT_FRONT : SPRITE_NEXT_BACK;
            PokedexDetailFormPos target;
            f32 targetX;
            f32 x;

            PokedexDetailForm_GetSpritePosF32(&wk->sprites[sprite], POS_RIGHT_OUT, &target);
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
                    PokedexDetailForm_GetSpritePos(&wk->sprites[SPRITE_NEXT_FRONT], POS_RIGHT_OUT, &place);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_FRONT].mcss, &place);
                }
                if (wk->sprites[SPRITE_NEXT_BACK].mcss != NULL) {
                    PokedexDetailForm_GetSpritePos(&wk->sprites[SPRITE_NEXT_BACK], POS_RIGHT_OUT, &place);
                    MCSS_SetPosition(wk->sprites[SPRITE_NEXT_BACK].mcss, &place);
                }
            }
        }
        if (wk->curInPlace && wk->nextInPlace) {
            PokedexDetailForm_SetMode(param, wk, common, FORM_MODE_CLOSED);
        }
    } else if (wk->mode == FORM_MODE_CLOSED) {
        PokedexDetailForm_SetMode(param, wk, common, FORM_MODE_SINGLE);
    }
}

// Loads the second entry's sprite for the comparison
static void PokedexDetailForm_LoadNext(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                       PokedexDetailCommon *common) {
    if (wk->mode == FORM_MODE_SINGLE) {
        int frontSprite;
        int backSprite;

        PokedexDetailForm_FreeSprite(&wk->sprites[SPRITE_NEXT_FRONT], wk->mcssSys);
        PokedexDetailForm_FreeSprite(&wk->sprites[SPRITE_NEXT_BACK], wk->mcssSys);
        MCSSSys_SetTCBManager(wk->mcssSys, NULL);
        frontSprite = wk->front ? SPRITE_NEXT_FRONT : SPRITE_NONE;
        backSprite = wk->front ? SPRITE_NONE : SPRITE_NEXT_BACK;
        PokedexDetailForm_CreateSprites(param, wk, common, frontSprite, backSprite, PLACE_RIGHT, wk->next);
        if (frontSprite != SPRITE_NONE) {
            PokedexDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_FRONT].anim, SPRITE_NEXT_FRONT, wk);
        }
        if (backSprite != SPRITE_NONE) {
            PokedexDetailForm_InitSpriteAnim(wk->sprites[SPRITE_NEXT_BACK].anim, SPRITE_NEXT_BACK, wk);
        }
        MCSSSys_SetTCBManager(wk->mcssSys, wk->tcbMgr);
    }
}

static void PokedexDetailForm_SwapCurNext(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                          PokedexDetailCommon *common) {
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
        PokedexDetailFormPos front = wk->sprites[SPRITE_CUR_FRONT].pos[i];
        PokedexDetailFormPos back;

        wk->sprites[SPRITE_CUR_FRONT].pos[i] = wk->sprites[SPRITE_NEXT_FRONT].pos[i];
        wk->sprites[SPRITE_NEXT_FRONT].pos[i] = front;
        back = wk->sprites[SPRITE_CUR_BACK].pos[i];
        wk->sprites[SPRITE_CUR_BACK].pos[i] = wk->sprites[SPRITE_NEXT_BACK].pos[i];
        wk->sprites[SPRITE_NEXT_BACK].pos[i] = back;
    }
}

static void PokedexDetailForm_SetActorsOpaque(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                              PokedexDetailCommon *common) {
    u8 i;

    for (i = 0; i < 2; i++) {
        if (wk->iconActors[i] != NULL) {
            ClActor_SetObjMode(wk->iconActors[i], 0);
        }
    }
    ClActor_SetObjMode(wk->actors[ACTOR_SLIDER_BAR], 0);
    ClActor_SetObjMode(wk->actors[ACTOR_SLIDER_KNOB], 0);
    for (i = 0; i < BUTTON_COUNT; i++) {
        ClActor_SetObjMode(wk->buttons[i].actor, 0);
    }
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, 0,
                       GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 |
                           GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ | GX_BLEND_PLANEMASK_BD,
                       12, 4);
    G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG3,
                       GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 |
                           GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ | GX_BLEND_PLANEMASK_BD,
                       12, 4);
}

static void PokedexDetailForm_SetActorsBlended(PokedexDetailFormParam *param, PokedexDetailFormWork *wk,
                                               PokedexDetailCommon *common) {
    u8 i;

    for (i = 0; i < 2; i++) {
        if (wk->iconActors[i] != NULL) {
            ClActor_SetObjMode(wk->iconActors[i], 1);
        }
    }
    ClActor_SetObjMode(wk->actors[ACTOR_SLIDER_BAR], 1);
    ClActor_SetObjMode(wk->actors[ACTOR_SLIDER_KNOB], 1);
    for (i = 0; i < BUTTON_COUNT; i++) {
        ClActor_SetObjMode(wk->buttons[i].actor, 1);
    }
    PokedexDetailBlend_InitPlanes(wk->blendMain);
    PokedexDetailBlend_InitPlanes(wk->blendSub);
}

static void PokedexDetailForm_InitSpritePositions(PokedexDetailFormPos *pos, u32 species, u32 form, u32 sex, u32 rare,
                                                  u32 a6, u32 back, u32 personality) {
    u8 i;
    u16 j;

    for (i = 0; i < POS_COUNT; i++) {
        pos[i] = POKEDEX_DETAIL_FORM_DEFAULT_POSITIONS[i];
    }
    for (j = 0; j < NELEMS(sPokedexDetailFormSpritePositions); j++) {
        const SpritePositions *positions = &sPokedexDetailFormSpritePositions[j];

        if (species == positions->species && form == positions->form && back == positions->back &&
            (positions->sex == 3 || sex == positions->sex)) {
            for (i = 0; i < POS_COUNT; i++) {
                pos[i] = positions->pos[i];
            }
            return;
        }
    }
}

static void PokedexDetailForm_GetSpritePos(Sprite *sprite, int pos, VecFx32 *out) {
    out->x = FX32_CONST(sprite->pos[pos].x);
    out->y = FX32_CONST(sprite->pos[pos].y);
    out->z = FX32_CONST(sprite->pos[pos].z);
}

static void PokedexDetailForm_GetPlacePos(Sprite *sprite, int place, VecFx32 *out) {
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
    PokedexDetailForm_GetSpritePos(sprite, pos, out);
}

static void PokedexDetailForm_GetSpritePosF32(Sprite *sprite, int pos, PokedexDetailFormPos *out) {
    *out = sprite->pos[pos];
}
