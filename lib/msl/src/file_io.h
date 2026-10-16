#ifndef POKEBW2_MSL_FILE_IO_H
#define POKEBW2_MSL_FILE_IO_H

// Compares two strings up to n characters without regard to case, as strnicmp and the other case blind comparisons do.
// Returns -1, 0 or 1
int __msl_strnicmp(const char *str1, const char *str2, int n);

#endif // POKEBW2_MSL_FILE_IO_H
