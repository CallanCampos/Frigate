// The port's run directory (port/app/run_dir.h), in the format pbw-oracle writes and compare.py reads
// (port/tools/oracle/README.md, "Run directories"): frames.tsv with a row at every frame, the screens of chosen frames
// as PNG, dumps of the game's globals at chosen frames, and run.txt (with, once the run ends, the random seeds the game
// used, port/hal/gfl/random_seed.h), so that a run of the game can be held against the melonDS oracle's. It reads the
// game's memory and screens and changes neither. Its files are written in binary mode, so that their lines end with
// "\n" on Windows too, as the oracle's do.
#include "app/run_dir.h"
#include "hal/gfl/random_seed.h"
#include "hal/host/host.h"
#include "hal/rom/sha1.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCREEN_PIXELS (256 * 192)
// A screen is blank when all but this many of its pixels have one colour, as pbw-oracle and compare.py decide it
#define BLANK_SLACK 256
// The longest frame number a list may name, as pbw-oracle's
#define MAX_DIGITS 9

// The longest dump --run-dump takes
#define MAX_DUMP_SIZE (1u << 20)

// A memory dump (--run-dump)
typedef struct Dump {
    char *label;
    const uint8_t *symbol;
    int follow;      // how many pointers lead to the bytes: 0, 1 (*, the pointer at symbol) or 2 (**, the pointer
                     // that one points at)
    uint32_t offset; // from the symbol, or from where the last pointer points
    uint32_t size;
    int lastFrame; // no list: the run's last frame
    PortFrameList frames;
    int warnedNull; // the pointer was NULL once, which was said
} Dump;

static FILE *sFramesTsv;
static FILE *sMemoryTxt;
static PortFrameList sScreens;
static PortRunDirPngWriter sWritePng;
static PortRunDirInfo sInfo;
static char *sDir;
static uint32_t sLastFrame;
static Dump *sDumps;
static size_t sDumpCount;

// Frame lists

// A number of 1 to MAX_DIGITS digits and nothing else
static int ParseNumber(const char *text, size_t length, uint32_t *out) {
    uint32_t value = 0;

    if (length == 0 || length > MAX_DIGITS) {
        return 0;
    }
    for (size_t i = 0; i < length; i++) {
        if (text[i] < '0' || text[i] > '9') {
            return 0;
        }
        value = value * 10 + (uint32_t)(text[i] - '0');
    }
    *out = value;
    return 1;
}

// N, A-B or A-B/STEP, with frames from 1 and A no later than B
static int ParseRange(const char *item, size_t length, PortFrameRange *range) {
    const char *slash = memchr(item, '/', length);
    size_t span = slash != NULL ? (size_t)(slash - item) : length;
    const char *dash = memchr(item, '-', span);

    range->step = 1;
    if (slash != NULL && (!ParseNumber(slash + 1, length - span - 1, &range->step) || range->step == 0)) {
        return 0;
    }
    if (dash == NULL) {
        // A single frame takes no step
        if (slash != NULL || !ParseNumber(item, span, &range->first)) {
            return 0;
        }
        range->last = range->first;
    } else if (!ParseNumber(item, (size_t)(dash - item), &range->first) ||
               !ParseNumber(dash + 1, span - (size_t)(dash - item) - 1, &range->last)) {
        return 0;
    }
    return range->first >= 1 && range->first <= range->last;
}

int port_frame_list_parse(PortFrameList *list, const char *text, char *error, size_t errorSize) {
    const char *at = text;

    memset(list, 0, sizeof(*list));
    for (;;) {
        const char *comma = strchr(at, ',');
        size_t length = comma != NULL ? (size_t)(comma - at) : strlen(at);

        if (length == 3 && strncmp(at, "all", 3) == 0) {
            list->all = 1;
        } else if (!(length == 4 && strncmp(at, "none", 4) == 0)) {
            PortFrameRange range;
            PortFrameRange *grown;

            if (!ParseRange(at, length, &range)) {
                snprintf(error, errorSize, "\"%.*s\" isn't N, A-B, A-B/STEP, all or none", (int)length, at);
                port_frame_list_free(list);
                return 0;
            }
            grown = realloc(list->ranges, (list->count + 1) * sizeof(*grown));
            if (grown == NULL) {
                snprintf(error, errorSize, "out of memory");
                port_frame_list_free(list);
                return 0;
            }
            list->ranges = grown;
            list->ranges[list->count++] = range;
        }
        if (comma == NULL) {
            return 1;
        }
        at = comma + 1;
    }
}

int port_frame_list_contains(const PortFrameList *list, uint32_t frame) {
    if (list->all) {
        return 1;
    }
    for (size_t i = 0; i < list->count; i++) {
        const PortFrameRange *range = &list->ranges[i];
        if (frame >= range->first && frame <= range->last && (frame - range->first) % range->step == 0) {
            return 1;
        }
    }
    return 0;
}

void port_frame_list_free(PortFrameList *list) {
    free(list->ranges);
    memset(list, 0, sizeof(*list));
}

// Screens

void port_run_dir_screen_hash(const uint32_t *pixels, char hex[41]) {
    static const char digits[] = "0123456789abcdef";
    PortSha1 sha1;
    uint8_t digest[PORT_SHA1_DIGEST_SIZE];
    uint8_t row[256 * 3];

    // The RGB bytes of 0xFFRRGGBB, a row at a time
    port_sha1_init(&sha1);
    for (int y = 0; y < 192; y++) {
        for (int x = 0; x < 256; x++) {
            uint32_t pixel = pixels[y * 256 + x];
            row[x * 3] = (uint8_t)(pixel >> 16);
            row[x * 3 + 1] = (uint8_t)(pixel >> 8);
            row[x * 3 + 2] = (uint8_t)pixel;
        }
        port_sha1_update(&sha1, row, sizeof(row));
    }
    port_sha1_final(&sha1, digest);
    for (int i = 0; i < PORT_SHA1_DIGEST_SIZE; i++) {
        hex[i * 2] = digits[digest[i] >> 4];
        hex[i * 2 + 1] = digits[digest[i] & 15];
    }
    hex[40] = '\0';
}

int port_run_dir_screen_blank(const uint32_t *pixels) {
    uint32_t candidate = 0;
    uint32_t votes = 0;
    uint32_t same = 0;

    // The colour that more than half of the screen has, if one does (Boyer and Moore's majority vote), then its count
    for (int i = 0; i < SCREEN_PIXELS; i++) {
        uint32_t pixel = pixels[i] & 0xffffffu;
        if (votes == 0) {
            candidate = pixel;
            votes = 1;
        } else if (pixel == candidate) {
            votes++;
        } else {
            votes--;
        }
    }
    for (int i = 0; i < SCREEN_PIXELS; i++) {
        same += (pixels[i] & 0xffffffu) == candidate;
    }
    return same + BLANK_SLACK >= SCREEN_PIXELS;
}

// The run directory

static char *JoinPath(const char *name) {
    size_t size = strlen(sDir) + 1 + strlen(name) + 1;
    char *path = malloc(size);

    if (path != NULL) {
        snprintf(path, size, "%s/%s", sDir, name);
    }
    return path;
}

static const char *Text(const char *value) {
    return value != NULL && *value != '\0' ? value : "-";
}

// The random seeds the game built (port/hal/gfl/random_seed.h): how many, then each one kept, from 1, with whether the
// input file gave it ("given") or the game's own hash made it ("built")
static void WriteSeeds(FILE *run) {
    size_t i;
    uint64_t seed;
    int given;

    fprintf(run, "seeds: %zu\n", port_random_seed_count());
    for (i = 0; port_random_seed_at(i, &seed, &given); i++) {
        fprintf(run, "seed_%zu: 0x%016llx %s\n", i + 1, (unsigned long long)seed, given ? "given" : "built");
    }
}

// run.txt; finished is 0 while the run goes on, when the frames completed and the status aren't known yet
static void WriteRunTxt(int finished, int status) {
    char *path = JoinPath("run.txt");
    FILE *run = path != NULL ? fopen(path, "wb") : NULL;

    if (run != NULL) {
        fprintf(run, "#pbwrun 1\n");
        fprintf(run, "program: %s\n", Text(sInfo.program));
        fprintf(run, "rom: %s\n", Text(sInfo.rom));
        fprintf(run, "frames: %u\n", sInfo.frames);
        if (finished) {
            fprintf(run, "completed: %u\n", sLastFrame);
        } else {
            fprintf(run, "completed: -\n");
        }
        fprintf(run, "time: %s\n", Text(sInfo.time));
        fprintf(run, "input: %s\n", Text(sInfo.input));
        fprintf(run, "input_frames: %s\n", Text(sInfo.inputFrames));
        fprintf(run, "counter: sVBlankCount\n");
        if (finished) {
            fprintf(run, "status: %d\n", status);
        } else {
            fprintf(run, "status: -\n");
        }
        if (finished) {
            WriteSeeds(run);
        }
        fclose(run);
    }
    free(path);
}

// Memory dumps

// A digit's value in base 10 or 16, or -1
static int DigitValue(char c, int base) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (base == 16 && c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (base == 16 && c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

// A count in decimal or 0x hexadecimal, no more than max
static int ParseCount(const char *text, size_t length, uint32_t max, uint32_t *out) {
    uint64_t value = 0;
    int base = 10;
    size_t i = 0;

    if (length > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        base = 16;
        i = 2;
    }
    if (i == length) {
        return 0;
    }
    for (; i < length; i++) {
        int digit = DigitValue(text[i], base);
        if (digit < 0) {
            return 0;
        }
        value = value * (uint64_t)base + (uint64_t)digit;
        if (value > max) {
            return 0;
        }
    }
    *out = (uint32_t)value;
    return 1;
}

// A C identifier
static int IsSymbol(const char *text, size_t length) {
    if (length == 0 || (text[0] >= '0' && text[0] <= '9')) {
        return 0;
    }
    for (size_t i = 0; i < length; i++) {
        char c = text[i];
        if (!(c == '_' || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
            return 0;
        }
    }
    return 1;
}

static void FreeDump(Dump *dump) {
    free(dump->label);
    port_frame_list_free(&dump->frames);
}

// Split as pbw-oracle's --dump splits it: the list after the last @, the label before the first =, the size after the
// last :
int port_run_dir_add_dump(const char *spec, PortRunDirResolver resolve, char *error, size_t errorSize) {
    const char *end = spec + strlen(spec);
    const char *at = strrchr(spec, '@');
    const char *equals;
    const char *colon;
    const char *target;
    const char *plus;
    char name[256];
    Dump dump;
    Dump *grown;

    memset(&dump, 0, sizeof(dump));
    if (sFramesTsv == NULL) {
        snprintf(error, errorSize, "needs --run-dir");
        return 0;
    }
    if (at != NULL) {
        char listError[128];
        if (!port_frame_list_parse(&dump.frames, at + 1, listError, sizeof(listError))) {
            snprintf(error, errorSize, "%s", listError);
            return 0;
        }
        end = at;
    } else if (sInfo.frames == 0) {
        snprintf(error, errorSize, "without @LIST it dumps at the run's last frame, which needs --frames");
        return 0;
    } else {
        dump.lastFrame = 1;
    }
    equals = memchr(spec, '=', (size_t)(end - spec));
    target = equals != NULL ? equals + 1 : spec;
    colon = NULL;
    for (const char *c = target; c < end; c++) {
        if (*c == ':') {
            colon = c;
        }
    }
    if (colon == NULL || !ParseCount(colon + 1, (size_t)(end - colon - 1), MAX_DUMP_SIZE, &dump.size) ||
        dump.size == 0) {
        snprintf(error, errorSize, "\"%.*s\" needs :SIZE, 1 to %u bytes", (int)(end - spec), spec, MAX_DUMP_SIZE);
        FreeDump(&dump);
        return 0;
    }
    if (equals != NULL) {
        dump.label = malloc((size_t)(equals - spec) + 1);
        if (dump.label != NULL) {
            memcpy(dump.label, spec, (size_t)(equals - spec));
            dump.label[equals - spec] = '\0';
        }
    } else {
        dump.label = malloc((size_t)(colon - spec) + 1);
        if (dump.label != NULL) {
            memcpy(dump.label, spec, (size_t)(colon - spec));
            dump.label[colon - spec] = '\0';
        }
    }
    if (dump.label == NULL || dump.label[0] == '\0' || strpbrk(dump.label, " \t\n") != NULL) {
        snprintf(error, errorSize, "a dump's label is a word without spaces");
        FreeDump(&dump);
        return 0;
    }
    // * follows the pointer at the symbol, ** the pointer that one points at: a static that a host file gives a global
    // pointer to (port/hal/gfl's port_gfl_use_work and port_gfl_std_tables), when the static is a pointer itself
    while (*target == '*' && dump.follow < 2) {
        dump.follow++;
        target++;
    }
    plus = memchr(target, '+', (size_t)(colon - target));
    if (plus != NULL && !ParseCount(plus + 1, (size_t)(colon - plus - 1), UINT32_MAX, &dump.offset)) {
        snprintf(error, errorSize, "\"%.*s\" isn't an offset", (int)(colon - plus - 1), plus + 1);
        FreeDump(&dump);
        return 0;
    }
    if (plus == NULL) {
        plus = colon;
    }
    if (!IsSymbol(target, (size_t)(plus - target)) || (size_t)(plus - target) >= sizeof(name)) {
        snprintf(error, errorSize, "\"%.*s\" isn't a symbol", (int)(plus - target), target);
        FreeDump(&dump);
        return 0;
    }
    memcpy(name, target, (size_t)(plus - target));
    name[plus - target] = '\0';
    dump.symbol = resolve(name);
    if (dump.symbol == NULL) {
        snprintf(error, errorSize, "the program has no global %s (statics aren't visible)", name);
        FreeDump(&dump);
        return 0;
    }
    if (sMemoryTxt == NULL) {
        char *path = JoinPath("memory.txt");
        sMemoryTxt = path != NULL ? fopen(path, "wb") : NULL;
        free(path);
        if (sMemoryTxt == NULL) {
            snprintf(error, errorSize, "can't write %s/memory.txt", sDir);
            FreeDump(&dump);
            return 0;
        }
        fprintf(sMemoryTxt, "#pbwmem 1\n");
    }
    grown = realloc(sDumps, (sDumpCount + 1) * sizeof(*grown));
    if (grown == NULL) {
        snprintf(error, errorSize, "out of memory");
        FreeDump(&dump);
        return 0;
    }
    sDumps = grown;
    sDumps[sDumpCount++] = dump;
    return 1;
}

// The dumps of a frame, a line each: FRAME LABEL ADDRESS SIZE HEX
static int WriteDumps(uint32_t frame) {
    static const char digits[] = "0123456789abcdef";

    for (size_t i = 0; i < sDumpCount; i++) {
        Dump *dump = &sDumps[i];
        const uint8_t *from = dump->symbol;

        if (dump->lastFrame ? frame != sInfo.frames : !port_frame_list_contains(&dump->frames, frame)) {
            continue;
        }
        // Each * reads the pointer stored where the last one led
        for (int k = 0; k < dump->follow && from != NULL; k++) {
            const uint8_t *next;
            memcpy(&next, from, sizeof(next));
            from = next;
        }
        if (from == NULL) {
            if (!dump->warnedNull) {
                fprintf(stderr, "port: --run-dump %s: the pointer is NULL at frame %u; no dump there\n", dump->label,
                        frame);
                dump->warnedNull = 1;
            }
            continue;
        }
        from += dump->offset;
        fprintf(sMemoryTxt, "%u %s 0x%llx %u ", frame, dump->label, (unsigned long long)(uintptr_t)from, dump->size);
        for (uint32_t j = 0; j < dump->size; j++) {
            fputc(digits[from[j] >> 4], sMemoryTxt);
            fputc(digits[from[j] & 15], sMemoryTxt);
        }
        fputc('\n', sMemoryTxt);
    }
    if (sMemoryTxt != NULL && fflush(sMemoryTxt) != 0) {
        fprintf(stderr, "port: --run-dir: can't write %s/memory.txt\n", sDir);
        return 0;
    }
    return 1;
}

int port_run_dir_open(const char *dir, const char *screens, PortRunDirPngWriter writePng, const PortRunDirInfo *info) {
    char error[160];
    char *path;

    if (!port_frame_list_parse(&sScreens, screens != NULL ? screens : "none", error, sizeof(error))) {
        fprintf(stderr, "port: --run-screens: %s\n", error);
        return 0;
    }
    if (port_host_make_directory(dir) != 0 && errno != EEXIST) {
        fprintf(stderr, "port: --run-dir: can't create %s: %s\n", dir, strerror(errno));
        port_frame_list_free(&sScreens);
        return 0;
    }
    sDir = malloc(strlen(dir) + 1);
    if (sDir == NULL) {
        port_frame_list_free(&sScreens);
        return 0;
    }
    strcpy(sDir, dir);
    path = JoinPath("frames.tsv");
    sFramesTsv = path != NULL ? fopen(path, "wb") : NULL;
    if (sFramesTsv == NULL) {
        fprintf(stderr, "port: --run-dir: can't write %s/frames.tsv\n", dir);
        free(path);
        port_run_dir_close(0);
        return 0;
    }
    free(path);
    fprintf(sFramesTsv, "#pbwframes 2\n");
    fprintf(sFramesTsv,
            "frame\tvblank\tcounter\tgame_frame\tkeyread\ttop_3d\tbottom_3d\ttop\tbottom\ttop_blank\tbottom_blank\n");
    sWritePng = writePng;
    sInfo = *info;
    sLastFrame = 0;
    // A run that a trap or OS_Terminate ends never closes the directory: its run.txt says so
    WriteRunTxt(0, 0);
    return 1;
}

int port_run_dir_active(void) {
    return sFramesTsv != NULL;
}

int port_run_dir_frame(const PortRunDirFrame *frame, const uint32_t *top, const uint32_t *bottom) {
    char topHash[41];
    char bottomHash[41];

    if (sFramesTsv == NULL) {
        return 1;
    }
    port_run_dir_screen_hash(top, topHash);
    port_run_dir_screen_hash(bottom, bottomHash);
    // The port has no lag flag and doesn't decide which screens show 3D: compare.py takes those from the oracle's run
    fprintf(sFramesTsv, "%u\t%u\t%u\t%u\t-\t-\t-\t%s\t%s\t%d\t%d\n", frame->frame, frame->vblank, frame->counter,
            frame->gameFrame, topHash, bottomHash, port_run_dir_screen_blank(top), port_run_dir_screen_blank(bottom));
    if (fflush(sFramesTsv) != 0) {
        fprintf(stderr, "port: --run-dir: can't write %s/frames.tsv\n", sDir);
        return 0;
    }
    sLastFrame = frame->frame;
    if (!WriteDumps(frame->frame)) {
        return 0;
    }
    if (sWritePng != NULL && port_frame_list_contains(&sScreens, frame->frame)) {
        char name[32];
        char *path;
        int ok;

        snprintf(name, sizeof(name), "frame_%06u.png", frame->frame);
        path = JoinPath(name);
        ok = path != NULL && sWritePng(path);
        free(path);
        if (!ok) {
            fprintf(stderr, "port: --run-dir: can't write %s/%s\n", sDir, name);
            return 0;
        }
    }
    return 1;
}

void port_run_dir_close(int status) {
    if (sDir == NULL) {
        return;
    }
    if (sFramesTsv != NULL) {
        WriteRunTxt(1, status);
        fclose(sFramesTsv);
        sFramesTsv = NULL;
    }
    if (sMemoryTxt != NULL) {
        fclose(sMemoryTxt);
        sMemoryTxt = NULL;
    }
    for (size_t i = 0; i < sDumpCount; i++) {
        FreeDump(&sDumps[i]);
    }
    free(sDumps);
    sDumps = NULL;
    sDumpCount = 0;
    port_frame_list_free(&sScreens);
    free(sDir);
    sDir = NULL;
    sWritePng = NULL;
}
