track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    random pan 40, 88
    note_wait 1
    program 0
    random volume 84, 127
    bend_range 2
    random pitch_bend 0, 127
    note C4, 127, 0
    random rest 30, 60
    jump track_00_000d
    .byte 0xff
track_01:
    random pan 32, 96
    note_wait 1
    program 0
    random volume 84, 127
    bend_range 2
    random pitch_bend -127, -10
    note A3, 127, 0
    random rest 20, 60
    jump track_01
    .byte 0xff, 0xff, 0x00, 0x00
