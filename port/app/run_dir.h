// The port's run directory, for comparing it with the melonDS oracle (port/tools/oracle/README.md, "Run directories"):
// frames.tsv in its version 2 with a row at every frame hook, both screens as frame_NNNNNN.png at the frames of a list,
// and run.txt, so that one run of the program gives compare.py every frame it pairs. The program writes it with
// --run-dir and --run-screens (port/app/main.c). Plain C types only.
#ifndef PORT_APP_RUN_DIR_H
#define PORT_APP_RUN_DIR_H

#include <stddef.h>
#include <stdint.h>

// A list of frames as pbw-oracle's --screens takes it: N, A-B or A-B/STEP (frames from 1, at most 9 digits), all or
// none, comma-separated
typedef struct PortFrameRange {
    uint32_t first;
    uint32_t last;
    uint32_t step;
} PortFrameRange;

typedef struct PortFrameList {
    PortFrameRange *ranges;
    size_t count;
    int all;
} PortFrameList;

// Parses text into list (initialised here). Returns 1, or 0 with a message naming the item in error
int port_frame_list_parse(PortFrameList *list, const char *text, char *error, size_t errorSize);
int port_frame_list_contains(const PortFrameList *list, uint32_t frame);
void port_frame_list_free(PortFrameList *list);

// What a frame's row records besides its screens, at the frame hook of VBlank `frame`
typedef struct PortRunDirFrame {
    uint32_t frame;     // the video frame: the VBlanks since the game's main() started, from 1
    uint32_t vblank;    // NitroSDK's VBlank count (HW_VBLANK_COUNT_BUF) as the game would read it
    uint32_t counter;   // GFL's frame count (sVBlankCount, port/hal/gfl/gfl_frame.h)
    uint32_t gameFrame; // the game frame whose input the game reads after this VBlank (port/app/replay.h)
} PortRunDirFrame;

// Writes both screens, the top one above the bottom one, as a 256x384 PNG; returns 0 when it can't
typedef int (*PortRunDirPngWriter)(const char *path);

// What run.txt says of the run, besides the frames; NULL is written as "-"
typedef struct PortRunDirInfo {
    const char *program; // the program's name
    const char *rom;
    const char *time;        // the clock at power-on
    const char *input;       // the input file
    const char *inputFrames; // "game" or "video"
    uint32_t frames;         // the frames asked for (--frames), 0 for no limit
} PortRunDirInfo;

// Creates dir (one level) when it doesn't exist and opens frames.tsv there; the PNG files of the frames of screens
// (NULL for none) are written with writePng. Returns 0, after printing why, when it can't
int port_run_dir_open(const char *dir, const char *screens, PortRunDirPngWriter writePng, const PortRunDirInfo *info);
// Whether a run directory is open
int port_run_dir_active(void);
// The row of a frame, from the screens' pixels (192 rows of 256, 0xFFRRGGBB as port/gfx/screens.h gives them), and
// its PNG file when the list has the frame. Each row reaches the file at once, so that a run that a trap ends keeps
// the frames before it. Returns 0, after printing why, when a file can't be written
int port_run_dir_frame(const PortRunDirFrame *frame, const uint32_t *top, const uint32_t *bottom);
// Writes run.txt with the frames completed and the exit status, and closes the files
void port_run_dir_close(int status);

// The SHA-1 of a screen's RGB bytes as frames.tsv gives it (40 hexadecimal digits and a NUL), and whether the screen
// is blank: all but 256 of its pixels have one colour (compare.py's BLANK_SLACK)
void port_run_dir_screen_hash(const uint32_t *pixels, char hex[41]);
int port_run_dir_screen_blank(const uint32_t *pixels);

#endif // PORT_APP_RUN_DIR_H
