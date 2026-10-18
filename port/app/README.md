# port/app/

The program itself, `frigate-black2` or `frigate-white2`: its entry point and options, its window and input, and what
it writes for the tests. It drives the game, which runs on the platform layer in
[../hal](../README.md#directory-map), and draws what the renderers in [../gfx](../README.md#directory-map) make of the
DS's screens. Everything here runs on the program's one thread. The build rules are in
[../CMakeLists.txt](../CMakeLists.txt) and [../cmake/app.cmake](../cmake/app.cmake); the tests are in
[../tests/app](../tests/app).

| File | What it does | Part of the game it touches |
| --- | --- | --- |
| [main.c](main.c) | The entry point: the options and `--help`, the window, the platform layer, and the game's `main()` on the OS core; the frame hook, which takes the input, presents each frame and paces to the DS's 59.8261 frames a second | The whole game: it starts it and keeps its time |
| [present.c](present.c), [present.h](present.h) | The window: both screens stacked in one SDL window, the keyboard, gamepads and mouse as the DS's buttons and touch screen, and PNG screenshots | The DS's two screens, its buttons and its touch screen |
| [gamepad.c](gamepad.c), [gamepad.h](gamepad.h) | Gamepads as the DS's buttons, by position, with the table of their controls and the pad that drives | The DS's buttons |
| [window_icon.c](window_icon.c), [window_icon.h](window_icon.h) | The window's icon on Linux and Windows, Frigate's for the version, from a bitmap the build makes of the icon's master ([../assets/branding](../assets/branding/README.md)) | None: the program's look in the taskbar |
| [message_log.c](message_log.c), [message_log.h](message_log.h) | An app opened from the Finder: its messages to a log file in `~/Library/Logs/Frigate`, and an alert for a stop the player can act on | None: the program's messages |
| [replay.c](replay.c), [replay.h](replay.h) | Input replays (`--input`): an input file's buttons, lid and touch on each frame, in place of the window's | The DS's buttons, lid and touch screen, as the game reads them |
| [run_dir.c](run_dir.c), [run_dir.h](run_dir.h) | The run directory (`--run-dir`) for comparisons with the melonDS oracle: frame rows, screens and memory dumps | Any of the game's memory, read for the comparisons |
| [symbols.c](symbols.c), [symbols.h](symbols.h) | The program's globals by name, for the run directory's memory dumps | The game's globals |
| [macos/](macos) | The macOS app: [make_app.sh](macos/make_app.sh) builds and signs it, [make_icns.sh](macos/make_icns.sh) makes its icon, [Info.plist.in](macos/Info.plist.in) is its Info.plist | None: the program as a macOS app |
