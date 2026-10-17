// PNG files for the oracle's screens; see pngout.h. The format is the PNG specification's (ISO/IEC 15948): the
// signature, IHDR, one IDAT holding a zlib stream of the rows (each with filter type 0), IEND; each chunk with its
// CRC-32.
#include "pngout.h"
#include <cstdio>
#include <vector>

#ifdef ORACLE_HAVE_ZLIB
#include <zlib.h>
#endif

namespace oracle {

namespace {

uint32_t Crc32(const uint8_t *data, size_t length, uint32_t crc = 0) {
    static uint32_t table[256];
    static bool ready = false;

    if (!ready) {
        for (uint32_t n = 0; n < 256; n++) {
            uint32_t c = n;
            for (int k = 0; k < 8; k++) {
                c = (c & 1) ? 0xedb88320u ^ (c >> 1) : c >> 1;
            }
            table[n] = c;
        }
        ready = true;
    }
    crc = ~crc;
    for (size_t i = 0; i < length; i++) {
        crc = table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);
    }
    return ~crc;
}

void PutBig32(std::vector<uint8_t> &out, uint32_t value) {
    out.push_back(static_cast<uint8_t>(value >> 24));
    out.push_back(static_cast<uint8_t>(value >> 16));
    out.push_back(static_cast<uint8_t>(value >> 8));
    out.push_back(static_cast<uint8_t>(value));
}

void PutChunk(std::vector<uint8_t> &out, const char type[4], const std::vector<uint8_t> &data) {
    size_t start;

    PutBig32(out, static_cast<uint32_t>(data.size()));
    start = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), data.begin(), data.end());
    PutBig32(out, Crc32(out.data() + start, out.size() - start));
}

#ifndef ORACLE_HAVE_ZLIB
// A zlib stream of stored (uncompressed) deflate blocks, for builds without zlib
std::vector<uint8_t> StoredZlib(const std::vector<uint8_t> &raw) {
    std::vector<uint8_t> out = { 0x78, 0x01 };
    uint32_t a = 1;
    uint32_t b = 0;
    size_t at = 0;

    do {
        size_t length = raw.size() - at < 65535 ? raw.size() - at : 65535;
        bool last = at + length == raw.size();

        out.push_back(last ? 1 : 0);
        out.push_back(static_cast<uint8_t>(length));
        out.push_back(static_cast<uint8_t>(length >> 8));
        out.push_back(static_cast<uint8_t>(~length));
        out.push_back(static_cast<uint8_t>(~length >> 8));
        out.insert(out.end(), raw.begin() + static_cast<long>(at), raw.begin() + static_cast<long>(at + length));
        at += length;
    } while (at < raw.size());
    for (uint8_t byte : raw) {
        a = (a + byte) % 65521;
        b = (b + a) % 65521;
    }
    PutBig32(out, (b << 16) | a);
    return out;
}
#endif

} // namespace

bool WritePng(const std::string &path, const uint8_t *rgb, uint32_t width, uint32_t height, std::string &error) {
    static const uint8_t signature[8] = { 0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n' };
    const size_t rowBytes = static_cast<size_t>(width) * 3;
    std::vector<uint8_t> raw;
    std::vector<uint8_t> header;
    std::vector<uint8_t> compressed;
    std::vector<uint8_t> file(signature, signature + 8);

    raw.reserve((rowBytes + 1) * height);
    for (uint32_t y = 0; y < height; y++) {
        raw.push_back(0);
        raw.insert(raw.end(), rgb + y * rowBytes, rgb + (y + 1) * rowBytes);
    }
#ifdef ORACLE_HAVE_ZLIB
    uLongf size = compressBound(static_cast<uLong>(raw.size()));
    compressed.resize(size);
    if (compress2(compressed.data(), &size, raw.data(), static_cast<uLong>(raw.size()), 6) != Z_OK) {
        error = "zlib failed to compress " + path;
        return false;
    }
    compressed.resize(size);
#else
    compressed = StoredZlib(raw);
#endif

    PutBig32(header, width);
    PutBig32(header, height);
    // 8 bits a channel, RGB, deflate, adaptive filtering (all rows use none), no interlace
    header.insert(header.end(), { 8, 2, 0, 0, 0 });
    PutChunk(file, "IHDR", header);
    PutChunk(file, "IDAT", compressed);
    PutChunk(file, "IEND", {});

    FILE *out = std::fopen(path.c_str(), "wb");
    if (out == nullptr) {
        error = "can't create " + path;
        return false;
    }
    bool ok = std::fwrite(file.data(), 1, file.size(), out) == file.size();
    ok = std::fclose(out) == 0 && ok;
    if (!ok) {
        error = "can't write " + path;
    }
    return ok;
}

} // namespace oracle
