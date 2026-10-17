track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    pan 7
    program 0
    attack 100
    release 84
    note F3, 127, 180
    end
track_01:
    tempo 75
    rest 3
    pan 120
    program 0
    attack 98
    release 84
    note F3, 127, 180
    end
    .byte 0xff, 0x00, 0x00, 0x00
