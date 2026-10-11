Test inputs for `tools/decomp/compiler_probe.py`:

- `main_loops.c` holds six ARM9 main functions that match with every compiler version.
- `corpus.c` covers common C constructs, for comparing the compiler versions against each other. Only
  `t_local_array` differs, and only between `dsi/1.3p1` and `dsi/1.6`.
