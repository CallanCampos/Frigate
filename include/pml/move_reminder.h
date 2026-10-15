#ifndef POKEBW2_PML_MOVE_REMINDER_H
#define POKEBW2_PML_MOVE_REMINDER_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The move reminder's screen, overlay 258
#define OVERLAY_MOVE_REMINDER OVERLAY_ID(258)

// What the screen is for, which picks its messages
typedef enum {
    MOVE_REMINDER_MODE_TEACH,
    MOVE_REMINDER_MODE_REMEMBER,
} MoveReminderMode;

// How the screen was left
enum {
    MOVE_REMINDER_STATUS_LEARNED,
    MOVE_REMINDER_STATUS_CANCELLED,
};

struct MoveReminderProcessData {
    PartyPkm *pkm;
    PlayerInfo *playerInfo;
    // Passed on to the summary screen
    TrainerDataSave *trainerData;
    GameSystem *gsys;
    // The moves to pick from, ended by 0xffff
    u16 *moves;
    // The cursor's row in the list and the list's first row, kept while the summary screen is shown
    u16 cursorPos;
    u16 scroll;
    u8 unk18;
    u8 mode;
    u8 status;
    // The slot of the move that the new one replaces, 4 if none was picked
    u8 slot;
};

MoveReminderProcessData *func_ov012_02169c7c(HeapID heapId);
void func_ov012_02169ca4(MoveReminderProcessData *data);

// The screen's process (waza_oshie_main.c)
extern const GameProcFunctions WazaOshieProcFunctions;
// The same, under the name its caller still uses
extern const GameProcFunctions WazaOshieProcFunctions;

#endif // POKEBW2_PML_MOVE_REMINDER_H
