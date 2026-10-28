#ifndef POKEBW2_FIELD_PLACE_NAME_LETTER_H
#define POKEBW2_FIELD_PLACE_NAME_LETTER_H

// Overlay 36's place_name_letter.c: the two animated parts of the place name banner, the zone's name and its frame
// with the season's icon. Create one of a type, free it, set it up, update it, slide it in or out from a start frame
// over a number of frames, hide it, and whether it has finished

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"
#include "system/printsys.h"

// What the name's part is created with: the OAM system of its text, the text's palette and the print queue
typedef struct {
    BmpOamSys *bmpOam;
    u16 palette;
    PrintQueue *printQueue;
} PlaceNameTextSetup;

// What the frame's part is created with: the graphics of its icon
typedef struct {
    u16 palette;
    u16 chars;
    u16 cellAnims;
} PlaceNameFrameSetup;

// What the name's part is set up with: the font, the zone and its name. The frame's part is set up with a u32 season
typedef struct {
    Font *font;
    u16 zoneId;
    StrBuf *name;
} PlaceNameTextParam;

// The parts, by type
enum {
    PLACE_NAME_LETTER_TEXT,
    PLACE_NAME_LETTER_FRAME,
};

PlaceNameLetter *PlaceNameLetter_Create(HeapID heapId, u32 type, void *setup, ClActUnit *unit, ArcTool *arc,
                                        u16 bgPriority, u16 palette, u16 chars, u16 cellAnims);
void PlaceNameLetter_Delete(PlaceNameLetter *letter);
void PlaceNameLetter_SetParam(PlaceNameLetter *letter, void *setup);
void PlaceNameLetter_Update(PlaceNameLetter *letter);
void PlaceNameLetter_SlideIn(PlaceNameLetter *letter, s16 startFrame, u16 frames);
void PlaceNameLetter_SlideOut(PlaceNameLetter *letter, s16 startFrame, u16 frames);
void PlaceNameLetter_Hide(PlaceNameLetter *letter);
BOOL PlaceNameLetter_IsFinished(PlaceNameLetter *letter);

#endif // POKEBW2_FIELD_PLACE_NAME_LETTER_H
