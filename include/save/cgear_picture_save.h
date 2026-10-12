#ifndef POKEBW2_SAVE_CGEAR_PICTURE_SAVE_H
#define POKEBW2_SAVE_CGEAR_PICTURE_SAVE_H

#include "types.h"

// The downloaded C-Gear skin, extra save block 4. Its size, func_0200ce50, is declared in save/save_control.h

typedef struct {
    u8 chars[0x2000];
    u8 unk2000[0x600];
} CGearPictureSave; // size: 0x2600

void func_0200ce58(CGearPictureSave *picture);
// Whether the skin's last tile has any pixel, which an empty block doesn't
BOOL func_0200ce5c(CGearPictureSave *picture);

#endif // POKEBW2_SAVE_CGEAR_PICTURE_SAVE_H
