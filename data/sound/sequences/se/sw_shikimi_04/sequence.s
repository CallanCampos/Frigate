track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    rest 40
    program 0
    note D2, 127, 9
    mod_depth 13
    mod_range 2
    mod_speed 13
    mod_type 1
    note E3, 127, 0
    end
track_01:
    tempo 75
    rest 40
    program 0
    attack 20
    decay 127
    sustain 127
    release 115
    mod_depth 28
    mod_range 1
    mod_speed 7
    mod_type 1
    note G3, 94, 0
    end
    .byte 0xff, 0x00
