track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_0012
    open_track 1, track_01
    open_track 2, track_02
track_00_0012:
    program 0
    mod_depth 25
    mod_range 1
    mod_speed 3
    mod_type 1
    attack 7
    decay 2
    sustain 89
    release 90
    portamento_time 0
    portamento 1
    portamento_key Ds3
    note G3, 117, 200
    end
track_01:
    rest 2
    program 0
    attack 7
    decay 2
    sustain 89
    release 90
    mod_depth 32
    mod_range 1
    mod_speed 8
    mod_type 2
    portamento_time 0
    portamento 1
    portamento_key Gs4
    note C5, 117, 200
    end
track_02:
    program 0
    mod_depth 25
    mod_range 2
    mod_speed 1
    mod_type 1
    attack 7
    decay 2
    sustain 89
    release 90
    portamento_time 0
    portamento 1
    portamento_key Gs2
    note C3, 106, 200
    end
    .byte 0xff
