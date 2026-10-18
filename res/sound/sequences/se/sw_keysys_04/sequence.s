track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_0012
    open_track 1, track_01
    open_track 2, track_02
track_00_0012:
    program 1
    note C4, 127, 0
    end
track_01:
    rest 16
    program 1
    mod_depth 43
    mod_range 2
    mod_speed 5
    mod_type 2
    note C4, 54, 0
    end
track_02:
    rest 32
    program 1
    mod_depth 56
    mod_range 2
    mod_speed 7
    mod_type 2
    note C4, 24, 0
    end
    .byte 0xff, 0x00, 0x00, 0x00
