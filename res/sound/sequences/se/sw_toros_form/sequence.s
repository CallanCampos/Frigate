track_00:
    alloc_tracks 0x000f
    open_track 0, track_00_0017
    open_track 1, track_01
    open_track 2, track_02
    open_track 3, track_03
track_00_0017:
    tempo 75
    program 0
    rest 8
    mod_depth 30
    mod_range 1
    mod_speed 8
    mod_type 2
    attack 42
    decay 78
    sustain 8
    release 115
    note C6, 88, 72
    end
track_01:
    tempo 75
    program 1
    attack 15
    decay 22
    sustain 28
    release 100
    mod_depth 42
    mod_range 2
    mod_speed 6
    mod_type 2
    mod_delay 20
    note F3, 127, 0
    end
track_02:
    tempo 75
    attack 15
    decay 22
    sustain 28
    release 100
    program 1
    note C4, 100, 0
    end
track_03:
    tempo 75
    attack 20
    decay 78
    sustain 8
    release 115
    program 0
    note C5, 60, 80
    end
    .byte 0xff, 0x00, 0x00, 0x00
