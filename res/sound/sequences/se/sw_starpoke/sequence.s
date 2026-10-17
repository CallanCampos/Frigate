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
    program 0
    tempo 75
    rest 12
    mod_depth 17
    mod_range 2
    mod_speed 10
    mod_type 1
    note F4, 68, 0
    end
track_02:
    program 0
    tempo 75
    rest 25
    mod_depth 17
    mod_range 2
    mod_speed 10
    mod_type 1
    note C5, 60, 0
    end
    .byte 0xff, 0x00, 0x00
