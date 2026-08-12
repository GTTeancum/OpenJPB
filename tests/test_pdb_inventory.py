import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from tools.pdb_inventory import (
    Section,
    is_core_object,
    merge_data_symbols,
    parse_global_data,
    parse_line_ranges,
    parse_symbols,
    write_scaffold,
)


class InventoryParserTests(unittest.TestCase):
    def setUp(self):
        self.sections = {
            1: Section(
                index=1,
                name=".text",
                virtual_address=0x1000,
                virtual_size=0x2000,
                raw_address=0x400,
                raw_size=0x2000,
            )
        }

    def test_core_classifier_excludes_resource_compiland(self):
        root = r"W:\SWJediPowerBattles\winver\obj\x64\Steam_Release"
        self.assertTrue(is_core_object(root + r"\player.obj"))
        self.assertFalse(is_core_object(root + r"\winver.res"))

    def test_symbol_offsets_are_decimal_and_become_rvas(self):
        text = r"""
Mod 0064 | `W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\player.obj`:
    76 | S_COMPILE3 [size = 60]
         machine = intel x86-x64, Ver = compiler, language = c
   100 | S_GPROC32 [size = 56] `jedi_InitPlayer`
         parent = 0, end = 200, addr = 0001:729632, code size = 559
         type = `0x1298 (int (playerObject*))`, flags = opt debuginfo
   156 | S_LOCAL [size = 20] `player`
         type=0x10E4 (playerObject*), flags = param
"""
        modules, functions, globals_ = parse_symbols(
            text, self.sections, 0x140000000
        )
        self.assertEqual(len(modules), 1)
        self.assertEqual(len(functions), 1)
        self.assertEqual(globals_, [])
        function = functions[0]
        self.assertEqual(function["section_offset"], 729632)
        self.assertEqual(function["rva"], 0x1000 + 729632)
        self.assertEqual(function["signature"], "int (playerObject*)")
        self.assertEqual(function["locals"][0]["name"], "player")

    def test_data_symbol_offsets_are_decimal_and_become_rvas(self):
        text = r"""
Mod 0087 | `W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\timer.obj`:
    76 | S_COMPILE3 [size = 60]
         machine = intel x86-x64, Ver = compiler, language = c
   100 | S_GDATA32 [size = 28] `mRoundTimer`
         type = 0x0012 (long), addr = 0003:4863836
"""
        sections = {
            3: Section(
                index=3,
                name=".data",
                virtual_address=0x4A1000,
                virtual_size=0xC4F000,
                raw_address=0x49F600,
                raw_size=0x3B400,
            )
        }
        modules, functions, globals_ = parse_symbols(
            text, sections, 0x140000000
        )
        self.assertEqual(len(modules), 1)
        self.assertEqual(functions, [])
        self.assertEqual(len(globals_), 1)
        self.assertEqual(globals_[0]["name"], "mRoundTimer")
        self.assertEqual(globals_[0]["section_offset"], 4863836)
        self.assertEqual(globals_[0]["rva"], 0x94475C)
        self.assertEqual(globals_[0]["type"], "long")

    def test_global_stream_data_is_parsed_and_module_evidence_wins(self):
        text = r"""
    800560 | S_GDATA32 [size = 28] `mRoundTimer`
             type = 0x0012 (long), addr = 0003:4863836
"""
        sections = {
            3: Section(
                index=3,
                name=".data",
                virtual_address=0x4A1000,
                virtual_size=0xC4F000,
                raw_address=0x49F600,
                raw_size=0x3B400,
            )
        }
        linked = parse_global_data(text, sections, 0x140000000)
        self.assertEqual(len(linked), 1)
        self.assertEqual(linked[0]["origin"], "global_stream")
        self.assertEqual(linked[0]["rva"], 0x94475C)

        owned = {
            **linked[0],
            "origin": "module_symbols",
            "module_id": 87,
            "module": "timer",
        }
        merged = merge_data_symbols(linked, [owned])
        self.assertEqual(len(merged), 1)
        self.assertEqual(merged[0]["origin"], "module_symbols")
        self.assertEqual(merged[0]["module"], "timer")

    def test_line_offsets_are_hexadecimal(self):
        text = r"""
Mod 0064 | `W:\SWJediPowerBattles\winver\obj\x64\Steam_Release\player.obj`:
W:\SWJediPowerBattles\Work\player.c (SHA-256: 4F8FA55C30B1163C0F6256FD1FF97830B95360C3A308D4A18EA3DD5082C86F51)
  0001:000E5660-000E5B0B, line/addr entries = 49
"""
        ranges = parse_line_ranges(text, self.sections)
        self.assertEqual(len(ranges), 1)
        self.assertEqual(ranges[0]["start_rva"], 0xE6660)
        self.assertEqual(ranges[0]["end_rva"], 0xE6B0B)

    def test_scaffold_generation_preserves_reviewed_source(self):
        module = {
            "id": 50,
            "object_path": (
                r"W:\SWJediPowerBattles\winver\obj\x64"
                r"\Steam_Release\list.obj"
            ),
            "stem": "list",
            "language": "c",
            "primary_source": r"W:\SWJediPowerBattles\Work\list.c",
        }
        function = {
            "module_id": 50,
            "rva": 0xBBBB0,
            "size": 29,
            "linkage": "global",
            "locals": [],
            "signature": "Node* (List*, Node*)",
            "name": "list_AddHead",
            "source_path": r"W:\SWJediPowerBattles\Work\list.c",
        }

        with TemporaryDirectory() as directory:
            root = Path(directory)
            reviewed = root / "original" / "list.c"
            reviewed.parent.mkdir(parents=True)
            reviewed.write_text("/* reviewed */\n", encoding="utf-8")

            write_scaffold(root, [module], [function])

            self.assertEqual(
                reviewed.read_text(encoding="utf-8"), "/* reviewed */\n"
            )


if __name__ == "__main__":
    unittest.main()
