#ifndef POKEBW2_STDLIB_H
#define POKEBW2_STDLIB_H

// The C library's stdlib.h, from CodeWarrior's MSL, which the main module links

#include <stddef.h>

// What rand returns at most: it keeps 15 bits of its generator
#define RAND_MAX 0x7FFF

int abs(int x);

// The pseudo-random number generator (rand.c): a linear congruential one, seeded to 1
int rand(void);
void srand(unsigned int seed);

// A heapsort of nmemb elements of the given size, in place, by the comparison function
void qsort(void *base, size_t nmemb, size_t size, int (*compare)(const void *, const void *));

// The multibyte conversions (mbstring.c), through the current locale's conversion. wctomb returns the number of bytes
// it wrote to s; wcstombs, the number it wrote to s, of at most n
int wctomb(char *s, wchar_t wc);
size_t wcstombs(char *s, const wchar_t *pwcs, size_t n);

// The floating-point conversions (strtold.c). strtod reads the number at the start of str, after its white space, and
// sets *end to where it stopped unless end is NULL. atod is swan's name for atof, which is strtod without end.
double strtod(const char *str, char **end);
double atof(const char *str);

// The integer conversions (strtoul.c). They read the number at the start of str, after its white space and sign, in the
// given base (0 reads the base off the prefix: 0x is 16, 0 is 8, else 10), and set *end to where they stopped unless
// end is NULL. A number out of range gives the nearest value and sets errno to ERANGE. atol and atoll are strtol and
// strtoll in base 10.
unsigned long strtoul(const char *str, char **end, int base);
long strtol(const char *str, char **end, int base);
long long strtoll(const char *str, char **end, int base);
long atol(const char *str);
long long atoll(const char *str);

// The exit functions: atexit puts a function on the list that exit runs, and returns what __register_atexit does
int atexit(void (*func)(void));

// The heap, which NitroSDK's OS heap of the main arena holds
void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t size);
void *calloc(size_t nmemb, size_t size);

#endif // POKEBW2_STDLIB_H
