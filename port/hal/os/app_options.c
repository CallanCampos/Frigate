// The program's options and its presentation hook (port_app.h), kept in the platform layer rather than in the program
// (port/app/main.c), so that the modules that read them link into every executable that has the platform layer.
#include "hal/os/port_app.h"
#include <string.h>

static int sArgc;
static char **sArgv;
static const char *const *sFlags;
static size_t sFlagCount;

void (*port_app_present)(void);

void port_app_set_arguments(int argc, char **argv, const char *const *flags, size_t flagCount) {
    sArgc = argc;
    sArgv = argv;
    sFlags = flags;
    sFlagCount = flagCount;
}

int port_app_is_flag(const char *name, size_t length) {
    for (size_t i = 0; i < sFlagCount; i++) {
        if (strlen(sFlags[i]) == length && strncmp(sFlags[i], name, length) == 0) {
            return 1;
        }
    }
    return 0;
}

const char *port_app_option(const char *name) {
    size_t length = strlen(name);

    for (int i = 1; i < sArgc; i++) {
        const char *arg = sArgv[i];

        if (strncmp(arg, "--", 2) != 0 || strncmp(arg + 2, name, length) != 0) {
            continue;
        }
        if (arg[2 + length] == '=') {
            return arg + 2 + length + 1;
        }
        if (arg[2 + length] != '\0') {
            continue;
        }
        if (port_app_is_flag(name, length) || i + 1 >= sArgc || strncmp(sArgv[i + 1], "--", 2) == 0) {
            return "";
        }
        return sArgv[i + 1];
    }
    return NULL;
}
