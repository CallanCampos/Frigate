track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    rest 18
    program 0
    note E3, 110, 0
    end
track_01:
    tempo 75
    program 1
    note C4, 127, 0
    rest 10
    program 2
    note C4, 118, 0
    end
    .byte 0xff, 0x00, 0x00, 0x00
