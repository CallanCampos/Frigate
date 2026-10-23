// Checks MSL's 16-bit strings at the host libc boundary (port/hal/stubs/wstring_host.c), used by DWC's login
// fields, and the case-blind ASCII extensions used by GameSpy (port/hal/stubs/extras_string.c). Adjacent nonzero code
// units and an odd-length terminator expose a mistaken 32-bit libc scan or copy.
#include "nitrowifi/soc.h"
#include <assert.h>
#include <string.h>
#include <wchar.h>

// DWC utility's SOC configuration keeps its hostname pointer whole after upstream named the SDK fields.
static const char sHostName[] = "NINTENDO-DS";
static const SOCConfig sSocConfig = { .hostName = sHostName, .rdhcp = 4 };

int main(void) {
    _Static_assert(sizeof(sSocConfig.hostName) == sizeof(void *), "SOC options preserve native pointers");
    assert((const char *)sSocConfig.hostName == sHostName);
    _Static_assert(sizeof(wchar_t) == 2, "DS strings use 16-bit code units");
    const wchar_t source[] = { 0x41, 0x3042, 0xd83d, 0xde00, 0x42, 0 };
    wchar_t copy[7] = { 0, 0, 0, 0, 0, 0, 0x1234 };
    assert(wcslen(source) == 5);
    assert(wcslen(source + 5) == 0);
    assert(wcscpy(copy, source) == copy);
    for (size_t i = 0; i < 6; i++) {
        assert(copy[i] == source[i]);
    }
    assert(copy[6] == 0x1234);
    assert(strcmpi("AbC", "aBc") == 0);
    assert(stricmp("a", "Z") == -1);
    assert(stricmp("z", "A") == 1);
    assert(stricmp("\x80", "\x81") == -1);
    assert(strnicmp("ABx", "abY", 2) == 0);
    assert(strnicmp("ABx", "abY", 3) == -1);
    assert(strnicmp(NULL, NULL, 0) == 0);
    assert(strnicmp(NULL, NULL, -1) == 0);
    return 0;
}
