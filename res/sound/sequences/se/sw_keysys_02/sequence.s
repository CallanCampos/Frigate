track_00:
    alloc_tracks 0x0007
    open_track 0, track_00_0012
    open_track 1, track_01
    open_track 2, track_02
track_00_0012:
    program 1
    attack 8
    decay 60
    sustain 40
    release 85
    mod_depth 38
    mod_range 1
    mod_speed 5
    mod_type 2
    note F4, 127, 270
    end
track_01:
    program 1
    rest 10
    attack 3
    decay 60
    sustain 40
    release 85
    mod_depth 20
    mod_range 2
    mod_speed 8
    mod_type 2
    note As4, 74, 260
    end
track_02:
    program 1
    attack 8
    decay 60
    sustain 40
    release 85
    mod_depth 18
    mod_range 1
    mod_speed 5
    mod_type 1
    note C4, 127, 270
    end
    .byte 0xff, 0x00, 0x00
