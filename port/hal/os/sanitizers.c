// The sanitizer runtime's hooks for Frigate's sanitized build (PORT_SANITIZE, port/docs/sanitizers.md), so that
// UndefinedBehaviorSanitizer reports the game's code where it runs: on the fibers that carry the SDK's threads
// (port/hal/os/fiber.c), the game's main thread among them. Every program that links the platform layer has it; in a
// build without AddressSanitizer and UndefinedBehaviorSanitizer both, it is empty.
//
// UBSan's own stack trace (UBSAN_OPTIONS=print_stacktrace=1) stops at the frame of the report on a fiber: it unwinds
// only within the host thread's own stack, and ignores the switches that fiber.c tells AddressSanitizer about. The
// runtime calls __ubsan_on_report at each report it makes, before printing it; this one prints ASan's trace, which
// follows the fiber's frames down to FiberMain, so the report's own lines come after its stack.
//
// A build with PORT_SANITIZE_RECOVER, whose checks can go on after a report, also makes going on ASan's default
// (halt_on_error=0), so that one run surveys every report of a route; ASan still reports each place once
// (suppress_equal_pcs), as UBSan always does.
#if defined(__has_feature)
#if __has_feature(undefined_behavior_sanitizer) && __has_feature(address_sanitizer)
#include <sanitizer/common_interface_defs.h>
#include <stdio.h>

// The runtime's monitor interface (compiler-rt's ubsan_monitor.h), which no installed header declares
void __ubsan_on_report(void);
void __ubsan_get_current_report_data(const char **outIssueKind, const char **outMessage, const char **outFilename,
                                     unsigned *outLine, unsigned *outCol, char **outMemoryAddr);

void __ubsan_on_report(void) {
    const char *kind = NULL;
    const char *message = NULL;
    const char *file = NULL;
    unsigned line = 0;
    unsigned column = 0;
    char *address = NULL;

    __ubsan_get_current_report_data(&kind, &message, &file, &line, &column, &address);
    fprintf(stderr, "port: UndefinedBehaviorSanitizer: %s at %s:%u:%u, from this stack:\n", kind ? kind : "a report",
            file ? file : "?", line, column);
    __sanitizer_print_stack_trace();
}

#ifdef PORT_SANITIZE_RECOVER
// The runtime's defaults for this program, which ASAN_OPTIONS overrides
const char *__asan_default_options(void);

const char *__asan_default_options(void) {
    return "halt_on_error=0";
}
#endif
#endif
#endif
