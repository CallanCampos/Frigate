// Real streaming recorder + pbwinput parser for field/menu/battle reproductions (app/record.c). Exercises complete
// state commits, alias protection, actual partial file writes and flush/close/rollback failures without a ROM.
#include "app/record.h"
#include "hal/input/port_input.h"
#include "hal/tp/tp_host.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define MKDIR(p) _mkdir(p)
#else
#include <unistd.h>
#define MKDIR(p) mkdir(p, 0700)
#endif
#define CHECK(x)                                                                                                       \
    do {                                                                                                               \
        if (!(x)) {                                                                                                    \
            fprintf(stderr, "record fixture line %d: %s\n", __LINE__, #x);                                             \
            exit(1);                                                                                                   \
        }                                                                                                              \
    } while (0)
static uint32_t sButtons;
static int sLid, sTouch, sX, sY;
uint32_t port_input_held(void) {
    return sButtons;
}
int port_input_closed(void) {
    return sLid;
}
void port_tp_get_touch(int *down, int *x, int *y) {
    *down = sTouch;
    *x = sX;
    *y = sY;
}
static char sPath[4096];
static void Header(PbwInput *h) {
    pbw_input_init(h);
    strcpy(h->version, "b2_us");
    strcpy(h->time, "2012-10-07 12:00:00");
    h->frames = PBW_INPUT_VIDEO_FRAMES;
    h->seedCount = 2;
    h->seeds[0] = 1;
    h->seeds[1] = 2;
    h->hasInitial = 1;
    h->initial.buttons = PBW_INPUT_B;
}
static PbwInput Load(void) {
    PbwInput back;
    char error[256];
    CHECK(pbw_input_load(&back, sPath, error, sizeof(error)));
    return back;
}
static void Start(void) {
    PbwInput h;
    Header(&h);
    remove(sPath);
    port_record_test_io(NULL);
    CHECK(port_record_create(sPath, NULL, 0, NULL));
    CHECK(port_record_start(&h));
}
static size_t Partial(const void *data, size_t size, FILE *file) {
    CHECK(fwrite(data, 1, size / 2, file) == size / 2);
    return size / 2;
}
static size_t Full(const void *data, size_t size, FILE *file) {
    return fwrite(data, 1, size, file);
}
static int BadFlush(FILE *file) {
    CHECK(fflush(file) == 0);
    return EOF;
}
static int BadClose(FILE *file) {
    CHECK(fclose(file) == 0);
    return EOF;
}
static int RealRollback(FILE *file, int64_t offset) {
#ifdef _WIN32
    return _chsize_s(_fileno(file), offset) == 0 && _fseeki64(file, offset, SEEK_SET) == 0 ? 0 : -1;
#else
    return ftruncate(fileno(file), (off_t)offset) == 0 && fseeko(file, (off_t)offset, SEEK_SET) == 0 ? 0 : -1;
#endif
}
static int BadRollback(FILE *file, int64_t offset) {
    (void)file;
    (void)offset;
    return -1;
}
static void TestFaults(void) {
    PbwInputState a = { PBW_INPUT_A, 0, 1, 128, 96 };
    PbwInput back;
    // Partial frame write + failed rollback leaves a numeric unterminated tail. The production reader discards it.
    PortRecordIO rollbackWorks = { Partial, fflush, fclose, RealRollback };
    Start();
    CHECK(port_record_frame(1, &a));
    port_record_test_io(&rollbackWorks);
    a.touchX = 129;
    CHECK(!port_record_frame(2, &a));
    CHECK(!port_record_close());
    back = Load();
    CHECK(back.count == 1 && back.changes[0].frame == 1 && back.changes[0].state.touchX == 128);
    pbw_input_free(&back);
    PortRecordIO flushRollback = { Full, BadFlush, fclose, RealRollback };
    Start();
    CHECK(port_record_frame(1, &a));
    port_record_test_io(&flushRollback);
    a.touchX = 130;
    CHECK(!port_record_frame(2, &a));
    CHECK(!port_record_close());
    back = Load();
    CHECK(back.count == 1 && back.changes[0].state.touchX == 129);
    pbw_input_free(&back);
    PortRecordIO writeFail = { Partial, fflush, fclose, BadRollback };
    Start();
    port_record_test_io(&writeFail);
    CHECK(!port_record_frame(1, &a));
    CHECK(!port_record_close());
    back = Load();
    CHECK(back.count == 0 && pbw_input_state(&back, 1).buttons == PBW_INPUT_B);
    pbw_input_free(&back);
    // A real committed line followed by failed flush must stop and remain a parseable prefix, even if rollback fails.
    Start();
    CHECK(port_record_frame(1, &a));
    PortRecordIO flushFail = { Full, BadFlush, fclose, BadRollback };
    port_record_test_io(&flushFail);
    a.touching = 0;
    CHECK(!port_record_frame(2, &a));
    CHECK(!port_record_close());
    back = Load();
    CHECK(back.changes[0].frame == 1);
    pbw_input_free(&back);
    Start();
    CHECK(port_record_frame(1, &a));
    PortRecordIO closeFail = { Full, fflush, BadClose, BadRollback };
    port_record_test_io(&closeFail);
    CHECK(!port_record_close());
    back = Load();
    CHECK(back.count == 1);
    pbw_input_free(&back);
    port_record_test_io(NULL);
}
int main(int argc, char **argv) {
    PbwInputState a = { PBW_INPUT_A, 0, 1, 100, 50 };
    PbwInput h, back;
    uint64_t seeds[64];
    CHECK(argc == 2);
    CHECK(snprintf(sPath, sizeof(sPath), "%s/input.pbwinput", argv[1]) < (int)sizeof(sPath));
    MKDIR(argv[1]);
    Start();
    CHECK(port_record_frame(1, &a));
    CHECK(port_record_frame(2, &a)); // unchanged held touch is coalesced
    a.touching = 0;
    a.lidClosed = 1;
    CHECK(port_record_frame(3, &a));
    CHECK(port_record_close());
    back = Load();
    CHECK(back.hasInitial && back.count == 2 && back.frames == PBW_INPUT_VIDEO_FRAMES);
    CHECK(pbw_input_initial_state(&back).buttons == PBW_INPUT_B);
    CHECK(pbw_input_state(&back, 1).touching && !pbw_input_state(&back, 3).touching &&
          pbw_input_state(&back, 3).lidClosed);
    pbw_input_free(&back);
    CHECK(!port_record_create(sPath, NULL, 0, NULL)); // existing files are never truncated
    remove(sPath);
    const char *protectedPaths[] = { sPath };
    CHECK(!port_record_create(sPath, protectedPaths, 1, NULL)); // future save target, not yet on disk
    CHECK(!port_record_create(sPath, NULL, 0, argv[1]));        // recording cannot alias run-dir-generated outputs
    char alias[4096];
    CHECK(snprintf(alias, sizeof(alias), "%s/./input.pbwinput", argv[1]) < (int)sizeof(alias));
    protectedPaths[0] = alias;
    CHECK(!port_record_create(sPath, protectedPaths, 1, NULL));
    CHECK(!port_record_create("no/such/recorder/parent/file", NULL, 0, NULL));
    char temporary[4096];
    CHECK(snprintf(temporary, sizeof(temporary), "%s.tmp", sPath) < (int)sizeof(temporary));
    protectedPaths[0] = sPath;
    CHECK(!port_record_create(temporary, protectedPaths, 1, NULL));
#ifndef _WIN32
    char link[4096];
    CHECK(snprintf(link, sizeof(link), "%s/save-link.sav", argv[1]) < (int)sizeof(link));
    remove(link);
    CHECK(symlink("input.pbwinput", link) == 0);
    protectedPaths[0] = link;
    CHECK(!port_record_create(sPath, protectedPaths, 1, NULL));
    remove(link);
#endif
    Start();
    CHECK(!port_record_frame(PBW_INPUT_MAX_FRAME + 1ull, &a));
    CHECK(!port_record_close());
    Start();
    CHECK(port_record_frame(1, &a));
    CHECK(!port_record_frame(1, &a));
    CHECK(!port_record_close());
    Start();
    a.touching = 1;
    a.touchY = 192;
    CHECK(!port_record_frame(1, &a));
    CHECK(!port_record_close());
    a.touchY = 96;
    // Header-only readable prefix holds initial forever; no fabricated zero input at VBlank1.
    Start();
    CHECK(port_record_close());
    back = Load();
    CHECK(back.count == 0 && pbw_input_state(&back, 999).buttons == PBW_INPUT_B);
    pbw_input_free(&back);
    Header(&h);
    h.seedCount = 0;
    remove(sPath);
    CHECK(port_record_create(sPath, NULL, 0, NULL));
    CHECK(!port_record_start(&h));
    CHECK(!port_record_close());
    remove(sPath);
    TestFaults();
    sButtons = PBW_INPUT_X;
    sLid = 1;
    sTouch = 1;
    sX = 255;
    sY = 191;
    a = port_record_input();
    CHECK(a.buttons == PBW_INPUT_X && a.lidClosed && a.touching && a.touchX == 255 && a.touchY == 191);
    sTouch = 0;
    a = port_record_input();
    CHECK(!a.touching && a.touchX == 0 && a.touchY == 0);
    CHECK(port_record_random_seeds(seeds, 64));
    CHECK(!port_record_random_seeds(seeds, 65));
    remove(sPath);
    puts("record: all checks passed");
    return 0;
}
