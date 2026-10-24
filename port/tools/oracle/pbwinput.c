// Input files ("pbwinput 1"), read and written: what the player holds on each frame, for the melonDS oracle and for
// Frigate's replays (port/app/replay.c), which compile this file as it is. The format is in
// port/tools/oracle/README.md and the interface in pbwinput.h.
#include "pbwinput.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// The most tokens a line can have: a frame, the buttons and the touch, and one more to report a line that has too many
#define MAX_TOKENS 4
// The room for a token and its NUL; a longer token is an error
#define MAX_TOKEN 64

typedef struct ButtonName {
    const char *name;
    uint32_t bit;
} ButtonName;

// The names in the order a written line lists them; LID stands for the lid, closed
#define LID_BIT 0x80000000u
static const ButtonName sButtonNames[] = {
    { "A", PBW_INPUT_A },         { "B", PBW_INPUT_B },         { "SELECT", PBW_INPUT_SELECT },
    { "START", PBW_INPUT_START }, { "RIGHT", PBW_INPUT_RIGHT }, { "LEFT", PBW_INPUT_LEFT },
    { "UP", PBW_INPUT_UP },       { "DOWN", PBW_INPUT_DOWN },   { "R", PBW_INPUT_R },
    { "L", PBW_INPUT_L },         { "X", PBW_INPUT_X },         { "Y", PBW_INPUT_Y },
    { "DEBUG", PBW_INPUT_DEBUG }, { "LID", LID_BIT },
};
#define BUTTON_NAME_COUNT (sizeof(sButtonNames) / sizeof(sButtonNames[0]))

static void SetError(char *error, size_t errorSize, unsigned line, const char *format, ...) {
    va_list args;
    int used = 0;

    if (error == NULL || errorSize == 0) {
        return;
    }
    if (line != 0) {
        used = snprintf(error, errorSize, "line %u: ", line);
        if (used < 0 || (size_t)used >= errorSize) {
            return;
        }
    }
    va_start(args, format);
    vsnprintf(error + used, errorSize - (size_t)used, format, args);
    va_end(args);
}

void pbw_input_init(PbwInput *input) {
    memset(input, 0, sizeof(*input));
}

void pbw_input_free(PbwInput *input) {
    free(input->changes);
    pbw_input_init(input);
}

static int SameState(const PbwInputState *a, const PbwInputState *b) {
    return a->buttons == b->buttons && (a->lidClosed != 0) == (b->lidClosed != 0) &&
           (a->touching != 0) == (b->touching != 0) &&
           (a->touching == 0 || (a->touchX == b->touchX && a->touchY == b->touchY));
}

static int Push(PbwInput *input, uint32_t frame, const PbwInputState *state) {
    if (input->count == input->capacity) {
        size_t capacity = input->capacity != 0 ? input->capacity * 2 : 64;
        PbwInputChange *changes = realloc(input->changes, capacity * sizeof(*changes));

        if (changes == NULL) {
            return 0;
        }
        input->changes = changes;
        input->capacity = capacity;
    }
    input->changes[input->count].frame = frame;
    input->changes[input->count].state = *state;
    input->count++;
    return 1;
}

// A decimal number of up to maxValue, with nothing after it
static int ParseNumber(const char *text, uint32_t maxValue, uint32_t *out) {
    uint64_t value = 0;

    if (*text == '\0') {
        return 0;
    }
    for (; *text != '\0'; text++) {
        if (*text < '0' || *text > '9') {
            return 0;
        }
        value = value * 10 + (uint64_t)(*text - '0');
        if (value > maxValue) {
            return 0;
        }
    }
    *out = (uint32_t)value;
    return 1;
}

static int EqualsIgnoringCase(const char *a, const char *b, size_t length) {
    size_t i;

    if (strlen(b) != length) {
        return 0;
    }
    for (i = 0; i < length; i++) {
        char c = a[i];
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
        }
        if (c != b[i]) {
            return 0;
        }
    }
    return 1;
}

static int ParseButtons(const char *text, PbwInputState *state, char *error, size_t errorSize, unsigned line) {
    uint32_t bits = 0;

    if (strcmp(text, "-") == 0) {
        return 1;
    }
    while (*text != '\0') {
        const char *end = strchr(text, '+');
        size_t length = end != NULL ? (size_t)(end - text) : strlen(text);
        size_t i;

        for (i = 0; i < BUTTON_NAME_COUNT; i++) {
            if (EqualsIgnoringCase(text, sButtonNames[i].name, length)) {
                break;
            }
        }
        if (i == BUTTON_NAME_COUNT) {
            SetError(error, errorSize, line, "unknown button \"%.*s\"", (int)length, text);
            return 0;
        }
        if (bits & sButtonNames[i].bit) {
            SetError(error, errorSize, line, "button %s named twice", sButtonNames[i].name);
            return 0;
        }
        bits |= sButtonNames[i].bit;
        if (end == NULL) {
            break;
        }
        text = end + 1;
        if (*text == '\0') {
            SetError(error, errorSize, line, "a button list ends with +");
            return 0;
        }
    }
    if (bits == 0) {
        SetError(error, errorSize, line, "an empty button list (write - for none)");
        return 0;
    }
    state->buttons = bits & ~LID_BIT;
    state->lidClosed = (bits & LID_BIT) != 0;
    return 1;
}

static int ParseTouch(const char *text, PbwInputState *state, char *error, size_t errorSize, unsigned line) {
    char x[MAX_TOKEN];
    const char *comma;
    uint32_t touchX;
    uint32_t touchY;

    if (strcmp(text, "-") == 0) {
        return 1;
    }
    comma = strchr(text, ',');
    if (comma == NULL || (size_t)(comma - text) >= sizeof(x)) {
        SetError(error, errorSize, line, "a touch is X,Y or -, not \"%s\"", text);
        return 0;
    }
    memcpy(x, text, (size_t)(comma - text));
    x[comma - text] = '\0';
    if (!ParseNumber(x, PBW_INPUT_TOUCH_WIDTH - 1, &touchX) ||
        !ParseNumber(comma + 1, PBW_INPUT_TOUCH_HEIGHT - 1, &touchY)) {
        SetError(error, errorSize, line, "a touch is X,Y with X from 0 to %d and Y from 0 to %d, not \"%s\"",
                 PBW_INPUT_TOUCH_WIDTH - 1, PBW_INPUT_TOUCH_HEIGHT - 1, text);
        return 0;
    }
    state->touching = 1;
    state->touchX = (uint8_t)touchX;
    state->touchY = (uint8_t)touchY;
    return 1;
}

static int IsLeapYear(unsigned year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

int pbw_input_valid_time(const char *text) {
    static const unsigned char daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    // Digits where the pattern has 9, the separators elsewhere
    static const char pattern[] = "9999-99-99 99:99:99";
    unsigned year;
    unsigned month;
    unsigned day;
    unsigned hour;
    unsigned minute;
    unsigned second;
    unsigned days;
    size_t i;

    if (strlen(text) != sizeof(pattern) - 1) {
        return 0;
    }
    for (i = 0; i < sizeof(pattern) - 1; i++) {
        if (pattern[i] == '9' ? text[i] < '0' || text[i] > '9' : text[i] != pattern[i]) {
            return 0;
        }
    }
#define DIGITS2(at) ((unsigned)(text[at] - '0') * 10 + (unsigned)(text[(at) + 1] - '0'))
    year = DIGITS2(0) * 100 + DIGITS2(2);
    month = DIGITS2(5);
    day = DIGITS2(8);
    hour = DIGITS2(11);
    minute = DIGITS2(14);
    second = DIGITS2(17);
#undef DIGITS2
    if (year < 2000 || year > 2099 || month < 1 || month > 12 || hour > 23 || minute > 59 || second > 59) {
        return 0;
    }
    days = daysInMonth[month - 1] + (month == 2 && IsLeapYear(year));
    return day >= 1 && day <= days;
}

// Splits a line (comments already cut) into tokens; returns how many, up to MAX_TOKENS, or -1 for a token too long
static int Tokenize(const char *line, size_t length, char tokens[MAX_TOKENS][MAX_TOKEN]) {
    int count = 0;
    size_t i = 0;

    while (i < length && count < MAX_TOKENS) {
        size_t start;
        size_t size;

        while (i < length && (line[i] == ' ' || line[i] == '\t')) {
            i++;
        }
        if (i == length) {
            break;
        }
        start = i;
        while (i < length && line[i] != ' ' && line[i] != '\t') {
            i++;
        }
        size = i - start;
        if (size >= MAX_TOKEN) {
            return -1;
        }
        memcpy(tokens[count], line + start, size);
        tokens[count][size] = '\0';
        count++;
    }
    return count;
}

int pbw_input_parse(PbwInput *input, const char *text, size_t length, char *error, size_t errorSize) {
    char tokens[MAX_TOKENS][MAX_TOKEN];
    size_t at = 0;
    unsigned line = 0;
    int sawHeader = 0;
    int sawFrames = 0;
    int sawClock = 0;
    uint32_t lastFrame = 0;

    pbw_input_init(input);
    while (at < length) {
        const char *start = text + at;
        const char *newline = memchr(start, '\n', length - at);
        size_t lineLength = newline != NULL ? (size_t)(newline - start) : length - at;
        const char *comment = memchr(start, '#', lineLength);
        int count;

        at += lineLength + (newline != NULL);
        line++;
        if (comment != NULL) {
            lineLength = (size_t)(comment - start);
        }
        if (lineLength > 0 && start[lineLength - 1] == '\r') {
            lineLength--;
        }
        if (memchr(start, '\0', lineLength) != NULL) {
            SetError(error, errorSize, line, "a NUL byte");
            goto fail;
        }
        count = Tokenize(start, lineLength, tokens);
        if (count < 0) {
            SetError(error, errorSize, line, "a word longer than %d characters", MAX_TOKEN - 1);
            goto fail;
        }
        if (count == 0) {
            continue;
        }

        if (!sawHeader) {
            if (count != 2 || strcmp(tokens[0], "pbwinput") != 0) {
                SetError(error, errorSize, line, "the file must start with \"pbwinput 1\"");
                goto fail;
            }
            if (strcmp(tokens[1], "1") != 0) {
                SetError(error, errorSize, line, "format version %s; this reader knows version 1", tokens[1]);
                goto fail;
            }
            sawHeader = 1;
        } else if (strcmp(tokens[0], "time") == 0) {
            char time[MAX_TOKEN * 2];

            if (input->count != 0 || input->time[0] != '\0') {
                SetError(error, errorSize, line, "one \"time\" line at most, before the first frame");
                goto fail;
            }
            if (count != 3) {
                SetError(error, errorSize, line, "\"time\" takes YYYY-MM-DD HH:MM:SS");
                goto fail;
            }
            snprintf(time, sizeof(time), "%s %s", tokens[1], tokens[2]);
            if (!pbw_input_valid_time(time)) {
                SetError(error, errorSize, line, "\"%s\" isn't a time YYYY-MM-DD HH:MM:SS from 2000 to 2099", time);
                goto fail;
            }
            memcpy(input->time, time, sizeof(input->time));
        } else if (strcmp(tokens[0], "version") == 0) {
            if (input->count != 0 || input->version[0] != '\0') {
                SetError(error, errorSize, line, "one \"version\" line at most, before the first frame");
                goto fail;
            }
            if (count != 2 || (strcmp(tokens[1], "b2_us") != 0 && strcmp(tokens[1], "w2_us") != 0)) {
                SetError(error, errorSize, line, "\"version\" takes b2_us or w2_us");
                goto fail;
            }
            memcpy(input->version, tokens[1], strlen(tokens[1]) + 1);
        } else if (strcmp(tokens[0], "frames") == 0) {
            if (input->count != 0 || sawFrames) {
                SetError(error, errorSize, line, "one \"frames\" line at most, before the first frame");
                goto fail;
            }
            if (count != 2 || (strcmp(tokens[1], "game") != 0 && strcmp(tokens[1], "video") != 0)) {
                SetError(error, errorSize, line, "\"frames\" takes game or video");
                goto fail;
            }
            input->frames = strcmp(tokens[1], "video") == 0 ? PBW_INPUT_VIDEO_FRAMES : PBW_INPUT_GAME_FRAMES;
            sawFrames = 1;
        } else if (strcmp(tokens[0], "clock") == 0) {
            if (input->count != 0 || sawClock) {
                SetError(error, errorSize, line, "one \"clock\" line at most, before the first frame");
                goto fail;
            }
            if (count != 2 || (strcmp(tokens[1], "running") != 0 && strcmp(tokens[1], "pinned") != 0)) {
                SetError(error, errorSize, line, "\"clock\" takes running or pinned");
                goto fail;
            }
            input->clock = strcmp(tokens[1], "pinned") == 0 ? PBW_INPUT_CLOCK_PINNED : PBW_INPUT_CLOCK_RUNNING;
            sawClock = 1;
        } else {
            PbwInputState state = { 0 };
            uint32_t frame;

            if (!ParseNumber(tokens[0], PBW_INPUT_MAX_FRAME, &frame) || frame == 0) {
                SetError(error, errorSize, line, "\"%s\" is neither a directive nor a frame from 1 to %u", tokens[0],
                         PBW_INPUT_MAX_FRAME);
                goto fail;
            }
            if (frame <= lastFrame) {
                SetError(error, errorSize, line, "frame %u comes after frame %u", frame, lastFrame);
                goto fail;
            }
            if (count < 2 || count > 3) {
                SetError(error, errorSize, line, "a frame line is FRAME BUTTONS [TOUCH]");
                goto fail;
            }
            if (!ParseButtons(tokens[1], &state, error, errorSize, line) ||
                (count == 3 && !ParseTouch(tokens[2], &state, error, errorSize, line))) {
                goto fail;
            }
            if (!Push(input, frame, &state)) {
                SetError(error, errorSize, line, "out of memory");
                goto fail;
            }
            lastFrame = frame;
        }
    }
    if (!sawHeader) {
        SetError(error, errorSize, 0, "an empty file: it must start with \"pbwinput 1\"");
        goto fail;
    }
    return 1;

fail:
    pbw_input_free(input);
    return 0;
}

int pbw_input_load(PbwInput *input, const char *path, char *error, size_t errorSize) {
    FILE *file = fopen(path, "rb");
    char *text = NULL;
    size_t length = 0;
    size_t capacity = 0;
    int ok;

    pbw_input_init(input);
    if (file == NULL) {
        SetError(error, errorSize, 0, "can't open %s", path);
        return 0;
    }
    for (;;) {
        size_t got;

        if (length == capacity) {
            size_t grown = capacity != 0 ? capacity * 2 : 4096;
            char *bigger = realloc(text, grown);

            if (bigger == NULL) {
                free(text);
                fclose(file);
                SetError(error, errorSize, 0, "out of memory reading %s", path);
                return 0;
            }
            text = bigger;
            capacity = grown;
        }
        got = fread(text + length, 1, capacity - length, file);
        length += got;
        if (got == 0) {
            break;
        }
    }
    if (ferror(file)) {
        free(text);
        fclose(file);
        SetError(error, errorSize, 0, "can't read %s", path);
        return 0;
    }
    fclose(file);
    ok = pbw_input_parse(input, text, length, error, errorSize);
    free(text);
    return ok;
}

PbwInputState pbw_input_state(const PbwInput *input, uint32_t frame) {
    PbwInputState none = { 0 };
    size_t low = 0;
    size_t high = input->count;

    // The first change after frame; the one before it holds
    while (low < high) {
        size_t middle = low + (high - low) / 2;

        if (input->changes[middle].frame <= frame) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return low == 0 ? none : input->changes[low - 1].state;
}

void pbw_game_clock_init(PbwGameClock *clock) {
    clock->frames = 0;
    clock->lastCount = 0;
}

uint32_t pbw_input_frame_at_vblank(const PbwInput *input, PbwGameClock *clock, uint32_t videoFrame, uint32_t count) {
    // A step of one is a pass of the main loop. Anything else is the count's memory before the game clears it (the
    // ARM9's image there at power-on), its reset to 0 (power-on, a soft reset), or no pass since the last VBlank (a
    // pass longer than a frame): no game frame went by
    if (count != 0 && count == clock->lastCount + 1) {
        clock->frames++;
    }
    clock->lastCount = count;
    return input->frames == PBW_INPUT_VIDEO_FRAMES ? videoFrame : clock->frames + 1;
}

uint32_t pbw_input_frame_asleep(const PbwInput *input, PbwGameClock *clock, uint32_t videoFrame) {
    clock->frames++;
    return input->frames == PBW_INPUT_VIDEO_FRAMES ? videoFrame : clock->frames + 1;
}

int pbw_input_append(PbwInput *input, uint32_t frame, const PbwInputState *state) {
    PbwInputState clean = *state;
    PbwInputState current;

    if (frame == 0 || frame > PBW_INPUT_MAX_FRAME ||
        (input->count != 0 && frame <= input->changes[input->count - 1].frame)) {
        return 0;
    }
    clean.buttons &= PBW_INPUT_ALL;
    clean.lidClosed = clean.lidClosed != 0;
    clean.touching = clean.touching != 0;
    if (!clean.touching) {
        clean.touchX = 0;
        clean.touchY = 0;
    } else if (clean.touchY >= PBW_INPUT_TOUCH_HEIGHT) {
        return 0;
    }
    current = pbw_input_state(input, frame);
    if (SameState(&current, &clean)) {
        return 1;
    }
    return Push(input, frame, &clean);
}

int pbw_input_format_state(const PbwInputState *state, char *out, size_t size) {
    size_t used = 0;
    size_t i;
    int written;

    if (size == 0) {
        return 0;
    }
    out[0] = '\0';
    for (i = 0; i < BUTTON_NAME_COUNT; i++) {
        int held = sButtonNames[i].bit == LID_BIT ? state->lidClosed != 0 : (state->buttons & sButtonNames[i].bit) != 0;

        if (held) {
            written = snprintf(out + used, size - used, "%s%s", used != 0 ? "+" : "", sButtonNames[i].name);
            if (written < 0 || (size_t)written >= size - used) {
                return 0;
            }
            used += (size_t)written;
        }
    }
    if (used == 0) {
        written = snprintf(out, size, "-");
        if (written < 0 || (size_t)written >= size) {
            return 0;
        }
        used = (size_t)written;
    }
    if (state->touching) {
        written = snprintf(out + used, size - used, " %u,%u", (unsigned)state->touchX, (unsigned)state->touchY);
    } else {
        written = snprintf(out + used, size - used, " -");
    }
    return written >= 0 && (size_t)written < size - used;
}

int pbw_input_write(const PbwInput *input, FILE *file) {
    size_t i;

    if (fprintf(file, "pbwinput 1\n") < 0) {
        return 0;
    }
    if (input->time[0] != '\0' && fprintf(file, "time %s\n", input->time) < 0) {
        return 0;
    }
    if (input->version[0] != '\0' && fprintf(file, "version %s\n", input->version) < 0) {
        return 0;
    }
    if (fprintf(file, "frames %s\n", input->frames == PBW_INPUT_VIDEO_FRAMES ? "video" : "game") < 0) {
        return 0;
    }
    // A running clock is the default, which a file written before the line existed has too
    if (input->clock == PBW_INPUT_CLOCK_PINNED && fprintf(file, "clock pinned\n") < 0) {
        return 0;
    }
    for (i = 0; i < input->count; i++) {
        char columns[96];

        if (!pbw_input_format_state(&input->changes[i].state, columns, sizeof(columns)) ||
            fprintf(file, "%u %s\n", input->changes[i].frame, columns) < 0) {
            return 0;
        }
    }
    return fflush(file) == 0;
}
