track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    program 0
    note D3, 127, 9
    mod_depth 13
    mod_range 2
    mod_speed 13
    mod_type 1
    note E4, 127, 0
    end
track_01:
    program 0
    attack 20
    decay 127
    sustain 127
    release 115
    mod_depth 28
    mod_range 1
    mod_speed 7
    mod_type 1
    note C4, 94, 0
    end
    .byte 0xff, 0x00, 0x00, 0x00
