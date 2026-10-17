"""Read Nitro wave archives (SWAR) into their waves, write them back byte for byte, and convert waves to and from WAV.

A wave is PCM8, PCM16 or IMA-ADPCM. Its WAV keeps its own encoding, so nothing is lost: an 8-bit or 16-bit PCM WAV, or
an IMA ADPCM WAV whose blocks hold the wave's ADPCM data, in one block, or several where the wave is larger than a
WAV block can be, each block's header holding the decoder's state at its start. A PCM WAV edited in any audio editor
packs as the PCM wave it is.
"""
import struct

PCM8, PCM16, ADPCM = 0, 1, 2
INFO = struct.Struct("<BBHHHI")
TIMER_CLOCK = 16756991
WAVE_FORMAT_PCM, WAVE_FORMAT_IMA_ADPCM = 1, 0x11
# The most ADPCM bytes a WAV block holds after its header: its samples a block are a 16-bit count
MAX_BLOCK_BYTES = 0x7FFC
STEPS = [7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97,
         107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
         876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871,
         5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385,
         24623, 27086, 29794, 32767]
INDEX_CHANGES = [-1, -1, -1, -1, 2, 4, 6, 8]


class WaveError(Exception):
    pass


def read(swar: bytes) -> list[dict]:
    """Returns an archive's waves: format, loop, rate, timer, loop_start and loop_length (in 4-byte words) and data."""
    if swar[:4] != b"SWAR" or swar[16:20] != b"DATA":
        raise WaveError("not a wave archive")
    count = struct.unpack_from("<I", swar, 0x38)[0]
    offsets = list(struct.unpack_from(f"<{count}I", swar, 0x3C))
    ends = offsets[1:] + [len(swar)]
    waves = []
    for offset, end in zip(offsets, ends):
        fmt, loop, rate, timer, start, length = INFO.unpack_from(swar, offset)
        data = swar[offset + INFO.size:end]
        if (start + length) * 4 != len(data):
            raise WaveError("a wave's data isn't its loop's end")
        waves.append({"format": fmt, "loop": loop, "rate": rate, "timer": timer, "loop_start": start,
                      "loop_length": length, "data": data})
    return waves


def write(waves: list[dict]) -> bytes:
    """Returns the archive of waves."""
    header = 0x3C + 4 * len(waves)
    offsets, body = [], bytearray()
    for wave in waves:
        offsets.append(header + len(body))
        body += INFO.pack(wave["format"], wave["loop"], wave["rate"], wave["timer"], wave["loop_start"],
                          wave["loop_length"]) + wave["data"]
    data = bytes(32) + struct.pack(f"<I{len(waves)}I", len(waves), *offsets) + body
    block = struct.pack("<4sI", b"DATA", 8 + len(data)) + data
    return struct.pack("<4sHHIHH", b"SWAR", 0xFEFF, 0x0100, 0x10 + len(block), 0x10, 1) + block


def adpcm_state(data: bytes, nibbles: int) -> tuple[int, int]:
    """Returns the ADPCM decoder's sample and step index after a number of a wave's nibbles."""
    sample, index = struct.unpack_from("<hB", data, 0)
    for position in range(nibbles):
        nibble = data[4 + position // 2] >> (4 * (position % 2)) & 15
        step = STEPS[index]
        delta = step >> 3
        if nibble & 1:
            delta += step >> 2
        if nibble & 2:
            delta += step >> 1
        if nibble & 4:
            delta += step
        sample = max(-32768, min(32767, sample - delta if nibble & 8 else sample + delta))
        index = max(0, min(88, index + INDEX_CHANGES[nibble & 7]))
    return sample, index


def chunk(kind: bytes, data: bytes) -> bytes:
    return kind + struct.pack("<I", len(data)) + data + bytes(len(data) % 2)


def to_wav(wave: dict) -> bytes:
    """Returns a wave as a WAV file of its own encoding."""
    data, rate = wave["data"], wave["rate"]
    if wave["format"] == ADPCM:
        body = data[4:]
        block_bytes = min(len(body), MAX_BLOCK_BYTES)
        blocks = bytearray()
        for start in range(0, len(body), block_bytes):
            sample, index = adpcm_state(data, start * 2)
            blocks += struct.pack("<hBB", sample, index, 0) + body[start:start + block_bytes]
        block_align = block_bytes + 4
        samples_per_block = 1 + block_bytes * 2
        fmt = struct.pack("<HHIIHHHH", WAVE_FORMAT_IMA_ADPCM, 1, rate, rate * block_align // samples_per_block,
                          block_align, 4, 2, samples_per_block)
        content = (chunk(b"fmt ", fmt) + chunk(b"fact", struct.pack("<I", len(body) * 2 + 1)) +
                   chunk(b"data", bytes(blocks)))
    elif wave["format"] == PCM16:
        content = chunk(b"fmt ", struct.pack("<HHIIHH", WAVE_FORMAT_PCM, 1, rate, rate * 2, 2, 16)) + chunk(b"data", data)
    elif wave["format"] == PCM8:
        content = (chunk(b"fmt ", struct.pack("<HHIIHH", WAVE_FORMAT_PCM, 1, rate, rate, 1, 8))
                   + chunk(b"data", bytes(value ^ 0x80 for value in data)))
    else:
        raise WaveError(f"unknown wave format {wave['format']}")
    return b"RIFF" + struct.pack("<I", 4 + len(content)) + b"WAVE" + content


def from_wav(wav: bytes) -> tuple[int, int, bytes]:
    """Returns a WAV's wave format, rate and data, as a wave stores them."""
    if wav[:4] != b"RIFF" or wav[8:12] != b"WAVE":
        raise WaveError("not a WAV file")
    chunks, position = {}, 12
    while position + 8 <= len(wav):
        kind, size = wav[position:position + 4], struct.unpack_from("<I", wav, position + 4)[0]
        chunks.setdefault(kind, wav[position + 8:position + 8 + size])
        position += 8 + size + size % 2
    if b"fmt " not in chunks or b"data" not in chunks:
        raise WaveError("a WAV file needs a fmt and a data chunk")
    tag, channels, rate, _, block_align, bits = struct.unpack_from("<HHIIHH", chunks[b"fmt "])
    data = chunks[b"data"]
    if channels != 1:
        raise WaveError("a wave must be mono")
    if tag == WAVE_FORMAT_PCM and bits == 8:
        return PCM8, rate, bytes(value ^ 0x80 for value in data)
    if tag == WAVE_FORMAT_PCM and bits == 16:
        return PCM16, rate, data[:len(data) // 2 * 2]
    if tag == WAVE_FORMAT_IMA_ADPCM and bits == 4:
        blocks = [data[start:start + block_align] for start in range(0, len(data), block_align)]
        joined = bytearray(blocks[0])
        for block in blocks[1:]:
            if struct.unpack_from("<hB", block) != adpcm_state(bytes(joined), (len(joined) - 4) * 2):
                raise WaveError("an IMA ADPCM WAV's blocks don't continue each other, as a wave's must; save it as "
                                "16-bit PCM instead")
            joined += block[4:]
        return ADPCM, rate, bytes(joined)
    raise WaveError("a wave's WAV must be 8-bit or 16-bit PCM, or IMA ADPCM")
