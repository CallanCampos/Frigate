#include "dwc/screen.h"
#include "dwc/nonport.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// The screen that GameSpy's debug output prints to, as lines of characters for a text background, and the clock in
// seconds. The ROM names neither the file nor its functions: the file, the screen's functions and their data are
// GameSpy's own, as its common/nitro/screen.c has them; gsiTimeInSec is GameSpy's, as pret's pokerevo Wii build has it
// (nonport.s), but not screen.c's, so the original file may end before it

static void ScrollTopScreen(void);
static void PrintChar(int c);

// The text being printed, the screen's 24 lines of 32 characters, and where the next character goes in the last line
static char text[0x800];
static gsi_u16 gTopScreen[24 * 32];
static int gPos;

// Whether the text is printed (bit 0), which it is not
static int gPrintMode = 2;

// Moves every line up one and clears the last one
static void ScrollTopScreen(void) {
    int i;

    for (i = 0; i < 23; i++) {
        MIi_CpuCopyFast(&gTopScreen[(i + 1) * 32], &gTopScreen[i * 32], 0x40);
    }
    MIi_CpuClearFast(0, &gTopScreen[23 * 32], 0x40);
}

// Puts a character at the end of the last line, going up a line when the line is full or a new line comes
static void PrintChar(int c) {
    if (c == '\r') {
        return;
    }
    if (gPos == 32) {
        ScrollTopScreen();
        gPos = 0;
        if (c == '\n') {
            return;
        }
    }
    if (c == '\n') {
        gPos = 32;
        return;
    }
    gTopScreen[23 * 32 + gPos] = c | 0xf000;
    gPos++;
}

// Prints the text, and waits for the next vertical blank
void VPrintf(const char *format, va_list args) {
    int i;

    if (gPrintMode & 1) {
        OS_VSNPrintf(text, 0x7ff, format, args);
        text[0x7ff] = '\0';
        for (i = 0; text[i] != '\0'; i++) {
            PrintChar(text[i]);
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
