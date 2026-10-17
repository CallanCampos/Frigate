track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    note G3, 127, 0
    end
track_01:
    program 0
    rest 21
    mod_depth 28
    mod_range 3
    mod_speed 10
    mod_type 2
    note G3, 70, 0
    end
    .byte 0xff
