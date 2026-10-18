#include "types.h"

// One finding of each rule check_style.py checks in game code, in order. The expected output is expected.txt.

extern int gOtherFileCounter;
void OtherFile_Function(int value);

static volatile int sFlag;

int Flagged_GotoReturn(const u8 *mac) {
    int j;
    int code;

    for (j = 0; j < 6; j++) {
        if (mac[j] != 0) {
            code = 1;
            goto end;
        }
    }
    code = 2;
end:
    return code;
}

u16 Flagged_ByteOffset(void *work) {
    return *(u16 *)((u8 *)work + 0x1310);
}

void Flagged_Asm(void) {
    asm {
        nop
    }
}

void Flagged_Comments(int x) {
    // TODO: name this
    // c is declared first to match
    // BUG: x is never checked
    OtherFile_Function(x);
}

static // A comment between the specifiers and the name
int Flagged_SplitDeclaration(void) {
    return 0;
}
