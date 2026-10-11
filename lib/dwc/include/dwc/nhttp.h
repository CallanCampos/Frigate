#ifndef POKEBW2_DWC_NHTTP_H
#define POKEBW2_DWC_NHTTP_H

#include "types.h"

// Nintendo's HTTP client (NHTTP), in overlay 189 beside nhttp_rap.c, which wraps it. The library's code is
// lib/dwc/src/nhttp/. The ROM names none of it except in one assertion's text, " NHTTP_SetRootCA(%d)"; the names are
// those of pret/pokerevo's Wii build of the same library (asm/SDK/NHTTP/d_nhttp*.s), whose functions these are
// (NHTTPCreateConnection, NHTTPStartConnection, NHTTPGetBodyBuffer, NHTTPGetUserParam, NHTTPGetConnectionError,
// NHTTPStartup, NHTTPCleanupAsync, NHTTPAddHeaderField, NHTTPAddPostDataAscii, NHTTPGetResultCode, NHTTPSetVerifyOption),
// or descriptive for the rest. A connection is a handle that NHTTPCreateConnection returns: 0 if it could not

typedef void *(*NHttpAllocFunc)(u32 size, int align);
typedef void (*NHttpFreeFunc)(void *ptr);
// Called with the connection, the event and the argument given to NHTTPCreateConnection. The events are the post data
// being sent (1), the buffer being full (2), the body being received (3) and the connection ending (4)
typedef int (*NHttpCallback)(int connection, int event, void *arg);

// The root certificate that the answers' TLS is checked against, in overlay 11
extern u8 data_ov011_021866c0[];

// Starts the library with an allocator and the priority of the thread that runs the requests; 0 once it started
int NHTTPStartup(NHttpAllocFunc alloc, NHttpFreeFunc free, u32 priority);
// Ends the library, calling back when it has ended
void NHTTPCleanupAsync(void (*callback)(void));
void NHTTPCleanup(void);
// The library's last error
int NHTTPGetError(void);
// Adds a header field to a request; nonzero when it failed
int NHTTPAddHeaderField(int connection, const char *name, const char *value);
// Adds a part to the post data of a request; nonzero when it failed
int NHTTPAddPostDataAscii(int connection, const char *name, const char *value);
// Sets the data that a request posts
int NHTTPSetPostDataRaw(int connection, const void *data, u32 size);
// Copies the value of a header field of the answer out, and returns its length, or -1
int NHTTPGetHeaderField(int connection, const char *name, char **value);
// Gives the answer's whole header; its size, or -1
int NHTTPGetHeaderAll(int connection, char **header);
// The answer's HTTP status, or -1
int NHTTPGetResultCode(int connection);
int NHTTPSetVerifyOption(int connection, int option);
// Sets the user name and the password of a request's basic authorization; nonzero when it failed
int NHTTPSetBasicAuthorization(int connection, const char *user, const char *password);
// Sets the root certificates of a request, count of them; negative when it failed
int NHTTPSetRootCA(int connection, void *certificates, int count);
// Creates a connection for a URL with a method (0 to get, 1 to post, 2 to ask for the header) and the buffer that receives
// the answer, and returns it, 0 when it failed
int NHTTPCreateConnection(const char *url, int method, void *buffer, u32 bufferSize, NHttpCallback callback, void *arg);
// Destroys a connection; nonzero when it could not, negative for an unknown one
int NHTTPDestroyConnection(int connection);
// Sends a connection's request, returning negative when it failed
int NHTTPStartConnection(int connection);
// Cancels a connection's request
int NHTTPCancelConnection(int connection);
// The stage of a connection
int NHTTPGetConnectionState(int connection);
// The buffer of the answer and its size, and the number of bytes that the answer has so far
int NHTTPGetBodyBuffer(int connection, char **buffer, int *size);
// The argument that the connection's request was made with
void *NHTTPGetUserParam(int connection);
// A connection's state: 0 once its answer is complete, 15 while it is receiving, negative for an unknown one
int NHTTPGetConnectionError(int connection);
// What has been received so far, and the size of the answer
int NHTTPGetProgress(int connection, int *received, int *size);

#endif // POKEBW2_DWC_NHTTP_H
