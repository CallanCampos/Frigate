#include "nhttp_internal.h"

// NHTTP_recvbuf: the header that a response has received, which is kept in the first NHTTP_HEADER_SIZE bytes of the
// response and in blocks after them, and the body that is read into the user's buffer

static void NHTTPi_getRecvBufPos(NHttpResponse *response, int offset, NHttpHeaderBlock **block, int *blockOffset);
static char NHTTPi_getRecvBufChar(NHttpResponse *response, NHttpHeaderBlock **block, int *blockOffset);

// Gives the block that holds a byte of the header and the byte's place in it (no block for the first bytes)
static void NHTTPi_getRecvBufPos(NHttpResponse *response, int offset, NHttpHeaderBlock **block, int *blockOffset) {
    NHttpHeaderBlock *current;
    int count;

    if (offset < NHTTP_HEADER_SIZE) {
        *block = NULL;
        *blockOffset = offset;
        return;
    }
    count = (offset - NHTTP_HEADER_SIZE) >> 9;
    current = response->blocks;
    while (count-- != 0) {
        current = current->next;
    }
    *block = current;
    *blockOffset = (offset - NHTTP_HEADER_SIZE) & (NHTTP_HEADER_BLOCK_SIZE - 1);
}

// Reads the byte at the cursor of the header and moves it on
static char NHTTPi_getRecvBufChar(NHttpResponse *response, NHttpHeaderBlock **block, int *blockOffset) {
    int index;

    if (*block == NULL) {
        if (*blockOffset < NHTTP_HEADER_SIZE) {
            int offset = (*blockOffset)++;

            return response->header[offset];
        }
        *blockOffset = 0;
        *block = response->blocks;
    } else if (*blockOffset == NHTTP_HEADER_BLOCK_SIZE) {
        *blockOffset = 0;
        *block = (*block)->next;
    }
    index = (*blockOffset)++;
    return (*block)->data[index];
}

// Looks for the end of the line that starts at start. Gives where the next line starts (0 if the header ends there), and
// the place of the first colon in colon, and how the line ended in lineEnd (1 for LF or CR, 2 for CR LF). -1 if the line
// does not end before end
int NHTTPi_findNextLineHdrRecvBuf(NHttpResponse *response, int start, int end, int *colon, int *lineEnd) {
    NHttpHeaderBlock *block;
    int offset;
    int result;
    int last;
    BOOL sawCR;
    char c;

    if (colon != NULL) {
        *colon = -1;
    }
    if (start < end) {
        result = -1;
        sawCR = FALSE;
        NHTTPi_getRecvBufPos(response, start, &block, &offset);
        if (start < end) {
            last = end - 1;
            do {
                c = NHTTPi_getRecvBufChar(response, &block, &offset);
                if (c == ':' && colon != NULL && *colon < 0) {
                    *colon = start;
                }
                if (sawCR) {
                    if (c == '\n') {
                        result = (start == end - 1) ? 0 : start + 1;
                        if (lineEnd != NULL) {
                            *lineEnd = 2;
                        }
                    }
                    return result;
                }
                if (c == '\r') {
                    result = (start == last) ? 0 : start + 1;
                    sawCR = TRUE;
                    if (lineEnd != NULL) {
                        *lineEnd = 1;
                    }
                }
                if (c == '\n') {
                    int next = (start == end - 1) ? 0 : start + 1;

                    if (lineEnd != NULL) {
                        *lineEnd = 1;
                        return next;
                    }
                    return next;
                }
                start++;
            } while (start < end);
        }
    }
    return -1;
}

// Gives the place of the first byte that is not a space, or -1
int NHTTPi_skipSpaceHdrRecvBuf(NHttpResponse *response, int start, int end) {
    NHttpHeaderBlock *block;
    int offset;

    if (start < end) {
        NHTTPi_getRecvBufPos(response, start, &block, &offset);
        if (start < end) {
            do {
                if (NHTTPi_getRecvBufChar(response, &block, &offset) != ' ') {
                    return start;
                }
                start++;
            } while (start < end);
        }
    }
    return -1;
}

// Compares the header from start with a token without caring for the case. 0 if they are equal up to the end of the token
// (its end, a space or the terminator), -1 if not
int NHTTPi_compareTokenN_HdrRecvBuf(NHttpResponse *response, int start, int end, const char *token, int terminator) {
    NHttpHeaderBlock *block;
    int offset;
    int c;

    if (start < end) {
        NHTTPi_getRecvBufPos(response, start, &block, &offset);
        c = NHTTPi_getRecvBufChar(response, &block, &offset);
        while ((NHTTPi_isUpper(c) ? c + ('a' - 'A') : c)
               == (NHTTPi_isUpper(*token) ? *token + ('a' - 'A') : *token)) {
            if (*token == 0 || *token == ' ' || *token == terminator || start == end - 1) {
                return 0;
            }
            c = NHTTPi_getRecvBufChar(response, &block, &offset);
            start++;
            token++;
        }
    }
    return -1;
}

// Copies size bytes of the header, from start, into dest. FALSE if the header does not reach that far
BOOL NHTTPi_loadFromHdrRecvBuf(NHttpResponse *response, char *dest, int start, int size) {
    NHttpHeaderBlock *block;
    int count;

    if (start + size <= response->headerSize) {
        if (size != 0) {
            if (start < NHTTP_HEADER_SIZE) {
                count = NHTTP_HEADER_SIZE - start;
                if (size <= count) {
                    count = size;
                }
                NHTTPi_memcpy(dest, response->header + start, count);
                start += count;
                size -= count;
                dest += count;
            }
            if (size != 0) {
                int blocks;

                start -= NHTTP_HEADER_SIZE;
                blocks = start >> 9;
                start &= NHTTP_HEADER_BLOCK_SIZE - 1;
                block = response->blocks;
                while (blocks-- != 0) {
                    block = block->next;
                }
                if (size != 0) {
                    do {
                        count = NHTTP_HEADER_BLOCK_SIZE - start;
                        if (size <= count) {
                            count = size;
                        }
                        NHTTPi_memcpy(dest, block->data + start, count);
                        start = (start + count) & (NHTTP_HEADER_BLOCK_SIZE - 1);
                        size -= count;
                        block = block->next;
                        dest += count;
                    } while (size != 0);
                }
            }
        }
        return TRUE;
    }
    return FALSE;
}

BOOL NHTTPi_isRecvBufFull(NHttpResponse *response, u32 size) {
    return response->bufferSize <= size;
}

// Reads from the socket into the free part of the user's buffer
int NHTTPi_RecvBuf(NHttpMutexInfo *mutexInfo, NHttpRequest *request, int sock, int offset, int flags) {
    NHttpResponse *response = request->response;
    int size = response->bufferSize - offset;
    char *buffer = response->buffer + offset;

    return NHTTPi_SocRecv(mutexInfo, request, sock, buffer, size, flags);
}

// The same for no more than size bytes, or -1003 when there is no room left
int NHTTPi_RecvBufN(NHttpMutexInfo *mutexInfo, NHttpRequest *request, int sock, int offset, int size, int flags) {
    NHttpResponse *response = request->response;
    int available;

    if (NHTTPi_isRecvBufFull(response, offset)) {
        return NHTTP_BUFFER_FULL;
    }
    available = response->bufferSize - offset;
    if (size <= available) {
        available = size;
    }
    return NHTTPi_SocRecv(mutexInfo, request, sock, response->buffer + offset, available, flags);
}
