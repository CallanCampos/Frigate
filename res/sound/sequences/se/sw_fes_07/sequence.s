track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    program 0
    note C3, 121, 0
    end
track_01:
    tempo 75
    rest 5
    attack 105
    decay 60
    sustain 40
    release 85
    program 1
    mod_depth 20
    mod_range 1
    mod_speed 18
    mod_type 2
    note C4, 110, 76
    end
    .byte 0xff, 0x00, 0x00
