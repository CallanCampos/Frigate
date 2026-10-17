track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    tempo 75
    pan 1
    pitch_bend -50
    note Ds3, 127, 0
    end
track_01:
    program 0
    tempo 60
    rest 1
    pan 126
    pitch_bend 50
    note Ds3, 127, 0
    end
    .byte 0xff, 0x00, 0x00
