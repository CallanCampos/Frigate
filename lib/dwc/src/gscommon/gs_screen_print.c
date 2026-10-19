#include "dwc/gs_screen_print.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// The screen that GameSpy's debug output prints to, as lines of characters for a text background, and the clock in
// seconds. The ROM names neither the file nor its functions: the file's name is ours, for what it does, and so are the
// names of the screen's functions; gsiTimeInSec is GameSpy's, as pret's pokerevo Wii build has it (nonport.s)

static void ScreenScrollUp(void);
static void ScreenPutChar(int c);

// The text being printed, the screen's 24 lines of 32 characters, and where the next character goes in the last line
static char sText[0x800];
static u16 sScreen[24 * 32];
static int sCursor;

// Whether the text is printed (bit 0), which it is not
static int sScreenFlags = 2;

// Moves every line up one and clears the last one
static void ScreenScrollUp(void) {
    int i;

    for (i = 0; i < 23; i++) {
        MIi_CpuCopyFast(&sScreen[(i + 1) * 32], &sScreen[i * 32], 0x40);
    }
    MIi_CpuClearFast(0, &sScreen[23 * 32], 0x40);
}

// Puts a character at the end of the last line, going up a line when the line is full or a new line comes
static void ScreenPutChar(int c) {
    if (c == '\r') {
        return;
    }
    if (sCursor == 32) {
        ScreenScrollUp();
        sCursor = 0;
        if (c == '\n') {
            return;
        }
    }
    if (c == '\n') {
        sCursor = 32;
        return;
    }
    sScreen[23 * 32 + sCursor] = c | 0xf000;
    sCursor++;
}

// Prints the text, and waits for the next vertical blank
void ScreenVPrintf(const char *format, va_list args) {
    int i;

    if (sScreenFlags & 1) {
        OS_VSNPrintf(sText, 0x7ff, format, args);
        sText[0x7ff] = '\0';
        for (i = 0; sText[i] != '\0'; i++) {
            ScreenPutChar(sText[i]);
        }
        OS_WaitVBlankIntr();
    }
}

int gsiTimeInSec(int *seconds) {
    int time = OS_TicksToSeconds(OS_GetTick());

    if (seconds != NULL) {
        *seconds = time;
    }
    return time;
}
