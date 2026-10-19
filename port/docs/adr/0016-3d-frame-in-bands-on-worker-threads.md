# 0016: The 3D frame drawn in bands on worker threads, the game on one

- **Status:** proposed
- **Date:** 2026-10-19
- **Plan:** §4.6 (renderers), §7 phase 5 (60 fps with headroom), the boundary rule of one host thread
  ([ARCHITECTURE.md](../ARCHITECTURE.md#the-rules-at-the-boundaries))
- **Upstream:** `2825ea75f`

## Context

Frigate runs everything on one host thread: the game's C on fibers, the platform layer, the frame hooks and the
renderers. That is what makes a run deterministic (PLAN.md §6). A measurement of every route on an M2 MacBook Air
(STATUS.md, 2026-10-19) found that the frames are bound by the CPU 3D renderer, `port/gfx/ref/ds3d.c`: 71% of the
title screen's profile samples, about 3 ms of its 4.3 ms frame, 2.7 of New Game's 3.8 ms flight, and every pixel of
every polygon through a perspective divide. Windowed and paced, the core slows down while the program sleeps through
the rest of a frame, and the same 3D frames cost 5.9 to 6.1 ms, about 2 ms under half the DS's 16.7 ms; the x86_64
slice under Rosetta takes 1.7 times as long. The compiler had nothing left to give: -O3, `-mcpu=apple-m1`, PGO and the
hoisting of the pixel loop's invariants all measured within noise.

The renderer keeps no state from one polygon's line to another line except the shadow stencil, which it clears per
pixel, and edge marking, which reads the depths and polygon IDs of a pixel's neighbours above and below once every
polygon is drawn. So the lines of a frame can be drawn apart, as long as each line sees the polygons in the same
order.

## Decision

- **Bands of lines.** `port_ds3d_render` sorts the polygons once, then runs one job of the renderers' workers
  (`port/gfx/workers.c`): each part draws a band, the blocks of 8 lines dealt round the parts in turn (24 blocks;
  interleaved, so that a scene's busy lines spread over every part), and draws every polygon, in the sorted order, on
  its own lines only: the rear plane, the stencil's clears, the polygons, then, once every part has drawn its polygons
  (a sync of the job's parts, only when edge marking is on), edge marking, fog, anti-aliasing and the picture's
  pixels. A part keeps every polygon's edge walking on every line, so each line it draws is the line one thread would
  draw. Edge marking now marks a pixel as it finds it: its test reads only the depths and IDs, which it doesn't write.
- **The workers.** As many parts as the host has performance cores (`port_host_performance_cores`: macOS's
  `hw.perflevel0.physicalcpu`, every online processor elsewhere), at most 8, the game's thread being part 0;
  `--render-threads N` (1 to 8) overrides it, and 1 draws on the game's thread alone. The workers are started once at
  boot, at macOS's user-interactive QoS, with the asynchronous signals blocked so that the watchdog's SIGALRM reaches
  the game's thread (`port/hal/host`'s threads and signals, pthreads or Win32). A waiting thread spins briefly (20 µs
  for the next job, 0.5 ms for the other parts) before it sleeps on a host signal (a count, a mutex and a condition
  variable).
- **Only renderer code runs on the workers**, on memory the parts share out by lines. The game, the platform layer, the
  frame and scanline hooks, the geometry engine and the 2D renderer stay on the game's one thread, so a run stays
  deterministic, and the picture is the same however many parts draw it.
- **Proof:** `g3d_ds3d` runs every hand-worked scene in one band and in three; `g3d_ds3d_bands` draws random scenes
  of 2,000 textured triangles (anti-aliasing, blending, edge marking, fog, Z and W, the clear image) in 2 to 8 bands
  against one, pixel for pixel, depths, IDs and fog flags included; and the routes' `frames.tsv` (both screens'
  hashes at every frame) are byte for byte the one-thread program's.

## Alternatives

- **Contiguous bands** (a quarter of the screen each): as exact, and measured so, but a 3D scene's polygons gather on
  part of the screen (the title's Kyurem is in its middle), so the parts are unbalanced.
- **Polygons split among threads, then merged:** the depth test, translucency and anti-aliasing's second layer make a
  pixel depend on the order of every polygon over it, so a merge would have to redo that order; bands keep it whole.
- **Drawing frame N's 3D while the game runs frame N + 1:** the game changes texture VRAM at the VBlank, and display
  capture and the depth readers read the frame, so the picture could change.
- **Drawing the 3D lazily, or not when BG0 doesn't show it:** VRAM can change between lines, and capture reads the
  3D layer whatever BG0 shows.
- **A GPU renderer** (PLAN.md §4.6) is the long-term answer and stays planned; the CPU renderer stays the reference it
  is compared with, and bands keep it fast enough meanwhile.
- **A thread pool library or OpenMP:** a dependency for 150 lines, and OpenMP's runtime isn't in every toolchain the
  build uses (Apple clang has none).

## Consequences

- The 3D frame costs about a third as much: the title's 3.0 ms to 1.0, New Game's flight 2.7 to 0.8, Juniper's intro
  0.21 to 0.08 on the M2 in four bands, which takes the title's frames from 4.25 to 2.24 ms and the flight's from 3.79
  to 1.93 (STATUS.md, 2026-10-19). Paced, the gain is smaller, since the game's thread's other work runs on a core
  that has clocked down while it slept: the title's frame takes 6.61 ms on one thread and 5.18 on four (SDL's dummy
  video driver). Under heavy load the workers wake later and the gain shrinks; it never changes the picture.
- The program uses up to 8 threads while it draws 3D. `ctest -j4` runs four programs at once, each with its own
  workers; the timing tests run alone (`RUN_SERIAL`).
- The renderer must keep every pixel a function of its own line and the polygons' order. A new pass that reads
  another line (a blur, a GPU-style post pass) goes after a `port_workers_sync`, and anything that keeps state across
  lines (as the stencil's clear does) must be cleared per band. `g3d_ds3d_bands` is the guard.
- The sanitized build runs the workers too; ThreadSanitizer isn't used, since the parts share no memory but the job's
  read-only inputs and the signals.
- Revisit when the GPU renderer replaces the CPU one on the screen, or if a host shows the workers' wake-ups costing
  more than they save (then `--render-threads 1`, or fewer parts by default).
