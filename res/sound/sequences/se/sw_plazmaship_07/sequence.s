track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 1
    note F3, 127, 0
    end
track_01:
    program 1
    tempo 74
    note F5, 80, 0
    end
    .byte 0xff, 0x00, 0x00, 0x00
