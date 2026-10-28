# Test source tools

| File | Actual owner and contract | Test |
| --- | --- | --- |
| [test_project_tr_ai_tables.py](test_project_tr_ai_tables.py) | [../../tools/project_tr_ai_tables.py](../../tools/project_tr_ai_tables.py) selects exact definitions/types from `src/ov170/tr_ai.c` for the actual-command/VM regression. Tests original-source byte retention, propagation of edits, comment/string/character braces, rejected missing/duplicate/malformed boundaries and preservation of an existing output on failure. | `integrate_tr_ai_projection` |

Run with `PYTHONDONTWRITEBYTECODE=1 python3 port/tests/tools/test_project_tr_ai_tables.py`, or the registered CTest.
No ROM, extracted data, game build or platform linker is needed. The companion native test compiles the generated
source with the normal SDK headers and fatal sanitizer policy; these tests do not replace that compile/runtime check.
