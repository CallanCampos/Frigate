#ifndef POKEBW2_BATTLE_B_CHOICE_MAIN_H
#define POKEBW2_BATTLE_B_CHOICE_MAIN_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The battle's choice screen in overlay 289 (b_choice_main.c, the ROM's embedded string): two to four choices on the
// lower screen, one button each, chosen with the touch screen or the keys, which flash when chosen. It sets done when
// it ends. Every name here is ours; swan has none for this overlay.

// What the battle passes to the choice screen, 0x24 bytes, inside BtlvCore
typedef struct {
    Font *font;      // 0x00
    HeapID heapId;   // 0x04
    u8 done;         // 0x06
    u8 unk07;        // 0x07
    int count;       // 0x08  the number of choices, 2 to 4
    StrBuf *strs[4]; // 0x0c  the text of each choice, or NULL
    u32 *result;     // 0x1c  out, the choice made, or NULL
    BOOL skip;       // 0x20  1 chooses the first at once
} BChoiceParam;

// Starts the choice screen, which runs as a task
void BChoiceMain_Start(BChoiceParam *param);

#endif // POKEBW2_BATTLE_B_CHOICE_MAIN_H
