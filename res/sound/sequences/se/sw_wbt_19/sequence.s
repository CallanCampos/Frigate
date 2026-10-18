track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    pan 30
    tempo 75
    release 100
    random pitch_bend -127, -20
    note B2, 127, 48
    end
track_01:
    program 0
    tempo 75
    pan 98
    rest 3
    release 100
    random pitch_bend 0, 64
    note B2, 127, 48
    end
    .byte 0xff, 0x00, 0x00
