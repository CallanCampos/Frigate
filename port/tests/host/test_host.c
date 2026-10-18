// Tests of the host's system (port/hal/host) as the platform layer and the program use it, the same on macOS, Linux
// and Windows: a trap placeholder touched stops the program with its name and status 70 (port/hal/trap.c), as the game
// would; a crash is reported with a backtrace before the fault ends the program; the watchdog ends a program that
// spins, with its message and status, and leaves one alone that kicks it; and the program's static data, directories,
// environment and private file mappings that the PXI bus (port/hal/pxi/pointer.c) and the tests stand on. The
// scenarios that end the process run one a process. None needs a ROM.
//
//   port_host_tests NAME
#include "hal/host/host.h"
#include "hal/trap.h"
#include <SDL3/SDL.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SKIP_STATUS 77
// The status the watchdog scenario asks for, as the program's watchdog has (port/app/main.c)
#define WATCHDOG_STATUS 73

static int sFailed;
// A zeroed static and a set one, which the static data must hold
static int sZeroed;
static int sSet = 5;

static void Check(int ok, const char *what) {
    if (!ok) {
        printf("  check failed: %s\n", what);
        sFailed = 1;
    }
}

// The first data placeholder, read once the placeholders are protected: the report names it, and the program stops
// with PORT_TRAP_EXIT_STATUS. Skipped once the game has C for every data object it references
static int TestTrapPlaceholder(void) {
    if (port_trap_data_count == 0) {
        printf("no data placeholders: skipped\n");
        return SKIP_STATUS;
    }
    port_trap_protect_data();
    printf("reading %s\n", port_trap_data_symbols[0].name);
    fflush(stdout);
    (void)*(volatile unsigned char *)port_trap_data_addresses[0];
    printf("never: the placeholder could be read\n");
    return 1;
}

// A write to an address no memory is at, with trap.c's handler in place: a crash, reported with the fault and a
// backtrace, then the program ends as the fault would end it (a signal, or the exception's status on Windows)
static int TestCrashReport(void) {
    port_trap_protect_data();
    printf("writing to address 0x10\n");
    fflush(stdout);
    *(volatile int *)(uintptr_t)0x10 = 1;
    printf("never: the write went through\n");
    return 1;
}

// Not inlined, so that the frame the watchdog shows is this one
__attribute__((noinline)) static void SpinForever(void) {
    volatile int spinning = 1;

    while (spinning) {
    }
}

// A program that spins where nothing kicks the watchdog: after a second it prints its message and ends with its status
static int TestWatchdog(void) {
    port_host_watchdog_start(1, "host test: the watchdog went off\n", WATCHDOG_STATUS);
    SpinForever();
    printf("never: the spin ended\n");
    return 1;
}

// A program that kicks it more often than its second runs on
static int TestWatchdogKicked(void) {
    Uint64 start = SDL_GetTicks();

    port_host_watchdog_start(1, "host test: the watchdog went off, though kicked\n", WATCHDOG_STATUS);
    while (SDL_GetTicks() - start < 2500) {
        port_host_watchdog_kick();
        SDL_Delay(100);
    }
    printf("watchdog_kicked: ok\n");
    return 0;
}

// The static data holds the program's statics, zeroed or set, and isn't empty
static int TestStaticData(void) {
    uintptr_t start;
    uintptr_t end;

    port_host_static_data(&start, &end);
    Check(start != 0 && end > start, "the static data has a range");
    Check((uintptr_t)&sZeroed >= start && (uintptr_t)(&sZeroed + 1) <= end, "it holds a zeroed static");
    Check((uintptr_t)&sSet >= start && (uintptr_t)(&sSet + 1) <= end && sSet == 5, "and a set one");
    printf("static_data: %s\n", sFailed ? "FAIL" : "ok");
    return sFailed;
}

// Pages, a directory, the environment and a private mapping of a file, in the temporary directory
static int TestFiles(void) {
    char dir[600];
    char path[700];
    size_t page = port_host_page_size();
    size_t size = 0;
    unsigned char *pages;
    unsigned char *copy;
    FILE *file;

    Check(page >= 4096 && (page & (page - 1)) == 0, "the page size is a power of two of 4 KiB or more");
    pages = port_host_map(4 * page);
    Check(pages != NULL && ((uintptr_t)pages & (page - 1)) == 0 && pages[page] == 0, "zeroed pages, page aligned");
    if (pages != NULL) {
        pages[3 * page] = 1;
        port_host_unmap(pages, 4 * page);
    }

    snprintf(dir, sizeof(dir), "%s/port_host_test_%d", port_host_temporary_directory(), (int)SDL_GetTicks());
    Check(port_host_make_directory(dir) == 0, "a directory is made");
    errno = 0;
    Check(port_host_make_directory(dir) != 0 && errno == EEXIST, "and not made again");

    Check(port_host_set_environment("PORT_HOST_TEST", "given") == 0 && getenv("PORT_HOST_TEST") != NULL &&
              strcmp(getenv("PORT_HOST_TEST"), "given") == 0,
          "a variable is set");
    Check(port_host_set_environment("PORT_HOST_TEST", NULL) == 0 && getenv("PORT_HOST_TEST") == NULL, "and removed");

    snprintf(path, sizeof(path), "%s/file", dir);
    file = fopen(path, "wb");
    Check(file != NULL && fwrite("abcdef", 1, 6, file) == 6, "a file is written");
    if (file != NULL) {
        fclose(file);
    }
    copy = port_host_map_file_copy(path, &size);
    Check(copy != NULL && size == 6 && memcmp(copy, "abcdef", 6) == 0, "its private mapping reads it");
    if (copy != NULL) {
        char read[7] = { 0 };

        copy[0] = 'X';
        file = fopen(path, "rb");
        Check(file != NULL && fread(read, 1, 6, file) == 6 && strcmp(read, "abcdef") == 0,
              "and a write to the mapping leaves the file as it was");
        if (file != NULL) {
            fclose(file);
        }
        port_host_unmap(copy, size);
    }
    Check(!port_host_is_symbolic_link(path), "a file isn't a link");
    remove(path);
    rmdir(dir);
    printf("files: %s\n", sFailed ? "FAIL" : "ok");
    return sFailed;
}

typedef struct Test {
    const char *name;
    int (*run)(void);
} Test;

static const Test sTests[] = {
    { "trap_placeholder", TestTrapPlaceholder }, { "crash_report", TestCrashReport }, { "watchdog", TestWatchdog },
    { "watchdog_kicked", TestWatchdogKicked },   { "static_data", TestStaticData },   { "files", TestFiles },
};

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, 0);
    for (size_t i = 0; i < sizeof(sTests) / sizeof(sTests[0]); i++) {
        if (argc > 1 && strcmp(argv[1], sTests[i].name) == 0) {
            return sTests[i].run();
        }
    }
    printf("usage: %s NAME\n", argv[0]);
    return 2;
}
