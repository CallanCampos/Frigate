// DWC's auto-connect, the error code that it gives the game (TWL-DWC 5.3, ac). The file name is a guess: it starts at
// the function that dwc_ac.c's driver calls for the state of the teardown, 17, and goes on with the one that turns the
// error noted by func_0205a85c into a code. It ends where the list of access points begins, and no data or call across
// either boundary says whether they are one file with dwc_ac_aplist.c.

#include "dwc/ac.h"

static s32 func_0205b6f0(s32 error);
static s32 func_0205b734(void);
static s32 func_0205b73c(const DWCAcMain *main);
static s32 func_0205b75c(const DWCAcMain *main);

// The state 17: takes the wireless manager and the sockets down from the state the failure came in, and returns 18
// once that is over, else 17
u8 func_0205b65c(void) {
    DWCAcMain *main = func_0205a78c(DWC_AC_WORK_MAIN);

    if (func_0205975c(&main->failedState) == TRUE) {
        return 18;
    }
    return 17;
}

// The error code that is told to the game, from the error that func_0205a85c noted. The big ones look like the 5xxxx
// numbers that the game shows the player, negated
s32 func_0205b680(void) {
    DWCAcMain *main = func_0205a78c(DWC_AC_WORK_MAIN);
    s32 error = func_0205a880();

    if (error < 4) {
        return func_0205b6f0(error);
    }
    if (error < 5) {
        return func_0205b734();
    }
    if (error == 5) {
        return func_0205b73c(main);
    }
    if (error == 7) {
        return -50599;
    }
    if (error == 8) {
        return -50699;
    }
    return func_0205b75c(main);
}

// The errors 0 to 3: the codes -10 to -7
static s32 func_0205b6f0(s32 error) {
    switch (error) {
    case 1:
        return -9;
    case 0:
        return -10;
    case 2:
        return -8;
    case 3:
        return -7;
    }
    return 0;
}

// The error 4: the code -6
static s32 func_0205b734(void) {
    return -6;
}

// The error 5, which the search gives when it ends without a connection: the code is another if it saw an access point
// at all
static s32 func_0205b73c(const DWCAcMain *main) {
    if (main->unk_0b_0 == 0) {
        return -50099;
    }
    return -51099;
}

// The other errors: the code depends on how far the connection came (the highest state that func_0205a7d4 was given),
// and has the type of the access point added, which is the one that func_0205a420 gave or else that of the candidate.
// Before the connection the access point refused (-511xx for a wrong key, -512xx for one that is full, -513xx for any
// other); in the states of the sockets the codes follow what the socket layer noted
static s32 func_0205b75c(const DWCAcMain *main) {
    u8 type = main->givenType ? main->givenType : main->candidateType;
    u8 maxState = main->maxState;
    s32 code;

    if (maxState < 10) {
        if (main->candidateResult == 3) {
            code = -51100 - type;
        } else if (main->candidateResult == 4) {
            code = -51200 - type;
        } else {
            code = -51300 - type;
        }
    } else if (maxState < 13) {
        code = -52000 - type;
    } else {
        code = main->unk_10;
        if (code == 0) {
            code = -50000 - type;
        } else if (main->unk_0b_4 == 1) {
            code = -52700 - type;
        } else if (code == -1) {
            code = -52100 - type;
        } else if (code == -2) {
            code = -52200 - type;
        } else if (code == -3) {
            code = -52300 - type;
        } else if (code == -4) {
            code = -53000 - type;
        } else if (code == -5) {
            code = -53100 - type;
        } else if (code == -6) {
            code = -53200 - type;
        }
    }
    return code;
}
