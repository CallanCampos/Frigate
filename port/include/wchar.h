// The DS wide-string ABI for decomp sources, used by DWC authentication and profanity requests. Host libc's
// wchar_t differs on macOS/Linux; only the original MSL scan/copy are exposed, with names from port_host.h.
// Other wide libc calls must get their own DS implementation before the game may use them.
#ifndef PORT_WCHAR_H
#define PORT_WCHAR_H
#include <inttypes.h>
#include <stddef.h>
// Read the ordinary libc declarations before poisoning their unsupported conversion functions.
#include <stdio.h>
#include <stdlib.h>
#include <wctype.h>
size_t port_msl_wcslen(const wchar_t *s);
wchar_t *port_msl_wcscpy(wchar_t *dst, const wchar_t *src);
// Fail explicitly for unsupported host wide APIs, including on Windows, to keep one DS ABI everywhere.
#pragma GCC poison btowc fgetwc fgetws fputwc fputws fwide fwprintf fwscanf getwc getwchar iswalnum iswalpha iswblank iswcntrl iswctype iswdigit iswgraph iswlower iswprint iswpunct iswspace iswupper iswxdigit mbrlen mbrtowc mbsinit mbsrtowcs mbstowcs mbtowc putwc putwchar swprintf swscanf towctrans towlower towupper ungetwc vfwprintf vfwscanf vswprintf vswscanf vwprintf vwscanf wcrtomb wcscat wcschr wcscmp wcscoll wcscspn wcsftime wcsncat wcsncmp wcsncpy wcspbrk wcsrchr wcsrtombs wcsspn wcsstr wcstod wcstof wcstoimax wcstok wcstol wcstold wcstoll wcstombs wcstoul wcstoull wcstoumax wcsxfrm wctob wctomb wctrans wctype wmemchr wmemcmp wmemcpy wmemmove wmemset wprintf wscanf
#endif
