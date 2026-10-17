track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_0012
    open_track 1, track_01
    open_track 2, track_02
track_00_0012:
    tempo 75
    program 0
    attack 40
    mod_depth 22
    mod_range 2
    mod_speed 3
    mod_type 1
    portamento_time 20
    portamento 1
    portamento_key E3
    release 94
    note G3, 110, 260
    end
track_01:
    tempo 75
    program 1
    pan 14
    attack 45
    mod_depth 30
    mod_range 3
    mod_speed 4
    mod_type 0
    release 95
    note C3, 120, 260
    end
track_02:
    tempo 75
    program 1
    pan 114
    attack 45
    rest 2
    mod_depth 30
    mod_range 3
    mod_speed 5
    mod_type 0
    release 95
    note C3, 120, 260
    end
    .byte 0xff, 0x00, 0x00, 0x00
