track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_0012
    open_track 1, track_01
    open_track 2, track_02
track_00_0012:
    tempo 75
    program 0
    attack 56
    mod_depth 12
    mod_range 1
    mod_speed 2
    mod_type 1
    portamento_time 20
    portamento 1
    portamento_key F3
    note G3, 127, 120
    release 120
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
    note C4, 120, 120
    release 120
    end
track_02:
    tempo 75
    program 1
    note E2, 1, 120
    note Cs3, 127, 0
    end
    .byte 0xff, 0x00, 0x00, 0x00
