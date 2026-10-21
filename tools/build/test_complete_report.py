"""complete_report.py on a small report.

    python3 -m unittest tools.build.test_complete_report
"""
import unittest

from tools.build.complete_report import complete_report


def unit(name, complete, categories, code, matched, functions, matched_functions, data=0, matched_data=0, fuzzy=99.0):
    return {
        "name": name,
        "measures": {"fuzzy_match_percent": fuzzy, "total_code": str(code), "matched_code": str(matched),
                     "total_data": str(data), "matched_data": str(matched_data), "total_functions": functions,
                     "matched_functions": matched_functions, "complete_code": str(code if complete else 0),
                     "complete_data": str(data if complete else 0), "total_units": 1,
                     "complete_units": 1 if complete else 0},
        "functions": [{"name": "f", "size": str(code), "fuzzy_match_percent": fuzzy}],
        "sections": [{"name": ".text", "size": str(code), "fuzzy_match_percent": fuzzy}],
        "metadata": {"complete": complete, "progress_categories": categories},
    }


class CompleteReportTests(unittest.TestCase):
    def setUp(self):
        self.report = complete_report({
            "measures": {},
            "categories": [{"id": "main", "name": "Main", "measures": {}}, {"id": "lib", "name": "Lib", "measures": {}}],
            "units": [unit("a", True, ["main"], 100, 60, 4, 3, data=40, matched_data=10),
                      unit("b", False, ["main", "lib"], 300, 150, 6, 2, data=60, matched_data=30, fuzzy=50.0)],
        })

    def test_complete_unit_is_matched(self):
        a = self.report["units"][0]
        self.assertEqual(a["measures"]["matched_code"], "100")
        self.assertEqual(a["measures"]["matched_data"], "40")
        self.assertEqual(a["measures"]["matched_functions"], 4)
        self.assertEqual(a["functions"][0]["fuzzy_match_percent"], 100.0)

    def test_incomplete_unit_keeps_objdiff_score(self):
        b = self.report["units"][1]
        self.assertEqual(b["measures"]["matched_code"], "150")
        self.assertEqual(b["functions"][0]["fuzzy_match_percent"], 50.0)

    def test_totals_and_categories_are_recomputed(self):
        m = self.report["measures"]
        self.assertEqual((m["matched_code"], m["matched_functions"], m["complete_units"]), ("250", 6, 1))
        self.assertAlmostEqual(m["matched_code_percent"], 62.5)
        self.assertAlmostEqual(m["fuzzy_match_percent"], (100 * 100 + 50 * 300) / 400)
        main, lib = self.report["categories"]
        self.assertEqual(main["measures"]["matched_code"], "250")
        self.assertEqual(lib["measures"]["matched_code"], "150")


if __name__ == "__main__":
    unittest.main()
