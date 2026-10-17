track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_0012
    open_track 1, track_01
    open_track 2, track_02
track_00_0012:
    pan 107
    program 0
    mod_depth 25
    mod_range 1
    mod_speed 4
    mod_type 1
    attack 28
    decay 120
    sustain 89
    release 105
    note C4, 88, 32
    end
track_01:
    rest 2
    pan 20
    program 0
    mod_depth 25
    mod_range 2
    mod_speed 5
    mod_type 1
    attack 28
    decay 120
    sustain 89
    release 105
    note C4, 88, 32
    end
track_02:
    program 0
    mod_depth 25
    mod_range 2
    mod_speed 18
    mod_type 2
    attack 32
    decay 120
    sustain 89
    release 105
    note C6, 50, 32
    end
    .byte 0xff, 0x00
