track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    program 0
    attack 56
    mod_depth 12
    mod_range 1
    mod_speed 2
    mod_type 1
    portamento_time 26
    portamento 1
    portamento_key E3
    note F3, 127, 120
    release 115
    end
track_01:
    tempo 75
    program 0
    attack 45
    rest 2
    mod_depth 30
    mod_range 3
    mod_speed 4
    mod_type 0
    note As3, 120, 120
    release 116
    end
    .byte 0xff
