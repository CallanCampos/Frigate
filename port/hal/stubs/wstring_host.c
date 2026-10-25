// MSL wide-string operations for DWC authentication names (lib/dwc/src/auth/dwc_auth.c). The decomp uses
// 16-bit wchar_t, while macOS and Linux libc use 32-bit units. port_host.h renames these original MSL functions
// so neither the game nor the host can accidentally bind them to the other ABI.
#include "../../../lib/msl/src/wstring.c"
