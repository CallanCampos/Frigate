// MSL C's secure_error.c: where the library reports a runtime constraint violation of its bounds-checked functions.
// __msl_runtime_constraint_violation_s calls the constraint handler, or ignore_handler_s, which drops the report, when
// none is set. The handler pointer is the file's only data.
#include "secure_error.h"
#include "types.h"

static constraint_handler_t data_0214f1f0 = NULL;

void nothing(const char *msg, void *ptr, int error) {
    if (data_0214f1f0 != NULL) {
        data_0214f1f0(msg, ptr, error);
    } else {
        func_02085d98(msg, ptr, error);
    }
}

void func_02085d98(const char *msg, void *ptr, int error) {
}
