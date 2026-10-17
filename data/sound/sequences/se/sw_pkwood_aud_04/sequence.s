track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    attack 100
    pan 14
    program 0
    note F3, 127, 0
    end
track_01:
    tempo 75
    attack 105
    rest 3
    pan 114
    program 0
    note F3, 127, 0
    end
    .byte 0xff, 0x00
