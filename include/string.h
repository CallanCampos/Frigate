#ifndef POKEBW2_STRING_H
#define POKEBW2_STRING_H

// The C library's string.h, from CodeWarrior's MSL, which the main module links

#include <stddef.h>

void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memset(void *dst, int c, size_t n);
void *memchr(const void *s, int c, size_t n);
int memcmp(const void *a, const void *b, size_t n);

size_t strlen(const char *s);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t n);
char *strcat(char *dst, const char *src);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);
char *strchr(const char *s, int c);
size_t strcspn(const char *str, const char *set);
char *strstr(const char *str, const char *pat);

// MSL's case-blind comparisons, which compare the characters through tolower
int stricmp(const char *a, const char *b);
int strnicmp(const char *a, const char *b, int n);
int strcmpi(const char *a, const char *b);

#endif // POKEBW2_STRING_H
