"""Check objdiff's per-unit section options without a compiler or ROM.

    python3 -m unittest tools.build.test_objdiff_config
"""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

from tools.build import objdiff_config


class DtcmSectionOptionsTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.arm9 = self.root / "config" / "b2_us" / "arm9"
        self.arm9.mkdir(parents=True)
        patcher = patch.object(objdiff_config, "ROOT", self.root)
        patcher.start()
        self.addCleanup(patcher.stop)

    def delinks(self, text, version="b2_us"):
        path = self.root / "config" / version / "arm9" / "delinks.txt"
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)

    @staticmethod
    def unit(source, options=None):
        unit = {"name": source.removesuffix(".c"), "target_path": "original.o", "base_path": "compiled.o",
                "metadata": {"source_path": source, "complete": True},
                "sections": [{"name": ".dtcm.data", "size": 128}, {"name": ".dtcm.bss", "size": 8}]}
        if options is not None:
            unit["options"] = options
        return unit

    def test_migrated_owner_keeps_sections_and_all_other_unit_fields(self):
        owner = "lib/nitro/src/os/example_irq.c"
        self.delinks("    .data start:0x02000000 end:0x02000100 kind:data align:4\n\n"
                     f"{owner}:\n    complete\n    .dtcm.data start:0x02fe0020 end:0x02fe00a0\n"
                     "    .dtcm.bss start:0x02fe00a0 end:0x02fe00a8\n")
        config = {"units": [self.unit(owner, {"combine_text_sections": False})],
                  "options": {"combine_text_sections": True}}
        expected = copy.deepcopy(config)
        expected["units"][0]["options"]["combine_data_sections"] = False
        objdiff_config.preserve_dtcm_sections(config, "b2_us")
        self.assertEqual(config, expected)

    def test_each_dtcm_spelling_selects_its_actual_owner(self):
        self.delinks("lib/first.c:\n    .dtcm.data start:0x1000 end:0x1080\n\n"
                     "lib/second.c:\n    .dtcm.bss start:0x1080 end:0x1088\n")
        config = {"units": [self.unit("lib/first.c"), self.unit("lib/second.c")]}
        objdiff_config.preserve_dtcm_sections(config, "b2_us")
        self.assertEqual([u["options"] for u in config["units"]], [{"combine_data_sections": False}] * 2)

    def test_other_sources_and_gaps_retain_default_or_explicit_options(self):
        self.delinks("lib/irq.c:\n    .dtcm.data start:0x1000 end:0x1080\n\n"
                     "lib/ordinary.c:\n    .data start:0x2000 end:0x2010\n\n"
                     "lib/ltd.c:\n    .ltdmain.bss start:0x3000 end:0x3010\n\n"
                     "lib/legacy.c:\n    .dtcm start:0x4000 end:0x4010\n")
        config = {"units": [self.unit("lib/irq.c"), self.unit("lib/ordinary.c"),
                            self.unit("lib/ltd.c", {"combine_data_sections": True}), self.unit("lib/legacy.c"),
                            {"name": "_dsd_gap@dtcm_0", "target_path": "gap.o"}]}
        before = copy.deepcopy(config["units"][1:])
        objdiff_config.preserve_dtcm_sections(config, "b2_us")
        self.assertEqual(config["units"][1:], before)
        self.assertEqual(config["units"][0]["options"], {"combine_data_sections": False})

    def test_version_uses_its_own_delinks(self):
        self.delinks("lib/b2.c:\n    .dtcm.data start:0x1000 end:0x1080\n")
        self.delinks("lib/w2.c:\n    .dtcm.bss start:0x2000 end:0x2008\n", version="w2_us")
        config = {"units": [self.unit("lib/b2.c"), self.unit("lib/w2.c")]}
        objdiff_config.preserve_dtcm_sections(config, "w2_us")
        self.assertNotIn("options", config["units"][0])
        self.assertEqual(config["units"][1]["options"], {"combine_data_sections": False})

    def test_main_writes_the_unit_option_without_dropping_sections(self):
        owner = "lib/nitro/src/os/renamed_irq.c"
        self.delinks(f"{owner}:\n    .dtcm.data start:0x1000 end:0x1080\n"
                     "    .dtcm.bss start:0x1080 end:0x1088\n")
        source = self.root / owner
        source.parent.mkdir(parents=True)
        source.write_text("")
        output = self.root / "objdiff.json"
        unit = self.unit(owner)
        command = ["objdiff_config.py", "b2_us", "--dsd=dsd", "--compiler=test", "--c-flags=-O4,p",
                   "-o", str(output)]
        with patch.object(sys, "argv", command), \
                patch.object(objdiff_config.subprocess, "run", return_value=subprocess.CompletedProcess(
                    [], 0, json.dumps({"units": [unit]}), "")):
            objdiff_config.main()
        actual = json.loads(output.read_text())["units"][0]
        self.assertEqual(actual["options"], {"combine_data_sections": False})
        self.assertEqual(actual["sections"], unit["sections"])
        self.assertEqual(actual["target_path"], unit["target_path"])
        self.assertEqual(actual["base_path"], unit["base_path"])
        self.assertEqual(actual["metadata"]["complete"], unit["metadata"]["complete"])


if __name__ == "__main__":
    unittest.main()
