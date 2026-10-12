#ifndef POKEBW2_SYSTEM_TEXT_SPEED_H
#define POKEBW2_SYSTEM_TEXT_SPEED_H

#include "types.h"
#include "struct_decls.h"

// The waits between characters that system/printsys.h's streams take. The file these are in has no name in the ROM

// Sets the save control whose options give the text speed
void saveMainSaveAreaAddress(SaveControl *save);
// Returns the wait between characters for the text speed in the save data, or for a text speed from 0 to 4
s32 func_02017bcc(void);
s32 func_02017c50(u32 speed);
// The waits of the two faster speeds
s32 func_02017bf0(void);
s32 func_02017c20(void);

#endif // POKEBW2_SYSTEM_TEXT_SPEED_H
