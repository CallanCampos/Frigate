// The Metrowerks runtime's global_destructor_chain.c: the chain of global objects whose destructors are to run, which
// __register_global_object extends. The compiler's code for a global object with a destructor calls it as the
// constructor ends, with the object's link in the chain in memory it provides, and so does atexit's __register_atexit.
// NitroSDK's overlay unloading (fs_overlay.c) walks the chain.
#include <NMWException.h>

DestructorChain *__global_destructor_chain;

void *__register_global_object(void *object, void (*destructor)(void *object), void *regmem) {
    DestructorChain *chain = regmem;

    chain->next = __global_destructor_chain;
    chain->destructor = destructor;
    chain->object = object;
    __global_destructor_chain = chain;
    return object;
}
