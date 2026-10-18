track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    note B3, 127, 10
    rest 15
    note F4, 127, 10
    rest 43
    note B3, 127, 10
    rest 15
    note F4, 127, 10
    end
track_01:
    program 119
    note A1, 60, 10
    rest 15
    note E2, 60, 10
    rest 43
    note A1, 60, 10
    rest 15
    note E2, 60, 10
    end
    .byte 0xff
