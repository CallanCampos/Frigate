track_00:
    alloc_tracks 0x0003
    open_track 0, track_00_000d
    open_track 1, track_01
track_00_000d:
    tempo 75
    program 0
    volume 127
    note C4, 127, 47
    end
track_01:
    tempo 75
    rest 10
    mod_depth 20
    mod_range 2
    mod_speed 6
    mod_type 2
    program 0
    volume 127
    note C4, 73, 0
    end
    .byte 0xff, 0x00, 0x00
