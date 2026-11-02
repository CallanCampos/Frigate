"""Particle resource tests: res/particles/btlv_effvm_particles packs as the ROM's archive, and edits of an emitter's
fields, a texture's pixels and the emitters there are pack into a resource that the game would read with them. Each
edited resource is read back as SPLManager_LoadResources walks one (spl_walk), apart from nitro_particles.py's tables.
Needs the extracted ROMs (extract/), as the packer does.

    .venv/bin/python -m unittest tools.data.test_nitro_particles
"""
import copy
import json
import shutil
import struct
import tempfile
import unittest
from pathlib import Path

from tools.data import nitro_particles as particles
from tools.data.graphics_archive import pack
from tools.data.narc import read_narc

ROOT = Path(__file__).resolve().parents[2]
ARCHIVE = ROOT / "res" / "particles" / "btlv_effvm_particles"
EXTRACTED = {game: ROOT / "extract" / folder / "files" / "a" / "0" / "0" / "6"
             for game, folder in (("black2", "b2_us"), ("white2", "w2_us"))}
# The sizes of the blocks that may follow a resource header, by the bit of its flags that says it does
BLOCK_SIZES = {8: 12, 9: 12, 10: 8, 11: 12, 16: 20, 24: 8, 25: 8, 26: 16, 27: 4, 28: 8, 29: 16}
HEADER_SIZE = 0x58


def spl_walk(data: bytes) -> tuple[list[dict], list[dict]]:
    """Returns a resource's emitters and textures as SPLManager_LoadResources finds them: each emitter's header
    offset, flags and blocks' offsets by flag bit, and each texture's offset, size, parameters and data's sizes."""
    resource_count, texture_count = struct.unpack_from("<HH", data, 8)
    offset, emitters = 0x20, []
    for _ in range(resource_count):
        flags = struct.unpack_from("<I", data, offset)[0]
        emitter = {"offset": offset, "flags": flags, "blocks": {}}
        offset += HEADER_SIZE
        for bit, size in BLOCK_SIZES.items():
            if flags >> bit & 1:
                emitter["blocks"][bit] = offset
                offset += size
        emitters.append(emitter)
    textures = []
    for _ in range(texture_count):
        param, size, palette_offset, palette_size, _, _, total = struct.unpack_from("<7I", data, offset + 4)
        textures.append({"offset": offset, "width": 1 << ((param >> 4 & 15) + 3),
                         "height": 1 << ((param >> 8 & 15) + 3), "format": param & 15, "size": size,
                         "palette_offset": palette_offset, "palette_size": palette_size, "total": total})
        offset += total
    return emitters, textures


@unittest.skipUnless(all(path.exists() for path in EXTRACTED.values()) and ARCHIVE.is_dir(),
                     "needs extract/ and res/particles/btlv_effvm_particles")
class ParticleTests(unittest.TestCase):
    def setUp(self):
        self.temp = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.temp)
        self.members = read_narc(EXTRACTED["black2"].read_bytes())

    def member(self, test) -> tuple[int, Path, dict]:
        """A copy of the first member whose JSON and original resource pass test: its number, directory and JSON."""
        for number, original in enumerate(self.members):
            source = ARCHIVE / f"{number:03d}"
            document = json.loads((source / particles.JSON_NAME).read_text())
            if test(document, original):
                shutil.copytree(source, self.temp / source.name)
                return number, self.temp / source.name, document
        self.fail("no member for the test")

    def build(self, number: int, directory: Path, document: dict | None = None) -> bytes:
        if document is not None:
            (directory / particles.JSON_NAME).write_text(json.dumps(document))
        return particles.build_directory(directory, self.members[number])

    def test_archive_packs_as_the_rom(self):
        for game, path in EXTRACTED.items():
            pack(ARCHIVE, game, self.temp / game)
            self.assertEqual((self.temp / game).read_bytes(), path.read_bytes(), game)

    def test_emitter_fields(self):
        number, directory, document = self.member(lambda d, _: len(d["emitters"]) >= 2
                                                  and "colorAnim" in d["emitters"][0])
        original = self.members[number]
        emitter = document["emitters"][0]
        emitter["particleLifeTime"] += 7
        emitter["emissionInterval"] = 3
        emitter["color"] = {"red": 1, "green": 2, "blue": 3}
        emitter["colorAnim"]["end"] = {"red": 31, "green": 0, "blue": 16}
        built = self.build(number, directory, document)
        self.assertEqual(len(built), len(original))
        (walked, *_), _ = spl_walk(built)
        header, color_anim = walked["offset"], walked["blocks"][9]
        self.assertEqual(struct.unpack_from("<H", built, header + 0x3E)[0], emitter["particleLifeTime"])
        self.assertEqual(built[header + 0x44], 3)
        self.assertEqual(struct.unpack_from("<H", built, header + 0x22)[0], 1 | 2 << 5 | 3 << 10)
        self.assertEqual(struct.unpack_from("<H", built, color_anim + 2)[0], 31 | 16 << 10)
        changed = {at for at in range(len(built)) if built[at] != original[at]}
        allowed = {header + 0x3E, header + 0x3F, header + 0x44, header + 0x22, header + 0x23, color_anim + 2,
                   color_anim + 3}
        self.assertTrue(changed and changed <= allowed, sorted(changed - allowed))

    def test_texture_pixels(self):
        number, directory, document = self.member(lambda d, data: d["textures"] and d["textures"][0]["format"] == "a5i3"
                                                  and spl_walk(data)[1][0]["palette_size"] == 4)
        original = self.members[number]
        before = spl_walk(original)[1][0]
        path = directory / f"{document['textures'][0]['name']}.png"
        width, height = before["width"], before["height"]
        pixels = bytearray(particles.decode_png(path.read_bytes(), width, height))
        # Three pixels of colors the palette doesn't have, more than its two, which it grows to hold
        colors = [(8, 255, 8), (255, 8, 8), (8, 8, 255)]
        for at, color in enumerate(colors):
            pixels[4 * at:4 * at + 4] = bytes((*color, 255))
        path.write_bytes(particles.write_png(width, height, bytes(pixels)))
        built = self.build(number, directory)
        _, textures = spl_walk(built)
        after = textures[0]
        texels = built[after["offset"] + 0x20:after["offset"] + 0x20 + after["size"]]
        palette = built[after["offset"] + after["palette_offset"]:after["offset"] + after["total"]]
        for at, (red, green, blue) in enumerate(colors):
            index, alpha = texels[at] & 7, texels[at] >> 3
            self.assertEqual((struct.unpack_from("<H", palette, 2 * index)[0], alpha),
                             (round(red * 31 / 255) | round(green * 31 / 255) << 5 | round(blue * 31 / 255) << 10, 31))
        self.assertGreater(after["palette_size"], before["palette_size"])
        texture = {"format": 6, "width": width, "height": height, "texels": texels, "palette": palette}
        decoded = particles.texture_pixels(texture, True)
        view = {"name": "test", "kind": 6, "params": 0}
        self.assertEqual(particles.visible(decoded, view), particles.visible(bytes(pixels), view))
        # The other textures are the game's bytes
        later = before["offset"] + before["total"]
        self.assertEqual(built[after["offset"] + after["total"]:], original[later:])

    def test_emitter_added_and_removed(self):
        number, directory, document = self.member(lambda d, _: len(d["emitters"]) >= 2)
        original = self.members[number]
        emitters, textures = spl_walk(original)
        spans = [(emitter["offset"], following["offset"]) for emitter, following in zip(emitters, emitters[1:])]
        spans.append((emitters[-1]["offset"], textures[0]["offset"] if textures else len(original)))
        texture_data = original[spans[-1][1]:]

        added = copy.deepcopy(document)
        added["emitters"].append(copy.deepcopy(document["emitters"][0]))
        added["emitters"][-1]["particleLifeTime"] = 99
        built = self.build(number, directory, added)
        walked, _ = spl_walk(built)
        self.assertEqual(len(walked), len(emitters) + 1)
        new = walked[-1]["offset"]
        copied = bytearray(original[spans[0][0]:spans[0][1]])
        struct.pack_into("<H", copied, 0x3E, 99)
        self.assertEqual(built[new:new + len(copied)], bytes(copied))
        self.assertEqual(built[:new][0x20:], original[0x20:spans[-1][1]])
        self.assertEqual(built[new + len(copied):], texture_data)
        self.assertEqual(struct.unpack_from("<I", built, 0x18)[0], new + len(copied))

        removed = copy.deepcopy(document)
        del removed["emitters"][0]
        built = self.build(number, directory, removed)
        walked, _ = spl_walk(built)
        self.assertEqual(len(walked), len(emitters) - 1)
        self.assertEqual(built[0x20:], original[spans[1][0]:])
        self.assertEqual(struct.unpack_from("<HH", built, 8), (len(emitters) - 1, len(textures)))


if __name__ == "__main__":
    unittest.main()
