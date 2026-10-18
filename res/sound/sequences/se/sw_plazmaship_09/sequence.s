track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    program 0
    note C4, 127, 0
    end
track_01:
    program 0
    tempo 75
    rest 63
    program 0
    note C4, 127, 0
    end
    .byte 0xff
