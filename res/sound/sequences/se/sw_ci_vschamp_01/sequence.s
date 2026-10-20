track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    rest 1
    program 0
    pan 123
    pitch_bend 30
    note C3, 110, 0
    end
track_01:
    tempo 75
    program 0
    pan 4
    pitch_bend -10
    note C3, 110, 0
    end
    .byte 0xff, 0x00, 0x00
