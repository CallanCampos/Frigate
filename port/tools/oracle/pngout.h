// PNG files for the oracle's screens: 8-bit RGB without alpha (the port's --screenshot writes RGBA; compare.py reads
// both), compressed with zlib when the build has it (ORACLE_HAVE_ZLIB) and stored otherwise.
#ifndef PORT_TOOLS_ORACLE_PNGOUT_H
#define PORT_TOOLS_ORACLE_PNGOUT_H

#include <cstdint>
#include <string>

namespace oracle {

// rgb holds width * height pixels, three bytes each (R, G, B), row after row
bool WritePng(const std::string &path, const uint8_t *rgb, uint32_t width, uint32_t height, std::string &error);

} // namespace oracle

#endif // PORT_TOOLS_ORACLE_PNGOUT_H
