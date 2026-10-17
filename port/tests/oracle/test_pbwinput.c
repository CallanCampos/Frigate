// Tests of the input file reader and writer (port/tools/oracle/pbwinput.c): the format's rules as
// port/tools/oracle/README.md gives them, the states each frame gets, every error a file can have, a round trip
// through the writer, and the game clock that turns GFL's frame count into game frames. The round trip writes a file
// named after the program (argv[0] plus ".tmp"), so that the plain and sanitized builds can run at the same time.
#include "pbwinput.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int sFailed;
static int sChecks;

#define CHECK(condition)                                                                                               \
    do {                                                                                                               \
        sChecks++;                                                                                                     \
        if (!(condition)) {                                                                                            \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition);                              \
            sFailed++;                                                                                                 \
        }                                                                                                              \
    } while (0)

static int Parse(PbwInput *input, const char *text, char *error, size_t errorSize) {
    return pbw_input_parse(input, text, strlen(text), error, errorSize);
}

static int SameState(PbwInputState a, PbwInputState b) {
    return a.buttons == b.buttons && a.lidClosed == b.lidClosed && a.touching == b.touching && a.touchX == b.touchX &&
           a.touchY == b.touchY;
}

static void TestValidFile(void) {
    static const char text[] = "# a replay\r\n"
                               "pbwinput 1\r\n"
                               "\n"
                               "time 2012-10-07 12:00:00   # the RTC\n"
                               "version b2_us\n"
                               "frames video\n"
                               "1 -\n"
                               "120\tA\n"
                               "121 -   -\n"
                               "300 start+a\n"
                               "302 Up+Down+Left+Right+L+R+X+Y+Select+Debug\n"
                               "400 - 128,96\n"
                               "401 B 255,191\n"
                               "500 LID\n"
                               "501 -";
    PbwInput input;
    char error[256] = "";
    PbwInputState state;

    CHECK(Parse(&input, text, error, sizeof(error)));
    if (input.changes == NULL) {
        fprintf(stderr, "  %s\n", error);
        return;
    }
    CHECK(strcmp(input.time, "2012-10-07 12:00:00") == 0);
    CHECK(strcmp(input.version, "b2_us") == 0);
    CHECK(input.frames == PBW_INPUT_VIDEO_FRAMES);
    CHECK(input.count == 9);

    state = pbw_input_state(&input, 1);
    CHECK(state.buttons == 0 && !state.touching && !state.lidClosed);
    CHECK(pbw_input_state(&input, 119).buttons == 0);
    CHECK(pbw_input_state(&input, 120).buttons == PBW_INPUT_A);
    CHECK(pbw_input_state(&input, 121).buttons == 0);
    CHECK(pbw_input_state(&input, 300).buttons == (PBW_INPUT_START | PBW_INPUT_A));
    CHECK(pbw_input_state(&input, 301).buttons == (PBW_INPUT_START | PBW_INPUT_A));
    CHECK(pbw_input_state(&input, 302).buttons ==
          (PBW_INPUT_UP | PBW_INPUT_DOWN | PBW_INPUT_LEFT | PBW_INPUT_RIGHT | PBW_INPUT_L | PBW_INPUT_R | PBW_INPUT_X |
           PBW_INPUT_Y | PBW_INPUT_SELECT | PBW_INPUT_DEBUG));
    state = pbw_input_state(&input, 400);
    CHECK(state.buttons == 0 && state.touching && state.touchX == 128 && state.touchY == 96);
    state = pbw_input_state(&input, 450);
    CHECK(state.buttons == PBW_INPUT_B && state.touching && state.touchX == 255 && state.touchY == 191);
    state = pbw_input_state(&input, 500);
    CHECK(state.buttons == 0 && state.lidClosed && !state.touching);
    state = pbw_input_state(&input, 100000);
    CHECK(state.buttons == 0 && !state.lidClosed && !state.touching);
    pbw_input_free(&input);
}

static void TestStatesBeforeTheFirstChange(void) {
    PbwInput input;
    char error[256];

    CHECK(Parse(&input, "pbwinput 1\n10 A\n", error, sizeof(error)));
    CHECK(pbw_input_state(&input, 1).buttons == 0);
    CHECK(pbw_input_state(&input, 9).buttons == 0);
    CHECK(pbw_input_state(&input, 10).buttons == PBW_INPUT_A);
    CHECK(pbw_input_state(&input, 0xffffffffu).buttons == PBW_INPUT_A);
    CHECK(input.time[0] == '\0' && input.version[0] == '\0');
    // Game frames unless the file says otherwise
    CHECK(input.frames == PBW_INPUT_GAME_FRAMES);
    pbw_input_free(&input);
    CHECK(Parse(&input, "pbwinput 1\nframes game\n10 A\n", error, sizeof(error)));
    CHECK(input.frames == PBW_INPUT_GAME_FRAMES && input.count == 1);
    pbw_input_free(&input);

    // A header alone: nothing held on any frame
    CHECK(Parse(&input, "pbwinput 1", error, sizeof(error)));
    CHECK(input.count == 0);
    CHECK(pbw_input_state(&input, 5).buttons == 0);
    pbw_input_free(&input);

    // The empty input of pbw_input_init, as a program without a file uses it
    pbw_input_init(&input);
    CHECK(pbw_input_state(&input, 1).buttons == 0);
    pbw_input_free(&input);
}

typedef struct BadFile {
    const char *text;
    const char *message; // a part of the error
} BadFile;

static void TestErrors(void) {
    static const BadFile bad[] = {
        { "", "empty file" },
        { "# only a comment\n", "empty file" },
        { "1 A\n", "must start with" },
        { "pbwinput\n", "must start with" },
        { "pbwinput 2\n", "version 2" },
        { "pbwinput 1\n0 A\n", "frame from 1" },
        { "pbwinput 1\n100000001 A\n", "frame from 1" },
        { "pbwinput 1\n-5 A\n", "neither a directive nor a frame" },
        { "pbwinput 1\nhello\n", "neither a directive nor a frame" },
        { "pbwinput 1\n10 A\n10 B\n", "comes after" },
        { "pbwinput 1\n10 A\n9 B\n", "comes after" },
        { "pbwinput 1\n10\n", "FRAME BUTTONS" },
        { "pbwinput 1\n10 A - extra\n", "FRAME BUTTONS" },
        { "pbwinput 1\n10 Z\n", "unknown button" },
        { "pbwinput 1\n10 A+Z\n", "unknown button" },
        { "pbwinput 1\n10 A+a\n", "named twice" },
        { "pbwinput 1\n10 A+\n", "ends with +" },
        { "pbwinput 1\n10 +A\n", "unknown button" },
        { "pbwinput 1\n10 A++B\n", "unknown button" },
        { "pbwinput 1\n10 - 256,0\n", "a touch is" },
        { "pbwinput 1\n10 - 0,192\n", "a touch is" },
        { "pbwinput 1\n10 - 12\n", "a touch is" },
        { "pbwinput 1\n10 - 1,2,3\n", "a touch is" },
        { "pbwinput 1\n10 - x,2\n", "a touch is" },
        { "pbwinput 1\n10 - ,2\n", "a touch is" },
        { "pbwinput 1\ntime 2012-10-07\n", "takes YYYY-MM-DD" },
        { "pbwinput 1\ntime 1999-12-31 23:59:59\n", "isn't a time" },
        { "pbwinput 1\ntime 2100-01-01 00:00:00\n", "isn't a time" },
        { "pbwinput 1\ntime 2013-02-29 00:00:00\n", "isn't a time" },
        { "pbwinput 1\ntime 2012-13-01 00:00:00\n", "isn't a time" },
        { "pbwinput 1\ntime 2012-04-31 00:00:00\n", "isn't a time" },
        { "pbwinput 1\ntime 2012-10-07 24:00:00\n", "isn't a time" },
        { "pbwinput 1\ntime 2012-1-07 12:00:00\n", "isn't a time" },
        { "pbwinput 1\ntime 2012-10-07 12:00:00\ntime 2012-10-07 12:00:00\n", "one \"time\" line" },
        { "pbwinput 1\n1 A\ntime 2012-10-07 12:00:00\n", "one \"time\" line" },
        { "pbwinput 1\nversion b2_jp\n", "b2_us or w2_us" },
        { "pbwinput 1\nversion\n", "b2_us or w2_us" },
        { "pbwinput 1\nversion b2_us\nversion b2_us\n", "one \"version\" line" },
        { "pbwinput 1\nframes\n", "game or video" },
        { "pbwinput 1\nframes logic\n", "game or video" },
        { "pbwinput 1\nframes game video\n", "game or video" },
        { "pbwinput 1\nframes video\nframes video\n", "one \"frames\" line" },
        { "pbwinput 1\n1 A\nframes video\n", "one \"frames\" line" },
        { "pbwinput 1\n10 AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n",
          "longer than" },
    };
    size_t i;

    for (i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        PbwInput input;
        char error[256] = "";
        int ok = Parse(&input, bad[i].text, error, sizeof(error));

        CHECK(!ok);
        CHECK(input.changes == NULL && input.count == 0);
        if (strstr(error, bad[i].message) == NULL) {
            fprintf(stderr, "case %zu: error \"%s\" lacks \"%s\"\n", i, error, bad[i].message);
            sFailed++;
        }
    }

    // A NUL byte inside the text, which strlen would hide
    {
        static const char text[] = "pbwinput 1\n1 A\0B\n";
        PbwInput input;
        char error[256] = "";

        CHECK(!pbw_input_parse(&input, text, sizeof(text) - 1, error, sizeof(error)));
        CHECK(strstr(error, "NUL") != NULL);
        CHECK(strstr(error, "line 2") != NULL);
    }
    // A small error buffer still ends with a NUL; none at all is fine
    {
        PbwInput input;
        char error[8];

        memset(error, 'x', sizeof(error));
        CHECK(!Parse(&input, "pbwinput 1\n10 Z\n", error, sizeof(error)));
        CHECK(memchr(error, '\0', sizeof(error)) != NULL);
        CHECK(!Parse(&input, "pbwinput 1\n10 Z\n", NULL, 0));
    }
}

static void TestLeapDays(void) {
    CHECK(pbw_input_valid_time("2000-02-29 00:00:00"));
    CHECK(pbw_input_valid_time("2012-02-29 23:59:59"));
    CHECK(!pbw_input_valid_time("2013-02-29 00:00:00"));
    CHECK(pbw_input_valid_time("2099-12-31 23:59:59"));
    CHECK(!pbw_input_valid_time("2012-10-07 12:00:00 "));
    CHECK(!pbw_input_valid_time("2012-10-07T12:00:00"));
    CHECK(!pbw_input_valid_time("2012-00-07 12:00:00"));
    CHECK(!pbw_input_valid_time("2012-10-00 12:00:00"));
}

// The frames whose states a program applies at each VBlank, for a game-frame input and a video-frame one, as GFL's
// frame count goes on the DS: the ARM9's image at its address until the start-up code clears it, 0 until the main
// loop's first pass ends, then a step after each pass, a pass longer than a frame, a sleep, a soft reset, and the count
// wrapping around
static void TestGameClock(void) {
    static const struct {
        int asleep;     // a video frame without a VBlank
        uint32_t count; // else GFL's frame count at the VBlank
        uint32_t game;  // the game frame whose state applies then
    } steps[] = {
        { 0, 20999712, 1 }, // the ARM9's image, before the start-up code clears .bss
        { 0, 20999712, 1 },
        { 0, 7, 1 },
        { 0, 0, 1 }, // cleared; GFLAppInit sets it to 0
        { 0, 0, 1 }, // the main loop's first pass (game frame 0) waits
        { 0, 1, 2 }, // its second pass, game frame 1, waits; game frame 2 reads next
        { 0, 2, 3 },
        { 0, 2, 3 }, // a pass longer than a frame
        { 0, 3, 4 },
        { 1, 0, 5 }, // asleep: each video frame counts
        { 1, 0, 6 },
        { 0, 3, 6 }, // awake, the count where it was
        { 0, 4, 7 },
        { 0, 0, 7 }, // a soft reset
        { 0, 0, 7 },
        { 0, 1, 8 },
        { 0, 2, 9 },
        { 0, 0xfffffffeu, 9 }, // a jump is no step
        { 0, 0xffffffffu, 10 },
        { 0, 0, 10 }, // nor is wrapping around to 0
    };
    PbwInput game;
    PbwInput video;
    PbwGameClock gameClock;
    PbwGameClock videoClock;
    char error[256];
    size_t i;

    CHECK(Parse(&game, "pbwinput 1\n", error, sizeof(error)));
    CHECK(Parse(&video, "pbwinput 1\nframes video\n", error, sizeof(error)));
    pbw_game_clock_init(&gameClock);
    pbw_game_clock_init(&videoClock);
    for (i = 0; i < sizeof(steps) / sizeof(steps[0]); i++) {
        uint32_t videoFrame = (uint32_t)i + 1;
        uint32_t frame;
        uint32_t videoState;

        if (steps[i].asleep) {
            frame = pbw_input_frame_asleep(&game, &gameClock, videoFrame);
            videoState = pbw_input_frame_asleep(&video, &videoClock, videoFrame);
        } else {
            frame = pbw_input_frame_at_vblank(&game, &gameClock, videoFrame, steps[i].count);
            videoState = pbw_input_frame_at_vblank(&video, &videoClock, videoFrame, steps[i].count);
        }
        if (frame != steps[i].game) {
            fprintf(stderr, "step %zu: game frame %u, not %u\n", i, frame, steps[i].game);
            sFailed++;
        }
        // Video frames are the VBlanks' numbers, whatever the count does
        CHECK(videoState == videoFrame);
    }
    pbw_input_free(&game);
    pbw_input_free(&video);
}

static void TestFormatAndRoundTrip(const char *program) {
    char path[1024];
    PbwInputState state = { 0 };
    PbwInput input;
    PbwInput back;
    char columns[96];
    char error[256] = "";
    FILE *file;
    size_t i;

    CHECK(pbw_input_format_state(&state, columns, sizeof(columns)));
    CHECK(strcmp(columns, "- -") == 0);
    state.buttons = PBW_INPUT_START | PBW_INPUT_A;
    state.lidClosed = 1;
    state.touching = 1;
    state.touchX = 7;
    state.touchY = 191;
    CHECK(pbw_input_format_state(&state, columns, sizeof(columns)));
    CHECK(strcmp(columns, "A+START+LID 7,191") == 0);
    // Too small a buffer is refused
    CHECK(!pbw_input_format_state(&state, columns, 5));
    CHECK(!pbw_input_format_state(&state, columns, 0));

    // A recorder's input: repeats and pen coordinates without the pen are dropped, out-of-order frames refused
    pbw_input_init(&input);
    memcpy(input.time, "2012-10-07 12:00:00", 20);
    memcpy(input.version, "w2_us", 6);
    input.frames = PBW_INPUT_VIDEO_FRAMES;
    memset(&state, 0, sizeof(state));
    CHECK(pbw_input_append(&input, 1, &state));
    CHECK(input.count == 0);
    state.buttons = PBW_INPUT_A | 0x80000000u;
    CHECK(pbw_input_append(&input, 5, &state));
    CHECK(pbw_input_append(&input, 6, &state));
    CHECK(input.count == 1 && input.changes[0].state.buttons == PBW_INPUT_A);
    CHECK(!pbw_input_append(&input, 5, &state));
    CHECK(!pbw_input_append(&input, 0, &state));
    state.buttons = 0;
    state.touchX = 99;
    CHECK(pbw_input_append(&input, 8, &state));
    CHECK(input.count == 2 && input.changes[1].state.touchX == 0);
    state.touching = 1;
    state.touchY = 192;
    CHECK(!pbw_input_append(&input, 9, &state));
    state.touchY = 10;
    CHECK(pbw_input_append(&input, 9, &state));
    state.touching = 0;
    state.lidClosed = 1;
    CHECK(pbw_input_append(&input, 20, &state));
    for (i = 0; i < 300; i++) {
        state.buttons = (uint32_t)(i & 1) * PBW_INPUT_B;
        state.lidClosed = 0;
        CHECK(pbw_input_append(&input, (uint32_t)(100 + i), &state));
    }

    CHECK(snprintf(path, sizeof(path), "%s.tmp", program) < (int)sizeof(path));
    file = fopen(path, "wb");
    CHECK(file != NULL);
    if (file == NULL) {
        pbw_input_free(&input);
        return;
    }
    CHECK(pbw_input_write(&input, file));
    fclose(file);
    CHECK(pbw_input_load(&back, path, error, sizeof(error)));
    CHECK(back.count == input.count);
    CHECK(strcmp(back.time, input.time) == 0);
    CHECK(strcmp(back.version, input.version) == 0);
    CHECK(back.frames == PBW_INPUT_VIDEO_FRAMES);
    for (i = 0; i < input.count && i < back.count; i++) {
        CHECK(back.changes[i].frame == input.changes[i].frame);
        CHECK(SameState(back.changes[i].state, input.changes[i].state));
    }
    pbw_input_free(&back);
    pbw_input_free(&input);
    remove(path);

    CHECK(!pbw_input_load(&back, "no/such/dir/input.pbwinput", error, sizeof(error)));
    CHECK(strstr(error, "can't open") != NULL);
}

int main(int argc, char **argv) {
    TestValidFile();
    TestStatesBeforeTheFirstChange();
    TestErrors();
    TestLeapDays();
    TestGameClock();
    TestFormatAndRoundTrip(argc > 0 && argv[0] != NULL ? argv[0] : "test_pbwinput");
    if (sFailed != 0) {
        fprintf(stderr, "%d of %d checks failed\n", sFailed, sChecks);
        return 1;
    }
    printf("%d checks passed\n", sChecks);
    return 0;
}
