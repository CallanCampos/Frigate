# 0010: The macOS app as a signed copy of the program

- **Status:** accepted
- **Date:** 2026-10-17
- **Plan:** D11, §7 phase 5 (an ad-hoc signed `.app` in a DMG), after
  [ADR 0003](0003-ad-hoc-signing-no-apple-developer-account.md)
- **Upstream:** `615939e25`
- **Note (2026-10-18):** the port is now Frigate and the names below are those of the time: the program is
  `frigate-black2`, the app `Frigate Black 2.app` with the bundle identifier `io.github.callancampos.frigate.black2`,
  the ROM's variable `FRIGATE_ROM` and the log `~/Library/Logs/Frigate/<program>.log` (port/CLAUDE.md).
- **Note (2026-10-18):** the app for older Macs that its consequences leave untried is the distribution build of
  [ADR 0015](0015-universal-macos-app-for-macos-11.md): one universal app, arm64 and x86_64, for macOS 11 and later.

## Context

Phase 5 asks for the program as a macOS app, signed ad hoc, in a disk image, with no ROM or game data in it. The
program the host build links (`build/port/pokeblack2`) is what every test, replay and oracle script runs by that path,
and it loads Homebrew's SDL3 (`/opt/homebrew/opt/sdl3/lib/libSDL3.0.dylib`, built for macOS 26 on the machine this was
written on) through an absolute install name and an rpath to `/opt/homebrew/lib`. CMake builds the pinned SDL3
statically only when no SDL3 3.4 is installed. The player's ROM is found from `--rom`, `POKEBW2_ROM` or the checkout's
`orig/` (`port/hal/rom`), and a Finder launch passes no arguments, no environment of the shell and `/` as the working
directory (tried on macOS 26.5 with a probe app: no `-psn_` argument either), with stdout and stderr on `/dev/null`
(an `open` launch of the app with `HOME` on a scratch directory wrote its log under that directory, which it does
only with stderr on `/dev/null`).

## Decision

- **A separate target copies the program into the app** (`<program>_app`, `port/cmake/macos_app.cmake`, run by
  `port/app/macos/make_app.sh`), instead of making the program target itself a bundle. The program stays where the
  tests and tools expect it, and the app is made only when asked for (`-DPORT_MACOS_APP=ON` makes it every build).
- **The libraries the program loads from outside macOS go in `Contents/Frameworks`** with `@rpath` install names,
  however the program names them: by an absolute path (Homebrew's), or by `@rpath`, `@loader_path` or
  `@executable_path` (SDL3's own CMake install names it `@rpath/libSDL3.0.dylib`), each found where dyld would find it,
  in the binary's search paths and then the program's; the libraries they load go in the same way. Every search path
  outside macOS is replaced by `@executable_path/../Frameworks` in the program and `@loader_path` in the libraries, so
  the app never loads another copy of SDL3 from the machine. Before signing, the script checks that every binary of the
  app loads only macOS's libraries and the app's own; after, it starts the program from the app (`--help`), so an app
  that dyld can't load fails the build instead of the player's launch. `LSMinimumSystemVersion` is the newest `minos`
  among the binaries, so an older macOS refuses the app with a message instead of failing to load SDL3.
- **Signed ad hoc as a whole** with `codesign --force --deep --sign -` after the last change to any binary, checked
  with `codesign --verify --deep --strict`, without the hardened runtime (ADR 0003). The app is built next to its path
  and moved there only once it verifies.
- **The ROM is found as the program finds it.** A self-built app opens from the Finder with the checkout's `orig/`
  ROM; otherwise `open -n -a … --args --rom PATH` or `open -n --env POKEBW2_ROM=…` gives it (port/README.md).
- **An app opened from the Finder logs to a file.** LaunchServices starts the program with stderr on `/dev/null`, where
  every message, the reason it stops included, was lost. When the program runs from an app and stderr goes nowhere, it
  moves stderr and stdout to `~/Library/Logs/pokebw2-native/<program>.log` (`port/app/message_log.c`), which Console
  lists; for a stop the player can act on, no ROM or a ROM the port refuses, it also shows the reason in an alert
  (`port_app_fatal_notice`). The alert is shown only from the host's own stack: the other stops (a save file of the
  wrong size, a trap stub, a crash) happen on a game thread's fiber of 256 KiB or in a signal handler, where Cocoa's
  alert loop has no promise to run, so they are in the log alone.
- **The bundle identifier is a placeholder**, `com.example.pokebw2-native.<program>`, in the CMake cache variable
  `PORT_MACOS_BUNDLE_ID_PREFIX`, which port/README.md says to change.
- **The versions come from git** at each build of the target: the commit count as `CFBundleVersion` and in
  `CFBundleShortVersionString` (`0.0.N`), and `git describe` in `PortSourceRevision`.
- `port/tools/package_macos.sh` makes a compressed HFS+ disk image with the app and a link to `/Applications`, and
  checks it: `hdiutil verify`, the app's signature inside the mounted image, and no ROM, save, NARC or SDAT file in it.
  It refuses to write the image where git would track it, in any work tree, this repository's or another's (the clone
  a worktree sits in). Nothing is uploaded; CI only builds and verifies the app on its macOS runner.

## Alternatives

- **`MACOSX_BUNDLE` on the program target:** moves the program to `pokeblack2.app/Contents/MacOS/pokeblack2`, which
  every test, replay and oracle command would have to follow, and CMake's bundle step doesn't embed imported
  libraries without an install step (`BundleUtilities` or `install(RUNTIME_DEPENDENCY_SET)`).
- **SDL3 always static in the app:** needs the pinned release built (downloaded and compiled) even where Homebrew's is
  installed, which CI and local builds don't do today. Configuring with `-DCMAKE_DISABLE_FIND_PACKAGE_SDL3=ON` and a
  `CMAKE_OSX_DEPLOYMENT_TARGET` gives that build already; the script then has nothing to embed.
- **A first-launch ROM picker** (`SDL_ShowOpenFileDialog`, PLAN.md §4.3's "first-launch ROM setup"): the right fix for
  players who don't use the terminal, but it changes how the program finds the ROM; it belongs to the settings and
  paths work, not to packaging.
- **`xattr -dr com.apple.quarantine` in the instructions:** the Gatekeeper bypass ADR 0003 turned down; the README gives
  "Open Anyway" instead.

## Consequences

- An app built with Homebrew's SDL3 needs the macOS Homebrew built SDL3 for (26 here); an app for older Macs needs the
  static SDL3 and a deployment target, which hasn't been tried yet.
- The app holds the absolute path of the checkout it was built from (the ROM's development default) and the build's
  object paths in its debug map; a build meant for other people should be made from a neutral path, and stripped, when
  that day comes.
- A Finder launch without a ROM shows the reason in an alert and quits; the ROM picker above would let the player
  choose the ROM there instead. Stops during the game close the app with the reason in the log only.
- `port/tests/app/test_make_app.sh` builds small programs whose libraries are named every way dyld allows and checks the
  app make_app.sh makes from each, moved away from the originals, and its refusals; `test_message_log.sh` runs the
  program from a stand-in app with stderr on `/dev/null`; `test_package_macos.sh` checks the image path's refusals.
