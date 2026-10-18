// The program's name, options and hooks, for every module of the platform layer (port/hal, port/gfx) and the program
// itself. The program (port/app/main.c) records its arguments before it runs the game, and a module reads the option
// it needs with port_app_option; the renderer's integration sets port_app_present, which the program's frame hook
// calls at each VBlank; the program sets port_app_fatal_notice for a player who can't see stderr. They live in port_hal
// (app_options.c), so that every executable that links the platform layer has them, the OS tests included; one that
// records no arguments reads every option as absent. Plain C types only, like port_os.h, so that files without the
// decomp's headers can include it.
#ifndef PORT_HAL_OS_PORT_APP_H
#define PORT_HAL_OS_PORT_APP_H

#include <stddef.h>

// The product's name, as the player sees it: the window's title (port/app/present.c), --help (port/app/main.c), and
// the folders of the saves (port/hal/card/backup.c) and of an app's log (port/app/message_log.c). The build's names,
// the programs' and the macOS app's, are in port/CMakeLists.txt and port/cmake/macos_app.cmake
#define PORT_PRODUCT_NAME "Frigate"

// Records the program's arguments and the names of its flags, the options that never take a value. Both arrays must
// stay as they are while the program runs (main's argv, and a static table)
void port_app_set_arguments(int argc, char **argv, const char *const *flags, size_t flagCount);
// An option's value: "value" for --name=value, the next argument for --name followed by one that isn't an option
// (unless the option is a flag), "" for --name alone, and NULL when the option isn't given
const char *port_app_option(const char *name);
// Whether the first length characters of name are the name of a flag
int port_app_is_flag(const char *name, size_t length);

// The renderer's presentation of a frame, called by the program's frame hook on the host's stack at each VBlank; NULL
// until the renderer's integration sets it
extern void (*port_app_present)(void);

// Tells the player why the program stops, for a stop they can act on (no ROM, a ROM the port refuses): a module calls
// it with its message, which it has written to stderr already, just before it stops the program. NULL unless the
// program sets it, which it does when nobody sees stderr (an app opened from the Finder, port/app/message_log.h)
extern void (*port_app_fatal_notice)(const char *message);

#endif // PORT_HAL_OS_PORT_APP_H
