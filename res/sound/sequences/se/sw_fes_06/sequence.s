track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    program 0
    note C4, 127, 0
    end
track_01:
    tempo 75
    rest 10
    program 0
    mod_depth 20
    mod_range 1
    mod_speed 18
    mod_type 2
    note C4, 48, 0
    end
    .byte 0xff, 0x00, 0x00
