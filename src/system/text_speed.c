#include "system/text_speed.h"
#include "types.h"
#include "save/config.h"
#include "save/trainer_data_savedata.h"

// The waits between characters that the text speeds give. The file has no name in the ROM: text_speed.c is a guess
// from what its functions do. It follows game_data.c and keeps its own pointer to the save control

#define TEXT_SPEED_MAX 5

// The original has this in .data, as an explicit zero
#pragma explicit_zero_data on
static SaveControl *sSaveControl = NULL;
#pragma explicit_zero_data reset

// A negative wait prints that many characters a frame
static const s8 sTextSpeedWaits[TEXT_SPEED_MAX] = { 3, 1, -2, -4, 6 };

void saveMainSaveAreaAddress(SaveControl *save) {
    sSaveControl = save;
}

s32 func_02017bcc(void) {
    u32 speed = func_02008a14(&getTrainerDataBlkAddress(sSaveControl)->config);

    if (speed >= TEXT_SPEED_MAX) {
        speed = TEXT_SPEED_MAX - 1;
    }
    return sTextSpeedWaits[speed];
}

s32 func_02017bf0(void) {
    u32 speed = func_02008a14(&getTrainerDataBlkAddress(sSaveControl)->config);

    switch (speed) {
    case 0:
        speed = 1;
        break;
    case 1:
        speed = 2;
        break;
    case 2:
        speed = 3;
        break;
    }
    return func_02017c50(speed);
}

s32 func_02017c20(void) {
    u32 speed = func_02008a14(&getTrainerDataBlkAddress(sSaveControl)->config);

    switch (speed) {
    case 0:
        speed = 4;
        break;
    case 1:
        speed = 0;
        break;
    case 2:
        speed = 1;
        break;
    }
    return func_02017c50(speed);
}

s32 func_02017c50(u32 speed) {
    if (speed >= TEXT_SPEED_MAX) {
        speed = TEXT_SPEED_MAX - 1;
    }
    return sTextSpeedWaits[speed];
}
