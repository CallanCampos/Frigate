track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    mod_depth 25
    mod_range 1
    mod_speed 3
    mod_type 1
    attack 54
    decay 2
    sustain 89
    release 90
    note F4, 127, 52
    end
track_01:
    rest 2
    program 0
    attack 54
    decay 2
    sustain 89
    release 90
    mod_depth 32
    mod_range 1
    mod_speed 8
    mod_type 2
    note C5, 127, 50
    end
    .byte 0xff
