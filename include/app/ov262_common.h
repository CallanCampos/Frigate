#ifndef POKEBW2_APP_OV262_COMMON_H
#define POKEBW2_APP_OV262_COMMON_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "system/printsys.h"

// Overlay 262, the code that the live and Wi-Fi competition screens of overlay 261 (battle_championship_flow.c,
// digitalcardcheck.c) share: a task runner, a message box, a list menu, the net link and the digital card. It has no
// source file here yet, so its functions keep their addresses; only the ones that overlay 261 calls are declared, with
// types of ours

typedef struct Ov262Task Ov262Task;
typedef struct Ov262MsgBox Ov262MsgBox;
typedef struct Ov262Menu Ov262Menu;
typedef struct Ov262Net Ov262Net;
typedef struct Ov262NetLink Ov262NetLink;
typedef struct Ov262Seq Ov262Seq;
typedef struct Ov262Card Ov262Card;

// A step of a task: seq is the step it is in, and work is the work that the task was created with
typedef void (*Ov262TaskFunc)(Ov262Task *task, u32 *seq, void *work);

// What Ov262Menu is made from: a list of messages of msgData, with the cursor and its window
typedef struct {
    MsgData *msgData;
    Font *font;
    PrintQueue *printQueue;
    u32 msgIds[4];
    u32 count;
    u16 unk20;
    u16 unk22;
    u16 unk24;
    u16 unk26;
    u32 unk28;
    u16 unk2C;
    u16 unk2E;
} Ov262MenuParam;

// What the digital card is made from, for the Wi-Fi competition and for the live one
typedef struct {
    // The entry's data
    u8 data[0x92];
    u8 unk92[2];
    u32 unk94;
    u32 unk98;
    u32 unk9C;
    u16 unkA0;
    u8 unkA2[2];
    u32 unkA4;
    u8 unkA8[2];
    u16 unkAA;
} Ov262WifiCardInfo;

typedef struct {
    u8 data[0x92];
    u8 unk92[2];
    u32 unk94;
    u32 unk98;
    u32 unk9C;
    u16 unkA0;
    u16 unkA2;
    u16 unkA4;
    u16 unkA6;
    u16 unkA8;
    u8 unkAA[2];
} Ov262LiveCardInfo;

// Tasks
Ov262Task *func_ov262_021c5c10(void *work, Ov262TaskFunc func, HeapID heapId);
void func_ov262_021c5c4c(Ov262Task *task);
// Runs the current step
void func_ov262_021c5c54(Ov262Task *task);
// Is TRUE once the task has ended
BOOL func_ov262_021c5c64(Ov262Task *task);
// Makes func the step that runs next
void func_ov262_021c5c74(Ov262Task *task, Ov262TaskFunc func);
// Ends the task
void func_ov262_021c5c7c(Ov262Task *task);
// Remembers the step to go back to, which func_ov262_021c5c8c goes to
void func_ov262_021c5c88(Ov262Task *task, u32 seq);
void func_ov262_021c5c8c(Ov262Task *task);

// The net link
Ov262Net *func_ov262_021c2560(BOOL a0, HeapID heapId);
void func_ov262_021c2600(Ov262Net *net);
void func_ov262_021c2654(Ov262Net *net);
Ov262NetLink *func_ov262_021c2668(Ov262Net *net);
Ov262Seq *func_ov262_021c279c(Ov262NetLink *link, u32 a1, HeapID heapId);
void func_ov262_021c2988(Ov262Seq *seq);
void func_ov262_021c29c4(Ov262Seq *seq, u32 a1, HeapID heapId);
void func_ov262_021c2aa0(Ov262Seq *seq);

// The message box
Ov262MsgBox *func_ov262_021c568c(u32 a0, u32 a1, u32 a2, u32 a3, PrintQueue *printQueue, Font *font, HeapID heapId);
void func_ov262_021c5750(Ov262MsgBox *box);
void func_ov262_021c57a4(Ov262MsgBox *box);
// Prints a message of msgData
void func_ov262_021c5888(Ov262MsgBox *box, MsgData *msgData, u32 msgId, u32 a3);
void func_ov262_021c58a4(Ov262MsgBox *box, StrBuf *text, u32 a2);
void func_ov262_021c58bc(Ov262MsgBox *box);
// Is TRUE once the message is printed
BOOL func_ov262_021c5a1c(Ov262MsgBox *box);
void func_ov262_021c5a48(Ov262MsgBox *box);

// The list menu
Ov262Menu *func_ov262_021c5a88(const Ov262MenuParam *param, u8 x, u8 y, u8 width, u8 height, HeapID heapId);
void func_ov262_021c5ba4(Ov262Menu *menu);
// The item picked, or -1 while none is
s32 func_ov262_021c5bd4(Ov262Menu *menu);

// The digital card. The first of each pair is for the Wi-Fi competition's, the second for the live one's
Ov262Card *func_ov262_021c2e8c(const Ov262WifiCardInfo *info, u32 a1, PlayerInfo *playerInfo, Ov262NetLink *link,
                               Ov262Seq *seq, Font *font, PrintQueue *printQueue, MsgData *msgData, WordSet *wordSet,
                               void *unk, u32 a9, HeapID heapId);
Ov262Card *func_ov262_021c323c(const Ov262LiveCardInfo *info, PlayerInfo *playerInfo, Ov262NetLink *link, Ov262Seq *seq,
                               Font *font, PrintQueue *printQueue, MsgData *msgData, WordSet *wordSet, void *unk,
                               u32 a9, HeapID heapId);
void func_ov262_021c301c(Ov262Card *card);
void func_ov262_021c33c4(Ov262Card *card);
void func_ov262_021c3048(Ov262Card *card, u32 page, MsgData *msgData, PrintQueue *printQueue, Font *font,
                         HeapID heapId);
void func_ov262_021c33f0(Ov262Card *card, u32 page, MsgData *msgData, PrintQueue *printQueue, Font *font,
                         HeapID heapId);
void func_ov262_021c35e4(Ov262Card *card, PrintQueue *printQueue);
// Is TRUE once the card is shown
BOOL func_ov262_021c35ec(Ov262Card *card);
BOOL func_ov262_021c3720(Ov262Card *card);

#endif // POKEBW2_APP_OV262_COMMON_H
