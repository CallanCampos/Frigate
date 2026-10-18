#include "types.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "nitro/os.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wordset.h"

// The game's printing of text: a renderer that draws a string's characters into a bitmap, carrying out the string's
// commands; a queue that spreads printing over frames while the game is connected; a stream that prints into a window
// a few characters a frame; and the measures of strings in a font

// The characters that end a string or a line, and that start a command: the command's index in its category and its
// category in the next character, then the count of parameters and the parameters
#define CHAR_EOM 0xffff
#define CHAR_NEWLINE 0xfffe
#define STRCMD_CHAR 0xf000

#define STRCMD_CATEGORY_LAYOUT 0xbd
#define STRCMD_CATEGORY_STREAM 0xbe
#define STRCMD_CATEGORY_COLOR 0xff

// A color of letter, shadow and background indices
#define TEXT_COLOR(letter, shadow, background)                                                                         \
    ((((letter) & 0x1f) << 10) | (((shadow) & 0x1f) << 5) | ((background) & 0x1f))
#define TEXT_COLOR_LETTER(color) ((u8)(((color) >> 10) & 0x1f))
#define TEXT_COLOR_SHADOW(color) ((u8)(((color) >> 5) & 0x1f))
#define TEXT_COLOR_BACKGROUND(color) ((u8)((color) & 0x1f))

typedef struct TextRenderer TextRenderer;
typedef void (*TextRndDrawCharProc)(GFLBitmap *bitmap, int x, int y, Font *font, u16 c, GlyphInfo *info);

// Whether the renderer's color is set, and set in the glyphs' color table
enum {
    TEXT_RND_COLOR_GLOBAL,
    TEXT_RND_COLOR_PENDING,
    TEXT_RND_COLOR_APPLIED,
};

struct TextRenderer {
    s16 x;
    s16 y;
    s32 startX : 10;
    s32 startY : 10;
    s32 colorState : 4;
    s32 : 8;
    u32 unk8;
    // The color before the string's color commands, and the color they set
    u16 color;
    u16 defaultColor;
    Font *font;
    GFLBitmap *bitmap;
    TextRndDrawCharProc drawChar;
};

// A ring buffer of strings to print, each followed by its renderer and a pointer to the next one's renderer
struct PrintQueue {
    u16 writePos;
    u16 readPos;
    u16 size;
    u8 frames;
    u8 unk7 : 4;
    // Queues all text, not only while connected
    u8 queueAlways : 4;
    // The time a frame may spend printing
    u64 timeLimit;
    TextRenderer *job;
    const u16 *cursor;
    u8 buffer[];
};

struct PrintStream {
    u32 state;
    // How a page break continues: scrolling the text up a line, or clearing the window
    u32 pageMode;
    BmpWin *window;
    GFLBitmap *bitmap;
    TCBEx *task;
    const u16 *cursor;
    // Frames between steps and characters a step, as the stream started, as now, and when sped up
    u8 baseWait;
    u8 baseCharsPerStep;
    u8 wait;
    u8 charsPerStep;
    u8 fastWait;
    u8 fastCharsPerStep;
    u8 waitTimer;
    u8 scrollOffset;
    u8 backgroundColor;
    // Set while the callback holds the stream
    u8 callbackWait;
    u8 paused : 1;
    u8 scrolling : 1;
    u8 continueRequested : 1;
    u8 fast : 1;
    u8 fastPending : 1;
    u8 : 3;
    PrintStreamCallback callback;
    u32 defaultEvent;
    u32 nextEvent;
    u32 event;
    TextRenderer renderer;
};

enum {
    PRINT_STREAM_PAGE_SCROLL,
    PRINT_STREAM_PAGE_CLEAR,
};

GFLBitmap *g_TextRendererTempBitmap;
TextRenderer g_TextRenderer;

static void func_02021cb0(PrintQueue *queue, TextRenderer *renderer, const StrBuf *strbuf);
static void GFL_TextRndBind(TextRenderer *renderer, Font *font, GFLBitmap *bitmap, s16 x, s16 y);
static void GFL_TextRndSetDefaultColor(TextRenderer *renderer, u16 color);
static void GFL_TextRndDrawStrBuf(TextRenderer *renderer, const StrBuf *strbuf);
static const u16 *GFL_TextRndDrawString(TextRenderer *renderer, const u16 *str, BOOL skipStreamCmds);
static void GFL_TextRndDrawCharProc_IDX4(GFLBitmap *bitmap, int x, int y, Font *font, u16 c, GlyphInfo *info);
static void GFL_TextRndDrawCharProc_IDX8(GFLBitmap *bitmap, int x, int y, Font *font, u16 c, GlyphInfo *info);
static const u16 *GFL_TextRndExecLayoutCmd(TextRenderer *renderer, const u16 *cmd);
static const u16 *GFL_TextRndExecColorCmd(TextRenderer *renderer, const u16 *cmd);
static void func_02022378(s32 wait, u8 *steps);
static void func_02022464(TCBEx *task, void *data);
static void func_02022694(PrintStream *stream);
static u32 func_02022c08(const StrBuf *strbuf);
static u32 func_02022c24(const StrBuf *strbuf);
static u32 func_02022c30(PrintQueue *queue);
static void func_02022c44(PrintQueue *queue, TextRenderer *renderer, const StrBuf *strbuf);
static void *func_02022cd4(PrintQueue *queue, const void *src, u16 size, u16 chunk);
static TextRenderer *func_02022d30(PrintQueue *queue, TextRenderer *job);

static inline u32 StrCmd_Category(const u16 *cmd) {
    return *cmd == STRCMD_CHAR ? (u8)(cmd[1] >> 8) : 0xff;
}

static inline u16 StrCmd_Index(const u16 *cmd) {
    if (*cmd == STRCMD_CHAR) {
        return cmd[1] & 0xff;
    }
    return 0xffff;
}

static inline u32 StrCmd_Param(const u16 *cmd, int index) {
    return *cmd == STRCMD_CHAR ? (cmd + index)[3] : 0;
}

static inline u16 PrintQueue_Advance(PrintQueue *queue, u16 pos, u16 n) {
    pos += n;
    if (pos >= queue->size) {
        pos = 0;
    }
    return pos;
}

static inline const u16 *StrCmd_Skip(const u16 *cmd) {
    if (*cmd == STRCMD_CHAR) {
        const u16 *end = cmd + 3;

        end += cmd[2];
        return end;
    }
    return cmd;
}

void GFL_TextRndInit(HeapID heapId) {
    GFL_StrBufSetTerminator(CHAR_EOM);
    g_TextRendererTempBitmap = GFL_BitmapCreate(2, 2, 0x20, heapId);
}

PrintQueue *func_02021998(HeapID heapId) {
    return func_020219a8(0x400, heapId);
}

PrintQueue *func_020219a8(u16 size, HeapID heapId) {
    u16 bufferSize = size + sizeof(TextRenderer);
    PrintQueue *queue;

    while (bufferSize % 4 != 0) {
        bufferSize++;
    }
    queue = GFL_HeapAllocate(heapId, sizeof(PrintQueue) + bufferSize, FALSE, "printsys.c", 291);
    queue->size = size;
    queue->timeLimit = 0x1130;
    queue->job = NULL;
    queue->cursor = NULL;
    queue->unk7 = 0;
    queue->queueAlways = 0;
    queue->frames = 0;
    queue->writePos = 0;
    queue->readPos = size;
    return queue;
}

void func_02021a18(PrintQueue *queue) {
    GFL_HeapFree(queue);
}

void func_02021a20(PrintQueue *queue, u32 queueAlways) {
    queue->queueAlways = (u8)queueAlways;
}

void func_02021a34(PrintQueue *queue, u64 timeLimit) {
    queue->timeLimit = timeLimit;
}

// Prints what the queue holds until the frame's time runs out. Returns whether it is empty
BOOL func_02021a3c(PrintQueue *queue) {
    BOOL timeout = FALSE;
    BOOL setColor;
    u16 savedColor;
    u8 letter, shadow, background;
    u8 savedLetter, savedShadow, savedBackground;
    u64 start;

    queue->frames++;
    if (queue->job == NULL) {
        return TRUE;
    }

    GFL_TextRndGetGlobalColors(&savedLetter, &savedShadow, &savedBackground);
    savedColor = TEXT_COLOR(savedLetter, savedShadow, savedBackground);
    setColor = timeout;
    if (queue->job->colorState == TEXT_RND_COLOR_GLOBAL || queue->job->colorState == TEXT_RND_COLOR_APPLIED) {
        letter = TEXT_COLOR_LETTER(queue->job->defaultColor);
        shadow = TEXT_COLOR_SHADOW(queue->job->defaultColor);
        background = TEXT_COLOR_BACKGROUND(queue->job->defaultColor);
        setColor = TRUE;
    }
    if (setColor && GFL_TextRndCheckColorIndexChange(letter, shadow, background)) {
        GFL_TextRndUpdateColorIndexLUT(letter, shadow, background);
    }

    start = OS_GetTick();
    while (queue->job != NULL) {
        if (queue->job->colorState == TEXT_RND_COLOR_PENDING) {
            u8 jobLetter = TEXT_COLOR_LETTER(queue->job->defaultColor);
            u8 jobShadow = TEXT_COLOR_SHADOW(queue->job->defaultColor);
            u8 jobBackground = TEXT_COLOR_BACKGROUND(queue->job->defaultColor);

            if (GFL_TextRndCheckColorIndexChange(jobLetter, jobShadow, jobBackground)) {
                GFL_TextRndUpdateColorIndexLUT(jobLetter, jobShadow, jobBackground);
            }
            queue->job->colorState = TEXT_RND_COLOR_APPLIED;
        }
        while (*queue->cursor != CHAR_EOM) {
            queue->cursor = GFL_TextRndDrawString(queue->job, queue->cursor, TRUE);
            if (OS_GetTick() - start > queue->timeLimit) {
                timeout = TRUE;
                break;
            }
        }
        if (*queue->cursor == CHAR_EOM) {
            u16 offset = (u32)queue->job - (u32)queue->buffer;

            queue->job = func_02022d30(queue, queue->job);
            if (queue->job != NULL) {
                // The next string follows this renderer and its link
                offset = PrintQueue_Advance(queue, offset, sizeof(TextRenderer));
                offset = PrintQueue_Advance(queue, offset, sizeof(TextRenderer *));
                queue->cursor = (const u16 *)(queue->buffer + offset);
            }
        }
        if (timeout) {
            break;
        }
    }

    GFL_TextRndUpdateColorIndexLUT(TEXT_COLOR_LETTER(savedColor), TEXT_COLOR_SHADOW(savedColor),
                                   TEXT_COLOR_BACKGROUND(savedColor));
    if (queue->job != NULL) {
        queue->readPos = (u8 *)queue->cursor - queue->buffer;
        return FALSE;
    }
    queue->writePos = 0;
    queue->readPos = queue->size;
    queue->unk7 = 0;
    return TRUE;
}

BOOL func_02021c0c(PrintQueue *queue) {
    if (queue->job == NULL) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_02021c1c(PrintQueue *queue, GFLBitmap *bitmap) {
    TextRenderer *job;

    if (queue->job != NULL) {
        for (job = queue->job; job != NULL; job = func_02022d30(queue, job)) {
            if (job->bitmap == bitmap) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

void func_02021c44(PrintQueue *queue) {
    queue->writePos = 0;
    queue->readPos = queue->size;
    queue->cursor = NULL;
    queue->job = NULL;
}

void func_02021c54(PrintQueue *queue, GFLBitmap *bitmap, s16 x, s16 y, const StrBuf *strbuf, Font *font) {
    GFL_TextRndBind(&g_TextRenderer, font, bitmap, x, y);
    func_02021cb0(queue, &g_TextRenderer, strbuf);
}

void func_02021c7c(PrintQueue *queue, GFLBitmap *bitmap, s16 x, s16 y, const StrBuf *strbuf, Font *font, u16 color) {
    GFL_TextRndBind(&g_TextRenderer, font, bitmap, x, y);
    GFL_TextRndSetDefaultColor(&g_TextRenderer, color);
    func_02021cb0(queue, &g_TextRenderer, strbuf);
}

static inline BOOL PrintQueue_IsConnected(void) {
    if (func_02042a78()) {
        return TRUE;
    }
    return FALSE;
}

// Prints at once, unless connected, when the string goes in the queue if it fits
static void func_02021cb0(PrintQueue *queue, TextRenderer *renderer, const StrBuf *strbuf) {
    if (!PrintQueue_IsConnected() && queue->queueAlways == 0) {
        GFL_TextRndDrawStrBuf(renderer, strbuf);
        return;
    }
    if (func_02022c24(strbuf) <= func_02022c30(queue)) {
        func_02022c44(queue, renderer, strbuf);
    }
}

void GFL_TextRendererDrawToBitmap(GFLBitmap *bitmap, s16 x, s16 y, const StrBuf *strbuf, Font *font) {
    GFL_StrBufGetStringPtr(strbuf);
    GFL_TextRndBind(&g_TextRenderer, font, bitmap, x, y);
    GFL_TextRndDrawStrBuf(&g_TextRenderer, strbuf);
}

void GFL_TextRendererDrawToBitmapEx(GFLBitmap *bitmap, s16 x, s16 y, const StrBuf *strbuf, Font *font, u16 color) {
    GFL_StrBufGetStringPtr(strbuf);
    GFL_TextRndBind(&g_TextRenderer, font, bitmap, x, y);
    GFL_TextRndSetDefaultColor(&g_TextRenderer, color);
    GFL_TextRndDrawStrBuf(&g_TextRenderer, strbuf);
}

static void GFL_TextRndBind(TextRenderer *renderer, Font *font, GFLBitmap *bitmap, s16 x, s16 y) {
    u8 letter, shadow, background;

    renderer->bitmap = bitmap;
    if (GFL_BitmapGetBytesPerTile(bitmap) == 0x20) {
        renderer->drawChar = GFL_TextRndDrawCharProc_IDX4;
    } else {
        renderer->drawChar = GFL_TextRndDrawCharProc_IDX8;
    }
    renderer->font = font;
    renderer->x = x;
    renderer->startX = renderer->x;
    renderer->y = y;
    renderer->startY = renderer->y;
    renderer->colorState = TEXT_RND_COLOR_PENDING;
    GFL_TextRndGetGlobalColors(&letter, &shadow, &background);
    renderer->color = TEXT_COLOR(letter, shadow, background);
    renderer->defaultColor = renderer->color;
}

static void GFL_TextRndSetDefaultColor(TextRenderer *renderer, u16 color) {
    u16 newColor =
        TEXT_COLOR(TEXT_COLOR_LETTER(color), TEXT_COLOR_SHADOW(color), TEXT_COLOR_BACKGROUND(renderer->color));

    renderer->defaultColor = color;
    renderer->color = newColor;
    renderer->colorState = TEXT_RND_COLOR_PENDING;
}

static void GFL_TextRndDrawStrBuf(TextRenderer *renderer, const StrBuf *strbuf) {
    const u16 *str = GFL_StrBufGetStringPtr(strbuf);
    u8 restore;
    u8 savedLetter, savedShadow, savedBackground;

    savedLetter = savedShadow = savedBackground = restore = FALSE;
    if (renderer->colorState == TEXT_RND_COLOR_PENDING) {
        u8 letter = TEXT_COLOR_LETTER(renderer->defaultColor);
        u8 shadow = TEXT_COLOR_SHADOW(renderer->defaultColor);
        u8 background = TEXT_COLOR_BACKGROUND(renderer->defaultColor);

        if (GFL_TextRndCheckColorIndexChange(letter, shadow, background)) {
            GFL_TextRndGetGlobalColors(&savedLetter, &savedShadow, &savedBackground);
            GFL_TextRndUpdateColorIndexLUT(letter, shadow, background);
            restore = TRUE;
        }
        renderer->colorState = TEXT_RND_COLOR_APPLIED;
    }
    while (*str != CHAR_EOM) {
        str = GFL_TextRndDrawString(renderer, str, TRUE);
    }
    if (restore) {
        GFL_TextRndUpdateColorIndexLUT(savedLetter, savedShadow, savedBackground);
    }
}

// Draws the next character, carrying out the commands before it. Returns what follows it, or the end of the string,
// or a stream command when not skipping them
static const u16 *GFL_TextRndDrawString(TextRenderer *renderer, const u16 *str, BOOL skipStreamCmds) {
    GlyphInfo info;

    for (;;) {
        switch (*str) {
        case CHAR_EOM:
            return str;
        case CHAR_NEWLINE:
            str++;
            renderer->x = renderer->startX;
            renderer->y += 16;
            break;
        case STRCMD_CHAR:
            switch (StrCmd_Category(str)) {
            case STRCMD_CATEGORY_LAYOUT:
                str = GFL_TextRndExecLayoutCmd(renderer, str);
                break;
            case STRCMD_CATEGORY_COLOR:
                str = GFL_TextRndExecColorCmd(renderer, str);
                break;
            case STRCMD_CATEGORY_STREAM:
                if (skipStreamCmds) {
                    str = StrCmd_Skip(str);
                    break;
                }
                return str;
            default:
                str = StrCmd_Skip(str);
                break;
            }
            break;
        default:
            renderer->drawChar(renderer->bitmap, renderer->x, renderer->y, renderer->font, *str, &info);
            renderer->x += info.advance;
            return str + 1;
        }
    }
}

static void GFL_TextRndDrawCharProc_IDX4(GFLBitmap *bitmap, int x, int y, Font *font, u16 c, GlyphInfo *info) {
    GFL_FontGetGlyph(font, c, GFL_BitmapGetPixelData(g_TextRendererTempBitmap), info);
    GFL_BitmapCopyArea(g_TextRendererTempBitmap, bitmap, 0, 0, x + info->x, y, info->width, info->height, 0);
}

static void GFL_TextRndDrawCharProc_IDX8(GFLBitmap *bitmap, int x, int y, Font *font, u16 c, GlyphInfo *info) {
    GFL_FontGetGlyph(font, c, GFL_BitmapGetPixelData(g_TextRendererTempBitmap), info);
    GFL_BitmapCopyAreaRebased(g_TextRendererTempBitmap, bitmap, 0, 0, x + info->x, y, info->width, info->height, 0xf,
                              0);
}

static const u16 *GFL_TextRndExecLayoutCmd(TextRenderer *renderer, const u16 *cmd) {
    switch (StrCmd_Index(cmd)) {
    case 0: {
        u8 letter = StrCmd_Param(cmd, 0);
        u8 shadow = StrCmd_Param(cmd, 1);
        u8 background = StrCmd_Param(cmd, 2);

        GFL_TextRndUpdateColorIndexLUT(letter, shadow, background);
        renderer->defaultColor = TEXT_COLOR(letter, shadow, background);
        break;
    }
    case 1:
        GFL_TextRndUpdateColorIndexLUT(TEXT_COLOR_LETTER(renderer->color), TEXT_COLOR_SHADOW(renderer->color),
                                       TEXT_COLOR_BACKGROUND(renderer->color));
        break;
    case 3: {
        // Aligns the line to the right, less a margin
        int width = GFL_BitmapGetWidth(renderer->bitmap) - renderer->startX;
        int lineWidth = GFL_FontGetLineWidth(cmd, renderer->font, 0, NULL);
        u32 margin = StrCmd_Param(cmd, 0);

        renderer->x = renderer->startX + (width - lineWidth) - 1 - margin;
        break;
    }
    case 2: {
        // Centers the line
        int width = GFL_BitmapGetWidth(renderer->bitmap) - renderer->startX;
        int lineWidth = GFL_FontGetLineWidth(cmd, renderer->font, 0, NULL);

        if (lineWidth < width) {
            renderer->x = renderer->startX + (width - lineWidth) / 2;
        } else {
            renderer->x = renderer->startX;
        }
        break;
    }
    case 4:
        renderer->x = renderer->x + (s16)StrCmd_Param(cmd, 0);
        break;
    case 5:
        renderer->x = StrCmd_Param(cmd, 0);
        break;
    }
    return StrCmd_Skip(cmd);
}

static const u16 *GFL_TextRndExecColorCmd(TextRenderer *renderer, const u16 *cmd) {
    if (StrCmd_Index(cmd) == 0) {
        u8 color = StrCmd_Param(cmd, 0);

        if (color == 0xff) {
            GFL_TextRndUpdateColorIndexLUT(TEXT_COLOR_LETTER(renderer->defaultColor),
                                           TEXT_COLOR_SHADOW(renderer->defaultColor),
                                           TEXT_COLOR_BACKGROUND(renderer->defaultColor));
        } else {
            // Each color is a pair of letter and shadow indices from 1
            u8 background = TEXT_COLOR_BACKGROUND(renderer->color);

            GFL_TextRndUpdateColorIndexLUT(color * 2 + 1, color * 2 + 2, background);
            renderer->defaultColor = TEXT_COLOR((u8)(color * 2 + 1), (u8)(color * 2 + 2), background);
        }
    }
    return StrCmd_Skip(cmd);
}

PrintStream *func_02022268(BmpWin *window, s16 x, s16 y, const StrBuf *strbuf, Font *font, s32 wait,
                           TCBExManager *tcbManager, u32 a7, HeapID heapId, u16 a9) {
    return func_02022294(window, x, y, strbuf, font, wait, tcbManager, a7, heapId, a9, NULL);
}

PrintStream *func_02022294(BmpWin *window, s16 x, s16 y, const StrBuf *strbuf, Font *font, s32 wait,
                           TCBExManager *tcbManager, u32 a7, HeapID heapId, u16 a9, PrintStreamCallback callback) {
    GFLBitmap *bitmap = BmpWin_GetBitmap(window);
    TCBEx *task = GFL_TCBExMgrAddTask(tcbManager, func_02022464, sizeof(PrintStream), a7);
    PrintStream *stream = GFL_TCBExGetData(task);
    u8 steps[2];

    stream->task = task;
    GFL_TextRndBind(&stream->renderer, font, bitmap, x, y);
    func_02022378(wait, steps);
    stream->baseCharsPerStep = steps[1];
    stream->wait = stream->fastWait = stream->baseWait = steps[0];
    stream->charsPerStep = stream->fastCharsPerStep = stream->baseCharsPerStep;
    stream->cursor = GFL_StrBufGetStringPtr(strbuf);
    stream->waitTimer = 0;
    stream->callback = callback;
    stream->defaultEvent = 0;
    stream->nextEvent = 0;
    stream->event = 0;
    stream->window = window;
    stream->bitmap = bitmap;
    stream->state = PRINT_STREAM_RUNNING;
    stream->backgroundColor = a9;
    stream->pageMode = PRINT_STREAM_PAGE_CLEAR;
    stream->scrollOffset = 0;
    stream->continueRequested = FALSE;
    stream->paused = FALSE;
    stream->scrolling = FALSE;
    stream->callbackWait = FALSE;
    stream->fast = FALSE;
    stream->fastPending = FALSE;
    return stream;
}

// A wait of 0 or more frames between single characters, or a negative count of characters a frame
static void func_02022378(s32 wait, u8 *steps) {
    if (wait >= 0) {
        steps[1] = 1;
        steps[0] = wait;
    } else {
        steps[1] = -wait;
        steps[0] = 0;
    }
}

void func_02022390(PrintStream *stream) {
    stream->paused = TRUE;
}

void func_020223a4(PrintStream *stream) {
    stream->paused = FALSE;
}

u32 func_020223b4(PrintStream *stream) {
    return stream->state;
}

u32 func_020223b8(PrintStream *stream) {
    return stream->pageMode;
}

void func_020223bc(PrintStream *stream) {
    stream->continueRequested = TRUE;
}

void func_020223cc(PrintStream *stream) {
    if (stream->task != NULL) {
        GFL_TCBExRequestEnd(stream->task);
        stream->task = NULL;
    }
}

// Cuts the wait short, and prints at the fast speed from then on
void func_020223e0(PrintStream *stream, s32 waitTimer) {
    if (waitTimer < stream->waitTimer) {
        stream->waitTimer = waitTimer;
        if (waitTimer == 0) {
            func_02022410(stream, func_02017c50(2));
            stream->fastPending = TRUE;
        }
    }
}

void func_02022410(PrintStream *stream, s32 wait) {
    u8 steps[2];

    func_02022378(wait, steps);
    stream->fastCharsPerStep = steps[1];
    stream->fastWait = steps[0];
    stream->fast = TRUE;
}

void func_0202243c(PrintStream *stream) {
    stream->fast = FALSE;
    stream->fastCharsPerStep = stream->baseCharsPerStep;
    stream->fastWait = stream->baseWait;
}

BOOL func_02022458(PrintStream *stream) {
    return stream->fast;
}

static void func_02022464(TCBEx *task, void *data) {
    PrintStream *stream = data;

    if (stream->callback != NULL && stream->callbackWait) {
        stream->callbackWait = stream->callback(stream->event);
        if (stream->callbackWait) {
            return;
        }
        stream->event = stream->nextEvent;
    }
    if (stream->paused) {
        return;
    }
    switch (stream->state) {
    case PRINT_STREAM_RUNNING:
        if (stream->scrolling) {
            // Scrolls the window up a line, 4 pixels a frame
            if (stream->scrollOffset < 16) {
                u16 step = 4;
                u16 width;
                int height;

                if (stream->scrollOffset + 4 > 16) {
                    step = 16 - stream->scrollOffset;
                }
                stream->scrollOffset += (u8)step;
                width = GFL_BitmapGetWidth(stream->bitmap);
                height = GFL_BitmapGetHeight(stream->bitmap) - step;
                GFL_BitmapCopyArea(stream->bitmap, stream->bitmap, 0, step, 0, 0, width, height, 0xffff);
                GFL_BitmapFillArea(stream->bitmap, 0, (s16)height, width, step, stream->backgroundColor);
                BmpWin_FlushChar(stream->window);
            } else {
                stream->renderer.x = stream->renderer.startX;
                stream->renderer.y = 16;
                stream->scrolling = FALSE;
            }
            break;
        }
        if (stream->waitTimer == 0) {
            int chars = stream->fast ? stream->fastCharsPerStep : stream->charsPerStep;
            int i;

            if (stream->fastPending) {
                func_0202243c(stream);
                stream->fastPending = FALSE;
            }
            for (i = 0; i < chars; i++) {
                const u16 *str = stream->cursor;

                switch (*str) {
                case CHAR_EOM:
                    stream->state = PRINT_STREAM_DONE;
                    break;
                case STRCMD_CHAR:
                    if (StrCmd_Category(str) == STRCMD_CATEGORY_STREAM) {
                        func_02022694(stream);
                        if (stream->callback != NULL) {
                            stream->callbackWait = stream->callback(stream->event);
                            if (stream->callbackWait) {
                                i = chars;
                            } else {
                                stream->event = stream->nextEvent;
                            }
                        }
                        if (stream->state != PRINT_STREAM_RUNNING || stream->waitTimer != 0) {
                            i = chars;
                        }
                        break;
                    }
                    // fallthrough
                default:
                    stream->cursor = GFL_TextRndDrawString(&stream->renderer, str, FALSE);
                    if (*stream->cursor == CHAR_EOM) {
                        stream->state = PRINT_STREAM_DONE;
                    } else if (*stream->cursor != CHAR_NEWLINE) {
                        stream->waitTimer = stream->fast ? stream->fastWait : stream->wait;
                    }
                    BmpWin_FlushChar(stream->window);
                    if (stream->callback != NULL) {
                        stream->callbackWait = stream->callback(stream->event);
                        if (stream->callbackWait) {
                            i = chars;
                        } else {
                            stream->event = stream->nextEvent;
                        }
                    }
                    break;
                }
            }
        } else {
            stream->waitTimer--;
        }
        break;
    case PRINT_STREAM_PAUSED:
        if (stream->continueRequested) {
            switch (stream->pageMode) {
            case PRINT_STREAM_PAGE_SCROLL:
                stream->scrolling = TRUE;
                stream->state = PRINT_STREAM_RUNNING;
                break;
            case PRINT_STREAM_PAGE_CLEAR:
            default:
                GFL_BitmapFill(stream->bitmap, stream->backgroundColor);
                stream->state = PRINT_STREAM_RUNNING;
                break;
            }
        }
        break;
    }
}

// Carries out a stream command: page breaks, waits, speeds and the callback's events
static void func_02022694(PrintStream *stream) {
    BOOL pageBreak = FALSE;

    switch (StrCmd_Index(stream->cursor)) {
    case 0:
        pageBreak = TRUE;
        stream->state = PRINT_STREAM_PAUSED;
        stream->pageMode = PRINT_STREAM_PAGE_SCROLL;
        stream->scrollOffset = 0;
        stream->continueRequested = FALSE;
        stream->renderer.x = stream->renderer.startX;
        stream->renderer.y = 0;
        break;
    case 1:
        pageBreak = TRUE;
        stream->state = PRINT_STREAM_PAUSED;
        stream->pageMode = PRINT_STREAM_PAGE_CLEAR;
        stream->scrollOffset = 0;
        stream->continueRequested = FALSE;
        stream->renderer.x = stream->renderer.startX;
        stream->renderer.y = 0;
        break;
    case 2:
        stream->waitTimer = StrCmd_Param(stream->cursor, 0);
        break;
    case 3:
        stream->wait = StrCmd_Param(stream->cursor, 0);
        break;
    case 4:
        stream->wait = stream->baseWait;
        break;
    case 9: {
        u8 steps[2];

        switch ((u8)StrCmd_Param(stream->cursor, 0)) {
        case 0:
        default:
            steps[0] = stream->baseWait;
            steps[1] = stream->baseCharsPerStep;
            break;
        case 1:
            func_02022378(func_02017bf0(), steps);
            break;
        case 2:
            func_02022378(func_02017c20(), steps);
            break;
        }
        stream->wait = steps[0];
        stream->charsPerStep = steps[1];
        break;
    }
    case 5:
        stream->event = StrCmd_Param(stream->cursor, 0);
        break;
    case 6:
        stream->nextEvent = StrCmd_Param(stream->cursor, 0);
        stream->event = StrCmd_Param(stream->cursor, 0);
        break;
    case 7:
        stream->nextEvent = stream->defaultEvent;
        stream->event = stream->defaultEvent;
        break;
    case 8:
        GFL_BitmapFill(stream->bitmap, stream->backgroundColor);
        break;
    }
    stream->cursor = StrCmd_Skip(stream->cursor);
    if (pageBreak && *stream->cursor == CHAR_NEWLINE) {
        stream->cursor++;
    }
}

// The width of the line str starts, and where the next line starts
u32 GFL_FontGetLineWidth(const u16 *str, Font *font, u32 spacing, const u16 **end) {
    u32 width = 0;

    while (*str != CHAR_EOM && *str != CHAR_NEWLINE) {
        if (*str != STRCMD_CHAR) {
            width += GFL_FontGetCharWidth(font, *str) + spacing;
            str++;
        } else {
            str = StrCmd_Skip(str);
        }
    }
    if (end != NULL) {
        if (*str == CHAR_NEWLINE) {
            str++;
        }
        *end = str;
    }
    return width;
}

u32 func_0202284c(const StrBuf *strbuf) {
    const u16 *str = GFL_StrBufGetStringPtr(strbuf);
    u32 lines = 1;

    while (*str != CHAR_EOM) {
        if (*str != STRCMD_CHAR) {
            if (*str == CHAR_NEWLINE) {
                lines++;
            }
            str++;
        } else {
            str = StrCmd_Skip(str);
        }
    }
    return lines;
}

u32 GFL_FontGetBlockWidth(const StrBuf *strbuf, Font *font, u32 spacing) {
    u32 width = 0;
    const u16 *str = GFL_StrBufGetStringPtr(strbuf);

    while (*str != CHAR_EOM) {
        u32 lineWidth = GFL_FontGetLineWidth(str, font, spacing, &str);

        if (lineWidth > width) {
            width = lineWidth;
        }
    }
    return width;
}

// The width of each line, up to maxLines. Returns the count of lines
u32 func_020228c0(const StrBuf *strbuf, Font *font, u32 spacing, u32 *widths, u32 maxLines) {
    const u16 *str = GFL_StrBufGetStringPtr(strbuf);
    u32 i;

    for (i = 0; i < maxLines; i++) {
        if (*str == CHAR_EOM) {
            break;
        }
        widths[i] = GFL_FontGetLineWidth(str, font, spacing, &str);
    }
    return i;
}

// Copies a line of the string, with its commands. Returns whether it has the line
BOOL func_02022900(const StrBuf *strbuf, StrBuf *dest, u32 line) {
    const u16 *str = GFL_StrBufGetStringPtr(strbuf);

    GFL_StrBufClear(dest);
    while (line != 0) {
        while (*str != CHAR_EOM && *str != CHAR_NEWLINE) {
            if (*str != STRCMD_CHAR) {
                str++;
            } else {
                str = StrCmd_Skip(str);
            }
        }
        line--;
        if (*str == CHAR_EOM) {
            break;
        }
        str++;
    }
    if (line == 0 && *str != CHAR_EOM) {
        while (*str != CHAR_EOM && *str != CHAR_NEWLINE) {
            if (*str != STRCMD_CHAR) {
                GFL_StrBufAppend(dest, *str++);
            } else {
                const u16 *end = StrCmd_Skip(str);

                while (str < end) {
                    GFL_StrBufAppend(dest, *str++);
                }
            }
        }
        GFL_StrBufAppend(dest, CHAR_EOM);
        return TRUE;
    }
    return FALSE;
}

u32 GFL_FontGetBlockHeight(const StrBuf *strbuf, Font *font) {
    const u16 *str = GFL_StrBufGetStringPtr(strbuf);
    u16 lines = 0;

    while (*str != CHAR_EOM) {
        if (*str == CHAR_NEWLINE) {
            lines++;
        }
        if (*str == STRCMD_CHAR) {
            str = StrCmd_Skip(str);
        } else {
            str++;
        }
    }
    return GFL_FontGetCharHeight(font) * (lines + 1);
}

u16 GFL_StrCmdGetWordSetCommandCount(const StrBuf *strbuf) {
    const u16 *str;
    u16 count = 0;

    str = GFL_StrBufGetStringPtr(strbuf);

    while (*str != CHAR_EOM) {
        if (*str == STRCMD_CHAR) {
            if (GFL_StrCmdIsWordSet(str)) {
                count++;
            }
            str = GFL_StrCmdSkipCommand(str);
        } else {
            str++;
        }
    }
    return count;
}

u8 GFL_StrCmdCountLinesUntilWordSetIndex(const StrBuf *strbuf, u8 index) {
    const u16 *str;
    u16 lines = 0;

    GFL_StrBufGetStringPtr(strbuf);
    str = GFL_StrBufGetStringPtr(strbuf);
    while (*str != CHAR_EOM) {
        if (*str == CHAR_NEWLINE) {
            lines++;
        }
        if (*str == STRCMD_CHAR) {
            if (GFL_StrCmdIsWordSet(str) && index == StrCmd_Param(str, 0)) {
                return lines;
            }
            str = GFL_StrCmdSkipCommand(str);
        } else {
            str++;
        }
    }
    return 0;
}

u8 GFL_StrCmdGetStrWidthUntilWordSetIndex(const StrBuf *strbuf, u8 index, Font *font, u32 spacing) {
    u32 width = 0;
    const u16 *str = GFL_StrBufGetStringPtr(strbuf);

    while (*str != CHAR_EOM) {
        if (*str == CHAR_NEWLINE) {
            width = 0;
            str++;
        } else if (*str != STRCMD_CHAR) {
            width += GFL_FontGetCharWidth(font, *str) + spacing;
            str++;
        } else {
            if (GFL_StrCmdIsWordSet(str) && index == StrCmd_Param(str, 0)) {
                return width;
            }
            str = StrCmd_Skip(str);
        }
    }
    return 0;
}

u16 GFL_StrCmdGetIdentChar(void) {
    return STRCMD_CHAR;
}

BOOL GFL_StrCmdIsWordSet(const u16 *cmd) {
    switch (StrCmd_Category(cmd)) {
    case 1:
    case 2:
        return TRUE;
    }
    return FALSE;
}

u32 GFL_StrCmdGetCommandCategory(const u16 *cmd) {
    if (*cmd == STRCMD_CHAR) {
        return (u8)(cmd[1] >> 8);
    }
    return 0xff;
}

void GFL_StrCmdBuild(StrBuf *strbuf, u32 category, u16 index, u8 paramCount, const u16 *params) {
    u8 i;

    GFL_StrBufAppend(strbuf, STRCMD_CHAR);
    GFL_StrBufAppend(strbuf, ((category & 0xff) << 8) | index);
    GFL_StrBufAppend(strbuf, paramCount);
    for (i = 0; i < paramCount; i++) {
        GFL_StrBufAppend(strbuf, params[i]);
    }
}

u32 GFL_StrCmdGetCommandIndex(const u16 *cmd) {
    if (*cmd == STRCMD_CHAR) {
        return (u8)cmd[1];
    }
    return 0xff;
}

u16 GFL_WordSetGetCommandParameter(const u16 *cmd, u32 index) {
    if (*cmd == STRCMD_CHAR) {
        return (cmd + index)[3];
    }
    return 0;
}

const u16 *GFL_StrCmdSkipCommand(const u16 *cmd) {
    if (*cmd == STRCMD_CHAR) {
        cmd = cmd + 3 + cmd[2];
    }
    return cmd;
}

// The size a string takes in the queue, in 4-byte steps, and with its renderer and link
static u32 func_02022c08(const StrBuf *strbuf) {
    u32 size = (GFL_StrBufGetCharCount(strbuf) + 1) * 2;

    while (size & 3) {
        size++;
    }
    return size;
}

static u32 func_02022c24(const StrBuf *strbuf) {
    return func_02022c08(strbuf) + sizeof(TextRenderer) + sizeof(TextRenderer *);
}

// The free space of the queue
static u32 func_02022c30(PrintQueue *queue) {
    if (queue->writePos < queue->readPos) {
        return queue->readPos - queue->writePos;
    }
    return queue->size - queue->writePos + queue->readPos;
}

// Adds the string and a copy of the renderer, and links them after the last one
static void func_02022c44(PrintQueue *queue, TextRenderer *renderer, const StrBuf *strbuf) {
    const u16 *str = GFL_StrBufGetStringPtr(strbuf);
    const u16 *queued = func_02022cd4(queue, str, func_02022c08(strbuf), sizeof(u16));
    TextRenderer *job = func_02022cd4(queue, renderer, sizeof(TextRenderer), sizeof(TextRenderer));
    TextRenderer *next = NULL;

    func_02022cd4(queue, &next, sizeof(next), sizeof(next));
    if (queue->job != NULL) {
        TextRenderer *last = queue->job;
        TextRenderer *following;
        u16 offset;

        for (;;) {
            following = func_02022d30(queue, last);
            if (following == NULL) {
                break;
            }
            last = following;
        }
        offset = (u8 *)last - queue->buffer;
        offset += sizeof(TextRenderer);
        if (offset >= queue->size) {
            offset = 0;
        }
        MI_CpuCopy8(&job, queue->buffer + offset, sizeof(job));
    } else {
        queue->job = job;
        queue->cursor = queued;
    }
}

// Writes to the ring in pieces of chunk bytes, which do not wrap. Returns where it started
static void *func_02022cd4(PrintQueue *queue, const void *src, u16 size, u16 chunk) {
    u16 start;
    u8 *buffer;
    u16 written;
    u16 pos;

    written = 0;
    buffer = queue->buffer;
    start = queue->writePos;
    while (written < size) {
        pos = queue->writePos;
        MI_CpuCopy8(src, buffer + pos, chunk);
        pos += chunk;
        if (pos >= queue->size) {
            pos = 0;
        }
        queue->writePos = pos;
        src = (const u8 *)src + chunk;
        written += chunk;
    }
    return buffer + start;
}

static TextRenderer *func_02022d30(PrintQueue *queue, TextRenderer *job) {
    u32 offset = (u16)((u8 *)job - queue->buffer);

    offset = PrintQueue_Advance(queue, offset, sizeof(TextRenderer));
    return *(TextRenderer **)(queue->buffer + (u16)offset);
}
