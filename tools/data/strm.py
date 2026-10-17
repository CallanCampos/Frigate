"""Read Nitro sound streams (STRM) of PCM audio in one block, as the title's is, and write them back byte for byte,
and convert them to and from WAV.

A stream's channels are each stored whole, one after the other, in its one block; its WAV interleaves them, as WAV
does, and is 16-bit or 8-bit PCM as the stream is.
"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.swar import WAVE_FORMAT_PCM, wav_chunks  # noqa: E402

PCM8, PCM16 = 0, 1
HEAD = struct.Struct("<BBBBHHIIIIIIII")
HEAD_SIZE = 0x50
DATA_OFFSET = 0x68
WIDTH = {PCM8: 1, PCM16: 2}


class StreamError(Exception):
    pass


def read(strm: bytes) -> dict:
    """Returns a stream's format, loop, rate, timer, loop start and channels' samples, as bytes."""
    if strm[:4] != b"STRM" or strm[0x10:0x14] != b"HEAD" or strm[0x60:0x64] != b"DATA":
        raise StreamError("not a sound stream")
    (fmt, loop, channels, _, rate, timer, loop_start, samples, data_offset, blocks, block_size, block_samples,
     last_size, last_samples) = HEAD.unpack_from(strm, 0x18)
    if fmt not in WIDTH or blocks != 1 or data_offset != DATA_OFFSET or any(strm[0x18 + HEAD.size:0x60]):
        raise StreamError("only a PCM stream in one block can be written again")
    width = WIDTH[fmt]
    if (block_size, block_samples, last_size, last_samples) != (samples * width, samples, samples * width, samples):
        raise StreamError("the stream's block isn't its whole audio")
    data = strm[DATA_OFFSET:]
    if len(data) != channels * samples * width:
        raise StreamError("the stream's data isn't its channels' samples")
    return {"format": fmt, "loop": loop, "rate": rate, "timer": timer, "loop_start": loop_start,
            "channels": [data[index * samples * width:(index + 1) * samples * width] for index in range(channels)]}


def write(stream: dict) -> bytes:
    fmt, channels = stream["format"], stream["channels"]
    width = WIDTH[fmt]
    samples = len(channels[0]) // width
    head = HEAD.pack(fmt, stream["loop"], len(channels), 0, stream["rate"], stream["timer"], stream["loop_start"],
                     samples, DATA_OFFSET, 1, samples * width, samples, samples * width, samples)
    head_block = b"HEAD" + struct.pack("<I", HEAD_SIZE) + head + bytes(HEAD_SIZE - 8 - len(head))
    data = b"".join(channels)
    data_block = b"DATA" + struct.pack("<I", 8 + len(data)) + data
    size = 0x10 + len(head_block) + len(data_block)
    return struct.pack("<4sHHIHH", b"STRM", 0xFEFF, 0x0100, size, 0x10, 2) + head_block + data_block


def to_wav(stream: dict) -> bytes:
    fmt, channels = stream["format"], stream["channels"]
    width = WIDTH[fmt]
    frames = bytearray(len(channels[0]) * len(channels))
    step = width * len(channels)
    for index, channel in enumerate(channels):
        for byte in range(width):
            frames[index * width + byte::step] = channel[byte::width]
    if fmt == PCM8:
        frames = bytes(value ^ 0x80 for value in frames)
    rate = stream["rate"]
    fmt_chunk = struct.pack("<HHIIHH", 1, len(channels), rate, rate * width * len(channels), width * len(channels),
                            8 * width)
    content = b"fmt " + struct.pack("<I", len(fmt_chunk)) + fmt_chunk + b"data" + struct.pack("<I", len(frames)) + frames
    return b"RIFF" + struct.pack("<I", 4 + len(content)) + b"WAVE" + content


def from_wav(wav: bytes) -> tuple[int, int, list[bytes]]:
    """Returns a PCM WAV's format, rate and channels, as a stream stores them."""
    tag, count, rate, _, bits, frames = wav_chunks(wav, StreamError)
    if tag != WAVE_FORMAT_PCM or bits not in (8, 16) or not 1 <= count <= 2:
        raise StreamError("a stream's WAV must be 8-bit or 16-bit PCM, mono or stereo")
    width = bits // 8
    frames = frames[:len(frames) // (width * count) * width * count]
    if width == 1:
        frames = bytes(value ^ 0x80 for value in frames)
    channels = []
    step = width * count
    for index in range(count):
        channel = bytearray(len(frames) // count)
        for byte in range(width):
            channel[byte::width] = frames[index * width + byte::step]
        channels.append(bytes(channel))
    return (PCM8 if width == 1 else PCM16), rate, channels
