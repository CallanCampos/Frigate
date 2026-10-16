// MSL C's secure_error.c: where the library reports a runtime constraint violation of its bounds-checked functions.
// __msl_runtime_constraint_violation_s calls the constraint handler, or ignore_handler_s, which drops the report, when
// none is set. The handler pointer is the file's only data.
#include "secure_error.h"
#include "types.h"

static constraint_handler_t __msl_constraint_handler = NULL;

void __msl_runtime_constraint_violation_s(const char *msg, void *ptr, int error) {
    if (__msl_constraint_handler != NULL) {
        __msl_constraint_handler(msg, ptr, error);
    } else {
        ignore_handler_s(msg, ptr, error);
    }
}

void ignore_handler_s(const char *msg, void *ptr, int error) {
}
