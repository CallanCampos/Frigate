# Build details

The [README](../README.md#build) covers the standard build and required ROM filenames and SHA1s.

## Requirements

Use Linux, WSL on Windows, or macOS with Python 3.11+, ninja, and LLVM. Both `clang` and `llvm-objcopy` must be on
your `PATH`; they assemble the game's scripts. Linux also needs a C compiler for the host, as `cc` or that `clang`.

`configure.py` downloads [wibo](https://github.com/decompals/wibo), objdiff, the Metrowerks CodeWarrior tools and
dsd. The Metrowerks tools are Windows programs, run through wibo. To use an existing Wine installation instead:

```sh
python3 configure.py --wine wine
```

### The compiler's floating-point precision

A Win32 process starts with the x87 control word 0x27f (53-bit precision), and wibo leaves Linux's 0x37f (64-bit).
mwccarm lays out some `switch` statements differently between the two (see
[Matching](matching.md#switches)), so on Linux x86-64 `configure.py` compiles `tools/win32_fpu.c` with the
system's C compiler (`cc`, or `clang`) and runs wibo through it: `tools/win32_fpu tools/wibo mwccarm.exe ...` sets
the control word of the process it starts. The decompilation tools (`compiler_probe.py`, `compile_all.py`, the
permuter) use it too once `configure.py` has built it.

On macOS, and with `--wine`, the compiler runs as before and `configure.py` says so. Under wibo alone these
functions come out different, so the ROMs do not match on macOS; whether they do with `--wine` depends on the
control word that program starts mwccarm with, which is untested:

| File | Function |
| --- | --- |
| `src/ov036/enc_pokeset.c` | `SetupEncountFlagsByPartyPokeAbil` |
| `src/ov142/itemmenu_disp.c` | `ItemMenuDisp_IsFieldMoveTM` |
| `src/ov165/plist_sys.c` | `PokeList_GetHidenResult` |
| `src/ov255/box_search.c` | `func_ov255_021d40e8` |
| `src/ov293/demo3d_capture.c` | `Demo3DCapture_End`, `Demo3DCapture_VBlank`, `Demo3DCapture_Begin` |

### macOS

The downloaded macOS wibo build is x86-64. Apple silicon requires Rosetta 2:

```sh
softwareupdate --install-rosetta
```

Homebrew's LLVM is not linked by default. Add its executables to your `PATH`:

```sh
export PATH="$(brew --prefix llvm)/bin:$PATH"
```

## ROMs and verification

Supply your own dumps at `orig/baserom_b2_us.nds` and/or `orig/baserom_w2_us.nds`, matching the SHA1s in the README.
`tools/build/verify_dsi_rom.py` can also check a dump against the digests in its header.

`configure.py` selects every version with an available base ROM, or the versions passed as arguments:

```sh
python3 configure.py b2_us w2_us
ninja
```

`ninja` extracts and delinks each ROM, compiles the sources, links with `mwldarm`, rebuilds the ROM and checks its
SHA1. `ninja progress` prints matching progress.

## dsd toolchain

The build uses DSi hybrid support from the `dsi-hybrid` forks of
[ds-rom](https://github.com/fuddlesworth/ds-rom/tree/dsi-hybrid) and
[ds-decomp](https://github.com/fuddlesworth/ds-decomp/tree/dsi-hybrid).

`configure.py` updates dsd when `DSD_VERSION` changes. `--dsd-from-source` builds that release with Cargo instead
of downloading a binary. To use your own build, pass `--dsd PATH` or copy it to `tools/dsd`; a copy without
`tools/dsd.rev` is never replaced.

## Modified builds

- `python3 configure.py --bugfix` enables fixes guarded by `BUGFIX`. Only files marked complete are linked from C,
  so fixes in incomplete files do not apply yet.
- `python3 configure.py --shift 0x100` moves the code to test changes in code size. See
  [Shifting](configs.md#shifting).

Run `ninja` after configuring. These builds skip the matching checks. Configure again without the option to restore
the standard matching build.
