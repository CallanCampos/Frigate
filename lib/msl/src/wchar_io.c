// MSL's wchar_io.c: fwide, which fixes whether a stream is used as bytes or as wide characters. fread and the other
// stream functions set a stream to bytes the first time they touch it.
#include "ansi_files.h"
#include <wchar.h>

int fwide(FILE *file, int mode) {
    if (file == NULL || file->mode.file_kind == __closed_file) {
        return 0;
    }

    switch (file->mode.file_orientation) {
    case __unoriented:
        // A stream without an orientation takes the one asked for (none for 0), and answers with the request
        if (mode > 0) {
            file->mode.file_orientation = __wide_oriented;
        } else if (mode < 0) {
            file->mode.file_orientation = __char_oriented;
        }
        break;
    case __char_oriented:
        mode = -1;
        break;
    case __wide_oriented:
        mode = 1;
        break;
    }
    return mode;
}
