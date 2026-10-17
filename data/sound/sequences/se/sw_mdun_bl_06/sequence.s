track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    note C3, 127, 0
    end
track_01:
    rest 18
    mod_depth 42
    mod_range 2
    mod_speed 5
    mod_type 2
    program 0
    note G3, 76, 0
    end
    .byte 0xff
