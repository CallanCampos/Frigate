# port/audio/

The host's sound output: the DS's speakers. The game's sound code (NitroSystem's players and NitroSDK's ARM9 SND
library, driven by `src/system/snd_sys.c`) compiles unchanged and sends its commands to the ARM7's sound driver, which
the platform layer has in host C ([../hal/snd7](../hal/README.md#the-modules)). The driver's sound unit makes the
samples in step with the game, on the game's thread, as the virtual clock passes them (at each VBlank, at each word
the ARM9 sends the driver, and at the driver's alarms), and at each VBlank hands that frame's samples to this module,
which plays them on the default device and can write them to a WAV file.

| File | What it does | Game functionality that depends on it | Tests |
| --- | --- | --- | --- |
| [audio.c](audio.c), [audio.h](audio.h) | The default playback device through an SDL3 audio stream at the sound unit's 32,728 Hz in stereo, with the stream's speed nudged within half a percent to keep about a tenth of a second queued; `--audio-dump PATH` writes the same samples to a WAV file, with or without a window | Music, sound effects and cries, as the player hears them | `snd_audio_dump`; the other `snd_*` tests link it ([../cmake/sound.cmake](../cmake/sound.cmake)) |

The microphone's input (PLAN.md §4.3) isn't here yet: the platform layer's microphone hears silence
([../hal/mic/mic.c](../hal/mic/mic.c)).
