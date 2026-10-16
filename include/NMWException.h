#ifndef POKEBW2_NMWEXCEPTION_H
#define POKEBW2_NMWEXCEPTION_H

// The Metrowerks runtime's NMWException.h: the list of the global objects whose destructors run at exit or when an
// overlay unloads (lib/msl/src/runtime/global_destructor_chain.c). Each object adds a link as its constructor ends: the
// compiler passes the link's memory, and for a function that atexit registered there is no object, only the function.

typedef struct DestructorChain {
    struct DestructorChain *next;
    void (*destructor)(void *object);
    void *object;
} DestructorChain;

// The newest link, which the unloading of an overlay reads and unlinks
extern DestructorChain *__global_destructor_chain;

// Links regmem in front of the chain and returns object
void *__register_global_object(void *object, void (*destructor)(void *object), void *regmem);

#endif // POKEBW2_NMWEXCEPTION_H
