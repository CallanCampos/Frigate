track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    pan 34
    program 0
    attack 110
    release 84
    note A3, 100, 0
    end
track_01:
    tempo 100
    rest 1
    pan 94
    program 0
    attack 110
    release 84
    note A3, 100, 0
    end
    .byte 0xff, 0x00, 0x00
