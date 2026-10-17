// A stand-in for the UCRT's errno.h, for the Windows compile check in CMakeLists.txt (this Mac has no Windows SDK or
// MinGW sysroot): only what the spike uses, with the UCRT's values
#ifndef WINDOWS_STUB_ERRNO_H
#define WINDOWS_STUB_ERRNO_H

int *_errno(void);
#define errno (*_errno())
#define ENOMEM 12
#define EINVAL 22
#define ENOTSUP 129

#endif
