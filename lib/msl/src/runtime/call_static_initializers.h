#ifndef POKEBW2_MSL_RUNTIME_CALL_STATIC_INITIALIZERS_H
#define POKEBW2_MSL_RUNTIME_CALL_STATIC_INITIALIZERS_H

// The static initializers of the program, which the linker collects in a table (the .ctor section of the linker
// script) that ends with a zero entry
extern void (*ARM9_CTOR_START[])(void);

// Calls the program's static initializers (call_static_initializers.c), which NitroSDK's startup code runs right after
// the runtime's own initialization. The runtime calls it __call_static_initializers.
void __call_static_initializers(void); // swan's boot_CallStaticInit

#endif // POKEBW2_MSL_RUNTIME_CALL_STATIC_INITIALIZERS_H
