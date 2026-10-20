track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    tempo 75
    note Gs3, 127, 0
    end
track_01:
    program 0
    tempo 75
    rest 8
    mod_depth 45
    mod_range 1
    mod_speed 7
    mod_type 2
    note C4, 120, 0
    end
    .byte 0xff, 0x00, 0x00
