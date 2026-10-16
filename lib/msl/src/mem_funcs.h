#ifndef POKEBW2_MSL_MEM_FUNCS_H
#define POKEBW2_MSL_MEM_FUNCS_H

// The fill of the memory functions (mem_funcs.c), which memset uses. The name is MSL's own (the library's private
// header)
void __fill_mem(void *dst, int val, unsigned long n);

#endif // POKEBW2_MSL_MEM_FUNCS_H
