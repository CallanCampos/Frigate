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
    portamento_time 12
    portamento 1
    portamento_key A2
    note A2, 127, 84
    release 100
    end
track_01:
    tempo 75
    program 0
    attack 45
    rest 4
    mod_depth 30
    mod_range 2
    mod_speed 3
    mod_type 0
    note C4, 80, 80
    release 100
    end
    .byte 0xff
