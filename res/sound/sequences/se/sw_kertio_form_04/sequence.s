track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_0012
    open_track 1, track_01
    open_track 2, track_02
track_00_0012:
    program 0
    tempo 75
    note C4, 127, 0
    end
track_01:
    program 1
    tempo 75
    rest 22
    note F3, 127, 0
    end
track_02:
    program 1
    tempo 75
    mod_depth 38
    mod_range 3
    mod_speed 11
    mod_type 2
    rest 23
    note A3, 127, 0
    end
    .byte 0xff, 0x00, 0x00
