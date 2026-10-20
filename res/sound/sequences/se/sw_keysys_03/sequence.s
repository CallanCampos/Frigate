track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 1
    note Gs3, 127, 0
    end
track_01:
    rest 9
    program 1
    note Cs4, 127, 0
    end
    .byte 0xff
