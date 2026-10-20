track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    tempo 75
    mod_depth 20
    mod_range 1
    mod_speed 8
    mod_type 1
    release 118
    note As4, 82, 50
    end
track_01:
    program 1
    tempo 75
    note Cs2, 1, 76
    note C4, 127, 0
    end
    .byte 0xff, 0x00, 0x00, 0x00
