# 0015: A universal macOS app for macOS 11, with the pinned SDL3 linked statically

- **Status:** accepted
- **Date:** 2026-10-18
- **Plan:** D11, §7 phase 5 (an ad-hoc signed `.app` in a DMG), after
  [ADR 0003](0003-ad-hoc-signing-no-apple-developer-account.md) and
  [ADR 0010](0010-macos-app-as-a-signed-copy-of-the-program.md)
- **Upstream:** `993e6c49b` (merged by the port's `1f0c184f6`)
- **Note (2026-10-18):** accepted at the macOS distribution track's merge into `port-integrate` (`4122413bf`) by the
  port's developer, as every earlier ADR was: the deployment target, macOS 11, and the architectures, arm64 and x86_64
  in one app, are the developer's call. The branch's first commits numbered it 0013, which the Windows track's ADR
  took; it is 0015 everywhere in the tree.
- **Note (2026-10-18):** CI's `macos-distribution` job first ran at the merge (run 39191368934): green on `macos-26`,
  `macos-26-intel`, `macos-15` and `macos-15-intel` once a host test took the byte an Intel Mac's `memset` touches
  first; macOS 15 is tested there without the ROM, and 11 to 14 still aren't.

## Context

The app ADR 0010 made needs macOS 26 on Apple Silicon, for two reasons. CMake sets no deployment target, so Apple clang
builds the program for the SDK's macOS (minos 26.0 with Xcode 26); and Homebrew's SDL3 3.4.18, which `make_app.sh`
copies into the app, is arm64 only and built for macOS 26. `LSMinimumSystemVersion` said 26.0, and no Intel Mac and no
older macOS could open it. ADR 0010's consequences left "an app for older Macs" untried.

What was measured on 2026-10-18 (macOS 26.5.1 on an M2, Xcode 26.6 with SDK 26.5, Apple clang 21.0.0, CMake 4.4.4,
Rosetta installed; the STATUS.md entry of that day has the runs):

- **The floors.** The SDK's `SDKSettings.json` gives a minimum deployment target of 10.13 and a recommended one of 11.0.
  arm64 has none below 11.0 (clang raises a lower one to 11.0).
- **The port's and the game's code** call only long-standing APIs (`mach_vm_map`, `mach_vm_remap`,
  `mach_make_memory_entry_64`, `shm_open`, `mprotect`, `sigaltstack`, `backtrace`, `dladdr`, `dlsym`,
  `getsegmentdata`, `_NSGetExecutablePath`), have no Objective-C, and reach AppKit, Metal and GameController only
  through SDL3. Compiled for both architectures at 11.0 with `-Werror=unguarded-availability`, and for x86_64 at 10.13,
  the 1,585 files give no availability diagnostic. minicoro switches fibers with assembly for arm64 and x86-64 System
  V, and the stack probes work on x86-64 (`os_guard_huge_frame`).
- **SDL3 3.4.18** deploys to "macOS 10.13 and newer" (its `docs/README-macos.md`) and guards its newer calls itself:
  its 249 sources, compiled again for each architecture at 11.0 with only the availability warnings on, give none (a
  call of macOS 12's `isLowPowerModeEnabled` in a test file does). Built statically for 11.0 it brings frameworks that
  macOS 11 has, UniformTypeIdentifiers and CoreHaptics weakly.
- **The link changes with the target**, which `port/tools/gen_overlay_sections.py` must know, since it predicts the
  program's sections before the link (Mach-O holds at most 255): ld64 links a program for chained fixups only from macOS
  12.0 on arm64 and 13.0 on x86_64, and below that binds lazily (`__stub_helper`, `__la_symbol_ptr`, a pointer of
  dyld's in `__data`, constructors in `__DATA_CONST,__mod_init_func`); below 10.15 there is no `__DATA_CONST`. A static
  SDL3 brings Objective-C: ld64 moves its lists and constant strings to `__DATA_CONST`, and on arm64 makes
  `__objc_stubs` and `__objc_methlist`. Each was measured with a small program for both architectures, from 10.13 to
  26.
- **x86_64 Mach-O caps a section's alignment** at its 4 KiB page: the 16 KiB-aligned DS memory windows
  (`port/hal/hw/hw.c`) and trap paddings (`port/tools/gen_stubs.py`) got 20 linker warnings and a 4 KiB alignment, and
  `test_gx.c`'s check of the views' 16 KiB alignment failed in 9 of 12 runs. The game needs only the host's page.
- **Both slices run the game the same.** 900 headless frames from power-on with `--fixed-time`, under each
  architecture (the x86_64 slice under Rosetta) and each of the four VRAM backends, gave eight byte-identical
  `frames.tsv`, with 153 distinct top screens.
  With the executables thinned to x86_64, the whole test suite runs under Rosetta, the oracle's comparisons with
  melonDS included.
- **Other ports** ship without notarization: Ship of Harkinian 9.3.0 and 2Ship2Harkinian 5.0.1 as universal apps for
  10.15, Zelda64Recompiled 1.2.2 universal for 11.0, sm64coopdx v1.5.1 as separate ARM (11) and Intel (10.15) builds.
  Those with a real minimum build or install SDL for their target rather than take Homebrew's.
- **Apple** says macOS 26 is the last release for Intel Macs, and keeps Rosetta whole through macOS 27.
- Nothing older than this Mac's macOS 26 can be run on it: there is no virtual machine. GitHub's macOS 15 runners,
  `macos-15` (Apple Silicon) and `macos-15-intel`, aren't deprecated (runner-images' README of 2026-10-18), run macOS
  15.7.9 and have Xcode 26.0.1 to 26.3 beside their default 16.4, and their arm64 image installs Rosetta as the macOS
  26 one does. `macos-14` is deprecated, and its image can't have Xcode 26, which needs macOS 15.6.

## Decision

- **A distribution build** for other Macs than the one that builds it: `-DPORT_MACOS_DISTRIBUTION=ON`, in a build
  directory of its own (`build/port-dist`; `port/tools/package_macos.sh --distribution` configures it, builds it and
  makes the disk image in one command). Before `project()` it sets `CMAKE_OSX_ARCHITECTURES` to `arm64;x86_64` and
  `CMAKE_OSX_DEPLOYMENT_TARGET` to 11.0 when the cache has neither (a directory configured with it refuses to be
  configured without it, since both stay in its cache), and it always builds SDL3's pinned release,
  statically, for the same target and architectures, never an installed SDL3, and without SDL3's libusb support, which
  otherwise opens Homebrew's libusb at run time when the builder's Mac has it (the app loads nothing from outside
  macOS, as ADR 0010 has it). A development build is unchanged: built for the machine and the SDK, with Homebrew's
  SDL3 when there is one.
- **One universal program**, arm64 and x86_64, in one app: Intel Macs run the x86_64 slice natively, and one disk image
  serves every Mac.
- **macOS 11.0 for both slices**: arm64's floor and the SDK's recommendation, every Apple Silicon Mac and the Intel Macs
  that run Big Sur.
- **An API newer than the target fails the build**: every target the port defines, and SDL3 when it is built along,
  gets `-Werror=unguarded-availability` whenever a deployment target is set (`port/CMakeLists.txt`). SDL3 guards its
  newer calls itself, and the check holds each pinned release to it. Warnings, this one included, are off in system
  headers, so minicoro's implementation, all in its header, is compiled as a header of its own then
  (`port/cmake/os.cmake`); the decomp's headers, system headers too, call nothing of macOS.
- **The tools follow the slices**: `gen_overlay_sections.py` predicts and checks each slice, with the link format of
  its architecture and macOS (taken from the program's own objects), and compares each slice's sections by name;
  `make_app.sh` reads every slice (`otool -arch all`), takes `--archs` and `--minimum` from the build, refuses a binary
  without a slice or with one for a newer macOS, writes the minimum into `LSMinimumSystemVersion`, and starts the
  program as each slice the Mac runs; `check_struct_layout.py` reads a universal object slice by slice; the host
  seam's `PORT_HOST_PAGE_ALIGNMENT` (`port/hal/host/host.h`, [ADR 0013](0013-windows-build-with-msys2-clang64.md)),
  which `hw.c`'s memory windows, `gen_stubs.py`'s paddings and `test_gx.c` take, is 4 KiB in an x86_64 Mach-O program,
  as on Windows, and 16 KiB elsewhere.
- **The checks**: `port/app/macos/check_app.sh` prints what each binary's load commands say, slice by slice, and fails
  on a slice missing, another platform or macOS, a library from outside the app, a signature that isn't ad hoc or has
  the hardened runtime, or a slice that doesn't start. The test `app_slices` runs it on the program of every macOS
  build; `app_universal_slices` runs both slices of a universal program for 900 frames and requires the same frames
  (skipped without Rosetta or the ROM). CI's `macos-distribution` job builds and tests the distribution build on
  macOS 26 and 15, Apple Silicon and Intel each (`macos-26`, `macos-26-intel`, `macos-15` and `macos-15-intel`, on 15
  with its newest Xcode 26), where ctest runs the tests that don't need the ROM (on x86_64 natively on Intel), makes the
  app and its disk image with `package_macos.sh --distribution`, and runs `check_app.sh` on the app, every slice
  started on Apple Silicon, after checking that Rosetta is there. `port/tools/ctest_one_slice.sh` runs a universal
  build's whole suite as one slice, the x86_64 one under Rosetta on Apple Silicon.
- **What the documents claim** is what was run: built for macOS 11 and later, tested on macOS 26 on Apple Silicon, the
  x86_64 slice under Rosetta. CI's job is set up to run it on macOS 15 and 26, on Apple Silicon and Intel; the
  documents say it has run there only once it has. Nothing below macOS 26 has run it yet.
- **Nothing changes in what is distributed.** The app is still made locally, and CI uploads nothing
  (port/README.md); the distribution build makes an app that other Macs can open, and whether Frigate ever offers
  downloads stays the user's decision. A downloaded app opened from the Finder finds no ROM, so a first-launch ROM
  picker (ADR 0010's alternative) would come before any download.

## Alternatives

- **Homebrew's SDL3 in the app** (ADR 0010's way): arm64 only and built for the macOS Homebrew builds on (26).
- **arm64 only**: loses every Intel Mac, which macOS still supports through 26.
- **Two apps, one an architecture** (sm64coopdx): two disk images and two builds to keep apart, for nothing a universal
  app lacks; its only gain, a smaller download, is about 15 MB.
- **macOS 12, 13 or 14**: no less work (any universal build below 13 binds lazily on x86_64, so the section check must
  model that anyway) and fewer Macs. 14 would claim only what a CI runner can run, but that runner is deprecated.
- **10.15 or 10.13 for the x86_64 slice alone** (Ship of Harkinian's 10.15): a minimum per architecture, two
  `LSMinimumSystemVersion` stories and the old `__DATA` layout to model below 10.15, for Intel Macs that stopped at
  Catalina or earlier.
- **SDL3 as a dynamic library in `Contents/Frameworks`**: works with `make_app.sh` as it is, but adds an install name
  and a second binary to check, sign and keep at the same target; statically, the program is the app's only binary.
- **CMake presets**: `cmake --workflow --preset` runs only from `port/`, and the one command also has to make the disk
  image, which `package_macos.sh` already does.

## Consequences

- The distribution build compiles everything twice, and SDL3 too: the universal program is 29 MB (14 MB arm64, 15 MB
  x86_64), its disk image 13 MB, and the build directory about 700 MB against a development build's 230 MB.
- A new Xcode that links differently fails `overlay_sections_image_*`, which names the sections it didn't predict;
  `gen_overlay_sections.py`'s rules say where each came from.
- Locally, Homebrew's `cmake`, `ctest`, `ninja` and the `.venv` Python are arm64 only, so ctest on a universal tree runs
  the arm64 slices; the x86_64 slice runs through `app_slices` (`--help`), `app_universal_slices` and `make_app.sh`,
  through the whole suite with `port/tools/ctest_one_slice.sh --arch x86_64`, which thins the executables for the run,
  and on CI's Intel runners natively. `asm_oracle` loads its library into the arm64 Python, so it checks the arm64 code.
- macOS 11 to 14 are untested, and 15 is tested only once CI's job has run. The availability check covers the port's
  code, minicoro's and SDL3's, so a new pinned SDL3 that calls a newer API unguarded fails the build; what stays
  outside it is the system headers, macOS's own and the decomp's.
- The distribution app holds the builder's paths: the checkout's, which the program keeps to find the ROM of `orig/`
  (`port/hal/rom`), and every object's in the debug map of each slice, which name the builder's home folder. ADR 0010's
  caveat stands: a build meant for other people is made from a neutral path, and stripped, before any public download,
  which stays the user's decision.
- Revisit when Apple drops Rosetta (after macOS 27; then the x86_64 slice can be tested only on Intel), when Xcode no
  longer builds for 11.0, or when the user decides about downloads.
