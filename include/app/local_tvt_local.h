#ifndef POKEBW2_APP_LOCAL_TVT_LOCAL_H
#define POKEBW2_APP_LOCAL_TVT_LOCAL_H

// The parts of the Xtransceiver's calls with the game's characters (overlay 259), one per file of the ROM's
// "local_tvt_*.c". Every name here is ours

#include "types.h"
#include "app/xtransceiver.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "struct_decls.h"
#include "system/gf_font.h"
#include "system/printsys.h"

#define HEAPID_LOCAL_TVT 85

// The archive of the faces, backgrounds and objects of the calls
#define ARC_LOCAL_TVT 132
// The communication Xtransceiver's archive, which has the frame, the cursors and the lower screen
#define ARC_LOCAL_TVT_COMMON 170

// At most this many faces are on the upper screen
#define LOCAL_TVT_FACES 4

// The persons of a call. The player is LOCAL_TVT_PERSON_PLAYER_FEMALE when female
#define LOCAL_TVT_PERSON_PLAYER 0
#define LOCAL_TVT_PERSON_RIVAL 1
#define LOCAL_TVT_PERSON_PLAYER_FEMALE 13
#define LOCAL_TVT_PERSON_COUNT 14
#define LOCAL_TVT_PERSON_NONE 0xff

// A face has no object beside it
#define LOCAL_TVT_OBJECT_NONE 3
// The name of a face is the person's own
#define LOCAL_TVT_NAME_DEFAULT 0xff

// The part of the day, which picks the backgrounds of the faces
enum {
    LOCAL_TVT_TIME_DAY,
    LOCAL_TVT_TIME_EVENING,
    LOCAL_TVT_TIME_NIGHT,
    LOCAL_TVT_TIME_COUNT,
};

typedef struct LocalTvtGraphic LocalTvtGraphic;
typedef struct LocalTvtMessage LocalTvtMessage;
typedef struct LocalTvtChara LocalTvtChara;
typedef struct LocalTvtFace LocalTvtFace;
typedef struct LocalTvtBG LocalTvtBG;
typedef struct LocalTvtScriptMode LocalTvtScriptMode;
typedef struct LocalTvtCallMode LocalTvtCallMode;
typedef struct LocalTvtCallData LocalTvtCallData;
typedef struct LocalTvtCallSelect LocalTvtCallSelect;
typedef struct LocalTvtPerson LocalTvtPerson;
typedef struct LocalTvtSeq LocalTvtSeq;
typedef struct LocalTvtMenu LocalTvtMenu;
typedef struct LocalTvtPokeSelect LocalTvtPokeSelect;

// The persons the player can call, which the save data of the calls counts
enum {
    LOCAL_TVT_CALL_DOCTOR,
    LOCAL_TVT_CALL_RIVAL,
    LOCAL_TVT_CALL_CHELEN,
    LOCAL_TVT_CALL_SUPPORT,
    LOCAL_TVT_CALL_MOTHER,
    LOCAL_TVT_CALL_SECRET,
    LOCAL_TVT_CALL_COUNT,
};

// local_tvt_chara.c: the faces of the upper screen

// What a face shows
typedef struct {
    u16 person;
    u16 background;
    u16 object;
    // A message of the names' bank, or LOCAL_TVT_NAME_DEFAULT
    u16 nameMsg;
} LocalTvtFaceSetup;

typedef struct {
    ClActUnit *unit;
    PlayerInfo *playerInfo;
    RivalDataSave *rival;
    LocalTvtMessage *message;
    u32 time;
} LocalTvtCharaSetup;

LocalTvtChara *LocalTvtChara_Create(const LocalTvtCharaSetup *setup, HeapID heapId);
void LocalTvtChara_Free(LocalTvtChara *chara);
void LocalTvtChara_Update(LocalTvtChara *chara);
// Starts loading the faces of a call, one step a frame, until LocalTvtChara_IsReady
void LocalTvtChara_Start(LocalTvtChara *chara, const LocalTvtFaceSetup *setups, int count);
BOOL LocalTvtChara_IsReady(LocalTvtChara *chara);
void LocalTvtChara_SetVisible(LocalTvtChara *chara, BOOL visible);
LocalTvtFace *LocalTvtChara_GetFace(LocalTvtChara *chara, int index);
// The face says a message, and LocalTvtFace_UpdateTalk returns TRUE once it is over. With a key wait the last page
// stays until the player presses a key
void LocalTvtFace_Talk(LocalTvtFace *face, MsgData *msgData, u16 messageId);
void LocalTvtFace_TalkKeyWait(LocalTvtFace *face, MsgData *msgData, u16 messageId);
BOOL LocalTvtFace_UpdateTalk(LocalTvtFace *face, LocalTvtChara *chara);

// local_tvt_graphic.c

LocalTvtGraphic *LocalTvtGraphic_Create(u32 layout, HeapID heapId);
void LocalTvtGraphic_Free(LocalTvtGraphic *graphic);
void LocalTvtGraphic_Update(LocalTvtGraphic *graphic);
ClActUnit *LocalTvtGraphic_GetClActUnit(LocalTvtGraphic *graphic);

// local_tvt_message.c: the fonts, message banks and windows

// How LocalTvtMessage_SetPosition places the text
enum {
    LOCAL_TVT_ALIGN_LEFT,
    LOCAL_TVT_ALIGN_CENTER,
};

// Where the window of a talk is
enum {
    LOCAL_TVT_TALK_TOP,
    LOCAL_TVT_TALK_BOTTOM,
};

LocalTvtMessage *LocalTvtMessage_Create(HeapID heapId);
void LocalTvtMessage_Free(LocalTvtMessage *message);
void LocalTvtMessage_Update(LocalTvtMessage *message);
Font *LocalTvtMessage_GetFont(LocalTvtMessage *message);
PrintQueue *LocalTvtMessage_GetPrintQueue(LocalTvtMessage *message);
MsgData *LocalTvtMessage_GetMenuMsgData(LocalTvtMessage *message);
MsgData *LocalTvtMessage_GetTalkMsgData(LocalTvtMessage *message);
// The color and position of the next LocalTvtMessage_Print, which resets them
void LocalTvtMessage_SetColor(LocalTvtMessage *message, u16 color);
void LocalTvtMessage_SetPosition(LocalTvtMessage *message, s16 x, s16 y, u32 align);
void LocalTvtMessage_PrintMsg(LocalTvtMessage *message, BmpWin *window, MsgData *msgData, u32 messageId);
void LocalTvtMessage_Print(LocalTvtMessage *message, BmpWin *window, const StrBuf *strbuf);
// The nickname, level and gender of a party Pokémon
void LocalTvtMessage_PrintPokemon(LocalTvtMessage *message, BmpWin *window, PartyPkm *pkm);
void LocalTvtMessage_DrawFrame(LocalTvtMessage *message, BmpWin *window);
void LocalTvtMessage_ClearFrame(LocalTvtMessage *message, BmpWin *window);
// The words of the messages
void LocalTvtMessage_SetPlayerName(LocalTvtMessage *message, GameData *gameData);
void LocalTvtMessage_SetSurveyAnswer(LocalTvtMessage *message, u32 index, u16 answer);
void LocalTvtMessage_SetPokemonNickname(LocalTvtMessage *message, PartyPkm *pkm, u32 index);
void LocalTvtMessage_SetWord(LocalTvtMessage *message, MsgData *msgData, u16 messageId, u16 index);
void LocalTvtMessage_SetPlaceName(LocalTvtMessage *message, GameData *gameData, u32 index);
void LocalTvtMessage_SetNumber(LocalTvtMessage *message, s32 number, u32 digits, u32 index);
// The talk of a face: its window at the top or bottom, and the message printed a character at a time
void LocalTvtMessage_OpenTalkWindow(LocalTvtMessage *message, u32 position);
void LocalTvtMessage_StartTalk(LocalTvtMessage *message, MsgData *msgData, u32 messageId, u32 keyWait);
BOOL LocalTvtMessage_IsTalkFinished(LocalTvtMessage *message);
BOOL LocalTvtMessage_IsTalkPaused(LocalTvtMessage *message);
BOOL LocalTvtMessage_IsTalkDone(LocalTvtMessage *message);
// Lets the player close the last page of a talk with a key wait
void LocalTvtMessage_AllowTalkEnd(LocalTvtMessage *message);

// local_tvt_scriptmode.c: plays a call's script

LocalTvtScriptMode *LocalTvtScriptMode_Create(HeapID heapId, GameSystem *gsys, GameData *gameData, u16 scriptId,
                                              LocalTvtChara *chara);
void LocalTvtScriptMode_Free(LocalTvtScriptMode *mode);
BOOL LocalTvtScriptMode_Main(LocalTvtScriptMode *mode);

// local_tvt_callmode.c: the calls the player makes

// Whether the player picks the person, or the call is with LOCAL_TVT_CALL_SECRET at once
enum {
    LOCAL_TVT_CALL_MODE_SELECT,
    LOCAL_TVT_CALL_MODE_SECRET,
    LOCAL_TVT_CALL_MODE_2,
};

typedef struct {
    GameSystem *gsys;
    GameData *gameData;
    LocalTvtChara *chara;
    LocalTvtMessage *message;
    ClActUnit *unit;
    // LOCAL_TVT_CALL_MODE_*
    u32 mode;
} LocalTvtCallModeSetup;

LocalTvtCallMode *LocalTvtCallMode_Create(const LocalTvtCallModeSetup *setup, HeapID heapId);
void LocalTvtCallMode_Free(LocalTvtCallMode *mode);
BOOL LocalTvtCallMode_Main(LocalTvtCallMode *mode);

// local_tvt_callselect.c: the list of the persons to call on the lower screen

#define LOCAL_TVT_SELECT_MAX 6

// The results of the list
enum {
    LOCAL_TVT_SELECT_NONE,
    LOCAL_TVT_SELECT_CALL,
    LOCAL_TVT_SELECT_EXIT,
};

typedef struct {
    RivalDataSave *rival;
    LocalTvtMessage *message;
    ClActUnit *unit;
    MsgData *msgData;
    // The messages of the names
    u32 names[LOCAL_TVT_SELECT_MAX];
    BOOL enabled[LOCAL_TVT_SELECT_MAX];
    int count;
} LocalTvtCallSelectSetup;

LocalTvtCallSelect *LocalTvtCallSelect_Create(const LocalTvtCallSelectSetup *setup, HeapID heapId);
void LocalTvtCallSelect_Free(LocalTvtCallSelect *select);
void LocalTvtCallSelect_Main(LocalTvtCallSelect *select);
u32 LocalTvtCallSelect_GetResult(LocalTvtCallSelect *select);
int LocalTvtCallSelect_GetSelected(LocalTvtCallSelect *select);

// local_tvt_calldata.c: the table of the calls and the save data of the calls made

LocalTvtCallData *LocalTvtCallData_Create(GameSystem *gsys, GameData *gameData, HeapID heapId);
void LocalTvtCallData_Free(LocalTvtCallData *data);
// The background and the object of the face of a person the player calls, for the calls made so far
u32 LocalTvtCallData_GetBackground(LocalTvtCallData *data, u16 person);
u32 LocalTvtCallData_GetObject(LocalTvtCallData *data, u16 person);
u32 LocalTvtCallData_GetBadgeCount(LocalTvtCallData *data);
GameData *LocalTvtCallData_GetGameData(LocalTvtCallData *data);
// The messages of the call with a person, for the calls made so far
u16 LocalTvtCallData_GetGreetingMsg(LocalTvtCallData *data, u16 person);
u16 LocalTvtCallData_GetChatMsg(LocalTvtCallData *data, u16 person);
u16 LocalTvtCallData_GetByeMsg(LocalTvtCallData *data, u16 person);
u16 LocalTvtCallData_GetExtraMsg(LocalTvtCallData *data, u16 person);
BOOL LocalTvtCallData_CanCall(LocalTvtCallData *data, u16 person);
BOOL LocalTvtCallData_IsListed(LocalTvtCallData *data, u16 person);
// The background and the object of a face in a zone, by its battle background and weather
u32 LocalTvtCallData_GetZoneBackground(LocalTvtCallData *data, u16 zoneId);
u32 LocalTvtCallData_GetZoneObject(LocalTvtCallData *data, u16 zoneId);
// The calls made with a person, which count up to a maximum
u16 LocalTvtCallData_GetCallCount(LocalTvtCallData *data, u16 person);
BOOL LocalTvtCallData_IsPlayerMale(LocalTvtCallData *data);
void LocalTvtCallData_AddCall(LocalTvtCallData *data, u16 person, u32 max);
void LocalTvtCallData_Set0200ff68(LocalTvtCallData *data, u32 value);
void LocalTvtCallData_Set0200ff78(LocalTvtCallData *data);
BOOL LocalTvtCallData_Get0200ff8c(LocalTvtCallData *data);
void LocalTvtCallData_Reset0200ffcc(LocalTvtCallData *data);
BOOL LocalTvtCallData_IsSecretUnknown(LocalTvtCallData *data);

// local_tvt_person.c: the call with a person, who greets and then talks about the topic the player picks

#define LOCAL_TVT_TOPIC_MAX 4

typedef struct {
    LocalTvtMessage *message;
    LocalTvtChara *chara;
    ClActUnit *unit;
    LocalTvtCallData *callData;
} LocalTvtPersonSetup;

// A step of a call, which returns TRUE once it is over. The work starts with the step's own counter
typedef BOOL (*LocalTvtPersonFunc)(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);

// What each local_tvt_person_*.c gives of its person
typedef struct {
    void *work;
    LocalTvtPersonFunc greet;
    LocalTvtPersonFunc topics[LOCAL_TVT_TOPIC_MAX];
    void (*free)(LocalTvtPerson *person, const LocalTvtPersonSetup *setup, void *work);
    // The messages of the topics in the menu
    u32 topicNames[LOCAL_TVT_TOPIC_MAX];
    u32 topicCount;
    // The person's question before the menu
    u32 menuMsg;
    MsgData *msgData;
} LocalTvtPersonFuncs;

LocalTvtPerson *LocalTvtPerson_Create(const LocalTvtPersonFuncs *funcs, const LocalTvtPersonSetup *setup,
                                      HeapID heapId);
void LocalTvtPerson_Free(LocalTvtPerson *person);
void LocalTvtPerson_Main(LocalTvtPerson *person);
BOOL LocalTvtPerson_IsEnd(LocalTvtPerson *person);
// Ends the call; the topic that says goodbye calls it
void LocalTvtPerson_End(LocalTvtPerson *person);

// local_tvt_person_*.c
void LocalTvtSupport_Setup(LocalTvtPersonFuncs *funcs, MsgData *msgData, GameData *gameData, HeapID heapId);
void LocalTvtMother_Setup(LocalTvtPersonFuncs *funcs, LocalTvtMessage *message, MsgData *msgData, GameData *gameData,
                          HeapID heapId);
void LocalTvtDoctor_Setup(LocalTvtPersonFuncs *funcs, MsgData *msgData, GameData *gameData, HeapID heapId);
void LocalTvtChelen_Setup(LocalTvtPersonFuncs *funcs, MsgData *msgData, GameData *gameData, HeapID heapId);
void LocalTvtRival_Setup(LocalTvtPersonFuncs *funcs, MsgData *msgData, GameData *gameData, HeapID heapId);
void LocalTvtSecret_Setup(LocalTvtPersonFuncs *funcs, MsgData *msgData, GameData *gameData, LocalTvtCallData *callData,
                          BOOL unk, HeapID heapId);
// The face of the person, which depends on the player's gender
u16 LocalTvtSecret_GetFacePerson(LocalTvtCallData *data);
// The message of the name of the person, whose name the player learns later
u16 LocalTvtSecret_GetNameMsg(LocalTvtCallData *data);
// How often the Xtransceiver rings before the person answers
u32 LocalTvtSecret_GetRingCount(LocalTvtCallData *data);
// The stage of the calls with the person for a count of calls, and what a stage has
u32 LocalTvtSecret_GetStage(int callCount);
u32 LocalTvtSecret_GetStage0378(u32 stage);
u32 LocalTvtSecret_GetStageCount(u32 stage);

// local_tvt_util.c

// The part of the day by the season's hours of light
u32 LocalTvtUtil_GetTime(GameData *gameData);
// The backgrounds of both screens
LocalTvtBG *LocalTvtBG_Create(GameData *gameData, HeapID heapId, u32 unk04);
void LocalTvtBG_Free(LocalTvtBG *bg);

// A sequence of functions with a step each, until one ends it
typedef void (*LocalTvtSeqFunc)(LocalTvtSeq *seq, int *step, void *work);

LocalTvtSeq *LocalTvtSeq_Create(void *work, LocalTvtSeqFunc func, HeapID heapId);
void LocalTvtSeq_Free(LocalTvtSeq *seq);
void LocalTvtSeq_Main(LocalTvtSeq *seq);
BOOL LocalTvtSeq_IsEnd(LocalTvtSeq *seq);
void LocalTvtSeq_SetNext(LocalTvtSeq *seq, LocalTvtSeqFunc func);
void LocalTvtSeq_End(LocalTvtSeq *seq);

// The list of the party on the lower screen, of which the player picks a Pokémon
typedef struct {
    LocalTvtMessage *message;
    ClActUnit *unit;
    PokeParty *party;
} LocalTvtPokeSelectSetup;

// The results of the list
enum {
    LOCAL_TVT_POKE_SELECT_NONE,
    LOCAL_TVT_POKE_SELECT_DECIDE,
    LOCAL_TVT_POKE_SELECT_CANCEL,
};

LocalTvtPokeSelect *LocalTvtPokeSelect_Create(const LocalTvtPokeSelectSetup *setup, HeapID heapId);
void LocalTvtPokeSelect_Free(LocalTvtPokeSelect *select);
void LocalTvtPokeSelect_Main(LocalTvtPokeSelect *select);
void LocalTvtPokeSelect_SetActive(LocalTvtPokeSelect *select, BOOL active);
void LocalTvtPokeSelect_Restart(LocalTvtPokeSelect *select);
u32 LocalTvtPokeSelect_GetResult(LocalTvtPokeSelect *select);
PartyPkm *LocalTvtPokeSelect_GetPkm(LocalTvtPokeSelect *select);

// The menu of topics on the lower screen
typedef struct {
    LocalTvtMessage *message;
    ClActUnit *unit;
    MsgData *msgData;
    u32 items[LOCAL_TVT_TOPIC_MAX];
    u32 count;
    u32 cursor;
} LocalTvtMenuSetup;

LocalTvtMenu *LocalTvtMenu_Create(const LocalTvtMenuSetup *setup, HeapID heapId);
void LocalTvtMenu_Free(LocalTvtMenu *menu);
void LocalTvtMenu_Main(LocalTvtMenu *menu);
BOOL LocalTvtMenu_IsEnd(LocalTvtMenu *menu);
u32 LocalTvtMenu_GetSelected(LocalTvtMenu *menu);

// The touch bar of the lower screen with its return button, which the B button presses as well
typedef struct LocalTvtReturnBar LocalTvtReturnBar;

LocalTvtReturnBar *LocalTvtReturnBar_Create(ClActUnit *unit, HeapID heapId);
void LocalTvtReturnBar_Free(LocalTvtReturnBar *bar);
void LocalTvtReturnBar_Main(LocalTvtReturnBar *bar);
BOOL LocalTvtReturnBar_IsPressed(LocalTvtReturnBar *bar);
// Whether the button's animation of being pressed is over
BOOL LocalTvtReturnBar_IsAnimEnd(LocalTvtReturnBar *bar);

// The input of a list by the keys or the touch screen, whichever the player used last. The cursor shows only with
// the keys
typedef struct LocalTvtKeyTouch LocalTvtKeyTouch;

// What the callbacks of the input return
enum {
    LOCAL_TVT_INPUT_NONE,
    // The cursor moved, or the touch only picked an entry
    LOCAL_TVT_INPUT_MOVE,
    LOCAL_TVT_INPUT_DECIDE,
};

typedef struct {
    void *work;
    int (*touch)(void *work);
    int (*key)(void *work);
    // Draws the list with or without its cursor
    void (*draw)(void *work, BOOL touchMode);
} LocalTvtKeyTouchSetup;

LocalTvtKeyTouch *LocalTvtKeyTouch_Create(const LocalTvtKeyTouchSetup *setup, HeapID heapId);
void LocalTvtKeyTouch_Free(LocalTvtKeyTouch *input);
void LocalTvtKeyTouch_Main(LocalTvtKeyTouch *input);
BOOL LocalTvtKeyTouch_IsDecided(LocalTvtKeyTouch *input);

#endif // POKEBW2_APP_LOCAL_TVT_LOCAL_H
