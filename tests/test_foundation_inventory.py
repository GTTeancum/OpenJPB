import unittest

from tools.foundation_inventory import build_inventory


class FoundationInventoryTests(unittest.TestCase):
    def test_reviewed_fmath_counts_all_symbols(self):
        functions = [
            {
                "module": "fmath",
                "name": "FindSinCos",
                "size": 114,
                "locals": [],
                "line_ranges": [{}],
            },
            {
                "module": "fmath",
                "name": "PerspectiveTransform",
                "size": 318,
                "locals": [],
                "line_ranges": [{}],
            },
        ]

        items = build_inventory(functions, [])
        fmath = next(item for item in items if item["module"] == "fmath")

        self.assertEqual(fmath["status"], "reviewed")
        self.assertEqual(fmath["procedure_count"], 2)
        self.assertEqual(fmath["procedure_bytes"], 432)
        self.assertEqual(fmath["reviewed_procedure_count"], 2)
        self.assertEqual(fmath["reviewed_procedure_bytes"], 432)

    def test_reviewed_module_counts_all_symbols(self):
        functions = [
            {
                "module": "list",
                "name": "list_InitList",
                "size": 3,
                "locals": [],
                "line_ranges": [],
            }
        ]

        items = build_inventory(functions, [])
        list_item = next(item for item in items if item["module"] == "list")

        self.assertEqual(list_item["reviewed_procedure_count"], 1)
        self.assertEqual(list_item["reviewed_procedure_bytes"], 3)

    def test_reviewed_flex_counts_all_symbols(self):
        functions = [
            {
                "module": "flex",
                "name": "CROSS",
                "size": 112,
                "locals": [],
                "line_ranges": [{}],
            },
            {
                "module": "flex",
                "name": "vecsub",
                "size": 40,
                "locals": [],
                "line_ranges": [{}],
            },
        ]

        items = build_inventory(functions, [])
        flex = next(item for item in items if item["module"] == "flex")

        self.assertEqual(flex["status"], "reviewed")
        self.assertEqual(flex["reviewed_procedure_count"], 2)
        self.assertEqual(flex["reviewed_procedure_bytes"], 152)


if __name__ == "__main__":
    unittest.main()
