// The log of an app opened from the Finder or the Dock (port/app/message_log.h), on macOS only: when macOS gives the
// app no stderr, the program's messages, the reason it stops included, go to ~/Library/Logs/Frigate/<program>.log,
// and a stop the player can act on (no ROM, a ROM Frigate refuses) shows an alert as well. It touches no part of the
// game; it is how a player who opened the app learns why the game stopped.
#include "app/message_log.h"
#include "hal/os/fiber.h"
#include "hal/os/port_app.h"
#include "hal/rom/rom_manifest.h"
#include <SDL3/SDL.h>
#include <stdio.h>

#ifdef __APPLE__
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <mach-o/dyld.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define APP_EXECUTABLE_DIRECTORY ".app/Contents/MacOS"
// The directory under ~/Library/Logs: the product's name, as the saves' directory has under Application Support
#define LOG_DIRECTORY "Library/Logs/" PORT_PRODUCT_NAME

static char sPath[PATH_MAX];

// Whether stderr goes nowhere: closed, or on /dev/null, where LaunchServices puts an app's
static bool StderrUnseen(void) {
    struct stat stream;
    struct stat null;

    if (fstat(STDERR_FILENO, &stream) != 0) {
        return true;
    }
    return S_ISCHR(stream.st_mode) && stat("/dev/null", &null) == 0 && stream.st_rdev == null.st_rdev;
}

// The executable's file name when it runs from an app, that is from <name>.app/Contents/MacOS, through any links
static bool AppExecutable(char *name, size_t size) {
    char raw[PATH_MAX];
    char real[PATH_MAX];
    uint32_t rawSize = sizeof(raw);
    const size_t suffix = strlen(APP_EXECUTABLE_DIRECTORY);
    char *slash;

    if (_NSGetExecutablePath(raw, &rawSize) != 0 || realpath(raw, real) == NULL) {
        return false;
    }
    slash = strrchr(real, '/');
    if (slash == NULL || (size_t)(slash - real) < suffix ||
        strncmp(slash - suffix, APP_EXECUTABLE_DIRECTORY, suffix) != 0) {
        return false;
    }
    return snprintf(name, size, "%s", slash + 1) < (int)size;
}

// Makes each directory of the log's path under home that doesn't exist yet
static bool MakeLogDirectories(const char *home) {
    char directory[PATH_MAX];
    size_t length = (size_t)snprintf(directory, sizeof(directory), "%s/%s", home, LOG_DIRECTORY);

    if (length >= sizeof(directory)) {
        return false;
    }
    for (size_t i = strlen(home) + 1; i <= length; i++) {
        if (directory[i] == '/' || directory[i] == '\0') {
            const char separator = directory[i];

            directory[i] = '\0';
            if (mkdir(directory, 0755) != 0 && errno != EEXIST) {
                return false;
            }
            directory[i] = separator;
        }
    }
    return true;
}

const char *port_message_log_open(void) {
    char name[NAME_MAX + 1];
    const char *home = getenv("HOME");
    int fd;

    if (!StderrUnseen() || !AppExecutable(name, sizeof(name)) || home == NULL || home[0] != '/' ||
        !MakeLogDirectories(home) ||
        snprintf(sPath, sizeof(sPath), "%s/%s/%s.log", home, LOG_DIRECTORY, name) >= (int)sizeof(sPath)) {
        return NULL;
    }
    fd = open(sPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        return NULL;
    }
    // The descriptor may already be 1 or 2, when stdout or stderr was closed; it stays open as that one
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    if (fd > STDERR_FILENO) {
        close(fd);
    }
    return sPath;
}

void port_message_log_alert(const char *message) {
    char title[128];
    char text[2048];

    if (port_fiber_current() != NULL) {
        return;
    }
    // Titled as the window is (port/app/present.c): "Frigate — Pokémon Black 2"
    snprintf(title, sizeof(title), "%s — %s", PORT_PRODUCT_NAME, port_rom_manifest_game);
    snprintf(text, sizeof(text), "%s\n\nThe program's messages are in %s.", message, sPath);
    if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, text, NULL)) {
        fprintf(stderr, "port: the alert didn't show: %s\n", SDL_GetError());
    }
}

#else

const char *port_message_log_open(void) {
    return NULL;
}

void port_message_log_alert(const char *message) {
    (void)message;
}

#endif
