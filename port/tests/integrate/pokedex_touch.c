// The actual Pokédex habitat tables (src/ov303/pokedex_habitat.c) through GFL's genuine touch search
// (src/gfl/touchpanel.c). Every screen pixel checks bounds, inclusive edges and original first-hit precedence.
#include "src/ov303/pokedex_habitat.c"
#include <stdio.h>
#include <stdlib.h>

static void CheckTable(const TouchRect *table, unsigned count, unsigned capacity) {
    unsigned misses = 0;
    for (unsigned y = 0; y < 192; ++y) {
        for (unsigned x = 0; x < 256; ++x) {
            int expected = TOUCH_RECT_NONE;
            for (unsigned i = 0; i < count; ++i) {
                const TouchRect *r = &table[i];
                if (x >= r->left && x <= r->right && y >= r->top && y <= r->bottom) {
                    expected = (int)i;
                    break;
                }
            }
            int actual = GFL_HIDGetTouchRectAt(table, x, y);
            if (actual != expected || actual < TOUCH_RECT_NONE || actual >= (int)count) {
                fprintf(stderr, "pokedex touch: (%u,%u) expected %d, got %d\n", x, y, expected, actual);
                exit(1);
            }
            misses += expected == TOUCH_RECT_NONE;
        }
    }
    if (capacity != count + 1 || table[count].top != TOUCH_RECT_END) {
        fprintf(stderr, "pokedex touch: missing physical terminator\n");
        exit(1);
    }
    if (misses == 0) {
        fprintf(stderr, "pokedex touch: fixture never checked an unhit point\n");
        exit(1);
    }
    printf("pokedex touch: %u rectangles, 49152 pixels, %u unhit PASS\n", count, misses);
}
int main(void) {
    CheckTable(sPokedexHabitatCursorTouchRects, CURSOR_TOUCH_COUNT,
               sizeof(sPokedexHabitatCursorTouchRects) / sizeof(TouchRect));
    CheckTable(sPokedexHabitatListTouchRects, LIST_TOUCH_COUNT,
               sizeof(sPokedexHabitatListTouchRects) / sizeof(TouchRect));
    return 0;
}
