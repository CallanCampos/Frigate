# 0010: The macOS app as a signed copy of the program

- **Status:** accepted
- **Date:** 2026-10-17
- **Plan:** D11, §7 phase 5 (an ad-hoc signed `.app` in a DMG), after
  [ADR 0003](0003-ad-hoc-signing-no-apple-developer-account.md)
- **Upstream:** `615939e25`

## Context

Phase 5 asks for the program as a macOS app, signed ad hoc, in a disk image, with no ROM or game data in it. The
program the host build links (`build/port/pokeblack2`) is what every test, replay and oracle script runs by that path,
and it loads Homebrew's SDL3 (`/opt/homebrew/opt/sdl3/lib/libSDL3.0.dylib`, built for macOS 26 on the machine this was
written on) through an absolute install name and an rpath to `/opt/homebrew/lib`. CMake builds the pinned SDL3
statically only when no SDL3 3.4 is installed. The player's ROM is found from `--rom`, `POKEBW2_ROM` or the checkout's
`orig/` (`port/hal/rom`), and a Finder launch passes no arguments, no environment of the shell and `/` as the working
directory (tried on macOS 26.5 with a probe app: no `-psn_` argument either).

## Decision

- **A separate target copies the program into the app** (`<program>_app`, `port/cmake/macos_app.cmake`, run by
  `port/app/macos/make_app.sh`), instead of making the program target itself a bundle. The program stays where the
  tests and tools expect it, and the app is made only when asked for (`-DPORT_MACOS_APP=ON` makes it every build).
- **The libraries the program loads from outside macOS go in `Contents/Frameworks`** with `@rpath` install names, and
  the program's rpaths are replaced by `@executable_path/../Frameworks`, so the app never loads another copy of SDL3
  from the machine. `LSMinimumSystemVersion` is the newest `minos` among the binaries, so an older macOS refuses the
  app with a message instead of failing to load SDL3.
- **Signed ad hoc as a whole** with `codesign --force --deep --sign -` after the last change to any binary, checked
  with `codesign --verify --deep --strict`, without the hardened runtime (ADR 0003). The app is built next to its path
  and moved there only once it verifies.
- **The ROM is found as the program finds it.** A self-built app opens from the Finder with the checkout's `orig/`
  ROM; otherwise `open -a … --args --rom PATH` or `open --env POKEBW2_ROM=…` gives it (port/README.md).
- **The bundle identifier is a placeholder**, `com.example.pokebw2-native.<program>`, in the CMake cache variable
  `PORT_MACOS_BUNDLE_ID_PREFIX`, which port/README.md says to change.
- **The versions come from git** at each build of the target: the commit count as `CFBundleVersion` and in
  `CFBundleShortVersionString` (`0.0.N`), and `git describe` in `PortSourceRevision`.
- `port/tools/package_macos.sh` makes a compressed HFS+ disk image with the app and a link to `/Applications`, and
  checks it: `hdiutil verify`, the app's signature inside the mounted image, and no ROM, save, NARC or SDAT file in it.
  Nothing is uploaded; CI only builds and verifies the app on its macOS runner.

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
- A Finder launch without a ROM quits with its reason on stderr only, so nothing shows; the ROM picker above fixes it.
