#ifndef POKEBW2_MSL_SECURE_ERROR_H
#define POKEBW2_MSL_SECURE_ERROR_H

// The error reports of the library's bounds-checked functions (secure_error.c), such as printf's __pformatter gives for
// a null argument of a secure conversion: __msl_runtime_constraint_violation_s, and the handler that ignores a report,
// ignore_handler_s. The names are MSL's own (the library's private header)

// What the handler is given: a message, which a caller may leave out, a pointer to the data at fault, and an error
// code. The library has no handler of its own to set, so reports go to the ignoring one.
typedef void (*constraint_handler_t)(const char *msg, void *ptr, int error);

void nothing(const char *msg, void *ptr, int error);
void func_02085d98(const char *msg, void *ptr, int error);

#endif // POKEBW2_MSL_SECURE_ERROR_H
