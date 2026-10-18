// Tests of the port's run directory (port/app/run_dir.c): frame lists as pbw-oracle's --screens takes them, each
// screen's hash and blank decision as pbw-oracle and compare.py make them, and the files a run writes: frames.tsv in
// its version 2, the PNG files of the listed frames, and run.txt while the run goes on and after it. The screen hashes
// expected are the oracle's own (port/tests/oracle/boot_b2_us.tsv: frame 1 shows both displays off, white, and frame
// 400 both screens black). And memory.txt, from dump specs as --run-dump takes them, over globals of the test that a
// resolver names. The files go in a directory named after the program (argv[0] plus ".tmp"), so that the plain and
// sanitized builds can run at the same time.

// POSIX's rmdir, which glibc hides from strict C11
#define _POSIX_C_SOURCE 200809L

#include "app/run_dir.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

#define PIXELS (256 * 192)
// The oracle's hashes of a screen that is all white (a display that is off) and all black
#define WHITE_HASH "76449f41286e8a2a18554c535b750bf6a958f4c9"
#define BLACK_HASH "cd838da3bb89232479050e179d5804ccc02f41be"

static uint32_t sTop[PIXELS];
static uint32_t sBottom[PIXELS];
static char sDir[512];
static char sPngPaths[8][600];
static int sPngCount;

static void Fill(uint32_t *pixels, uint32_t colour) {
    for (int i = 0; i < PIXELS; i++) {
        pixels[i] = colour;
    }
}

static int Contains(const char *text, uint32_t frame) {
    PortFrameList list;
    char error[128];
    int contains;

    if (!port_frame_list_parse(&list, text, error, sizeof(error))) {
        return -1;
    }
    contains = port_frame_list_contains(&list, frame);
    port_frame_list_free(&list);
    return contains;
}

static int Rejects(const char *text, const char *message) {
    PortFrameList list;
    char error[128] = "";

    if (port_frame_list_parse(&list, text, error, sizeof(error))) {
        port_frame_list_free(&list);
        return 0;
    }
    return strstr(error, message) != NULL;
}

static void TestFrameLists(void) {
    static const char list[] = "1,30,60-90,100-200/10";

    CHECK(Contains(list, 1) == 1);
    CHECK(Contains(list, 2) == 0);
    CHECK(Contains(list, 30) == 1);
    CHECK(Contains(list, 59) == 0);
    CHECK(Contains(list, 60) == 1);
    CHECK(Contains(list, 75) == 1);
    CHECK(Contains(list, 90) == 1);
    CHECK(Contains(list, 91) == 0);
    CHECK(Contains(list, 100) == 1);
    CHECK(Contains(list, 105) == 0);
    CHECK(Contains(list, 110) == 1);
    CHECK(Contains(list, 200) == 1);
    CHECK(Contains(list, 210) == 0);
    // all, none and their mixes
    CHECK(Contains("all", 123456) == 1);
    CHECK(Contains("none", 1) == 0);
    CHECK(Contains("none,5", 5) == 1);
    CHECK(Contains("7,all", 8) == 1);
    // A step counts from the range's first frame
    CHECK(Contains("3-20/5", 13) == 1);
    CHECK(Contains("3-20/5", 15) == 0);
    CHECK(Contains("999999999", 999999999) == 1);

    CHECK(Rejects("0", "\"0\" isn't N, A-B, A-B/STEP, all or none"));
    CHECK(Rejects("5-3", "\"5-3\""));
    CHECK(Rejects("4/2", "\"4/2\""));
    CHECK(Rejects("1-9/0", "\"1-9/0\""));
    CHECK(Rejects("1-", "\"1-\""));
    CHECK(Rejects("-4", "\"-4\""));
    CHECK(Rejects("1,,2", "\"\""));
    CHECK(Rejects("", "\"\""));
    CHECK(Rejects("x", "\"x\""));
    CHECK(Rejects("1 ", "\"1 \""));
    CHECK(Rejects("1000000000", "\"1000000000\""));
    CHECK(Rejects("All", "\"All\""));
}

static void TestScreens(void) {
    char hash[41];

    Fill(sTop, 0xffffffffu);
    port_run_dir_screen_hash(sTop, hash);
    CHECK(strcmp(hash, WHITE_HASH) == 0);
    // The alpha byte isn't part of the picture
    Fill(sTop, 0x00ffffffu);
    port_run_dir_screen_hash(sTop, hash);
    CHECK(strcmp(hash, WHITE_HASH) == 0);
    Fill(sTop, 0xff000000u);
    port_run_dir_screen_hash(sTop, hash);
    CHECK(strcmp(hash, BLACK_HASH) == 0);
    // One pixel's change changes the hash
    sTop[PIXELS - 1] = 0xff000004u;
    port_run_dir_screen_hash(sTop, hash);
    CHECK(strcmp(hash, BLACK_HASH) != 0);

    // Blank: all but 256 pixels of one colour, wherever the others are and whatever colours they have
    Fill(sTop, 0xff000000u);
    CHECK(port_run_dir_screen_blank(sTop));
    for (uint32_t i = 0; i < 256; i++) {
        sTop[i * 191] = 0xff000000u | (i + 1);
    }
    CHECK(port_run_dir_screen_blank(sTop));
    sTop[PIXELS - 1] = 0xffffffffu;
    CHECK(!port_run_dir_screen_blank(sTop));
    // Two colours in halves: none has all but 256
    for (int i = 0; i < PIXELS; i++) {
        sTop[i] = i < PIXELS / 2 ? 0xff000000u : 0xffffffffu;
    }
    CHECK(!port_run_dir_screen_blank(sTop));
    // A colour that differs only in alpha is the same colour
    Fill(sTop, 0xff102030u);
    for (int i = 0; i < 1000; i++) {
        sTop[i] = 0x00102030u;
    }
    CHECK(port_run_dir_screen_blank(sTop));
}

static int RecordPng(const char *path) {
    if (sPngCount < 8) {
        snprintf(sPngPaths[sPngCount], sizeof(sPngPaths[0]), "%s", path);
    }
    sPngCount++;
    return 1;
}

static int FailPng(const char *path) {
    (void)path;
    return 0;
}

static char *ReadFile(const char *name) {
    char path[600];
    FILE *file;
    long size;
    char *text;

    snprintf(path, sizeof(path), "%s/%s", sDir, name);
    file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    size = ftell(file);
    fseek(file, 0, SEEK_SET);
    text = calloc((size_t)size + 1, 1);
    if (text != NULL && fread(text, 1, (size_t)size, file) != (size_t)size) {
        free(text);
        text = NULL;
    }
    fclose(file);
    return text;
}

static void RemoveDir(void) {
    static const char *const names[] = { "frames.tsv", "memory.txt", "run.txt" };
    char path[600];

    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        snprintf(path, sizeof(path), "%s/%s", sDir, names[i]);
        remove(path);
    }
    rmdir(sDir);
}

static void TestRun(void) {
    static const char tsvHead[] =
        "#pbwframes 2\n"
        "frame\tvblank\tcounter\tgame_frame\tkeyread\ttop_3d\tbottom_3d\ttop\tbottom\ttop_blank\tbottom_blank\n";
    static const char rows[] = "1\t1\t0\t1\t-\t-\t-\t" WHITE_HASH "\t" WHITE_HASH "\t1\t1\n"
                               "2\t2\t1\t2\t-\t-\t-\t" WHITE_HASH "\t" BLACK_HASH "\t1\t1\n"
                               "3\t3\t1\t2\t-\t-\t-\t" WHITE_HASH "\t" BLACK_HASH "\t1\t1\n"
                               "4\t4\t2\t3\t-\t-\t-\t" WHITE_HASH "\t" BLACK_HASH "\t1\t1\n";
    static const char runningRun[] = "#pbwrun 1\n"
                                     "program: frigate-black2\n"
                                     "rom: rom.nds\n"
                                     "frames: 4\n"
                                     "completed: -\n"
                                     "time: 2012-10-07 12:00:00\n"
                                     "input: -\n"
                                     "input_frames: -\n"
                                     "counter: sVBlankCount\n"
                                     "status: -\n";
    static const char finishedRun[] = "#pbwrun 1\n"
                                      "program: frigate-black2\n"
                                      "rom: rom.nds\n"
                                      "frames: 4\n"
                                      "completed: 4\n"
                                      "time: 2012-10-07 12:00:00\n"
                                      "input: -\n"
                                      "input_frames: -\n"
                                      "counter: sVBlankCount\n"
                                      "status: 0\n";
    const PortRunDirInfo info = { "frigate-black2", "rom.nds", "2012-10-07 12:00:00", NULL, "", 4 };
    const PortRunDirFrame frames[] = { { 1, 1, 0, 1 }, { 2, 2, 1, 2 }, { 3, 3, 1, 2 }, { 4, 4, 2, 3 } };
    char expected[sizeof(tsvHead) + sizeof(rows)];
    char wanted[600];
    char *text;

    RemoveDir();
    CHECK(!port_run_dir_active());
    CHECK(port_run_dir_open(sDir, "2-3,9", RecordPng, &info));
    CHECK(port_run_dir_active());
    text = ReadFile("run.txt");
    CHECK(text != NULL && strcmp(text, runningRun) == 0);
    free(text);

    Fill(sTop, 0xffffffffu);
    Fill(sBottom, 0xffffffffu);
    CHECK(port_run_dir_frame(&frames[0], sTop, sBottom));
    Fill(sBottom, 0xff000000u);
    for (int i = 1; i < 4; i++) {
        CHECK(port_run_dir_frame(&frames[i], sTop, sBottom));
    }
    // Each row is in the file as soon as it is written
    text = ReadFile("frames.tsv");
    snprintf(expected, sizeof(expected), "%s%s", tsvHead, rows);
    CHECK(text != NULL && strcmp(text, expected) == 0);
    free(text);
    CHECK(sPngCount == 2);
    snprintf(wanted, sizeof(wanted), "%s/frame_000002.png", sDir);
    CHECK(strcmp(sPngPaths[0], wanted) == 0);
    snprintf(wanted, sizeof(wanted), "%s/frame_000003.png", sDir);
    CHECK(strcmp(sPngPaths[1], wanted) == 0);

    port_run_dir_close(0);
    CHECK(!port_run_dir_active());
    text = ReadFile("run.txt");
    CHECK(text != NULL && strcmp(text, finishedRun) == 0);
    free(text);
    // A frame after the close writes nothing
    CHECK(port_run_dir_frame(&frames[0], sTop, sBottom));
    text = ReadFile("frames.tsv");
    CHECK(text != NULL && strcmp(text, expected) == 0);
    free(text);

    // Opening again starts the files anew; a PNG file that can't be written fails the frame
    CHECK(port_run_dir_open(sDir, "all", FailPng, &info));
    CHECK(!port_run_dir_frame(&frames[0], sTop, sBottom));
    port_run_dir_close(1);
    text = ReadFile("run.txt");
    CHECK(text != NULL && strstr(text, "completed: 1\n") != NULL && strstr(text, "status: 1\n") != NULL);
    free(text);
    text = ReadFile("frames.tsv");
    CHECK(text != NULL && strncmp(text, tsvHead, strlen(tsvHead)) == 0 && strlen(text) > strlen(tsvHead) &&
          strstr(text + strlen(tsvHead), "\n2\t") == NULL);
    free(text);
    RemoveDir();

    // A bad list or a directory that can't be made opens nothing
    CHECK(!port_run_dir_open(sDir, "1-", RecordPng, &info));
    CHECK(!port_run_dir_active());
    snprintf(wanted, sizeof(wanted), "%s/missing/deeper", sDir);
    CHECK(!port_run_dir_open(wanted, NULL, RecordPng, &info));
    CHECK(!port_run_dir_active());
}

// What --run-dump reads: 16 bytes 00, 11, ... ff, and a pointer into them; the program's globals as the resolver gives
// them by name
static uint8_t sBlock[16] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                              0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff };
static uint8_t *sPointer;

static void *Resolve(const char *name) {
    if (strcmp(name, "block") == 0) {
        return sBlock;
    }
    if (strcmp(name, "pointer") == 0) {
        return &sPointer;
    }
    return NULL;
}

static int DumpRejected(const char *spec, const char *message) {
    char error[256] = "";

    if (port_run_dir_add_dump(spec, Resolve, error, sizeof(error))) {
        return 0;
    }
    if (strstr(error, message) == NULL) {
        fprintf(stderr, "%s: \"%s\" doesn't say \"%s\"\n", spec, error, message);
        return 0;
    }
    return 1;
}

static void TestDumps(void) {
    const PortRunDirInfo info = { "frigate-black2", "rom.nds", "2012-10-07 12:00:00", NULL, "", 4 };
    const PortRunDirInfo unlimited = { "frigate-black2", "rom.nds", "2012-10-07 12:00:00", NULL, "", 0 };
    char error[256];
    char expected[600];
    char *text;

    RemoveDir();
    CHECK(DumpRejected("block:4", "needs --run-dir"));
    CHECK(port_run_dir_open(sDir, NULL, RecordPng, &info));
    // No dump, no memory.txt
    text = ReadFile("memory.txt");
    CHECK(text == NULL);
    free(text);
    CHECK(DumpRejected("block", "needs :SIZE"));
    CHECK(DumpRejected("block:0", "needs :SIZE"));
    CHECK(DumpRejected("block:0x100001", "needs :SIZE"));
    CHECK(DumpRejected("block:4x", "needs :SIZE"));
    CHECK(DumpRejected("block+x:4", "\"x\" isn't an offset"));
    CHECK(DumpRejected("block+:4", "\"\" isn't an offset"));
    CHECK(DumpRejected("9block:4", "\"9block\" isn't a symbol"));
    CHECK(DumpRejected("bl-ock:4", "\"bl-ock\" isn't a symbol"));
    CHECK(DumpRejected("*:4", "\"\" isn't a symbol"));
    CHECK(DumpRejected("nothing:4", "no global nothing"));
    CHECK(DumpRejected("a b=block:4", "without spaces"));
    CHECK(DumpRejected("=block:4", "without spaces"));
    CHECK(DumpRejected("block:4@0", "\"0\" isn't N, A-B"));

    // Two bytes from the third at frame 2; where the pointer points, plus one, at frames 1 and 4 (1-4/3); the
    // pointer's NULL at frame 3, where its dump writes nothing; the run's last frame (4) without a list
    CHECK(port_run_dir_add_dump("block+2:3@2", Resolve, error, sizeof(error)));
    CHECK(port_run_dir_add_dump("p=*pointer+0x1:2@1-4/3", Resolve, error, sizeof(error)));
    CHECK(port_run_dir_add_dump("*pointer:1@3", Resolve, error, sizeof(error)));
    CHECK(port_run_dir_add_dump("last=block:1", Resolve, error, sizeof(error)));
    Fill(sTop, 0xffffffffu);
    Fill(sBottom, 0xffffffffu);
    for (uint32_t frame = 1; frame <= 4; frame++) {
        const PortRunDirFrame row = { frame, frame, frame - 1, frame };
        sPointer = frame == 3 ? NULL : sBlock + 8;
        CHECK(port_run_dir_frame(&row, sTop, sBottom));
    }
    port_run_dir_close(0);
    snprintf(expected, sizeof(expected),
             "#pbwmem 1\n"
             "1 p 0x%llx 2 99aa\n"
             "2 block+2 0x%llx 3 223344\n"
             "4 p 0x%llx 2 99aa\n"
             "4 last 0x%llx 1 00\n",
             (unsigned long long)(uintptr_t)(sBlock + 9), (unsigned long long)(uintptr_t)(sBlock + 2),
             (unsigned long long)(uintptr_t)(sBlock + 9), (unsigned long long)(uintptr_t)sBlock);
    text = ReadFile("memory.txt");
    CHECK(text != NULL && strcmp(text, expected) == 0);
    free(text);
    RemoveDir();

    // Without --frames, a dump needs its list
    CHECK(port_run_dir_open(sDir, NULL, RecordPng, &unlimited));
    CHECK(DumpRejected("block:4", "needs --frames"));
    CHECK(port_run_dir_add_dump("block:4@1", Resolve, error, sizeof(error)));
    port_run_dir_close(0);
    RemoveDir();
}

int main(int argc, char **argv) {
    (void)argc;
    snprintf(sDir, sizeof(sDir), "%s.tmp", argv[0]);
    TestFrameLists();
    TestScreens();
    TestRun();
    TestDumps();
    if (sFailed != 0) {
        fprintf(stderr, "%d of %d checks failed\n", sFailed, sChecks);
        return 1;
    }
    printf("%d checks passed\n", sChecks);
    return 0;
}
