track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    attack 103
    program 0
    note C4, 127, 0
    end
track_01:
    program 0
    attack 92
    tempo 75
    rest 12
    mod_depth 25
    mod_range 1
    mod_speed 15
    mod_type 2
    program 0
    note C4, 75, 0
    end
    .byte 0xff
