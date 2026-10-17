track_00:
    alloc_tracks 0x000f
    open_track 0, track_00_0017
    open_track 1, track_01
    open_track 2, track_02
    open_track 3, track_03
track_00_0017:
    program 0
    portamento_key C3
    portamento_time 30
    portamento 1
    mod_depth 13
    mod_range 7
    mod_speed 8
    mod_type 1
    mod_delay 100
    attack 14
    release 60
    note G3, 127, 100
    end
track_01:
    program 0
    pan 54
    portamento_key C3
    portamento_time 0
    portamento 1
    mod_depth 26
    mod_range 2
    mod_speed 5
    mod_type 2
    attack 10
    release 80
    note B3, 127, 240
    end
track_02:
    tempo 300
    rest 560
    program 1
    attack 10
    release 86
    note C4, 125, 0
    end
track_03:
    tempo 300
    rest 585
    mod_depth 55
    mod_range 5
    mod_speed 9
    mod_type 2
    program 1
    attack 10
    release 86
    note C4, 100, 0
    end
    .byte 0xff, 0x00, 0x00
