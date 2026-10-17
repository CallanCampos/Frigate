track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    program 0
    pan 34
    pitch_bend -50
    tempo 75
    note A3, 127, 120
    end
track_01:
    tempo 75
    program 0
    pan 94
    pitch_bend 50
    tempo 75
    note A3, 127, 120
    end
    .byte 0xff, 0x00, 0x00
