# 0009: Gamepads by position, one pad at a time

- **Status:** accepted
- **Date:** 2026-10-17
- **Plan:** §4.5 (input), §7 phase 5 (controller support)
- **Upstream:** `615939e25`

## Context

The DS has A on the right of its diamond of face buttons, B at the bottom, X at the top and Y on the left: a Nintendo
pad's layout. SDL3's gamepad API names buttons by position (`SDL_GAMEPAD_BUTTON_SOUTH` is the bottom one whatever it
says on it) and reports each pad's labels separately (`SDL_GetGamepadButtonLabel`): an Xbox pad has A at the bottom,
a PlayStation pad has shapes and no letters. Phase 1 opened the first pad only, with a fixed table in
`port/app/present.c`. Phase 5 asks for several pads, pads that come and go, the left stick as the D-pad with a dead
zone, and a mapping a settings file can replace later.

## Decision

- **By position by default.** `port_gamepad_default_map` (`port/app/gamepad.c`) puts the DS's A, B, X and Y on the
  right, bottom, top and left face buttons of every pad, so the hands do what they do on a DS (confirm with the right
  button, cancel with the bottom one) on any pad. A binding can name a label instead (`PortGamepadBinding.label`),
  resolved per pad from the labels SDL reports, with its position as the fallback for a pad without that label; a
  settings file can choose that. Each pad's log line names its DS buttons by its own labels ("DS A, B, X, Y on its B,
  A, Y, X" on an Xbox pad).
- **One table.** A `PortGamepadMap` holds the bindings, whether the left stick is the D-pad, and its dead zone;
  `port_gamepads_set_map` replaces it for every pad. The keyboard keeps its own table in `present.c`, and both work at
  once (their buttons are ORed).
- **The stick:** a circular dead zone of half the travel (SDL says a stick at rest reads within about 8000 of 32767),
  then eight sectors of 45 degrees, computed in integers. A per-axis threshold would need a longer push on diagonals.
  Both edges have hysteresis: once the stick presses, it lets go only back inside seven eighths of the dead zone, and
  it keeps its direction or diagonal until it leans about 4 degrees past its sector's edge (the widened edges are
  where the tangent is 1/2, 1/3, 2 and 3, so the test stays in integers). A stick held still at an edge reads a few
  hundred units of noise; without the margins, each wobble across the edge was a new press, which the game's key
  handling (`src/gfl/key.c`) takes as a new button press and a menu cursor as another step.
- **One pad drives at a time.** Every connected pad is open, up to 8; one connected beyond them waits and is opened
  when one of them is disconnected. The pad that drives keeps driving while it holds anything; once it holds nothing,
  the first pad to press something new (a button it didn't hold at the last poll) takes over. A new press, not a held
  one, takes over, so a button held on another pad doesn't take the game from the one in use once it has pressed; and
  while that pad is connected, a stick's drift doesn't take the game from it either: a stick resting near the dead
  zone's edge (a pad on a desk, a worn stick) presses once and holds that press, so a stick takes over only when it is
  pushed beyond halfway from the dead zone's edge to the end of its travel. A drifting stick can still start a pad when
  none drives. What it doesn't cover: a pad whose button is stuck presses once when it connects, and takes the game if the
  pad in use holds nothing at that moment; holding that press for good, it keeps the game until it is disconnected.
- The mapping, the dead zone and its hysteresis, and the choice of the pad are plain functions;
  `port/tests/app/test_gamepad.c` checks them with hand-worked values and drives SDL's virtual joysticks as pads that
  come and go (a desk pad whose stick reads noise at the dead zone's edge, nine pads at once), with SDL's drivers for
  real pads turned off.

## Alternatives

- **By label by default** (the DS's A on the button labelled A): an Xbox pad's A is at the bottom, so confirm and
  cancel would swap places against the DS's, and PlayStation pads would need a rule of their own anyway. Kept as an
  option of the table.
- **Every pad at once** (ORing all pads): two people holding pads, or a pad on a desk with a drifting stick, would
  press buttons together. **The first pad connected drives until it is disconnected:** a second pad would do nothing
  while the first stays paired, which surprises a player who picks up the other one.
- **SDL's mapping string format as the settings file:** it maps devices to SDL's buttons, not SDL's buttons to the
  DS's, so it answers a different question; SDL already applies it underneath.

## Consequences

- A settings file (phase 6, PLAN.md §5) only has to fill a `PortGamepadMap` and call `port_gamepads_set_map`.
- The right stick and the triggers, which are axes, do nothing: a map binds buttons and the left stick only. The
  paddles and the guide button do nothing until a map binds them, the touch screen still needs the mouse, and the DS's
  lid has no control.
- Revisit if two-pad features (local multiplayer through two port instances, phase 7) need pads assigned to players,
  or if broken pads (a stuck button, a stick resting far out) turn out common enough to need a calibration at connect
  or a rule against a press that never lets go.
