track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    program 0
    pan 14
    attack 45
    mod_depth 30
    mod_range 3
    mod_speed 4
    mod_type 0
    release 98
    note C4, 110, 90
    end
track_01:
    tempo 75
    program 0
    pan 114
    attack 45
    rest 2
    mod_depth 30
    mod_range 3
    mod_speed 5
    mod_type 0
    release 98
    note C4, 110, 90
    end
    .byte 0xff, 0x00, 0x00
