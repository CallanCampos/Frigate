#ifndef POKEBW2_DWC_NHTTP_H
#define POKEBW2_DWC_NHTTP_H

#include "types.h"

// Nintendo's HTTP client (NHTTP), in overlay 189 beside nhttp_rap.c, which wraps it. The ROM names none of it except in
// one assertion's text, " NHTTP_SetRootCA(%d)"; the functions are named by what nhttp_rap.c does with them

typedef void *(*NHttpAllocFunc)(u32 size, int align);
typedef void (*NHttpFreeFunc)(void *ptr);
// Called with the request, the event and the argument given to the request. The request ends with events 2 and 3
typedef int (*NHttpCallback)(int request, int event, void *arg);

// The root certificate that the answers' TLS is checked against, in overlay 11
extern u8 data_ov011_021866c0[];

// Starts the library with an allocator and the number of requests it may hold, probably NHTTP_Startup; 0 once it
// started
int func_ov189_021a076c(NHttpAllocFunc alloc, NHttpFreeFunc free, u32 maxRequests);
// Ends the library, probably NHTTP_Cleanup
int func_ov189_021a07a0(void);
// The library's last error
int func_ov189_021a07b0(void);
// Adds a header field to a request; nonzero when it failed
int func_ov189_021a07bc(int request, const char *name, const char *value);
// Sets the data that a request posts
int func_ov189_021a0854(int request, const void *data, u32 size);
// Copies the answer's body out; the size of the answer, or -1
int func_ov189_021a08cc(int request, int *size);
// The answer's HTTP status, or -1
int func_ov189_021a0900(int request);
// Sets the user name and the password of a request's basic authentication; nonzero when it failed
int func_ov189_021a0954(int request, const char *user, const char *password);
// Sets the root certificates of a request, count of them; negative when it failed
int func_ov189_021a09ec(int request, void *certificates, int count);
// Creates a request for a URL with a method (0 to get, 1 to post) and the buffer that receives the answer, and
// returns it, 0 when it failed
int func_ov189_021a0e1c(const char *url, int method, void *buffer, u32 bufferSize, NHttpCallback callback, void *arg);
// Destroys a request
void func_ov189_021a0edc(int request);
// Sends a request, returning negative when it failed
int func_ov189_021a0fe4(int request);
// A request's state: 0 once its answer is complete, 15 while it is receiving, negative for an unknown request
int func_ov189_021a10f4(int request);
// What has been received so far, and its size
int func_ov189_021a1114(int request, int *received, int *size);

#endif // POKEBW2_DWC_NHTTP_H
