track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_0012
    open_track 1, track_01
    open_track 2, track_02
track_00_0012:
    attack 40
    program 0
    portamento_time 24
    portamento 1
    portamento_key C2
    note C3, 127, 0
    end
track_01:
    attack 30
    rest 28
    mod_depth 28
    mod_range 1
    mod_speed 10
    mod_type 2
    portamento_time 24
    portamento 1
    portamento_key C3
    program 0
    note C4, 97, 0
    end
track_02:
    attack 52
    mod_depth 13
    mod_range 1
    mod_speed 10
    mod_type 1
    program 1
    release 90
    note A3, 115, 70
    end
    .byte 0xff, 0x00
