// Messages of an app opened from the Finder or the Dock (port/app/message_log.c). macOS starts such an app with stderr
// on /dev/null, so every message of the program and the platform layer, the reason it stops included, would be lost.
// The program moves stderr and stdout to a log file instead, ~/Library/Logs/pokebw2-native/<program>.log, which
// Console lists under Log Reports, and shows the reason for a stop that the player can act on (no ROM, or a ROM it
// refuses) in an alert as well (port_app_fatal_notice, port/hal/os/port_app.h).
#ifndef PORT_APP_MESSAGE_LOG_H
#define PORT_APP_MESSAGE_LOG_H

// When the program runs from an app (its executable in <name>.app/Contents/MacOS) and stderr goes nowhere (/dev/null,
// or closed), opens the log, emptied, as stderr and stdout, and returns its path; NULL otherwise, when the log can't be
// opened, and on any host but macOS. main() calls it before anything is written
const char *port_message_log_open(void);

// Shows a message in an alert, with the log's path, for port_app_fatal_notice. Only on the host's own stack: Cocoa
// runs an alert's event loop on the stack it is called from, and most game threads' fibers have 256 KiB of stack
// (port/hal/os/core.c), which nothing says an alert fits in; from a fiber, the message is in the log alone
void port_message_log_alert(const char *message);

#endif // PORT_APP_MESSAGE_LOG_H
