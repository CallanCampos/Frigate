track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    rest 48
    program 0
    mod_depth 13
    mod_range 2
    mod_speed 13
    mod_type 1
    note D4, 127, 15
    note B4, 55, 0
    end
track_01:
    program 1
    mod_depth 28
    mod_range 1
    mod_speed 7
    mod_type 1
    release 100
    note A2, 90, 4
    release 100
    note Ds3, 100, 18
    end
    .byte 0xff, 0x00, 0x00, 0x00
