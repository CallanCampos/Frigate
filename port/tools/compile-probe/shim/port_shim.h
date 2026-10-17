/* Force-included (-include) by the host probe: the MWCC built-ins and keywords that clang lacks.
   A real port would put these in a platform header behind a macro (e.g. PLATFORM_PC). */
#define __memcpy __builtin_memcpy
