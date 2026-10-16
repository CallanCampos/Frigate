#ifndef POKEBW2_MSL_RUNTIME_REGISTER_ATEXIT_H
#define POKEBW2_MSL_RUNTIME_REGISTER_ATEXIT_H

// The runtime's list of the functions that exit runs (register_atexit.c), which atexit adds to. It returns 0, or -1
// when the list's 64 entries are full.
int __register_atexit(void (*func)(void));

#endif // POKEBW2_MSL_RUNTIME_REGISTER_ATEXIT_H
