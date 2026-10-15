# decomp-permuter, capped

The permuter mutates the source at random and keeps what scores better. Here it is a hint generator for scheduling
and register differences. Its output is never committed as it is: find the plain change it points at. For example,
`GFL_BGSysAllocChar` matched once its tile size, a `u8` from a call, was held in an `int`. Other projects removed
the permuter from their agent loops because its odd output sent the model in circles. Use it in the background, and
time-box it.

## Never uncapped

On 2026-10-04 a `-j8` run on `spl_draw.c` used up 32 GB of RAM plus swap. systemd-oomd then killed the whole
terminal, Claude Code with it, twice. `.claude/hooks/guard.py` refuses `permuter.py` without `systemd-run` and
`MemoryMax`.

## Setup (once per machine)

```sh
git clone https://github.com/simonlindholm/decomp-permuter ../decomp-permuter   # beside ds-decomp, outlives sessions
```

Its Python needs pycparser, toml, capstone, pyelftools and pyyaml, all of which `.venv` has.

## Run

```sh
.venv/bin/python tools/decomp/permuter_setup.py src/X.c F    # writes build/permuter/F/
```

Then, with `run_in_background`:

```sh
systemd-run --user --scope --unit=perm-F -p MemoryMax=6G -p MemorySwapMax=0 .venv/bin/python ../decomp-permuter/permuter.py -j2 build/permuter/F
```

- Don't keep a full log: the progress lines are full of backspaces and once made a 99 KB result. Redirect output to
  a file in the scratchpad and read the end with `tail -c 2000`. The best candidates are in `build/permuter/F/output-*`.
- Work on other functions while it runs. Check it after 10 to 15 minutes, and stop it after about 30 without a new
  best score.
- Stop it with `systemctl --user stop perm-F.scope`. Never `pkill -f`, which matches and kills its own shell; the guard
  refuses it.

## Reading the result

Diff `output-*/source.c` against `base.c` for the function only. Then:

- Find the one or two changes that moved the score: a type, a statement order, a local that was introduced or
  removed, an expression that was split.
- Ask what a programmer would have written that gives the same effect, try it with `try_variants.py`, and keep only
  natural C.
- Reject the noise: unused variables, uninitialized reads, `(void)` casts, address-taken temporaries, a variable
  reused for unrelated values, code moved into the wrong branch, a type that clips a value.

If the only matching candidate is noise, write in the nonmatching row what it changed. "decomp-permuter matched it
only by storing X before Y" is useful to the next attempt.
