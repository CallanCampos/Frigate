// --record's bounded, newline-committed stream for field/button/lid/touch play and battle seeds. app/main.c supplies
// effective RTC/seeds and VBlank input; tools/oracle/pbwinput.c owns serialization and recovery semantics. The output
// is exclusively created, so rollback can truncate only this recording, never a player's existing replay or save.
#include "app/record.h"
#include "hal/input/port_input.h"
#include "hal/tp/tp_host.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <bcrypt.h>
#include <direct.h>
#include <io.h>
#include <windows.h>
#define PATH_SIZE 32768
#define PATH_SEP '\\'
#define PATH_COMPARE _stricmp
#else
#include <unistd.h>
#define PATH_SIZE 4096
#define PATH_SEP '/'
#define PATH_COMPARE strcmp
#endif

static FILE *sFile;
static int sFailed;
static int sStarted;
static int64_t sCommitted;
static uint64_t sFrame;
static PbwInputState sState;
static char sPath[PATH_SIZE];
static int sExitRegistered;
static void ExitClose(void) {
    if (sFile != NULL && !sStarted) {
        fclose(sFile);
        sFile = NULL;
        remove(sPath);
    } else if (sFile != NULL) {
        port_record_close();
    }
}
static size_t Write(const void *data, size_t size, FILE *file) {
    return fwrite(data, 1, size, file);
}
static int Truncate(FILE *file, int64_t size) {
#ifdef _WIN32
    return _chsize_s(_fileno(file), size) == 0 && _fseeki64(file, size, SEEK_SET) == 0 ? 0 : -1;
#else
    return ftruncate(fileno(file), (off_t)size) == 0 && fseeko(file, (off_t)size, SEEK_SET) == 0 ? 0 : -1;
#endif
}
#ifdef PORT_RECORD_TEST
static PortRecordIO sIO = { Write, fflush, fclose, Truncate };
void port_record_test_io(const PortRecordIO *ops) {
    sIO = ops != NULL ? *ops : (PortRecordIO){ Write, fflush, fclose, Truncate };
}
#else
static const struct {
    size_t (*write)(const void *, size_t, FILE *);
    int (*flush)(FILE *);
    int (*close)(FILE *);
    int (*truncate)(FILE *, int64_t);
} sIO = { Write, fflush, fclose, Truncate };
#endif

// Resolve existing ancestors and future leaves. Output's parent must exist for exclusive creation; protected saves
// may not exist yet. POSIX realpath also resolves symlink parents; a dangling save symlink is resolved explicitly.
static int Canonical(const char *path, char *out, size_t size, unsigned depth) {
    char absolute[PATH_SIZE], suffix[PATH_SIZE] = "", parent[PATH_SIZE], resolved[PATH_SIZE];
    char *slash;
    if (path == NULL || *path == '\0' || depth > 32) {
        return 0;
    }
#ifdef _WIN32
    if (_fullpath(absolute, path, sizeof(absolute)) == NULL) {
        return 0;
    }
    for (char *p = absolute; *p; ++p) {
        if (*p == '/') {
            *p = '\\';
        }
    }
#else
    if (realpath(path, resolved) != NULL) {
        return snprintf(out, size, "%s", resolved) < (int)size;
    }
    {
        char link[PATH_SIZE];
        ssize_t n = readlink(path, link, sizeof(link) - 1);
        if (n >= 0) {
            link[n] = '\0';
            if (link[0] == '/') {
                return Canonical(link, out, size, depth + 1);
            }
            if (snprintf(parent, sizeof(parent), "%s", path) >= (int)sizeof(parent)) {
                return 0;
            }
            slash = strrchr(parent, '/');
            if (slash) {
                slash[1] = '\0';
            } else {
                strcpy(parent, "");
            }
            if (snprintf(absolute, sizeof(absolute), "%s%s", parent, link) >= (int)sizeof(absolute)) {
                return 0;
            }
            return Canonical(absolute, out, size, depth + 1);
        }
    }
    if (path[0] == '/') {
        if (snprintf(absolute, sizeof(absolute), "%s", path) >= (int)sizeof(absolute)) {
            return 0;
        }
    } else {
        if (getcwd(parent, sizeof(parent)) == NULL ||
            snprintf(absolute, sizeof(absolute), "%s/%s", parent, path) >= (int)sizeof(absolute)) {
            return 0;
        }
    }
#endif
    // Walk up to the nearest existing ancestor, then normalize only unresolved dot components below it.
    for (;;) {
#ifdef _WIN32
        HANDLE h = CreateFileA(absolute, 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
                               FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if (h != INVALID_HANDLE_VALUE) {
            DWORD n = GetFinalPathNameByHandleA(h, resolved, sizeof(resolved), FILE_NAME_NORMALIZED);
            CloseHandle(h);
            if (n == 0 || n >= sizeof(resolved)) {
                return 0;
            }
            if (strncmp(resolved, "\\\\?\\", 4) == 0) {
                memmove(resolved, resolved + 4, strlen(resolved + 4) + 1);
            }
            break;
        }
#else
        if (realpath(absolute, resolved) != NULL) {
            break;
        }
#endif
        slash = strrchr(absolute, PATH_SEP);
        if (slash == NULL || slash[1] == '\0') {
            return 0;
        }
        if (snprintf(parent, sizeof(parent), "%s%s%s", slash + 1, suffix[0] ? "/" : "", suffix) >=
            (int)sizeof(parent)) {
            return 0;
        }
        strcpy(suffix, parent);
        if (slash == absolute) {
            slash[1] = '\0';
        } else {
            *slash = '\0';
        }
    }
    for (char *part = strtok(suffix, "/"); part != NULL; part = strtok(NULL, "/")) {
        if (strcmp(part, ".") == 0) {
            continue;
        }
        if (strcmp(part, "..") == 0) {
            slash = strrchr(resolved, PATH_SEP);
            if (slash != NULL) {
                if (slash == resolved) {
                    slash[1] = '\0';
                } else {
                    *slash = '\0';
                }
            }
        } else {
            if (snprintf(parent, sizeof(parent), "%s%s%s", resolved,
                         resolved[strlen(resolved) - 1] == PATH_SEP ? "" : (PATH_SEP == '/' ? "/" : "\\"),
                         part) >= (int)sizeof(parent)) {
                return 0;
            }
            strcpy(resolved, parent);
        }
    }
    return snprintf(out, size, "%s", resolved) < (int)size;
}

int port_record_create(const char *path, const char *const *protectedPaths, size_t count, const char *runDir) {
    char output[PATH_SIZE], other[PATH_SIZE];
    if (sFile != NULL || !Canonical(path, output, sizeof(output), 0)) {
        goto bad;
    }
    for (size_t i = 0; i < count; ++i) {
        size_t n;
        if (protectedPaths[i] == NULL) {
            continue;
        }
        if (!Canonical(protectedPaths[i], other, sizeof(other), 0)) {
            goto bad;
        }
        if (PATH_COMPARE(output, other) == 0) {
            goto bad;
        }
        n = strlen(other);
        if (n + 4 < sizeof(other)) {
            memcpy(other + n, ".tmp", 5);
            if (PATH_COMPARE(output, other) == 0) {
                goto bad;
            }
        }
    }
    if (runDir != NULL) {
        size_t n;
        if (!Canonical(runDir, other, sizeof(other), 0)) {
            goto bad;
        }
        n = strlen(other);
        if (PATH_COMPARE(output, other) == 0) {
            goto bad;
        }
        // Compare directory prefix with platform's case rules, without rejecting a sibling such as run-other.
        if (strlen(output) > n && (other[n - 1] == PATH_SEP || output[n] == '/' || output[n] == '\\')) {
            char saved = output[n];
            output[n] = '\0';
            int inside = PATH_COMPARE(output, other) == 0;
            output[n] = saved;
            if (inside) {
                goto bad;
            }
        }
    }
    sFile = fopen(path, "wbx");
    if (sFile == NULL) {
        goto bad;
    }
    if (setvbuf(sFile, NULL, _IONBF, 0) != 0) {
        fclose(sFile);
        sFile = NULL;
        remove(path);
        goto bad;
    }
    snprintf(sPath, sizeof(sPath), "%s", path);
    if (!sExitRegistered) {
        if (atexit(ExitClose) != 0) {
            fclose(sFile);
            sFile = NULL;
            remove(path);
            goto bad;
        }
        sExitRegistered = 1;
    }
    sFailed = 0;
    sStarted = 0;
    sCommitted = 0;
    sFrame = 0;
    return 1;
bad:
    fprintf(stderr,
            "port: --record: cannot exclusively create a safe output %s (existing file, alias, or inaccessible path)\n",
            path ? path : "");
    return 0;
}

static int Fail(void) {
    sFailed = 1;
    fprintf(stderr, "port: --record: write/flush failed; preserving committed prefix\n");
    if (sFile != NULL) {
        clearerr(sFile);
        if (sIO.truncate(sFile, sCommitted) != 0) {
            fprintf(stderr, "port: --record: prefix rollback failed\n");
        }
    }
    return 0;
}
static int Commit(void) {
    int64_t position;
    if (sIO.flush(sFile) != 0) {
        return Fail();
    }
#ifdef _WIN32
    position = _ftelli64(sFile);
#else
    position = ftello(sFile);
#endif
    if (position < 0) {
        return Fail();
    }
    sCommitted = position;
    return 1;
}
int port_record_start(const PbwInput *header) {
    if (sFile == NULL || sStarted || !header->hasInitial || header->seedCount == 0 ||
        header->seedCount > PBW_INPUT_MAX_SEEDS || header->frames != PBW_INPUT_VIDEO_FRAMES) {
        return Fail();
    }
    // Header writes use the shared serializer. No game runs until all its complete directives have been flushed.
    if (!pbw_input_write_header(header, sFile) || !Commit()) {
        port_record_close();
        remove(sPath);
        return 0;
    }
    sState = header->initial;
    sStarted = 1;
    return 1;
}
int port_record_frame(uint64_t frame, const PbwInputState *state) {
    char columns[96], line[128];
    PbwInputState clean = *state;
    int n;
    if (sFile == NULL || !sStarted || sFailed || frame == 0 || frame > PBW_INPUT_MAX_FRAME || frame <= sFrame) {
        return Fail();
    }
    clean.buttons &= PBW_INPUT_ALL;
    clean.lidClosed = clean.lidClosed != 0;
    clean.touching = clean.touching != 0;
    if (!clean.touching) {
        clean.touchX = clean.touchY = 0;
    }
    if (clean.touching && clean.touchY >= PBW_INPUT_TOUCH_HEIGHT) {
        return Fail();
    }
    sFrame = frame;
    if (memcmp(&clean, &sState, sizeof(clean)) == 0) {
        return 1;
    }
    if (!pbw_input_format_state(&clean, columns, sizeof(columns))) {
        return Fail();
    }
    n = snprintf(line, sizeof(line), "%u %s\n", (unsigned)frame, columns);
    if (n < 0 || (size_t)n >= sizeof(line) || sIO.write(line, (size_t)n, sFile) != (size_t)n || !Commit()) {
        return Fail();
    }
    sState = clean;
    return 1;
}
int port_record_active(void) {
    return sFile != NULL && sStarted;
}
int port_record_close(void) {
    int ok = !sFailed;
    if (sFile != NULL) {
        if (sIO.flush(sFile) != 0) {
            Fail();
            ok = 0;
        }
        if (sIO.close(sFile) != 0) {
            fprintf(stderr, "port: --record: close failed\n");
            ok = 0;
        }
        sFile = NULL;
    }
    sStarted = 0;
    return ok;
}
PbwInputState port_record_input(void) {
    PbwInputState state = { 0 };
    int down, x, y;
    state.buttons = port_input_held();
    state.lidClosed = (uint8_t)port_input_closed();
    port_tp_get_touch(&down, &x, &y);
    state.touching = (uint8_t)(down != 0);
    if (down) {
        state.touchX = (uint8_t)x;
        state.touchY = (uint8_t)y;
    }
    return state;
}
int port_record_random_seeds(uint64_t *seeds, size_t count) {
    if (count > PBW_INPUT_MAX_SEEDS) {
        return 0;
    }
#ifdef _WIN32
    typedef NTSTATUS(WINAPI * Random)(BCRYPT_ALG_HANDLE, PUCHAR, ULONG, ULONG);
    HMODULE module = LoadLibraryA("bcrypt.dll");
    Random random;
    NTSTATUS status;
    if (module == NULL) {
        return 0;
    }
    random = (Random)(void *)GetProcAddress(module, "BCryptGenRandom");
    status = random != NULL
                 ? random(NULL, (PUCHAR)seeds, (ULONG)(count * sizeof(*seeds)), BCRYPT_USE_SYSTEM_PREFERRED_RNG)
                 : (NTSTATUS)-1;
    FreeLibrary(module);
    return status >= 0;
#else
    FILE *random = fopen("/dev/urandom", "rb");
    size_t n;
    int closeResult;
    if (random == NULL) {
        return 0;
    }
    n = fread(seeds, sizeof(*seeds), count, random);
    closeResult = fclose(random);
    return n == count && closeResult == 0;
#endif
}
