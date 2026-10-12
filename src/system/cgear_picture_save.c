#include "types.h"
#include "save/cgear_picture_save.h"
#include "gfl/std.h"
#include "save/save_control.h"

// The downloaded C-Gear skin, extra save block 4. The ROM has no string for the file, so the name is a guess

u32 func_0200ce50(void) {
    return sizeof(CGearPictureSave);
}

void func_0200ce58(CGearPictureSave *picture) {
}

BOOL func_0200ce5c(CGearPictureSave *picture) {
    u8 empty[0x20];
    BOOL valid = FALSE;

    sys_memset(empty, 0, sizeof(empty));
    if (GFL_STD_MemCmp(empty, &picture->chars[sizeof(picture->chars) - sizeof(empty)], sizeof(empty))) {
        valid = TRUE;
    }
    return valid;
}
